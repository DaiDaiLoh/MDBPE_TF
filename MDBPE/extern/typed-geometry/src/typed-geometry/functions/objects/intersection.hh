#pragma once

#include <clean-core/optional.hh>

#include <typed-geometry/feature/assert.hh>
#include <typed-geometry/functions/basic/scalar_math.hh>

#include <typed-geometry/types/objects/aabb.hh>
#include <typed-geometry/types/objects/box.hh>
#include <typed-geometry/types/objects/capsule.hh>
#include <typed-geometry/types/objects/cylinder.hh>
#include <typed-geometry/types/objects/ellipse.hh>
#include <typed-geometry/types/objects/halfspace.hh>
#include <typed-geometry/types/objects/hemisphere.hh>
#include <typed-geometry/types/objects/inf_cone.hh>
#include <typed-geometry/types/objects/inf_cylinder.hh>
#include <typed-geometry/types/objects/line.hh>
#include <typed-geometry/types/objects/plane.hh>
#include <typed-geometry/types/objects/pyramid.hh>
#include <typed-geometry/types/objects/ray.hh>
#include <typed-geometry/types/objects/segment.hh>
#include <typed-geometry/types/objects/sphere.hh>
#include <typed-geometry/types/objects/triangle.hh>
#include <typed-geometry/types/span.hh>

#include <typed-geometry/functions/matrix/determinant.hh>
#include <typed-geometry/functions/matrix/inverse.hh>
#include <typed-geometry/functions/vector/cross.hh>
#include <typed-geometry/functions/vector/dot.hh>
#include <typed-geometry/functions/vector/length.hh>

#include "aabb.hh"
#include "centroid.hh"
#include "closest_points.hh"
#include "contains.hh"
#include "coordinates.hh"
#include "direction.hh"
#include "faces.hh"
#include "normal.hh"
#include "plane.hh"
#include "project.hh"

#include <array>
#include <type_traits> // ?
#include <utility>
#include <vector> // ?

// family of intersection functions:

// intersects(a, b)              -> bool
// intersects_conservative(a, b) -> bool
// intersection(a, b)            -> ???
// intersection_safe(a, b)       -> cc::optional<???>
// intersection_parameter(a, b)  -> coords? (for a line or a ray: hits<N, ScalarT> or cc::optional<hit_interval> (when b is solid))
// intersection_parameters(a, b) -> pair<coords, coords>?
// intersection_exact(a, b)      -> variant
// closest_intersection(a, b)            -> position/object (for a ray: cc::optional<pos>)
// closest_intersection_parameter(a, b)  -> coords (for a ray: cc::optional<ScalarT>)

// "intersects" returns true iff any point lies in a and in b
// "intersects_conservative" returns true if any point lies in a and in b, but might also return true if they are disjoint
// "intersection" returns an object describing the intersection (NOTE: does NOT handle degenerate cases)
// "intersection_safe" is the same as "intersection" but returns nullopt for degenerate cases
// "intersection_parameter" returns coordinates for the first object such that a[coords] == intersection(a, b)
// "intersection_parameters" returns coordinates for both objects
// "intersection_exact" returns a variant type describing all possible intersections, including degenerate cases
// the "closest_" variants only return the closest intersection for objects where that concept is applicable (e.g. for rays)

// Notes:
//  - intersection_exact is currently unsupported
//  - intersection_safe is currently unsupported
//  - for more elaborate ray-tracing, a future ray_cast function will exist (which also returns the intersection normal)

// Implementation guidelines:
// if object has boundary_of(obj) defined
//      explicit intersection_parameter(line, obj_boundary), which gives intersection_parameter(line, obj)
//      if obj is infinite and contains(obj, pos) is not cheap, intersection_parameter(line, obj) can be implemented additionally
// else
//      explicit intersection_parameter(line, obj)
// this gives intersection, intersects, closest_intersection_parameter, and closest_intersection for line and ray
//
// if closest ray intersection is faster than computing all line intersections
//      explicit closest_intersection_parameter(ray, obj), same for obj_boundary
// this is then also used by closest_intersection and intersects
//
// explicit intersects(obj, aabb), which gives intersects(aabb, obj)
//
// for convex compound objects (like cylinder or pyramids), decompose the object into primitive shapes and pass them to a helper function:
// - call merge_hits(line, objPart1, objPart2, ...) in the implementation of intersection_parameter(line, obj_boundary)
// - call intersects_any(lineRayObj, objPart1, objPart2, ...) in the implementation of intersects(lineRayObj, obj<TraitsT>), which can shortcut and be faster than the default


namespace tg
{
// ====================================== Result Structs ======================================

/// ordered list of ray intersection hits
/// behaves like a container with
///   .size()
///   operator[]
///   range-based-for
template <int MaxHits, class HitT>
struct hits
{
    static constexpr bool is_hits = true; // tag
    static constexpr int max_hits = MaxHits;
    using hit_t = HitT;

    template <class OtherT>
    using as_hits = hits<MaxHits, OtherT>;

    [[nodiscard]] int size() const { return _size; }
    [[nodiscard]] bool any() const { return _size > 0; }

    HitT const& operator[](int idx) const
    {
        TG_ASSERT(0 <= idx && idx < _size);
        return _hit[idx];
    }
    [[nodiscard]] HitT const& first() const
    {
        TG_ASSERT(_size > 0);
        return _hit[0];
    }
    [[nodiscard]] HitT const& last() const
    {
        TG_ASSERT(_size > 0);
        return _hit[_size - 1];
    }

    [[nodiscard]] HitT const* begin() const { return _hit; }
    [[nodiscard]] HitT const* end() const { return _hit + _size; }

    hits() = default;
    hits(HitT* hits, int size) : _size(size)
    {
        for (auto i = 0; i < size; ++i)
            _hit[i] = hits[i];
    }
    template <typename... HitTs>
    hits(HitTs... hits) : _size(sizeof...(HitTs)), _hit{hits...}
    {
    }

private:
    int _size = 0;
    HitT _hit[MaxHits] = {};
};

/// describes a continuous interval on a line or ray between start and end
template <class ScalarT>
struct hit_interval
{
    static constexpr bool is_hit_interval = true; // tag

    ScalarT start;
    ScalarT end;

    [[nodiscard]] constexpr bool is_unbounded() const { return end == tg::max<ScalarT>() || start == tg::min<ScalarT>(); }

    cc::optional<hit_interval> clamped(ScalarT s, ScalarT e) const
    {
        CC_ASSERT(start <= end);
        CC_ASSERT(s <= e);
        auto new_s = max(start, s);
        auto new_e = min(end, e);
        if (new_e < new_s)
            return cc::nullopt;
        return hit_interval{new_s, new_e};
    }
};


// ====================================== Helper functions ======================================

namespace detail
{
// intersects the given line with all given objects and returns the concatenated intersections. A maximal number of 2 intersections is assumed.
template <int D, class ScalarT, class... Objs>
[[nodiscard]] constexpr hits<2, ScalarT> merge_hits(line<D, ScalarT> const& line, Objs const&... objs)
{
    ScalarT hits[2] = {};
    hits[0] = tg::max<ScalarT>();
    hits[1] = tg::min<ScalarT>();
    auto numHits = 0;

    const auto find_hits = [&](const auto& obj)
    {
        const auto inters = intersection_parameter(line, obj);
        for (const auto& inter : inters)
        {
            hits[0] = tg::min(hits[0], inter);
            hits[1] = tg::max(hits[1], inter);
            numHits++;
        }
    };
    (find_hits(objs), ...);

    TG_ASSERT(numHits <= 2);
    return {hits, numHits};
}
template <int D, class ScalarT, class ObjT, u64 N, size_t... I>
[[nodiscard]] constexpr hits<2, ScalarT> merge_hits_array(line<D, ScalarT> const& line, array<ObjT, N> objs, std::index_sequence<I...>)
{
    return merge_hits(line, objs[I]...);
}

// returns true, iff the given line or ray object intersects any of the given other objects (with short-circuiting after the first intersection)
template <class Obj, class... Objs>
[[nodiscard]] constexpr bool intersects_any(Obj const& obj, Objs const&... objs)
{
    return (intersects(obj, objs) || ...);
}
template <class Obj, class ObjT, u64 N, size_t... I>
[[nodiscard]] constexpr bool intersects_any_array(Obj const& obj, array<ObjT, N> objs, std::index_sequence<I...>)
{
    return intersects_any(obj, objs[I]...);
}

// Solves the quadratic equation ax^2 + bx + c = 0
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> solve_quadratic(ScalarT const& a, ScalarT const& b, ScalarT const& c)
{
    const auto discriminant = b * b - ScalarT(4) * a * c;
    if (discriminant < ScalarT(0))
        return {}; // No solution

    const auto sqrtD = sqrt(discriminant);
    const auto t1 = (-b - sqrtD) / (ScalarT(2) * a);
    const auto t2 = (-b + sqrtD) / (ScalarT(2) * a);

    const auto [tMin, tMax] = minmax(t1, t2);
    return {tMin, tMax};
}

// segment - convex object
// constexpr cc::optional<segment<3, ScalarT>>
template <class ScalarT, class B>
[[nodiscard]] auto intersection_segment_object_impl(segment<3, ScalarT> const& s, B const& o)
    -> enable_if<!std::is_same_v<typename object_traits<B>::tag_t, boundary_tag>, cc::optional<segment<3, ScalarT>>>
{
    bool con_pos0 = contains(o, s.pos0);
    bool con_pos1 = contains(o, s.pos1);

    // case 1: Both seg. points are inside the convex object
    if (con_pos0 && con_pos1)
        return segment<3, ScalarT>{s.pos0, s.pos1};

    line<3, ScalarT> segment_line = line<3, ScalarT>(s.pos0, normalize(s.pos1 - s.pos0));
    auto insec = intersection_parameter(segment_line, o);

    // no intersection exists
    if (!insec.has_value())
        return {};

    // case 2: One seg. point inside the convex object and one outside -> intersection with boundary must exist
    if (con_pos0)
    {
        ScalarT param = (dot(s.pos1 - s.pos0, segment_line[insec.value().start]) > ScalarT(0)) ? insec.value().start : insec.value().end;
        return segment<3, ScalarT>{s.pos0, segment_line.pos + segment_line.dir * param};
    }
    else if (con_pos1)
    {
        ScalarT param = (dot(s.pos0 - s.pos1, segment_line[insec.value().start]) > 0) ? insec.value().start : insec.value().end;
        return segment<3, ScalarT>{segment_line.pos + segment_line.dir * param, s.pos1};
    }

    // case 3: both points of segment outside of the convex object
    if (ScalarT(0) < insec.value().start && insec.value().start < length(s) && 0 < insec.value().end && insec.value().end < length(s))
        return segment<3, ScalarT>{segment_line.pos + segment_line.dir * insec.value().start, segment_line.pos + segment_line.dir * insec.value().end};

    return {};
}

// segment - boundary object
template <class ScalarT, class B>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection_segment_boundary_impl(segment<3, ScalarT> const& s, B const& b)
{
    // line extension of segment
    line<3, ScalarT> const l = line<3, ScalarT>(s.pos0, normalize(s.pos1 - s.pos0));
    // intersection of line with boundary object
    auto const params = intersection_parameter(l, b);

    if (!params.any())
        return {};

    auto const dist = distance(s.pos0, s.pos1);
    auto n_hits = 0;
    pos<3, ScalarT> ps[2];
    // check if line intersections are within the segment range
    for (auto i = 0; i < params.size(); ++i)
    {
        auto const p = params[i];
        if (ScalarT(0) <= p && p <= dist)
        {
            ps[n_hits++] = l[p];
        }
    }
    return hits<2, pos<3, ScalarT>>(ps, n_hits);
}

template <class A, class B>
using try_closest_intersection_parameter = decltype(closest_intersection_parameter(std::declval<A const&>(), std::declval<B const&>()));

// circular permutation to the vertices of triangle ta such that ta.pos0 is the only vertex that lies on positive halfspace induced by tb
template <class ScalarT>
void rotate_devillers_triangle(triangle<3, ScalarT>& ta, triangle<3, ScalarT>& tb, comp<3, ScalarT>& determinants, comp<3, ScalarT>& determinants_t2)
{
    // implementation of triangle permutation according to: https://hal.inria.fr/inria-00072100/document

    ScalarT d01 = determinants[0] * determinants[1];
    ScalarT d02 = determinants[0] * determinants[2];

    if (d01 > ScalarT(0)) // vertices 0 and 1 on one side
    {
        ta = {ta.pos2, ta.pos0, ta.pos1};
        determinants = {determinants[2], determinants[0], determinants[1]};
    }
    else if (d02 > ScalarT(0)) // vertices 0 and 2 on one side
    {
        ta = {ta.pos1, ta.pos2, ta.pos0};
        determinants = {determinants[1], determinants[2], determinants[0]};
    }
    else if (determinants[0] == ScalarT(0))
    {
        if (determinants[1] * determinants[2] < ScalarT(0) || determinants[1] == ScalarT(0)) // vertices 1 and 2 on different sides of plane and vertex 0 on plane
        {
            ta = {ta.pos2, ta.pos0, ta.pos1};
            determinants = {determinants[2], determinants[0], determinants[1]};
        }
        else if (determinants[2] == ScalarT(0)) // vertices 0 and 2 on the plane
        {
            ta = {ta.pos1, ta.pos2, ta.pos0};
            determinants = {determinants[1], determinants[2], determinants[0]};
        }
    }

    // swap operation to map triangle1.pos0 to positive halfspace induced by plane of triangle2
    if (determinants[0] < ScalarT(0))
    {
        tb = {tb.pos0, tb.pos2, tb.pos1};
        determinants = {-determinants[0], -determinants[1], -determinants[2]};
        determinants_t2 = {determinants_t2[0], determinants_t2[2], determinants_t2[1]};
    }

    else if (determinants[0] == ScalarT(0) && (determinants[1] * determinants[2] > ScalarT(0)))
    {
        if (determinants[1] > 0) // swap
        {
            tb = {tb.pos0, tb.pos2, tb.pos1};
            determinants = {-determinants[0], -determinants[1], -determinants[2]};
            determinants_t2 = {determinants_t2[0], determinants_t2[2], determinants_t2[1]};
        }
    }
}
}


// ====================================== Default Implementations ======================================
// TODO: intersection_parameter from intersection_parameters

// returns whether two objects intersect
// if intersection is available and applicable, use that
template <class A, class B>
[[nodiscard]] constexpr auto intersects(A const& a, B const& b)
    -> std::enable_if_t<!can_apply<detail::try_closest_intersection_parameter, A, B>, decltype(intersection(a, b).has_value())>
{
    return intersection(a, b).has_value();
}

// if closest intersection parameter is available and applicable, use that
template <class A, class B>
[[nodiscard]] constexpr auto intersects(A const& a, B const& b) -> decltype(closest_intersection_parameter(a, b).has_value())
{
    return closest_intersection_parameter(a, b).has_value();
}

// if A is a _boundary, check if B is completely contained within (then false), otherwise same as intersects solid_of(A)
template <class A, class B>
[[nodiscard]] constexpr auto intersects(A const& a, B const& b) -> enable_if<std::is_same_v<typename object_traits<A>::tag_t, boundary_tag>, bool>
{
    using ScalarT = typename A::scalar_t;
    auto const solidA = solid_of(a);
    if (contains(solidA, b, ScalarT(-16) * epsilon<ScalarT>))
        return false;

    return intersects(solidA, b);
}

// parameters for intersects with aabb can switch order
template <int D, class ScalarT, class Obj>
[[nodiscard]] constexpr bool intersects(aabb<D, ScalarT> const& b, Obj const& obj)
{
    return intersects(obj, b);
}

// if a value-typed intersection parameter is available and applicable, use that
template <class A, class B>
[[nodiscard]] constexpr auto intersection(A const& a, B const& b) -> decltype(a[intersection_parameter(a, b)])
{
    return a[intersection_parameter(a, b)];
}

// if an optional intersection parameter is available and applicable, use that
template <class A, class B>
[[nodiscard]] constexpr auto intersection(A const& a, B const& b) -> cc::optional<decltype(a[intersection_parameter(a, b).value()])>
{
    if (auto t = intersection_parameter(a, b); t.has_value())
        return a[t.value()];
    return {};
}

// if a value-typed closest intersection parameter is available and applicable, use that
template <class A, class B>
[[nodiscard]] constexpr auto closest_intersection(A const& a, B const& b) -> decltype(a[closest_intersection_parameter(a, b)])
{
    return a[closest_intersection_parameter(a, b)];
}

// if an optional closest intersection parameter is available and applicable, use that
template <class A, class B>
[[nodiscard]] constexpr auto closest_intersection(A const& a, B const& b) -> cc::optional<decltype(a[closest_intersection_parameter(a, b).value()])>
{
    if (auto t = closest_intersection_parameter(a, b); t.has_value())
        return a[t.value()];
    return {};
}

// if hits intersection parameter is available, use that
template <class A, class B>
[[nodiscard]] constexpr auto intersection(A const& a, B const& b) -> typename decltype(intersection_parameter(a, b))::template as_hits<typename A::pos_t>
{
    auto ts = intersection_parameter(a, b);
    typename A::pos_t hits[ts.max_hits] = {};
    for (auto i = 0; i < ts.size(); ++i)
        hits[i] = a[ts[i]];
    return {hits, ts.size()};
}

//// if hits intersection parameter is available, use that
// template <class A, class B>
//[[nodiscard]] constexpr auto intersection(A const& a, B const& b) -> typename decltype(intersection_parameter(b, a))::template as_hits<typename A::pos_t>
//{
//    // TODO: Check if both way intersection is not destroyed. ambiguous overload?
//    return intersection(b, a);
//}

// if an optional hit_interval intersection parameter is available, use that
template <class A, class B, std::enable_if_t<std::decay_t<decltype(intersection_parameter(std::declval<A>(), std::declval<B>()).value())>::is_hit_interval, int> = 0>
[[nodiscard]] constexpr auto intersection(A const& a, B const& b)
    -> cc::optional<segment<object_traits<A>::domain_dimension, typename object_traits<A>::scalar_t>>
{
    using seg_t = segment<object_traits<A>::domain_dimension, typename object_traits<A>::scalar_t>;
    auto ts = intersection_parameter(a, b);
    if (ts.has_value())
        return seg_t{a[ts.value().start], a[ts.value().end]};
    return {};
}

// if hits intersection parameter is available, use that
template <class A, class B>
[[nodiscard]] constexpr auto closest_intersection_parameter(A const& a, B const& b) -> cc::optional<typename decltype(intersection_parameter(a, b))::hit_t>
{
    const auto hits = intersection_parameter(a, b);
    if (hits.any())
        return hits.first();
    return {};
}

// if an optional hit_interval intersection parameter is available, use that
template <class A, class B>
[[nodiscard]] constexpr auto closest_intersection_parameter(A const& a, B const& b) -> cc::optional<decltype(intersection_parameter(a, b).value().start)>
{
    const auto hits = intersection_parameter(a, b);
    if (hits.has_value())
        return hits.value().start;
    return {};
}

// if boundary_of a solid object returns hits, use this to construct the hit_interval result of the solid intersection
template <int D, class ScalarT, class Obj>
[[nodiscard]] constexpr auto intersection_parameter(line<D, ScalarT> const& l, Obj const& obj)
    -> enable_if<!std::is_same_v<Obj, decltype(boundary_of(obj))>, cc::optional<hit_interval<ScalarT>>>
{
    const hits<2, ScalarT> inter = intersection_parameter(l, boundary_of(obj));

    if (inter.size() == 2)
        return {{inter[0], inter[1]}};

    if constexpr (object_traits<Obj>::is_finite)
    {
        TG_ASSERT(inter.size() == 0); // a line intersects a finite solid object either twice or not at all
        return {};
    }
    else
    {
        if (inter.size() == 0)
        {
            if (contains(obj, l.pos))
                return {{tg::min<ScalarT>(), tg::max<ScalarT>()}};
            return {};
        }

        TG_ASSERT(inter.size() == 1);                     // no other number remains
        if (contains(obj, l[inter.first() + ScalarT(1)])) // the line continues into the object
            return {{inter.first(), tg::max<ScalarT>()}};
        return {{tg::min<ScalarT>(), inter.first()}};
    }
}

// intersection between point and obj is same as contains
template <int D, class ScalarT, class Obj, class = void_t<decltype(contains(std::declval<pos<D, ScalarT>>(), std::declval<Obj>()))>>
constexpr cc::optional<pos<D, ScalarT>> intersection(pos<D, ScalarT> const& p, Obj const& obj)
{
    if (contains(obj, p))
        return p;
    return {};
}

// intersection between point and obj is same as contains
template <int D, class ScalarT, class Obj, class = void_t<decltype(contains(std::declval<pos<D, ScalarT>>(), std::declval<Obj>()))>>
constexpr cc::optional<pos<D, ScalarT>> intersection(Obj const& obj, pos<D, ScalarT> const& p)
{
    if (contains(obj, p))
        return p;
    return {};
}

// intersects between point and obj is same as contains
template <int D, class ScalarT, class Obj>
constexpr bool intersects(pos<D, ScalarT> const& p, Obj const& obj)
{
    return contains(obj, p);
}


// ====================================== Ray Intersections from Line Intersections ======================================

template <int D, class ScalarT, class Obj>
[[nodiscard]] constexpr auto intersection_parameter(ray<D, ScalarT> const& ray, Obj const& obj)
    -> decltype(intersection_parameter(inf_of(ray), obj).is_hits, intersection_parameter(inf_of(ray), obj))
{
    const auto inter = intersection_parameter(inf_of(ray), obj);
    constexpr auto maxHits = inter.max_hits;
    if (!inter.any() || inter.last() < ScalarT(0))
        return {};

    if constexpr (maxHits == 2)
    {
        if (inter.first() < ScalarT(0))
            return {inter[1]};
    }
    else
        static_assert(maxHits == 1, "Only up to two intersections implemented");

    return inter;
}
template <int D, class ScalarT, class Obj>
[[nodiscard]] constexpr auto intersection_parameter(ray<D, ScalarT> const& ray, Obj const& obj)
    -> decltype(intersection_parameter(inf_of(ray), obj).value().is_hit_interval, intersection_parameter(inf_of(ray), obj))
{
    const auto inter = intersection_parameter(inf_of(ray), obj);

    if (!inter.has_value())
        return inter;

    auto interval = inter.value();
    if (interval.end < ScalarT(0))
        return {};

    TG_ASSERT((interval.start <= ScalarT(0)) == contains(obj, ray.origin));

    interval.start = max(interval.start, ScalarT(0));
    return interval;
}

// ====================================== Line - Object Intersections ======================================

// line - point
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<1, ScalarT> const& l, pos<1, ScalarT> const& p)
{
    return coordinates(l, p);
}

