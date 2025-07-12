#pragma once

#include <clean-core/iterator.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
/// Sets all elements of the range to the provided value
/// NOTE: requires operator=
template <class Range, class T>
constexpr void fill(Range&& range, T const& value)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);

    while (it != end)
    {
        *it = value;
        ++it;
    }
}

// [smart_range implementation]
template <class ContainerT>
template <class T>
constexpr void smart_range<ContainerT>::fill(T const& value)
{
    cr::fill(_container, value);
}
}
