#include "OpaqueStage.hh"

#include <array>
#include <cassert>

#include <glow/common/scoped_gl.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Sampler.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/Texture2DArray.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow-extras/geometry/FullscreenTriangle.hh>

#include "../../RenderCallback.hh"
#include "../../RenderPipeline.hh"
#include "../../RenderScene.hh"
#include "AOStage.hh"
#include "DepthPreStage.hh"
#include "ShadowStage.hh"

#include "shader/glow-pipeline/internal/common/globals.hh"

void glow::pipeline::OpaqueStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback& rc)
{
    PIPELINE_PROFILE("Opaque Stage");

    auto const& res = ctx.camData.resolution;

    mAllocTargetHdr = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGB16F, res});
    mAllocTargetVelocity = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RG16F, res});

    // Rendering
    {
        auto fbo = mFboHdr->bind();
        fbo.attachColor("fHdr", mAllocTargetHdr);
        fbo.attachColor("fVelocity", mAllocTargetVelocity);
        fbo.attachDepth(mStageDepthPre->getTargetDepth());
        fbo.setViewport(res);

        GLOW_SCOPED(clearColor, ctx.scene.backgroundColor);
        glClear(GL_COLOR_BUFFER_BIT);

        {
            auto velFbo = mFboVelocity->bind();
            velFbo.attachColor("fVelocity", mAllocTargetVelocity);
            velFbo.setViewport(res);

            // Velocity Buffer Init
            GLOW_SCOPED(clearColor, tg::color3::black);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        GLOW_SCOPED(enable, GL_CULL_FACE);
        GLOW_SCOPED(cullFace, GL_BACK);

        // [L/G]EQUAL Depth test
        // The Z-Prepass is optional
        // Only geometry in the Z-Pre will have AO, but this stage can add geometry to the depth buffer
        // Geometry in the Z-Pre has less overdraw, geometry not included does not have to get rendered twice
        GLOW_SCOPED(enable, GL_DEPTH_TEST);
#if GLOW_PIPELINE_ENABLE_REVERSE_Z
        GLOW_SCOPED(depthFunc, GL_GEQUAL); // greater or equal - reverse Z
#else
        GLOW_SCOPED(depthFunc, GL_LEQUAL);
#endif
        rc.onRenderStage(ctx);
    }

    if (ctx.scene.edgeOutline.enabled)
    {
        auto fbo = mFboHdrOnly->bind();
        fbo.attachColor("fHdr", mAllocTargetHdr);
        fbo.setViewport(res);

        GLOW_SCOPED(enable, GL_BLEND);
        GLOW_SCOPED(blendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        auto shader = mShaderEdgeOutline->use();
        shader.setTexture("uDepth", mStageAo->getTargetLinearDepth());
        shader.setTexture("uNormals", mStageAo->getTargetNormals());

        shader.setUniform("uDepthThreshold", ctx.scene.edgeOutline.depthThreshold);
        shader.setUniform("uNormalThreshold", ctx.scene.edgeOutline.normalThreshold);
        shader.setUniform("uColor", ctx.scene.edgeOutline.color);
        shader.setUniform("uInverseResolution", 1.f / tg::comp2(ctx.camData.resolution));

        geometry::FullscreenTriangle::draw();
    }
}

void glow::pipeline::OpaqueStage::onBeginNewFrame(glow::pipeline::RenderPipeline& pipeline)
{
    // This check is only necessary for the very first frame
    if (mAllocTargetHdr)
    {
        pipeline.getTexPoolRect().free(&mAllocTargetHdr);
        pipeline.getTexPoolRect().free(&mAllocTargetVelocity);
    }
}

glow::pipeline::OpaqueStage::OpaqueStage(const SharedDepthPreStage& depthPreStage, SharedShadowStage const& shadowStage, const SharedAOStage& aoStage)
  : mStageDepthPre(depthPreStage), mStageShadow(shadowStage), mStageAo(aoStage)
{
    registerDependency(mStageDepthPre);
    registerDependency(mStageShadow);
    registerDependency(mStageAo);

    mFboHdr = Framebuffer::create();
    mFboHdrOnly = Framebuffer::create();
    mFboVelocity = Framebuffer::create();

    mSamplerAo = Sampler::create();
    mSamplerAo->setFilter(GL_NEAREST, GL_NEAREST);
    mSamplerAo->setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

    mShaderVelocityInit
        = Program::createFromFiles({"glow-pipeline/internal/pass/opaque/velocityInit.vsh", "glow-pipeline/internal/pass/opaque/velocityInit.fsh"});

    mShaderEdgeOutline = Program::createFromFiles({"glow-pipeline/internal/pass/postprocessing/edgeOutline.fsh", "glow-pipeline/internal/"
                                                                                                                 "fullscreenTriangle.vsh"});
}

void glow::pipeline::OpaqueStage::registerShader(RenderContext const& ctx, const glow::SharedProgram& shader) const
{
    RenderStage::registerShader(ctx, shader);
    shader->setShaderStorageBuffer("sClusterLightIndexList", mStageDepthPre->getSsboLightIndices());
    shader->setShaderStorageBuffer("sClusterVisibilities", mStageDepthPre->getSsboClusterVisibilites());
    shader->setShaderStorageBuffer("sLightData", mStageDepthPre->getSsboLightData());
    shader->setUniformBuffer("uShadowLightVPUBO", mStageShadow->getLightVpUbo());
    shader->setUniformBuffer("uShadowCascadeLimitUBO", mStageShadow->getCascadeLimitUbo());
}

void glow::pipeline::OpaqueStage::uploadShaderData(RenderContext const& ctx, glow::UsedProgram& shader) const
{
    shader.setTexture("uSceneAo", mStageAo->getTargetAO(), mSamplerAo);
    shader.setTexture("uShadowMaps", ctx.scene.shadow.cascades);

    shader.setUniform("uClipInfo", ctx.camData.clipInfo);
    shader.setUniform("uClusterDimensions", tg::usize3(ctx.cam.getClusterData().clusterGridSize));
    shader.setUniform("uClusteringScaleBias", ctx.cam.getClusterData().scaleBias);
}
