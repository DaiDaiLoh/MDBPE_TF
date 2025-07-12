#include "trace-container.hh"

#include <clean-core/map.hh>

#include "trace-config.hh"

using namespace ct;

cc::vector<event> trace::compute_events() const
{
    struct my_visitor : ct::visitor
    {
        cc::vector<event> events;
        cc::vector<location const*> loc_stack;

        void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t cpu) override
        {
            loc_stack.push_back(&loc);
            events.push_back({loc_stack.back(), cycles, cpu, true});
        }

        void on_trace_end(uint64_t cycles, uint32_t cpu) override
        {
            events.push_back({loc_stack.back(), cycles, cpu, false});
            loc_stack.pop_back();
        }
    };

    my_visitor v;
    visit(*this, v);

    return cc::move(v.events);
}

cc::vector<event_scope> trace::compute_event_scopes() const
{
    struct my_visitor : ct::visitor
    {
        cc::vector<event_scope> scopes;

        cc::vector<location const*> loc_stack;
        cc::vector<uint64_t> cycle_stack;
        cc::vector<uint32_t> cpu_stack;

        void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t cpu) override
        {
            loc_stack.push_back(&loc);
            cycle_stack.push_back(cycles);
            cpu_stack.push_back(cpu);
        }

        void on_trace_end(uint64_t cycles, uint32_t cpu) override
        {
            auto loc = loc_stack.back();
            auto& s = scopes.emplace_back();
            s.loc = loc;
            s.start_cycles = cycle_stack.back();
            s.end_cycles = cycles;
            s.start_cpu = cpu_stack.back();
            s.end_cpu = cpu;

            cycle_stack.pop_back();
            cpu_stack.pop_back();
            loc_stack.pop_back();
        }
    };

    my_visitor v;
    visit(*this, v);

    return cc::move(v.scopes);
}

cc::vector<location_stats> trace::compute_location_stats() const
{
    struct my_visitor : ct::visitor
    {
        cc::map<location const*, location_stats> stats;

        cc::vector<location const*> loc_stack;
        cc::vector<uint64_t> cycle_stack;

        void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t /*cpu*/) override
        {
            loc_stack.push_back(&loc);
            cycle_stack.push_back(cycles);
        }

        void on_trace_end(uint64_t cycles, uint32_t /*cpu*/) override
        {
            auto loc = loc_stack.back();
            auto& s = stats[loc];
            s.loc = loc;
            s.samples++;
            s.total_cycles += cycles - cycle_stack.back();

            cycle_stack.pop_back();
            loc_stack.pop_back();
        }
    };

    my_visitor v;
    visit(*this, v);

    return cc::vector<location_stats>(v.stats.values());
}

trace::trace(cc::string name, cc::vector<uint32_t> data, trace::time_point time_start, trace::time_point time_end, uint64_t cycles_start, uint64_t cycles_end)
  : _name(cc::move(name)), //
    _data(cc::move(data)),
    _time_start(time_start),
    _time_end(time_end),
    _cycles_start(cycles_start),
    _cycles_end(cycles_end)
{
}

void trace::add_start(const location& loc, uint64_t cycles, uint32_t cpu)
{
    _data.push_back(uint64_t(&loc)); // truncated to 32 bit
    _data.push_back(uint64_t(&loc) >> 32uLL);
    _data.push_back(cycles); // truncated to 32 bit
    _data.push_back(cycles >> 32uLL);
    _data.push_back(cpu);
}

void trace::add_end(uint64_t cycles, uint32_t cpu)
{
    _data.push_back(CTRACER_END_VALUE);
    _data.push_back(cycles); // truncated to 32 bit
    _data.push_back(cycles >> 32uLL);
    _data.push_back(cpu);
}

void trace::add(const trace& t) { _data.push_back_range(t._data); }

trace ct::filter_subscope(trace const& t, cc::function_ref<bool(location const&)> predicate)
{
    auto res = trace(t.name(), {}, t.time_start(), t.time_end(), t.cycles_start(), t.cycles_end());

    struct my_visitor : ct::visitor
    {
        cc::function_ref<bool(location const&)> predicate;
        trace& res;
        int true_cnt = 0;
        cc::vector<bool> true_stack;

        my_visitor(cc::function_ref<bool(location const&)> p, trace& r) : predicate(p), res(r) {}

        void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t cpu) override
        {
            true_stack.push_back(predicate(loc));
            true_cnt += true_stack.back();

            if (true_cnt > 0)
                res.add_start(loc, cycles, cpu);
        }

        void on_trace_end(uint64_t cycles, uint32_t cpu) override
        {
            if (true_cnt > 0)
                res.add_end(cycles, cpu);

            true_cnt -= true_stack.back();
            true_stack.pop_back();
        }
    };

    auto v = my_visitor{predicate, res};
    visit(t, v);

    return res;
}

trace ct::map_cpu(const trace& t, uint32_t new_cpu)
{
    auto res = trace(t.name(), {}, t.time_start(), t.time_end(), t.cycles_start(), t.cycles_end());

    struct my_visitor : ct::visitor
    {
        trace& res;
        uint32_t new_cpu;

        my_visitor(trace& r, uint32_t new_cpu) : res(r), new_cpu(new_cpu) {}

        void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t) override { res.add_start(loc, cycles, new_cpu); }

        void on_trace_end(uint64_t cycles, uint32_t) override { res.add_end(cycles, new_cpu); }
    };

    auto v = my_visitor{res, new_cpu};
    visit(t, v);

    return res;
}
