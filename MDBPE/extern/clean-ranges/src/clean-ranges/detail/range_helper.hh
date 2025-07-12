#pragma once

#include <utility>

#include <clean-core/always_false.hh>
#include <clean-core/forward.hh>
#include <clean-core/priority_tag.hh>
#include <clean-core/sentinel.hh>

namespace cr::detail
{
template <class ItT>
constexpr auto impl_deref(ItT&& it, cc::priority_tag<1>) -> decltype(*it)
{
    return *it;
}
template <class ItT>
constexpr auto impl_deref(ItT&& it, cc::priority_tag<0>)
{
    return cc::forward<ItT>(it);
}

struct pre_increment
{
    template <class T>
    constexpr void operator()(T& it)
    {
        ++it;
    }
};

template <class StartT, class IncT, class = void>
struct inc_plus_eq : std::false_type
{
};
template <class StartT, class IncT>
struct inc_plus_eq<StartT, IncT, std::void_t<decltype(std::declval<StartT&>() += std::declval<IncT>())>> : std::true_type
{
};

template <class StartT, class IncT, class = void>
struct inc_fun_assign : std::false_type
{
};
template <class StartT, class IncT>
struct inc_fun_assign<StartT, IncT, std::void_t<decltype(std::declval<StartT&>() = std::declval<IncT>()(std::declval<StartT>()))>> : std::true_type
{
};

template <class StartT, class IncT, class = void>
struct inc_fun_mut : std::false_type
{
};
template <class StartT, class IncT>
struct inc_fun_mut<StartT, IncT, std::void_t<decltype(std::declval<IncT>()(std::declval<StartT&>()))>> : std::true_type
{
};

template <class StartT, class IncT>
constexpr auto get_inc_class_for(IncT&& inc)
{
    if constexpr (inc_plus_eq<StartT, IncT>::value)
        return [inc = cc::forward<IncT>(inc)](StartT& v) { v += inc; };
    else if constexpr (inc_fun_assign<StartT, IncT>::value)
        return [inc = cc::forward<IncT>(inc)](StartT& v) { v = inc(v); };
    else if constexpr (inc_fun_mut<StartT, IncT>::value)
        return cc::forward<IncT>(inc);
    else
        static_assert(cc::always_false<StartT, IncT>, "increment not supported");
}

template <class ItT, class = void>
struct is_sentinel_iterator : std::false_type
{
};
template <class ItT>
struct is_sentinel_iterator<ItT,
                            std::void_t<decltype(*std::declval<ItT>()),  //
                                        decltype(++std::declval<ItT>()), //
                                        decltype(std::declval<ItT>() != cc::sentinel{})>> : std::true_type
{
};
}
