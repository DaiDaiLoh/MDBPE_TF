#pragma once

#include <string>
#include <vector>

#include <typed-geometry/tg-lean.hh>

#include <glow/common/shared.hh>
#include <glow/fwd.hh>

namespace glow
{
namespace assimp
{
GLOW_SHARED(struct, MeshData);

struct MeshData
{
    std::string filename;

    tg::pos3 aabbMin;
    tg::pos3 aabbMax;

    std::vector<tg::pos3> positions;
    std::vector<tg::vec3> normals;
    std::vector<tg::vec3> tangents;

    // one vector per channel
    std::vector<std::vector<tg::pos2>> texCoords;
    std::vector<std::vector<tg::color4>> colors;

    std::vector<uint32_t> indices;

    SharedVertexArray createVertexArray() const;
};
}
}
