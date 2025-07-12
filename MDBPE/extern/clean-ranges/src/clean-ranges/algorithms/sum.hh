#pragma once

#include <clean-core/assert.hh>
#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/detail/type_or.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
/// computes the sum of f(element)
/// NOTE:
///   - requires a non-empty range
///   - accumulation type can be optionally specified
template <class AccumT = void, class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto sum(Range&& range, MapF&& f = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;
    using T = cr::detail::type_or<AccumT, decltype(cr::detail::call(idx, f, *it))>;
    CC_ASSERT(it != end && "requires non-empty range");

    auto s = static_cast<T>(cr::detail::call(idx, f, *it));
    ++it;
    ++idx;
    while (it != end)
    {
        s = s + static_cast<T>(cr::detail::call(idx, f, *it));
        ++it;
        ++idx;
    }

    return s;
}

// [smart_range implementation]
template <class ContainerT>
template <class MapF>
constexpr auto smart_range<ContainerT>::sum(MapF&& f)
{
    return cr::sum(_container, f);
}
}
