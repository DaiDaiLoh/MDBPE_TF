#pragma once

#include <string>

#include <glow/common/shared.hh>
#include <glow/fwd.hh>

#include "../../fwd.hh"
#include "../RenderStage.hh"
#include "../StageCamera.hh"

namespace glow
{
namespace pipeline
{
/**
    Lighting Combination Stage
    Blends the results of the Opaque Stage and the OIT Stage
    Creates the unprocessed HDR and the unprocessed Bloom target
 */
GLOW_SHARED(class, LightingCombinationStage);
class LightingCombinationStage : public RenderStage
{
private:
    // == Render Targets ==
    SharedTextureRectangle mAllocTargetHdr;

    // == FBOs ==
    SharedFramebuffer mFboLightingCombination;

    // == Shaders ==
    SharedProgram mShaderLightingCombination;
    bool mShaderRegistered = false;

    // == Dependencies ==
    SharedOITStage mStageOIT;
    SharedOpaqueStage mStageOpaque;
    SharedDepthPreStage mStageDepthPre;

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;
    /// Called when a new frame begins
    void onBeginNewFrame(RenderPipeline& pipeline) override;

public:
    LightingCombinationStage(SharedOITStage const& oitStage, SharedOpaqueStage const& opaqueStage, SharedDepthPreStage const& depthPreStage);

    SharedTextureRectangle const& getTargetHdr() const { return mAllocTargetHdr; }

    StageType getType() const override { return StageType::Internal; }

    std::string name() const override { return "Lighting Combination"; }
};

}
}
