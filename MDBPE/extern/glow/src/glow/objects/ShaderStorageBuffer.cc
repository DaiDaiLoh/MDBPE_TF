#include "ShaderStorageBuffer.hh"

#include <glow/glow.hh>

#include <glow/common/runtime_assert.hh>
#include <glow/common/thread_local.hh>

using namespace glow;

/// Currently bound buffer
static GLOW_THREADLOCAL BoundShaderStorageBuffer* sCurrentShaderStorageBuffer = nullptr;

BoundShaderStorageBuffer* ShaderStorageBuffer::getCurrentBuffer() { return sCurrentShaderStorageBuffer; }

ShaderStorageBuffer::ShaderStorageBuffer(const SharedBuffer& originalBuffer) : Buffer(GL_SHADER_STORAGE_BUFFER, originalBuffer) {}

SharedShaderStorageBuffer ShaderStorageBuffer::create() { return std::make_shared<ShaderStorageBuffer>(); }

SharedShaderStorageBuffer ShaderStorageBuffer::create(size_t sizeInBytes)
{
    auto buffer = create();
    buffer->bind().reserve(sizeInBytes);
    return buffer;
}

SharedShaderStorageBuffer ShaderStorageBuffer::createAliased(const SharedBuffer& originalBuffer)
{
    return std::make_shared<ShaderStorageBuffer>(originalBuffer);
}

bool BoundShaderStorageBuffer::verifyStride(size_t size, size_t stride) const
{
    if (size % stride == 0)
        return true;

    error() << "Buffer size is not multiple of data type! " << to_string(buffer);
    return false;
}

bool BoundShaderStorageBuffer::isCurrent() const
{
    GLOW_RUNTIME_ASSERT(sCurrentShaderStorageBuffer == this, "Currently bound UB does NOT match represented buffer " << to_string(buffer), return false);
    return true;
}

void BoundShaderStorageBuffer::setData(size_t size, const void* data, GLenum usage)
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
}

void BoundShaderStorageBuffer::setSubData(size_t offset, size_t size, const void* data)
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, GLintptr(offset), size, data);
}

void BoundShaderStorageBuffer::getData(void* destination, size_t maxSize, bool warnOnTruncate)
{
    if (!isCurrent())
        return;

    auto size = getSize();
    if (maxSize > 0 && maxSize < size)
    {
        if (warnOnTruncate)
            warning() << "Buffer size is " << size << " B but only " << maxSize << " B is guaranteed. " << to_string(buffer);

        size = maxSize;
    }

    checkValidGLOW();
    glGetBufferSubData(buffer->getType(), 0, size, destination);
}

void BoundShaderStorageBuffer::getSubData(void* destination, size_t offset, size_t size)
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glGetBufferSubData(buffer->getType(), GLintptr(offset), size, destination);
}

size_t BoundShaderStorageBuffer::getSize() const
{
    if (!isCurrent())
        return 0u;

    checkValidGLOW();
    int bufSize = 0;
    glGetBufferParameteriv(buffer->getType(), GL_BUFFER_SIZE, &bufSize);

    return bufSize;
}

void BoundShaderStorageBuffer::reserve(size_t sizeInBytes, GLenum usage)
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeInBytes, nullptr, usage);
}

BoundShaderStorageBuffer::BoundShaderStorageBuffer(ShaderStorageBuffer* buffer) : buffer(buffer)
{
    checkValidGLOW();
    glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &previousBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer->getObjectName());

    previousBufferPtr = sCurrentShaderStorageBuffer;
    sCurrentShaderStorageBuffer = this;
}

BoundShaderStorageBuffer::BoundShaderStorageBuffer(BoundShaderStorageBuffer&& rhs)
  : buffer(rhs.buffer), previousBuffer(rhs.previousBuffer), previousBufferPtr(rhs.previousBufferPtr)
{
    // invalidate rhs
    rhs.previousBuffer = -1;
    sCurrentShaderStorageBuffer = this;
}

BoundShaderStorageBuffer::~BoundShaderStorageBuffer()
{
    if (previousBuffer != -1) // if valid
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, previousBuffer);
        sCurrentShaderStorageBuffer = previousBufferPtr;
    }
}
