#pragma once

#include <cstddef>

#include <clean-core/array.hh>
#include <clean-core/invoke.hh>
#include <clean-core/string_view.hh>

#include <reflector/introspect.hh>

namespace rf
{
namespace detail
{
template <class EnumT>
constexpr size_t enum_value_count()
{
    size_t cnt = 0;
    EnumT v = {};
    rf::do_introspect_enum([&cnt](EnumT&, EnumT, cc::string_view) { ++cnt; }, v);
    return cnt;
}

template <class EnumT>
constexpr auto enum_values()
{
    constexpr auto cnt = enum_value_count<EnumT>();
    auto r = cc::array<EnumT, cnt>();
    size_t i = 0;
    EnumT v = {};
    rf::do_introspect_enum([&r, &i](EnumT&, EnumT val, cc::string_view) { r[i++] = val; }, v);
    return r;
}

template <class EnumT>
constexpr auto enum_names()
{
    constexpr auto cnt = enum_value_count<EnumT>();
    auto r = cc::array<cc::string_view, cnt>();
    size_t i = 0;
    EnumT v = {};
    rf::do_introspect_enum([&r, &i](EnumT&, EnumT, cc::string_view name) { r[i++] = name; }, v);
    return r;
}
}

/// the number of (registered) enum values
template <class EnumT>
constexpr size_t enum_value_count = detail::enum_value_count<EnumT>();

/// an array containing all registered enum values
/// NOTE: index does NOT correspond to enum value!
template <class EnumT>
constexpr cc::array<EnumT, enum_value_count<EnumT>> enum_values = detail::enum_values<EnumT>();

/// an array containing all registered enum names
/// NOTE: index does NOT correspond to enum value!
///       however, same index in rf::enum_values corresponds to same index in rf::enum_names
template <class EnumT>
constexpr cc::array<cc::string_view, enum_value_count<EnumT>> enum_names = detail::enum_names<EnumT>();

/// converts an enum value to its name
/// (returns def_value if for some reason the value is not registered)
template <class EnumT>
constexpr cc::string_view enum_to_string(EnumT value, cc::string_view def_value = "<invalid>")
{
    cc::string_view r = def_value;
    rf::do_introspect_enum(
        [value, &r](EnumT&, EnumT val, cc::string_view name) {
            if (value == val)
                r = name;
        },
        value);
    return r;
}

/// converts a string to an enum value
/// returns false if the conversion was not succesful, otherwise the result is written to 'value'
/// NOTE: comparison is case SENSITIVE
template <class EnumT>
constexpr bool enum_from_string(cc::string_view name, EnumT& value)
{
    auto ok = false;
    rf::do_introspect_enum(
        [&](EnumT&, EnumT ref_val, cc::string_view ref_name) {
            if (!ok && ref_name == name)
            {
                value = ref_val;
                ok = true;
            }
        },
        value);
    return ok;
}

/// same as bool enum_from_string(name, value)
/// but returns the value and has undefined behavior if name is not found
template <class EnumT>
constexpr EnumT enum_from_string(cc::string_view name)
{
    EnumT val = {};
    if (enum_from_string(name, val))
        return val;
    CC_UNREACHABLE_SWITCH_WORKAROUND(val);
}

/// returns true if the given enum value was registered
template <class EnumT>
constexpr bool is_enum_value_valid(EnumT value)
{
    for (EnumT v : rf::enum_values<EnumT>)
        if (v == value)
            return true;
    return false;
}

namespace detail
{
template <int I, class Fun, class EnumT, class... Args>
constexpr decltype(auto) enum_invoke(EnumT val, Fun&& fun, Args&&... args)
{
    constexpr EnumT v = rf::enum_values<EnumT>[I];
    if (v == val)
        return cc::invoke(fun, std::integral_constant<EnumT, v>(), cc::forward<Args>(args)...);
    else if constexpr (I < rf::enum_value_count<EnumT> - 1)
        return detail::enum_invoke<I + 1>(val, fun, cc::forward<Args>(args)...);

    CC_UNREACHABLE("unknown enum value (did you forget to add it to introspect_enum?)");
}
}

/// takes a runtime enum value and invokes 'fun' with a compile time value for it
/// (makes it easy to write functions that instantiate different types for enum values)
/// the first argument to 'fun' is an std::integral_constant<EnumT, v> containing the value
/// all other arguments are simply forwarded 'args'
/// NOTE: currently, all 'fun' instantiations need to return the same type
///       value must be registered, otherwise this triggers UB (an assertion)
///
/// Usage:
///
///   my_enum value = ...;
///   rf::enum_invoke(value, [&](auto v, int i) {
///     using type_t = type_for_my_enum<v.value>; // e.g. a partial specialization, note v.value is same as decltype(v)::value (and constexpr)
///     auto obj = type_t(i);
///     // do something with obj
///   }, 17);
template <class EnumT, class Fun, class... Args>
constexpr decltype(auto) enum_invoke(EnumT val, Fun&& fun, Args&&... args)
{
    static_assert(rf::is_enum_introspectable<EnumT>, "enum must be introspectable");
    static_assert((rf::enum_value_count<EnumT>) > 0, "enum must have at least one value");
    return detail::enum_invoke<0>(val, fun, cc::forward<Args>(args)...);
}
}
