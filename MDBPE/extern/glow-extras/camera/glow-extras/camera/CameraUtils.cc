#include "CameraUtils.hh"

#include <typed-geometry/tg.hh>

tg::ray3 glow::camera::getViewRay(glow::camera::CameraBase const& cam, tg::pos2 const& mousePosition)
{
    tg::vec3 ps[2];
    auto i = 0;
    for (auto d : {0.5f, -0.5f})
    {
        tg::vec4 v{mousePosition.x * 2 - 1, 1 - mousePosition.y * 2, d * 2 - 1, 1.0};

        v = tg::inverse(cam.getProjectionMatrix()) * v;
        v /= v.w;
        v = tg::inverse(cam.getViewMatrix()) * v;
        ps[i++] = tg::vec3(v);
    }

    return {cam.getPosition(), normalize(ps[0] - ps[1])};
}
