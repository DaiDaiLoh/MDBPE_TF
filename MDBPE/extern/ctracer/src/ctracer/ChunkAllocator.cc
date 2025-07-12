#include "ChunkAllocator.hh"

#include <atomic>
#include <cassert>

#include "chunk.hh"
#include "trace-config.hh"

static std::atomic<size_t> _total_memory = 0;

using namespace ct;

size_t ct::get_total_memory_consumption() { return _total_memory.load(); }

void chunk::allocate(std::shared_ptr<ChunkAllocator> const& allocator)
{
    assert(!is_allocated() && "cannot allocate an already allocated chunk");
    assert(allocator && "allocator must not be null");

    _data = allocator->alloc_data();
    _size = 0;
    _capacity = allocator->chunk_size;
    _allocator = allocator->shared_from_this();
}

uint32_t* ChunkAllocator::alloc_data()
{
    {
        std::scoped_lock l(mutex);
        if (!free_list.empty())
        {
            auto d = free_list.back().release();
            free_list.pop_back();
            return d;
        }
    }

    _total_memory.fetch_add(chunk_size * sizeof(uint32_t));
    return new uint32_t[chunk_size];
}

void chunk::free()
{
    if (!_data)
        return;

    auto a = _allocator.lock();

    if (a) // allocator still valid
        a->free(_data);
    else
    {
        _total_memory.fetch_sub(_capacity * sizeof(uint32_t));
        delete[] _data;
    }

    _data = nullptr;
    _size = 0;
    _capacity = 0;
    _allocator.reset();
}

std::shared_ptr<ChunkAllocator> ChunkAllocator::create(size_t chunk_size) { return std::shared_ptr<ChunkAllocator>(new ChunkAllocator(chunk_size)); }

std::shared_ptr<ChunkAllocator> ChunkAllocator::global()
{
    static std::shared_ptr<ChunkAllocator> a = create();
    return a;
}

chunk ChunkAllocator::allocate()
{
    chunk c;
    c.allocate(shared_from_this());
    return c;
}

void ChunkAllocator::free(uint32_t data[])
{
    mutex.lock();
    free_list.emplace_back(data);
    mutex.unlock();
}
