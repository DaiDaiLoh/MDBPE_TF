#pragma once

#include <array>
#include <string>

#include <glow/common/shared.hh>
#include <glow/fwd.hh>

#include "../../Settings.hh"
#include "../../fwd.hh"
#include "../RenderStage.hh"

#include "AO/AOGlobalBuffer.hh"

namespace glow
{
namespace pipeline
{
GLOW_SHARED(class, AOStage);
class AOStage : public RenderStage
{
private:
    // == Render Targets ==
    SharedTextureRectangle mTargetLinearDepth;
    SharedTexture2DArray mTargetViewDepthArray;
    SharedTexture2D mTargetNormals;
    SharedTexture2DArray mTargetAoArray;
    SharedTexture2D mTargetAOZ;
    SharedTexture2D mTargetAOZ2;
    SharedTextureRectangle mTargetAoFinal;

    // == FBOs ==
    SharedFramebuffer mFboLinDepth;
    std::array<SharedFramebuffer, 2> mFbosViewDepthArray;
    SharedFramebuffer mFboNormalReconstruct;
    SharedFramebuffer mFboLayeredCoarseAo;
    SharedFramebuffer mFboAOZ1;
    SharedFramebuffer mFboAOZ2;

    // == UBOs ==
    SharedUniformBuffer mUboAOGlobal;
    detail::AOGlobalBuffer mBufferAOGlobal;

    std::array<SharedUniformBuffer, 16> mUbosAOPerPass;

    // == Shaders ==
    SharedProgram mShaderLinearizeDepth;
    SharedProgram mShaderDeinterleavedDepth;
    SharedProgram mShaderReconstructNormals;
    SharedProgram mShaderCoarseAO;
    SharedProgram mShaderReinterleavedAO;
    SharedProgram mShaderBlurX;
    SharedProgram mShaderBlurY;

    // == Dependencies ==
    SharedDepthPreStage mStageDepthPre;

    // == Samplers ==
    SharedSampler mSamplerLinear;
    SharedSampler mSamplerNearest;

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;
    /// Called when a new frame begins
    void onBeginNewFrame(RenderPipeline& pipeline) override;

public:
    AOStage(SharedDepthPreStage const& depthPreStage);

    SharedTextureRectangle const& getTargetAO() const { return mTargetAoFinal; }
    GLOW_GETTER(TargetLinearDepth);
    GLOW_GETTER(TargetNormals);

    StageType getType() const override { return StageType::Internal; }

    std::string name() const override { return "Ambient Occlusion"; }
};

}
}
