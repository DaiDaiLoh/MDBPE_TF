#include "Program.hh"

#include "AtomicCounterBuffer.hh"
#include "Sampler.hh"
#include "ShaderStorageBuffer.hh"
#include "Texture.hh"
#include "UniformBuffer.hh"

#include <glow/glow.hh>
#include <glow/limits.hh>
#include <glow/util/UniformState.hh>

#include <glow/common/runtime_assert.hh>

using namespace glow;

void UsedProgram::setTexture(std::string_view name, const SharedTexture& tex, SharedSampler const& sampler)
{
    if (!isCurrent())
        return;

    checkValidGLOW();

    // get unit
    auto unit = program->mTextureUnitMapping.getOrAddLocation(name);

    // ensure enough unit entries
    while (program->mTextures.size() <= unit)
    {
        program->mTextures.push_back(nullptr);
        program->mSamplers.push_back(nullptr);
    }

    // set unit entries
    program->mTextures[unit] = tex;
    program->mSamplers[unit] = sampler;

    // bind or unbind texture
    if (tex)
    {
        // bind texture to unit
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(tex->getTarget(), tex->getObjectName());

        // safety net: activate different unit
        glActiveTexture(GL_TEXTURE0 + limits::maxCombinedTextureImageUnits - 1);

        // update shader binding
        glUniform1i(program->useUniformLocationAndVerify(name, 1, tex->getUniformType()), unit);
    }
    else // nullptr
    {
        // update shader binding
        glUniform1i(program->getUniformLocation(name), limits::maxCombinedTextureImageUnits - 1);
    }

    // bind or unbind sampler
    glBindSampler(unit, sampler ? sampler->getObjectName() : 0);
}

void UsedProgram::setImage(int bindingLocation, const SharedTexture& tex, GLenum usage, int mipmapLevel, int layer)
{
    GLOW_RUNTIME_ASSERT(tex->isStorageImmutable(), "Texture has to be storage immutable for image binding", return );
    checkValidGLOW();

    // TODO: format handling
    // TODO: slicing
    glBindImageTexture(bindingLocation, tex->getObjectName(), mipmapLevel, GL_TRUE, layer, usage, tex->getInternalFormat());
}

void UsedProgram::setUniforms(const SharedUniformState& state)
{
    if (!isCurrent())
        return;

    state->restore();
}

void UsedProgram::setUniformIntInternal(std::string_view name, int count, const int32_t* values, GLenum uniformType) const
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glUniform1iv(program->useUniformLocationAndVerify(name, count, uniformType), count, values);
}

