#pragma once

#include <cstdint>

#include <clean-core/assert.hh>
#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>

namespace cr
{
/// retruns the index of the first element satisfying the predicate in the range
/// NOTE:
///   - requires at least one matching element
///   - requires operator==(element, value)
/// Complexity: O(i) where i is the "index" of the returned element
template <class Range, class T>
[[nodiscard]] constexpr size_t index_of(Range&& range, T const& value)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    while (it != end)
    {
        if (*it == value)
            return idx;

        ++it;
        ++idx;
    }

    CC_UNREACHABLE("no element satisfying the predicate found");
}

/// same as cr::index_of(range, value) but returns -1 if no element found
template <class Range, class T>
[[nodiscard]] constexpr int64_t index_of_safe(Range&& range, T const& value)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    int64_t idx = 0;

    while (it != end)
    {
        if (*it == value)
            return idx;

        ++it;
        ++idx;
    }

    return -1;
}

/// retruns the index of the first element satisfying the predicate in the range
/// NOTE: requires at least one matching element
/// Complexity: O(i) where i is the "index" of the returned element
template <class Range, class Predicate>
[[nodiscard]] constexpr size_t index_of_first(Range&& range, Predicate&& predicate = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    while (it != end)
    {
        if (cr::detail::call(idx, predicate, *it))
            return idx;

        ++it;
        ++idx;
    }

    CC_UNREACHABLE("no element satisfying the predicate found");
}

/// same as cr::index_of_first(range, predicate) but returns -1 if no element found
template <class Range, class Predicate>
[[nodiscard]] constexpr int64_t index_of_first_safe(Range&& range, Predicate&& predicate = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    int64_t idx = 0;

    while (it != end)
    {
        if (cr::detail::call(idx, predicate, *it))
            return idx;

        ++it;
        ++idx;
    }

    return -1;
}

/// retruns the index of the last element satisfying the predicate in the range
/// NOTE: requires at least one matching element
/// Complexity: O(n)
template <class Range, class Predicate>
[[nodiscard]] constexpr size_t index_of_last(Range&& range, Predicate&& predicate = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;
    size_t r = size_t(-1);

    while (it != end)
    {
        if (cr::detail::call(idx, predicate, *it))
            r = idx;

        ++it;
        ++idx;
    }

    CC_ASSERT(r != size_t(-1) && "no element satisfying the predicate found");

    return r;
}

/// same as cr::index_of_last(range, predicate) but returns -1 if no element found
template <class Range, class Predicate>
[[nodiscard]] constexpr int64_t index_of_last_safe(Range&& range, Predicate&& predicate = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    int64_t idx = 0;
    int64_t r = -1;

    while (it != end)
    {
        if (cr::detail::call(idx, predicate, *it))
            r = idx;

        ++it;
        ++idx;
    }

    return r;
}

}
