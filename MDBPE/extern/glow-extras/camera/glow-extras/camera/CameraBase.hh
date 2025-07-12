#pragma once

#include <typed-geometry/tg-lean.hh>

#include <glow/common/shared.hh>

namespace glow
{
namespace camera
{
/**
 * @brief Common interface for cameras
 */
GLOW_SHARED(class, CameraBase);
class CameraBase
{
public:
    virtual ~CameraBase() = default;

    /**
     * @brief gets the Position of the camera
     * @return a 3-dimensional position in the global coordinate system
     */
    virtual tg::pos3 getPosition() const = 0;
    /**
     * @brief gets the ViewMatrix of the camera
     * @return a 4x4 matrix containing projection independent camera transforms
     */
    virtual tg::mat4 getViewMatrix() const = 0;
    /**
     * @brief gets the ProjectionMatrix of the camera
     * @return a 4x4 matrix containing the projection into normalized device coordinates
     */
    virtual tg::mat4 getProjectionMatrix() const = 0;
    /**
     * @brief gets the ViewportSize of the current viewport of this camera
     * @return the 2-dimensional size of the viewport
     */
    virtual tg::isize2 getViewportSize() const = 0;
    /**
     * @brief Gets the near clipping plane as a distance from the camera.
     * @return the near clipping plane
     */
    virtual float getNearClippingPlane() const = 0;
    /**
     * @brief Gets the far clipping plane as a distance from the camera. Note that it could be inf!
     * Not all projection matrices have a real far plane but some are unlimited!
     * @return the near clipping plane
     */
    virtual float getFarClippingPlane() const = 0;
    /**
     * @brief Gets the vertical field of view in degrees. Returns -1 if the camera does not have
     * a field of view.
     * @return the vertical field of view
     */
    virtual tg::angle getVerticalFieldOfView() const = 0;
    /**
     * @brief Gets the horizontal field of view in degrees. Returns -1 if the camera does not have
     * a field of view.
     * @return the horizontal field of view
     */
    virtual tg::horizontal_fov getHorizontalFieldOfView() const = 0;
};
}
}
