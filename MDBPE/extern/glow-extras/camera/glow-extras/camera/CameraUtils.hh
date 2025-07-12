#pragma once

#include <typed-geometry/tg-lean.hh>

#include "CameraBase.hh"

namespace glow
{
namespace camera
{
tg::ray3 getViewRay(CameraBase const& cam, tg::pos2 const& mousePosition);
}
}
