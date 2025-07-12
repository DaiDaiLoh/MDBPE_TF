#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT, class MapF>
struct mapped_iterator
{
    mapped_iterator(ItT it, EndT end, MapF& mapF) : _it(cc::move(it)), _end(cc::move(end)), _mapF(mapF) {}

    constexpr decltype(auto) operator*() { return cr::detail::call(_idx, _mapF, *_it); }
    constexpr void operator++()
    {
        ++_it;
        ++_idx;
    }
    constexpr bool operator!=(cc::sentinel) const { return _it != _end; }

private:
    size_t _idx = 0;
    ItT _it;
    EndT _end;
    MapF& _mapF;
};

template <class Range, class MapF>
struct mapped_range
{
    constexpr auto begin() { return mapped_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr auto begin() const { return mapped_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    MapF mapF;
};

template <class ItT, class EndT, class MapF, class Predicate, class ExpectT>
struct mapped_filtered_iterator
{
    mapped_filtered_iterator(ItT it, EndT end, MapF& mapF, Predicate& pred, ExpectT)
      : _it(cc::move(it)), _end(cc::move(end)), _mapF(mapF), _pred(pred)
    {
        while (_it != _end && cr::detail::call(_idx, _pred, *_it) != ExpectT::value)
        {
            ++_idx;
            ++_it;
        }
    }

    constexpr decltype(auto) operator*() { return cr::detail::call(_idx, _mapF, *_it); }
    constexpr void operator++()
    {
        ++_idx;
        ++_it;
        while (_it != _end && cr::detail::call(_idx, _pred, *_it) != ExpectT::value)
        {
            ++_idx;
            ++_it;
        }
    }
    constexpr bool operator!=(cc::sentinel) const { return _it != _end; }

private:
    size_t _idx = 0;
    ItT _it;
    EndT _end;
    MapF& _mapF;
    Predicate& _pred;
};

template <class Range, class MapF, class Predicate, class ExpectT>
struct mapped_filtered_range
{
    constexpr auto begin() { return mapped_filtered_iterator(cc::begin(range), cc::end(range), mapF, pred, ExpectT{}); }
    constexpr auto begin() const { return mapped_filtered_iterator(cc::begin(range), cc::end(range), mapF, pred, ExpectT{}); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    MapF mapF;
    Predicate pred;
};
}

/// returns a new range consisting of f(e) for each old element
template <class Range, class MapF>
[[nodiscard]] constexpr auto map(Range&& range, MapF&& f)
{
    return smart_range<detail::mapped_range<Range, MapF>>({cc::forward<Range>(range), cc::forward<MapF>(f)});
}
/// returns a new range consisting of f(e) for each old element and only where pred(e) is true
template <class Range, class MapF, class Predicate>
[[nodiscard]] constexpr auto map_where(Range&& range, MapF&& f, Predicate&& pred)
{
    return smart_range<detail::mapped_filtered_range<Range, MapF, Predicate, std::true_type>>(
        {cc::forward<Range>(range), cc::forward<MapF>(f), cc::forward<Predicate>(pred)});
}
/// returns a new range consisting of f(e) for each old element and only where pred(e) is false
template <class Range, class MapF, class Predicate>
[[nodiscard]] constexpr auto map_where_not(Range&& range, MapF&& f, Predicate&& pred)
{
    return smart_range<detail::mapped_filtered_range<Range, MapF, Predicate, std::false_type>>(
        {cc::forward<Range>(range), cc::forward<MapF>(f), cc::forward<Predicate>(pred)});
}

// [smart_range implementation]
template <class ContainerT>
template <class MapF>
constexpr auto smart_range<ContainerT>::map(MapF&& f)
{
    return cr::map(_container, cc::forward<MapF>(f));
}
template <class ContainerT>
template <class MapF, class Predicate>
constexpr auto smart_range<ContainerT>::map_where(MapF&& f, Predicate&& pred)
{
    return cr::map_where(_container, cc::forward<MapF>(f), cc::forward<Predicate>(pred));
}
template <class ContainerT>
template <class MapF, class Predicate>
constexpr auto smart_range<ContainerT>::map_where_not(MapF&& f, Predicate&& pred)
{
    return cr::map_where_not(_container, cc::forward<MapF>(f), cc::forward<Predicate>(pred));
}
}
