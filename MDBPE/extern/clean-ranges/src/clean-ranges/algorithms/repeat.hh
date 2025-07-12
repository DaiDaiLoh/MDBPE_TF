#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT, class Range, class MapF>
struct repeat_iterator
{
    repeat_iterator(ItT it, EndT end, Range& range, MapF& mapF) : _it(cc::move(it)), _end(cc::move(end)), _range(range), _mapF(mapF)
    {
        CC_ASSERT(it != end && "cannot repeat empty range");
    }

    constexpr decltype(auto) operator*() { return cr::detail::call(_idx, _mapF, *_it); }
    constexpr void operator++()
    {
        ++_it;
        ++_idx;

        if (!(_it != _end))
            _it = cc::begin(_range);
    }
    constexpr bool operator!=(cc::sentinel) const { return true; }

private:
    size_t _idx = 0;
    ItT _it;
    EndT _end;
    Range& _range;
    MapF& _mapF;
};

template <class Range, class MapF>
struct repeat_range
{
    constexpr auto begin() { return repeat_iterator(cc::begin(range), cc::end(range), range, mapF); }
    constexpr auto begin() const { return repeat_iterator(cc::begin(range), cc::end(range), range, mapF); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    MapF mapF;
};
}

/// returns a range that infinitely repeats the given range (optionally a mapped version of it)
template <class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto repeat(Range&& range, MapF&& f = {})
{
    return smart_range<detail::repeat_range<Range, MapF>>({cc::forward<Range>(range), cc::forward<MapF>(f)});
}

// [smart_range implementation]
template <class ContainerT>
template <class MapF>
constexpr auto smart_range<ContainerT>::repeat(MapF&& f)
{
    return cr::repeat(_container, cc::forward<MapF>(f));
}
}
