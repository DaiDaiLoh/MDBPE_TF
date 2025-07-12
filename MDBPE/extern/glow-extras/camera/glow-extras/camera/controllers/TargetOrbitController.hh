#pragma once

#include "../SmoothedCamera.hh"

namespace glow
{
namespace camera
{
/**
 * Lets the camera orbit around its target
 * Hold LMB - Orbit
 */
class TargetOrbitController
{
private:
    float mCameraOrbitSpeed = 10.f;
    float mCameraScrollingSpeed = 0.1f;

public:
    void update(float, glow::input::InputState const& input, SmoothedCamera& camera) const;

    void setOrbitSpeed(float speed) { mCameraOrbitSpeed = speed; }
    void setScrollingSpeed(float speed) { mCameraScrollingSpeed = speed; }
};
}
}
