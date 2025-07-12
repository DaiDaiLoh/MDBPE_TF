#include "UniformBuffer.hh"

#include <glow/glow.hh>

#include <glow/common/runtime_assert.hh>
#include <glow/common/thread_local.hh>

using namespace glow;

/// Currently bound buffer
static GLOW_THREADLOCAL BoundUniformBuffer* sCurrentUniformBuffer = nullptr;

BoundUniformBuffer* UniformBuffer::getCurrentBuffer() { return sCurrentUniformBuffer; }

UniformBuffer::UniformBuffer() : Buffer(GL_UNIFORM_BUFFER) {}

void UniformBuffer::addVerification(int offset, std::string_view nameInShader) { mVerificationOffsets[nameInShader] = offset; }

SharedUniformBuffer UniformBuffer::create() { return std::make_shared<UniformBuffer>(); }

bool BoundUniformBuffer::isCurrent() const
{
    GLOW_RUNTIME_ASSERT(sCurrentUniformBuffer == this, "Currently bound UB does NOT match represented buffer " << to_string(buffer), return false);
    return true;
}

void BoundUniformBuffer::setData(size_t size, const void* data, GLenum usage)
{
    if (!isCurrent())
        return;

    checkValidGLOW();
    glBufferData(GL_UNIFORM_BUFFER, GLsizeiptr(size), data, usage);
}

BoundUniformBuffer::BoundUniformBuffer(UniformBuffer* buffer) : buffer(buffer)
{
    checkValidGLOW();
    glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &previousBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer->getObjectName());

    previousBufferPtr = sCurrentUniformBuffer;
    sCurrentUniformBuffer = this;
}

BoundUniformBuffer::BoundUniformBuffer(BoundUniformBuffer&& rhs)
  : buffer(rhs.buffer), previousBuffer(rhs.previousBuffer), previousBufferPtr(rhs.previousBufferPtr)
{
    // invalidate rhs
    rhs.previousBuffer = -1;
    sCurrentUniformBuffer = this;
}

BoundUniformBuffer::~BoundUniformBuffer()
{
    if (previousBuffer != -1) // if valid
    {
        glBindBuffer(GL_UNIFORM_BUFFER, GLuint(previousBuffer));
        sCurrentUniformBuffer = previousBufferPtr;
    }
}
