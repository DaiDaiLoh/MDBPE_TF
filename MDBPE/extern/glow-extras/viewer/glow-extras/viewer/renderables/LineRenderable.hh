#pragma once

#include "GeometricRenderable.hh"

#include "../objects/geometry/LineBuilder.hh"

namespace glow
{
namespace viewer
{
class LineRenderable final : public GeometricRenderable
{
private:
    bool mRoundCaps = false;
    bool mNoCaps = false;
    bool mExtrapolate = false;
    bool m3D = false;
    bool mCameraFacing = false;
    bool mWorldSpaceSize = false;
    bool mDashSizeWorld = false;
    bool mForceTwoColored = false;

    // is lazily built
    glow::SharedVertexArray mVertexArray;
    glow::SharedProgram mForwardShader;
    glow::SharedProgram mShadowShader;
    glow::SharedProgram mPickingShader;

public:
    aabb computeAabb() override;

    void renderShadow(RenderInfo const& info) override;
    void renderForward(RenderInfo const& info) override;
    void renderPicking(RenderInfo const& info, int32_t renderableID) override;

    size_t computeHash() const override;

public:
    static SharedLineRenderable create(builder::LineBuilder const& builder);

private:
    void initFromBuilder(builder::LineBuilder const& builder);

    void init() override;
};
}
}
