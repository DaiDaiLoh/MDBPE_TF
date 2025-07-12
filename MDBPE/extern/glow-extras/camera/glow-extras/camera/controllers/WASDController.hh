#pragma once

#include "../SmoothedCamera.hh"

namespace glow
{
namespace camera
{
/**
 * Controls a free, flying camera
 * WASD - Forward/Backward, Left/Right
 * QE - Up/Down
 * Hold Shift - Increase Speed
 */
class WASDController
{
private:
    float mCameraMoveSpeed = 30.f;
    float mShiftSpeedMultiplier = 4.f;

public:
    void update(float dt, glow::input::InputState const& input, SmoothedCamera& camera) const;

    void setCameraSpeed(float speed) { mCameraMoveSpeed = speed; }
    void setShiftSpeedMultiplier(float mult) { mShiftSpeedMultiplier = mult; }
};
}
}
