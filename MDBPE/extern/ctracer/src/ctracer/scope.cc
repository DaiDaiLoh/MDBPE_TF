#include "scope.hh"

#include <cstring>

#include "ChunkAllocator.hh"
#include "detail.hh"
#include "trace-container.hh"

using namespace ct;

scope::scope(cc::string name, std::shared_ptr<ChunkAllocator> const& allocator)
  : _name(cc::move(name)), _allocator(allocator ? allocator : ChunkAllocator::global())
{
    // after this call all TRACE(...)s are directed into this scope
    ct::detail::push_scope(*this);

    _time_start = std::chrono::high_resolution_clock::now();
    _cycles_start = ct::current_cycles();
}

scope::~scope()
{
    // restores previous scope
    if (!_orphaned) // if not scope of finished thread
        ct::detail::pop_scope(*this);
}

ct::trace scope::trace() const
{
    auto time_end = std::chrono::high_resolution_clock::now();
    auto cycles_end = ct::current_cycles();

    // ensure that all chunk size are correct
    ct::detail::update_current_chunk_size();

    // precompute final size
    size_t cnt = 0;
    for (auto const& c : _chunks)
        cnt += c.size();

    // copy trace into preallocated data
    cc::vector<uint32_t> data;
    data.resize(cnt);
    size_t idx = 0;
    for (auto const& c : _chunks)
    {
        std::memcpy(data.data() + idx, c.data(), c.size() * sizeof(uint32_t));
        idx += c.size();
    }

    // return trace
    return ct::trace(_name, move(data), _time_start, time_end, _cycles_start, cycles_end);
}
