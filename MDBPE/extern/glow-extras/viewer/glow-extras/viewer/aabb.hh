#pragma once

#include <initializer_list>
#include <sstream>
#include <string>

#include <typed-geometry/tg-std.hh>

#include <glow/common/log.hh>

namespace glow
{
namespace viewer
{
struct aabb
{
    // member
public:
    tg::pos3 min = tg::pos3(std::numeric_limits<tg::f32>::max());
    tg::pos3 max = tg::pos3(std::numeric_limits<tg::f32>::lowest());

    // properties
public:
    constexpr tg::vec3 size() const;
    constexpr tg::pos3 center() const;
    constexpr float radius() const; ///< Radius of the bounding sphere (positioned at the center)
    constexpr float volume() const;
    constexpr bool isEmpty() const; ///< NOT the same as zero volume! 2D aabb is not empty.

    // mutating methods
public:
    /// includes a given point
    constexpr void add(tg::pos3 const& p);
    /// includes a given aabb
    constexpr void add(aabb const& rhs);

    // non-mutating
public:
    /// calculates a transformed aabb
    constexpr aabb transformed(tg::mat4 const& transform) const;

    // ctor
public:
    aabb() = default;
    constexpr aabb(tg::pos3 min, tg::pos3 max) : min(min), max(max) {}

    static aabb empty() { return aabb(); }
};

// ======== IMPLEMENTATION ========

constexpr inline tg::vec3 aabb::size() const { return max - min; }

constexpr inline tg::pos3 aabb::center() const { return tg::centroid_of(tg::aabb3(min, max)); }

constexpr inline float aabb::radius() const { return tg::length(size()) * .5f; }

constexpr inline float aabb::volume() const
{
    if (isEmpty())
        return 0;

    auto s = size();
    return s.x * s.y * s.z;
}

constexpr inline bool aabb::isEmpty() const { return max.x < min.x || max.y < min.y || max.z < min.z; }

constexpr inline void aabb::add(const tg::pos3& p)
{
    min = tg::min(min, p);
    max = tg::max(max, p);
}

constexpr inline void aabb::add(const aabb& rhs)
{
    if (!rhs.isEmpty())
    {
        add(rhs.min);
        add(rhs.max);
    }
}

constexpr inline aabb aabb::transformed(const tg::mat4& transform) const
{
    aabb r;
    for (auto dz : {0.f, 1.f})
        for (auto dy : {0.f, 1.f})
            for (auto dx : {0.f, 1.f})
            {
                auto p = tg::comp3(min) + tg::comp3(max - min) * tg::comp3(dx, dy, dz);
                r.add(tg::pos3(transform * tg::vec4(p.comp0, p.comp1, p.comp2, 1.0f)));
            }
    return r;
}

inline std::string to_string(aabb const& aabb)
{
    std::stringstream ss;
    ss << "aabb[" << aabb.min << ", " << aabb.max << "]";
    return ss.str();
}
}
}
