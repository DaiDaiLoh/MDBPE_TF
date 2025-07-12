#pragma once

#include <polymesh/Mesh.hh>

namespace glow
{
namespace viewer
{
namespace detail
{
enum class MeshType
{
    Unknown,

    PolyMesh,

    PolygonList,
    LineList,
    PointList
};

struct MeshInfo
{
    MeshType type = MeshType::Unknown;

    // -1 means "does not apply"
    int vertexCount = -1;
    int faceCount = -1;
    int edgeCount = -1;
};

inline MeshInfo make_mesh_info(pm::Mesh const& m)
{
    MeshInfo mi;
    mi.type = MeshType::PolyMesh;
    mi.vertexCount = m.all_vertices().size();
    mi.faceCount = m.all_faces().size();
    mi.edgeCount = m.all_edges().size();
    return mi;
}
}
}
}
