#include "LightingCombinationStage.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow-extras/geometry/FullscreenTriangle.hh>

#include "../../RenderCallback.hh"
#include "../../RenderPipeline.hh"
#include "../../RenderScene.hh"
#include "DepthPreStage.hh"
#include "OITStage.hh"
#include "OpaqueStage.hh"

void glow::pipeline::LightingCombinationStage::onExecuteStage(RenderContext const& ctx, RenderCallback&)
{
    PIPELINE_PROFILE("Lighting Combination Stage");

    mAllocTargetHdr = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGB16F, ctx.camData.resolution});

    {
        auto fbo = mFboLightingCombination->bind();
        fbo.attachColor("fHDR", mAllocTargetHdr);
        fbo.setViewport(ctx.camData.resolution);

        // OIT Resolve
        {
            GLOW_SCOPED(disable, GL_DEPTH_TEST);
            GLOW_SCOPED(disable, GL_CULL_FACE);

            if (!mShaderRegistered)
            {
                registerShader(ctx, mShaderLightingCombination);
                mShaderRegistered = true;
            }

            auto shader = mShaderLightingCombination->use();
            shader.setTexture("uShadedOpaque", mStageOpaque->getTargetHdr());
            shader.setTexture("uAccumulationA", mStageOIT->getTargetAccuA());
            shader.setTexture("uAccumulationB", mStageOIT->getTargetAccuB());
            shader.setTexture("uDistortion", mStageOIT->getTargetDistortion());
            shader.setTexture("uDepth", mStageDepthPre->getTargetDepth());

            shader.setUniform("uCamPos", ctx.camData.camPos);
            geometry::FullscreenTriangle::draw();
        }
    }
}

void glow::pipeline::LightingCombinationStage::onBeginNewFrame(glow::pipeline::RenderPipeline& pipeline)
{
    if (mAllocTargetHdr)
    {
        pipeline.getTexPoolRect().free(&mAllocTargetHdr);
    }
}

glow::pipeline::LightingCombinationStage::LightingCombinationStage(SharedOITStage const& oitStage, SharedOpaqueStage const& opaqueStage, SharedDepthPreStage const& depthPreStage)
  : mStageOIT(oitStage), mStageOpaque(opaqueStage), mStageDepthPre(depthPreStage)
{
    registerDependency(mStageOIT);
    registerDependency(mStageOpaque);
    registerDependency(mStageDepthPre);

    // FBOs
    {
        mFboLightingCombination = Framebuffer::create();
    }

    // Shaders
    {
        auto constexpr fullscreenPath = "glow-pipeline/internal/fullscreenTriangle.vsh";

        mShaderLightingCombination = Program::createFromFiles({"glow-pipeline/internal/pass/lightingcombination/combination.fsh", fullscreenPath});
    }


    geometry::FullscreenTriangle::init();
}
