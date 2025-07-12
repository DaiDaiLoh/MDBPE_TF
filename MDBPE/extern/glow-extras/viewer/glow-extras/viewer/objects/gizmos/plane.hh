#pragma once

#include "PlaneBuilder.hh"

namespace glow
{
namespace viewer
{
inline builder::PlaneBuilder plane(tg::dir3 n = {0, 1, 0})
{
    return builder::PlaneBuilder(n).relativeToAABB({n.x > 0 ? 1.f : 0.f, n.y > 0 ? 1.f : 0.f, n.z > 0 ? 1.f : 0.f});
}
inline builder::PlaneBuilder plane(tg::dir3 n, tg::pos3 pos)
{
    return builder::PlaneBuilder(n).origin(pos);
}
}
}
