#pragma once

#include <vector>

#include <typed-geometry/tg-lean.hh>

#include <glow/fwd.hh>

#include "color.hh"

namespace glow
{
namespace colors
{
struct colormap
{
    std::vector<color> colors;
    bool clamped = false;

    colormap& clamp()
    {
        clamped = true;
        return *this;
    }

    colormap& repeat()
    {
        clamped = false;
        return *this;
    }

    colormap() = default;
    colormap(std::vector<tg::color3> const& rgb);
    colormap(std::vector<tg::color4> const& rgb);
    colormap(std::vector<color> const& c) : colors(c) {}

    static colormap isosurface(int size = 16, color base = {1, 1, 1}, color accent = {0, 0, 0});
    /// linear interpolation between start and end
    static colormap linear(int size, color start, color end);
    /// linear interpolation between weighted colors
    static colormap linear(int size, std::vector<std::pair<color, float>> const& colors);
    static colormap linear(std::vector<color> const& colors);

    /// https://au.mathworks.com/matlabcentral/fileexchange/35730-fire-and-or-custom-colormap-function?focused=5226280&tab=function
    static colormap fire();

    glow::SharedTexture1D texture(bool srgb = true) const;
};
}
}