// line - line
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<2, ScalarT> const& l0, line<2, ScalarT> const& l1)
{
    // l0.pos + l0.dir * t.x == l1.pos + l1.dir * t.y  <=>  (l0.dir | -l1.dir) * (t.x | t.y)^T == l1.pos - l0.pos
    auto M = mat<2, 2, ScalarT>::from_cols(l0.dir, -l1.dir);
    auto t = inverse(M) * (l1.pos - l0.pos);
    if (!tg::is_finite(t.x))
        return {};
    return t.x;
}

// line - ray
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<2, ScalarT> const& l, ray<2, ScalarT> const& r)
{
    auto M = mat<2, 2, ScalarT>::from_cols(l.dir, -r.dir);
    auto t = inverse(M) * (r.origin - l.pos);
    if (t.y < ScalarT(0) || !tg::is_finite(t.x))
        return {};
    return t.x;
}

// line - segment
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<2, ScalarT> const& l, segment<2, ScalarT> const& s)
{
    auto M = mat<2, 2, ScalarT>::from_cols(l.dir, s.pos0 - s.pos1);
    auto t = inverse(M) * (s.pos0 - l.pos);
    if (t.y < ScalarT(0) || t.y > ScalarT(1) || !tg::is_finite(t.x))
        return {};
    return t.x;
}

// line - plane
template <int D, class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<D, ScalarT> const& l, plane<D, ScalarT> const& p)
{
    const auto dotND = dot(p.normal, l.dir);
    if (dotND == ScalarT(0)) // if plane normal and line direction are orthogonal, there is no intersection
        return {};

    // <l.pos + t * l.dir, p.normal> = p.dis  <=>  t = (p.dis - <l.pos, p.normal>) / <l.dir, p.normal>
    return (p.dis - dot(p.normal, l.pos)) / dotND;
}

// line - halfspace
template <int D, class ScalarT>
[[nodiscard]] constexpr cc::optional<hit_interval<ScalarT>> intersection_parameter(line<D, ScalarT> const& l, halfspace<D, ScalarT> const& h)
{
    const auto dotND = dot(h.normal, l.dir);
    const auto dist = signed_distance(l.pos, h);

    if (dotND == ScalarT(0)) // if halfspace normal and line direction are orthogonal, there is no intersection
    {
        if (dist <= ScalarT(0))
            return {{tg::min<ScalarT>(), tg::max<ScalarT>()}}; // completely contained
        return {};                                             // completely outside
    }

    const auto t = -dist / dotND;
    if (dotND < ScalarT(0))
        return {{t, tg::max<ScalarT>()}}; // line goes into the halfspace
    return {{tg::min<ScalarT>(), t}};     // line goes out of the halfspace
}
template <int D, class ScalarT>
[[nodiscard]] constexpr cc::optional<ScalarT> closest_intersection_parameter(ray<D, ScalarT> const& r, halfspace<D, ScalarT> const& h)
{
    // check if ray origin is already contained in the halfspace
    const auto dist = signed_distance(r.origin, h);
    if (dist <= ScalarT(0))
        return ScalarT(0);

    // if ray points away from the halfspace there is no intersection
    const auto dotND = dot(h.normal, r.dir);
    if (dotND >= ScalarT(0))
        return {};

    return -dist / dotND;
}

// line - aabb
template <int D, class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<D, ScalarT> const& l, aabb_boundary<D, ScalarT> const& b)
{
    // based on ideas from https://gamedev.stackexchange.com/q/18436
    auto tFirst = tg::min<ScalarT>();
    auto tSecond = tg::max<ScalarT>();
    for (auto i = 0; i < D; ++i)
    {
        if (abs(l.dir[i]) > ScalarT(100) * tg::epsilon<ScalarT>)
        {
            const auto tMin = (b.min[i] - l.pos[i]) / l.dir[i];
            const auto tMax = (b.max[i] - l.pos[i]) / l.dir[i];
            tFirst = max(tFirst, min(tMin, tMax));
            tSecond = min(tSecond, max(tMin, tMax));
        }
        else if (l.pos[i] < b.min[i] || l.pos[i] > b.max[i])
            return {}; // ray parallel to this axis and outside of the aabb
    }

    // no intersection if line misses the aabb
    if (tFirst > tSecond)
        return {};

    return {tFirst, tSecond};
}

// line - box
template <int D, class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<D, ScalarT> const& l, box_boundary<D, ScalarT> const& b)
{
    const auto bMin = b[comp<D, ScalarT>(-1)] - l.pos;
    const auto bMax = b[comp<D, ScalarT>(1)] - l.pos;
    auto tFirst = tg::min<ScalarT>();
    auto tSecond = tg::max<ScalarT>();
    for (auto i = 0; i < D; ++i)
    {
        const auto rDir = dot(l.dir, b.half_extents[i]);
        if (abs(rDir) > ScalarT(100) * tg::epsilon<ScalarT>)
        {
            const auto tMin = dot(bMin, b.half_extents[i]) / rDir;
            const auto tMax = dot(bMax, b.half_extents[i]) / rDir;
            tFirst = max(tFirst, min(tMin, tMax));
            tSecond = min(tSecond, max(tMin, tMax));
        }
        else if (dot(bMin, b.half_extents[i]) > ScalarT(0) || dot(bMax, b.half_extents[i]) < ScalarT(0))
            return {}; // ray parallel to this half_extents axis and outside of the box
    }

    // no intersection if line misses the box
    if (tFirst > tSecond)
        return {};

    return {tFirst, tSecond};
}
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<3, ScalarT> const& l, box<2, ScalarT, 3> const& b)
{
    const auto t = intersection_parameter(l, plane<3, ScalarT>(normal_of(b), b.center));
    if (!t.any()) // line parallel to box plane
        return {};

    const auto p = l[t.first()] - b.center;
    if (abs(dot(b.half_extents[0], p)) > length_sqr(b.half_extents[0]) || abs(dot(b.half_extents[1], p)) > length_sqr(b.half_extents[1]))
        return {};

    return t;
}

// line - disk
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<3, ScalarT> const& l, sphere<2, ScalarT, 3> const& d)
{
    const auto t = intersection_parameter(l, plane<3, ScalarT>(d.normal, d.center));
    if (!t.any()) // line parallel to disk plane
        return {};

    const auto p = l[t.first()];
    if (distance_sqr(p, d.center) > d.radius * d.radius)
        return {};

    return t;
}

// line - sphere_boundary
template <int D, class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<D, ScalarT> const& l, sphere_boundary<D, ScalarT> const& s)
{
    auto t = dot(s.center - l.pos, l.dir);

    auto d_sqr = distance_sqr(l[t], s.center);
    auto r_sqr = s.radius * s.radius;
    if (d_sqr > r_sqr)
        return {};

    auto dt = sqrt(r_sqr - d_sqr);
    return {t - dt, t + dt};
}

// line - hemisphere
template <int D, class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<D, ScalarT> const& l, hemisphere_boundary<D, ScalarT> const& h)
{
    return detail::merge_hits(l, caps_of(h), boundary_no_caps_of(h));
}
template <int D, class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<D, ScalarT> const& l, hemisphere_boundary_no_caps<D, ScalarT> const& h)
{
    ScalarT hits[2] = {};
    auto numHits = 0;
    const auto sphereHits = intersection_parameter(l, sphere_boundary<D, ScalarT>(h.center, h.radius));
    const auto halfSpace = halfspace<D, ScalarT>(-h.normal, h.center); // the intersection of this halfspace and the sphere is exactly the hemisphere
    for (const auto& hit : sphereHits)
        if (contains(halfSpace, l[hit]))
            hits[numHits++] = hit;

    return {hits, numHits};
}
template <class Obj, int D, class ScalarT, class TraitsT>
[[nodiscard]] constexpr bool intersects(Obj const& lr, hemisphere<D, ScalarT, TraitsT> const& h)
{
    static_assert(object_traits<Obj>::is_infinite, "For finite objects, complete containment within boundary has to be considered as well");
    if constexpr (std::is_same_v<TraitsT, boundary_no_caps_tag>)
        return intersection_parameter(lr, h).any();
    else
        return detail::intersects_any(lr, caps_of(h), boundary_no_caps_of(h));
}

// line - capsule
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<3, ScalarT> const& l, capsule_boundary<3, ScalarT> const& c)
{
    using caps_t = hemisphere_boundary_no_caps<3, ScalarT>;
    const auto n = direction(c);
    return detail::merge_hits(l, caps_t(c.axis.pos0, c.radius, -n), caps_t(c.axis.pos1, c.radius, n), cylinder_boundary_no_caps<3, ScalarT>(c.axis, c.radius));
}
template <class Obj, class ScalarT, class TraitsT>
[[nodiscard]] constexpr bool intersects(Obj const& lr, capsule<3, ScalarT, TraitsT> const& c)
{
    static_assert(object_traits<Obj>::is_infinite, "For finite objects, complete containment within boundary has to be considered as well");
    using caps_t = sphere_boundary<3, ScalarT>; // spheres are faster than hemispheres and equivalent for the yes/no decision
    return detail::intersects_any(lr, caps_t(c.axis.pos0, c.radius), caps_t(c.axis.pos1, c.radius), cylinder_boundary_no_caps<3, ScalarT>(c.axis, c.radius));
}