void UsedProgram::setUniform(std::string_view name, GLenum uniformType, GLint size, void const* data)
{
    switch (uniformType)
    {
    // floats
    case GL_FLOAT:
        uniform<float[]>(name) = array_view<const float>(data, size);
        break;
    case GL_FLOAT_VEC2:
        uniform<tg::vec2[]>(name) = array_view<const tg::vec2>(data, size);
        break;
    case GL_FLOAT_VEC3:
        uniform<tg::vec3[]>(name) = array_view<const tg::vec3>(data, size);
        break;
    case GL_FLOAT_VEC4:
        uniform<tg::vec4[]>(name) = array_view<const tg::vec4>(data, size);
        break;
    case GL_FLOAT_MAT2:
        uniform<tg::mat2[]>(name) = array_view<const tg::mat2>(data, size);
        break;
    case GL_FLOAT_MAT3:
        uniform<tg::mat3[]>(name) = array_view<const tg::mat3>(data, size);
        break;
    case GL_FLOAT_MAT4:
        uniform<tg::mat4[]>(name) = array_view<const tg::mat4>(data, size);
        break;
    case GL_FLOAT_MAT2x3:
        uniform<tg::mat2x3[]>(name) = array_view<const tg::mat2x3>(data, size);
        break;
    case GL_FLOAT_MAT2x4:
        uniform<tg::mat2x4[]>(name) = array_view<const tg::mat2x4>(data, size);
        break;
    case GL_FLOAT_MAT3x2:
        uniform<tg::mat3x2[]>(name) = array_view<const tg::mat3x2>(data, size);
        break;
    case GL_FLOAT_MAT3x4:
        uniform<tg::mat3x4[]>(name) = array_view<const tg::mat3x4>(data, size);
        break;
    case GL_FLOAT_MAT4x2:
        uniform<tg::mat4x2[]>(name) = array_view<const tg::mat4x2>(data, size);
        break;
    case GL_FLOAT_MAT4x3:
        uniform<tg::mat4x3[]>(name) = array_view<const tg::mat4x3>(data, size);
        break;

    // doubles
    /* MAYBE some day
case GL_DOUBLE:
    setUniform(name, size, reinterpret_cast<double const*>(data));
    break;
case GL_DOUBLE_VEC2:
    setUniform(name, size, reinterpret_cast<glm::dvec2 const*>(data));
    break;
case GL_DOUBLE_VEC3:
    setUniform(name, size, reinterpret_cast<glm::dvec3 const*>(data));
    break;
case GL_DOUBLE_VEC4:
    setUniform(name, size, reinterpret_cast<glm::dvec4 const*>(data));
    break;
case GL_DOUBLE_MAT2:
    setUniform(name, size, reinterpret_cast<glm::dmat2 const*>(data));
    break;
case GL_DOUBLE_MAT3:
    setUniform(name, size, reinterpret_cast<glm::dmat3 const*>(data));
    break;
case GL_DOUBLE_MAT4:
    setUniform(name, size, reinterpret_cast<glm::dmat4 const*>(data));
    break;
case GL_DOUBLE_MAT2x3:
    setUniform(name, size, reinterpret_cast<glm::dmat2x3 const*>(data));
    break;
case GL_DOUBLE_MAT2x4:
    setUniform(name, size, reinterpret_cast<glm::dmat2x4 const*>(data));
    break;
case GL_DOUBLE_MAT3x2:
    setUniform(name, size, reinterpret_cast<glm::dmat3x2 const*>(data));
    break;
case GL_DOUBLE_MAT3x4:
    setUniform(name, size, reinterpret_cast<glm::dmat3x4 const*>(data));
    break;
case GL_DOUBLE_MAT4x2:
    setUniform(name, size, reinterpret_cast<glm::dmat4x2 const*>(data));
    break;
case GL_DOUBLE_MAT4x3:
    setUniform(name, size, reinterpret_cast<glm::dmat4x3 const*>(data));
    break;
    */

    // uint
    case GL_UNSIGNED_INT:
    case GL_UNSIGNED_INT_ATOMIC_COUNTER:
        uniform<uint32_t[]>(name) = array_view<const uint32_t>(data, size);
        break;
    case GL_UNSIGNED_INT_VEC2:
        uniform<tg::uvec2[]>(name) = array_view<const tg::uvec2>(data, size);
        break;
    case GL_UNSIGNED_INT_VEC3:
        uniform<tg::uvec3[]>(name) = array_view<const tg::uvec3>(data, size);
        break;
    case GL_UNSIGNED_INT_VEC4:
        uniform<tg::uvec4[]>(name) = array_view<const tg::uvec4>(data, size);
        break;

    // int
    case GL_INT:
        uniform<int[]>(name) = array_view<const int>(data, size);
        break;
    case GL_INT_VEC2:
        uniform<tg::ivec2[]>(name) = array_view<const tg::ivec2>(data, size);
        break;
    case GL_INT_VEC3:
        uniform<tg::ivec3[]>(name) = array_view<const tg::ivec3>(data, size);
        break;
    case GL_INT_VEC4:
        uniform<tg::ivec4[]>(name) = array_view<const tg::ivec4>(data, size);
        break;
    // bool
    case GL_BOOL:
        setUniformIntInternal(name, size, reinterpret_cast<int32_t const*>(data), GL_BOOL);
        break;
    case GL_BOOL_VEC2:
        uniform<tg::bcomp2[]>(name) = array_view<const tg::icomp2>(data, size);
        break;
    case GL_BOOL_VEC3:
        uniform<tg::bcomp3[]>(name) = array_view<const tg::icomp3>(data, size);
        break;
    case GL_BOOL_VEC4:
        uniform<tg::bcomp4[]>(name) = array_view<const tg::icomp4>(data, size);
        break;
    // sampler
    case GL_SAMPLER_1D:
    case GL_SAMPLER_2D:
    case GL_SAMPLER_3D:
    case GL_SAMPLER_CUBE:
    case GL_SAMPLER_1D_SHADOW:
    case GL_SAMPLER_2D_SHADOW:
    case GL_SAMPLER_1D_ARRAY:
    case GL_SAMPLER_2D_ARRAY:
    case GL_SAMPLER_1D_ARRAY_SHADOW:
    case GL_SAMPLER_2D_ARRAY_SHADOW:
    case GL_SAMPLER_2D_MULTISAMPLE:
    case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_SAMPLER_CUBE_SHADOW:
    case GL_SAMPLER_BUFFER:
    case GL_SAMPLER_2D_RECT:
    case GL_SAMPLER_2D_RECT_SHADOW:
    case GL_INT_SAMPLER_1D:
    case GL_INT_SAMPLER_2D:
    case GL_INT_SAMPLER_3D:
    case GL_INT_SAMPLER_CUBE:
    case GL_INT_SAMPLER_1D_ARRAY:
    case GL_INT_SAMPLER_2D_ARRAY:
    case GL_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_INT_SAMPLER_BUFFER:
    case GL_INT_SAMPLER_2D_RECT:
    case GL_UNSIGNED_INT_SAMPLER_1D:
    case GL_UNSIGNED_INT_SAMPLER_2D:
    case GL_UNSIGNED_INT_SAMPLER_3D:
    case GL_UNSIGNED_INT_SAMPLER_CUBE:
    case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_SAMPLER_BUFFER:
    case GL_UNSIGNED_INT_SAMPLER_2D_RECT:
    // images
    case GL_IMAGE_1D:
    case GL_IMAGE_2D:
    case GL_IMAGE_3D:
    case GL_IMAGE_CUBE:
    case GL_IMAGE_1D_ARRAY:
    case GL_IMAGE_2D_ARRAY:
    case GL_IMAGE_2D_MULTISAMPLE:
    case GL_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_IMAGE_BUFFER:
    case GL_IMAGE_2D_RECT:
    case GL_INT_IMAGE_1D:
    case GL_INT_IMAGE_2D:
    case GL_INT_IMAGE_3D:
    case GL_INT_IMAGE_CUBE:
    case GL_INT_IMAGE_1D_ARRAY:
    case GL_INT_IMAGE_2D_ARRAY:
    case GL_INT_IMAGE_2D_MULTISAMPLE:
    case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_INT_IMAGE_BUFFER:
    case GL_INT_IMAGE_2D_RECT:
    case GL_UNSIGNED_INT_IMAGE_1D:
    case GL_UNSIGNED_INT_IMAGE_2D:
    case GL_UNSIGNED_INT_IMAGE_3D:
    case GL_UNSIGNED_INT_IMAGE_CUBE:
    case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
    case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
    case GL_UNSIGNED_INT_IMAGE_BUFFER:
    case GL_UNSIGNED_INT_IMAGE_2D_RECT:
        setUniformIntInternal(name, size, reinterpret_cast<int32_t const*>(data), uniformType);
        break;

    default:
        error() << "Uniform type not implemented: " << uniformType;
        break;
    }
}

