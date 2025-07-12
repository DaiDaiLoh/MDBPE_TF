#pragma once

#include <clean-core/apply.hh>
#include <clean-core/iterator.hh>
#include <clean-core/move.hh>
#include <clean-core/pair.hh>
#include <clean-core/sentinel.hh>
#include <clean-core/tuple.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class... ItPairs>
struct zip_iterator
{
    constexpr auto operator*()
    {
        return cc::apply([](auto&&... it) { return cc::tuple<decltype(*it.first)...>(*it.first...); }, _its);
    }
    constexpr void operator++()
    {
        cc::apply([](auto&... it) { (++it.first, ...); }, _its);
    }
    constexpr bool operator!=(cc::sentinel) const
    {
        return cc::apply([](auto&&... it) { return (... && (it.first != it.second)); }, _its);
    }

    constexpr zip_iterator(cc::tuple<ItPairs...> its) : _its(cc::move(its)) {}

    cc::tuple<ItPairs...> _its;
};

template <class... Ranges>
struct zip_range
{
    constexpr auto begin()
    {
        return cc::apply([](auto&&... r) { return zip_iterator(cc::tuple(cc::pair(cc::begin(r), cc::end(r))...)); }, ranges);
    }
    constexpr auto begin() const
    {
        return cc::apply([](auto&&... r) { return zip_iterator(cc::tuple(cc::pair(cc::begin(r), cc::end(r))...)); }, ranges);
    }
    constexpr cc::sentinel end() const { return {}; }

    cc::tuple<Ranges...> ranges;
};
}

/// returns a range that traverses all given ranges at the same time
/// (stops when reaching the end of any range)
template <class Range, class... Ranges>
[[nodiscard]] constexpr auto zip(Range&& range, Ranges&&... ranges)
{
    return smart_range(cr::detail::zip_range<Range, Ranges...>{cc::tuple<Range, Ranges...>(cc::forward<Range>(range), cc::forward<Ranges>(ranges)...)});
}

// [smart_range implementation]
template <class ContainerT>
template <class Range, class... Ranges>
constexpr auto smart_range<ContainerT>::zip(Range&& range, Ranges&&... ranges)
{
    return cr::zip(_container, cc::forward<Range>(range), cc::forward<Ranges>(ranges)...);
}
}
