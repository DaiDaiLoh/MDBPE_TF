#pragma once

#include <clean-core/functors.hh>
#include <clean-core/iterator.hh>
#include <clean-core/priority_tag.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class Container, class T>
constexpr auto impl_collect(Container& c, T&& v, cc::priority_tag<5>) -> decltype(c.push_back(cc::forward<T>(v)))
{
    return c.push_back(cc::forward<T>(v));
}
template <class Container, class T>
constexpr auto impl_collect(Container& c, T&& v, cc::priority_tag<4>) -> decltype(c.add(cc::forward<T>(v)))
{
    return c.add(cc::forward<T>(v));
}
template <class Container, class T>
constexpr auto impl_collect(Container& c, T&& v, cc::priority_tag<3>) -> decltype(c.insert(cc::forward<T>(v)))
{
    return c.insert(cc::forward<T>(v));
}
template <class Container, class T>
constexpr auto impl_collect(Container& c, T&& v, cc::priority_tag<2>) -> decltype(c.push(cc::forward<T>(v)))
{
    return c.push(cc::forward<T>(v));
}
template <class Container, class T>
constexpr auto impl_collect(Container& c, T&& v, cc::priority_tag<1>) -> decltype(c << cc::forward<T>(v))
{
    return c << cc::forward<T>(v);
}
template <class Container, class T>
constexpr auto impl_collect(Container& c, T&& v, cc::priority_tag<0>) -> decltype(c += cc::forward<T>(v))
{
    return c += cc::forward<T>(v);
}

template <class Container, class T>
constexpr void impl_add_to_container(Container& c, T&& v)
{
    detail::impl_collect(c, cc::forward<T>(v), cc::priority_tag<5>{});
}
}

/// adds all elements of the range to the given container.
/// the following methods to add an element `e` to a container `c` are checked (descending priority):
///   - c.push_back(e)
///   - c.add(e)
///   - c.insert(e)
///   - c.push(e)
///   - c << e
///   - c += e
template <class Range, class Container, class MapF = cc::identity_function>
constexpr void collect(Range&& range, Container& container, MapF&& f = {})
{
    auto it = cc::begin(range);
    auto end = cc::end(range);
    size_t idx = 0;

    while (it != end)
    {
        detail::impl_add_to_container(container, cr::detail::call(idx, f, *it));
        ++it;
        ++idx;
    }
}

// [smart_range implementation]
template <class ContainerT>
template <class Container, class MapF>
constexpr void smart_range<ContainerT>::collect(Container& container, MapF&& f)
{
    cr::collect(_container, container, cc::forward<MapF>(f));
}
}
