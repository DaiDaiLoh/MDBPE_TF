#pragma once

#include <string>

#include <glow/fwd.hh>

namespace glow
{
/// Color, depth, or stencil attachment
struct FramebufferAttachment
{
    std::string locationName; ///< ignored for non-color
    SharedTexture texture;
    int mipmapLevel = 0;
    int layer = 0; ///< only valid for nD-array and cubemaps
    // TODO: support for "image"

    FramebufferAttachment() = default;
    FramebufferAttachment(std::string const& loc, SharedTexture const& tex, int miplevel = 0, int layer = 0)
      : locationName(loc), texture(tex), mipmapLevel(miplevel), layer(layer)
    {
    }
};
}
