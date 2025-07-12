#pragma once

#include <reflector/detail/null_inspector.hh>

#include <type_traits>

namespace rf
{
namespace detail
{
template <class T, class = void>
struct is_introspectable_t : std::false_type
{
};
template <class T>
struct is_introspectable_t<T, std::void_t<decltype(introspect(null_inspector{}, std::declval<T&>()))>> : std::true_type
{
};

template <class T, class = void>
struct is_enum_introspectable_t : std::false_type
{
};
template <class T>
struct is_enum_introspectable_t<T, std::void_t<decltype(introspect_enum(null_inspector{}, std::declval<T&>()))>> : std::true_type
{
};
}

/// is true iff an introspect function can be found via ADL
template <class T>
static constexpr bool is_introspectable = rf::detail::is_introspectable_t<T>::value;

/// is true iff an introspect_enum function can be found via ADL (and the type is an enum)
template <class T>
static constexpr bool is_enum_introspectable = std::is_enum_v<T>&& rf::detail::is_enum_introspectable_t<T>::value;
}
