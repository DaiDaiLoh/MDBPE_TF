#include "Program.hh"

#include <typed-geometry/feature/assert.hh>

#include <chrono>
#include <fstream>

#include "AtomicCounterBuffer.hh"
#include "Framebuffer.hh"
#include "Shader.hh"
#include "ShaderStorageBuffer.hh"
#include "Texture.hh"
#include "UniformBuffer.hh"

#include <glow/callbacks.hh>
#include <glow/common/gl_to_string.hh>
#include <glow/common/gltypeinfo.hh>
#include <glow/common/ogl_typeinfo.hh>
#include <glow/common/runtime_assert.hh>
#include <glow/common/shader_endings.hh>
#include <glow/common/str_utils.hh>
#include <glow/common/thread_local.hh>
#include <glow/glow.hh>
#include <glow/limits.hh>
#include <glow/util/LocationMapping.hh>
#include <glow/util/UniformState.hh>

#include <glow/common/profiling.hh>

using namespace glow;

/// Currently used program
static GLOW_THREADLOCAL UsedProgram* sCurrentProgram = nullptr;

bool Program::sCheckShaderReloading = true;

bool Program::linkAndCheckErrors()
{
    checkValidGLOW();
    GLOW_ACTION();

    // link
    glLinkProgram(mObjectName);

    // check error log
    GLint logLength = 0;
    glGetProgramiv(mObjectName, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        std::vector<GLchar> log;
        log.resize(unsigned(logLength) + 1);
        glGetProgramInfoLog(mObjectName, logLength + 1, nullptr, log.data());

        error() << "Linker log for: " << to_string(this);
        for (auto const& s : mShader)
            error() << "  " << glShaderTypeToString(s->getType()) << ": " << (s->getFileName().empty() ? to_string(s.get()) : s->getFileName());
        error() << "Shader linker: " << log.data();
        return false;
    }

    return true;
}

void Program::restoreExtendedState()
{
    checkValidGLOW();

    // check transform feedback
    if (isConfiguredForTransformFeedback() && !mIsLinkedForTransformFeedback)
        link();

    // bind uniform buffer
    for (auto const& kvp : mUniformBuffers)
    {
        auto loc = mUniformBufferMapping.queryLocation(kvp.first);
        glBindBufferBase(GL_UNIFORM_BUFFER, GLuint(loc), kvp.second ? kvp.second->getObjectName() : 0);
    }

    // bind shader storage buffer
    for (auto const& kvp : mShaderStorageBuffers)
    {
        auto loc = mShaderStorageBufferMapping.queryLocation(kvp.first);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GLuint(loc), kvp.second ? kvp.second->getObjectName() : 0);
    }

    // bind atomic counters
    for (auto const& kvp : mAtomicCounterBuffers)
    {
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, GLuint(kvp.first), kvp.second ? kvp.second->getObjectName() : 0);
    }

    // restore texture bindings
    for (auto unit = 0u; unit < mTextures.size(); ++unit)
    {
        auto const& tex = mTextures[unit];
        if (!tex)
            continue;

        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(tex->getTarget(), tex->getObjectName());
    }

    // safety net: activate different unit
    glActiveTexture(GL_TEXTURE0 + GLenum(limits::maxCombinedTextureImageUnits - 1));
}

bool Program::hasShaderType(GLenum type) const
{
    for (auto const& shader : mShader)
        if (shader->getType() == type)
            return true;

    return false;
}

UsedProgram* Program::getCurrentProgram() { return sCurrentProgram; }

void Program::setShaderReloading(bool enabled) { sCheckShaderReloading = enabled; }

void Program::validateTextureMipmaps() const
{
    for (auto const& t : mTextures)
        if (t != nullptr && !t->areMipmapsGenerated() && t->hasMipmapsEnabled())
            glow::error() << "Texture is bound to shader but does not have mipmaps generated. " << to_string(t.get());
}

GLint Program::getUniformLocation(std::string_view name) const
{
    auto info = getUniformInfo(name);
    return info ? info->location : -1;
}

Program::UniformInfo const* Program::getUniformInfo(std::string_view name) const
{
    for (auto const& e : mUniformCache)
        if (e.name == name)
            return &e;

    return nullptr;
}

