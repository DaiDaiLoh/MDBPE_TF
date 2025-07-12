#pragma once

#include <clean-ranges/algorithms/first.hh>
#include <clean-ranges/algorithms/last.hh>

namespace cr
{
/// returns the first element satisfying the predicate in the range
/// NOTE: requires at least one matching element
/// Complexity: O(i) where i is the "index" of the returned element
template <class Range, class Predicate>
[[nodiscard]] constexpr decltype(auto) find(Range&& range, Predicate&& predicate)
{
    return cr::first(cc::forward<Range>(range), cc::forward<Predicate>(predicate));
}

/// returns the last element satisfying the predicate in the range
/// NOTE: requires at least one matching element
/// Complexity: O(n)
template <class Range, class Predicate>
[[nodiscard]] constexpr decltype(auto) find_last(Range&& range, Predicate&& predicate)
{
    return cr::last(cc::forward<Range>(range), cc::forward<Predicate>(predicate));
}

/// returns the first element satisfying the predicate in the range
/// if no element matches, returns `value`
template <class Range, class Predicate, class T>
[[nodiscard]] constexpr decltype(auto) find_or(Range&& range, Predicate&& predicate, T&& value)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    using R = decltype(true ? *it : cc::forward<T>(value));

    while (it != end)
    {
        decltype(auto) v = *it;
        if (cr::detail::call(idx, predicate, v))
            return static_cast<R>(v);

        ++it;
        ++idx;
    }

    return static_cast<R>(cc::forward<T>(value));
}

/// returns the last element satisfying the predicate in the range
/// if no element matches, returns `value`
template <class Range, class Predicate, class T>
[[nodiscard]] constexpr decltype(auto) find_last_or(Range&& range, Predicate&& predicate, T&& value)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    auto found = false;
    auto res_it = it;

    while (it != end)
    {
        if (cr::detail::call(idx, predicate, *it))
        {
            res_it = it;
            found = true;
        }

        ++it;
        ++idx;
    }

    return found ? *res_it : cc::forward<T>(value);
}
}
