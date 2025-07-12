#pragma once

#include <clean-core/forward.hh>
#include <clean-core/move.hh>
#include <clean-core/priority_tag.hh>
#include <clean-core/sentinel.hh>

#include <clean-ranges/detail/range_helper.hh>

namespace cr::detail
{
template <class ItT, class IncT>
struct unbounded_range : private IncT
{
    constexpr unbounded_range(ItT it, IncT inc) : IncT(cc::move(inc)), _it(cc::move(it)) {}

    constexpr auto begin() const { return *this; }
    constexpr cc::sentinel end() const { return {}; }

    constexpr decltype(auto) operator*() { return cr::detail::impl_deref(_it, cc::priority_tag<1>{}); }
    constexpr void operator++() { IncT::operator()(_it); }

    constexpr bool operator!=(cc::sentinel) const { return true; }

private:
    ItT _it;
};
}
