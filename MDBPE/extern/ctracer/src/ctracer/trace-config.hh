#pragma once

#include <ctracer/ChunkAllocator.hh>
#include <ctracer/trace-container.hh>
#include <ctracer/trace.hh>

#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <cstdint>
#include <memory>

namespace ct
{
enum class print_unit
{
    cycles,
    time, // automatic suffix
    seconds,
    milliseconds,
};

/// visitor base class, call order is:
///   -> nested on_trace_start .. on_trace_end
/// traces might not have _end if they are still running
struct visitor
{
    virtual void on_trace_start(location const& /* loc */, uint64_t /* cycles */, uint32_t /* cpu */) {}
    virtual void on_trace_end(uint64_t /* cycles */, uint32_t /* cpu */) {}

    virtual ~visitor() = default;
};

/// returns the total memory consumption of all traced chunks in byte
size_t get_total_memory_consumption();

/// sets the default chunk allocator for new threads (nullptr resets to builtin alloc)
void set_default_allocator(std::shared_ptr<ChunkAllocator> const& allocator);
/// sets the chunk allocator of the current thread (nullptr resets to builtin alloc)
void set_thread_allocator(std::shared_ptr<ChunkAllocator> const& allocator);
/// user-defined name for this thread
void set_thread_name(cc::string name);
/// set the threshhold after which new allocations will trigger a warning
void set_thread_alloc_warn_threshold(uint64_t bytes);

/// returns a trace object for the current thread
trace get_current_thread_trace();
/// returns a trace objects for all finished threads
cc::vector<trace> get_finished_thread_traces();
/// frees memory of finished threads
void clear_finished_thread_traces();

/// calls visitor callbacks for each event in the trace
void visit(trace const& t, visitor& v);

/// writes a csv where all trace points are summarized per-location
void write_summary_csv(cc::string_view filename);
/// Json file for use with https://github.com/jlfwong/speedscope
/// see https://github.com/jlfwong/speedscope/wiki/Importing-from-custom-sources
void write_speedscope_json(cc::string_view filename = "speedscope.json", size_t max_events = 1'000'000);
void write_speedscope_json(trace const& t, cc::string_view filename = "speedscope.json", size_t max_events = 1'000'000);
/// use about:tracing
/// or https://ui.perfetto.dev/
void write_chrome_tracing_json(trace const& t, cc::string_view filename = "chrome-tracing.json", size_t max_events = 1'000'000);

/// prints summary statistics of locations, sorted by time
/// NOTE: currently misleading for recursive locations
void print_location_stats(trace const& t, int max_locs = 10, print_unit unit = print_unit::time);
} // namespace ct
