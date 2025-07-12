#pragma once

#include <glow/fwd.hh>
#include <glow/objects/Program.hh>

#include "../detail/MeshAttribute.hh"
#include "../detail/MeshShaderBuilder.hh"
#include "../fwd.hh"

namespace glow
{
namespace viewer
{
class Masking
{
private:
    detail::SharedMeshAttribute mDataAttribute;
    float mThreshold = 0.5f;

    void buildShader(detail::MeshShaderBuilder& shader) const;
    void prepareShader(UsedProgram& shader) const;

public:
    size_t computeHash() const;

    Masking& visibilityMask(polymesh::vertex_attribute<bool> const& attr) { return visibilityMask(attr.to<float>()); }
    Masking& visibilityMask(polymesh::face_attribute<bool> const& attr) { return visibilityMask(attr.to<float>()); }
    Masking& visibilityMask(polymesh::edge_attribute<bool> const& attr) { return visibilityMask(attr.to<float>()); }
    Masking& visibilityMask(polymesh::halfedge_attribute<bool> const& attr) { return visibilityMask(attr.to<float>()); }

    template <class T>
    Masking& visibilityMask(T const& data)
    {
        mDataAttribute = detail::make_mesh_attribute("aVisible", data);
        return *this;
    }

    Masking& threshold(float thres)
    {
        mThreshold = thres;
        return *this;
    }

    friend class MeshRenderable;
    friend class PointRenderable;
    friend class LineRenderable;
};
}
}
