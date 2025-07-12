#pragma once

#include <typed-geometry/tg-lean.hh>

namespace glow::viewer
{
struct RenderInfo
{
    tg::mat4 view;
    tg::mat4 proj;
    tg::pos3 sunPos;
    tg::isize2 resolution;
    tg::pos3 camPos;
    tg::dir3 camForward;
    tg::dir3 camUp;
    tg::dir3 camRight;
    int accumulationCount;
    bool reverseZEnabled;
    double elapsedSeconds;
};
}
