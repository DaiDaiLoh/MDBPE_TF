#include "LookAroundController.hh"

#include <GLFW/glfw3.h>

void glow::camera::LookAroundController::update(float, const glow::input::InputState& input, glow::camera::SmoothedCamera& camera) const
{
    if (input.isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        float dX = input.getMouseDeltaF().x / camera.getViewportWidth() * mCameraTurnSpeed;
        float dY = input.getMouseDeltaF().y / camera.getViewportHeight() * mCameraTurnSpeed;

        camera.handle.lookAround(dX, dY);
    }
}