// line - cylinder
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<3, ScalarT> const& l, cylinder_boundary<3, ScalarT> const& c)
{
    const auto caps = caps_of(c);
    return detail::merge_hits(l, caps[0], caps[1], boundary_no_caps_of(c));
}
template <class Obj, class ScalarT, class TraitsT>
[[nodiscard]] constexpr bool intersects(Obj const& lr, cylinder<3, ScalarT, TraitsT> const& c)
{
    static_assert(object_traits<Obj>::is_infinite, "For finite objects, complete containment within boundary has to be considered as well");
    if constexpr (std::is_same_v<TraitsT, boundary_no_caps_tag>)
        return intersection_parameter(lr, c).any();
    else
    {
        const auto caps = caps_of(c);
        return detail::intersects_any(lr, caps[0], caps[1], boundary_no_caps_of(c));
    }
}
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<3, ScalarT> const& l, cylinder_boundary_no_caps<3, ScalarT> const& c)
{
    auto const infInter = intersection_parameter(l, inf_of(c));
    if (!infInter.any())
        return infInter;

    auto const d = c.axis.pos1 - c.axis.pos0;
    auto const lambda0 = dot(l[infInter[0]] - c.axis.pos0, d);
    auto const lambda1 = dot(l[infInter[1]] - c.axis.pos0, d);

    ScalarT hits[2] = {};
    auto numHits = 0;
    auto const dDotD = dot(d, d);
    if (ScalarT(0) <= lambda0 && lambda0 <= dDotD)
        hits[numHits++] = infInter[0];
    if (ScalarT(0) <= lambda1 && lambda1 <= dDotD)
        hits[numHits++] = infInter[1];

    return {hits, numHits};
}

// line - inf_cylinder
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<3, ScalarT> const& l, inf_cylinder_boundary<3, ScalarT> const& c)
{
    auto const cosA = dot(c.axis.dir, l.dir);
    auto const sinA_sqr = 1 - cosA * cosA;

    if (sinA_sqr <= 0)
        return {}; // line and cylinder are parallel

    // compute closest points of line and cylinder axis
    auto const origDiff = l.pos - c.axis.pos;
    auto const fLine = dot(l.dir, origDiff);
    auto const fAxis = dot(c.axis.dir, origDiff);
    auto const tLine = (cosA * fAxis - fLine) / sinA_sqr;
    auto const tAxis = (fAxis - cosA * fLine) / sinA_sqr;

    auto const line_axis_dist_sqr = distance_sqr(l[tLine], c.axis[tAxis]);
    auto const cyl_radius_sqr = c.radius * c.radius;

    if (cyl_radius_sqr < line_axis_dist_sqr)
        return {}; // line misses the cylinder

    // radius in 2D slice = sqrt(cyl_radius_sqr - line_axis_dist_sqr)
    // infinite tube intersection
    auto const s = sqrt((cyl_radius_sqr - line_axis_dist_sqr) / sinA_sqr);
    return {tLine - s, tLine + s};
}
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<2, ScalarT> const& l, inf_cylinder_boundary<2, ScalarT> const& c)
{
    const auto n = perpendicular(c.axis.dir);
    const auto d = dot(l.dir, n);
    if (d == ScalarT(0)) // line parallel to inf_cylinder
        return {};

    const auto dist = dot(c.axis.pos - l.pos, n);
    const auto [tMin, tMax] = minmax((dist - c.radius) / d, (dist + c.radius) / d);
    return {tMin, tMax};
}

// line - inf_cone
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<2, ScalarT> const& l, inf_cone_boundary<2, ScalarT> const& c)
{
    auto ray1 = ray<2, ScalarT>(c.apex, rotate(c.opening_dir, c.opening_angle / ScalarT(2)));
    auto ray2 = ray<2, ScalarT>(c.apex, rotate(c.opening_dir, -c.opening_angle / ScalarT(2)));
    return detail::merge_hits(l, ray1, ray2);
}
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<3, ScalarT> const& l, inf_cone_boundary<3, ScalarT> const& ic)
{
    // see https://lousodrome.net/blog/light/2017/01/03/intersection-of-a-ray-and-a-cone/
    auto const dv = dot(l.dir, ic.opening_dir);
    auto const cos2 = pow2(cos(ic.opening_angle * ScalarT(0.5)));
    auto const co = l.pos - ic.apex;
    auto const cov = dot(co, ic.opening_dir);
    auto const a = dv * dv - cos2;
    auto const b = ScalarT(2) * (dv * cov - dot(l.dir, co) * cos2);
    auto const c = cov * cov - dot(co, co) * cos2;
    auto const inter = detail::solve_quadratic(a, b, c);
    if (!inter.any())
        return inter;

    // exclude intersections with mirrored cone
    ScalarT hits[2] = {};
    auto numHits = 0;
    TG_ASSERT(ic.opening_angle <= tg::angle::from_degree(ScalarT(180))
              && "Only convex objects are supported, but an inf_cone with openinge angle > 180 degree is not convex.");
    // if it is not used for solid cones, this works:
    // auto const coneDir = ic.opening_angle > 180_deg ? -ic.opening_dir : ic.opening_dir;
    // if (dot(l[inter[0]] - ic.apex, coneDir) >= ScalarT(0)) ...
    if (dot(l[inter[0]] - ic.apex, ic.opening_dir) >= ScalarT(0))
        hits[numHits++] = inter[0];
    if (dot(l[inter[1]] - ic.apex, ic.opening_dir) >= ScalarT(0))
        hits[numHits++] = inter[1];

    return {hits, numHits};
}

// line - cone
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<3, ScalarT> const& l, cone_boundary_no_caps<3, ScalarT> const& cone)
{
    auto const apex = apex_of(cone);
    auto const openingDir = -normal_of(cone.base);
    auto const borderPos = any_point(boundary_of(cone.base));
    auto const openingAngleHalf = angle_between(openingDir, normalize(borderPos - apex));

    // see https://lousodrome.net/blog/light/2017/01/03/intersection-of-a-ray-and-a-cone/
    auto const dv = dot(l.dir, openingDir);
    auto const cos2 = pow2(cos(openingAngleHalf));
    auto const co = l.pos - apex;
    auto const cov = dot(co, openingDir);
    auto const a = dv * dv - cos2;
    auto const b = ScalarT(2) * (dv * cov - dot(l.dir, co) * cos2);
    auto const c = cov * cov - dot(co, co) * cos2;
    auto const inter = detail::solve_quadratic(a, b, c);
    if (!inter.any())
        return inter;

    // exclude intersections with mirrored cone
    ScalarT hits[2] = {};
    auto numHits = 0;
    auto const h0 = dot(l[inter[0]] - apex, openingDir);
    auto const h1 = dot(l[inter[1]] - apex, openingDir);
    if (ScalarT(0) <= h0 && h0 <= cone.height)
        hits[numHits++] = inter[0];
    if (ScalarT(0) <= h1 && h1 <= cone.height)
        hits[numHits++] = inter[1];

    return {hits, numHits};
}

// line - pyramid
template <class BaseT, typename = std::enable_if_t<!std::is_same_v<BaseT, sphere<2, typename BaseT::scalar_t, 3>>>>
[[nodiscard]] constexpr hits<2, typename BaseT::scalar_t> intersection_parameter(line<3, typename BaseT::scalar_t> const& l,
                                                                                 pyramid_boundary_no_caps<BaseT> const& py)
{
    auto const faces = faces_of(py);
    return detail::merge_hits_array(l, faces, std::make_index_sequence<faces.size()>{});
}
template <class BaseT>
[[nodiscard]] constexpr hits<2, typename BaseT::scalar_t> intersection_parameter(line<3, typename BaseT::scalar_t> const& l, pyramid_boundary<BaseT> const& py)
{
    return detail::merge_hits(l, py.base, boundary_no_caps_of(py));
}

// line - triangle2
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<hit_interval<ScalarT>> intersection_parameter(line<2, ScalarT> const& l, triangle<2, ScalarT> const& t)
{
    ScalarT closestIntersection = tg::max<ScalarT>();
    ScalarT furtherIntersection = tg::min<ScalarT>();
    auto numIntersections = 0;
    for (const auto& edge : edges_of(t))
    {
        const auto inter = intersection_parameter(l, edge);
        if (inter.any())
        {
            numIntersections++;
            closestIntersection = min(closestIntersection, inter.first());
            furtherIntersection = max(furtherIntersection, inter.first());
        }
    }

    if (numIntersections == 0)
        return {}; // No intersection

    TG_ASSERT(numIntersections == 2);
    return {{closestIntersection, furtherIntersection}};
}

// line - triangle3
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<3, ScalarT> const& l,
                                                                triangle<3, ScalarT> const& t,
                                                                dont_deduce<ScalarT> eps = 100 * tg::epsilon<ScalarT>)
{
    auto e1 = t.pos1 - t.pos0;
    auto e2 = t.pos2 - t.pos0;

    auto pvec = tg::cross(l.dir, e2);
    auto det = dot(pvec, e1);

    if (det < ScalarT(0))
    {
        std::swap(e1, e2);
        pvec = tg::cross(l.dir, e2);
        det = -det;
    }

    if (det < eps)
        return {};

    auto tvec = l.pos - t.pos0;
    auto u = dot(tvec, pvec);
    if (u < ScalarT(0) || u > det)
        return {};

    auto qvec = cross(tvec, e1);
    auto v = dot(l.dir, qvec);
    if (v < ScalarT(0) || v + u > det)
        return {};

    return (ScalarT(1) / det) * dot(e2, qvec);
}

// line - ellipse
template <class ScalarT>
[[nodiscard]] constexpr hits<1, ScalarT> intersection_parameter(line<3, ScalarT> const& l, ellipse<2, ScalarT, 3> const& e)
{
    const auto t = intersection_parameter(l, plane<3, ScalarT>(normal_of(e), e.center));
    if (!t.any()) // line parallel to ellipse plane
        return {};

    // simplified contains(e, p) without plane check and eps == 0
    auto pc = l[t.first()] - e.center;
    auto x = dot(pc, e.semi_axes[0]);
    auto y = dot(pc, e.semi_axes[1]);
    auto a = length_sqr(e.semi_axes[0]);
    auto b = length_sqr(e.semi_axes[1]);

    if (pow2(x / a) + pow2(y / b) <= ScalarT(1))
        return t;
    return {};
}
template <int D, class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<D, ScalarT> const& l, ellipse_boundary<D, ScalarT> const& e)
{
    // transform line to ellipse space (ellipse gets unit sphere at origin)
    auto const pc = l.pos - e.center;
    vec<D, ScalarT> p;
    vec<D, ScalarT> d; // in ellipse space, this is no longer a unit vector
    for (auto i = 0; i < D; ++i)
    {
        auto const axis2 = dot(e.semi_axes[i], e.semi_axes[i]);
        p[i] = dot(pc, e.semi_axes[i]) / axis2;
        d[i] = dot(l.dir, e.semi_axes[i]) / axis2;
    }
    // to find intersection with unit sphere, <t*d + p, t*d + p> == 1 has to be solved
    return detail::solve_quadratic(dot(d, d), ScalarT(2) * dot(d, p), dot(p, p) - ScalarT(1));
}

// line - quadric_boundary (as an isosurface, not error quadric)
template <class ScalarT>
[[nodiscard]] constexpr hits<2, ScalarT> intersection_parameter(line<3, ScalarT> const& l, quadric<3, ScalarT> const& q)
{
    const auto Ad = q.A() * l.dir;
    const auto p = l.pos;

    // Substituting x in Quadric equation x^TAx + 2b^Tx + c = 0 by ray equation x = t * dir + p yields
    // d^TAd t^2 + (2p^TAd + 2bd) t + p^TAp + 2bp + c = 0
    const auto a = dot(l.dir, Ad);
    const auto b = ScalarT(2) * (dot(p, Ad) + dot(q.b(), l.dir));
    const auto c = dot(p, q.A() * vec3(p)) + ScalarT(2) * dot(q.b(), p) + q.c;
    return detail::solve_quadratic(a, b, c);
}


// ====================================== Object - Object Intersections ======================================

// sphere boundary - sphere boundary
// returns intersection circle of sphere and sphere (normal points from a to b)
// for now does not work if spheres are identical (result would be a sphere3 again)
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<sphere_boundary<2, ScalarT, 3>> intersection(sphere_boundary<3, ScalarT> const& a, sphere_boundary<3, ScalarT> const& b)
{
    auto d2 = distance_sqr(a.center, b.center);

    // TODO: intersection sphere
    if (a.center == b.center && a.radius == b.radius)
        return {};

    auto d = sqrt(d2);

    // no intersection
    if (d > a.radius + b.radius)
        return {};

    // radius and centers of larger sphere (ls) and smaller sphere (ss)
    auto lsr = a.radius;
    auto ssr = b.radius;
    auto lsc = a.center;
    auto ssc = b.center;
    if (b.radius > a.radius)
    {
        // TODO: tg::swap?
        lsr = b.radius;
        ssr = a.radius;
        lsc = b.center;
        ssc = a.center;
    }

    if (d + ssr < lsr)
    {
        // Smaller sphere inside larger one and not touching it
        return {};
    }

    TG_INTERNAL_ASSERT(d > ScalarT(0));

    // squared radii of a and b
    auto ar2 = a.radius * a.radius;
    auto br2 = b.radius * b.radius;

    auto t = ScalarT(0.5) + (ar2 - br2) / (ScalarT(2) * d2);

    // position and radius of intersection circle
    auto ipos = a.center + t * (b.center - a.center);
    auto irad = sqrt(ar2 - t * t * d2);

    // non-empty intersection (circle)
    return sphere_boundary<2, ScalarT, 3>{ipos, irad, dir<3, ScalarT>((b.center - a.center) / d)};
}

// sphere boundary - sphere boundary
// returns intersection points of two circles in 2D
// for now does not work if circles are identical (result would be a circle2 again)
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<pair<pos<2, ScalarT>, pos<2, ScalarT>>> intersection(sphere_boundary<2, ScalarT> const& a,
                                                                                          sphere_boundary<2, ScalarT> const& b)
{
    if (a.center == b.center && a.radius == b.radius)
        return {}; // degenerate case

    auto d2 = distance_sqr(a.center, b.center);
    auto d = sqrt(d2);
    auto ar = a.radius;
    auto br = b.radius;
    if (ar + br < d) // no intersection
        return {};

    if (d < abs(ar - br)) // no intersection (one inside the other)
        return {};

    TG_INTERNAL_ASSERT(d > ScalarT(0));

    auto t = (ar * ar - br * br + d2) / (2 * d);
    auto h2 = max(ScalarT(0), ar * ar - t * t);

    auto h = sqrt(h2);
    auto h_by_d = h / d;

    auto p_between = a.center + t / d * (b.center - a.center);

    auto a_to_b = b.center - a.center;
    auto a_to_b_swap = tg::vec2(-a_to_b.y, a_to_b.x);

    // imagining circle a on the left side of circle b...
    auto p_above = p_between + h_by_d * a_to_b_swap;
    auto p_below = p_between - h_by_d * a_to_b_swap;

    return pair{p_above, p_below};
}

// sphere boundary - plane
// returns intersection circle of sphere and sphere (normal points from a to b)
// for now does not work if spheres are identical (result would be a sphere3 again)
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<sphere_boundary<2, ScalarT, 3>> intersection(sphere_boundary<3, ScalarT> const& a, plane<3, ScalarT> const& b)
{
    auto const d = dot(a.center, b.normal) - b.dis;
    if (d > a.radius)
        return {};
    if (d < -a.radius)
        return {};

    sphere_boundary<2, ScalarT, 3> r;
    r.center = a.center - b.normal * d;
    r.normal = d >= ScalarT(0) ? b.normal : -b.normal;
    r.radius = sqrt(a.radius * a.radius - d * d);
    return r;
}
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<sphere_boundary<2, ScalarT, 3>> intersection(plane<3, ScalarT> const& a, sphere_boundary<3, ScalarT> const& b)
{
    auto r = intersection(b, a);
    if (r.has_value())
    {
        auto c = r.value();
        c.normal = -c.normal; // make sure to point from a to b
        return c;
    }
    return r;
}

// circle - plane
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(sphere<2, ScalarT, 3, boundary_tag> const& a, plane<3, ScalarT> const& b)
{
    auto const l = intersection(plane_of(a), b);
    return intersection(l, sphere_boundary<3, ScalarT>(a.center, a.radius));
}
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(plane<3, ScalarT> const& a, sphere<2, ScalarT, 3, boundary_tag> const& b)
{
    return intersection(b, a);
}

