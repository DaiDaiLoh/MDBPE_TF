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
    Order Independent Transparency Stage
    Draws transparent geometry into the TBuffer
    Gets resolved in the Lighting Combination Stage
 */
GLOW_SHARED(class, OITStage);
class OITStage : public RenderStage
{
private:
    // == Render Targets ==
    SharedTextureRectangle mAllocTargetAccuA;
    SharedTextureRectangle mAllocTargetAccuB;
    SharedTextureRectangle mAllocTargetDistortion;

    // == FBOs ==
    SharedFramebuffer mFboTbuffer;

    // == Dependencies ==
    SharedDepthPreStage mStageDepthPre;

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;
    /// Called when a new frame begins
    void onBeginNewFrame(RenderPipeline& pipeline) override;

public:
    OITStage(SharedDepthPreStage const& depthPreStage);

    SharedTextureRectangle const& getTargetAccuA() const { return mAllocTargetAccuA; }
    SharedTextureRectangle const& getTargetAccuB() const { return mAllocTargetAccuB; }
    SharedTextureRectangle const& getTargetDistortion() const { return mAllocTargetDistortion; }

    void uploadShaderData(RenderContext const& ctx, UsedProgram& shader) const override;

    StageType getType() const override { return StageType::Transparent; }

    std::string name() const override { return "OIT"; }
};

}
}
