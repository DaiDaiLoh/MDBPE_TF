#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/detail/call.hh>
#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT>
struct drop_iterator
{
    drop_iterator(ItT it, EndT end, size_t cnt) : _it(cc::move(it)), _end(cc::move(end))
    {
        while (cnt > 0 && _it != _end)
        {
            --cnt;
            ++_it;
        }
    }

    constexpr decltype(auto) operator*() { return *_it; }
    constexpr void operator++() { ++_it; }
    constexpr bool operator!=(cc::sentinel) const { return _it != _end; }

private:
    ItT _it;
    EndT _end;
};

template <class Range>
struct drop_range
{
    constexpr auto begin() { return drop_iterator(cc::begin(range), cc::end(range), cnt); }
    constexpr auto begin() const { return drop_iterator(cc::begin(range), cc::end(range), cnt); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    size_t cnt;
};

template <class ItT, class EndT>
struct drop_while_iterator
{
    template <class Predicate, class Expected>
    drop_while_iterator(ItT it, EndT end, Predicate&& pred, Expected) : _it(cc::move(it)), _end(cc::move(end))
    {
        size_t idx = 0;
        while (_it != _end && cr::detail::call(idx, pred, *_it) == Expected::value)
        {
            ++idx;
            ++_it;
        }
    }

    constexpr decltype(auto) operator*() { return *_it; }
    constexpr void operator++() { ++_it; }
    constexpr bool operator!=(cc::sentinel) const { return _it != _end; }

private:
    ItT _it;
    EndT _end;
};

template <class Range, class Predicate, class Expected>
struct drop_while_range
{
    constexpr auto begin() { return drop_while_iterator(cc::begin(range), cc::end(range), pred, Expected{}); }
    constexpr auto begin() const { return drop_while_iterator(cc::begin(range), cc::end(range), pred, Expected{}); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    Predicate pred;
};
}

/// skips the first `cnt` elements while iterating
template <class Range>
[[nodiscard]] constexpr auto drop(Range&& range, size_t cnt)
{
    return cr::smart_range(cr::detail::drop_range<Range>{cc::forward<Range>(range), cnt});
}
template <class Range, class Predicate>
[[nodiscard]] constexpr auto drop_while(Range&& range, Predicate&& pred)
{
    return cr::smart_range(cr::detail::drop_while_range<Range, Predicate, std::true_type>{cc::forward<Range>(range), cc::forward<Predicate>(pred)});
}
template <class Range, class Predicate>
[[nodiscard]] constexpr auto drop_while_not(Range&& range, Predicate&& pred)
{
    return cr::smart_range(cr::detail::drop_while_range<Range, Predicate, std::false_type>{cc::forward<Range>(range), cc::forward<Predicate>(pred)});
}

// [smart_range implementation]
template <class ContainerT>
constexpr auto smart_range<ContainerT>::drop(size_t cnt)
{
    return cr::drop(_container, cnt);
}
template <class ContainerT>
template <class Predicate>
constexpr auto smart_range<ContainerT>::drop_while(Predicate&& pred)
{
    return cr::drop_while(_container, cc::forward<Predicate>(pred));
}
template <class ContainerT>
template <class Predicate>
constexpr auto smart_range<ContainerT>::drop_while_not(Predicate&& pred)
{
    return cr::drop_while_not(_container, cc::forward<Predicate>(pred));
}
}
