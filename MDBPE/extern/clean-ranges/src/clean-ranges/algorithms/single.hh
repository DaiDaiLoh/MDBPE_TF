#pragma once

#include <clean-core/assert.hh>
#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
/// returning the one element satisfying the predicate in the range
/// asserts if no element or more than one found
template <class Range, class Predicate = cc::constant_function<true>>
[[nodiscard]] constexpr decltype(auto) single(Range&& range, Predicate&& predicate = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    while (it != end)
    {
        decltype(auto) v = *it;
        if (cr::detail::call(idx, predicate, v))
        {
#ifdef CC_ENABLE_ASSERTIONS
            ++it;
            ++idx;
            while (it != end)
            {
                CC_ASSERT(!cr::detail::call(idx, predicate, *it) && "is not actually the only element");
                ++it;
                ++idx;
            }
#endif

            return v;
        }

        ++it;
        ++idx;
    }

    CC_UNREACHABLE("no element satisfying the predicate found");
}

// [smart_range implementation]
template <class ContainerT>
template <class Predicate>
constexpr decltype(auto) smart_range<ContainerT>::single(Predicate&& predicate)
{
    return cr::single(_container, predicate);
}
}
