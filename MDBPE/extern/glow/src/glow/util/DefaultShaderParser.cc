#include "DefaultShaderParser.hh"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>

#include <glow/common/log.hh>
#include <glow/common/shader_endings.hh>
#include <glow/common/str_utils.hh>
#include <glow/common/stream_readall.hh>
#include <glow/glow.hh>

#include <glow/objects/Shader.hh>

using namespace glow;

std::vector<std::string> DefaultShaderParser::sIncludePaths = {"."};
std::unordered_map<std::string, std::string> DefaultShaderParser::sIncludeResources;
std::unordered_map<std::string, std::string> DefaultShaderParser::sVirtualFiles;
std::unordered_map<std::string, DefaultShaderParser::PragmaCallback> DefaultShaderParser::sPragmaCallbacks;

void DefaultShaderParser::parseWithInclude(Shader* shader,
                                           std::stringstream& parsedSrc,
                                           const std::string& source,
                                           int sourceIdx,
                                           int& nextSrcIdx,
                                           std::set<std::string>& includes,
                                           const std::string& relativePath,
                                           bool isVirtualFile)
{
    auto src = source;

    // replace \r by \n for consistent line endings
    replace(begin(src), end(src), '\r', '\n');

    // add __LINE__ and __FILE__ info to src
    parsedSrc << "#line 1 " << sourceIdx << "\n";

    std::stringstream ss(src);
    std::string line;
    auto lineIdx = 0u;
    while (getline(ss, line, '\n'))
    {
        ++lineIdx;

        // skip #version tags (and empty strings)
        if (line.find("#version ") == line.find_first_not_of(" \t"))
        {
            parsedSrc << "\n";
            continue;
        }

        // resolve custom pragmas
        if (line.find("#glow ") == line.find_first_not_of(" \t"))
        {
            std::vector<std::string> wordsInLine;
            std::istringstream wordStream(line);
            std::copy(std::istream_iterator<std::string>(wordStream), std::istream_iterator<std::string>(), std::back_inserter(wordsInLine));

            if (wordsInLine.size() == 3)
            {
                auto& keyword = wordsInLine[1];
                auto& instruction = wordsInLine[2];

                auto foundPragmaCallback = sPragmaCallbacks.find(keyword);
                if (foundPragmaCallback != sPragmaCallbacks.end())
                {
                    CallbackShaderWriter writer(parsedSrc);
                    if (!foundPragmaCallback->second(instruction, writer))
                    {
                        error() << "Instruction #glow " << keyword << " " << instruction << " not recognized by callback.";
                    }
                }
                else
                {
                    error() << "Keyword #glow " << keyword << " not registered.";
                }
            }
            else
            {
                error() << "Pragma `" << line << "' malformed.";
            }

            parsedSrc << "#line " << lineIdx << " " << sourceIdx << "\n";
            parsedSrc << "\n";
            continue;
        }

        // resolve includes
        if (line.find("#include ") == line.find_first_not_of(" \t"))
        {
            // perform include

            // absolute
            if (line.find('<') != std::string::npos)
            {
                auto p0 = line.find('<');
                auto p1 = line.find('>');

                if (p0 > p1 || p1 == std::string::npos)
                    error() << "Include `" << line << "' not recognized/invalid.";
                else
                {
                    auto path = line.substr(p0 + 1, p1 - p0 - 1);

                    auto virtualFileIt = sVirtualFiles.find(path);
                    if (virtualFileIt != sVirtualFiles.end())
                    {
                        // Found virtual file for path which overrides real files

                        auto incIdx = nextSrcIdx;
                        ++nextSrcIdx;
                        parseWithInclude(shader, parsedSrc, virtualFileIt->second, incIdx, nextSrcIdx, includes, "NONE_VIRTUAL", true);
                    }
                    else
                    {
                        // Resolve real file path

                        auto absfile = resolve(path, "");

                        if (absfile.empty())
                            error() << "Could not resolve `" << line << "'.";
                        else
                        {
                            if (includes.insert(absfile).second) // pragma once
                            {
                                addDependency(shader, absfile);
                                std::ifstream fs(absfile);
                                auto incIdx = nextSrcIdx;
                                ++nextSrcIdx;
                                parseWithInclude(shader, parsedSrc, util::readall(fs), incIdx, nextSrcIdx, includes, util::pathOf(absfile));
                            }
                        }
                    }
                }
            }
            // relative
            else if (line.find('"'))
            {
                auto p0 = line.find('"');
                auto p1 = line.rfind('"');

                if (p0 > p1 || p1 == std::string::npos)
                    error() << "Include `" << line << "' not recognized/invalid.";
                else if (isVirtualFile)
                    error() << "Relative include `" << line << "' invalid: File is virtual";
                else
                {
                    auto path = line.substr(p0 + 1, p1 - p0 - 1);
                    auto absfile = resolve(path, relativePath);

                    if (absfile.empty())
                        error() << "Could not resolve `" << line << "'.";
                    else
                    {
                        if (includes.insert(absfile).second) // pragma once
                        {
                            addDependency(shader, absfile);
                            std::ifstream fs(absfile);
                            auto incIdx = nextSrcIdx;
                            ++nextSrcIdx;
                            parseWithInclude(shader, parsedSrc, util::readall(fs), incIdx, nextSrcIdx, includes, util::pathOf(absfile));
                        }
                    }
                }
            }
            // internal
            else if (line.find(':') != std::string::npos)
            {
                auto p = line.find(':');
                auto file = line.substr(p + 1);
                while (!file.empty() && (file.back() == ' ' || file.back() == '\t' || file.back() == '\r' || file.back() == '\n'))
                    file.pop_back();

                if (sIncludeResources.count(file))
                {
                    auto const& directSrc = sIncludeResources[file];
                    if (includes.insert(file).second) // pragma once
                    {
                        auto incIdx = nextSrcIdx;
                        ++nextSrcIdx;
                        parseWithInclude(shader, parsedSrc, directSrc, incIdx, nextSrcIdx, includes, "<internal resource>");
                    }
                }
                else
                    error() << "Include `" << line << "' is not a registered resource (forgot to call DefaultShaderParser::addIncludeResource(...)).";
            }
            else
                error() << "Include `" << line << "' not recognized/invalid.";

            // resume file
            parsedSrc << "#line " << lineIdx << " " << sourceIdx << "\n";
            parsedSrc << "\n";
            continue;
        }

        parsedSrc << line << "\n";
    }
}