Program::UniformInfo* Program::getUniformInfo(std::string_view name)
{
    for (auto& e : mUniformCache)
        if (e.name == name)
            return &e;

    return nullptr;
}

GLint Program::useUniformLocationAndVerify(std::string_view name, GLint size, GLenum type)
{
    auto info = getUniformInfo(name);

    if (!info)
    {
        // This uniform might be missing from the cache because it's name is formatted differently
        // by the user, vs the output from glGetActiveUniform that was used to build the cache.
        // Retry with glGetUniformLocation

        auto nameString = std::string(name);
        auto loc = glGetUniformLocation(mObjectName, nameString.c_str());

        if (loc != -1)
        {
            mUniformCache.emplace_back(nameString, loc, size, type);
            info = &mUniformCache.back();
        }
    }

    if (info)
    {
        if (info->size > 0 && size > 0) // -1 means no check
            if (info->size != size || info->type != type)
                warning() << "Uniform `" << name << "' has type `" << glUniformTypeToString(info->type, info->size) << "' in shader but is set as `"
                          << glUniformTypeToString(type, size) << "' in GLOW. " << to_string(this);
        info->wasSet = true;
        return info->location;
    }

    return -1;
}

GLuint Program::getUniformBlockIndex(std::string_view name) const
{
    checkValidGLOW();
    return glGetUniformBlockIndex(mObjectName, std::string(name).c_str());
}

Program::Program()
{
    checkValidGLOW();
    mObjectName = glCreateProgram();

    mAttributeMapping = std::make_shared<LocationMapping>();
    mFragmentMapping = std::make_shared<LocationMapping>();
}

Program::~Program()
{
    checkValidGLOW();
    glDeleteProgram(mObjectName);
}

bool Program::isLinked() const
{
    checkValidGLOW();
    GLint linkStatus;
    glGetProgramiv(mObjectName, GL_LINK_STATUS, &linkStatus);

    if (linkStatus == GL_FALSE)
        return false;
    else
        return true;
}

std::vector<std::pair<std::string, int>> Program::extractAttributeLocations()
{
    checkValidGLOW();
    std::vector<std::pair<std::string, int>> locs;

    // get attribute locations
    GLint maxAttrLength = 0;
    GLint attrCnt = 0;
    glGetProgramiv(mObjectName, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttrLength);
    glGetProgramiv(mObjectName, GL_ACTIVE_ATTRIBUTES, &attrCnt);

    std::vector<char> nameBuffer;
    nameBuffer.resize(unsigned(maxAttrLength));
    for (auto i = 0; i < attrCnt; ++i)
    {
        GLint size;
        GLenum type;
        glGetActiveAttrib(mObjectName, GLuint(i), GLsizei(nameBuffer.size()), nullptr, &size, &type, nameBuffer.data());

        std::string name{nameBuffer.data()};
        if (name.find("gl_") == 0)
            continue;

        auto shaderLoc = glGetAttribLocation(mObjectName, name.c_str());
        TG_ASSERT(shaderLoc != -1); // should not happen
        locs.push_back({name, shaderLoc});
    }

    return locs;
}

void Program::attachShader(const SharedShader& shader)
{
    checkValidGLOW();
    mShader.push_back(shader);
    glAttachShader(mObjectName, shader->getObjectName());
}