void Program::setUniformBuffer(std::string_view bufferName, const SharedUniformBuffer& buffer)
{
    checkValidGLOW();
    auto loc = mUniformBufferMapping.getOrAddLocation(bufferName);
    auto idx = getUniformBlockIndex(bufferName);

    auto isNew = !mUniformBuffers.count(bufferName);
    mUniformBuffers[bufferName] = buffer;

    if (idx == GL_INVALID_INDEX)
        return; // not active

    glUniformBlockBinding(mObjectName, idx, loc);

    if (getCurrentProgram() && getCurrentProgram()->program == this)
        glBindBufferBase(GL_UNIFORM_BUFFER, loc, buffer ? buffer->getObjectName() : 0);

    if (isNew)
        verifyUniformBuffer(bufferName, buffer);
}

void Program::setShaderStorageBuffer(std::string_view bufferName, const SharedShaderStorageBuffer& buffer)
{
    checkValidGLOW();
    auto loc = mShaderStorageBufferMapping.getOrAddLocation(bufferName);
    auto idx = glGetProgramResourceIndex(mObjectName, GL_SHADER_STORAGE_BLOCK, std::string(bufferName).c_str());

    mShaderStorageBuffers[bufferName] = buffer;

    if (idx == GL_INVALID_INDEX)
        return; // not active

    glShaderStorageBlockBinding(mObjectName, idx, loc);

    if (getCurrentProgram() && getCurrentProgram()->program == this)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, loc, buffer ? buffer->getObjectName() : 0);
}

