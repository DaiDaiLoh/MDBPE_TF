#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
template <class T>
struct indexed_value
{
    size_t index;
    T value;
};

namespace detail
{
template <class ItT, class EndT, class MapF>
struct indexed_iterator
{
    indexed_iterator(ItT it, EndT end, MapF& mapF) : _it(cc::move(it)), _end(cc::move(end)), _mapF(mapF) {}

    constexpr decltype(auto) operator*()
    {
        using T = decltype(cr::detail::call(_idx, _mapF, *_it));
        return indexed_value<T>{_idx, cr::detail::call(_idx, _mapF, *_it)};
    }
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
struct indexed_range
{
    constexpr auto begin() { return indexed_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr auto begin() const { return indexed_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    MapF mapF;
};
}

/// returns a new range consisting of indexed_value{idx, f(e)} for each element
template <class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto indexed(Range&& range, MapF&& f = {})
{
    return smart_range<detail::indexed_range<Range, MapF>>({cc::forward<Range>(range), cc::forward<MapF>(f)});
}

// [smart_range implementation]
template <class ContainerT>
template <class MapF>
constexpr auto smart_range<ContainerT>::indexed(MapF&& f)
{
    return cr::indexed(_container, cc::forward<MapF>(f));
}
}