// circle - sphere
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(sphere<2, ScalarT, 3, boundary_tag> const& a, sphere_boundary<3, ScalarT> const& s)
{
    auto const is = intersection(plane_of(a), s);
    if (!is.has_value())
        return {};

    auto const b = is.value();

    auto d2 = distance_sqr(a.center, b.center);
    auto d = sqrt(d2);
    auto ar = a.radius;
    auto br = b.radius;
    if (ar + br < d) // no intersection
        return {};

    if (d < abs(ar - br)) // no intersection (one inside the other)
        return {};

    TG_INTERNAL_ASSERT(d > ScalarT(0));

    auto t = (ar * ar - br * br + d2) / (2 * d);
    auto h2 = ar * ar - t * t;
    TG_INTERNAL_ASSERT(h2 >= ScalarT(0));

    auto h = sqrt(h2);
    auto h_by_d = h / d;

    auto p_between = a.center + t / d * (b.center - a.center);

    auto bitangent = cross(b.center - a.center, a.normal);

    // imagining circle a on the left side of circle b...
    auto p_above = p_between + h_by_d * bitangent;
    auto p_below = p_between - h_by_d * bitangent;

    return {p_above, p_below};
}
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(sphere_boundary<3, ScalarT> const& a, sphere<2, ScalarT, 3, boundary_tag> const& b)
{
    return intersection(b, a);
}

