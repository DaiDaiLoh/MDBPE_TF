#pragma once

#include <chrono>
#include <limits>
#include <type_traits>

#include <clean-core/is_range.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

// TODO: replace by cc once tuple conversion is added
#include <tuple>

#include "trace.hh"

/*
 * Benchmarking functionality
 *
 * Usage:
 *   int foo(int) { ... }
 *   auto res = ct::benchmark(foo, 1);
 *   res.print_summary();
 *
 *   ct::benchmark([](int a, int b) { return a % b; }, 17, 5);
 *
 * How to prevent optimization (manual version):
 *   ct::sink << x; // writes value to volatile var (guaranteed write)
 *   auto const s = ct::source(0.0f);
 *   float x = s; // reads 0.0f from a volatile var (guaranteed read)
 *
 *   (outputs are automatically passed to the sink)
 *   (inputs are automatically read from a source)
 *
 *   Example:
 *     auto s = ct::source(0.0f);
 *     ct::benchmark([&] {
 *        float a = s;
 *        float b = s;
 *        ct::sink << a + b;
 *     });
 *
 * How to extend sink and source for custom types:
 *
 *     namespace ct
 *     {
 *     sink_t operator<<(sink_t s, vec3 const& v) { return s << v.x << v.y << v.z; }
 *     }
 *
 *     template <>
 *     struct ct::source<vec3>
 *     {
 *         explicit source(vec3 v = {}) : x(v.x), y(v.y), z(v.z) {}
 *         operator vec3() const { return {x, y, z}; }
 *
 *     private:
 *         source<float> x, y, z;
 *     };
 *
 * FIXME: gcc is misbehaving (https://godbolt.org/z/0HR2bL)
 */

namespace ct
{
struct sink_t
{
    template <class T>
    sink_t operator<<(T const& v) const
    {
        if constexpr (cc::is_any_range<T>)
        {
            for (auto const& e : v)
                *this << e;
        }
        else
        {
            [[maybe_unused]] volatile T s;
            s = v;
            return *this;
        }
    }
};

static constexpr sink_t sink;

template <class T>
struct source
{
    explicit source(T v = {}) : value(v) {}
    operator T() const { return value; }

private:
    volatile T value;
};

struct benchmark_results
{
    struct timing
    {
        int samples = 0;
        uint64_t cycles = 0;
        double seconds = 0;
    };

    cc::vector<timing> experiments;
    cc::vector<timing> warmups;
    cc::vector<timing> baselines;

    void print_all(cc::string_view prefix = "") const;
    void print_summary(cc::string_view prefix = "") const;

    double seconds_per_sample(float percentile = 0.0f) const;
    double cycles_per_sample(float percentile = 0.0f) const;
    double baseline_seconds_per_sample() const;
    double baseline_cycles_per_sample() const;
};

template <class F, class... Args>
benchmark_results benchmark(F&& f, Args... args)
{
    static_assert(std::is_invocable_v<F, Args...>, "f is not invocable with the provided parameters");
    using R = std::invoke_result_t<F, Args...>;
    static_assert(std::is_same_v<R, void> || std::is_default_constructible_v<R>, "requires default-constructible return type of f");

    auto args_src = std::tuple{source<Args>(args)...};
    auto args_in = std::tuple{args...};

    auto constexpr initial_check_cnt = 3;
    auto constexpr extra_long_cycles = 100'000'000; // above this only initial check is executed
    auto constexpr long_cycles = 1'000'000;         // above this a few individual runs are performed
    auto constexpr medium_cycles = 10000;           // above this a few clustered runs are performed
    auto constexpr short_cycles = 500;              // above this a some clustered runs are performed

    auto constexpr long_run_cnt = 5;
    auto constexpr long_cluster_cnt = 1;

    auto constexpr medium_run_cnt = 5;
    auto constexpr medium_cluster_cnt = 5;

    auto constexpr short_run_cnt = 15;
    auto constexpr short_cluster_cnt = 200;

    auto constexpr very_short_run_cnt = 30;
    auto constexpr very_short_cluster_cnt = 3000;

    auto constexpr baseline_run_cnt = 30;
    auto constexpr baseline_cluster_cnt = 3000;

    benchmark_results res;

    auto const execute = [&]
    {
        // read inputs from sources
        args_in = args_src;

        // execute function
        if constexpr (std::is_same_v<R, void>)
            std::apply(f, args_in);
        else // sinking result
            sink << std::apply(f, args_in);
    };

    auto const baseline = [&]
    {
        args_in = args_src; // read
        if constexpr (!std::is_same_v<R, void>)
            sink << R{}; // write
    };

    auto const time = [&](auto&& code, int count) -> benchmark_results::timing
    {
        auto t_start = std::chrono::high_resolution_clock::now();
        auto c_start = ct::current_cycles();
        for (auto i = 0; i < count; ++i)
            code();
        auto c_end = ct::current_cycles();
        auto t_end = std::chrono::high_resolution_clock::now();
        return {count, c_end - c_start, std::chrono::duration<double>(t_end - t_start).count()};
    };

    benchmark_results::timing t_init;

    // gauge function running time
    {
        auto c_min = std::numeric_limits<uint64_t>::max();
        auto i_min = 0;
        for (auto i = 0; i < initial_check_cnt; ++i)
        {
            auto t = time(execute, 1);
            res.warmups.push_back(t);

            if (t.cycles < c_min)
            {
                c_min = t.cycles;
                i_min = i;
            }
        }
        t_init = res.warmups[size_t(i_min)];
    }

    // function takes too long to do more than one run
    if (t_init.cycles > extra_long_cycles)
    {
        res.experiments.push_back(t_init);
    }
    // a few individual runs are ok
    else if (t_init.cycles > long_cycles)
    {
        res.experiments.push_back(t_init);
        for (auto i = 0; i < long_run_cnt; ++i)
            res.experiments.push_back(time(execute, long_cluster_cnt));
    }
    // a few clustered runs
    else if (t_init.cycles > medium_cycles)
    {
        for (auto i = 0; i < medium_run_cnt; ++i)
            res.experiments.push_back(time(execute, medium_cluster_cnt));
    }
    // some clustered runs
    else if (t_init.cycles > short_cycles)
    {
        for (auto i = 0; i < short_run_cnt; ++i)
            res.experiments.push_back(time(execute, short_cluster_cnt));
    }
    // heavily clustered runs
    else
    {
        for (auto i = 0; i < very_short_run_cnt; ++i)
            res.experiments.push_back(time(execute, very_short_cluster_cnt));
    }

    // baseline
    for (auto i = 0; i < baseline_run_cnt; ++i)
        res.baselines.push_back(time(baseline, baseline_cluster_cnt));

    return res;
}
}
