#pragma once

#include <cstddef>
#include <cstdint>

// TODO: replace by cc:: something in the future
#include <memory>

namespace ct
{
class ChunkAllocator;

namespace detail
{
void update_current_chunk_size();
}

/// an opaque block of memory for storing trace data
struct chunk
{
    uint32_t* data() { return _data; }
    uint32_t const* data() const { return _data; }
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    bool is_allocated() const { return _data != nullptr; }

    chunk() = default; // empty chunk

    // non-copyable
    chunk(chunk const&) = delete;
    chunk& operator=(chunk const&) = delete;

    // movable
    chunk(chunk&& c) noexcept
    {
        _data = c._data;
        _size = c._size;
        _capacity = c._capacity;
        _allocator = c._allocator;

        c._data = nullptr;
        c._size = 0;
        c._capacity = 0;
        c._allocator.reset();
    }
    chunk& operator=(chunk&& c) noexcept
    {
        if (this != &c)
        {
            free();

            _data = c._data;
            _size = c._size;
            _capacity = c._capacity;
            _allocator = c._allocator;

            c._data = nullptr;
            c._size = 0;
            c._capacity = 0;
            c._allocator.reset();
        }

        return *this;
    }

    ~chunk() { free(); }

    /// NOTE: must not be allocated
    void allocate(std::shared_ptr<ChunkAllocator> const& allocator);
    /// NOTE: works for non-allocated chunks
    void free();

private:
    uint32_t* _data = nullptr;
    size_t _capacity = 0;
    size_t _size = 0;
    std::weak_ptr<ChunkAllocator> _allocator;

    friend class ChunkAllocator;
    friend void detail::update_current_chunk_size();
};
}
