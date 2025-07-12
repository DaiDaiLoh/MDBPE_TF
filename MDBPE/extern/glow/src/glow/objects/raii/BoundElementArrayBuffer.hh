#pragma once

#include <vector>

#include <clean-core/span.hh>

#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

namespace glow
{
/// RAII-object that defines a "bind"-scope for an ElementArrayBuffer
/// All functions that operate on the currently bound buffer are accessed here
struct BoundElementArrayBuffer
{
    GLOW_RAII_CLASS(BoundElementArrayBuffer);

    /// Backreference to the buffer
    ElementArrayBuffer* const buffer;

    /// Uploads a set of indices
    /// Automatically picks the right data type
    ///
    /// usage is a hint to the GL implementation as to how a buffer object's data store will be accessed.
    /// This enables the GL implementation to make more intelligent decisions that may significantly impact buffer
    /// object performance. It does not, however, constrain the actual usage of the data store.
    /// (see https://www.opengl.org/sdk/docs/man/html/glBufferData.xhtml)
    void setIndices(cc::span<int8_t const> indices, GLenum usage = GL_STATIC_DRAW) { setIndices(int(indices.size()), indices.data(), usage); }
    void setIndices(cc::span<uint8_t const> indices, GLenum usage = GL_STATIC_DRAW) { setIndices(int(indices.size()), indices.data(), usage); }
    void setIndices(cc::span<int16_t const> indices, GLenum usage = GL_STATIC_DRAW) { setIndices(int(indices.size()), indices.data(), usage); }
    void setIndices(cc::span<uint16_t const> indices, GLenum usage = GL_STATIC_DRAW) { setIndices(int(indices.size()), indices.data(), usage); }
    void setIndices(cc::span<int32_t const> indices, GLenum usage = GL_STATIC_DRAW) { setIndices(int(indices.size()), indices.data(), usage); }
    void setIndices(cc::span<uint32_t const> indices, GLenum usage = GL_STATIC_DRAW) { setIndices(int(indices.size()), indices.data(), usage); }
    void setIndices(int indexCount, const int8_t* data = nullptr, GLenum usage = GL_STATIC_DRAW);
    void setIndices(int indexCount, const uint8_t* data = nullptr, GLenum usage = GL_STATIC_DRAW);
    void setIndices(int indexCount, const int16_t* data = nullptr, GLenum usage = GL_STATIC_DRAW);
    void setIndices(int indexCount, const uint16_t* data = nullptr, GLenum usage = GL_STATIC_DRAW);
    void setIndices(int indexCount, const int32_t* data = nullptr, GLenum usage = GL_STATIC_DRAW);
    void setIndices(int indexCount, const uint32_t* data = nullptr, GLenum usage = GL_STATIC_DRAW);
    void setIndices(int indexCount, GLenum indexType, const void* data = nullptr, GLenum usage = GL_STATIC_DRAW);

private:
    GLint previousBuffer;                       ///< previously bound buffer
    BoundElementArrayBuffer* previousBufferPtr; ///< previously bound buffer
    BoundElementArrayBuffer(ElementArrayBuffer* buffer);
    friend class ElementArrayBuffer;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

public:
    BoundElementArrayBuffer(BoundElementArrayBuffer&&); // allow move
    ~BoundElementArrayBuffer();
};
}
