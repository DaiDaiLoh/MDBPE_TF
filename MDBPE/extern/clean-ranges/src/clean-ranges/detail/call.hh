#pragma once

#include <type_traits>

#include <clean-core/forward.hh>
#include <clean-core/priority_tag.hh>

namespace cr::detail
{
template <class F, class Arg>
constexpr auto impl_obj_member_call(F&& fun, Arg&& arg, cc::priority_tag<1>) -> decltype(arg.*fun)
{
    return arg.*fun;
}
template <class F, class Arg>
constexpr auto impl_obj_member_call(F&& fun, Arg&& arg, cc::priority_tag<0>) -> decltype((*arg).*fun)
{
    return (*arg).*fun;
}

template <class F, class Arg>
constexpr auto impl_fun_member_call(size_t, F&& fun, Arg&& arg, cc::priority_tag<3>) -> decltype((arg.*fun)())
{
    return (arg.*fun)();
}
template <class F, class Arg>
constexpr auto impl_fun_member_call(size_t, F&& fun, Arg&& arg, cc::priority_tag<2>) -> decltype(((*arg).*fun)())
{
    return ((*arg).*fun)();
}
template <class F, class Arg>
constexpr auto impl_fun_member_call(size_t idx, F&& fun, Arg&& arg, cc::priority_tag<1>) -> decltype((arg.*fun)(idx))
{
    return (arg.*fun)(idx);
}
template <class F, class Arg>
constexpr auto impl_fun_member_call(size_t idx, F&& fun, Arg&& arg, cc::priority_tag<0>) -> decltype(((*arg).*fun)(idx))
{
    return ((*arg).*fun)(idx);
}

template <class F, class Arg>
constexpr auto impl_fun_call(size_t, F&& fun, Arg&& arg, cc::priority_tag<1>) -> decltype(fun(cc::forward<Arg>(arg)))
{
    return fun(cc::forward<Arg>(arg));
}
template <class F, class Arg>
constexpr auto impl_fun_call(size_t idx, F&& fun, Arg&& arg, cc::priority_tag<0>) -> decltype(fun(idx, cc::forward<Arg>(arg)))
{
    return fun(idx, cc::forward<Arg>(arg));
}

template <class F, class Arg>
constexpr decltype(auto) call(size_t idx, F&& fun, Arg&& arg)
{
    if constexpr (std::is_member_object_pointer_v<std::remove_reference_t<F>>)
        return cr::detail::impl_obj_member_call(cc::forward<F>(fun), cc::forward<Arg>(arg), cc::priority_tag<1>{});
    else if constexpr (std::is_member_function_pointer_v<std::remove_reference_t<F>>)
        return cr::detail::impl_fun_member_call(idx, cc::forward<F>(fun), cc::forward<Arg>(arg), cc::priority_tag<3>{});
    else
        return cr::detail::impl_fun_call(idx, cc::forward<F>(fun), cc::forward<Arg>(arg), cc::priority_tag<1>{});
}
}
