#pragma once

#include <initializer_list>

#include <clean-core/enable_if.hh>

#include <clean-ranges/ranges/bounded_range.hh>
#include <clean-ranges/ranges/sentinel_range.hh>
#include <clean-ranges/ranges/unbounded_range.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
/// returns a smart range that iterates over the given range
template <class RangeT, cc::enable_if<cc::is_any_range<RangeT>> = true>
[[nodiscard]] constexpr auto range(RangeT&& r)
{
    if constexpr (std::is_rvalue_reference_v<RangeT>)
        return cr::smart_range<std::remove_reference_t<RangeT>>(cc::move(r));
    else
        return cr::smart_range<RangeT>(cc::forward<RangeT>(r));
}
/// returns a smart range that iterates over the given values
template <class T>
[[nodiscard]] constexpr auto range(std::initializer_list<T> r)
{
    return cr::smart_range<std::initializer_list<T>>(r);
}

/// returns a smart range that iterates from 0..end (exclusive)
/// e.g. useful in `for (auto i : cr::range(v.size()))` to get values up to v.size() - 1
template <class EndT, cc::enable_if<std::is_integral_v<EndT>> = true>
[[nodiscard]] constexpr auto range(EndT&& end_exclusive)
{
    return cr::smart_range(cr::detail::bounded_range(EndT(), end_exclusive, cr::detail::pre_increment{}));
}

/// given an iterator that can compare against cc::sentinel, makes a smart range out of it
template <class ItT, cc::enable_if<cr::detail::is_sentinel_iterator<ItT>::value> = true>
[[nodiscard]] constexpr auto range(ItT&& it)
{
    return cr::smart_range(cr::detail::sentinel_range(cc::forward<ItT>(it)));
}

/// returns a smart_range that iterates from start..end (exclusive)
/// using `++v` to get to the next element
/// if `*v` is a valid expression, then it is returned, otherwise `v` directly (proper behavior on iterators)
template <class StartT, class EndT>
[[nodiscard]] constexpr auto range(StartT&& start, EndT&& end_exclusive)
{
    return cr::smart_range(cr::detail::bounded_range(start, end_exclusive, cr::detail::pre_increment{}));
}
/// returns a smart_range that iterates from start..end (exclusive)
/// to get to the next value, the following expressions are used (priority top-down):
///   `v += inc`
///   `v = inc(v)`
///   `inc(v)`
/// if `*v` is a valid expression, then it is returned, otherwise `v` directly (proper behavior on iterators)
/// NOTE: the range runs while `start != end`, thus `cr::range(5, 10, 2)` is an infinite loop
template <class StartT, class EndT, class IncT>
[[nodiscard]] constexpr auto range(StartT&& start, EndT&& end_exclusive, IncT&& inc)
{
    return cr::smart_range(cr::detail::bounded_range(start, end_exclusive, cr::detail::get_inc_class_for<StartT>(cc::forward<IncT>(inc))));
}

/// returns a smart_range that iterates endlessly from the start value
/// using `++v` to get to the next element
/// if `*v` is a valid expression, then it is returned, otherwise `v` directly (proper behavior on iterators)
template <class StartT>
[[nodiscard]] constexpr auto inf_range(StartT&& start)
{
    return cr::smart_range(cr::detail::unbounded_range(start, cr::detail::pre_increment{}));
}
/// returns a smart_range that iterates endlessly from the start value
/// to get to the next value, the following expressions are used (priority top-down):
///   `v += inc`
///   `v = inc(v)`
///   `inc(v)`
/// if `*v` is a valid expression, then it is returned, otherwise `v` directly (proper behavior on iterators)
template <class StartT, class IncT>
[[nodiscard]] constexpr auto inf_range(StartT&& start, IncT&& inc)
{
    return cr::smart_range(cr::detail::unbounded_range(start, cr::detail::get_inc_class_for<StartT>(cc::forward<IncT>(inc))));
}
}