// plane - plane
template <class ScalarT>
[[nodiscard]] constexpr line<3, ScalarT> intersection(plane<3, ScalarT> const& a, plane<3, ScalarT> const& b)
{
    // see http://mathworld.wolfram.com/Plane-PlaneIntersection.html
    auto dir = normalize(cross(a.normal, b.normal));
    auto p = pos<3, ScalarT>::zero;

    if (abs(dir.z) > abs(dir.x)) // solve with p.z = 0
    {
        auto n0 = vec<2, ScalarT>(a.normal.x, b.normal.x);
        auto n1 = vec<2, ScalarT>(a.normal.y, b.normal.y);
        auto r = vec<2, ScalarT>(a.dis, b.dis);
        auto p2 = inverse(mat<2, 2, ScalarT>::from_cols(n0, n1)) * r;
        p.x = p2.x;
        p.y = p2.y;
    }
    else if (abs(dir.y) > abs(dir.x)) // solve with p.y = 0
    {
        auto n0 = vec<2, ScalarT>(a.normal.x, b.normal.x);
        auto n1 = vec<2, ScalarT>(a.normal.z, b.normal.z);
        auto r = vec<2, ScalarT>(a.dis, b.dis);
        auto p2 = inverse(mat<2, 2, ScalarT>::from_cols(n0, n1)) * r;
        p.x = p2.x;
        p.z = p2.y;
    }
    else // solve with p.x = 0
    {
        auto n0 = vec<2, ScalarT>(a.normal.y, b.normal.y);
        auto n1 = vec<2, ScalarT>(a.normal.z, b.normal.z);
        auto r = vec<2, ScalarT>(a.dis, b.dis);
        auto p2 = inverse(mat<2, 2, ScalarT>::from_cols(n0, n1)) * r;
        p.y = p2.x;
        p.z = p2.y;
    }

    return {p, dir};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<pair<ScalarT, ScalarT>> intersection_parameters(segment<2, ScalarT> const& seg_0, segment<2, ScalarT> const& seg_1)
{
    /// https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
    auto const denom = (seg_0.pos0.x - seg_0.pos1.x) * (seg_1.pos0.y - seg_1.pos1.y) - (seg_0.pos0.y - seg_0.pos1.y) * (seg_1.pos0.x - seg_1.pos1.x);

    // todo: might want to check == 0 with an epsilon corridor
    // todo: colinear line segments can still intersect in a point or a line segment.
    //       This might require api changes, as either a point or a line segment can be returned!
    //       Possible solution: return a segment where pos0 == pos1
    if (denom == ScalarT(0))
        return {}; // colinear

    auto const num0 = (seg_0.pos0.x - seg_1.pos0.x) * (seg_1.pos0.y - seg_1.pos1.y) - (seg_0.pos0.y - seg_1.pos0.y) * (seg_1.pos0.x - seg_1.pos1.x);
    auto const num1 = (seg_0.pos0.x - seg_0.pos1.x) * (seg_0.pos0.y - seg_1.pos0.y) - (seg_0.pos0.y - seg_0.pos1.y) * (seg_0.pos0.x - seg_1.pos0.x);
    auto const t = num0 / denom;
    auto const u = -num1 / denom;
    if (ScalarT(0) <= t && t <= ScalarT(1) && ScalarT(0) <= u && u <= ScalarT(1))
        return pair<ScalarT, ScalarT>{t, u};
    return {};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<ScalarT> intersection_parameter(segment<2, ScalarT> const& seg_0, segment<2, ScalarT> const& seg_1)
{
    auto ip = intersection_parameters(seg_0, seg_1);
    if (ip.has_value())
        return ip.value().first;
    return {};
}

template <class ScalarT>
[[nodiscard]] constexpr pair<ScalarT, ScalarT> intersection_parameters(line<2, ScalarT> const& l0, line<2, ScalarT> const& l1)
{
    auto M = mat<2, 2, ScalarT>::from_cols(l0.dir, -l1.dir);
    auto t = inverse(M) * (l1.pos - l0.pos);
    return {t.x, t.y};
}

template <int D, class ScalarT>
[[nodiscard]] constexpr cc::optional<aabb<D, ScalarT>> intersection(aabb<D, ScalarT> const& a, aabb<D, ScalarT> const& b)
{
    for (auto i = 0; i < D; ++i)
    {
        if (a.max[i] < b.min[i])
            return {};

        if (b.max[i] < a.min[i])
            return {};
    }

    // overlap!
    aabb<D, ScalarT> res;

    for (auto i = 0; i < D; ++i)
    {
        res.min[i] = max(a.min[i], b.min[i]);
        res.max[i] = min(a.max[i], b.max[i]);
    }

    return {res};
}

template <class ScalarT>
[[nodiscard]] constexpr pos<3, ScalarT> intersection(plane<3, ScalarT> const& a, plane<3, ScalarT> const& b, plane<3, ScalarT> const& c)
{
    return intersection(intersection(a, b), c).first();
}

template <int D, class ScalarT>
[[nodiscard]] constexpr cc::optional<ScalarT> intersection_parameter(segment<D, ScalarT> const& a, plane<D, ScalarT> const& p)
{
    auto denom = dot(p.normal, a.pos1 - a.pos0);
    if (denom == ScalarT(0))
        return {};

    auto t = (p.dis - dot(p.normal, a.pos0)) / denom;
    if (t < 0 || t > 1)
        return {};
    return t;
}

template <int D, class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<D, ScalarT>> intersection(segment<D, ScalarT> const& a, sphere<D, ScalarT> const& b)
{
    // early-out: both segment points inside the sphere
    if ((distance_sqr(a.pos0, b.center) < pow2(b.radius)) && (distance_sqr(a.pos1, b.center) < pow2(b.radius)))
        return segment<D, ScalarT>{a.pos0, a.pos1};

    auto const l = line<D, ScalarT>(a.pos0, normalize(a.pos1 - a.pos0));
    auto const params = intersection_parameter(l, b);

    if (!params.has_value())
        return {};

    if (params.value().is_unbounded())
        return {};

    auto const dist = distance(a.pos0, a.pos1);
    auto n_hits = 0;
    pos<D, ScalarT> ps[2];

    if (params.value().start < dist && params.value().start > ScalarT(0))
        ps[n_hits++] = l[params.value().start];

    if (params.value().end < dist && params.value().end > ScalarT(0))
        ps[n_hits++] = l[params.value().end];

    if (n_hits == 1)
        return segment<D, ScalarT>{ps[0], ps[0]};

    if (n_hits == 2)
        return segment<D, ScalarT>{ps[0], ps[1]};

    return {};
}

template <int D, class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<D, ScalarT>> intersection(sphere<D, ScalarT> const& b, segment<D, ScalarT> const& a)
{
    return intersection(b, a);
}

// sphere_boundary<3, ScalarT> -- segment3
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(sphere_boundary<3, ScalarT> const& sphere_boundary, segment<3, ScalarT> const& segment)
{
    auto const line = line3::from_points(segment.pos0, segment.pos1);
    auto const params = intersection_parameter(line, sphere_boundary);

    if (!params.any())
        return {};

    auto const dist = distance(segment.pos0, segment.pos1);
    auto n_hits = 0;
    pos<3, ScalarT> ps[2];
    for (auto i = 0; i < params.size(); ++i)
    {
        auto const t = params[i];
        if (ScalarT(0) <= t && t <= dist)
        {
            ps[n_hits++] = line[t];
        }
    }
    return hits<2, pos<3, ScalarT>>(ps, n_hits);
}

template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, sphere_boundary<3, ScalarT> const& sphere_boundary)
{
    return intersection(sphere_boundary, segment);
}


// ====================================== Checks if Object Intersects aabb ======================================

namespace detail
{
template <class ScalarT>
[[nodiscard]] constexpr bool are_separate(hit_interval<ScalarT> const& a, hit_interval<ScalarT> const& b)
{
    return b.end < a.start || a.end < b.start;
}
template <int D, class ScalarT>
[[nodiscard]] constexpr hit_interval<ScalarT> shadow(aabb<D, ScalarT> const& b, vec<D, ScalarT> const& axis)
{
    auto const center = centroid_of(b);
    auto const c = dot(center, axis);
    auto const e = dot(b.max - center, abs(axis));
    return {c - e, c + e};
}
template <int ObjectD, class ScalarT, int DomainD>
[[nodiscard]] constexpr hit_interval<ScalarT> shadow(box<ObjectD, ScalarT, DomainD> const& b, vec<DomainD, ScalarT> const& axis)
{
    auto const c = dot(b.center, axis);
    auto e = ScalarT(0);
    for (auto i = 0; i < ObjectD; ++i)
        e += abs(dot(b.half_extents[i], axis));

    return {c - e, c + e};
}
template <class BaseT>
[[nodiscard]] constexpr hit_interval<typename BaseT::scalar_t> shadow(pyramid<BaseT> const& p, vec<3, typename BaseT::scalar_t> const& axis)
{
    using ScalarT = typename BaseT::scalar_t;
    auto tMin = tg::max<ScalarT>();
    auto tMax = tg::min<ScalarT>();
    for (auto const& vertex : vertices_of(p))
    {
        auto const t = dot(vertex, axis);
        tMin = tg::min(tMin, t);
        tMax = tg::max(tMax, t);
    }
    return {tMin, tMax};
}
// Helper function that uses the separating axis theorem and the provided list of axes to determine whether a and b intersect
template <class A, class B>
[[nodiscard]] constexpr bool intersects_SAT(A const& a, B const& b, span<vec<object_traits<B>::domain_dimension, typename B::scalar_t> const> axes)
{
    for (auto const& axis : axes)
        if (are_separate(shadow(a, axis), shadow(b, axis)))
            return false;

    return true;
}
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(line<1, ScalarT> const& l, aabb<1, ScalarT> const& b)
{
    TG_UNUSED(l);
    TG_UNUSED(b);
    return true;
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(line<2, ScalarT> const& l, aabb<2, ScalarT> const& b)
{
    auto const c = centroid_of(b);
    auto const shadow = dot(b.max - c, abs(perpendicular(l.dir)));
    return pow2(shadow) >= distance_sqr(c, l);
}
// line3 and line4 is deduced from intersection_parameter(l, b).has_value()

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(ray<D, ScalarT> const& r, aabb<D, ScalarT> const& b)
{
    for (auto i = 0; i < D; ++i)
    {
        if ((r.origin[i] > b.max[i] && r.dir[i] >= ScalarT(0)) || (r.origin[i] < b.min[i] && r.dir[i] <= ScalarT(0)))
            return false;
    }

    return intersects(inf_of(r), b);
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(segment<D, ScalarT> const& s, aabb<D, ScalarT> const& b)
{
    if (!intersects(aabb_of(s), b))
        return false;

    return intersects(inf_of(s), b);
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(plane<D, ScalarT> const& p, aabb<D, ScalarT> const& b)
{
    auto const c = centroid_of(b);
    auto const shadow = dot(b.max - c, abs(p.normal));
    return shadow >= distance(c, p); // Note: no square needed, since no sqrt involved
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(halfspace<D, ScalarT> const& h, aabb<D, ScalarT> const& b)
{
    auto const c = centroid_of(b);
    auto const dist = signed_distance(c, h);
    if (dist <= ScalarT(0))
        return true;

    auto const shadow = dot(b.max - c, abs(h.normal));
    return shadow >= dist;
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(halfspace<D, ScalarT> const& h, box<D, ScalarT> const& b)
{
    auto const c = centroid_of(b);
    auto const dist = signed_distance(c, h);
    if (dist <= ScalarT(0))
        return true;

    auto shadow = tg::abs(dot(b.half_extents[0], h.normal));
    if constexpr (D >= 2)
        shadow += tg::abs(dot(b.half_extents[1], h.normal));
    if constexpr (D >= 3)
        shadow += tg::abs(dot(b.half_extents[2], h.normal));
    if constexpr (D >= 4)
        shadow += tg::abs(dot(b.half_extents[3], h.normal));

    return shadow >= dist;
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(aabb<D, ScalarT> const& a, aabb<D, ScalarT> const& b)
{
    for (auto i = 0; i < D; ++i)
    {
        if (b.max[i] < a.min[i] || a.max[i] < b.min[i])
            return false;
    }
    return true;
}
template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(aabb_boundary<D, ScalarT> const& a, aabb<D, ScalarT> const& b)
{
    auto contained = true;
    for (auto i = 0; i < D; ++i)
    {
        if (b.max[i] < a.min[i] || a.max[i] < b.min[i])
            return false;

        contained = contained && a.min[i] < b.min[i] && b.max[i] < a.max[i];
    }
    return !contained;
}

template <int ObjectD, class ScalarT, int DomainD>
[[nodiscard]] constexpr bool intersects(box<ObjectD, ScalarT, DomainD> const& box, aabb<DomainD, ScalarT> const& b)
{
    if (!intersects(aabb_of(box), b))
        return false;

    if constexpr (DomainD == 1)
        return true; // the only axis was already checked above

    using vec_t = vec<DomainD, ScalarT>;

    constexpr int max_axes_count = DomainD + (DomainD > 2 ? DomainD * DomainD : 0);

    auto axes = array<vec_t, max_axes_count>();
    size_t curr_axis = 0;

    [[maybe_unused]] auto axisDirs = tg::array<vec_t, DomainD>();
    if constexpr (DomainD == 3)
        axisDirs = {vec_t::unit_x, vec_t::unit_y, vec_t::unit_z};

    for (auto i = 0; i < DomainD; ++i)
    {
        vec_t d;
        if constexpr (ObjectD == 2 && DomainD == 3) // box2in3
            d = i == 2 ? normal_of(box) : box.half_extents[i];
        else
            d = box.half_extents[i];
        axes[curr_axis++] = d;

        if constexpr (DomainD > 2)
            for (auto j = 0u; j < DomainD; ++j)
                axes[curr_axis++] = cross(d, axisDirs[j]);

        static_assert(DomainD < 4 && "Not implemented for 4D");
    }

    return detail::intersects_SAT(box, b, span<vec_t const>(axes).subspan(0, curr_axis));
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box_boundary<2, ScalarT, 3> const& box, aabb<3, ScalarT> const& b)
{
    return detail::intersects_any_array(b, edges_of(box), std::make_index_sequence<4>{});
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<D, ScalarT> const& s, aabb<D, ScalarT> const& b)
{
    auto const b_min = b.min;
    auto const b_max = b.max;
    auto const c = s.center;
    auto const clamped_sqr = [](ScalarT v)
    {
        v = tg::max(ScalarT(0), v);
        return v * v;
    };

    auto d_min = ScalarT(0);

    if constexpr (D >= 1)
    {
        d_min += clamped_sqr(b_min.x - c.x);
        d_min += clamped_sqr(c.x - b_max.x);
    }

    if constexpr (D >= 2)
    {
        d_min += clamped_sqr(b_min.y - c.y);
        d_min += clamped_sqr(c.y - b_max.y);
    }

    if constexpr (D >= 3)
    {
        d_min += clamped_sqr(b_min.z - c.z);
        d_min += clamped_sqr(c.z - b_max.z);
    }

    if constexpr (D >= 4)
    {
        d_min += clamped_sqr(b_min.w - c.w);
        d_min += clamped_sqr(c.w - b_max.w);
    }

    return d_min <= s.radius * s.radius;
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<1, ScalarT, 2> const& s, aabb<2, ScalarT> const& b)
{
    auto const v = perpendicular(s.normal) * s.radius;
    return intersects(segment<2, ScalarT>(s.center - v, s.center + v), b);
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere_boundary<1, ScalarT, 2> const& s, aabb<2, ScalarT> const& b)
{
    auto const v = perpendicular(s.normal) * s.radius;
    return contains(b, s.center - v) || contains(b, s.center + v);
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<2, ScalarT, 3> const& s, aabb<3, ScalarT> const& b)
{
    auto const diskPlane = plane<3, ScalarT>(s.normal, s.center);
    if (!intersects(diskPlane, b))
        return false;

    // early out, contains SAT for each aabb axis
    if (!intersects(sphere<3, ScalarT>(s.center, s.radius), b))
        return false;

    // check if disk extrema are within aabb. cross(cross(axisDir, n)) yields the following vectors
    if (contains(b, s.center))
        return true;
    auto const c = s.center;
    auto const n = s.normal;
    using vec_t = vec<3, ScalarT>;
    auto const vx = s.radius * normalize(vec_t(-n.y * n.y - n.z * n.z, n.x * n.y, n.x * n.z));
    if (contains(b, c + vx) || contains(b, c - vx))
        return true;
    auto const vy = s.radius * normalize(vec_t(n.x * n.y, -n.x * n.x - n.z * n.z, n.y * n.z));
    if (contains(b, c + vy) || contains(b, c - vy))
        return true;
    auto const vz = s.radius * normalize(vec_t(n.x * n.z, n.y * n.z, -n.x * n.x - n.y * n.y));
    if (contains(b, c + vz) || contains(b, c - vz))
        return true;

    // intersection test with each aabb edge
    for (auto const& edge : edges_of(b))
        if (intersects(edge, s))
            return true;

    return false;
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere_boundary<2, ScalarT, 3> const& s, aabb<3, ScalarT> const& b)
{
    auto const diskPlane = plane<3, ScalarT>(s.normal, s.center);
    if (!intersects(diskPlane, b))
        return false;

    // early out, contains SAT for each aabb axis
    if (!intersects(sphere<3, ScalarT>(s.center, s.radius), b))
        return false;

    // check if disk extrema are within aabb. cross product of axis dir and two times with n yield the following vectors
    auto const c = s.center;
    auto const n = s.normal;
    using vec_t = vec<3, ScalarT>;
    auto const eps = ScalarT(16) * tg::epsilon<ScalarT>;
    auto const vx = s.radius * normalize(vec_t(-n.y * n.y - n.z * n.z, n.x * n.y, n.x * n.z));
    if (contains(b, c + vx, eps) || contains(b, c - vx, eps))
        return true;
    auto const vy = s.radius * normalize(vec_t(n.x * n.y, -n.x * n.x - n.z * n.z, n.y * n.z));
    if (contains(b, c + vy, eps) || contains(b, c - vy, eps))
        return true;
    auto const vz = s.radius * normalize(vec_t(n.x * n.z, n.y * n.z, -n.x * n.x - n.y * n.y));
    if (contains(b, c + vz, eps) || contains(b, c - vz, eps))
        return true;

    // intersection test with each aabb edge
    auto inside = 0, outside = 0;
    for (auto const& edge : edges_of(b))
    {
        auto const t = intersection(edge, diskPlane);
        if (!t.has_value())
            continue;
        if (distance_sqr(t.value(), s.center) <= pow2(s.radius))
            inside++;
        else
            outside++;

        if (inside > 0 && outside > 0)
            return true;
    }
    return false;
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(hemisphere<D, ScalarT> const& h, aabb<D, ScalarT> const& b)
{
    auto const closestP = project(h.center, b);
    return contains(h, closestP) || intersects(caps_of(h), b);
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(hemisphere_boundary_no_caps<1, ScalarT> const& h, aabb<1, ScalarT> const& b)
{
    return contains(b, h.center + h.radius * h.normal);
}
template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(hemisphere_boundary_no_caps<D, ScalarT> const& h, aabb<D, ScalarT> const& b)
{
    auto const fullSphere = sphere<D, ScalarT>(h.center, h.radius);
    if (!intersects(fullSphere, b))
        return false; // early out

    if (intersects(caps_of(h), b))
        return true;

    // classify aabb vertices
    auto const spaceUnder = halfspace<D, ScalarT>(h.normal, h.center);
    auto inside = 0, outside = 0, under = 0;
    for (auto const& vertex : vertices_of(b))
    {
        if (contains(spaceUnder, vertex))
            under++;
        else if (contains(fullSphere, vertex))
            inside++;
        else
            outside++;

        if (inside > 0 && outside > 0)
            return true; // has to intersect the boundary
    }
    if (outside < 2)
        return false; // cannot cross the boundary without intersecting the caps_of(h)

    (void)under; // note: where was this intended?

    // note: outside and under cannot cross hemisphere through the inside due to Thales' theorem
    // now only a secant is left to check. Since inside == 0, we can check the closest projection onto the aabb
    auto const closestP = project(h.center, b);
    return contains(solid_of(h), closestP);
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(capsule<3, ScalarT> const& c, aabb<3, ScalarT> const& b)
{
    if (!intersects(aabb_of(c), b))
        return false;

    // check if the line through the axis intersects the aabb
    auto const line = inf_of(c.axis);
    auto const hits = intersection_parameter(line, boundary_of(b));
    if (hits.any())
    {
        auto const len = length(c.axis);
        auto const t = clamp(hits.first(), ScalarT(0), len);
        for (auto const& hit : hits)
        {
            if (ScalarT(0) - c.radius <= hit && hit <= len + c.radius)
                return true; // capsule axis intersects aabb

            if (t != clamp(hit, ScalarT(0), len))
                return true; // intersections before and after the axis can only occur if it lies within aabb
        }
        return intersects(sphere<3, ScalarT>(line[t], c.radius), b);
    }

    // test spheres at both capsule ends (cheap)
    if (intersects(sphere<3, ScalarT>(c.axis.pos0, c.radius), b) || intersects(sphere<3, ScalarT>(c.axis.pos1, c.radius), b))
        return true;

    // now only intersections between aabb edges and capsule mantle remain
    auto const r2 = c.radius * c.radius;
    for (auto const& edge : edges_of(b))
        if (distance_sqr(edge, c.axis) <= r2)
            return true;

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(cylinder<3, ScalarT> const& c, aabb<3, ScalarT> const& b)
{
    if (!intersects(aabb_of(c), b))
        return false;

    // check if the line through the axis intersects the aabb
    auto const line = inf_of(c.axis);
    auto const len = length(c.axis);
    auto const hits = intersection_parameter(line, boundary_of(b));
    if (hits.any())
    {
        auto const t = clamp(hits.first(), ScalarT(0), len);
        for (auto const& hit : hits)
        {
            if (ScalarT(0) <= hit && hit <= len)
                return true; // cylinder axis intersects aabb

            if (t != clamp(hit, ScalarT(0), len))
                return true; // intersections before and after the axis can only occur if it lies within aabb
        }
        return intersects(sphere<2, ScalarT, 3>(line[t], c.radius, line.dir), b);
    }

    // test disks at both cylinder ends
    if (intersects(sphere<2, ScalarT, 3>(c.axis.pos0, c.radius, line.dir), b) || //
        intersects(sphere<2, ScalarT, 3>(c.axis.pos1, c.radius, line.dir), b))
        return true;

    // now only intersections between aabb edges and cylinder mantle remain
    auto const r2 = c.radius * c.radius;
    for (auto const& edge : edges_of(b))
    {
        auto [te, tl] = closest_points_parameters(edge, line);
        if (ScalarT(0) < tl && tl < len && distance_sqr(edge[te], line[tl]) <= r2)
            return true;
    }

    return false;
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(cylinder_boundary_no_caps<3, ScalarT> const& c, aabb<3, ScalarT> const& b)
{
    // alternative idea that is more efficient:
    // compute the polygon of the aabb from the projection in cylinder direction and intersect it with a circle
    //
    // line = inf_of(c.axis);
    // len = length(c.axis);
    // compute planes p1 and p2 spanned by both caps
    // foreach(vertex in vertices_of(b)) {
    //      t = coordinates(line, vertex);
    //      if (0 <= t <= len) add(vertex - t*line.dir);
    // }
    // foreach(edge in edges_of(b)) {
    //      p = intersection(edge, p1);
    //      if (p.has_value()) add(p.value());
    //      p = intersection(edge, p2);
    //      if (p.has_value()) add(p.value() - len*line.dir);
    // }
    // compute polygon as convex hull of all added vertices
    // return intersects(caps_of(c)[0], polygon); // maybe in 2D

    if (!intersects(aabb_of(c), b))
        return false;

    // check intersections between line through the axis and the aabb
    auto const line = inf_of(c.axis);
    auto const len = length(c.axis);
    auto const intersects_at = [&](ScalarT t) { return intersects(sphere_boundary<2, ScalarT, 3>(line[t], c.radius, line.dir), b); };

    auto const hits = intersection_parameter(line, boundary_of(b));
    for (auto const& hit : hits)
        if (ScalarT(0) < hit && hit < len && intersects_at(hit))
            return true;

    // test disks at both cylinder ends
    if (intersects(sphere_boundary<2, ScalarT, 3>(c.axis.pos0, c.radius, line.dir), b) || //
        intersects(sphere_boundary<2, ScalarT, 3>(c.axis.pos1, c.radius, line.dir), b))
        return true;

    // now only intersections between aabb edges and cylinder mantle remain
    for (auto const& edge : edges_of(b))
    {
        auto [te, tl] = closest_points_parameters(edge, line);
        if (ScalarT(0) < tl && tl < len && intersects_at(tl))
            return true;
    }

    return false;
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(inf_cylinder<D, ScalarT> const& c, aabb<D, ScalarT> const& b)
{
    if (intersects(c.axis, b))
        return true;

    auto const r2 = c.radius * c.radius;
    for (auto const& edge : edges_of(b))
        if (distance_sqr(edge, c.axis) <= r2)
            return true;

    return false;
}

template <class BaseT>
[[nodiscard]] constexpr auto intersects(pyramid<BaseT> const& p, aabb<3, typename BaseT::scalar_t> const& b) -> decltype((void)faces_of(p), true)
{
    // SAT: box faces
    if (!intersects(aabb_of(p), b))
        return false;

    // SAT: pyramid faces
    using vec_t = vec<3, typename BaseT::scalar_t>;
    {
        auto const faces = faces_of(p);
        constexpr int max_axes_count = 1 + faces.mantle.size();

        auto axes = array<vec_t, max_axes_count>();
        size_t curr_axis = 0;
        axes[curr_axis++] = normal_of(faces.base);
        for (auto const& face : faces.mantle)
            axes[curr_axis++] = normal_of(face);

        if (!detail::intersects_SAT(p, b, axes))
            return false;
    }

    // SAT: cross product of edge pairs
    {
        auto edges = edges_of(p);
        constexpr int max_axes_count = 3 * edges.size();
        auto axes = array<vec_t, max_axes_count>();
        size_t curr_axis = 0;

        array<vec_t, 3> axisDirs = {vec_t::unit_x, vec_t::unit_y, vec_t::unit_z};
        for (auto const& edge : edges)
        {
            vec_t d = direction(edge);
            for (auto j = 0u; j < 3; ++j)
                axes[curr_axis++] = cross(d, axisDirs[j]);
        }

        return detail::intersects_SAT(p, b, axes);
    }
}
template <class BaseT>
[[nodiscard]] constexpr auto intersects(pyramid_boundary_no_caps<BaseT> const& p, aabb<3, typename BaseT::scalar_t> const& b)
    -> decltype((void)faces_of(p), true)
{
    // SAT: box faces
    if (!intersects(aabb_of(p), b))
        return false;

    auto const faces = faces_of(p);
    return detail::intersects_any_array(b, faces, std::make_index_sequence<faces.size()>{});
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<2, ScalarT> const& a, aabb<2, ScalarT> const& b)
{
    if (!intersects(aabb_of(a), b))
        return false;

    auto p0 = a.pos0;
    auto p1 = a.pos1;
    auto p2 = a.pos2;
    if (contains(b, p0) || contains(b, p1) || contains(b, p2))
        return true;

    auto aabb_pts = vertices_of(b);

    auto const is_separate = [&](pos<2, ScalarT> pa, vec<2, ScalarT> n, pos<2, ScalarT> pb)
    {
        auto da = dot(n, pa);
        auto db = dot(n, pb);

        // TODO: faster
        auto a_min = min(da, db);
        auto a_max = max(da, db);

        auto b_min = dot(n, aabb_pts[0]);
        auto b_max = b_min;
        for (auto i = 1u; i < 4; ++i)
        {
            auto d = dot(n, aabb_pts[i]);
            b_min = min(b_min, d);
            b_max = max(b_max, d);
        }

        if (b_max < a_min || b_min > a_max)
            return true;

        return false;
    };

    if (is_separate(p0, perpendicular(p1 - p0), p2))
        return false;
    if (is_separate(p1, perpendicular(p2 - p1), p0))
        return false;
    if (is_separate(p2, perpendicular(p0 - p2), p1))
        return false;

    return true;
}

// NOTE: does NOT work for integer objects
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& tri_in, aabb<3, ScalarT> const& bb_in)
{
    using pos_t = pos<3, ScalarT>;
    using vec_t = vec<3, ScalarT>;

    auto const center = centroid_of(bb_in);
    auto const amin = pos_t(bb_in.min - center);
    auto const amax = pos_t(bb_in.max - center);
    auto const bb = aabb<3, ScalarT>(amin, amax);

    auto const p0 = pos_t(tri_in.pos0 - center);
    auto const p1 = pos_t(tri_in.pos1 - center);
    auto const p2 = pos_t(tri_in.pos2 - center);

    // early out: AABB vs tri AABB
    auto tri_aabb = aabb_of(p0, p1, p2);
    if (tri_aabb.max.x < amin.x || tri_aabb.max.y < amin.y || tri_aabb.max.z < amin.z || //
        tri_aabb.min.x > amax.x || tri_aabb.min.y > amax.y || tri_aabb.min.z > amax.z)
        return false;

    auto const proper_contains = [](aabb<3, ScalarT> const& b, pos_t const& p)
    {
        return b.min.x < p.x && p.x < b.max.x && //
               b.min.y < p.y && p.y < b.max.y && //
               b.min.z < p.z && p.z < b.max.z;
    };

    // early in: tri points vs AABB
    if (proper_contains(bb, p0) || proper_contains(bb, p1) || proper_contains(bb, p2))
        return true;

    // get adjusted tri base plane
    auto p = plane<3, ScalarT>(normal_of(tri_in), p0);

    // fast plane / AABB test
    {
        auto pn = p.normal;
        auto bn = dot(abs(pn), amax);

        // min dis: d - bn
        if (bn < -p.dis)
            return false;

        // max dis: d + bn
        if (-p.dis < -bn)
            return false;
    }

    // 9 axis SAT test
    {
        auto const is_seperating = [amax](vec<3, ScalarT> const& n, pos_t const& tp0, pos_t const& tp1) -> bool
        {
            if (tg::is_zero_vector(n))
                return false; // not a real candidate axis

            // fast point / AABB separation test
            auto bn = dot(abs(n), amax);

            auto tn0 = dot(n, tp0);
            auto tn1 = dot(n, tp1);

            auto tmin = min(tn0, tn1);
            auto tmax = max(tn0, tn1);

            auto bmin = -bn;
            auto bmax = bn;

            if (tmax < bmin)
                return true;
            if (bmax < tmin)
                return true;

            return false;
        };

        if (is_seperating(cross(p1 - p0, vec_t::unit_x), p0, p2))
            return false;
        if (is_seperating(cross(p1 - p0, vec_t::unit_y), p0, p2))
            return false;
        if (is_seperating(cross(p1 - p0, vec_t::unit_z), p0, p2))
            return false;

        if (is_seperating(cross(p2 - p0, vec_t::unit_x), p0, p1))
            return false;
        if (is_seperating(cross(p2 - p0, vec_t::unit_y), p0, p1))
            return false;
        if (is_seperating(cross(p2 - p0, vec_t::unit_z), p0, p1))
            return false;

        if (is_seperating(cross(p1 - p2, vec_t::unit_x), p0, p2))
            return false;
        if (is_seperating(cross(p1 - p2, vec_t::unit_y), p0, p2))
            return false;
        if (is_seperating(cross(p1 - p2, vec_t::unit_z), p0, p2))
            return false;
    }

    // found no separating axis? -> intersection
    return true;
}

// ====================================== Checks if Object Intersects Object ======================================

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(segment<3, ScalarT> const& segment, sphere<2, ScalarT, 3> const& disk)
{
    auto t = intersection(segment, plane<3, ScalarT>(disk.normal, disk.center));
    return t.has_value() && distance_sqr(t.value(), disk.center) <= pow2(disk.radius);
}
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<2, ScalarT, 3> const& disk, segment<3, ScalarT> const& segment)
{
    return intersects(segment, disk);
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box<2, ScalarT> const& a, box<2, ScalarT> const& b)
{
    if (a.center == b.center)
        return true;

    auto const ab = b.center - a.center;
    auto const ba = -ab;

    /// compute the smallest corner of box in direction d
    auto const min_point = [](dir<2, ScalarT> d, box<2, ScalarT> const& box)
    {
        auto point = box.center;
        if (dot(d, box.half_extents[0]) > 0)
            point -= box.half_extents[0];
        else
            point += box.half_extents[0];
        if (dot(d, box.half_extents[1]) > 0)
            point -= box.half_extents[1];
        else
            point += box.half_extents[1];
        return point;
    };

    // the idea here is that we need only check the two planes facing the center of the other bounding box
    // and only need to check against the smallest corner

    // check planes of a vs smallest point of b
    if (dot(ab, a.half_extents[0]) > 0)
    {
        auto pl = plane<2, ScalarT>(normalize(a.half_extents[0]), a.center + a.half_extents[0]);
        auto point_to_check = min_point(pl.normal, b);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }
    else
    {
        auto pl = plane<2, ScalarT>(normalize(-a.half_extents[0]), a.center - a.half_extents[0]);
        auto point_to_check = min_point(pl.normal, b);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }

    if (dot(ab, a.half_extents[1]) > 0)
    {
        auto pl = plane<2, ScalarT>(normalize(a.half_extents[1]), a.center + a.half_extents[1]);
        auto point_to_check = min_point(pl.normal, b);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }
    else
    {
        auto pl = plane<2, ScalarT>(normalize(-a.half_extents[1]), a.center - a.half_extents[1]);
        auto point_to_check = min_point(pl.normal, b);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }

    // check planes of b vs smallest point of a
    if (dot(ba, b.half_extents[0]) > 0)
    {
        auto pl = plane<2, ScalarT>(normalize(b.half_extents[0]), b.center + b.half_extents[0]);
        auto point_to_check = min_point(pl.normal, a);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }
    else
    {
        auto pl = plane<2, ScalarT>(normalize(-b.half_extents[0]), b.center - b.half_extents[0]);
        auto point_to_check = min_point(pl.normal, a);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }

    if (dot(ba, b.half_extents[1]) > 0)
    {
        auto pl = plane<2, ScalarT>(normalize(b.half_extents[1]), b.center + b.half_extents[1]);
        auto point_to_check = min_point(pl.normal, a);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }
    else
    {
        auto pl = plane<2, ScalarT>(normalize(-b.half_extents[1]), b.center - b.half_extents[1]);
        auto point_to_check = min_point(pl.normal, a);
        if (signed_distance(point_to_check, pl) > 0)
            return false;
    }

    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects_conservative(frustum<3, ScalarT> const& frustum, sphere<3, ScalarT> const& sphere, dont_deduce<ScalarT> eps = ScalarT(0))
{
    // if center is further away than radius, there cannot be any intersection

    auto const d_nx = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_neg_x]);
    if (d_nx > sphere.radius + eps)
        return false;

    auto const d_ny = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_neg_y]);
    if (d_ny > sphere.radius + eps)
        return false;

    auto const d_nz = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_neg_z]);
    if (d_nz > sphere.radius + eps)
        return false;

    auto const d_px = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_pos_x]);
    if (d_px > sphere.radius + eps)
        return false;

    auto const d_py = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_pos_y]);
    if (d_py > sphere.radius + eps)
        return false;

    auto const d_pz = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_pos_z]);
    if (d_pz > sphere.radius + eps)
        return false;

    // conservative approximation!
    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects_conservative(frustum<3, ScalarT> const& frustum, aabb<3, ScalarT> const& bb)
{
    using halfspace_t = halfspace<3, ScalarT>;

    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_x]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_y]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_z]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_x]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_y]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_z]), bb))
        return false;

    // conservative approximation!
    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects_conservative(frustum<3, ScalarT> const& frustum, box<3, ScalarT> const& box)
{
    using halfspace_t = halfspace<3, ScalarT>;

    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_x]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_y]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_z]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_x]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_y]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_z]), box))
        return false;

    // conservative approximation!
    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects_conservative(inf_frustum<3, ScalarT> const& frustum, sphere<3, ScalarT> const& sphere, dont_deduce<ScalarT> eps = ScalarT(0))
{
    // if center is further away than radius, there cannot be any intersection

    auto const d_nx = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_neg_x]);
    if (d_nx > sphere.radius + eps)
        return false;

    auto const d_ny = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_neg_y]);
    if (d_ny > sphere.radius + eps)
        return false;

    auto const d_px = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_pos_x]);
    if (d_px > sphere.radius + eps)
        return false;

    auto const d_py = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_pos_y]);
    if (d_py > sphere.radius + eps)
        return false;

    auto const d_pz = signed_distance(sphere.center, frustum.planes[frustum.plane_idx_pos_z]);
    if (d_pz > sphere.radius + eps)
        return false;

    // conservative approximation!
    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects_conservative(inf_frustum<3, ScalarT> const& frustum, aabb<3, ScalarT> const& bb)
{
    using halfspace_t = halfspace<3, ScalarT>;

    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_x]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_y]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_x]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_y]), bb))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_z]), bb))
        return false;

    // conservative approximation!
    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects_conservative(inf_frustum<3, ScalarT> const& frustum, box<3, ScalarT> const& box)
{
    using halfspace_t = halfspace<3, ScalarT>;

    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_x]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_neg_y]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_x]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_y]), box))
        return false;
    if (!intersects(halfspace_t(frustum.planes[frustum.plane_idx_pos_z]), box))
        return false;

    // conservative approximation!
    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(plane<3, ScalarT> const& plane, triangle<3, ScalarT> const& triangle)
{
    // classify vertices
    auto sign_v1 = signed_distance(triangle.pos0, plane) < 0 ? false : true;
    auto sign_v2 = signed_distance(triangle.pos1, plane) < 0 ? false : true;
    auto sign_v3 = signed_distance(triangle.pos2, plane) < 0 ? false : true;

    // exclude some degenerate cases? e.g. vertices of triangle on same positions, angle constraints..

    if (sign_v1 == sign_v2 && sign_v2 == sign_v3) // no intersection (early out)
        return {};

    // isolated vertex
    bool iv = (sign_v1 == sign_v2) ? sign_v3 : (sign_v1 == sign_v3) ? sign_v2 : sign_v1;

    pos<3, ScalarT> i1, i2;

    // intersection exists (exactly 2 vertices on one side of the plane and exactly 1 vertex on the other side)
    if (iv == sign_v1)
    {
        i1 = intersection(segment<3, ScalarT>(triangle.pos0, triangle.pos1), plane).value();
        i2 = intersection(segment<3, ScalarT>(triangle.pos0, triangle.pos2), plane).value();
    }
    else if (iv == sign_v2)
    {
        i1 = intersection(segment<3, ScalarT>(triangle.pos0, triangle.pos1), plane).value();
        i2 = intersection(segment<3, ScalarT>(triangle.pos1, triangle.pos2), plane).value();
    }
    else if (iv == sign_v3)
    {
        i1 = intersection(segment<3, ScalarT>(triangle.pos0, triangle.pos2), plane).value();
        i2 = intersection(segment<3, ScalarT>(triangle.pos1, triangle.pos2), plane).value();
    }
    else
        return {};

    return segment<3, ScalarT>(i1, i2);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(triangle<3, ScalarT> const& triangle, plane<3, ScalarT> const& plane)
{
    return intersection(plane, triangle);
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(plane<3, ScalarT> const& plane, triangle<3, ScalarT> const& triangle)
{
    tg::array<pos<3, ScalarT>, 3> triangle_pos = {triangle.pos0, triangle.pos1, triangle.pos2};
    ScalarT sign = 0;

    for (auto tr : triangle_pos)
    {
        if (sign == ScalarT(0))
        {
            sign = dot(plane.normal, tr) - plane.dis;
            if (sign == ScalarT(0))
                return true;
        }

        else
        {
            if ((dot(plane.normal, tr) - plane.dis) * sign < ScalarT(0))
                return true;
        }
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& triangle, plane<3, ScalarT> const& plane)
{
    return intersects(plane, triangle);
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<2, ScalarT> const& t1, triangle<2, ScalarT> const& t2)
{
    // Implementation of: https://hal.inria.fr/inria-00072100/document

    auto const determin = [](pos<2, ScalarT> pa, pos<2, ScalarT> pb, pos<2, ScalarT> pc) -> float
    {
        auto m = mat<2, 2, ScalarT>::from_data_colwise({pa.x - pc.x, pb.x - pc.x, pa.y - pc.y, pb.y - pc.y});
        return determinant(m);
    };

    auto const counter_clock = [determin](tg::triangle<2, ScalarT>& tri_a) -> void
    {
        if (determin(tri_a.pos0, tri_a.pos1, tri_a.pos2) < ScalarT(0))
            tri_a = triangle<2, ScalarT>(tri_a.pos0, tri_a.pos2, tri_a.pos1); // swap
    };

    auto const rotate = [determin](triangle<2, ScalarT>& tri_b, triangle<2, ScalarT>& tri_a, comp<3, ScalarT>& determinants_a) -> void
    {
        // rotate triangle vertices by one
        tri_b = triangle<2, ScalarT>(tri_b.pos2, tri_b.pos0, tri_b.pos1);

        // recompute determinants
        determinants_a = comp<3, ScalarT>(determin(tri_b.pos0, tri_b.pos1, tri_a.pos0), determin(tri_b.pos1, tri_b.pos2, tri_a.pos0),
                                          determin(tri_b.pos2, tri_b.pos0, tri_a.pos0));
    };

    triangle<2, ScalarT> ta = t1;
    triangle<2, ScalarT> tb = t2;

    // ensure counter-clockwise orientation
    counter_clock(ta);
    counter_clock(tb);

    auto det_ta0 = tg::comp<3, ScalarT>(determin(tb.pos0, tb.pos1, ta.pos0), determin(tb.pos1, tb.pos2, ta.pos0), determin(tb.pos2, tb.pos0, ta.pos0));
    auto det_01 = det_ta0[0] * det_ta0[1];
    auto det_12 = det_ta0[1] * det_ta0[2];
    auto det_02 = det_ta0[0] * det_ta0[2];


    if (det_01 > ScalarT(0) && det_12 > ScalarT(0)) // ta.pos0 interior of tb
        return true;

    if (det_01 == ScalarT(0) && det_12 == ScalarT(0) && det_02 == ScalarT(0)) // ta.pos0 lies on vertex of tb
        return true;

    if (det_01 == ScalarT(0) && ((det_ta0[1] > ScalarT(0) && det_ta0[2] > ScalarT(0)) || (det_ta0[0] > ScalarT(0) && det_ta0[2] > ScalarT(0)))) // ta.pos0 interior of edge of tb
        return true;

    if (det_12 == 0 && ((det_ta0[0] > ScalarT(0) && det_ta0[1] > ScalarT(0)) || (det_ta0[0] > ScalarT(0) && det_ta0[2] > ScalarT(0)))) // ta.pos0 interior of edge of tb
        return true;

    // Rotate to Region1 or Region2
    while (!(det_ta0[0] > ScalarT(0) && det_ta0[2] < ScalarT(0)))
        rotate(tb, ta, det_ta0);

    // decision tree
    // R1
    if (det_ta0[1] > ScalarT(0))
    {
        // I
        if (determin(tb.pos2, tb.pos0, ta.pos1) >= ScalarT(0))
        {
            // II.a
            if (determin(tb.pos2, ta.pos0, ta.pos1) < ScalarT(0))
                return false;
            // III.a
            else if (determin(ta.pos0, tb.pos0, ta.pos1) < ScalarT(0))
            {
                // IV.a
                if (determin(ta.pos0, tb.pos0, ta.pos2) < ScalarT(0))
                    return false;
                // V
                else if (determin(ta.pos1, ta.pos2, tb.pos0) < ScalarT(0))
                    return false;
            }
            return true;
        }
        // II.b
        else if (determin(tb.pos2, tb.pos0, ta.pos2) < ScalarT(0))
            return false;
        // III.b
        else if (determin(ta.pos1, ta.pos2, tb.pos2) < ScalarT(0))
            return false;
        // IV.b
        else if (determin(ta.pos0, tb.pos0, ta.pos2) < ScalarT(0))
            return false;

        return true;
    }

    // R2
    else if (determin(tb.pos2, tb.pos0, ta.pos1) >= ScalarT(0))
    {
        // II.a
        if (determin(tb.pos1, tb.pos2, ta.pos1) >= ScalarT(0))
        {
            // III.a
            if (determin(ta.pos0, tb.pos0, ta.pos1) >= ScalarT(0))
            {
                // IV.a
                if (determin(ta.pos0, tb.pos1, ta.pos1) > ScalarT(0))
                    return false;

                return true;
            }
            // IV.b
            else if (determin(ta.pos0, tb.pos0, ta.pos2) < ScalarT(0))
                return false;
            // V.a
            else if (determin(tb.pos2, tb.pos0, ta.pos2) < ScalarT(0))
            {
                if (determin(ta.pos1, tb.pos0, ta.pos2) > ScalarT(0))
                    return false;
            }

            return true;
        }
        // III.b
        else if (determin(ta.pos0, tb.pos1, ta.pos1) > ScalarT(0))
            return false;
        // IV.c
        else if (determin(tb.pos1, tb.pos2, ta.pos2) < ScalarT(0))
            return false;
        // V.b
        else if (determin(ta.pos1, ta.pos2, tb.pos1) < ScalarT(0))
            return false;

        return true;
    }
    // II.b
    else if (determin(tb.pos2, tb.pos0, ta.pos2) < ScalarT(0))
        return false;
    // III.c
    else if (determin(ta.pos1, ta.pos2, tb.pos2) >= ScalarT(0))
    {
        // IV.d
        if (determin(ta.pos2, ta.pos0, tb.pos0) < ScalarT(0))
            return false;
    }
    // IV.e
    else if (determin(ta.pos1, ta.pos2, tb.pos1) < ScalarT(0))
        return false;
    // V.c
    else if (determin(tb.pos1, tb.pos2, ta.pos2) < ScalarT(0))
        return false;

    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& t1, triangle<3, ScalarT> const& t2)
{
    // Implementation of https://hal.inria.fr/inria-00072100/document

    auto const determin = [](pos<3, ScalarT> pa, pos<3, ScalarT> pb, pos<3, ScalarT> pc, pos<3, ScalarT> pd) -> float
    {
        auto m = mat<3, 3, ScalarT>::from_data_colwise(
            {pa.x - pd.x, pb.x - pd.x, pc.x - pd.x, pa.y - pd.y, pb.y - pd.y, pc.y - pd.y, pa.z - pd.z, pb.z - pd.z, pc.z - pd.z});
        return determinant(m);
    };

    auto det_t2_t1 = comp<3, ScalarT>(determin(t2.pos0, t2.pos1, t2.pos2, t1.pos0), determin(t2.pos0, t2.pos1, t2.pos2, t1.pos1),
                                      determin(t2.pos0, t2.pos1, t2.pos2, t1.pos2));

    auto const dt2_01 = det_t2_t1[0] * det_t2_t1[1];
    auto const dt2_02 = det_t2_t1[0] * det_t2_t1[2];

    // a) no insec of t1 with plane of t2
    if (dt2_01 > ScalarT(0) && dt2_02 > ScalarT(0))
        return false;

    // b) coplanar -> all dets are 0
    if (det_t2_t1[0] == det_t2_t1[1] && det_t2_t1[1] == det_t2_t1[2] && det_t2_t1[2] == ScalarT(0))
    {
        auto n = normal_of(t1);
        // find appropriate projection plane
        auto proj_plane = dot(n, tg::dir<3, ScalarT>(0.f, 1.f, 0.f)) == 0.f ? plane<3, ScalarT>({0.f, 0.f, 1.f}, pos<3, ScalarT>::zero)
                                                                            : plane<3, ScalarT>({0.f, 1.f, 0.f}, pos<3, ScalarT>::zero);
        // project triangles onto plane
        auto t1_2D = proj_plane.normal.z == ScalarT(0)
                         ? triangle<2, ScalarT>(xz(project(t1.pos0, proj_plane)), xz(project(t1.pos1, proj_plane)), xz(project(t1.pos2, proj_plane)))
                         : triangle<2, ScalarT>(xy(project(t1.pos0, proj_plane)), xy(project(t1.pos1, proj_plane)), xy(project(t1.pos2, proj_plane)));
        auto t2_2D = proj_plane.normal.z == ScalarT(0)
                         ? triangle<2, ScalarT>(xz(project(t2.pos0, proj_plane)), xz(project(t2.pos1, proj_plane)), xz(project(t2.pos2, proj_plane)))
                         : triangle<2, ScalarT>(xy(project(t2.pos0, proj_plane)), xy(project(t2.pos1, proj_plane)), xy(project(t2.pos2, proj_plane)));

        return intersects(t1_2D, t2_2D);
    }

    auto det_t1_t2 = comp<3, ScalarT>(determin(t1.pos0, t1.pos1, t1.pos2, t2.pos0), determin(t1.pos0, t1.pos1, t1.pos2, t2.pos1),
                                      determin(t1.pos0, t1.pos1, t1.pos2, t2.pos2));

    auto const dt1_01 = det_t1_t2[0] * det_t1_t2[1];
    auto const dt1_02 = det_t1_t2[0] * det_t1_t2[2];

    // a) no insec of t2 with plane of t1
    if (dt1_01 > ScalarT(0) && dt1_02 > ScalarT(0))
        return false;

    triangle<3, ScalarT> ta = t1;
    triangle<3, ScalarT> tb = t2;

    // circular permutation
    detail::rotate_devillers_triangle(ta, tb, det_t2_t1, det_t1_t2);
    detail::rotate_devillers_triangle(tb, ta, det_t1_t2, det_t2_t1);

    // decision tree
    if (determin(ta.pos0, ta.pos1, tb.pos0, tb.pos1) > ScalarT(0))
        return false;

    if (determin(ta.pos0, ta.pos2, tb.pos2, tb.pos0) > ScalarT(0))
        return false;

    return true;
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(triangle<3, ScalarT> const& t1, triangle<3, ScalarT> const& t2)
{
    // Implementation of https://hal.inria.fr/inria-00072100/document

    auto const determin = [](pos<3, ScalarT> pa, pos<3, ScalarT> pb, pos<3, ScalarT> pc, pos<3, ScalarT> pd) -> float
    {
        auto m = mat<3, 3, ScalarT>::from_data_colwise(
            {pa.x - pd.x, pb.x - pd.x, pc.x - pd.x, pa.y - pd.y, pb.y - pd.y, pc.y - pd.y, pa.z - pd.z, pb.z - pd.z, pc.z - pd.z});
        return determinant(m);
    };

    auto det_t2_t1 = comp<3, ScalarT>(determin(t2.pos0, t2.pos1, t2.pos2, t1.pos0), determin(t2.pos0, t2.pos1, t2.pos2, t1.pos1),
                                      determin(t2.pos0, t2.pos1, t2.pos2, t1.pos2));

    auto const dt2_01 = det_t2_t1[0] * det_t2_t1[1];
    auto const dt2_02 = det_t2_t1[0] * det_t2_t1[2];

    // a) no insec of t1 with plane of t2
    if (dt2_01 > ScalarT(0) && dt2_02 > ScalarT(0))
        return {};

    // b) coplanar -> case not yet handled (arbitrary polygonal return type required)
    if (det_t2_t1[0] == det_t2_t1[1] && det_t2_t1[1] == det_t2_t1[2] && det_t2_t1[2] == 0.f)
        return {};

    auto det_t1_t2 = comp<3, ScalarT>(determin(t1.pos0, t1.pos1, t1.pos2, t2.pos0), determin(t1.pos0, t1.pos1, t1.pos2, t2.pos1),
                                      determin(t1.pos0, t1.pos1, t1.pos2, t2.pos2));

    auto const dt1_01 = det_t1_t2[0] * det_t1_t2[1];
    auto const dt1_02 = det_t1_t2[0] * det_t1_t2[2];

    // a) no insec of t2 with plane of t1
    if (dt1_01 > ScalarT(0) && dt1_02 > ScalarT(0))
        return {};

    triangle<3, ScalarT> ta = t1;
    triangle<3, ScalarT> tb = t2;

    // circular permutation for triangle orientation
    detail::rotate_devillers_triangle(ta, tb, det_t2_t1, det_t1_t2);
    detail::rotate_devillers_triangle(tb, ta, det_t1_t2, det_t2_t1);

    // decision tree
    plane<3, ScalarT> p1 = plane_of(ta);
    plane<3, ScalarT> p2 = plane_of(tb);

    // intersects tests
    if (determin(ta.pos0, ta.pos1, tb.pos0, tb.pos1) > ScalarT(0))
        return {};

    else if (determin(ta.pos0, ta.pos2, tb.pos2, tb.pos0) > ScalarT(0))
        return {};

    // decision tree to find intersection segment
    else if (determin(ta.pos0, ta.pos2, tb.pos1, tb.pos0) > ScalarT(0))
    {
        if (determin(ta.pos0, ta.pos1, tb.pos2, tb.pos0) > ScalarT(0))
        {
            return segment<3, ScalarT>{intersection(tg::inf_of(segment<3, ScalarT>{ta.pos0, ta.pos2}), p2).first(),
                                       intersection(tg::inf_of(segment<3, ScalarT>{tb.pos0, tb.pos2}), p1).first()};
        }

        return segment<3, ScalarT>{intersection(tg::inf_of(segment<3, ScalarT>{ta.pos0, ta.pos2}), p2).first(),
                                   intersection(tg::inf_of(segment<3, ScalarT>{tb.pos0, tb.pos1}), p1).first()};
    }

    else if (determin(ta.pos0, ta.pos1, tb.pos2, tb.pos0) > ScalarT(0))
        return segment<3, ScalarT>{intersection(tg::inf_of(segment<3, ScalarT>{tb.pos0, tb.pos1}), p1).first(),
                                   intersection(tg::inf_of(segment<3, ScalarT>{tb.pos0, tb.pos2}), p1).first()};

    return segment<3, ScalarT>{intersection(tg::inf_of(segment<3, ScalarT>{tb.pos0, tb.pos1}), p1).first(),
                               intersection(tg::inf_of(segment<3, ScalarT>{ta.pos0, ta.pos1}), p2).first()};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<pos<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, triangle<3, ScalarT> const& triangle)
{
    dir<3, ScalarT> normal_t = normalize(cross((triangle.pos1 - triangle.pos0), (triangle.pos2 - triangle.pos0)));

    plane<3, ScalarT> plane_t = plane<3, ScalarT>(normal_t, triangle.pos0);
    // intersection point segment-plane
    auto insec = intersection(segment, plane_t);
    // early out
    if (!insec.has_value())
        return {};

    // insec in triangle?
    dir<3, ScalarT> a = normalize(cross(triangle.pos1 - triangle.pos0, normal_t));
    dir<3, ScalarT> b = normalize(cross(triangle.pos2 - triangle.pos1, normal_t));
    dir<3, ScalarT> c = normalize(cross(triangle.pos0 - triangle.pos2, normal_t));
    bool b_a = signed_distance(insec.value(), plane<3, ScalarT>(a, triangle.pos1)) <= 0 ? false : true;
    bool b_b = signed_distance(insec.value(), plane<3, ScalarT>(b, triangle.pos2)) <= 0 ? false : true;
    bool b_c = signed_distance(insec.value(), plane<3, ScalarT>(c, triangle.pos0)) <= 0 ? false : true;

    if (b_a == b_b && b_b == b_c)
        return insec;

    return {};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<pos<3, ScalarT>> intersection(triangle<3, ScalarT> const& triangle, segment<3, ScalarT> const& segment)
{
    return intersection(segment, triangle);
}

// TODO: there might be a more effective way
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& seg, aabb<3, ScalarT> const& bb)
{
    // segment entirely inside the bounding box
    if (contains(bb, seg.pos0) && contains(bb, seg.pos1))
        return seg;

    auto segment_line = line<3, ScalarT>(seg.pos0, normalize(seg.pos1 - seg.pos0));
    auto param_insec = intersection_parameter(segment_line, bb);

    if (!param_insec.has_value())
        return {};

    auto interval = param_insec.value().clamped(ScalarT(0), length(seg));
    if (!interval.has_value())
        return {};

    return segment<3, ScalarT>(segment_line[interval.value().start], segment_line[interval.value().end]);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(aabb<3, ScalarT> const& bb, segment<3, ScalarT> const& segment)
{
    return intersection(segment, bb);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& seg, box<3, ScalarT> const& box)
{
    // early-out: Both segment points inside of box
    if (contains(box, seg.pos0) && contains(box, seg.pos1))
        return seg;

    line<3, ScalarT> segment_line = {seg.pos0, normalize(seg.pos1 - seg.pos0)};
    auto param_insec = intersection_parameter(segment_line, box);

    if (!param_insec.has_value())
        return {};

    auto interval = param_insec.value().clamped(ScalarT(0), length(seg));
    if (!interval.has_value())
        return {};

    return segment<3, ScalarT>(segment_line[interval.value().start], segment_line[interval.value().end]);
}

// segment3 - capsule3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, capsule<3, ScalarT> const& capsule)
{
    return detail::intersection_segment_object_impl(segment, capsule);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(capsule<3, ScalarT> const& capsule, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_object_impl(segment, capsule);
}

// segment3 - cylinder3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, cylinder<3, ScalarT> const& cylinder)
{
    return detail::intersection_segment_object_impl(segment, cylinder);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(cylinder<3, ScalarT> const& cylinder, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_object_impl(segment, cylinder);
}

// segment3 - ellipse3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, ellipse<3, ScalarT> const& ellipse)
{
    return detail::intersection_segment_object_impl(segment, ellipse);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(ellipse<3, ScalarT> const& ellipse, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_object_impl(segment, ellipse);
}

// segment3 - sphere3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, sphere<3, ScalarT> const& sphere)
{
    return detail::intersection_segment_object_impl(segment, sphere);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(sphere<3, ScalarT> const& sphere, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_object_impl(segment, sphere);
}

// segment3 - cone3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, cone<3, ScalarT> const& cone)
{
    return detail::intersection_segment_object_impl(segment, cone);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(cone<3, ScalarT> const& cone, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_object_impl(segment, cone);
}

// segment3 - tube3
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, tube<3, ScalarT> const& tube)
{
    return detail::intersection_segment_boundary_impl(segment, tube);
}

template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(tube<3, ScalarT> const& tube, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_boundary_impl(segment, tube);
}

// segment3 - cylinder_boundary
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, cylinder_boundary<3, ScalarT> const& cylinder)
{
    return detail::intersection_segment_boundary_impl(segment, cylinder);
}

template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(cylinder_boundary<3, ScalarT> const& cylinder, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_boundary_impl(segment, cylinder);
}

// segment3 - box_boundary3
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, box_boundary<3, ScalarT> const& box)
{
    return detail::intersection_segment_boundary_impl(segment, box);
}

template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(box_boundary<3, ScalarT> const& box, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_boundary_impl(segment, box);
}

// segment3 - capsule_boundary3
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, capsule_boundary<3, ScalarT> const& capsule)
{
    return detail::intersection_segment_boundary_impl(segment, capsule);
}

template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersection(capsule_boundary<3, ScalarT> const& capsule, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_boundary_impl(segment, capsule);
}

// segment3 - cone_boundary3
template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersecion(segment<3, ScalarT> const& segment, cone_boundary<3, ScalarT> const& cone)
{
    return detail::intersection_segment_boundary_impl(segment, cone);
}

template <class ScalarT>
[[nodiscard]] constexpr hits<2, pos<3, ScalarT>> intersecion(cone_boundary<3, ScalarT> const& cone, segment<3, ScalarT> const& segment)
{
    return detail::intersection_segment_boundary_impl(segment, cone);
}


template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box<3, ScalarT> const& a, box<3, ScalarT> const& b)
{
    if (a.center == b.center)
        return true;

    // Separating Axes Theorem
    auto const axis_check = [](dir<3, ScalarT> d, box<3, ScalarT> const& box_a, box<3, ScalarT> const& box_b) -> bool
    {
        ScalarT minProja = tg::max<ScalarT>();
        ScalarT maxProja = tg::min<ScalarT>();
        ScalarT minProjb = tg::max<ScalarT>();
        ScalarT maxProjb = tg::min<ScalarT>();

        for (auto x : {-1, 1})
            for (auto y : {-1, 1})
                for (auto z : {-1, 1})
                {
                    vec3 v = {float(x), float(y), float(z)};

                    auto proja = dot(d, box_a.half_extents * v);
                    if (proja < minProja)
                        minProja = proja;
                    if (proja > maxProja)
                        maxProja = proja;

                    auto projb = dot(d, box_b.half_extents * v);
                    if (projb < minProjb)
                        minProjb = projb;
                    if (projb > maxProjb)
                        maxProjb = projb;
                }

        if (maxProja < minProjb || minProja > maxProjb)
            return true; // extents of boxes projected onto axis are overlapping -> intersection)

        return false;
    };

    dir<3, ScalarT> ax = normalize(a.half_extents[0]);
    dir<3, ScalarT> ay = normalize(a.half_extents[1]);
    dir<3, ScalarT> az = normalize(a.half_extents[2]);

    dir<3, ScalarT> bx = normalize(b.half_extents[0]);
    dir<3, ScalarT> by = normalize(b.half_extents[1]);
    dir<3, ScalarT> bz = normalize(b.half_extents[2]);

    auto cross_ax_bx = cross(ax, bx);
    auto cross_ax_by = cross(ax, by);
    auto cross_ax_bz = cross(ax, bz);

    auto cross_ay_bx = cross(ay, bx);
    auto cross_ay_by = cross(ay, by);
    auto cross_ay_bz = cross(ay, bz);

    auto cross_az_bx = cross(az, bx);
    auto cross_az_by = cross(az, by);
    auto cross_az_bz = cross(az, bz);

    // ax
    if (axis_check(ax, a, b))
        return true;

    // ay
    if (axis_check(ay, a, b))
        return true;

    // az
    if (axis_check(az, a, b))
        return true;

    // bx
    if (axis_check(bx, a, b))
        return true;

    // by
    if (axis_check(by, a, b))
        return true;

    // bz
    if (axis_check(bz, a, b))
        return true;

    // ax_bx
    if (length_sqr(cross_ax_bx) > 0 ? axis_check(normalize(cross_ax_bx), a, b) : false)
        return true;

    // ax_by
    if (length_sqr(cross_ax_by) > 0 ? axis_check(normalize(cross_ax_by), a, b) : false)
        return true;

    // ax_bz
    if (length_sqr(cross_ax_bz) > 0 ? axis_check(normalize(cross_ax_bz), a, b) : false)
        return true;

    // ay_by
    if (length_sqr(cross_ay_bx) > 0 ? axis_check(normalize(cross_ay_bx), a, b) : false)
        return true;

    // ay_by
    if (length_sqr(cross_ay_by) > 0 ? axis_check(normalize(cross_ay_by), a, b) : false)
        return true;

    // ay_bz
    if (length_sqr(cross_ay_bz) > 0 ? axis_check(normalize(cross_ay_bz), a, b) : false)
        return true;

    // az_bx
    if (length_sqr(cross_az_bx) > 0 ? axis_check(normalize(cross_az_bx), a, b) : false)
        return true;

    // az_by
    if (length_sqr(cross_az_by) > 0 ? axis_check(normalize(cross_az_by), a, b) : false)
        return true;

    // az_bz
    if (length_sqr(cross_az_bz) > 0 ? axis_check(normalize(cross_az_bz), a, b) : false)
        return true;

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<3, ScalarT> const& s0, sphere<3, ScalarT> const& s1)
{
    return distance(s0.center, s1.center) <= (s0.radius + s1.radius);
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box<3, ScalarT> const& box, sphere<3, ScalarT> const& sphere)
{
    // early-out: sphere-center inside box
    if (contains(box, sphere.center))
        return true;

    array<pos<3, ScalarT>, 8> box_vertices = vertices_of(box);
    // box vertex inside the sphere
    for (auto const& v : box_vertices)
    {
        if (length_sqr(v - sphere.center) < pow2(sphere.radius))
            return true;
    }

    array<segment<3, ScalarT>, 12> box_edges = edges_of(box);
    // box edge intersects sphere
    for (auto const& e : box_edges)
    {
        if (intersects(e, sphere))
            return true;
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<3, ScalarT> const& sphere, box<3, ScalarT> const& box)
{
    return intersects(box, sphere);
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(plane<3, ScalarT> const& plane, sphere<3, ScalarT> const& sphere)
{
    if (distance(plane, sphere.center) <= sphere.radius)
        return true;

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<3, ScalarT> const& sphere, plane<3, ScalarT> const& plane)
{
    return intersects(plane, sphere);
}

// box3 -plane3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box<3, ScalarT> const& box, plane<3, ScalarT> const& plane)
{
    float sign = 0;

    // iterate over box-vertices
    for (auto x : {-1, 1})
        for (auto y : {-1, 1})
            for (auto z : {-1, 1})
            {
                pos<3, ScalarT> box_vertex = box.center + x * box.half_extents[0] + y * box.half_extents[1] + z * box.half_extents[2];
                if (sign == 0)
                {
                    sign = dot(plane.normal, box_vertex) - plane.dis;

                    if (sign == 0)
                        return true;

                    continue;
                }

                if ((dot(plane.normal, box_vertex) - plane.dis) * sign <= 0) // box vertices on different sides of the plane
                    return true;
            }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(plane<3, ScalarT> const& plane, box<3, ScalarT> const& box)
{
    return intersects(box, plane);
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box<3, ScalarT> const& box, triangle<3, ScalarT> const& triangle)
{
    // early out
    if (contains(box, triangle))
        return true;

    // intersection of box with triangle-plane
    plane<3, ScalarT> plane_of_triangle = plane_of(triangle);
    if (!intersects(plane_of_triangle, box))
        return false;

    array<segment<3, ScalarT>, 3> edges_triangle = edges_of(triangle);
    array<segment<3, ScalarT>, 12> edges_box = edges_of(box);

    // intersection of triangle edges with box
    for (auto& e : edges_triangle)
    {
        if (intersects(e, box))
            return true;
    }

    // intersection of box edges with triangle
    for (auto& e : edges_box)
    {
        if (intersects(e, triangle))
            return true;
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& triangle, box<3, ScalarT> const& box)
{
    return intersects(box, triangle);
}

// box2 - sphere2
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box<2, ScalarT> const& box, sphere<2, ScalarT> const& sphere)
{
    array<segment<2, ScalarT>, 4> edges_box = edges_of(box);

    if (contains(box, sphere.center))
        return true;

    for (auto const& e : edges_box)
    {
        if (intersects(e, sphere))
            return true;
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<2, ScalarT> const& sphere, box<2, ScalarT> const& box)
{
    return intersects(box, sphere);
}

template <int D, class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<D, ScalarT> const& sphere, halfspace<D, ScalarT> const& halfspace)
{
    if (dot(halfspace.normal, sphere.center) - halfspace.dis <= sphere.radius)
        return true;

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(halfspace<3, ScalarT> const& halfspace, sphere<3, ScalarT> const& sphere)
{
    return intersects(sphere, halfspace);
}

// TODO: optimized version
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(box<3, ScalarT> const& box, halfspace<3, ScalarT> const& halfspace)
{
    array<pos<3, ScalarT>, 8> vertices_box = vertices_of(box);
    for (auto const& v : vertices_box)
    {
        if (dot(halfspace.normal, v) - halfspace.dis <= 0)
            return true;
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(halfspace<3, ScalarT> const& halfspace, box<3, ScalarT> const& box)
{
    return intersects(box, halfspace);
}

// segment3 - halfspace3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(segment<3, ScalarT> const& segment, halfspace<3, ScalarT> const& halfspace)
{
    if ((dot(halfspace.normal, segment.pos0) - halfspace.dis) <= 0 || (dot(halfspace.normal, segment.pos1) - halfspace.dis) <= 0)
        return true;

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(halfspace<3, ScalarT> const& halfspace, segment<3, ScalarT> const& segment)
{
    return intersects(segment, halfspace);
}

// triangle3 - sphere3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& triangle, sphere<3, ScalarT> const& sphere)
{
    // triangle vertex inside sphere
    if (contains(sphere, triangle.pos0) || contains(sphere, triangle.pos1) || contains(sphere, triangle.pos2))
        return true;

    plane<3, ScalarT> plane_t = plane_of(triangle);

    // check if the closest point on triangle to sphere center is inside the sphere
    auto cp = closest_points(sphere.center, triangle);

    if (contains(sphere, cp.first) && contains(sphere, cp.second))
        return true;

    // triangle edge intersects sphere
    for (auto const& e : edges_of(triangle))
    {
        if (intersects(e, sphere))
            return true;
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(sphere<3, ScalarT> const& sphere, triangle<3, ScalarT> const& triangle)
{
    return intersects(triangle, sphere);
}

// disk3 - plane3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(disk<3, ScalarT> const& disk, plane<3, ScalarT> const& pl)
{
    auto plane_s = plane<3, ScalarT>(disk.normal, disk.center);

    // sphere center on plane
    if (contains(pl, disk.center))
        return true;

    // no intersection if planes are parallel
    if ((plane_s.normal == pl.normal || plane_s.normal == -pl.normal) && !contains(pl, disk.center))
        return false;

    // line intersection of two planes
    auto insec = intersection(plane_s, pl);

    // if distance of plane intersection is inside the sphere, intersection exists
    if (distance_sqr(insec, disk.center) <= pow2(disk.radius))
        return true;

    return false;
}


template <class ScalarT>
[[nodiscard]] constexpr bool intersects(plane<3, ScalarT> const& plane, disk<3, ScalarT> const& disk)
{
    return intersects(disk, plane);
}

// plane3 - cone3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(plane<3, ScalarT> const& plane, cone<3, ScalarT> const& cone)
{
    // cone base intersects the plane
    if (intersects(cone.base, plane))
        return true;

    auto d_cone_tip = (dot(plane.normal, apex_of(cone)) - plane.dis) >= 0;
    auto d_cone_base = (dot(plane.normal, cone.base.center) - plane.dis) >= 0;

    // base and tip of the cone are on different sides of the plane
    if (d_cone_tip != d_cone_base)
        return true;

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(cone<3, ScalarT> const& cone, plane<3, ScalarT> const& plane)
{
    return intersects(plane, cone);
}

// triangle3 - halfspace3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& triangle, halfspace<3, ScalarT> const halfspace)
{
    if ((dot(halfspace.normal, triangle.pos0) - halfspace.dis <= 0) || (dot(halfspace.normal, triangle.pos1) - halfspace.dis <= 0)
        || (dot(halfspace.normal, triangle.pos2) - halfspace.dis <= 0))
        return true;

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(halfspace<3, ScalarT> const halfspace, triangle<3, ScalarT> const& triangle)
{
    return intersects(triangle, halfspace);
}

// disk3 - triangle3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(disk<3, ScalarT> const& disk, triangle<3, ScalarT> const& triangle)
{
    // circle inside triangle or triangle vertex inside the disk
    if (contains(disk, centroid_of(triangle)) || contains(disk, triangle.pos0) || contains(disk, triangle.pos1) || contains(disk, triangle.pos2))
        return true;

    // area of triangle intersects with disk
    auto cp = closest_points(disk.center, triangle);

    if (contains(disk, cp.first) && contains(disk, cp.second))
        return true;

    // triangle edge intersects with disk
    for (auto const& e : edges_of(triangle))
    {
        if (intersects(e, disk))
            return true;
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& triangle, disk<3, ScalarT> const& disk)
{
    return intersects(disk, triangle);
}

// cone3 - triangle3
template <class ScalarT>
[[nodiscard]] constexpr bool intersects(cone<3, ScalarT> const& cone, triangle<3, ScalarT> const& triangle)
{
    auto mid_axis = segment<3, ScalarT>(cone.base.center, apex_of(cone));

    // area of the triangle intersects with cone
    if (intersects(mid_axis, triangle))
        return true;

    // triangle intersects with the cone base
    if (intersects(cone.base, triangle))
        return true;

    // at least one segment of triangle intersects with cone
    for (auto const& e : edges_of(triangle))
    {
        if (intersects(e, cone))
            return true;
    }

    return false;
}

template <class ScalarT>
[[nodiscard]] constexpr bool intersects(triangle<3, ScalarT> const& triangle, cone<3, ScalarT> const& cone)
{
    return intersects(cone, triangle);
}

// segment3 - halfspace3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& seg, halfspace<3, ScalarT> const& halfspace)
{
    bool cont_pos0 = contains(halfspace, seg.pos0);
    bool cont_pos1 = contains(halfspace, seg.pos1);

    // both segment points inside the halfspace
    if (cont_pos0 && cont_pos1)
        return seg;

    // check if there is an intersection with the plane of the halfspace
    auto insec = intersection(seg, plane_of(halfspace));

    if (!insec.has_value())
        return {};

    if (cont_pos0)
        return segment<3, ScalarT>{seg.pos0, insec.value()};

    if (cont_pos1)
        return segment<3, ScalarT>{insec.value(), seg.pos1};

    return {};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(halfspace<3, ScalarT> const& halfspace, segment<3, ScalarT> const& segment)
{
    return intersection(segment, halfspace);
}

// segment3 - disk3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<pos<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, disk<3, ScalarT> const& disk)
{
    auto plane_disk = plane<3, ScalarT>(disk.normal, disk.center);
    // check if both seg points lie on same side of the circle
    if (!intersects(segment, plane_disk))
        return {};

    // seg points on different sides of the circle
    auto insec = intersection(segment, plane_disk);
    if (!insec.has_value())
        return {};

    if (distance_sqr(insec.value(), disk.center) <= pow2(disk.radius))
        return insec.value();

    return {};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<pos<3, ScalarT>> intersection(disk<3, ScalarT> const& disk, segment<3, ScalarT> const& segment)
{
    return intersection(segment, disk);
}

// segment3 - hemisphere3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(segment<3, ScalarT> const& segment, hemisphere<3, ScalarT> const& hemisphere)
{
    // early-out: both seg points inside hemisphere
    if (contains(hemisphere, segment.pos0) && contains(hemisphere, segment.pos1))
        return segment;

    // sphere extension of hemisphere
    auto sp = sphere<3, ScalarT>(hemisphere.center, hemisphere.radius);

    // intersection with sphere extension
    auto insec_sp = intersection(segment, sp);

    if (!insec_sp.has_value())
        return {};

    // halfspace spanned by hemisphere base containing the hemisphere
    auto halfspace_hs = halfspace<3, ScalarT>(-hemisphere.normal, hemisphere.center);

    // intersection with halfspace
    auto insec_hs = intersection(insec_sp.value(), halfspace_hs);

    if (insec_hs.has_value())
        return insec_hs;

    else
        return {};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(hemisphere<3, ScalarT> const& halfspace, segment<3, ScalarT> const& segment)
{
    return intersection(segment, halfspace);
}

// sphere3 - plane3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<disk<3, ScalarT>> intersection(sphere<3, ScalarT> const& sphere, plane<3, ScalarT> const& plane)
{
    // project sphere center onto plane
    pos<3, ScalarT> disk_center = project(sphere.center, plane);

    auto dist_sqr = sphere.radius * sphere.radius - distance_sqr(sphere.center, disk_center);

    if (dist_sqr < 0) // no intersection
        return {};

    // pythagoras
    auto rad = sqrt(sphere.radius * sphere.radius - distance_sqr(sphere.center, disk_center));

    return disk<3, ScalarT>(disk_center, rad, plane.normal);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<sphere<2, ScalarT, 3>> intersection(plane<3, ScalarT> const& plane, sphere<3, ScalarT> const& sphere)
{
    return intersection(sphere, plane);
}

// plane3 - inf_cylinder3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<ellipse<2, ScalarT, 3>> intersection(plane<3, ScalarT> const& plane, inf_cylinder<3, ScalarT> const& cylinder)
{
    // early-out: plane normal and cylinder axis are orthogonal
    if (dot(plane.normal, cylinder.axis.dir) == 0)
        return {};

    // ellipse mid_point = intersection cylinder axis and plane
    auto insec_mid_axis = intersection(cylinder.axis, plane);
    auto mid_point = insec_mid_axis.first();

    // find semi axes
    // vector orthogonal to cylinder axis and plane normal
    vec<3, ScalarT> orth_vec1 = cross(cylinder.axis.dir, plane.normal);

    if (is_zero_vector(orth_vec1))
    {
        // return will be a disk (i.e. ellipse with identical sized semi axes)
        auto semi_vec1 = any_normal(cylinder.axis.dir) * cylinder.radius;
        auto semi_vec2 = cylinder.radius * normalize(cross(semi_vec1, cylinder.axis.dir));
        auto semi_axes = mat<2, 3, ScalarT>::from_cols(semi_vec1, semi_vec2);

        return ellipse<2, ScalarT, 3>(mid_point, semi_axes);
    }

    // first semi-axis
    vec<3, ScalarT> semi_vec1 = cylinder.radius * normalize(orth_vec1);

    // vector orthogonal to plane_normal and orthogonal to orth_vec1
    vec<3, ScalarT> orth_vec2 = cross(orth_vec1, plane.normal);

    // second semi-axis
    vec<3, ScalarT> semi_vec2 = cylinder.radius / (length(cross(cylinder.axis.dir, plane.normal))) * normalize(orth_vec2);

    auto semi_axes = mat<2, 3, ScalarT>::from_cols(semi_vec1, semi_vec2);

    return ellipse<2, ScalarT, 3>(mid_point, semi_axes);
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<ellipse<2, ScalarT, 3>> intersection(inf_cylinder<3, ScalarT> const& t, plane<3, ScalarT> const& p)
{
    return intersection(p, t);
}

// disk3 - plane3
template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(disk<3, ScalarT> const& disk, plane<3, ScalarT> const& plane)
{
    if (is_zero_vector(cross(disk.normal, plane.normal)))
        return {};

    auto disk_plane = plane_of(disk);

    // intersection of circle_plane and plane results in line parallel to intersection of disk and plane
    line<3, ScalarT> insec_line = intersection(plane, disk_plane);
    auto sphere_disk = sphere<3, ScalarT>(disk.center, disk.radius);

    auto insec_sphere = intersection_parameter(insec_line, boundary_of(sphere_disk));

    if (insec_sphere.size() < 1)
        return {};

    if (insec_sphere.size() == 1)
        return segment<3, ScalarT>{insec_line[insec_sphere[0]], insec_line[insec_sphere[0]]};

    return segment<3, ScalarT>{insec_line[insec_sphere[0]], insec_line[insec_sphere[1]]};
}

template <class ScalarT>
[[nodiscard]] constexpr cc::optional<segment<3, ScalarT>> intersection(plane<3, ScalarT> const& plane, disk<3, ScalarT> const& disk)
{
    return intersection(disk, plane);
}
} // namespace tg
