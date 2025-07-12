#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT, class T>
struct pad_with_iterator
{
    pad_with_iterator(ItT it, EndT end, T& value) : _it(cc::move(it)), _end(cc::move(end)), _value(value) {}

    constexpr decltype(auto) operator*() { return _it != _end ? *_it : _value; }
    constexpr void operator++()
    {
        if (_it != _end)
            ++_it;
    }
    constexpr bool operator!=(cc::sentinel) const { return true; }

private:
    size_t _idx = 0;
    ItT _it;
    EndT _end;
    T& _value;
};

template <class Range, class T>
struct pad_with_range
{
    constexpr auto begin() { return pad_with_iterator(cc::begin(range), cc::end(range), value); }
    constexpr auto begin() const { return pad_with_iterator(cc::begin(range), cc::end(range), value); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    T value;
};
}

/// returns an infinite range that consists of the given `range` followed by infinitely repeating `value`
template <class Range, class T>
[[nodiscard]] constexpr auto pad_with(Range&& range, T&& value)
{
    return smart_range<detail::pad_with_range<Range, T>>({cc::forward<Range>(range), cc::forward<T>(value)});
}

// [smart_range implementation]
template <class ContainerT>
template <class T>
constexpr auto smart_range<ContainerT>::pad_with(T&& value)
{
    return cr::pad_with(_container, cc::forward<T>(value));
}
}