void Program::link(bool saveUniformState)
{
    checkValidGLOW();
    GLOW_ACTION();

    // save uniforms
    auto uniforms = saveUniformState ? getUniforms() : nullptr;

    auto requiresRelink = true;
    auto linkCnt = 0;
    while (requiresRelink)
    {
        requiresRelink = false;

        // set attribute locations
        for (auto const& mapping : mAttributeMapping->getMap())
            glBindAttribLocation(mObjectName, GLuint(mapping.second), mapping.first.c_str());

        // set fragment locations
        for (auto const& mapping : mFragmentMapping->getMap())
            glBindFragDataLocation(mObjectName, GLuint(mapping.second), mapping.first.c_str());

        // set transform feedback
        if (isConfiguredForTransformFeedback())
        {
            std::vector<const char*> varyings;
            for (auto const& s : mTransformFeedbackVaryings)
                varyings.push_back(s.c_str());
            glTransformFeedbackVaryings(mObjectName, GLsizei(varyings.size()), varyings.data(), mTransformFeedbackMode);
            mIsLinkedForTransformFeedback = true;
        }

        ++linkCnt;
        if (!linkAndCheckErrors())
            return; // ERROR!

        // get attribute locations
        GLint maxAttrLength = 0;
        GLint attrCnt = 0;
        glGetProgramiv(mObjectName, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttrLength);
        glGetProgramiv(mObjectName, GL_ACTIVE_ATTRIBUTES, &attrCnt);

        std::vector<char> nameBuffer;
        nameBuffer.resize(unsigned(maxAttrLength));
        for (auto i = 0; i < attrCnt; ++i)
        {
            GLint size;
            GLenum type;
            glGetActiveAttrib(mObjectName, GLuint(i), GLsizei(nameBuffer.size()), nullptr, &size, &type, nameBuffer.data());

            std::string name{nameBuffer.data()};
            if (name.find("gl_") == 0)
                continue;

            auto shaderLoc = glGetAttribLocation(mObjectName, name.c_str());
            TG_ASSERT(shaderLoc != -1); // should not happen
            auto refLoc = mAttributeMapping->getOrAddLocation(name, GLuint(shaderLoc));
            if (refLoc != GLuint(shaderLoc))
                requiresRelink = true;
        }

        // sanity
        if (linkCnt > 2)
        {
            warning() << "Linking does not converge. Aborting. " << to_string(this);
            break;
        }
    }

    // perform layout location check
    // TODO: configure via cmake
    if (!mCheckedForAttributeLocationLayout)
    {
        GLint maxAttrs = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttrs);

        // check mapping
        for (auto const& mapping : mAttributeMapping->getMap())
        {
            auto shaderLoc = glGetAttribLocation(mObjectName, mapping.first.c_str());
            if (shaderLoc == -1)
                continue; // unused

            if (shaderLoc != mapping.second)
            {
                error() << "Attribute '" << mapping.first << "' has inconsistent location (" << mapping.second << " vs " << shaderLoc
                        << "). Note, use of layout(location = X) is not allowed! " << to_string(this);
            }
        }

        // map with offset
        for (auto const& mapping : mAttributeMapping->getMap())
            glBindAttribLocation(mObjectName, GLuint((mapping.second + 1) % maxAttrs), mapping.first.c_str());

        // relink
        linkAndCheckErrors();

        // check offset mapping
        for (auto const& mapping : mAttributeMapping->getMap())
        {
            auto shaderLoc = glGetAttribLocation(mObjectName, mapping.first.c_str());
            if (shaderLoc == -1)
                continue; // unused

            if (shaderLoc != (mapping.second + 1) % maxAttrs)
            {
                error() << "Attribute '" << mapping.first << "' has inconsistent location (" << ((mapping.second + 1) % maxAttrs) << " vs "
                        << shaderLoc << "). Note, use of layout(location = X) is not allowed! " << to_string(this);
            }
        }

        // map normal again
        for (auto const& mapping : mAttributeMapping->getMap())
            glBindAttribLocation(mObjectName, GLuint(mapping.second), mapping.first.c_str());
        // link normal again
        linkAndCheckErrors();

        mCheckedForAttributeLocationLayout = true;
    }

    // perform layout location check
    // TODO: configure via cmake
    if (!mCheckedForFragmentLocationLayout)
    {
        GLint maxFrags = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxFrags);

        // check mapping
        for (auto const& mapping : mFragmentMapping->getMap())
        {
            auto shaderLoc = glGetFragDataLocation(mObjectName, mapping.first.c_str());
            if (shaderLoc == -1)
                continue; // unused

            if (shaderLoc != mapping.second)
            {
                error() << "Fragment output '" << mapping.first << "' has inconsistent location (" << mapping.second << " vs " << shaderLoc
                        << "). Note, use of layout(location = X) is not allowed! " << to_string(this);
            }
        }

        // map with offset
        for (auto const& mapping : mFragmentMapping->getMap())
            glBindFragDataLocation(mObjectName, GLuint((mapping.second + 1) % maxFrags), mapping.first.c_str());

        // relink
        linkAndCheckErrors();

        // check offset mapping
        for (auto const& mapping : mFragmentMapping->getMap())
        {
            auto shaderLoc = glGetFragDataLocation(mObjectName, mapping.first.c_str());
            if (shaderLoc == -1)
                continue; // unused

            if (shaderLoc != (mapping.second + 1) % maxFrags)
            {
                error() << "Fragment output '" << mapping.first << "' has inconsistent location (" << ((mapping.second + 1) % maxFrags) << " vs "
                        << shaderLoc << "). Note, use of layout(location = X) is not allowed! " << to_string(this);
            }
        }

        // map normal again
        for (auto const& mapping : mFragmentMapping->getMap())
            glBindFragDataLocation(mObjectName, GLuint(mapping.second), mapping.first.c_str());
        // link normal again
        linkAndCheckErrors();

        mCheckedForFragmentLocationLayout = true;
    }

    // rebind uniform buffers
    for (auto const& kvp : mUniformBuffers)
        setUniformBuffer(kvp.first, kvp.second);

    for (auto const& kvp : mShaderStorageBuffers)
        setShaderStorageBuffer(kvp.first, kvp.second);

    // rebuild cache
    auto prevCache = mUniformCache; // copy
    mUniformCache.clear();
    GLint cnt;
    glGetProgramiv(mObjectName, GL_ACTIVE_UNIFORMS, &cnt);
    mUniformCache.reserve(size_t(cnt));

    // Receive all indices of uniforms that are inside uniform blocks (and thus do not need to get cached)
    auto constexpr maxUboUniformIndices = 2048;
    std::array<GLint, maxUboUniformIndices> uboUniformIndices;
    size_t numUboUniformIndices = 0;
    {
        GLint activeUniformBlocks;
        glGetProgramiv(mObjectName, GL_ACTIVE_UNIFORM_BLOCKS, &activeUniformBlocks);

        for (auto i = 0; i < activeUniformBlocks; ++i)
        {
            GLint numUboParameters;
            glGetActiveUniformBlockiv(mObjectName, GLuint(i), GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numUboParameters);

            // Skip empty UBOs
            if (numUboParameters <= 0)
                continue;

            if (numUboUniformIndices + GLuint(numUboParameters) >= maxUboUniformIndices)
            {
                // The indices buffer would overflow, stop caching
                glow::warning() << "Shader UBOs contains high number of UBO parameters, exceeding cache size " << to_string(this);
                glow::warning() << "False-positive unchanged uniform warnings might follow";
                break;
            }

            // Write the indices into the buffer
            glGetActiveUniformBlockiv(mObjectName, GLuint(i), GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, &uboUniformIndices[numUboUniformIndices]);
            numUboUniformIndices += GLuint(numUboParameters);
        }
    }

    GLchar uniformName[2048];
    for (auto i = 0; i < cnt; ++i)
    {
        // Skip this uniform if it is part of a UBO
        {
            bool uniformIsInUbo = false;
            for (auto j = 0u; j < numUboUniformIndices; ++j)
            {
                if (i == uboUniformIndices[j])
                {
                    uniformIsInUbo = true;
                    break;
                }
            }

            if (uniformIsInUbo)
                continue;
        }

        GLsizei length;
        GLint size;
        GLenum type;
        glGetActiveUniform(mObjectName, GLuint(i), sizeof(uniformName), &length, &size, &type, uniformName);
        std::string uniformNameStd = uniformName;

        // Skip reserved identifiers
        if (uniformNameStd.find("gl_") == 0)
            continue;

        // Cut down uArray[0] to uArray
        auto const bracketPos = uniformNameStd.find('[');
        if (bracketPos != std::string::npos)
            uniformNameStd = uniformNameStd.substr(0, bracketPos);

        mUniformCache.emplace_back(uniformNameStd, glGetUniformLocation(mObjectName, uniformNameStd.c_str()), size, type);
    }

    // restore uniforms
    if (uniforms)
        uniforms->restore();

    // restore "wasSet" information
    for (auto& info : mUniformCache)
        info.wasSet = false; // uniforms->restore will set it to true
    for (auto const& info : prevCache)
    {
        auto e = getUniformInfo(info.name);
        if (e)
            e->wasSet = info.wasSet;
    }
    mCheckedUnchangedUniforms = false;

    auto now = std::chrono::system_clock::now().time_since_epoch();
    mLastTimeLinked = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void Program::checkUnchangedUniforms()
{
    if (mCheckedUnchangedUniforms)
        return; // already checked
    if (!mWarnOnUnchangedUniforms)
        return; // warning disabled

    for (auto const& info : mUniformCache)
        if (!info.wasSet                  //
            && !isImageUniform(info.type) // not an image
        )
        {
            warning() << "Uniform `" << info.name << "' is used in the shader but was not set via GLOW. " << to_string(this);
            warning() << "  (This also applies for uniforms with default values. We recommend making them non-uniform "
                         "if you don't set them.) ";
            warning() << "  (This warning can be disabled via setWarnOnUnchangedUniforms(false).) ";
        }

    mCheckedUnchangedUniforms = true;
}

