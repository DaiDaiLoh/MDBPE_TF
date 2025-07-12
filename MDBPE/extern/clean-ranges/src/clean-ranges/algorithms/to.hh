#pragma once

#include <clean-ranges/algorithms/collect.hh>

namespace cr
{
/// same as collect_into but creates and returns a new container
template <template <class...> class ContainerTemplate, class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto to(Range&& range, MapF&& f = {})
{
    using T = std::decay_t<decltype(cr::detail::call(size_t(0), f, *cc::begin(range)))>;

    ContainerTemplate<T> container;
    cr::collect(cc::forward<Range>(range), container, cc::forward<MapF>(f));
    return container;
}

/// same as collect_into but creates and returns a new container
template <class Container, class Range, class MapF = cc::identity_function>
[[nodiscard]] constexpr auto to(Range&& range, MapF&& f = {})
{
    Container container;
    cr::collect(cc::forward<Range>(range), container, cc::forward<MapF>(f));
    return container;
}

// [smart_range implementation]
template <class ContainerT>
template <template <class...> class ContainerTemplate, class MapF>
constexpr auto smart_range<ContainerT>::to(MapF&& f)
{
    return cr::to<ContainerTemplate>(_container, cc::forward<MapF>(f));
}
template <class ContainerT>
template <class Container, class MapF>
constexpr auto smart_range<ContainerT>::to(MapF&& f)
{
    return cr::to<Container>(_container, cc::forward<MapF>(f));
}
}
