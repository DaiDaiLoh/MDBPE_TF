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
GLOW_SHARED(class, OutputStage);
class OutputStage : public RenderStage
{
private:
    /// Size of the color LUT in each dimensions
    /// This value has to coincide with the define COLOR_LUT_DIMS
    /// in shader/internal/pass/output/output.fsh
    static constexpr auto colorLutDimensions = 16u;

    // == Shaders ==
    SharedProgram mShaderOutput;
    bool mShaderRegistered = false;

    // == Dependencies ==
    SharedPostprocessingStage mStagePostprocessing;

    // == Other ==
    SharedTexture3D mNeutralColorLut;
    bool mViewportEnabled = false;
    tg::ipos2 mViewportOffset;
    tg::isize2 mViewportSize;

private:
    static SharedTexture3D generateNeutralLut();

protected:
    /// Called when the stage is supposed to run
    void onExecuteStage(RenderContext const& ctx, RenderCallback& rc) override;

public:
    OutputStage(SharedPostprocessingStage const& postprocessingStage);

    void setViewport(tg::ipos2 const& offset, tg::isize2 const& size);
    void clearViewport();

    bool isViewportEnabled() const { return mViewportEnabled; }

    SharedPostprocessingStage const& getPostprocessingStage() const { return mStagePostprocessing; }

    StageType getType() const override { return StageType::Internal; }
    std::string name() const override { return "Output"; }
};

}
}
