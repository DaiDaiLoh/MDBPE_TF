#include "ArrayBuffer.hh"

#include <typed-geometry/feature/assert.hh>

#include <algorithm>

#include <glow/glow.hh>

#include <glow/common/ogl_typeinfo.hh>
#include <glow/common/runtime_assert.hh>
#include <glow/common/thread_local.hh>

using namespace glow;

/// Currently bound buffer
static GLOW_THREADLOCAL BoundArrayBuffer* sCurrentArrayBuffer = nullptr;

BoundArrayBuffer* ArrayBuffer::getCurrentBuffer() { return sCurrentArrayBuffer; }

void ArrayBuffer::defineAttribute(const ArrayBufferAttribute& a)
{
    mAttributes.push_back(a);

    auto elementCnt = a.size == GL_BGRA ? 4 : a.size;
    auto sizeInBytes = elementCnt * sizeOfTypeInBytes(a.type);

    if (a.fixedStride > 0)
    {
        if (mStride > 0 && mStride != a.fixedStride)
        {
            error() << "Attribute requires fixed stride of " << a.fixedStride << " but stride is already set to " << mStride << " " << to_string(this);
            error() << "  Note: if you mix member-style attributes and manual ones, always use member-style first.";
        }
        mStride = a.fixedStride;
    }
    else
    {
        mStride = std::max(mStride, a.offset + sizeInBytes);
    }
}

void ArrayBuffer::defineAttributes(cc::span<ArrayBufferAttribute const> attrs)
{
    for (auto const& a : attrs)
        defineAttribute(a);
}

void ArrayBuffer::defineAttribute(const std::string& name, GLenum type, GLint size, AttributeMode mode, GLuint divisor)
{
    defineAttribute({name, type, size, mStride, divisor, mode});
}

void ArrayBuffer::defineAttributeWithPadding(const std::string& name, GLenum type, GLint size, GLuint padding, AttributeMode mode, GLuint divisor)
{
    defineAttribute({name, type, size, mStride, divisor, mode});
    mStride += padding; // extra padding
}

void ArrayBuffer::defineAttributeWithOffset(const std::string& name, GLenum type, GLint size, GLuint offset, AttributeMode mode, GLuint divisor)
{
    defineAttribute({name, type, size, offset, divisor, mode});
}

ArrayBuffer::ArrayBuffer() : Buffer(GL_ARRAY_BUFFER) {}

SharedArrayBuffer ArrayBuffer::create() { return std::make_shared<ArrayBuffer>(); }

SharedArrayBuffer ArrayBuffer::create(cc::span<ArrayBufferAttribute const> attrs)
{
    auto ab = create();
    ab->defineAttributes(attrs);
    return ab;
}


bool BoundArrayBuffer::isCurrent() const
{
    GLOW_RUNTIME_ASSERT(sCurrentArrayBuffer == this, "Currently bound AB does NOT match represented buffer " << to_string(buffer), return false);
    return true;
}

void BoundArrayBuffer::setData(size_t sizeInBytes, const void* data, GLenum usage) { implSetData(sizeInBytes, data, 0, usage); }

void BoundArrayBuffer::implSetData(size_t sizeInBytes, const void* data, size_t stride, GLenum usage)
{
    if (buffer->mStride > 0 && stride > 0 && stride != buffer->mStride)
        warning() << "Stride mismatch: expected " << buffer->mStride << ", got " << stride << " " << to_string(buffer);

    if (!isCurrent())
        return;

    checkValidGLOW();

    glBufferData(GL_ARRAY_BUFFER, sizeInBytes, data, usage);
    buffer->mElementCount = (GLuint)sizeInBytes / buffer->mStride;
    TG_ASSERT(sizeInBytes % buffer->mStride == 0 && "The size in byte doesn't match stride * elements");
}

BoundArrayBuffer::BoundArrayBuffer(ArrayBuffer* buffer) : buffer(buffer)
{
    checkValidGLOW();

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->getObjectName());

    previousBufferPtr = sCurrentArrayBuffer;
    sCurrentArrayBuffer = this;
}

BoundArrayBuffer::BoundArrayBuffer(BoundArrayBuffer&& rhs)
  : buffer(rhs.buffer), previousBuffer(rhs.previousBuffer), previousBufferPtr(rhs.previousBufferPtr)
{
    // invalidate rhs
    rhs.previousBuffer = -1;
    sCurrentArrayBuffer = this;
}

BoundArrayBuffer::~BoundArrayBuffer()
{
    if (previousBuffer != -1) // if valid
    {
        glBindBuffer(GL_ARRAY_BUFFER, previousBuffer);
        sCurrentArrayBuffer = previousBufferPtr;
    }
}

void glow::ArrayBuffer::setDivisor(int div)
{
    for (auto& a : mAttributes)
        a.divisor = div;
}
