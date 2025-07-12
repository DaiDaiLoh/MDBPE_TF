#include "Lens.hh"

#include <typed-geometry/tg.hh>

void glow::camera::Lens::setViewportSize(int w, int h)
{
    mViewportSize.width = w;
    mViewportSize.height = h;
    mAspectRatio = static_cast<float>(w) / static_cast<float>(h);
}

tg::mat4 glow::camera::Lens::getProjectionMatrix() const
{
    if (std::isinf(mFarPlane))
    {
        float const e = 1.0f / tan(getVerticalFov() * 0.5f);

        // infinite Perspective matrix reversed mapping to 1..-1
        tg::mat4 m;
        m[0] = {e / mAspectRatio, 0.0f, 0.0f, 0.0f};
        m[1] = {0.0f, e, 0.0f, 0.0f};
        m[2] = {0.0f, 0.0f, -1.0f, -1.0f};
        m[3] = {0.0f, 0.0f, -2.0f * mNearPlane, 0.0f};

        return m;
    }
    else
    {
        return tg::perspective_opengl(mHorizontalFov, mAspectRatio, mNearPlane, mFarPlane);
    }
}

tg::angle glow::camera::Lens::getVerticalFov() const { return tg::atan(tan(0.5f * mHorizontalFov.angle) / mAspectRatio) * 2.0f; }
