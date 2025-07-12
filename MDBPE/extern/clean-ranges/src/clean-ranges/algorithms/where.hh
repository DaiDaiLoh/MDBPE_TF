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
template <class ItT, class EndT, class Predicate, class ExpectT>
struct filtered_iterator
{
    filtered_iterator(ItT it, EndT end, Predicate& pred, ExpectT) : _it(cc::move(it)), _end(cc::move(end)), _pred(pred)
    {
        while (_it != _end && cr::detail::call(_idx, _pred, *_it) != ExpectT::value)
        {
            ++_idx;
            ++_it;
        }
    }

    constexpr decltype(auto) operator*() { return *_it; }
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
    Predicate& _pred;
};

template <class Range, class Predicate, class ExpectT>
struct filtered_range
{
    constexpr auto begin() { return filtered_iterator(cc::begin(range), cc::end(range), static_cast<Predicate&>(pred), ExpectT{}); }
    constexpr auto begin() const { return filtered_iterator(cc::begin(range), cc::end(range), static_cast<Predicate const&>(pred), ExpectT{}); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    Predicate pred;
};
}

/// returns a new range consisting only of elements where `pred(e)` is true
template <class Range, class Predicate>
[[nodiscard]] constexpr auto where(Range&& range, Predicate&& pred)
{
    return smart_range<detail::filtered_range<Range, Predicate, std::true_type>>({cc::forward<Range>(range), cc::forward<Predicate>(pred)});
}

/// returns a new range consisting only of elements where `pred(e)` is false
template <class Range, class Predicate>
[[nodiscard]] constexpr auto where_not(Range&& range, Predicate&& pred)
{
    return smart_range<detail::filtered_range<Range, Predicate, std::false_type>>({cc::forward<Range>(range), cc::forward<Predicate>(pred)});
}

// [smart_range implementation]
template <class ContainerT>
template <class Predicate>
constexpr auto smart_range<ContainerT>::where(Predicate&& pred)
{
    return cr::where(_container, cc::forward<Predicate>(pred));
}
template <class ContainerT>
template <class Predicate>
constexpr auto smart_range<ContainerT>::where_not(Predicate&& pred)
{
    return cr::where_not(_container, cc::forward<Predicate>(pred));
}
}
