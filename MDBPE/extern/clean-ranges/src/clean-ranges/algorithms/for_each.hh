#pragma once

#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>

namespace cr
{
/// calls f(e) for each element
template <class Range, class F>
constexpr void for_each(Range&& range, F&& f)
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    while (it != end)
    {
        cr::detail::call(idx, f, *it);
        ++it;
        ++idx;
    }
}
}
