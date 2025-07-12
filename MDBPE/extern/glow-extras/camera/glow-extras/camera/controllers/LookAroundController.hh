#pragma once

#include "../SmoothedCamera.hh"

namespace glow
{
namespace camera
{
/**
 * Lets the camera look around while standing still, FPS-style
 * Hold RMB - Look around
 */
class LookAroundController
{
private:
    float mCameraTurnSpeed = 5.f;

public:
    void update(float, glow::input::InputState const& input, SmoothedCamera& camera) const;

    void setTurnSpeed(float speed) { mCameraTurnSpeed = speed; }
};
}
}
