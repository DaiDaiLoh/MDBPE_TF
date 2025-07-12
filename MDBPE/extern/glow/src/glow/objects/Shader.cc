#include "Shader.hh"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

#include <glow/glow.hh>
#include <glow/util/DefaultShaderParser.hh>

#include <glow/common/log.hh>
#include <glow/common/profiling.hh>
#include <glow/common/shader_endings.hh>
#include <glow/common/str_utils.hh>
#include <glow/common/stream_readall.hh>

using namespace glow;

SharedShaderParser Shader::sParser = std::make_shared<DefaultShaderParser>();

Shader::Shader(GLenum shaderType) : mType(shaderType)
{
    checkValidGLOW();
    mObjectName = glCreateShader(mType);
}

Shader::~Shader()
{
    checkValidGLOW();
    glDeleteShader(mObjectName);
}

void Shader::compile()
{
    checkValidGLOW();
    GLOW_ACTION();
    glCompileShader(mObjectName);

    // check error log
    GLint logLength = 0;
    glGetShaderiv(mObjectName, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        std::string logOutput;
        {
            std::vector<GLchar> log;
            log.resize(static_cast<unsigned>(logLength) + 1u);
            glGetShaderInfoLog(mObjectName, logLength + 1, nullptr, log.data());
            logOutput = std::string(log.data());
        }

        error() << "Log for " << (mFileName.empty() ? to_string(this) : mFileName);
        error() << "Shader compiler: " << logOutput;
        mHasErrors = true;
        mCompiled = false; // is this true?

        // Check if NVidia-style line information is present in the log
        if (strncmp(logOutput.c_str(), "0(", 2) == 0)
        {
            auto it = logOutput.begin() + 2;
            while (isdigit(*it))
                ++it;

            auto const lineNumberString = std::string(logOutput.begin() + 2, it);
            auto const lineNumber = strtoul(lineNumberString.data(), nullptr, 0);
            if (lineNumber != 0)
            {
                // Found a line number
                unsigned readLineNumber = 1;
                std::stringstream sourceStream(mSources.at(0));

                for (; readLineNumber < lineNumber - 1; ++readLineNumber)
                    sourceStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::string line;
                line.reserve(256);

                error() << "Source provoking error:";

                for (auto num = (readLineNumber != 1) ? 0 : 1; std::getline(sourceStream, line, '\n') && num < 3; ++readLineNumber, ++num)
                    error() << readLineNumber << " | " << line;
            }
        }
    }
    else
    {
        mHasErrors = false;
        mCompiled = true;
    }
}

void Shader::reload()
{
    GLOW_ACTION();

    // reload source
    if (!mFileName.empty())
    {
        std::ifstream file(mFileName);
        if (!file.good())
        {
            warning() << "Skipping reload for " << mFileName << ", file not readable. " << to_string(this);
            return;
        }
        setSource(util::readall(file));
        mFileFlag->clear();
    }

    // compile
    compile();
}

void Shader::setSource(std::string_view source) { setSource({std::string(source)}); }

void Shader::setSource(cc::span<std::string const> sources)
{
    checkValidGLOW();
    mDependencyFlags.clear();
    mSources = std::vector(sources.begin(), sources.end());

    auto parsedSources = sParser ? sParser->parse(this, mSources) : mSources;

    std::vector<const GLchar*> srcs;
    srcs.resize(parsedSources.size());
    for (auto i = 0u; i < parsedSources.size(); ++i)
        srcs[i] = parsedSources[i].c_str();
    glShaderSource(mObjectName, static_cast<GLint>(srcs.size()), srcs.data(), nullptr);
}

SharedShader Shader::createFromSource(GLenum shaderType, std::string_view source)
{
    GLOW_ACTION();

    auto shader = std::make_shared<Shader>(shaderType);
    shader->setSource(source);
    shader->compile();
    return shader;
}

SharedShader Shader::createFromSource(GLenum shaderType, cc::span<std::string const> sources)
{
    GLOW_ACTION();

    auto shader = std::make_shared<Shader>(shaderType);
    shader->setSource(sources);
    shader->compile();
    return shader;
}

SharedShader Shader::createFromSource(GLenum shaderType, const unsigned char source[])
{
    std::stringstream ss;
    ss << source;
    return createFromSource(shaderType, ss.str());
}

#include <codecvt>
#include <locale>
#include <string>

SharedShader Shader::createFromFile(GLenum shaderType, std::string_view filename)
{
    GLOW_ACTION();

    auto shaderFile = std::ifstream(std::string(filename));
    if (!shaderFile.good())
    {
        error() << "Unable to read shader file " << filename;
        return nullptr;
    }

    auto shader = std::make_shared<Shader>(shaderType);
    shader->setObjectLabel(filename);
    shader->mFileName = filename;
    shader->mFileFlag = FileWatch::watchFile(filename);
    shader->setSource(util::readall(shaderFile));
    shader->compile();
    return shader;
}

bool Shader::newerVersionAvailable()
{
    // Main file changes
    if (mFileFlag && mFileFlag->isChanged())
        return true;

    // Dependency changes
    for (auto const& dep : mDependencyFlags)
        if (dep->isChanged())
            return true;

    return false;
}

void Shader::setShaderParser(const SharedShaderParser& parser) { sParser = parser; }

bool Shader::resolveFile(std::string_view name, GLenum& shaderType, std::string& content, std::string& realFileName)
{
    if (sParser)
        return sParser->resolveFile(name, shaderType, content, realFileName);

    // no parser: fallback

    // detect shader type
    auto found = false;
    for (auto const& kvp : glow::shaderEndingToType)
        if (util::endswith(name, kvp.first))
        {
            shaderType = kvp.second;
            found = true;
            break;
        }

    // direct match
    if (std::ifstream(std::string(name)).good())
    {
        if (!found) // has to be done here, because non-existant files should fail silently
        {
            error() << "Could not deduce shader type of Shader file " << name << ".";
            return false;
        }

        realFileName = name;
        std::ifstream fs(realFileName);
        content = util::readall(fs);
        return true;
    }

    return false;
}

void Shader::addDependency(std::string_view filename)
{
    // Multiple adds of the same dependency will just result in aliased shared_ptrs,
    // not worth storing and comparing std::strings per dependency
    mDependencyFlags.push_back(FileWatch::watchFile(filename));
}
