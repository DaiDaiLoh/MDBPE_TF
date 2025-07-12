#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT, class T>
struct cast_iterator
{
    cast_iterator(ItT it, EndT end) : _it(cc::move(it)), _end(cc::move(end)) {}

    constexpr decltype(auto) operator*() { return T(*_it); }
    constexpr void operator++() { ++_it; }
    constexpr bool operator!=(cc::sentinel) const { return _it != _end; }

private:
    ItT _it;
    EndT _end;
};

template <class T, class ItT, class EndT>
auto make_cast_iterator(ItT&& it, EndT&& end)
{
    return cast_iterator<ItT, EndT, T>(cc::forward<ItT>(it), cc::forward<EndT>(end));
}

template <class Range, class T>
struct cast_range
{
    constexpr auto begin() { return cr::detail::make_cast_iterator<T>(cc::begin(range), cc::end(range)); }
    constexpr auto begin() const { return cr::detail::make_cast_iterator<T>(cc::begin(range), cc::end(range)); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
};
}

/// returns a new range consisting of T(e) for each old element
template <class T, class Range>
[[nodiscard]] constexpr auto cast_to(Range&& range)
{
    return smart_range<detail::cast_range<Range, T>>({cc::forward<Range>(range)});
}

// [smart_range implementation]
template <class ContainerT>
template <class T>
constexpr auto smart_range<ContainerT>::cast_to()
{
    return cr::cast_to<T>(_container);
}
}
