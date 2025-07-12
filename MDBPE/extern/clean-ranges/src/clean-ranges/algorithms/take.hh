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
struct take_iterator
{
    take_iterator(ItT it, EndT end, size_t cnt) : _it(cc::move(it)), _end(cc::move(end)), _cnt(cnt) {}

    constexpr decltype(auto) operator*() { return *_it; }
    constexpr void operator++()
    {
        ++_it;
        --_cnt;
    }
    constexpr bool operator!=(cc::sentinel) const { return _cnt > 0 && _it != _end; }

private:
    ItT _it;
    EndT _end;
    size_t _cnt;
};

template <class Range>
struct take_range
{
    constexpr auto begin() { return take_iterator(cc::begin(range), cc::end(range), cnt); }
    constexpr auto begin() const { return take_iterator(cc::begin(range), cc::end(range), cnt); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    size_t cnt;
};

template <class ItT, class EndT, class Predicate, class Expected>
struct take_while_iterator
{
    take_while_iterator(ItT it, EndT end, Predicate& pred, Expected) : _it(cc::move(it)), _end(cc::move(end)), _pred(pred) {}

    constexpr decltype(auto) operator*() { return *_it; }
    constexpr void operator++()
    {
        ++_it;
        ++_idx;
    }
    constexpr bool operator!=(cc::sentinel) { return _it != _end && cr::detail::call(_idx, _pred, *_it) == Expected::value; }

private:
    size_t _idx = 0;
    ItT _it;
    EndT _end;
    Predicate& _pred;
};

template <class Range, class Predicate, class Expected>
struct take_while_range
{
    constexpr auto begin() { return take_while_iterator(cc::begin(range), cc::end(range), static_cast<Predicate&>(pred), Expected{}); }
    constexpr auto begin() const { return take_while_iterator(cc::begin(range), cc::end(range), static_cast<Predicate&>(pred), Expected{}); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    Predicate pred;
};
}

/// skips the first `cnt` elements while iterating
template <class Range>
[[nodiscard]] constexpr auto take(Range&& range, size_t cnt)
{
    return cr::smart_range<cr::detail::take_range<Range>>({cc::forward<Range>(range), cnt});
}
template <class Range, class Predicate>
[[nodiscard]] constexpr auto take_while(Range&& range, Predicate&& pred)
{
    return cr::smart_range<cr::detail::take_while_range<Range, Predicate, std::true_type>>({cc::forward<Range>(range), cc::forward<Predicate>(pred)});
}
template <class Range, class Predicate>
[[nodiscard]] constexpr auto take_while_not(Range&& range, Predicate&& pred)
{
    return cr::smart_range<cr::detail::take_while_range<Range, Predicate, std::false_type>>({cc::forward<Range>(range), cc::forward<Predicate>(pred)});
}

// [smart_range implementation]
template <class ContainerT>
constexpr auto smart_range<ContainerT>::take(size_t cnt)
{
    return cr::take(_container, cnt);
}
template <class ContainerT>
template <class Predicate>
constexpr auto smart_range<ContainerT>::take_while(Predicate&& pred)
{
    return cr::take_while(_container, cc::forward<Predicate>(pred));
}
template <class ContainerT>
template <class Predicate>
constexpr auto smart_range<ContainerT>::take_while_not(Predicate&& pred)
{
    return cr::take_while_not(_container, cc::forward<Predicate>(pred));
}
}
