#pragma once

#include <clean-core/move.hh>
#include <clean-core/sentinel.hh>

namespace cr::detail
{
template <class ItT>
struct sentinel_range
{
    constexpr sentinel_range(ItT it) : _it(cc::move(it)) {}

    constexpr auto begin() const { return *this; }
    constexpr cc::sentinel end() const { return {}; }

    constexpr decltype(auto) operator*() { return *_it; }
    constexpr void operator++() { ++_it; }

    constexpr bool operator!=(cc::sentinel) const { return _it != cc::sentinel{}; }

private:
    ItT _it;
};
}
