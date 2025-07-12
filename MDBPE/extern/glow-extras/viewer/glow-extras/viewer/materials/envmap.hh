#pragma once

#include <glow/fwd.hh>
#include <glow/objects/TextureCubeMap.hh>
#include <glow/util/AsyncTexture.hh>

namespace glow::viewer
{
struct EnvMap
{
    AsyncTextureCubeMap texture;
    float reflectivity = 1;
};

inline EnvMap envmap(AsyncTextureCubeMap texture, float reflectivity = 1) { return {std::move(texture), reflectivity}; }
}
