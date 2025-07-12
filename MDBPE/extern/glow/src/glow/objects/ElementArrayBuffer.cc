#include "ElementArrayBuffer.hh"

#include "VertexArray.hh"

#include <typed-geometry/feature/assert.hh>

#include <glow/glow.hh>

#include <glow/common/runtime_assert.hh>
#include <glow/common/thread_local.hh>

using namespace glow;

/// Currently bound buffer
static GLOW_THREADLOCAL BoundElementArrayBuffer* sCurrentElementArrayBuffer = nullptr;

BoundElementArrayBuffer* ElementArrayBuffer::getCurrentBuffer() { return sCurrentElementArrayBuffer; }

ElementArrayBuffer::ElementArrayBuffer() : Buffer(GL_ELEMENT_ARRAY_BUFFER) {}

SharedElementArrayBuffer ElementArrayBuffer::create() { return std::make_shared<ElementArrayBuffer>(); }

SharedElementArrayBuffer ElementArrayBuffer::create(int indexCount, const int8_t* data)
{
    auto eab = create();
    eab->bind().setIndices(indexCount, data);
    return eab;
}

SharedElementArrayBuffer ElementArrayBuffer::create(int indexCount, const uint8_t* data)
{
    auto eab = create();
    eab->bind().setIndices(indexCount, data);
    return eab;
}

SharedElementArrayBuffer ElementArrayBuffer::create(int indexCount, const int16_t* data)
{
    auto eab = create();
    eab->bind().setIndices(indexCount, data);
    return eab;
}

SharedElementArrayBuffer ElementArrayBuffer::create(int indexCount, const uint16_t* data)
{
    auto eab = create();
    eab->bind().setIndices(indexCount, data);
    return eab;
}

SharedElementArrayBuffer ElementArrayBuffer::create(int indexCount, const int32_t* data)
{
    auto eab = create();
    eab->bind().setIndices(indexCount, data);
    return eab;
}

SharedElementArrayBuffer ElementArrayBuffer::create(int indexCount, const uint32_t* data)
{
    auto eab = create();
    eab->bind().setIndices(indexCount, data);
    return eab;
}

bool BoundElementArrayBuffer::isCurrent() const
{
    GLOW_RUNTIME_ASSERT(sCurrentElementArrayBuffer == this, "Currently bound EAB does NOT match represented buffer " << to_string(buffer), return false);
    return true;
}

void BoundElementArrayBuffer::setIndices(int indexCount, const int8_t* data, GLenum usage) { setIndices(indexCount, GL_UNSIGNED_BYTE, data, usage); }

void BoundElementArrayBuffer::setIndices(int indexCount, const uint8_t* data, GLenum usage) { setIndices(indexCount, GL_UNSIGNED_BYTE, data, usage); }

void BoundElementArrayBuffer::setIndices(int indexCount, const int16_t* data, GLenum usage)
{
    setIndices(indexCount, GL_UNSIGNED_SHORT, data, usage);
}

void BoundElementArrayBuffer::setIndices(int indexCount, const uint16_t* data, GLenum usage)
{
    setIndices(indexCount, GL_UNSIGNED_SHORT, data, usage);
}

void BoundElementArrayBuffer::setIndices(int indexCount, const int32_t* data, GLenum usage) { setIndices(indexCount, GL_UNSIGNED_INT, data, usage); }

void BoundElementArrayBuffer::setIndices(int indexCount, const uint32_t* data, GLenum usage) { setIndices(indexCount, GL_UNSIGNED_INT, data, usage); }

void BoundElementArrayBuffer::setIndices(int indexCount, GLenum indexType, const void* data, GLenum usage)
{
    if (!isCurrent())
        return;

    checkValidGLOW();

    auto sizeInBytes = indexCount;
    switch (indexType)
    {
    case GL_UNSIGNED_BYTE:
        sizeInBytes *= sizeof(uint8_t);
        break;
    case GL_UNSIGNED_SHORT:
        sizeInBytes *= sizeof(uint16_t);
        break;
    case GL_UNSIGNED_INT:
        sizeInBytes *= sizeof(uint32_t);
        break;
    default:
        TG_ASSERT(0);
        break;
    }

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeInBytes, data, usage);
    buffer->mIndexType = indexType;
    buffer->mIndexCount = indexCount;
}

BoundElementArrayBuffer::BoundElementArrayBuffer(ElementArrayBuffer* buffer) : buffer(buffer)
{
    GLOW_RUNTIME_ASSERT(VertexArray::getCurrentVAO() == nullptr,
                        "Cannot bind an EAB while a VAO is bound! (this has unintended side-effects in OpenGL) "
                            << to_string(buffer) << ", " << to_string(VertexArray::getCurrentVAO()->vao),
                        return);

    checkValidGLOW();

    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previousBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->getObjectName());

    previousBufferPtr = sCurrentElementArrayBuffer;
    sCurrentElementArrayBuffer = this;
}

BoundElementArrayBuffer::BoundElementArrayBuffer(BoundElementArrayBuffer&& rhs)
  : buffer(rhs.buffer), previousBuffer(rhs.previousBuffer), previousBufferPtr(rhs.previousBufferPtr)
{
    // invalidate rhs
    rhs.previousBuffer = -1;
    sCurrentElementArrayBuffer = this;
}

BoundElementArrayBuffer::~BoundElementArrayBuffer()
{
    if (previousBuffer != -1) // if valid
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, previousBuffer);
        sCurrentElementArrayBuffer = previousBufferPtr;
    }
}
