#include "TargetOrbitController.hh"

#include <glow/common/log.hh>

#include <GLFW/glfw3.h>

void glow::camera::TargetOrbitController::update(float, const glow::input::InputState& input, glow::camera::SmoothedCamera& camera) const
{
    if (input.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        TG_ASSERT(camera.getViewportWidth() > 0 && "did you forget to resize camera in onResize?");
        TG_ASSERT(camera.getViewportHeight() > 0 && "did you forget to resize camera in onResize?");

        float dX = input.getMouseDeltaF().x / camera.getViewportWidth() * mCameraOrbitSpeed;
        float dY = input.getMouseDeltaF().y / camera.getViewportHeight() * mCameraOrbitSpeed;

        if (std::abs(dX) + std::abs(dY) > .0005f)
            camera.handle.orbit(dX, dY);
    }

    auto const& scrollY = input.getScrollDelta().y;
    if (std::abs(scrollY) > 0.f)
    {
        camera.handle.addTargetDistance(-scrollY * mCameraScrollingSpeed);
    }
}
