#pragma once

#include "../CameraBase.hh"

#include <typed-geometry/tg-lean.hh>

namespace glow
{
namespace camera
{
GLOW_SHARED(class, FixedCamera);
/**
 * @brief A fixed camera where all attributes are set explicitly except for the near/far plane
 *        which are derrived from the projection matrix.
 */
class [[deprecated]] FixedCamera : public CameraBase
{
private:
    tg::pos3 mPosition;
    tg::mat4 mViewMatrix;
    tg::mat4 mProjectionMatrix;
    tg::isize2 mViewportSize;

    // will get calculated based on the projection matrix
    // so there are no explicit setters for this!
    float mNearPlane;
    float mFarPlane;

public:
    /// CAUTION: default ctor with zero values
    FixedCamera();
    FixedCamera(const tg::pos3& _pos, const tg::mat4& _view, const tg::mat4& _proj, const tg::isize2& _viewport);

    // Getter, Setter for Camera Position
    virtual tg::pos3 getPosition() const override { return mPosition; }
    virtual void setPosition(tg::pos3 const& _val) { mPosition = _val; }
    // Getter, Setter for Camera ViewMatrix
    virtual tg::mat4 getViewMatrix() const override { return mViewMatrix; }
    virtual void setViewMatrix(tg::mat4 const& _val) { mViewMatrix = _val; }
    // Getter, Setter for Camera ProjectionMatrix
    virtual tg::mat4 getProjectionMatrix() const override { return mProjectionMatrix; }
    virtual void setProjectionMatrix(tg::mat4 const& _val);

    // Getter, Setter for Camera ViewportSize
    virtual tg::isize2 getViewportSize() const override { return mViewportSize; }
    virtual void setViewportSize(tg::isize2 const& _val) { mViewportSize = _val; }
    // getters for near/far plane (far can be inf!)
    virtual float getNearClippingPlane() const override { return mNearPlane; }
    virtual float getFarClippingPlane() const override { return mFarPlane; }
    // getters for fov (TODO: possibly derive these from the projection matrix or add setters
    virtual tg::angle getVerticalFieldOfView() const override { return 0_deg; }
    virtual tg::horizontal_fov getHorizontalFieldOfView() const override { return tg::horizontal_fov(0_deg); }
};
}
}
