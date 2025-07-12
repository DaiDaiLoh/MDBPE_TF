#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>
#include <clean-core/tuple.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT, class MapF>
struct flat_mapped_iterator
{
    // TODO
    flat_mapped_iterator(ItT it, EndT end, MapF& mapF) : _it(cc::move(it)), _end(cc::move(end)), _mapF(mapF) {}

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

template <class Range, class... MapF>
struct flat_mapped_range
{
    // TODO
    constexpr auto begin() { return flat_mapped_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr auto begin() const { return flat_mapped_iterator(cc::begin(range), cc::end(range), mapF); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    cc::tuple<MapF...> mapF;
};
}

/// a generalized map:
/// for each element in the range, the first mapping function is applied.
/// if the result is another range, that range is also iterated (a flatten)
/// then the next mapping function is applied and so on.
/// (each mapping function can only expand the range once, multiple expansions can be forced by passing cc::identity)
/// (passing no function is equivalent to passing cc::identity once)
template <class Range, class... MapF>
[[nodiscard]] constexpr auto flatmap(Range&& range, MapF&&... maps)
{
    if constexpr (sizeof...(MapF) == 0)
        return smart_range<detail::flat_mapped_range<Range, cc::identity_function>>({cc::forward<Range>(range), cc::identity_function{}});
    else
        return smart_range<detail::flat_mapped_range<Range, MapF...>>({cc::forward<Range>(range), cc::forward<MapF>(maps)...});
}

// [smart_range implementation]
template <class ContainerT>
template <class... MapF>
constexpr auto smart_range<ContainerT>::flatmap(MapF&&... maps)
{
    return cr::flatmap(_container, cc::forward<MapF>(maps)...);
}
}