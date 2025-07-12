#pragma once

#include <vector>

#include <typed-geometry/tg-lean.hh>

#include <glow-extras/viewer/CameraController.hh>
#include <glow/common/property.hh>
#include <glow/common/shared.hh>

#include "aabb.hh"
#include "SceneConfig.hh"

namespace glow::viewer
{
GLOW_SHARED(class, Renderable);

class Scene
{
public:
    struct BoundingInfo
    {
        float diagonal = 1.f;
        tg::pos3 center = tg::pos3::zero;
        tg::aabb3 aabb = tg::aabb3::unit_centered;
    };

    // members
private:
    std::vector<SharedRenderable> mRenderables;

    /// computes the AABB of the scene
    aabb computeAabb() const;

    mutable cc::optional<size_t> mCachedHash;

    // properties
public:
    GLOW_GETTER(Renderables);

    SceneConfig config;

    bool enableScreenshotDebug = false;

    tg::mat4 viewMatrix;
    tg::mat4 projMatrix;

    Scene() = default;

    size_t queryHash() const;

    // methods
public:
    /// adds a renderable
    void add(SharedRenderable const& r);

    BoundingInfo getBoundingInfo() const;

    bool has_3d_renderables() const;

    bool shouldBeCleared() const;
};
}
