# ctracer

High-performance cycle-based C++ tracing library designed for minimal runtime and compile time overhead and maximum accuracy.

## Dependencies / Requirements

* C++11
* msvc, gcc, or clang
* x64
* cmake (optional)

## Usage:

### Tracing

Tracing arbitrary scopes is the main use case:

```cpp
#include <ctracer/trace.hh>

void foo()
{
    TRACE();

    do_stuff();
}
```

`TRACE("string literal")` can be used to add a custom name.
For performance reasons, only constant string literals are allowed.
Costs are 70-100 CPU cycles per `TRACE`.


### Scopes

By default all `TRACE`s are appended to thread-local root scopes.
This can be overridden by `ct::scope` RAII objects.
`TRACE`s are appended to the closest `ct::scope` in the call stack.
The recorded trace can be retrieved by `scope.trace()`.

```cpp
#include <ctracer/scope.hh>

void object::tick()
{
    TRACE();

    do_stuff();
}

void on_frame_tick()
{
    ct::scope s; // all nested TRACE()s are recorded into s

    for (auto const& obj : my_objects)
        obj.tick();

    print_summary(s.trace()); // prints performance summary
}
```

`ct::scope` optionally takes a name (`ct::scope("my scope")`) and an allocator.


### Introspection and IO

Internally, a chunked pool memory allocation system is used for maximum performance.
To analyze the traced functions, the opaque `ct::trace` value type can be used (which internally is just a `vector<uint32_t>` and a name).

Getting `trace`s:
```cpp
#include <ctracer/trace-config.hh>

// of current thread
auto trace = ct::get_current_thread_trace();

// of all previous threads that are already finished
auto traces = ct::get_finished_thread_traces();

// of a custom scope
ct::scope s;
do_stuff();
auto trace = s.trace();
```

There is no way to get the traces of currently running threads that are not the current one because there is no efficient way to do so safely.
If this functionality is required the user can manually call `get_current_thread_trace` in each thread and communicate the results.
Memory consumed by finished threads can be freed manually by calling `ct::clear_finished_thread_traces()`.

`trace`s can be inspected by a visitor API:
```cpp
#include <ctracer/trace-config.hh>

struct my_visitor : ct::visitor
{
    void on_trace_start(ct::location const& loc, uint64_t cycles, uint32_t cpu) override { /* ... */ }
    void on_trace_end(uint64_t cycles, uint32_t cpu) override { /* ... */ }
};
my_visitor v;
visit(some_trace, v);
```

### Utilities

```cpp
#include <ctracer/trace.hh>

ct::cycler c;
do_stuff();
c.elapsed_cycles(); // returns elapsed cycles from ctor to now
```

### Other Configuration

```cpp
#include <ctracer/trace-config.hh>

ct:set_default_allocator(...);
ct:set_thread_allocator(...);
ct:set_thread_name(...);
```

### Allocators

```cpp
auto a = ct::ChunkAllocator::create(chunk_size_in_dwords);

ct::set_default_allocator(a);
ct::set_thread_allocator(a);
ct::scope s("my_scope", a);
```

If many threads are created the default allocator might have too large default chunk sizes (256 kb).


## Resource Usage

* each `TRACE()` takes 70-100 cycles
* each `TRACE()` adds 36 bytes
* the default `ChunkAllocator` allocates 256 kb chunks


## Design Decisions and Structure

* `#include <ctracer/trace.hh>` has minimal compile time impact and exposes only the `TRACE()` macro
* `TRACE()` is designed for minimal runtime overhead (below 100 cycles per traced scope)
* `#include <ctracer/trace-config.hh>` provides configuration and analytics (traces, allocators, IO)
* `ct::location` is static memory. references and pointers to it are stable and always valid.


## TODO

* usage example for printing, ranges, speedscope.json
* record `alloc_chunk` calls so they can be ignored
* in `allocate`, touch all allocated memory to make measurement less noisy
* multiple threads in speedscope json
* range-based-for for iterating over traces
* benchmarks against other tracing libraries
* `TRACE_MINIMAL` for tracing without recording cpu index
* start and end cpu in `trace`
