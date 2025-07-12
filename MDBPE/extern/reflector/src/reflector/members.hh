#pragma once

#include <clean-core/array.hh>
#include <clean-core/string_view.hh>

#include <reflector/introspect.hh>

namespace rf
{
struct member_info
{
    cc::string_view name;
    size_t offset = 0;
};

struct class_info_t
{
};

// ==================================================
// runtime/hybrid information: (dynamic reflection)

template <class T>
constexpr size_t get_member_count(T const& t = {});

template <class T>
constexpr auto get_member_infos(T const& t = {});


// ==================================================
// compile time information: (static reflection)

template <class T>
static constexpr size_t member_count = get_member_count<T>();

template <class T>
inline constexpr auto member_infos = get_member_infos<T>();


// ==================================================
// implementation details:

namespace detail
{
struct MemberCounter
{
    size_t cnt = 0;
    template <class... Args>
    constexpr void operator()(Args&&...)
    {
        ++cnt;
    }
};

struct MemberInfoBuilder
{
    member_info* members;
    size_t struct_start;
    size_t struct_size;
    template <class T, class... Args>
    constexpr void operator()(T&, cc::string_view name, Args&&...)
    {
        members->name = name;
        members->offset = 0;
        ++members;
    }
};
}

template <class T>
constexpr size_t get_member_count(T const& t)
{
    detail::MemberCounter counter;
    rf::do_introspect(counter, const_cast<T&>(t));
    return counter.cnt;
}

template <class T>
constexpr auto get_member_infos(T const& t)
{
    auto constexpr cnt = member_count<T>;
    cc::array<member_info, cnt> members = {};

    auto builder = detail::MemberInfoBuilder{members.data(), 0, 0};
    rf::do_introspect(builder, const_cast<T&>(t));

    return members;
}
}
