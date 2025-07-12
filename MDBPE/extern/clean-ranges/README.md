# clean-ranges

A batteries-included functional C++ library for collections and ranges.


## Generalized function object

Many operations on ranges support a _generalized_ notion of function that can be passed.
For example, `cr::sum(my_range, f)` supports the following for `f`:

* free functions (e.g. `int f(int x) { return x + 1; }`), calls `f(e)` per element
* lambdas (e.g. `auto f = [](int x) { return x + 1; }`), calls `f(e)` per element
* function objects (e.g. `foo f` with `int foo::operator()(int x) { return x + 1; }`), calls `f(e)` per element
* pointer-to-member (e.g. `auto f = &foo::x` with `struct foo { int x; };`), evaluates `e.*f` per element
* pointer-to-member-function (e.g. `auto f = &foo::bar` with `struct foo { int bar() { return x + 1;} };`), calls `(e.*f)()` per element

NOTE: pointer-to-members (and -functions) do not only work on values or references:

* for values and references, uses `e.*f`
* for pointers, uses `e->*f`
* for smart pointers, uses `(*e).*f`

(implementation detail: actually just uses `e.*f` or `(*e).*f` depending on which one would compile)

### Indexed Calls

Most function objects can also be used with an index.
Internally, the index is a `size_t`, though all compatible types can be used.
The index is optional but always the first argument.

Examples:

```cpp
cc::vector<int> vals = ...;
cr::for_each(vals, [](int idx, int value) {
    std::cout << idx << ": " << value << std::endl;
});

auto same_as_idx = cr::find(vals, [](size_t idx, int v) { return v == idx; });

struct foo
{
    int value;

    bool is_above_idx(int idx) const { return value > idx; }
};
cc::array<foo> foos = ...;
foo& f = cr::first(foos, &foo::is_above_idx);
```


## TODO

Algorithms:

* median
* mode
* percentile
* flatten / flatmap (variadic!)
* ordered_by / sort (inplace)
* unzip
* windowed / chunked / pairwise
* reverse
* apply / transform
* unique / distinct
* group_by
* join_to / join_to_string
* partition (pred -> left and right result)
* shuffled
* slice
* associate (creates a map)
* prefix / suffix / subrange
* is_prefix / is_suffix / startswith / endswith / is_subrange
* 3way comp, lexicographic
* find_it, find_opt
* "is same value"
* clamp_to / clamped_to
* random
* "deref_valid"? (basically `where(x => bool(x)).map(x => *x)`, unpacking ptr, optional)

Algorithms that are unclear how to implement (for now):

* n_th_last
* drop_last

Smart-range features:

* have all algorithms in smart_range as well
* operator>> and << with callables (basically foreach, but also allows empty arg)

Optimizations:

* random access / reverse its for faster reverse
* _last versions can be faster with random access / reverse its
* O(1) size

QoL:

* xyz vs xyz'ed versions (e.g. sort vs sorted, reverse vs reversed)
* rethink const-correctness in smart_range
* find_or's default template arg T (so that find_or(r, f, {}) works)

Extensibility:

* CRTP range + inheritance

Benchmarks:

* check if `idx` is properly optimized out if not used

Testing:

* verify that lifetimes are proper