void Program::configureTransformFeedback(cc::span<std::string const> varyings, GLenum bufferMode)
{
    if (sCurrentProgram != nullptr && sCurrentProgram->program == this)
    {
        glow::error() << "Cannot set configureTransformFeedback while same program is active. " << to_string(this);
        TG_ASSERT(0);
        return;
    }

    mTransformFeedbackMode = bufferMode;
    mTransformFeedbackVaryings = std::vector(varyings.begin(), varyings.end());
    mIsLinkedForTransformFeedback = false;
}

SharedUniformState Program::getUniforms() const { return UniformState::create(this); }

void UsedProgram::compute(GLuint groupsX, GLuint groupsY, GLuint groupsZ)
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glDispatchCompute(groupsX, groupsY, groupsZ);

    // notify shader exection
    notifyShaderExecuted();
}

void UsedProgram::beginTransformFeedback(GLenum primitiveMode, const SharedBuffer& feedbackBuffer)
{
    if (!isCurrent())
        return;

    if (!program->isConfiguredForTransformFeedback())
    {
        error() << "Cannot call beginTransformFeedback before it is configured (see "
                   "Program::configureTransformFeedback) "
                << to_string(program);
        return;
    }

    if (!program->mIsLinkedForTransformFeedback)
        program->link(); // relink

    checkValidGLOW();

    if (feedbackBuffer != nullptr)
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedbackBuffer->getObjectName());

    glBeginTransformFeedback(primitiveMode);
}

