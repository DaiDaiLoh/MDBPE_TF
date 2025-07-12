#include <ctracer/trace-config.hh>

#include <clean-core/format.hh>

#include <algorithm>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include <fstream>

namespace ct
{
static cc::string format_cycles(double cycles, double to_sec_factor, print_unit unit)
{
    switch (unit)
    {
    case print_unit::cycles:
        return cc::format("{} cc", cycles);
    case print_unit::seconds:
        return cc::format("{:.4} s", cycles * to_sec_factor);
    case print_unit::milliseconds:
        return cc::format("{:.4} ms", cycles * to_sec_factor * 1000);
    case print_unit::time:
    {
        auto s = cycles * to_sec_factor;
        if (s < 1999e-9)
            return cc::format("{:.4} ns", s * 1e9);
        else if (s < 1999e-6)
            return cc::format("{:.4} us", s * 1e6);
        else if (s < 1999e-3)
            return cc::format("{:.4} ms", s * 1e3);
        else
            return cc::format("{:.4} s", s);
    }
    }
    return "<invalid unit>";
}

static std::string beautify_function_name(std::string const& name)
{
    auto p = name.rfind(')');
    if (p == std::string::npos) // no (..)
    {
        p = name.rfind(' ');
        if (p == std::string::npos) // no space
            return name;
        return name.substr(p + 1); // "void foo" -> "foo"
    }

    int i = int(p);
    int db = 0;
    int da = 0;
    int cc = 0;
    while (i >= 0)
    {
        if (name[i] == ')')
            ++db;
        if (name[i] == '>')
            ++da;
        if (name[i] == '<')
            --da;
        if (name[i] == '(')
            --db;

        if (name[i] == ':' && da == 0 && db == 0)
        {
            ++cc;
            if (cc > 2)
                break;
        }

        if (name[i] == ' ' && da == 0 && db == 0)
            break;

        --i;
    }

    if (i < 0)
        return name;

    return name.substr(i + 1);
}

void write_speedscope_json(cc::string_view filename, size_t max_events)
{
    return write_speedscope_json(ct::get_current_thread_trace(), filename, max_events);
}

void write_speedscope_json(trace const& tr, cc::string_view filename, size_t max_events)
{
    std::ofstream out(cc::string(filename).c_str());
    if (!out.good())
        return;

    struct event
    {
        char type;
        int frame;
        uint64_t at;
    };
    struct stack_entry
    {
        int frame;
    };

    struct visitor : ct::visitor
    {
        uint64_t min_cycles = std::numeric_limits<uint64_t>::max();
        uint64_t max_cycles = 0;
        uint32_t last_cpu = 0;
        std::unordered_map<location const*, int> frames;
        std::vector<location const*> locations;
        std::vector<stack_entry> stack;
        std::vector<event> events;

        int frame_of(location const& loc)
        {
            auto it = frames.find(&loc);
            if (it != frames.end())
                return it->second;

            auto f = int(frames.size());
            frames[&loc] = f;
            locations.push_back(&loc);
            return f;
        }

        void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t cpu) override
        {
            last_cpu = cpu;
            min_cycles = std::min(min_cycles, cycles);
            max_cycles = std::max(max_cycles, cycles);

            auto f = frame_of(loc);
            events.push_back({'O', f, cycles});

            stack.push_back({f});
        }
        void on_trace_end(uint64_t cycles, uint32_t cpu) override
        {
            last_cpu = cpu;
            min_cycles = std::min(min_cycles, cycles);
            max_cycles = std::max(max_cycles, cycles);

            auto se = stack.back();
            stack.pop_back();

            events.push_back({'C', se.frame, cycles});
        }

        void close_pending_actions()
        {
            while (!stack.empty())
                on_trace_end(max_cycles, last_cpu);
        }
    };
    visitor v;
    visit(tr, v);
    v.close_pending_actions();

    if (v.events.size() > max_events)
    {
        std::cerr << "Not writing speedscope json, too many events (" << v.events.size() << ")" << std::endl;
        return;
    }

    auto const to_sec = tr.elapsed_seconds() / tr.elapsed_cycles();

    out << "{";
    out << "\"version\":\"0.0.1\",";
    out << "\"$schema\": \"https://www.speedscope.app/file-format-schema.json\",";
    out << "\"shared\":{";
    out << "\"frames\":[";
    for (auto i = 0u; i < v.locations.size(); ++i)
    {
        auto loc = v.locations[i];
        if (i > 0)
            out << ",";
        out << "{";
        out << "\"name\":\"" << (std::string(loc->name).empty() ? beautify_function_name(loc->function) : loc->name) << "\",";
        std::string escapedFilePath = loc->file;
        std::replace(escapedFilePath.begin(), escapedFilePath.end(), '\\', '/');
        out << "\"file\":\"" << escapedFilePath << "\",";
        out << "\"line\":" << loc->line << "";
        out << "}";
    }
    out << "]";
    out << "},";
    out << "\"profiles\":[{";
    out << "\"type\":\"evented\",";
    out << "\"name\":\"ctracer\",";
    out << "\"unit\":\"seconds\","; // current version does not support 'none' anymore
    out << "\"startValue\":0,";
    out << "\"endValue\":" << (v.max_cycles - v.min_cycles) * to_sec << ",";
    out << "\"events\":[";
    auto first = true;
    for (auto const& e : v.events)
    {
        if (!first)
            out << ",";
        first = false;
        out << "{";
        out << "\"type\":\"" << e.type << "\",";
        out << "\"frame\":" << e.frame << ",";
        out << "\"at\":" << (e.at - v.min_cycles) * to_sec;
        out << "}";
    }
    out << "]";
    out << "}]";
    out << "}";
}

