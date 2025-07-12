#pragma once

#include <string>

#include <glow/common/optional.hh>
#include <glow/common/shared.hh>
#include <glow/fwd.hh>
#include <glow/std140.hh>

#include "../../Settings.hh"
#include "../../fwd.hh"
#include "../../lights/Light.hh"
#include "../RenderStage.hh"
#include "../StageCamera.hh"

namespace glow
{
namespace pipeline
{
GLOW_SHARED(class, DepthPreStage);
class DepthPreStage : public RenderStage
{
private:
    // == Render Targets ==
    SharedTextureRectangle mAllocTargetDepth;

    // == FBOs ==
    SharedFramebuffer mFboDepth;

    // == Cluster Data & SSBOs ==
    SharedShaderStorageBuffer mSsboClusterVisibilities;
    SharedShaderStorageBuffer mSsboLightIndexList;
    SharedShaderStorageBuffer mSsboClusterAabbs;
    unsigned mCachedClusterAmount = 0;
    SharedShaderStorageBuffer mSsboGlobalIndexCount;
    SharedShaderStorageBuffer mSsboLightData; ///< Contains an array of GPU-optimized Light structs
    std::vector<Light> mGpuLights;

    // == Shaders ==
    SharedProgram mComputeClusterAabbs;
    SharedProgram mComputeClusterLights;

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;
    /// Called when a new frame begins
    void onBeginNewFrame(RenderPipeline& pipeline) override;

public:
    DepthPreStage();

    SharedTextureRectangle const& getTargetDepth() const { return mAllocTargetDepth; }

    SharedShaderStorageBuffer const& getSsboClusterVisibilites() const { return mSsboClusterVisibilities; }
    SharedShaderStorageBuffer const& getSsboLightIndices() const { return mSsboLightIndexList; }
    SharedShaderStorageBuffer const& getSsboLightData() const { return mSsboLightData; }

    optional<tg::pos3> query3DPosition(StageCamera const& cam, tg::ipos2 pixel) const;

    StageType getType() const override { return StageType::DepthPre; }

    std::string name() const override { return "Depth Pre"; }
};
}
}
