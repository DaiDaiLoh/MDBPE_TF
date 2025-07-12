#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>

#include <clean-core/vector.hh>

namespace ct
{
struct chunk;

/// a pooled allocator for trace chunks
///
/// this class is thread-safe (can be used for allocating chunks in multiple threads at the same time)
class ChunkAllocator : public std::enable_shared_from_this<ChunkAllocator>
{
public:
    static std::shared_ptr<ChunkAllocator> create(size_t chunk_size = 64 * 1024);
    static std::shared_ptr<ChunkAllocator> global();

    chunk allocate();


private:
    ChunkAllocator(size_t chunk_size) : chunk_size(chunk_size) {}

    void free(uint32_t data[]);
    uint32_t* alloc_data();

    // ref type
    ChunkAllocator(ChunkAllocator const&) = delete;
    ChunkAllocator(ChunkAllocator&&) = delete;
    ChunkAllocator& operator=(ChunkAllocator const&) = delete;
    ChunkAllocator& operator=(ChunkAllocator&&) = delete;

private:
    size_t chunk_size;
    std::mutex mutex;
    cc::vector<std::unique_ptr<uint32_t[]>> free_list;

    friend struct chunk;
};
}
