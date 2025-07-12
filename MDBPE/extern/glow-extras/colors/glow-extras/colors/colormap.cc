#include "colormap.hh"

#include <typed-geometry/feature/assert.hh>

#include <glow/objects/Texture1D.hh>

using namespace glow;
using namespace glow::colors;

colormap::colormap(const std::vector<tg::color3> &rgb)
{
    for (auto c : rgb)
        colors.push_back(color(c));
}

colormap::colormap(const std::vector<tg::color4> &rgb)
{
    for (auto c : rgb)
        colors.push_back(color(c));
}

colormap colormap::isosurface(int size, color base, color accent)
{
    colormap cm;
    for (auto i = 0; i < size - 1; ++i)
        cm.colors.push_back(base);
    cm.colors.push_back(accent);
    return cm;
}

colormap colormap::linear(int size, color start, color end)
{
    return linear(size, {{start, 0.0f}, {end, 1.0f}});
}

colormap colormap::linear(const std::vector<color> &colors)
{
    return colormap(colors);
}

colormap colormap::linear(int size, const std::vector<std::pair<color, float>> &colors)
{
    TG_ASSERT(colors.size() > 0);

    colormap cm;
    if (colors.size() == 1)
    {
        cm.colors.resize(size, colors.front().first);
    }
    else
    {
        int ci = 1;
        for (auto i = 0; i < size - 1; ++i)
        {
            auto a = (i + 0.5f) / size;
            while (ci < (int)colors.size() - 1 && colors[ci].second < a)
                ++ci;

            auto a1 = colors[ci].second;
            auto a0 = colors[ci - 1].second;

            auto c1 = colors[ci].first;
            auto c0 = colors[ci - 1].first;

            auto t = (a - a0) / (a1 - a0);
            cm.colors.push_back(mix(c0, c1, t));
        }
    }
    return cm;
}

colormap colormap::fire()
{
    colormap cm;
    TG_ASSERT(0 && "not supported");
    return cm;
}

SharedTexture1D colormap::texture(bool srgb) const
{
    auto tex = Texture1D::create((int)colors.size());
    auto t = tex->bind();
    t.setWrap(clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
    t.setData(srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, (int)colors.size(), colors);
    t.generateMipmaps();
    return tex;
}
