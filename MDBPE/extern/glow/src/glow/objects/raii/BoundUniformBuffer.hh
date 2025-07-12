#pragma once

#include <vector>

#include <clean-core/span.hh>

#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

namespace glow
{
/// RAII-object that defines a "bind"-scope for an UniformBuffer
/// All functions that operate on the currently bound buffer are accessed here
struct BoundUniformBuffer
{
    GLOW_RAII_CLASS(BoundUniformBuffer);

    /// Backreference to the buffer
    UniformBuffer* const buffer;

    /// Sets the data of this uniform buffer (generic version)
    void setData(size_t size, const void* data, GLenum usage = GL_STATIC_DRAW);
    /// Sets the data of this uniform buffer (vector-of-data version)
    template <class DataRangeT>
    void setData(DataRangeT&& data, GLenum usage = GL_STATIC_DRAW)
    {
        auto bytes = cc::as_byte_span(data);
        setData(bytes.size(), bytes.data(), usage);
    }
    /// same as above for initializer list
    template <class DataT>
    void setData(std::initializer_list<DataT> data, GLenum usage = GL_STATIC_DRAW)
    {
        setData(data.size() * sizeof(DataT), data.begin(), usage);
    }

private:
    GLint previousBuffer;                  ///< previously bound buffer
    BoundUniformBuffer* previousBufferPtr; ///< previously bound buffer
    BoundUniformBuffer(UniformBuffer* buffer);
    friend class UniformBuffer;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

public:
    BoundUniformBuffer(BoundUniformBuffer&&); // allow move
    ~BoundUniformBuffer();
};
}
