#pragma once

#include <reflector/detail/is_introspectable.hh>

namespace rf
{
/**
 * Calls introspect on t, given an inspector
 * NOTE: calling introspect should always happen using this function
 *       (which performs proper ADL lookup and will work with future customization points)
 *
 * Declaration example:
 *
 *    struct foo
 *    {
 *        int a;
 *        char b;
 *        bool c;
 *    };
 *
 *    template <class In>
 *    constexpr void introspect(In&& inpsect, foo& v)
 *    {
 *        inspect(v.a, "a");
 *        inspect(v.b, "b");
 *        inspect(v.c, "c");
 *    }
 *
 * Usage example:
 *
 *   foo f;
 *   rf::do_introspect([&](auto& v, cc::string_view name) {
 *       LOG("member {} has value {}", name, v);
 *   }, f);
 */
template <class T, class Inspector>
constexpr void do_introspect(Inspector&& inspector, T& t)
{
    static_assert(!std::is_const_v<std::remove_reference_t<T>>, "do_introspect must be called with an l-value ref");
    static_assert(is_introspectable<T>, "type is not introspectable. provide `template <class In> constexpr void introspect(In&& inspect, T& v);'");

    // TODO: other customization points
    introspect(inspector, t);
}

/**
 * Calls introspect_enum on t, given an inspector
 * NOTE: calling introspect_enum should always happen using this function
 *       (which performs proper ADL lookup and will work with future customization points)
 *
 * Declaration example:
 *
 *    enum class foo
 *    {
 *        val0,
 *        val1,
 *        val2
 *    };
 *
 *    template <class In>
 *    constexpr void introspect_enum(In&& inspect, foo& f)
 *    {
 *        inspect(f, foo::val0, "val0");
 *        inspect(f, foo::val1, "val1");
 *        inspect(f, foo::val2, "val2");
 *    }
 *
 * Usage example:
 *
 *   rf::do_introspect_enum([&](foo& val, foo ref_val, cc::string_view name) {
 *       if (val == ref_val)
 *           LOG("enum has value {}", name);
 *   }, my_enum_val);
 */
template <class T, class Inspector>
constexpr void do_introspect_enum(Inspector&& inspector, T& t)
{
    static_assert(std::is_enum_v<T>, "only works for enums");
    static_assert(!std::is_const_v<std::remove_reference_t<T>>, "do_introspect_enum must be called with an l-value ref");
    static_assert(is_enum_introspectable<T>,
                  "enum is not enum_introspectable. provide `template <class In> constexpr void introspect_enum(In&& inspect, "
                  "EnumT& v);'");

    // TODO: other customization points
    introspect_enum(inspector, t);
}
}
