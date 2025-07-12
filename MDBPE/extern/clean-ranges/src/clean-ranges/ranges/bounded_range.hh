#pragma once

#include <clean-core/forward.hh>
#include <clean-core/move.hh>
#include <clean-core/priority_tag.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/detail/range_helper.hh>

namespace cr::detail
{
template <class ItT, class EndT, class IncT>
struct bounded_range : private IncT
{
    constexpr bounded_range(ItT it, EndT end, IncT inc) : IncT(cc::move(inc)), _it(cc::move(it)), _end(cc::move(end)) {}

    constexpr auto begin() const { return *this; }
    constexpr cc::sentinel end() const { return {}; }

    constexpr decltype(auto) operator*() { return cr::detail::impl_deref(_it, cc::priority_tag<1>{}); }
    constexpr void operator++() { IncT::operator()(_it); }

    constexpr bool operator!=(cc::sentinel) const { return _it != _end; }

private:
    ItT _it;
    EndT _end;
};
}
