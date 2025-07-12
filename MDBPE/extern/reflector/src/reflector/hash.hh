#pragma once

#include <clean-core/hash.hh>

#include <reflector/detail/hashify.hh>

namespace rf
{
/// general-purpose reflection-based hashing
struct hash
{
    template <class T>
    [[nodiscard]] constexpr uint64_t operator()(T const& v) const noexcept
    {
        return rf::detail::impl_make_hash(v);
    }
};

/// check if reflection-based hashing is available
template <class T>
static constexpr bool can_hash = detail::can_hash_t<T>::value;

/// variadic reflection-based hashing
template <class... Args>
[[nodiscard]] constexpr uint64_t make_hash(Args const&... values)
{
    return cc::make_hash<rf::hash>(values...);
}
}
