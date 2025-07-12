#include "OITStage.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/data/TextureData.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/Texture2DMultisample.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow-extras/geometry/Quad.hh>

#include "../../RenderCallback.hh"
#include "../../RenderPipeline.hh"
#include "DepthPreStage.hh"

#include "shader/glow-pipeline/internal/common/globals.hh"

void glow::pipeline::OITStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback& rc)
{
    PIPELINE_PROFILE("OIT Stage");

    auto const& res = ctx.camData.resolution;

    mAllocTargetAccuA = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGBA16F, {res}});
    mAllocTargetAccuB = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_R16F, {res}});
    mAllocTargetDistortion = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGB16F, {res}});

    // Non-MS OIT pass
    {
        auto fbo = mFboTbuffer->bind();
        fbo.attachColor("fAccuA", mAllocTargetAccuA);
        fbo.attachColor("fAccuB", mAllocTargetAccuB);
        fbo.attachColor("fDistortion", mAllocTargetDistortion);
        fbo.attachDepth(mStageDepthPre->getTargetDepth());
        fbo.setViewport(res);

        // Clear the TBuffer with A = 1 (revealage)
        GLOW_SCOPED(clearColor, 0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Special blending function
        GLOW_SCOPED(enable, GL_BLEND);
        glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);

        // Back face culling
        GLOW_SCOPED(enable, GL_CULL_FACE);

        // Read-only depth test
        GLOW_SCOPED(enable, GL_DEPTH_TEST);
#if GLOW_PIPELINE_ENABLE_REVERSE_Z
        GLOW_SCOPED(depthFunc, GL_GREATER); // reverse Z
#endif
        GLOW_SCOPED(depthMask, GL_FALSE);

        rc.onRenderStage(ctx);
    }
}

void glow::pipeline::OITStage::onBeginNewFrame(glow::pipeline::RenderPipeline& pipeline)
{
    // This check is only necessary for the very first frame
    if (mAllocTargetAccuA)
    {
        pipeline.getTexPoolRect().free(&mAllocTargetAccuA);
        pipeline.getTexPoolRect().free(&mAllocTargetAccuB);
        pipeline.getTexPoolRect().free(&mAllocTargetDistortion);
    }
}

glow::pipeline::OITStage::OITStage(SharedDepthPreStage const& depthPreStage) : mStageDepthPre(depthPreStage)
{
    registerDependency(mStageDepthPre);

    mFboTbuffer = Framebuffer::create();
}

void glow::pipeline::OITStage::uploadShaderData(RenderContext const&, glow::UsedProgram& shader) const
{
    shader.setTexture("uDepth", mStageDepthPre->getTargetDepth());
}
