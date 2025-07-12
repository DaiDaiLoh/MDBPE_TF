#pragma once

#include <typed-geometry/tg-lean.hh>

namespace glow
{
namespace viewer
{
namespace builder
{
class PlaneBuilder
{
public:
    PlaneBuilder(tg::dir3 normal) : mNormal(normal) {}

    PlaneBuilder& origin(tg::pos3 pos)
    {
        mOrigin = pos;
        mRelativeToAABB = false;
        return *this;
    }
    PlaneBuilder& relativeToAABB(tg::pos3 relativePos)
    {
        mOrigin = relativePos;
        mRelativeToAABB = true;
        return *this;
    }

private:
    tg::dir3 mNormal;
    tg::pos3 mOrigin;
    bool mRelativeToAABB = true;

    // TODO: material
};
}
}
}
