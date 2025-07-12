#pragma once

#include <cstddef>
#include <type_traits>

#include <clean-core/assert.hh>
#include <clean-core/bit_cast.hh>
#include <clean-core/has_operator.hh>
#include <clean-core/move.hh>

#include <reflector/introspect.hh>

namespace rf
{
/// returns lhs == rhs if defined, otherwise performs a memberwise comparison
template <class T>
[[nodiscard]] bool is_equal(T const& lhs, T const& rhs) noexcept;

/// lhs != rhs
template <class T>
[[nodiscard]] bool is_not_equal(T const& lhs, T const& rhs) noexcept
{
    return !is_equal(lhs, rhs);
}

/// lhs < rhs
template <class T>
[[nodiscard]] bool is_less(T const& lhs, T const& rhs) noexcept;

/// lhs >= rhs
template <class T>
[[nodiscard]] bool is_greater_equal(T const& lhs, T const& rhs) noexcept
{
    return !is_less(lhs, rhs);
}

/// lhs > rhs
template <class T>
[[nodiscard]] bool is_greater(T const& lhs, T const& rhs) noexcept
{
    return is_less(rhs, lhs);
}

/// lhs <= rhs
template <class T>
[[nodiscard]] bool is_less_equal(T const& lhs, T const& rhs) noexcept
{
    return !is_less(rhs, lhs);
}

// type operator versions
struct equal
{
    template <class T>
    [[nodiscard]] bool operator()(T const& lhs, T const& rhs) const noexcept
    {
        return rf::is_equal(lhs, rhs);
    }
};
struct not_equal
{
    template <class T>
    [[nodiscard]] bool operator()(T const& lhs, T const& rhs) const noexcept
    {
        return rf::is_not_equal(lhs, rhs);
    }
};
struct less
{
    template <class T>
    [[nodiscard]] bool operator()(T const& lhs, T const& rhs) const noexcept
    {
        return rf::is_less(lhs, rhs);
    }
};
struct less_equal
{
    template <class T>
    [[nodiscard]] bool operator()(T const& lhs, T const& rhs) const noexcept
    {
        return rf::is_less_equal(lhs, rhs);
    }
};
struct greater
{
    template <class T>
    [[nodiscard]] bool operator()(T const& lhs, T const& rhs) const noexcept
    {
        return rf::is_greater(lhs, rhs);
    }
};
struct greater_equal
{
    template <class T>
    [[nodiscard]] bool operator()(T const& lhs, T const& rhs) const noexcept
    {
        return rf::is_greater_equal(lhs, rhs);
    }
};

namespace detail
{
template <class FunctorT>
struct MemberwiseComparator
{
    MemberwiseComparator(FunctorT comp_op, size_t outer_size) : outer_size(outer_size), comp_op(comp_op) {}

    std::byte const* lhs_raw;
    std::byte const* rhs_raw;
    size_t outer_size;
    FunctorT comp_op;
    bool condition_true = true;

    template <class T, class... Args>
    void operator()(T const& rhs_member, Args&&...) noexcept
    {
        static_assert(sizeof(T) > 0, "No incomplete members allowed");
        if (condition_true)
        {
            size_t const member_offset = reinterpret_cast<std::byte const*>(&rhs_member) - rhs_raw;
            CC_ASSERT(member_offset < outer_size);
            T const& lhs_member = *reinterpret_cast<T const*>(lhs_raw + member_offset);
            static_assert(std::is_same_v<T const&, decltype(lhs_member)>);
            condition_true = comp_op(lhs_member, rhs_member);
        }
    }
};
}

template <class T>
[[nodiscard]] bool is_equal(T const& lhs, T const& rhs) noexcept
{
    static_assert(sizeof(T) > 0, "No incomplete types allowed");
    if constexpr (cc::has_operator_equal<T, T>)
    {
        return lhs == rhs;
    }
    else
    {
        auto const comp_op = [](auto const& lhs, auto const& rhs) { return rf::is_equal(lhs, rhs); };
        auto comparator = detail::MemberwiseComparator(comp_op, sizeof(T));
        comparator.lhs_raw = reinterpret_cast<std::byte const*>(&lhs);
        comparator.rhs_raw = reinterpret_cast<std::byte const*>(&rhs);
        do_introspect<T>(comparator, const_cast<T&>(rhs));
        return comparator.condition_true;
    }
}


template <class T>
[[nodiscard]] bool is_less(T const& lhs, T const& rhs) noexcept
{
    static_assert(sizeof(T) > 0, "No incomplete types allowed");
    if constexpr (cc::has_operator_less<T, T>)
    {
        return lhs < rhs;
    }
    else
    {
        auto const comp_op = [](auto const& lhs, auto const& rhs) { return rf::is_less(lhs, rhs); };
        auto comparator = detail::MemberwiseComparator(comp_op, sizeof(T));
        comparator.lhs_raw = reinterpret_cast<std::byte const*>(&lhs);
        comparator.rhs_raw = reinterpret_cast<std::byte const*>(&rhs);
        do_introspect<T>(comparator, const_cast<T&>(rhs));
        return comparator.condition_true;
    }
}
}
