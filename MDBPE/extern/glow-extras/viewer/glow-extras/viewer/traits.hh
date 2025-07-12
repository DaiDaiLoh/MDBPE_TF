#pragma once

#include <type_traits>

#include <polymesh/fwd.hh>

namespace glow
{
namespace viewer
{
namespace detail
{
struct type_like_helper
{
    template <class T>
    static auto is_pos2_like(T const& v) -> decltype(float(v[0]), std::enable_if_t<sizeof(T) == 2 * sizeof(v[0]), std::true_type>());
    static std::false_type is_pos2_like(...);

    template <class T>
    static auto is_pos3_like(T const& v) -> decltype(float(v[0]), std::enable_if_t<sizeof(T) == 3 * sizeof(v[0]), std::true_type>());
    static std::false_type is_pos3_like(...);

    template <class T>
    static auto is_pos4_like(T const& v) -> decltype(float(v[0]), std::enable_if_t<sizeof(T) == 4 * sizeof(v[0]), std::true_type>());
    static std::false_type is_pos4_like(...);

    template <class T>
    static auto is_color3_like(T const& v)
        -> decltype(float(v.r), float(v.g), float(v.b), std::enable_if_t<sizeof(T) == 3 * sizeof(v.r), std::true_type>());
    static std::false_type is_color3_like(...);

    template <class T>
    static auto is_color4_like(T const& v)
        -> decltype(float(v.r), float(v.g), float(v.b), float(v.a), std::enable_if_t<sizeof(T) == 4 * sizeof(v.r), std::true_type>());
    static std::false_type is_color4_like(...);
};

// ======================= Traits =======================
#define GLOW_VIEWER_IMPL_TYPE_LIKE_TRAIT(trait) \
    template <class T>                          \
    constexpr bool trait = decltype(type_like_helper::trait(std::declval<T>()))::value // force ;

GLOW_VIEWER_IMPL_TYPE_LIKE_TRAIT(is_pos2_like);
GLOW_VIEWER_IMPL_TYPE_LIKE_TRAIT(is_pos3_like);
GLOW_VIEWER_IMPL_TYPE_LIKE_TRAIT(is_pos4_like);

GLOW_VIEWER_IMPL_TYPE_LIKE_TRAIT(is_color3_like);
GLOW_VIEWER_IMPL_TYPE_LIKE_TRAIT(is_color4_like);

#undef GLOW_VIEWER_IMPL_TYPE_LIKE_TRAIT


template <class T>
struct is_vertex_attr_t : std::false_type
{
};
template <class T>
struct is_vertex_attr_t<pm::vertex_attribute<T>> : std::true_type
{
};
template <class T>
constexpr bool is_vertex_attr = is_vertex_attr_t<T>::value;

template <class T>
struct is_face_attr_t : std::false_type
{
};
template <class T>
struct is_face_attr_t<pm::face_attribute<T>> : std::true_type
{
};
template <class T>
constexpr bool is_face_attr = is_face_attr_t<T>::value;

template <class T>
struct is_edge_attr_t : std::false_type
{
};
template <class T>
struct is_edge_attr_t<pm::edge_attribute<T>> : std::true_type
{
};
template <class T>
constexpr bool is_edge_attr = is_edge_attr_t<T>::value;

template <class T>
struct is_halfedge_attr_t : std::false_type
{
};
template <class T>
struct is_halfedge_attr_t<pm::halfedge_attribute<T>> : std::true_type
{
};
template <class T>
constexpr bool is_halfedge_attr = is_halfedge_attr_t<T>::value;
}
}
}
