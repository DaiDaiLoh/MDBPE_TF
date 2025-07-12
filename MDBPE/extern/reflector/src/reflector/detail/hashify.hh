#pragma once

#include <clean-core/hash.hh>

#include <reflector/introspect.hh>

namespace rf::detail
{
template <class T>
constexpr uint64_t impl_make_hash(T const& v) noexcept;

struct hash_inspector
{
    uint64_t h = cc::hash_combine();

    template <class T, class... Args>
    constexpr void operator()(T const& v, Args&&...) noexcept
    {
        h = cc::hash_combine(h, impl_make_hash(v));
    }
};

template <class T, class = void>
struct can_hash_t : std::false_type
{
};
template <class T>
struct can_hash_t<T, std::enable_if_t<cc::can_hash<T> || rf::is_introspectable<T>>> : std::true_type
{
};

template <class T>
constexpr uint64_t impl_make_hash(T const& v) noexcept
{
    if constexpr (cc::can_hash<T>)
        return cc::hash<T>{}(v);
    else
    {
        static_assert(rf::is_introspectable<T>, "must be introspectable");

        hash_inspector i;
        rf::do_introspect(i, const_cast<T&>(v)); // promise we will not change v
        return i.h;
    }
}
}
