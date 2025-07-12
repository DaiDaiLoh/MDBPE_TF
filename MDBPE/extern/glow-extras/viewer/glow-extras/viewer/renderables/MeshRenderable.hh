#pragma once

#include "GeometricRenderable.hh"

#include "../objects/geometry/PolygonBuilder.hh"

namespace glow
{
namespace viewer
{
class MeshRenderable final : public GeometricRenderable
{
private:
    // is lazily built
    glow::SharedVertexArray mMesh;
    glow::SharedProgram mForwardShader;
    glow::SharedProgram mShadowShader;
    glow::SharedProgram mPickingShader;

public:
    aabb computeAabb() override;

    void renderShadow(RenderInfo const& info) override;
    void renderForward(RenderInfo const& info) override;
    void renderTransparent(RenderInfo const& info) override;
    void renderPicking(RenderInfo const& info, int32_t renderableID) override;

    void init() override;
    size_t computeHash() const override;

private:
    void renderMesh(RenderInfo const& info);

public:
    static SharedMeshRenderable create(builder::PolygonBuilder const& builder);
};
}
}
