#pragma once

#include <chrono>
#include <memory>

#include <clean-core/vector.hh>
#include <clean-core/string.hh>

#include "chunk.hh"
#include "detail.hh"

// technically not needed but including scope without trace is unusual
#include "trace-container.hh"
#include "trace.hh"

namespace ct
{
class ChunkAllocator;
struct trace;

/**
 * An arena for TRACE calls
 * All calls made when scope is valid are directed into its local trace and not into the global trace
 *
 * NOTE: scopes are NOT thread-safe. they must be used with C++ scopes and not concurrently accessed.
 *
 * Usage:
 *
 *   ct::scope s;
 *
 *   TRACE(...); // recorded into s
 *
 *   print(s.trace()); // show its trace
 */
struct scope
{
    using time_point = std::chrono::high_resolution_clock::time_point;

    /// creates a new scope and optionally specifies a custom allocator
    scope(cc::string name = "", std::shared_ptr<ChunkAllocator> const& allocator = nullptr);
    ~scope();

    // raii type
    scope(scope&&) = delete;
    scope(scope const&) = delete;
    scope& operator=(scope&&) = delete;
    scope& operator=(scope const&) = delete;

    /// creates a trace object (NOTE: copies chunk data)
    ct::trace trace() const;

    /// returns the trace name (either thread name or scope name)
    cc::string const& name() const { return _name; }

    bool is_null_scope() const { return _is_null_scope; }

    /// number of bytes after which new allocations will emit a warning
    void set_alloc_warn_threshold(uint64_t bytes) { _warn_bytes = bytes; }
    uint64_t alloc_warn_threshold() const { return _warn_bytes; }

    /// number of currently allocated bytes inside this scope, excluding nested scopes
    uint64_t allocated_bytes() const { return _allocated_bytes; }

protected:
    struct null_scope_tag
    {
    };

    scope(null_scope_tag) : scope() { _is_null_scope = true; }

private:
    cc::string _name;
    std::shared_ptr<ChunkAllocator> _allocator;
    cc::vector<chunk> _chunks;

    time_point _time_start;
    uint64_t _cycles_start;
    uint64_t _allocated_bytes = 0;
    uint64_t _warn_bytes = 1 << 30; // 1GiB

    bool _is_null_scope = false;
    bool _orphaned = false;

    // TODO: bool if orphaned scope

    friend uint32_t* detail::alloc_chunk();
    friend void detail::mark_as_orphaned(scope& s);
    friend void detail::pop_scope(scope&);
    friend void set_thread_name(cc::string name);
    friend void set_thread_allocator(std::shared_ptr<ChunkAllocator> const& allocator);
};

struct null_scope : private scope
{
    null_scope() : scope(null_scope_tag{}) {}
};


}
