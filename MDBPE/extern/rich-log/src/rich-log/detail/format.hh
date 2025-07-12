#pragma once

#include <clean-core/format.hh>
#include <reflector/to_string.hh>

namespace rlog::detail
{
struct formatter
{
    // TODO: make extensions of default formatting easier
    template <class T>
    static void do_format(cc::stream_ref<char> s, T const& v, cc::string_view fmt_args)
    {
        if constexpr (cc::detail::has_to_string_ss_args<T>)
        {
            to_string(s, v, fmt_args);
        }
        else if constexpr (cc::detail::has_to_string_args<T>)
        {
            if constexpr (cc::detail::has_to_string_ss<T>)
            {
                if (fmt_args.empty())
                    to_string(s, v);
                else
                    s << to_string(v, fmt_args);
            }
            else
            {
                s << cc::string_view(to_string(v, fmt_args));
            }
        }
        else if constexpr (cc::detail::has_to_string_ss<T>)
        {
            to_string(s, v);
        }
        else if constexpr (cc::detail::has_to_string<T>)
        {
            to_string(s, cc::string_view(to_string(v)), fmt_args);
        }
        else if constexpr (cc::detail::has_member_to_string<T>)
        {
            to_string(s, cc::string_view(v.to_string()), fmt_args);
        }
        else if constexpr (cc::detail::has_member_toStdString<T>)
        {
            to_string(s, cc::string_view(v.toStdString()), fmt_args);
        }
        else if constexpr (rf::has_to_string<T>)
        {
            // TODO: rf::to_string with args and streams
            to_string(s, rf::to_string(v), fmt_args);
        }
        else
        {
            static_assert(cc::always_false<T>, "Type requires a to_string() function");
        }
    }

    using arg_info = cc::detail::default_formatter::arg_info;

    // TODO: maybe pull out the lambda into a template function to reduce symbol size a bit
    template <class T>
    static arg_info make_arg_info(T const& v)
    {
        return {[](cc::stream_ref<char> s, void const* data, cc::string_view options) -> void
                { formatter::do_format(s, *static_cast<T const*>(data), options); },
                &v,
                {}};
    }

    template <class T>
    static arg_info make_arg_info(cc::format_arg<T> const& a)
    {
        return {[](cc::stream_ref<char> ss, void const* data, cc::string_view options) -> void
                { formatter::do_format(ss, *static_cast<T const*>(data), options); },
                &a.value, a.name};
    }

    static void vformat_to(cc::stream_ref<char> s, cc::string_view fmt_str, cc::span<arg_info> args)
    {
        // combination of printf + pythonic syntax
        cc::detail::default_formatter::vformat_to(s, fmt_str, args);
    }
};

template <class... Args>
cc::string format(char const* fmt_str, Args const&... args)
{
    return cc::format<formatter>(fmt_str, args...);
}
}
