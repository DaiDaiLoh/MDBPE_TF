#include "PostprocessingStage.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture3D.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow-extras/geometry/Quad.hh>

#include "../../RenderCallback.hh"
#include "../../RenderPipeline.hh"
#include "../../RenderScene.hh"
#include "TemporalResolveStage.hh"


void glow::pipeline::PostprocessingStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback&)
{
    PIPELINE_PROFILE("Postprocessing Stage");

    mAllocTargetLdr = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGB8, {ctx.camData.resolution}});

    if (!mShaderRegistered)
    {
        registerShader(ctx, mShaderPostprocessing);
        mShaderRegistered = true;
    }

    {
        auto fbo = mFboLdr->bind();
        fbo.attachColor("fLdr", mAllocTargetLdr);
        fbo.setViewport(ctx.camData.resolution);

        auto shader = mShaderPostprocessing->use();
        shader.setTexture("uInput", mStageTemporalResolve->getTargetHdr(ctx));
        shader.setTexture("uBloom", mStageTemporalResolve->getTargetBloom());

        mVaoQuad->bind().draw();
    }
}

void glow::pipeline::PostprocessingStage::onBeginNewFrame(RenderPipeline& pipeline)
{
    if (mAllocTargetLdr)
    {
        pipeline.getTexPoolRect().free(&mAllocTargetLdr);
    }
}

glow::pipeline::PostprocessingStage::PostprocessingStage(SharedTemporalResolveStage const& trStage) : mStageTemporalResolve(trStage)
{
    registerDependency(mStageTemporalResolve);

    mFboLdr = Framebuffer::create();

    mShaderPostprocessing
        = Program::createFromFiles({"glow-pipeline/internal/pass/postprocessing/postprocessing.fsh", "glow-pipeline/internal/fullscreen.vsh"});

    mVaoQuad = glow::geometry::Quad<>().generate();
}
