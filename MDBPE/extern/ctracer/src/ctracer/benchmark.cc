#include "benchmark.hh"

// TODO: remove me in some future cleanup
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

static std::string time_str(double s)
{
    std::stringstream ss;

    if (s >= 1)
        ss << std::setprecision(4) << s << " sec";
    else if (s >= 1 / 1000.)
        ss << std::setprecision(4) << s * 1000 << " ms";
    else if (s >= 1 / 1000. / 1000.)
        ss << std::setprecision(4) << s * 1000 * 1000 << " us";
    else
        ss << std::setprecision(4) << s * 1000 * 1000 * 1000 << " ns";

    return ss.str();
}

static bool comp_by_seconds(ct::benchmark_results::timing const& a, ct::benchmark_results::timing const& b) { return a.seconds < b.seconds; }
static bool comp_by_cycles(ct::benchmark_results::timing const& a, ct::benchmark_results::timing const& b) { return a.cycles < b.cycles; }

void ct::benchmark_results::print_all(cc::string_view prefix) const
{
    auto s_prefix = std::string(cc::string(prefix).c_str());

    auto const print = [&](timing const& t)
    { std::cout << s_prefix << "  " << t.cycles << " cycles, " << time_str(t.seconds) << ", " << t.samples << " sample(s)" << std::endl; };

    std::cout << s_prefix << "experiments:" << std::endl;
    for (auto const& t : experiments)
        print(t);
    std::cout << s_prefix << "warmup:" << std::endl;
    for (auto const& t : warmups)
        print(t);
    if (!baselines.empty())
    {
        std::cout << s_prefix << "baseline:" << std::endl;
        for (auto const& t : baselines)
            print(t);
    }
}

void ct::benchmark_results::print_summary(cc::string_view prefix) const
{
    auto const bsps = baseline_seconds_per_sample();
    auto const bcps = baseline_cycles_per_sample();
    auto const sps_min = std::max(0.0, seconds_per_sample() - bsps);
    auto const cps_min = std::max(0.0, cycles_per_sample() - bcps);
    auto const sps_max = seconds_per_sample(0.7f);
    auto const cps_max = cycles_per_sample(0.7f);
    std::cout << cc::string(prefix).c_str() << time_str(sps_min) << " .. " << time_str(sps_max) << " / sample, " << cps_min << " .. " << cps_max
              << " cycles / sample" << std::endl;
}

double ct::benchmark_results::seconds_per_sample(float percentile) const
{
    if (experiments.empty())
        return -1;

    auto const n = std::min(experiments.size() - 1, size_t(std::ceil(experiments.size() * percentile)));
    auto exp = experiments; // copy
    std::nth_element(exp.begin(), exp.begin() + n, exp.end(), comp_by_seconds);
    auto const t = exp[n];
    return t.seconds / static_cast<double>(t.samples);
}

double ct::benchmark_results::cycles_per_sample(float percentile) const
{
    if (experiments.empty())
        return -1;

    auto const n = std::min(experiments.size() - 1, size_t(std::ceil(experiments.size() * percentile)));
    auto exp = experiments; // copy
    std::nth_element(exp.begin(), exp.begin() + n, exp.end(), comp_by_cycles);
    auto const t = exp[n];
    return t.cycles / static_cast<double>(t.samples);
}

double ct::benchmark_results::baseline_seconds_per_sample() const
{
    if (baselines.empty())
        return 0;

    auto const t = std::min_element(baselines.begin(), baselines.end(), comp_by_seconds);
    return t->seconds / t->samples;
}

double ct::benchmark_results::baseline_cycles_per_sample() const
{
    if (baselines.empty())
        return 0;

    auto const t = std::min_element(baselines.begin(), baselines.end(), comp_by_cycles);
    return t->cycles / double(t->samples);
}
