#pragma once

#include <array>
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
GLOW_SHARED(class, PostprocessingStage);
class PostprocessingStage : public RenderStage
{
private:
    // == Render Targets ==
    SharedTextureRectangle mAllocTargetLdr;

    // == FBOs ==
    SharedFramebuffer mFboLdr;

    // == VAOs ==
    SharedVertexArray mVaoQuad;

    // == Shaders ==
    SharedProgram mShaderPostprocessing;
    bool mShaderRegistered = false;

    // == Dependencies ==
    SharedTemporalResolveStage mStageTemporalResolve;

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;
    /// Called when a new frame begins
    void onBeginNewFrame(RenderPipeline& pipeline) override;

public:
    PostprocessingStage(SharedTemporalResolveStage const& trStage);

    SharedTextureRectangle const& getTargetLdr() const { return mAllocTargetLdr; }

    StageType getType() const override { return StageType::Internal; }

    std::string name() const override { return "Postprocessing"; }
};

}
}