void UsedProgram::endTransformFeedback()
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glEndTransformFeedback();

    // TODO: error handling!
}

SharedProgram Program::create(cc::span<SharedShader const> shader)
{
    auto program = std::make_shared<Program>();
    for (auto const& s : shader)
        program->attachShader(s);
    program->link();
    return program;
}

SharedProgram Program::createFromFile(std::string_view fileOrBaseName)
{
    GLOW_ACTION();

    auto program = std::make_shared<Program>();
    program->setObjectLabel(fileOrBaseName);
    GLenum type;
    std::string content;
    std::string realFileName;

    // try to resolve file directly
    if (Shader::resolveFile(fileOrBaseName, type, content, realFileName))
    {
        if (!realFileName.empty())
            program->attachShader(Shader::createFromFile(type, realFileName));
        else
            program->attachShader(Shader::createFromSource(type, content));
    }
    else // otherwise check endings
    {
        auto foundAnyWithoutStripping = false;

        for (auto const& kvp : glow::shaderEndingToType)
        {
            auto fname = std::string(fileOrBaseName);
            auto wasStripped = false;
            while (fname != "")
            {
                if (Shader::resolveFile(fname + kvp.first, type, content, realFileName))
                {
                    if (!realFileName.empty())
                        program->attachShader(Shader::createFromFile(type, realFileName));
                    else
                        program->attachShader(Shader::createFromSource(type, content));

                    if (!wasStripped)
                        foundAnyWithoutStripping = true;

                    break; // found file
                }

                // try to find "base" versions
                fname = util::stripFileDot(fname);
                wasStripped = true;
            }
        }

        if (!foundAnyWithoutStripping)
            warning() << "No shader found for `" << fileOrBaseName
                      << "' that directly matched (without stripping parts of the filename). This most likely "
                         "indicates a typo or missing file. ";
    }

    if (program->mShader.empty())
        warning() << "No shaders attached from '" << fileOrBaseName << "'. This may indicate a path error.";

    if (program->hasShaderType(GL_COMPUTE_SHADER) && program->mShader.size() > 1)
        warning() << "Shader `" << fileOrBaseName << "' has compute shader and other shaders attached. This is not supported.";

    if (!program->hasShaderType(GL_VERTEX_SHADER) && program->hasShaderType(GL_FRAGMENT_SHADER))
        warning() << "Shader `" << fileOrBaseName << "' has a fragment shader but no vertex shader. This may indicate a path error.";

    program->link();
    return program;
}

