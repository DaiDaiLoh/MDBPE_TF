#pragma once

#include <map>
#include <string>
#include <string_view>

namespace glow::util
{
namespace detail
{
struct void_less
{
    using is_transparent = void;

    template <class T, class U>
    constexpr auto operator()(T&& lhs, U&& rhs) const noexcept
    {
        return std::forward<T>(lhs) < std::forward<U>(rhs);
    }
};
}

// A string to T map that supports heterogenous lookup via std::string_view,
// and only creates a std::string key if none exists yet
template <class T>
struct string_map : std::map<std::string, T, detail::void_less>
{
    using super_t = std::map<std::string, T, detail::void_less>;

    using super_t::operator[];

    T& operator[](std::string_view key)
    {
        if (auto it = this->find(key); it != this->end())
            return it->second;
        return super_t::operator[](std::string(key));
    }

    T& operator[](char const* key)
    {
        if (auto it = this->find(key); it != this->end())
            return it->second;
        return super_t::operator[](std::string(key));
    }
};
}
