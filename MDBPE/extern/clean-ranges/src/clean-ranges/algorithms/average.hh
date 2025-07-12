#pragma once

#include <clean-core/assert.hh>
#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/detail/type_or.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
/// computes the average of f(element)
/// NOTE:
///   - requires a non-empty range
///   - count type can be provided optionally
///   - returns sum(range, f) / static_cast<CountT>(count(v)) (i.e. requires operator/)
template <class CountT = void, class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto average(Range&& range, MapF&& f = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    using T = cr::detail::type_or<CountT, size_t>;
    size_t idx = 0;
    auto cnt = T(1);
    CC_ASSERT(it != end && "requires non-empty range");

    auto s = cr::detail::call(idx, f, *it);
    ++it;
    ++idx;
    while (it != end)
    {
        s = s + cr::detail::call(idx, f, *it);
        ++it;
        ++cnt;
        ++idx;
    }

    return s / cnt;
}

// [smart_range implementation]
template <class ContainerT>
template <class MapF>
constexpr auto smart_range<ContainerT>::average(MapF&& f)
{
    return cr::average(_container, f);
}
}
