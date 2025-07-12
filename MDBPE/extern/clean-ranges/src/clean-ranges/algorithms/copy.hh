#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/move.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
/// Goes over both ranges simultaneously and copies elements from rhs to lhs
/// NOTE:
///   - requires operator= (copy)
///   - stops as soon as one of the ranges is empty
template <class LhsRange, class RhsRange>
constexpr void copy_from(LhsRange&& lhs, RhsRange const& rhs)
{
    auto itA = cc::begin(lhs);
    auto itB = cc::begin(rhs);
    auto endA = cc::end(lhs);
    auto endB = cc::end(rhs);

    while (itA != endA && itB != endB)
    {
        *itA = *itB;
        ++itA;
        ++itB;
    }
}

/// Goes over both ranges simultaneously and moves elements from rhs to lhs
/// NOTE:
///   - requires operator= (move)
///   - stops as soon as one of the ranges is empty
template <class LhsRange, class RhsRange>
constexpr void move_from(LhsRange&& lhs, RhsRange const& rhs)
{
    auto itA = cc::begin(lhs);
    auto itB = cc::begin(rhs);
    auto endA = cc::end(lhs);
    auto endB = cc::end(rhs);

    while (itA != endA && itB != endB)
    {
        *itA = cc::move(*itB);
        ++itA;
        ++itB;
    }
}

// [smart_range implementation]
template <class ContainerT>
template <class RhsRange>
constexpr void smart_range<ContainerT>::copy_to(RhsRange&& range)
{
    cr::copy_from(range, _container);
}
template <class ContainerT>
template <class RhsRange>
constexpr void smart_range<ContainerT>::copy_from(RhsRange&& range)
{
    cr::copy_from(_container, range);
}
template <class ContainerT>
template <class RhsRange>
constexpr void smart_range<ContainerT>::move_to(RhsRange&& range)
{
    cr::move_from(range, _container);
}
template <class ContainerT>
template <class RhsRange>
constexpr void smart_range<ContainerT>::move_from(RhsRange&& range)
{
    cr::move_from(_container, range);
}
}
