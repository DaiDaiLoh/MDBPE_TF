#pragma once

#include <clean-core/enable_if.hh>

#include <reflector/detail/stringify.hh>

namespace rf
{
template <class T>
static constexpr bool has_to_string = ::rf_external_detail::has_to_string_t<T>::value;

/// generic, introspection-powered to_string
///
/// in this order, the following are tested:
/// * to_string(value)
/// * cc::to_string(value)
/// * introspect(..., value)
///
/// TODO: implement stream-based version for better performance
template <class T, cc::enable_if<has_to_string<T>> = true>
cc::string to_string(T const& value)
{
    auto s = ::rf_external_detail::impl_to_string(value, ::rf_external_detail::to_string_max_prio);

    if constexpr (std::is_same_v<decltype(s), cc::string>)
        return s;
    else
        return cc::string(cc::string_view(s));
}
}
