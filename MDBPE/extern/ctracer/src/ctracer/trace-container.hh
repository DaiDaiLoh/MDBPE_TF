#pragma once

#include <chrono>
#include <cstdint>

#include <clean-core/function_ref.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

namespace ct
{
struct visitor;
struct location;

struct event
{
    location const* loc = nullptr;
    uint64_t cycles = 0;
    uint32_t cpu = 0;
    bool enter = false;
};

struct event_scope
{
    location const* loc = nullptr;
    uint64_t start_cycles = 0;
    uint64_t end_cycles = 0;
    uint32_t start_cpu = 0;
    uint32_t end_cpu = 0;

    uint64_t cycles() const { return end_cycles - start_cycles; }
};

struct location_stats
{
    location const* loc = nullptr;
    int samples = 0;
    uint64_t total_cycles = 0;
};

/// An opaque value type representing a hierarchical call trace of TRACEs.
/// Not all TRACEs might be closed because traces can be queried in-between
struct trace
{
public:
    using time_point = std::chrono::high_resolution_clock::time_point;

    cc::string const& name() const { return _name; }

    /// convenience function that visits this trace and converts it into event form
    cc::vector<event> compute_events() const;
    /// convenience function that visits this trace and converts it into scoped event form
    /// NOTE: order is a post-order tree traversal
    cc::vector<event_scope> compute_event_scopes() const;
    /// convenience function that visits this trace and computes per-location stats
    cc::vector<location_stats> compute_location_stats() const;

    time_point time_start() const { return _time_start; }
    time_point time_end() const { return _time_end; }
    uint64_t cycles_start() const { return _cycles_start; }
    uint64_t cycles_end() const { return _cycles_end; }
    float elapsed_seconds() const { return std::chrono::duration<float>(_time_end - _time_start).count(); }
    uint64_t elapsed_cycles() const { return _cycles_end - _cycles_start; }

    bool empty() const { return _data.empty(); }

    // builder
public:
    trace() = default;
    trace(cc::string name, cc::vector<uint32_t> data, time_point time_start, time_point time_end, uint64_t cycles_start, uint64_t cycles_end);

    void add_start(location const& loc, uint64_t cycles, uint32_t cpu);
    void add_end(uint64_t cycles, uint32_t cpu);

    void add(trace const& t);

private:
    cc::string _name;
    cc::vector<uint32_t> _data;

    // timing of the whole trace
    // time points can be used to calibrate cycles <-> seconds
    time_point _time_start;
    time_point _time_end;
    uint64_t _cycles_start;
    uint64_t _cycles_end;

    friend struct scope;
    friend void visit(trace const& t, visitor& v);
};

/// returns a filtered version of the given trace
/// the new trace only contains samples where predicate was true for the sample or any parent
/// (e.g. useful to restrict to subscopes)
/// NOTE: time/cycles start/end is the same as input
trace filter_subscope(trace const& t, cc::function_ref<bool(location const&)> predicate);

/// returns a new trace where all cpu values are replaced with a given value
trace map_cpu(trace const& t, uint32_t new_cpu);
}
