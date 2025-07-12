#pragma once

#include <clean-core/apply.hh>
#include <clean-core/forward.hh>
#include <clean-core/iterator.hh>
#include <clean-core/sentinel.hh>
#include <clean-core/tuple.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItPair, class... ItPairs>
constexpr decltype(auto) concat_deref(ItPair&& it, ItPairs&&... rest)
{
    if constexpr (sizeof...(ItPairs) == 0)
        return *it.first;
    else
        return it.first != it.second ? *it.first : concat_deref(cc::forward<ItPairs>(rest)...);
}

template <class... ItPairs>
struct concat_iterator
{
    constexpr decltype(auto) operator*()
    {
        return cc::apply([](auto&... it) { return cr::detail::concat_deref(it...); }, _its);
    }
    constexpr void operator++()
    {
        // uses short-circuiting of ||
        // equivalent to:
        // if (it.first != it.second) {
        //    ++it.first;
        //    return;
        // }
        // ...
        cc::apply([](auto&... it) { (void)((it.first != it.second ? (++it.first, true) : false) || ...); }, _its);
    }
    constexpr bool operator!=(cc::sentinel) const
    {
        return cc::apply([](auto&&... it) { return ((it.first != it.second) || ...); }, _its);
    }

    constexpr concat_iterator(cc::tuple<ItPairs...> its) : _its(cc::move(its)) {}

    cc::tuple<ItPairs...> _its;
};

template <class... Ranges>
struct concat_range
{
    constexpr auto begin()
    {
        return cc::apply([](auto&&... r) { return concat_iterator(cc::tuple(cc::pair(cc::begin(r), cc::end(r))...)); }, ranges);
    }
    constexpr auto begin() const
    {
        return cc::apply([](auto&&... r) { return concat_iterator(cc::tuple(cc::pair(cc::begin(r), cc::end(r))...)); }, ranges);
    }
    constexpr cc::sentinel end() const { return {}; }

    cc::tuple<Ranges...> ranges;
};
}

/// returns a range that iterates successively over all the given ranges
/// NOTE:
///   - this can be slower than first adding the ranges to a common buffer
///   - uses a kind of std::common_type to unify the types
template <class Range, class... Ranges>
[[nodiscard]] constexpr auto concat(Range&& range, Ranges&&... ranges)
{
    return smart_range(cr::detail::concat_range<Range, Ranges...>{cc::tuple<Range, Ranges...>(cc::forward<Range>(range), cc::forward<Ranges>(ranges)...)});
}

// [smart_range implementation]
template <class ContainerT>
template <class Range, class... Ranges>
constexpr auto smart_range<ContainerT>::concat(Range&& range, Ranges&&... ranges)
{
    return cr::concat(_container, cc::forward<Range>(range), cc::forward<Ranges>(ranges)...);
}
}
