#include <ctracer/trace-config.hh>

#include "ChunkAllocator.hh"
#include "chunk.hh"
#include "detail.hh"
#include "scope.hh"
#include "trace-container.hh"

#include <clean-core/assert.hh>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

using namespace ct;

namespace
{
struct
{
    std::mutex mutex;
    std::shared_ptr<ChunkAllocator> allocator;
    cc::vector<std::unique_ptr<scope>> finished_threads;
} _global;

thread_local struct thread_info
{
    bool is_initialized = false;
    std::unique_ptr<scope> root_scope;
    cc::vector<scope*> scope_stack;
    cc::vector<detail::thread_data> tdata_stack;
    scope* current_scope = nullptr;
    chunk* current_chunk = nullptr;

    ~thread_info()
    {
        if (root_scope)
        {
            CC_ASSERT(scope_stack.size() == 1 && "only root scope should be alive");
            CC_ASSERT(tdata_stack.size() == 1 && "only root scope should be alive");

            // make sure it's dtor is not called
            detail::mark_as_orphaned(*root_scope);

            _global.mutex.lock();
            _global.finished_threads.emplace_back(std::move(root_scope));
            _global.mutex.unlock();
        }
    }
} _thread;

void init_thread()
{
    if (_thread.is_initialized)
        return; // already initialized
    _thread.is_initialized = true;

    _global.mutex.lock();
    auto alloc = _global.allocator;
    _global.mutex.unlock();

    std::stringstream ss;
    ss << std::this_thread::get_id();

    // also pushes the scope
    _thread.root_scope = std::make_unique<scope>(ss.str().c_str(), alloc);
}
} // namespace

namespace ct
{
void detail::mark_as_orphaned(scope& s)
{
    s._allocator = nullptr;
    s._orphaned = true;
}

void detail::push_scope(scope& s)
{
    init_thread();

    _thread.scope_stack.push_back(&s);
    _thread.current_scope = &s;

    // allocate chunk into current scope and change tdata()
    _thread.tdata_stack.push_back(tdata());
    _thread.current_chunk = nullptr;
    alloc_chunk();
}
void detail::pop_scope(scope& s)
{
    CC_ASSERT(_thread.scope_stack.size() >= 2 && "corrupted scope stack");
    CC_ASSERT(_thread.scope_stack.back() == &s && "corrupted scope stack");
    CC_ASSERT(_thread.tdata_stack.size() == _thread.scope_stack.size() && "corrupted tdata stack");

    // ensure correct size
    update_current_chunk_size();

    // restore current scope and tdata
    _thread.scope_stack.pop_back();
    _thread.current_scope = _thread.scope_stack.back();
    tdata() = _thread.tdata_stack.back();
    _thread.tdata_stack.pop_back();

    // set current chunk
    _thread.current_chunk = &_thread.current_scope->_chunks.back();
}
void detail::update_current_chunk_size()
{
    if (_thread.current_chunk == nullptr)
        return;

    _thread.current_chunk->_size = tdata().curr - _thread.current_chunk->data();
    CC_ASSERT(_thread.current_chunk->_size <= _thread.current_chunk->_capacity && "corrupted chunk");
}

void set_default_allocator(std::shared_ptr<ChunkAllocator> const& allocator)
{
    _global.mutex.lock();
    _global.allocator = allocator ? allocator : ChunkAllocator::global();
    _global.mutex.unlock();
}

void set_thread_allocator(std::shared_ptr<ChunkAllocator> const& allocator)
{
    init_thread();

    _thread.root_scope->_allocator = allocator ? allocator : ChunkAllocator::global();
}

void set_thread_alloc_warn_threshold(uint64_t bytes)
{
    init_thread();

    _thread.root_scope->set_alloc_warn_threshold(bytes);
}

void set_thread_name(cc::string name)
{
    init_thread();

    _thread.root_scope->_name = cc::move(name);
}

trace get_current_thread_trace()
{
    init_thread();

    return _thread.root_scope->trace();
}

cc::vector<trace> get_finished_thread_traces()
{
    cc::vector<trace> traces;
    _global.mutex.lock();
    for (auto const& s : _global.finished_threads)
        traces.emplace_back(s->trace());
    _global.mutex.unlock();
    return traces;
}

void clear_finished_thread_traces()
{
    _global.mutex.lock();
    _global.finished_threads.clear();
    _global.mutex.unlock();
}

uint32_t* detail::alloc_chunk()
{
    // new thread: register it
    init_thread();

    // ensure previous chunk has correct size
    update_current_chunk_size();

    // allocate and register chunk
    auto& s = *_thread.current_scope;

    chunk* c;
    if (!s.is_null_scope() || s._chunks.empty())
    {
        s._chunks.emplace_back(s._allocator->allocate());
        c = &s._chunks.back();
        s._allocated_bytes += c->capacity();
        if (s.alloc_warn_threshold() < s.allocated_bytes())
            std::cerr << "[ctracer] Scope allocates more than " << s.alloc_warn_threshold() << " bytes!\n";
    }
    else
    {
        c = &s._chunks.back();
    }

    CC_ASSERT(c->data() && "invalid chunk");
    CC_ASSERT(c->capacity() > 100 + CTRACER_TRACE_SIZE && "chunk too small");
    _thread.current_chunk = c;

    // update tdata()
    auto& td = tdata();
    td.curr = c->data();
    td.end = c->data() + c->capacity() - CTRACER_TRACE_SIZE;

    // return curr
    return td.curr;
}

void visit(trace const& t, visitor& v)
{
    auto idx = 0u;
    auto const& d = t._data;

    // get next word
    auto get = [&]() -> uint32_t {
        if (idx >= d.size())
            return 0;
        return d[idx++];
    };

    while (true)
    {
        auto v0 = get();
        if (v0 == 0x0)
            return; // rest is not done

        if (v0 != CTRACER_END_VALUE)
        {
            auto v1 = get();
            auto loc = (location const*)(((uint64_t)v1 << 32uLL) | v0);
            auto lo = get();
            auto hi = get();
            auto cpu = get();
            auto cycles = ((uint64_t)hi << 32) | lo;
            v.on_trace_start(*loc, cycles, cpu);
        }
        else
        {
            auto lo = get();
            auto hi = get();
            auto cpu = get();
            auto cycles = ((uint64_t)hi << 32) | lo;
            v.on_trace_end(cycles, cpu);
        }
    }
}
} // namespace ct
