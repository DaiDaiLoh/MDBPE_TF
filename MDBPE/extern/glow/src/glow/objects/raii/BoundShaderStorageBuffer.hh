#pragma once

#include <vector>

#include <clean-core/span.hh>

#include <glow/common/non_copyable.hh>
#include <glow/fwd.hh>
#include <glow/gl.hh>

namespace glow
{
/// RAII-object that defines a "bind"-scope for an ShaderStorageBuffer
/// All functions that operate on the currently bound buffer are accessed here
struct BoundShaderStorageBuffer
{
    GLOW_RAII_CLASS(BoundShaderStorageBuffer);

    /// Backreference to the buffer
    ShaderStorageBuffer* const buffer;

    /// Sets the data of this uniform buffer (generic version)
    void setData(size_t size, const void* data, GLenum usage = GL_STATIC_DRAW);
    /// Sets the data of this uniform buffer (range version)
    template <class DataRangeT>
    void setData(DataRangeT&& data, GLenum usage = GL_STATIC_DRAW)
    {
        auto bytes = cc::as_byte_span(data);
        setData(bytes.size(), bytes.data(), usage);
    }
    /// Same as above for initializer list
    template <class DataT>
    void setData(std::initializer_list<DataT> data, GLenum usage = GL_STATIC_DRAW)
    {
        setData(data.size() * sizeof(DataT), data.begin(), usage);
    }
    /// Ses a subportion of the buffer (does NOT allocate/enlarge memory!)
    void setSubData(size_t offset, size_t size, const void* data);
    /// vector-of-data version of setSubData
    template <typename DataRangeT>
    void setSubData(size_t offset, DataRangeT&& data)
    {
        auto bytes = cc::as_byte_span(data);
        setSubData(offset, bytes.size(), bytes.data());
    }
    /// Same as above for initializer list
    template <class DataT>
    void setSubData(size_t offset, std::initializer_list<DataT> data)
    {
        setSubData(offset, data.size() * sizeof(DataT), data.begin());
    }
    /// Writes all buffer data into the given memory
    /// Data is truncated to maxSize
    void getData(void* destination, size_t maxSize = 0, bool warnOnTruncate = true);

    /// Reads all data into a vector
    /// Generates an error if (size % sizeof(DataT)) != 0
    /// Optional: if maxCount is bigger than zero, it limits the number of returned elements
    template <typename DataT>
    std::vector<DataT> getData(size_t maxCount = 0)
    {
        auto size = getSize();
        if (!verifyStride(size, sizeof(DataT)))
            return {};
        auto count = size / sizeof(DataT);
        if (maxCount > 0 && maxCount < count)
            count = maxCount;

        std::vector<DataT> data(count);
        getData(data.data(), count * sizeof(DataT), false); // truncating is behavior-by-design
        return data;
    }

    /// Returns a sub region of the buffer
    void getSubData(void* destination, size_t offset, size_t size);
    /// getSubData but with a vector of data types (also size and count not in bytes)
    template <class DataT>
    std::vector<DataT> getSubData(size_t startIdx, size_t count)
    {
        std::vector<DataT> data(count);
        getSubData(data.data(), startIdx * sizeof(DataT), count * sizeof(DataT));
        return data;
    }

    /// Returns the size in bytes of this buffer
    size_t getSize() const;

    /// Reserves a certain buffer size
    /// CAUTION: will probably invalidate all data
    void reserve(size_t sizeInBytes, GLenum usage = GL_STATIC_DRAW);

private:
    GLint previousBuffer;                        ///< previously bound buffer
    BoundShaderStorageBuffer* previousBufferPtr; ///< previously bound buffer
    BoundShaderStorageBuffer(ShaderStorageBuffer* buffer);
    friend class ShaderStorageBuffer;

    bool verifyStride(size_t size, size_t stride) const;

    /// returns true iff it's safe to use this bound class
    /// otherwise, runtime error
    bool isCurrent() const;

public:
    BoundShaderStorageBuffer(BoundShaderStorageBuffer&&); // allow move
    ~BoundShaderStorageBuffer();
};
}
