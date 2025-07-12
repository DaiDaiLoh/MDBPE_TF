#pragma once

#include <array>
#include <string>

#include <glow/common/shared.hh>
#include <glow/fwd.hh>
#include <glow/std140.hh>

#include "../../fwd.hh"
#include "../RenderStage.hh"
#include "../StageCamera.hh"

#include "shader/glow-pipeline/internal/common/globals.hh"

namespace glow
{
namespace pipeline
{
/**
    Shadow Stage
    Produces shadow maps
 */
GLOW_SHARED(class, ShadowStage);
class ShadowStage : public RenderStage
{
private:
    // == FBOs ==
    SharedFramebuffer mFboShadows;

    // == UBOs ==
    SharedUniformBuffer mUboLightVPs;
    SharedUniformBuffer mUboSquaredCascadeLimits;

    // == Shaders ==
    SharedProgram mShaderShadow;
    SharedProgram mShaderShadowInstancing;

    // == Data ==
    tg::vec4 mSplitDepths;
    std::array<tg::mat4, GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT> mLightSpaceVps;

private:
    void updateShadowData(StageCamera const& cam, tg::vec3 const& lightDirection, float cascadeLambda);

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;

public:
    ShadowStage();

    SharedUniformBuffer const& getLightVpUbo() const { return mUboLightVPs; }
    SharedUniformBuffer const& getCascadeLimitUbo() const { return mUboSquaredCascadeLimits; }

    void registerShader(RenderContext const& ctx, SharedProgram const& shader) const override;

    StageType getType() const override { return StageType::Shadow; }

    std::string name() const override { return "Shadows"; }
};

}
}