std::string DefaultShaderParser::resolve(const std::string& filename, const std::string& relPath)
{
    if (filename.empty())
    {
        error() << "Empty include path";
        return "";
    }

    if (filename[0] == '/' || filename[0] == '\\')
    {
        error() << "Filename `" << filename << "' is absolute. Not supported.";
        return "";
    }

    // check rel path
    if (!relPath.empty() && std::ifstream(relPath + "/" + filename).good())
        return relPath + "/" + filename;

    // check inc paths
    for (auto const& path : sIncludePaths)
        if (std::ifstream(path + "/" + filename).good())
            return path + "/" + filename;

    return "";
}

DefaultShaderParser::DefaultShaderParser() {}

void DefaultShaderParser::setIncludePaths(const std::vector<std::string>& paths) { sIncludePaths = paths; }

void DefaultShaderParser::addIncludePath(const std::string& path)
{
    for (auto const& p : sIncludePaths)
        if (p == path)
            return;

    sIncludePaths.push_back(path);
}

void DefaultShaderParser::addIncludeResource(const std::string& file, const std::string& content) { sIncludeResources[file] = content; }

void DefaultShaderParser::addVirtualFile(const std::string& path, const std::string& content) { sVirtualFiles[path] = content; }

void DefaultShaderParser::addVirtualFile(const std::string& path, const unsigned char content[])
{
    std::stringstream ss;
    ss << content;
    addVirtualFile(path, ss.str());
}

void DefaultShaderParser::registerCustomPragma(const std::string& keyword, const DefaultShaderParser::PragmaCallback& callback)
{
    auto foundCallback = sPragmaCallbacks.find(keyword);
    if (foundCallback != sPragmaCallbacks.end())
    {
        error() << "Shader pragma " << keyword << " is already registered";
        return;
    }

    sPragmaCallbacks.insert({keyword, callback});
}

std::vector<std::string> DefaultShaderParser::getIncludePaths() { return sIncludePaths; }

std::vector<std::string> DefaultShaderParser::getIncludeResources()
{
    std::vector<std::string> r;
    for (auto const& kvp : sIncludeResources)
        r.push_back(kvp.first);
    return r;
}

std::vector<std::string> DefaultShaderParser::parse(Shader* shader, const std::vector<std::string>& sources)
{
    // may be empty
    auto sFilename = shader->getFileName();
    auto relPath = util::pathOf(sFilename);
    if (!sFilename.empty() && relPath.empty())
        relPath = ".";

    std::set<std::string> includes;

    std::stringstream parsedSrc;
#ifdef GLOW_OPENGL_PROFILE_CORE
    parsedSrc << "#version " + std::to_string(glow::OGLVersion.total * 10) + " core\n";
#else
    parsedSrc << "#version " + std::to_string(glow::OGLVersion.total * 10) + "\n";
#endif

    int nextSrcIdx = int(sources.size());

    for (auto sIdx = 0u; sIdx < sources.size(); ++sIdx)
        parseWithInclude(shader, parsedSrc, sources[sIdx], int(sIdx), nextSrcIdx, includes, relPath);

    return {parsedSrc.str()};
}

bool DefaultShaderParser::resolveFile(std::string_view name, GLenum& shaderType, std::string& content, std::string& realFileName)
{
    if (name.empty())
        return false;

    // detect shader type
    auto found = false;
    for (auto const& kvp : glow::shaderEndingToType)
        if (util::endswith(name, kvp.first))
        {
            found = true;
            shaderType = kvp.second;
            break;
        }

    auto nameString = std::string(name);

    // virtual file match
    auto virtualFileIt = sVirtualFiles.find(nameString);
    if (virtualFileIt != sVirtualFiles.end())
    {
        realFileName = "";
        content = virtualFileIt->second;
        return true;
    }

    // direct match
    if (std::ifstream(nameString).good())
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

    // includes if not absolute
    if (name[0] != '/')
    {
        for (auto const& inc : sIncludePaths)
        {
            if (std::ifstream(inc + "/" + nameString).good())
            {
                if (!found) // has to be done here, because non-existant files should fail silently
                {
                    error() << "Could not deduce shader type of Shader file " << name << ".";
                    return false;
                }

                realFileName = inc + "/" + nameString;
                std::ifstream fs(realFileName);
                content = util::readall(fs);
                return true;
            }
        }
    }

    return false;
}

DefaultShaderParser::CallbackShaderWriter::CallbackShaderWriter(std::stringstream& stream) : mStream(stream) {}

void DefaultShaderParser::CallbackShaderWriter::emitText(const std::string& content) { mStream << content; }