void Program::setAtomicCounterBuffer(int bindingPoint, const SharedAtomicCounterBuffer& buffer)
{
    checkValidGLOW();
    mAtomicCounterBuffers[bindingPoint] = buffer;

    if (getCurrentProgram() && getCurrentProgram()->program == this)
        glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, GLuint(bindingPoint), buffer ? buffer->getObjectName() : 0);
}

bool Program::verifyUniformBuffer(std::string_view bufferName, const SharedUniformBuffer& buffer)
{
    if (buffer->getVerificationOffsets().empty())
        return true; // nothing to verify

    checkValidGLOW();

    auto blockIdx = glGetUniformBlockIndex(mObjectName, std::string(bufferName).c_str());
    if (blockIdx == GL_INVALID_INDEX)
        return true; // not active

    // get nr of uniforms
    GLint uniformCnt = -1;
    glGetActiveUniformBlockiv(mObjectName, blockIdx, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &uniformCnt);

    // get uniform indices
    std::vector<GLint> uniformIndices;
    uniformIndices.resize(unsigned(uniformCnt));
    glGetActiveUniformBlockiv(mObjectName, blockIdx, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, uniformIndices.data());

    // get offsets of uniforms
    std::vector<GLint> uniformOffsets;
    uniformOffsets.resize(unsigned(uniformCnt));
    glGetActiveUniformsiv(mObjectName, uniformCnt, reinterpret_cast<GLuint const*>(uniformIndices.data()), GL_UNIFORM_OFFSET, uniformOffsets.data());

    // get name max length
    GLint nameMaxLength;
    glGetProgramiv(mObjectName, GL_ACTIVE_UNIFORM_MAX_LENGTH, &nameMaxLength);

    // get names
    std::vector<char> nameBuffer;
    nameBuffer.resize(unsigned(nameMaxLength) + 1);
    std::vector<std::string> uniformNames;
    for (auto uIdx : uniformIndices)
    {
        glGetActiveUniformName(mObjectName, GLuint(uIdx), GLsizei(nameBuffer.size()), nullptr, nameBuffer.data());
        uniformNames.push_back(nameBuffer.data());
    }

    // actual verification
    auto failure = false;
    auto const& vOffsets = buffer->getVerificationOffsets();
    for (auto i = 0u; i < uniformIndices.size(); ++i)
    {
        auto gpuOffset = uniformOffsets[i];
        auto const& name = uniformNames[i];

        if (vOffsets.count(name))
        {
            auto cpuOffset = int(vOffsets.at(name));

            // mismatch
            if (cpuOffset != gpuOffset)
            {
                if (!failure)
                    error() << "UniformBuffer Verification Failure for `" << bufferName << "':";
                failure = true;

                error() << "  * Uniform `" << name << "': CPU@" << cpuOffset << " vs GPU@" << gpuOffset;
            }
        }
    }

    return !failure;
}
