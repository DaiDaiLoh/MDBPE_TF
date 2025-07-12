#pragma once

#include "PlaneBuilder.hh"

namespace glow
{
namespace viewer
{
inline builder::PlaneBuilder floor()
{
    return builder::PlaneBuilder({0, 1, 0}).relativeToAABB({0, 0, 0});
}
inline builder::PlaneBuilder floor(float y)
{
    return builder::PlaneBuilder({0, 1, 0}).origin({0, y, 0});
}
}
}