SharedProgram Program::createFromFiles(const std::vector<std::string>& filenames)
{
    GLOW_ACTION();

    auto program = std::make_shared<Program>();
    program->setObjectLabel(util::joinToString(filenames));
    GLenum type;
    std::string content;
    std::string realFileName;

    for (auto const& filename : filenames)
        if (Shader::resolveFile(filename, type, content, realFileName))
        {
            if (!realFileName.empty())
                program->attachShader(Shader::createFromFile(type, realFileName));
            else
                program->attachShader(Shader::createFromSource(type, content));
        }
        else
            error() << "Unable to resolve shader file '" << filename << "'. " << to_string(program.get());

    program->link();
    return program;
}

UsedProgram::UsedProgram(Program* program) : program(program)
{
    checkValidGLOW();
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glUseProgram(program->mObjectName);

    previousProgramPtr = sCurrentProgram;
    sCurrentProgram = this;

    checkShaderReload();

    program->restoreExtendedState();
}

void UsedProgram::checkShaderReload()
{
    if (!isCurrent())
        return;

    // check for reloading every 100ms
    auto now = std::chrono::system_clock::now().time_since_epoch();
    if (Program::sCheckShaderReloading && now - std::chrono::milliseconds(program->mLastReloadCheck) > std::chrono::milliseconds(200))
    {
        program->mLastReloadCheck = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

        auto needsReload = false;
        for (auto const& shader : program->mShader)
            if (shader->newerVersionAvailable())
            {
                needsReload = true;
            }

        if (needsReload)
        {
            log() << "Auto-reloading shader program.";
            for (auto const& shader : program->mShader)
            {
                log() << "  reloading " << shader->getFileName();
                shader->reload();
            }

            log() << "  linking.";
            // relink (uniform state is restored)
            program->link();
        }
    }
}


bool UsedProgram::isCurrent() const
{
    GLOW_RUNTIME_ASSERT(sCurrentProgram == this, "Currently bound Program does NOT match represented program " << to_string(program), return false);
    return true;
}

UsedProgram::UsedProgram(UsedProgram&& rhs) : program(rhs.program), previousProgram(rhs.previousProgram), previousProgramPtr(rhs.previousProgramPtr)
{
    // invalidate rhs
    rhs.previousProgram = -1;
    sCurrentProgram = this;
}

UsedProgram::~UsedProgram()
{
    if (previousProgram != -1) // only if valid
    {
        checkValidGLOW();
        glUseProgram(GLuint(previousProgram));
        sCurrentProgram = previousProgramPtr;

        // re-restore prev state
        if (sCurrentProgram)
            sCurrentProgram->program->restoreExtendedState();
    }
}

void Program::implGetUniform(detail::glBaseType type, GLint loc, void* data) const
{
    checkValidGLOW();

    switch (type)
    {
    case detail::glBaseType::Float:
        glGetUniformfv(mObjectName, loc, reinterpret_cast<GLfloat*>(data));
        break;
    case detail::glBaseType::Int:
        glGetUniformiv(mObjectName, loc, reinterpret_cast<GLint*>(data));
        break;
    case detail::glBaseType::UInt:
        glGetUniformuiv(mObjectName, loc, reinterpret_cast<GLuint*>(data));
        break;
    case detail::glBaseType::Double:
        glGetUniformdv(mObjectName, loc, reinterpret_cast<GLdouble*>(data));
        break;
    }
}
