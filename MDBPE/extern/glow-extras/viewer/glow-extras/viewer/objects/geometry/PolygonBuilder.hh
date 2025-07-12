#pragma once

#include "GeometricRenderableBuilder.hh"

#include <polymesh/properties.hh>

namespace glow
{
namespace viewer
{
namespace builder
{
class PolygonBuilder : public GeometricRenderableBuilder<PolygonBuilder>
{
    // normals
public:
    template <class T>
    PolygonBuilder& normals(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aNormal", val));
        mHasNormals = true;
        return *this;
    }
    PolygonBuilder& face_normals()
    {
        addAttribute(mMeshDef->computeFaceNormalAttribute());
        mHasNormals = true;
        return *this;
    }
    template <class Pos3>
    PolygonBuilder& face_normals(polymesh::vertex_attribute<Pos3> const& pos)
    {
        addAttribute(detail::make_mesh_attribute("aNormal", polymesh::face_normals(pos.mesh(), pos)));
        mHasNormals = true;
        return *this;
    }
    PolygonBuilder& smooth_normals()
    {
        addAttribute(mMeshDef->computeSmoothNormalAttribute());
        mHasNormals = true;
        return *this;
    }
    template <class Pos3>
    PolygonBuilder& smooth_normals(polymesh::vertex_attribute<Pos3> const& pos)
    {
        addAttribute(detail::make_mesh_attribute("aNormal", polymesh::vertex_normals_by_area(pos)));
        mHasNormals = true;
        return *this;
    }

private:
    bool mHasNormals = false;
    std::shared_ptr<detail::PolyMeshDefinition> mMeshDef;

public:
    GLOW_GETTER(MeshDef);

public:
    explicit PolygonBuilder(std::shared_ptr<detail::PolyMeshDefinition> def) : mMeshDef(std::move(def)) {}
};
}
}
}
