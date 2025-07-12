#pragma once

#include <glow/math/transform.hh>

#include <glow-extras/input/InputState.hh>

#include "CameraBase.hh"
#include "Handle.hh"
#include "Lens.hh"

namespace glow
{
namespace camera
{
/**
 * The Smoothed Camera
 *
 * Consists of
 *  - the Lens (Projection)
 *  - the Handle (Positioning, View Matrix)
 *
 */
GLOW_SHARED(class, SmoothedCamera);
class SmoothedCamera : public CameraBase
{
public:
    Lens lens;
    Handle handle;

public:
    SmoothedCamera() = default;
    GLOW_SHARED_CREATOR(SmoothedCamera);

public:
    // -- CameraBase interface --
    tg::pos3 getPosition() const override { return handle.getPosition(); }
    tg::mat4 getViewMatrix() const override { return handle.getViewMatrix(); }
    tg::mat4 getProjectionMatrix() const override { return lens.getProjectionMatrix(); }
    tg::isize2 getViewportSize() const override { return lens.getViewportSize(); }
    float getNearClippingPlane() const override { return lens.getNearPlane(); }
    float getFarClippingPlane() const override { return lens.getFarPlane(); }
    tg::angle getVerticalFieldOfView() const override { return lens.getVerticalFov(); }
    tg::horizontal_fov getHorizontalFieldOfView() const override { return lens.getHorizontalFov(); }

public:
    // -- Handle pass-through --
    glow::transform const& transform() const { return handle.getTransform(); }
    void setPosition(tg::pos3 const& pos) { handle.setPosition(pos); }
    void setLookAt(tg::pos3 const& pos, tg::pos3 const& target) { handle.setLookAt(pos, target); }
    float getLookAtDistance() const { return handle.getLookAtDistance(); }

    // -- Lens pass-through --
    void setNearPlane(float nearPlane) { lens.setNearPlane(nearPlane); }
    void setFarPlane(float farPlane) { lens.setFarPlane(farPlane); }
    void setFoV(tg::horizontal_fov fov) { lens.setFoV(fov); }
    void setViewportSize(int w, int h) { lens.setViewportSize(w, h); }
    int getViewportWidth() const { return lens.getViewportSize().width; }
    int getViewportHeight() const { return lens.getViewportSize().height; }

public:
    // -- Controller update --
    template <class... Controllers>
    void update(float dt, glow::input::InputState const& input, Controllers&... controllers)
    {
        applyControllers(dt, input, controllers...);
        handle.update(dt);
    }

private:
    template <class Controller, class... Controllers>
    void applyControllers(float dt, glow::input::InputState const& input, Controller& controller, Controllers&... controllers)
    {
        controller.update(dt, input, *this);
        applyControllers(dt, input, controllers...);
    }

    void applyControllers(float, glow::input::InputState const&)
    {
        // noop, recursion end
    }
};
}
}
