#pragma once

#include <typed-geometry/tg-lean.hh>

#include <glow/math/transform.hh>

namespace glow
{
namespace camera
{
/**
 * The Handle
 *
 * Component of the camera responsible for position and orientation
 */
class Handle
{
private:
    enum class SmoothingMode
    {
        FPS,
        Orbit
    };

    struct HandleState
    {
        glow::transform transform = glow::transform::Identity;
        tg::vec3 forward = tg::vec3::unit_x;
        tg::pos3 target = tg::pos3::zero;
        float targetDistance = 20.f;
    };

    HandleState mPhysical;
    HandleState mTarget;

    SmoothingMode mSmoothingMode = SmoothingMode::FPS;
    tg::vec3 mCameraUp = glow::transform::Up();

public:
    /// Smoothing sensitivities per category
    struct
    {
        float distance = 25.f;
        float position = 25.f;
        float rotation = 35.f;
        float target = 25.f;
    } sensitivity;

private:
    static tg::quat forwardToRotation(tg::vec3 const& forward, tg::vec3 const& up = glow::transform::Up());

public:
    Handle(glow::transform const& transform = glow::transform::Identity);

    /// Performs smoothing
    void update(float dt);

    void snap() { mPhysical = mTarget; }  ///< Snap to the target state
    void abort() { mTarget = mPhysical; } ///< Abort smoothing to the target state

public:
    // -- Manipulators --
    void move(tg::vec3 const& distance);
    void setPosition(tg::pos3 const& pos);
    void setLookAt(tg::pos3 const& pos, tg::pos3 const& target, tg::vec3 const& up = glow::transform::Up());
    void setTarget(tg::pos3 const& target);
    void setTargetDistance(float dist);
    void addTargetDistance(float deltaDist);

    // -- Input-based manipulators --
    /// Rotate on the spot, like a FPS camera
    void lookAround(float dX, float dY);
    /// Orbit around the current target
    void orbit(float dX, float dY);
    /// Translate the camera to put the target in focus
    void translateFocus(tg::pos3 const& target);

    // -- Raw access --
    /// Directly manipulate the physical state
    /// Do not do this if you use manipulators and ::update
    HandleState& physicalState() { return mPhysical; }

    /// returns true if all values are finite
    bool isValid() const;

public:
    // -- Getters --
    tg::mat4 getViewMatrix() const;
    tg::pos3 const& getPosition() const { return mPhysical.transform.position; }
    tg::pos3 const& getTarget() const { return mPhysical.target; }
    float getLookAtDistance() const { return mPhysical.targetDistance; }
    tg::vec3 const& getForward() const { return mPhysical.forward; }

    glow::transform const& getTransform() const { return mPhysical.transform; }
};
}
}
