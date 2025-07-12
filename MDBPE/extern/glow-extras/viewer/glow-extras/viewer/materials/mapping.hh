#pragma once

#include "ColorMapping.hh"

namespace glow
{
namespace viewer
{
template <class T>
ColorMapping mapping(T const& data)
{
    return ColorMapping().data(data);
}
template <class T>
ColorMapping mapping(T const& data, glow::SharedTexture1D const& tex)
{
    return mapping(data).texture(tex);
}
template <class T>
ColorMapping mapping(T const& data, glow::SharedTexture2D const& tex)
{
    return mapping(data).texture(tex);
}
template <class T>
ColorMapping mapping(T const& data, glow::SharedTexture3D const& tex)
{
    return mapping(data).texture(tex);
}
}
}
