#pragma once

#include <clean-core/assert.hh>
#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>

namespace cr
{
/// returns the first element satisfying the predicate in the range
/// NOTE: requires at least one matching element
/// Complexity: O(i) where i is the "index" of the returned element
template <class Range, class Predicate = cc::constant_function<true>>
[[nodiscard]] constexpr decltype(auto) first(Range&& range, Predicate&& predicate = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    while (it != end)
    {
        decltype(auto) v = *it;
        if (cr::detail::call(idx, predicate, v))
            return v;

        ++it;
        ++idx;
    }

    CC_UNREACHABLE("no element satisfying the predicate found");
}
}
