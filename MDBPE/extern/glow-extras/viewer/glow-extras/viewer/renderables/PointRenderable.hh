#pragma once

#include "GeometricRenderable.hh"

#include "../objects/geometry/PointBuilder.hh"

namespace glow
{
namespace viewer
{
class PointRenderable final : public GeometricRenderable
{
private:
    enum class RenderMode
    {
        Round,
        Square,
        Sphere
    };

    RenderMode mRenderMode = RenderMode::Round;
    bool mCameraFacing = false;
    bool mWorldSpaceSize = false;

    // is lazily built
    glow::SharedVertexArray mVertexArray;
    glow::SharedProgram mForwardShader;
    glow::SharedProgram mShadowShader;
    glow::SharedProgram mPickingShader;

public:
    aabb computeAabb() override;
    size_t computeHash() const override;

    void renderShadow(RenderInfo const& info) override;
    void renderForward(RenderInfo const& info) override;
    void renderTransparent(RenderInfo const& info) override;
    void renderPicking(RenderInfo const& info, int32_t renderableID) override;

public:
    static SharedPointRenderable create(builder::PointBuilder const& builder);

private:
    void renderPoints(RenderInfo const& info);
    void initFromBuilder(builder::PointBuilder const& builder);

    void init() override;
};
}
}