void write_chrome_tracing_json(trace const& tr, cc::string_view filename, size_t max_events)
{
    std::ofstream out(cc::string(filename).c_str());
    if (!out.good())
        return;

    struct event
    {
        char type;
        int frame;
        uint64_t at;
        uint32_t cpu;
    };
    struct stack_entry
    {
        int frame;
    };

    struct visitor : ct::visitor
    {
        uint64_t min_cycles = std::numeric_limits<uint64_t>::max();
        uint64_t max_cycles = 0;
        uint32_t last_cpu = 0;
        std::unordered_map<location const*, int> frames;
        std::vector<location const*> locations;
        std::vector<stack_entry> stack;
        std::vector<event> events;

        int frame_of(location const& loc)
        {
            auto it = frames.find(&loc);
            if (it != frames.end())
                return it->second;

            auto f = int(frames.size());
            frames[&loc] = f;
            locations.push_back(&loc);
            return f;
        }

        void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t cpu) override
        {
            last_cpu = cpu;
            min_cycles = std::min(min_cycles, cycles);
            max_cycles = std::max(max_cycles, cycles);

            auto f = frame_of(loc);
            events.push_back({'B', f, cycles, cpu});

            stack.push_back({f});
        }
        void on_trace_end(uint64_t cycles, uint32_t cpu) override
        {
            last_cpu = cpu;
            min_cycles = std::min(min_cycles, cycles);
            max_cycles = std::max(max_cycles, cycles);

            auto se = stack.back();
            stack.pop_back();

            events.push_back({'E', se.frame, cycles, last_cpu});
        }

        void close_pending_actions()
        {
            while (!stack.empty())
                on_trace_end(max_cycles, last_cpu);
        }
    };
    visitor v;
    visit(tr, v);
    v.close_pending_actions();

    if (v.events.size() > max_events)
    {
        std::cerr << "Not writing speedscope json, too many events (" << v.events.size() << ")" << std::endl;
        return;
    }

    double time_factor = 1.;
    if (tr.elapsed_seconds() > 0)
        time_factor = 1e6 * tr.elapsed_seconds() / tr.elapsed_cycles();

    cc::string s;
    s += "[";
    for (auto const& e : v.events)
    {
        s += cc::format("{{\"name\": \"%s\", \"cat\": \"PERF\", \"ph\": \"%s\", \"pid\": 0, \"tid\": %s, \"ts\": %s}},\n", v.locations[e.frame]->name,
                        e.type, e.cpu, (e.at - v.min_cycles) * time_factor);
    }
    if (s.ends_with(",\n"))
    {
        s.pop_back();
        s.pop_back();
    }
    s += "]";
    out << s.c_str();
}

void write_summary_csv(cc::string_view filename)
{
    std::ofstream out(cc::string(filename).c_str());
    if (!out.good())
        return;

    struct entry
    {
        int count = 0;
        uint64_t cycles_total = 0;
        uint64_t cycles_children = 0;
        uint64_t cycles_min = std::numeric_limits<uint64_t>::max();
        uint64_t cycles_max = 0;
    };

    struct stack_entry
    {
        location const* loc;
        uint64_t cycles;
        uint64_t cycles_children;
    };

    struct visitor : ct::visitor
    {
        std::map<location const*, entry> entries;
        std::vector<stack_entry> stack;
        int depth = 1;

        virtual void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t /*cpu*/) override
        {
            //
            stack.push_back({&loc, cycles, 0});
        }
        virtual void on_trace_end(uint64_t cycles, uint32_t /*cpu*/) override
        {
            auto se = stack.back();
            stack.pop_back();
            auto dt = cycles - se.cycles;

            auto& e = entries[se.loc];
            e.count++;
            e.cycles_total += dt;
            e.cycles_children += se.cycles_children;
            e.cycles_min = std::min(e.cycles_min, dt);
            e.cycles_max = std::max(e.cycles_max, dt);

            if (!stack.empty())
                stack.back().cycles_children += dt;
        }
    };
    visitor v;
    visit(ct::get_current_thread_trace(), v);

    out << "name,file,function,count,total,avg,min,max,total_body,avg_body\n";
    for (auto const& kvp : v.entries)
    {
        auto l = kvp.first;
        auto e = kvp.second;
        out << '"' << l->name << '"' << ",";
        out << '"' << l->file << ":" << l->line << '"' << ",";
        out << '"' << l->function << '"' << ",";
        out << e.count << ",";
        out << e.cycles_total << ",";
        out << e.cycles_total / e.count << ",";
        out << e.cycles_min << ",";
        out << e.cycles_max << ",";
        out << e.cycles_total - e.cycles_children << ",";
        out << (e.cycles_total - e.cycles_children) / e.count;
        out << "\n";
    }
}

void print_location_stats(trace const& t, int max_locs, print_unit unit)
{
    auto locs = t.compute_location_stats();
    std::sort(locs.begin(), locs.end(), [](location_stats const& a, location_stats const& b) { return a.total_cycles > b.total_cycles; });

    if (int(locs.size()) < max_locs)
        max_locs = int(locs.size());

    auto const cc_to_sec = t.elapsed_seconds() / t.elapsed_cycles();

    for (auto i = 0; i < max_locs; ++i)
    {
        auto const& l = locs[i];

        auto name = std::string(l.loc->name ? l.loc->name : "");
        if (name.empty())
            name = beautify_function_name(l.loc->function);
        std::cout << format_cycles(l.total_cycles, cc_to_sec, unit).c_str() << " (" << l.samples << "x, "
                  << format_cycles(l.total_cycles / l.samples, cc_to_sec, unit).c_str() << " / sample) " << name << std::endl;
    }
}
} // namespace ct
