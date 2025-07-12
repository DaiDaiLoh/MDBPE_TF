#pragma once

#include <glow/common/shared.hh>
#include <glow/fwd.hh>

#include "../../RenderCallback.hh"
#include "../../fwd.hh"
#include "../RenderStage.hh"
#include "../StageCamera.hh"

namespace glow
{
namespace pipeline
{
/**
    Temporal Resolve Stage
    Performs Temporal Antialiasing
    Requires a HDR image and a velocity buffer
 */
GLOW_SHARED(class, TemporalResolveStage);
class TemporalResolveStage : public RenderStage
{
private:
    // == Render Targets ==
    SharedTextureRectangle mAllocTargetBloom; ///< Initially contains raw bloom, then eventually filtered and blurred output
    SharedTextureRectangle mAllocTargetBloomBlurA;
    SharedTextureRectangle mAllocTargetBloomBlurB;

    // == FBOs ==
    SharedFramebuffer mFboTemporalResolve;
    SharedFramebuffer mFboBloom;
    SharedFramebuffer mFboBloomFiltered;
    SharedFramebuffer mFboBloomBlurA;
    SharedFramebuffer mFboBloomBlurB;

    // == Shaders ==
    SharedProgram mShaderTemporalResolve;
    bool mShaderRegistered = false;
    SharedProgram mShaderKawaseBlur;
    SharedProgram mShaderDownsample;

    // == Dependencies ==
    SharedOpaqueStage mStageOpaque;
    SharedLightingCombinationStage mStageLightingCombination;

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;
    /// Called when a new frame begins
    void onBeginNewFrame(RenderPipeline& pipeline) override;

public:
    TemporalResolveStage(SharedOpaqueStage const& opaqueStage, SharedLightingCombinationStage const& lightingCombinationStage);

    /// The HDR target is not pool-allocated, but instead part of the stage camera
    SharedTextureRectangle const& getTargetHdr(RenderContext const& ctx) const
    {
        return ctx.cam.getTemporalData().evenFrame ? ctx.cam.getTemporalData().historyBufferEven : ctx.cam.getTemporalData().historyBufferOdd;
    }

    SharedTextureRectangle const& getTargetBloom() const { return mAllocTargetBloom; }

    StageType getType() const override { return StageType::Internal; }

    std::string name() const override { return "Temporal Resolve"; }
};
}
}
