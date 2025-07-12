#pragma once

#include <cstddef>

#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
/// returns the number of elements in the range
template <class Range>
[[nodiscard]] constexpr size_t count(Range&& range)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);

    size_t cnt = 0;
    while (it != end)
    {
        ++cnt;
        ++it;
    }
    return cnt;
}

/// returns the number of elements where (e == v)
template <class Range, class T>
[[nodiscard]] constexpr size_t count(Range&& range, T const& v)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);

    size_t cnt = 0;
    while (it != end)
    {
        if (*it == v)
            ++cnt;
        ++it;
    }
    return cnt;
}

/// returns the number of elements where predicate(e) true ist
template <class Range, class Predicate>
[[nodiscard]] constexpr size_t count_if(Range&& range, Predicate&& predicate)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    size_t cnt = 0;
    while (it != end)
    {
        if (cr::detail::call(idx, predicate, *it))
            ++cnt;

        ++it;
        ++idx;
    }
    return cnt;
}

// [smart_range implementation]
template <class ContainerT>
constexpr size_t smart_range<ContainerT>::count()
{
    return cr::count(_container);
}
template <class ContainerT>
template <class T>
constexpr size_t smart_range<ContainerT>::count(T const& v)
{
    return cr::count(_container, v);
}
template <class ContainerT>
template <class Predicate>
constexpr size_t smart_range<ContainerT>::count_if(Predicate&& predicate)
{
    return cr::count_if(_container, cc::forward<Predicate>(predicate));
}
}
