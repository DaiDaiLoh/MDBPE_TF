#pragma once

#include <cstdint>

#include <clean-core/macros.hh>

#ifdef _MSC_VER
#include <intrin.h>
#endif

/**
 * Main Macro: TRACE(...)
 *
 * See https://godbolt.org/z/qs33MN
 *
 * Usage:
 *   int foo() {
 *      TRACE();
 *
 *      return do_stuff();
 *   }
 *
 * Overhead: ~70-105 cycles
 *
 * TODO: low-overhead version without alloc_chunk check (~80 cycles)
 *
 * Explicit non-scope version:
 *    TRACE_BEGIN("some optional name");
 *    ... code ...
 *    TRACE_END();
 *
 *    NOTE: proper nesting must be respected
 */
#define TRACE(...)                                                                                                                 \
    (void)__VA_ARGS__ " has to be a string literal";                                                                               \
    static constexpr ct::location CC_MACRO_JOIN(_ct_trace_label, __LINE__) = {__FILE__, CC_PRETTY_FUNC, "" __VA_ARGS__, __LINE__}; \
    ct::detail::raii_tracer CC_MACRO_JOIN(_ct_trace_, __LINE__)(&CC_MACRO_JOIN(_ct_trace_label, __LINE__))

#define TRACE_BEGIN(...)                                                                                                           \
    (void)__VA_ARGS__ " has to be a string literal";                                                                               \
    static constexpr ct::location CC_MACRO_JOIN(_ct_trace_label, __LINE__) = {__FILE__, CC_PRETTY_FUNC, "" __VA_ARGS__, __LINE__}; \
    ct::detail::trace_begin(&CC_MACRO_JOIN(_ct_trace_label, __LINE__))

#define TRACE_END() ct::detail::trace_end()


// Implementation:

#define CTRACER_TRACE_SIZE 9
#define CTRACER_END_VALUE 0xFFFFFFFF

namespace ct
{
struct location
{
    char const* file;
    char const* function;
    char const* name;
    int line;
};

#ifdef _WIN32
CC_FORCE_INLINE uint64_t current_cycles() { return __rdtsc(); }
#else //  Linux/GCC
CC_FORCE_INLINE uint64_t current_cycles()
{
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}
#endif

namespace detail
{
struct thread_data
{
    uint32_t* curr;
    uint32_t* end; ///< not actually end, has a CTRACER_TRACE_SIZE buffer at the end
};

/// allocates a new chunk, returns "curr" and updates tdata()
CC_COLD_FUNC CC_DONT_INLINE uint32_t* alloc_chunk();

CC_FORCE_INLINE thread_data& tdata()
{
    static thread_local thread_data data = {nullptr, nullptr};
    return data;
}

CC_FORCE_INLINE void trace_begin(location const* loc)
{
    auto pd = tdata().curr;
    if CC_CONDITION_UNLIKELY (pd >= tdata().end) // alloc new chunk
        pd = alloc_chunk();
    tdata().curr = pd + 5;

    *(location const**)pd = loc;

    unsigned int core;
#ifdef _MSC_VER
    int64_t cc = __rdtscp(&core);
    *(int64_t*)(pd + 2) = cc;
#else
    unsigned int lo, hi;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(core));
    pd[2] = lo;
    pd[3] = hi;
#endif
    pd[4] = core;
}

CC_FORCE_INLINE void trace_end()
{
    auto pd = tdata().curr;
    if CC_CONDITION_UNLIKELY (pd >= tdata().end) // alloc new chunk
        pd = alloc_chunk();
    tdata().curr = pd + 4;

    unsigned int core;
#ifdef _MSC_VER
    int64_t cc = __rdtscp(&core);
    pd[0] = CTRACER_END_VALUE;
    *(int64_t*)(pd + 1) = cc;
#else
    unsigned int lo, hi;
    __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(core));
    pd[0] = CTRACER_END_VALUE;
    pd[1] = lo;
    pd[2] = hi;
#endif
    pd[3] = core;
}

struct raii_tracer
{
    CC_FORCE_INLINE raii_tracer(location const* loc) { trace_begin(loc); }
    CC_FORCE_INLINE ~raii_tracer() { trace_end(); }
};
} // namespace detail

// small utility
struct cycler
{
    uint64_t c_start = ct::current_cycles();
    CC_FORCE_INLINE uint64_t elapsed_cycles() const { return ct::current_cycles() - c_start; }
};

} // namespace ct
