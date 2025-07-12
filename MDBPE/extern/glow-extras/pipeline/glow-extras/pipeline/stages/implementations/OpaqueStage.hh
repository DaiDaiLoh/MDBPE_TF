#pragma once

#include <array>
#include <memory>
#include <string>

#include <glow/common/shared.hh>
#include <glow/fwd.hh>
#include <glow/std140.hh>

#include "../../fwd.hh"
#include "../../lights/Light.hh"
#include "../RenderStage.hh"
#include "../StageCamera.hh"

namespace glow
{
namespace pipeline
{
GLOW_SHARED(class, OpaqueStage);
class OpaqueStage : public RenderStage
{
private:
    // == Render Targets ==
    SharedTextureRectangle mAllocTargetHdr;
    SharedTextureRectangle mAllocTargetVelocity;

    // == FBOs ==
    SharedFramebuffer mFboHdr;
    SharedFramebuffer mFboHdrOnly;
    SharedFramebuffer mFboVelocity;

    // == Shaders ==
    SharedProgram mShaderVelocityInit;
    SharedProgram mShaderEdgeOutline;

    // == Dependencies ==
    SharedDepthPreStage mStageDepthPre;
    SharedShadowStage mStageShadow;
    SharedAOStage mStageAo;

    // == Samplers ==
    SharedSampler mSamplerAo;

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;
    /// Called when a new frame begins
    void onBeginNewFrame(RenderPipeline& pipeline) override;

public:
    OpaqueStage(SharedDepthPreStage const& depthPreStage, SharedShadowStage const& shadowStage, SharedAOStage const& aoStage);

    SharedTextureRectangle const& getTargetHdr() const { return mAllocTargetHdr; }
    SharedTextureRectangle const& getTargetVelocity() const { return mAllocTargetVelocity; }

    void registerShader(RenderContext const& ctx, SharedProgram const& shader) const override;
    void uploadShaderData(RenderContext const& ctx, UsedProgram& shader) const override;

    StageType getType() const override { return StageType::Opaque; }

    std::string name() const override { return "Opaque"; }
};

}
}
