#pragma once

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

#include <typed-geometry/feature/assert.hh>

namespace glow
{
template <class T>
struct array_view;

namespace detail
{
template <class Range, class T, class = void>
struct convertible_to_array_view_t : std::false_type
{
};
template <class Range, class T>
struct convertible_to_array_view_t<Range,
                                   T,
                                   std::void_t<decltype(static_cast<T*>(std::declval<Range>().data()), static_cast<size_t>(std::declval<Range>().size()))>>
  : std::true_type
{
};

template <class Range, class T>
static constexpr inline bool convertible_to_array_view = convertible_to_array_view_t<Range, T>::value;
}

// A non-owning wrapper around contiguous data
// CAUTION: only use this as a parameter, never as a member!
template <class T>
struct array_view
{
    using element_type = T;
    using value_type = std::remove_cv_t<T>;
    using index_type = size_t;
    using different_type = std::ptrdiff_t;
    using pointer = T*;
    using const_pointer = T const*;
    using reference = T&;
    using const_reference = T const&;
    using iterator = T*;
    using const_iterator = T const*;
    using void_t = std::conditional_t<std::is_const_v<T>, void const, void>;

    constexpr array_view() = default;

    template <size_t N>
    constexpr array_view(T (&a)[N]) : _data(a), _size(N)
    {
    }

    constexpr array_view(std::initializer_list<std::remove_const_t<T>> l) : _data(l.begin()), _size(l.size())
    {
        static_assert(std::is_const_v<T>, "the initializer_list ctor only works for const types");
    }

    constexpr explicit array_view(T* data, size_t size) : _data(data), _size(size) {}
    constexpr explicit array_view(void_t* data, size_t size) : _data(reinterpret_cast<T*>(data)), _size(size) {}

    template <class Range, class = std::enable_if_t<detail::convertible_to_array_view<Range, T>>>
    constexpr array_view(Range&& r) : _data(static_cast<T*>(r.data())), _size(static_cast<size_t>(r.size()))
    {
    }

    constexpr T* data() const { return _data; }
    constexpr size_t size() const { return _size; }

    constexpr T* begin() const { return _data; }
    constexpr T* end() const { return _data + _size; }

    constexpr T& operator[](size_t s) const
    {
        TG_ASSERT(s < _size && "out of bounds");
        return _data[s];
    }

    constexpr bool empty() const { return _size == 0; }

    array_view<std::byte const> as_bytes() const
    {
#ifndef GLOW_HAS_GLM
        static_assert(std::is_trivially_copyable_v<T>, "cannot make byte view for non-trivial type");
#endif
        return array_view<std::byte const>{reinterpret_cast<std::byte const*>(_data), _size * sizeof(T)};
    }

private:
    T* _data = nullptr;
    size_t _size = 0;
};

namespace detail
{
template <class Range>
constexpr bool can_make_array_view = convertible_to_array_view<Range, void const>;
}

template <class Range, class = std::enable_if_t<detail::can_make_array_view<Range>>>
auto make_array_view(Range&& r)
{
    using T = std::remove_reference_t<decltype(r.data()[0])>;
    return array_view<T>(std::forward<Range>(r));
}

template <class T>
array_view<std::byte const> as_byte_view(T const& v)
{
    if constexpr (detail::can_make_array_view<T const&>)
        return make_array_view(v).as_bytes();
    else
    {
#ifndef GLOW_HAS_GLM
        static_assert(std::is_trivially_copyable_v<T>, "type must be trivial");
#endif
        return array_view<T const>(&v, 1).as_bytes();
    }
}
}
