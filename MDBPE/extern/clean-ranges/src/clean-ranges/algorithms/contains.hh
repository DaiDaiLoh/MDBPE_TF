#pragma once

#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>

namespace cr
{
/// returns true if at least one element satisfies (e == v)
template <class Range, class T>
[[nodiscard]] constexpr bool contains(Range&& range, T const& v)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);

    while (it != end)
    {
        if (*it == v)
            return true;

        ++it;
    }

    return false;
}
}
