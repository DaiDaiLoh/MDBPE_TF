#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

#include <typed-geometry/tg.hh>

namespace glow
{
namespace pipeline
{
inline tg::vec3 componentWiseMin(tg::vec3 const& a, tg::vec3 const& b)
{
    return tg::vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}

inline tg::vec3 componentWiseMax(tg::vec3 const& a, tg::vec3 const& b)
{
    return tg::vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}

// -- Projection --

inline tg::vec3 ndcToWorld(tg::pos3 const& ndc, tg::mat4 const& projInverse, tg::mat4 const& viewInverse)
{
    auto viewPosition = projInverse * tg::vec4(ndc, 1);
    viewPosition /= viewPosition.w;
    return tg::vec3(viewInverse * viewPosition);
}

inline tg::vec3 ndcToView(tg::pos3 const& ndc, tg::mat4 const& projInverse)
{
    auto viewPosition = projInverse * tg::vec4(ndc, 1);
    return (tg::vec3(viewPosition) / viewPosition.w);
}


// ---- Screen Space Helpers ---

struct ScreenRectangle
{
    tg::isize2 min;
    tg::isize2 max;

    static void updateClipRegionRoot(float nc, // Tangent plane x/y normal coordinate (view space)
                                     float lc, // Light x/y coordinate (view space)
                                     float lz, // Light z coordinate (view space)
                                     float lightRadius,
                                     float cameraScale, // Project scale for coordinate (_11 or _22 for x/y respectively)
                                     float& clipMin,
                                     float& clipMax)
    {
        float const nz = (lightRadius - nc * lc) / lz;
        float const pz = (lc * lc + lz * lz - lightRadius * lightRadius) / (lz - (nz / nc) * lc);

        if (pz < 0.0f)
        {
            float c = -nz * cameraScale / nc;
            if (nc < 0.0f)
            {
                // Left side boundary
                clipMin = std::max(clipMin, c);
            }
            else
            { // Right side boundary
                clipMax = std::min(clipMax, c);
            }
        }
    }

    static void updateClipRegion(float lc, // Light x/y coordinate (view space)
                                 float lz, // Light z coordinate (view space)
                                 float lightRadius,
                                 float cameraScale, // Project scale for coordinate (_11 or _22 for x/y respectively)
                                 float& clipMin,
                                 float& clipMax)
    {
        float const rSq = lightRadius * lightRadius;
        float const lcSqPluslzSq = lc * lc + lz * lz;
        float const d = rSq * lc * lc - lcSqPluslzSq * (rSq - lz * lz);

        if (d >= 0.0f)
        {
            float const a = lightRadius * lc;
            float const b = sqrt(d);
            float const nx0 = (a + b) / lcSqPluslzSq;
            float const nx1 = (a - b) / lcSqPluslzSq;

            updateClipRegionRoot(nx0, lc, lz, lightRadius, cameraScale, clipMin, clipMax);
            updateClipRegionRoot(nx1, lc, lz, lightRadius, cameraScale, clipMin, clipMax);
        }
    }


    static glm::vec4 ComputeClipRegion(glm::mat4 const& proj, glm::vec3 const& position, float radius, float nearPlane)
    {
        if (position.z - radius <= -nearPlane)
        {
            glm::vec2 clipMin = {-1.0f, -1.0f};
            glm::vec2 clipMax = {1.0f, 1.0f};

            updateClipRegion(position.x, position.z, radius, proj[0][0], clipMin.x, clipMax.x);
            updateClipRegion(position.y, position.z, radius, proj[1][1], clipMin.y, clipMax.y);

            return glm::vec4(clipMin, clipMax);
        }
        else
        {
            // Empty rectangle if the light is too far behind the view frustum
            return {1.0f, 1.0f, -1.0f, -1.0f};
        }
    }

    static ScreenRectangle Find(glm::mat4 const& proj, glm::vec3 const& position, float radius, unsigned width, unsigned height, float nearPlane)
    {
        ScreenRectangle res;
        glm::vec4 clipRegion = ComputeClipRegion(proj, position, radius, nearPlane);
        clipRegion = -clipRegion;

        std::swap(clipRegion.x, clipRegion.z);
        std::swap(clipRegion.y, clipRegion.w);
        clipRegion *= 0.5f;
        clipRegion += 0.5f;

        clipRegion = glm::clamp(clipRegion, {0, 0, 0, 0}, {1.f, 1.f, 1.f, 1.f});

        res.min.x = static_cast<unsigned>(clipRegion.x * width);
        res.min.y = static_cast<unsigned>(clipRegion.y * height);
        res.max.x = static_cast<unsigned>(clipRegion.z * width);
        res.max.y = static_cast<unsigned>(clipRegion.w * height);
        return res;
    }
};

// -- Collision helpers --

struct BoundingSphere
{
    glm::vec3 center;
    float radius;

    BoundingSphere(glm::vec3 const& c, float r) : center(c), radius(r) {}

    template <typename Iterator> // TODO: Restrict to iterator of container that contains glm::vec3
    BoundingSphere(Iterator begin, Iterator const end)
    {
        center = glm::vec3(0);
        for (auto it = begin; it != end; ++it)
        {
            center += *it;
        }
        center *= (1.f / 8);

        radius = 0;
        for (; begin != end; ++begin)
        {
            float distanceToCenter = distance(*begin, center);
            if (distanceToCenter > radius)
            {
                radius = distanceToCenter;
            }
        }
    }

    BoundingSphere(std::vector<glm::vec3> const& points) : BoundingSphere(points.begin(), points.end()) {}

    bool intersects(BoundingSphere const& b) const { return distance(center, b.center) < radius + b.radius; }

    static bool Intersection(BoundingSphere const& lhs, BoundingSphere const& rhs) { return lhs.intersects(rhs); }

    static BoundingSphere FromTube(glm::vec3 const& tubeA, glm::vec3 const& tubeB, float tubeRadius)
    {
        float radius = tubeRadius + (distance(tubeA, tubeB) * 0.5f);
        glm::vec3 center = (tubeA + tubeB) * 0.5f;
        return BoundingSphere{center, radius};
    }
};

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
    unsigned index;

    AABB() = default;

    template <typename Iterator> // TODO: Restrict to iterator of container that contains glm::vec3
    AABB(Iterator begin, Iterator const end)
    {
        min = glm::vec3(std::numeric_limits<float>::max());
        max = glm::vec3(std::numeric_limits<float>::min());

        for (; begin != end; ++begin)
        {
            min = componentWiseMin(min, *begin);
            max = componentWiseMax(max, *begin);
        }
    }

    AABB(std::vector<glm::vec3> const& points) : AABB(points.begin(), points.end()) {}

    bool intersects(AABB const& b) const
    {
        if (min.x > b.max.x || min.y > b.max.y || min.z > b.max.z || max.x < b.min.x || max.y < b.min.y || max.z < b.min.z)
        {
            return false;
        }
        return true;
    }

    static bool Intersection(AABB const& lhs, AABB const& rhs) { return lhs.intersects(rhs); }

    static AABB FromTube(glm::vec3 const& tubeA, glm::vec3 const& tubeB, float tubeRadius)
    {
        AABB res{{tubeA, tubeB}};
        res.min -= glm::vec3(tubeRadius);
        res.max += glm::vec3(tubeRadius);
        return res;
    }
};
}
}
