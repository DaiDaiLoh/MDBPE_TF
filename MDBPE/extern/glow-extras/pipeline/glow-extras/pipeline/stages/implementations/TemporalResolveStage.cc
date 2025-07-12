#include "TemporalResolveStage.hh"

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
#include "LightingCombinationStage.hh"
#include "OITStage.hh"
#include "OpaqueStage.hh"

// Faster, more artifacts
#define USE_QUARTER_RES_BLOOM false

#if USE_QUARTER_RES_BLOOM
#define BLOOM_RES quarterResolution
#define BLOOM_DOWN_RATIO 4.f
#else
#define BLOOM_RES halfResolution
#define BLOOM_DOWN_RATIO 2.f
#endif

void glow::pipeline::TemporalResolveStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback&)
{
    PIPELINE_PROFILE("Temporal Resolve Stage");

    auto const& res = ctx.camData.resolution;
    auto const& bloomRes = ctx.camData.BLOOM_RES;

    mAllocTargetBloom = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGB16F, res});
    mAllocTargetBloomBlurA = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGB16F, bloomRes});
    mAllocTargetBloomBlurB = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_RGB16F, bloomRes});

    auto boundFullTri = geometry::FullscreenTriangle::bindBlankVao();

    {
        PIPELINE_PROFILE("TAA Resolve");

        auto fbo = mFboTemporalResolve->bind();
        fbo.attachColor("fHDR", getTargetHdr(ctx));
        fbo.attachColor("fBloom", mAllocTargetBloom);
        fbo.setViewport(res);

        GLOW_SCOPED(disable, GL_DEPTH_TEST);
        GLOW_SCOPED(disable, GL_CULL_FACE);

        if (!mShaderRegistered)
        {
            registerShader(ctx, mShaderTemporalResolve);
            mShaderRegistered = true;
        }

        auto shader = mShaderTemporalResolve->use();
        shader.setTexture("uShadedOpaque", mStageLightingCombination->getTargetHdr());
        shader.setTexture("uVelocity", mStageOpaque->getTargetVelocity());

        shader.setUniform("uEnableHistoryRejection", ctx.cam.isRejectingHistory());

        // Choose previous history buffer target based on frame parity
        if (ctx.cam.getTemporalData().evenFrame)
            shader.setTexture("uTemporalHistory", ctx.cam.getTemporalData().historyBufferOdd);
        else
            shader.setTexture("uTemporalHistory", ctx.cam.getTemporalData().historyBufferEven);

        shader.setUniform("uJitter", ctx.cam.getTemporalData().subpixelJitter);
        shader.setUniform("uDiscardHistory", false);

        boundFullTri.negotiateBindings();
        geometry::FullscreenTriangle::drawTriangle();
    }

    // Bloom processing
    {
        PIPELINE_PROFILE("Bloom processing");

        GLOW_SCOPED(clearColor, 0, 0, 0, 0);

        // Downsample
        {
            auto fbo = mFboBloomBlurA->bind();
            fbo.attachColor("fBloom", mAllocTargetBloomBlurA);
            fbo.setViewport(bloomRes);

            glClear(GL_COLOR_BUFFER_BIT);

            auto shader = mShaderDownsample->use();
            shader.setTexture("uInput", mAllocTargetBloom);
            shader.setUniform("uInverseRatio", BLOOM_DOWN_RATIO);

            boundFullTri.negotiateBindings();
            geometry::FullscreenTriangle::drawTriangle();
        }

        // Kawase Blur
        {
            // TODO: Clean this up (loop or similar)
            auto shader = mShaderKawaseBlur->use();

            // 0: A -> B
            {
                auto fbo = mFboBloomBlurB->bind();
                fbo.attachColor("fBloom", mAllocTargetBloomBlurB);
                fbo.setViewport(bloomRes);

                glClear(GL_COLOR_BUFFER_BIT);

                shader.setTexture("uInput", mAllocTargetBloomBlurA);
                shader.setUniform("uDistance", 0.f);
                geometry::FullscreenTriangle::drawTriangle();
            }

            // 1: B -> A
            {
                auto fbo = mFboBloomBlurA->bind();
                fbo.setViewport(bloomRes);

                shader.setTexture("uInput", mAllocTargetBloomBlurB);
                shader.setUniform("uDistance", 1.f);
                geometry::FullscreenTriangle::drawTriangle();
            }

            // 2: A -> B
            {
                auto fbo = mFboBloomBlurB->bind();
                fbo.setViewport(bloomRes);

                shader.setTexture("uInput", mAllocTargetBloomBlurA);
                shader.setUniform("uDistance", 2.f);
                geometry::FullscreenTriangle::drawTriangle();
            }

            // 2: B -> A
            {
                auto fbo = mFboBloomBlurA->bind();
                fbo.setViewport(bloomRes);

                shader.setTexture("uInput", mAllocTargetBloomBlurB);
                shader.setUniform("uDistance", 2.f);
                geometry::FullscreenTriangle::drawTriangle();
            }

            // 3: A -> B
            {
                auto fbo = mFboBloomBlurB->bind();
                fbo.setViewport(bloomRes);

                shader.setTexture("uInput", mAllocTargetBloomBlurA);
                shader.setUniform("uDistance", 3.f);
                geometry::FullscreenTriangle::drawTriangle();
            }
        }

        // Upsample
        {
            auto fbo = mFboBloom->bind();
            fbo.attachColor("fBloom", mAllocTargetBloom);
            fbo.setViewport(res);

            auto shader = mShaderDownsample->use();
            shader.setTexture("uInput", mAllocTargetBloomBlurB);
            shader.setUniform("uInverseRatio", 1.f / BLOOM_DOWN_RATIO);
            geometry::FullscreenTriangle::drawTriangle();
        }
    }
}

void glow::pipeline::TemporalResolveStage::onBeginNewFrame(glow::pipeline::RenderPipeline& pipeline)
{
    if (mAllocTargetBloom)
    {
        pipeline.getTexPoolRect().free(&mAllocTargetBloom);
        pipeline.getTexPoolRect().free(&mAllocTargetBloomBlurA);
        pipeline.getTexPoolRect().free(&mAllocTargetBloomBlurB);
    }
}

glow::pipeline::TemporalResolveStage::TemporalResolveStage(SharedOpaqueStage const& opaqueStage, SharedLightingCombinationStage const& lightingCombinationStage)
  : mStageOpaque(opaqueStage), mStageLightingCombination(lightingCombinationStage)
{
    registerDependency(mStageOpaque);
    registerDependency(mStageLightingCombination);

    // FBOs
    {
        mFboTemporalResolve = Framebuffer::create();
        mFboBloom = Framebuffer::create();
        mFboBloomBlurA = Framebuffer::create();
        mFboBloomBlurB = Framebuffer::create();
    }

    // Shaders
    {
        auto constexpr fullscreenPath = "glow-pipeline/internal/fullscreenTriangle.vsh";

        mShaderTemporalResolve = Program::createFromFiles({"glow-pipeline/internal/pass/temporalresolve/taa.fsh", fullscreenPath});
        mShaderKawaseBlur = Program::createFromFiles({"glow-pipeline/internal/pass/lightingcombination/kawaseBlur.fsh", fullscreenPath});
        mShaderDownsample = Program::createFromFiles({"glow-pipeline/internal/utility/downsample.fsh", fullscreenPath});
    }

    geometry::FullscreenTriangle::init();
}
