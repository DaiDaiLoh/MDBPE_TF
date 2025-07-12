#pragma once

#include <clean-core/equal_to.hh>
#include <clean-core/forward.hh>
#include <clean-core/iterator.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
/// returns true iff the two ranges contain the same elements in the same order
/// optionally by using a given comparator (testing Comp{}(a, b))
template <class LhsRange, class RhsRange, class Comp = cc::equal_to<void>>
[[nodiscard]] constexpr bool are_equal(LhsRange const& lhs, RhsRange const& rhs, Comp&& comp = {})
{
    auto itA = cc::begin(lhs);
    auto itB = cc::begin(rhs);
    auto endA = cc::end(lhs);
    auto endB = cc::end(rhs);

    while (itA != endA && itB != endB)
    {
        if (!comp(*itA, *itB))
            return false; // found unequal element

        ++itA;
        ++itB;
    }

    if (itA != endA || itB != endB)
        return false; // one range is longer than the other

    return true;
}

/// returns true iff the two ranges do NOT contain the same elements in the same order
/// optionally by using a given comparator (testing Comp{}(a, b))
template <class LhsRange, class RhsRange, class Comp = cc::equal_to<void>>
[[nodiscard]] constexpr bool are_not_equal(LhsRange const& lhs, RhsRange const& rhs, Comp&& comp = {})
{
    return !cr::are_equal(lhs, rhs, cc::forward<Comp>(comp));
}

// [smart_range implementation]
template <class ContainerT>
template <class RhsRange, class Comp>
[[nodiscard]] constexpr bool smart_range<ContainerT>::is_equal_to(RhsRange const& range, Comp&& comp)
{
    return cr::are_equal(_container, range, comp);
}
template <class ContainerT>
template <class RhsRange, class Comp>
[[nodiscard]] constexpr bool smart_range<ContainerT>::is_not_equal_to(RhsRange const& range, Comp&& comp)
{
    return cr::are_equal(_container, range, comp);
}
}
