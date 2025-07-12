#pragma once

#include <clean-core/iterator.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/smart_range.hh>

namespace cr
{
namespace detail
{
template <class ItT, class EndT, class Range>
struct times_iterator
{
    times_iterator(ItT it, EndT end, Range& range, size_t cnt) : _it(cc::move(it)), _end(cc::move(end)), _range(range), _cnt(cnt) {}

    constexpr decltype(auto) operator*() { return *_it; }
    constexpr void operator++()
    {
        ++_it;

        if (!(_it != _end))
        {
            --_cnt;
            _it = cc::begin(_range);
        }
    }
    constexpr bool operator!=(cc::sentinel) const { return _cnt != 0 && _it != _end; }

private:
    ItT _it;
    EndT _end;
    Range& _range;
    size_t _cnt;
};

template <class Range>
struct times_range
{
    constexpr auto begin() { return times_iterator(cc::begin(range), cc::end(range), range, cnt); }
    constexpr auto begin() const { return times_iterator(cc::begin(range), cc::end(range), range, cnt); }
    constexpr cc::sentinel end() const { return {}; }

    Range range;
    size_t cnt;
};
}

/// returns a range that repeats the given range `cnt` times
template <class Range>
[[nodiscard]] constexpr auto times(Range&& range, size_t cnt)
{
    return smart_range<detail::times_range<Range>>({cc::forward<Range>(range), cnt});
}

// [smart_range implementation]
template <class ContainerT>
constexpr auto smart_range<ContainerT>::times(size_t cnt)
{
    return cr::times(_container, cnt);
}
}
