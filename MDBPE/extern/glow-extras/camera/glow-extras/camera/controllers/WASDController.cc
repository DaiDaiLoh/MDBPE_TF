#include "WASDController.hh"

#include <GLFW/glfw3.h>

void glow::camera::WASDController::update(float dt, const glow::input::InputState& input, glow::camera::SmoothedCamera& camera) const
{
    auto distance = mCameraMoveSpeed * dt;

    if (input.isKeyDown(GLFW_KEY_LEFT_SHIFT))
        distance *= mShiftSpeedMultiplier;

    tg::vec3 distanceInput = tg::vec3(0);

    if (input.isKeyDown(GLFW_KEY_S))
        ++distanceInput.z;
    if (input.isKeyDown(GLFW_KEY_W))
        --distanceInput.z;
    if (input.isKeyDown(GLFW_KEY_D))
        ++distanceInput.x;
    if (input.isKeyDown(GLFW_KEY_A))
        --distanceInput.x;
    if (input.isKeyDown(GLFW_KEY_E))
        ++distanceInput.y;
    if (input.isKeyDown(GLFW_KEY_Q))
        --distanceInput.y;

    if (distanceInput != tg::vec3(0))
        camera.handle.move(distanceInput * distance);
}
