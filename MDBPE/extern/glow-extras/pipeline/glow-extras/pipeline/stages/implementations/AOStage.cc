#include "AOStage.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/data/TextureData.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Sampler.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/TextureRectangle.hh>
#include <glow/objects/UniformBuffer.hh>

#include <glow-extras/geometry/FullscreenTriangle.hh>

#include "../../RenderCallback.hh"
#include "../../RenderPipeline.hh"
#include "../../RenderScene.hh"
#include "../../Settings.hh"
#include "AO/AOPerPassBuffer.hh"
#include "DepthPreStage.hh"

namespace glow
{
namespace pipeline
{
namespace detail
{
static std::array<std::string, 8> const fboOutputNames = {{
    "fOut0",
    "fOut1",
    "fOut2",
    "fOut3",
    "fOut4",
    "fOut5",
    "fOut6",
    "fOut7",
}};
}
}
}


void glow::pipeline::AOStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback&)
{
    PIPELINE_PROFILE("AO Stage");

    auto const& scene = ctx.scene;
    auto& pipeline = ctx.pipeline;

    auto const& res = ctx.camData.resolution;
    auto const& quarterRes = ctx.camData.quarterResolution;

    // Update UBOs
    {
        PIPELINE_PROFILE("UBO Update");
        mBufferAOGlobal.setResolutionConstants(res);
        mBufferAOGlobal.setDepthData(ctx.camData.camNearPlane, ctx.camData.camFarPlane, ctx.camData.tanHalfFovX, ctx.camData.tanHalfFovY);
        mBufferAOGlobal.setAOParameters(scene.ao.radius, scene.ao.metersToViewSpaceUnits, ctx.camData.tanHalfFovY, static_cast<float>(res.height),
                                        scene.ao.sharpness, scene.ao.powerExponent, scene.ao.bias, scene.ao.smallScaleIntensity, scene.ao.largeScaleIntensity);
        mUboAOGlobal->bind().setData(mBufferAOGlobal.data, GL_STREAM_DRAW);
    }

    // Prepare Targets and FBOs
    {
        PIPELINE_PROFILE("Target and FBO preparation");

        mTargetLinearDepth = pipeline.getTexPoolRect().allocAtLeast({GL_R32F, {res}});
        mTargetViewDepthArray = pipeline.getTexPool2DArray().allocAtLeast({GL_R16F, {quarterRes.width, quarterRes.height, 16}, 1});
        mTargetNormals = pipeline.getTexPool2D().allocAtLeast({GL_RGB8, {res}, 1});
        mTargetAoArray = pipeline.getTexPool2DArray().allocAtLeast({GL_R8, {quarterRes.width, quarterRes.height, 16}, 1});
        mTargetAOZ = pipeline.getTexPool2D().allocAtLeast({GL_RG16F, {res}, 1});
        mTargetAOZ2 = pipeline.getTexPool2D().allocAtLeast({GL_RG16F, {res}, 1});
        mTargetAoFinal = pipeline.getTexPoolRect().allocAtLeast({GL_RG16F, {res}});

        // ViewDepthArray FBO
        for (auto pass = 0u; pass < 2; ++pass)
        {
            auto fbo = mFbosViewDepthArray.at(pass)->bind();
            for (auto target = 0u; target < 8; ++target)
            {
                auto const layerIndex = static_cast<int>(pass * 8 + target);
                fbo.attachColor(detail::fboOutputNames.at(target), mTargetViewDepthArray, 0, layerIndex);
            }
        }
    }

    auto fsTriVao = geometry::FullscreenTriangle::bindBlankVao();

    {
        PIPELINE_PROFILE("Raw AO");

        // Linearize Depth
        {
            auto fbo = mFboLinDepth->bind();
            fbo.attachColor("fOut", mTargetLinearDepth);
            fbo.setViewport(res);

            auto shader = mShaderLinearizeDepth->use();
            shader.setTexture("uInputDepth", mStageDepthPre->getTargetDepth());

#if GLOW_PIPELINE_ENABLE_REVERSE_Z
            shader.setUniform("uNearPlane", ctx.camData.camNearPlane);
#endif

            geometry::FullscreenTriangle::drawTriangle();
        }

        // Deinterleave Depth
        {
            auto shader = mShaderDeinterleavedDepth->use();
            shader.setTexture("uInputLinDepth", mTargetLinearDepth);

            for (auto pass = 0u; pass < 2; ++pass)
            {
                mShaderDeinterleavedDepth->setUniformBuffer("uCSSAOPerPass", mUbosAOPerPass.at(pass * 8));
                auto fbo = mFbosViewDepthArray.at(pass)->bind();
                fbo.setViewport(quarterRes);

                geometry::FullscreenTriangle::drawTriangle();
            }
        }

        // Reconstruct Normals
        {
            auto fbo = mFboNormalReconstruct->bind();
            fbo.attachColor("fOut", mTargetNormals);
            fbo.setViewport(res);

            auto shader = mShaderReconstructNormals->use();
            shader.setTexture("uInputDepth", mTargetLinearDepth);

            geometry::FullscreenTriangle::drawTriangle();
        }

        // Coarse AO
        {
            auto fbo = mFboLayeredCoarseAo->bind();
            fbo.attachColor("fOut", mTargetAoArray, 0, -1);
            fbo.setViewport(quarterRes);

            auto shader = mShaderCoarseAO->use();
            shader.setTexture("uInputNormals", mTargetNormals);
            shader.setTexture("uInputDepth", mTargetViewDepthArray);

            for (auto slice = 0u; slice < 16; ++slice)
            {
                mShaderCoarseAO->setUniformBuffer("uCSSAOPerPass", mUbosAOPerPass.at(slice));

                geometry::FullscreenTriangle::drawPoint();
            }
        }
    }


    // Blur
    {
        PIPELINE_PROFILE("Blur");
        // Reinterleaved AO
        {
            auto fbo = mFboAOZ2->bind();
            fbo.attachColor("fOut", mTargetAOZ2);
            fbo.setViewport(res);

            auto shader = mShaderReinterleavedAO->use();
            shader.setTexture("uInputAO", mTargetAoArray);
            shader.setTexture("uInputDepth", mTargetLinearDepth);

            geometry::FullscreenTriangle::drawTriangle();
        }

        // Blur X
        {
            auto fbo = mFboAOZ1->bind();
            fbo.attachColor("fOut", mTargetAOZ);
            fbo.setViewport(res);

            auto shader = mShaderBlurX->use();
            shader.setTexture("uInputNearest", mTargetAOZ2, mSamplerNearest);
            shader.setTexture("uInputLinear", mTargetAOZ2, mSamplerLinear);

            geometry::FullscreenTriangle::drawTriangle();
        }

        // Blur Y
        {
            auto fbo = mFboAOZ2->bind();
            fbo.attachColor("fOut", mTargetAoFinal);
            fbo.setViewport(res);

            auto shader = mShaderBlurY->use();
            shader.setTexture("uInputNearest", mTargetAOZ, mSamplerNearest);
            shader.setTexture("uInputLinear", mTargetAOZ, mSamplerLinear);

            geometry::FullscreenTriangle::drawTriangle();
        }
    }
}

void glow::pipeline::AOStage::onBeginNewFrame(glow::pipeline::RenderPipeline& pipeline)
{
    if (mTargetLinearDepth)
    {
        pipeline.getTexPool2D().free(&mTargetNormals);
        pipeline.getTexPool2D().free(&mTargetAOZ);
        pipeline.getTexPool2D().free(&mTargetAOZ2);

        pipeline.getTexPool2DArray().free(&mTargetViewDepthArray);
        pipeline.getTexPool2DArray().free(&mTargetAoArray);

        pipeline.getTexPoolRect().free(&mTargetLinearDepth);
        pipeline.getTexPoolRect().free(&mTargetAoFinal);
    }
}

glow::pipeline::AOStage::AOStage(SharedDepthPreStage const& depthPreStage) : mStageDepthPre(depthPreStage)
{
    registerDependency(mStageDepthPre);

    mFboLinDepth = Framebuffer::create();
    mFboNormalReconstruct = Framebuffer::create();
    mFboLayeredCoarseAo = Framebuffer::create();
    mFboAOZ1 = Framebuffer::create();
    mFboAOZ2 = Framebuffer::create();

    for (auto it = mFbosViewDepthArray.begin(); it != mFbosViewDepthArray.end(); ++it)
        *it = Framebuffer::create();

    mUboAOGlobal = UniformBuffer::create();
    mUboAOGlobal->bind().setData(mBufferAOGlobal.data);

    for (auto sliceIndex = 0u; sliceIndex < 16; ++sliceIndex)
    {
        mUbosAOPerPass[sliceIndex] = UniformBuffer::create();

        detail::AOPerPassBuffer buffer;

        buffer.setOffset(sliceIndex % 4, sliceIndex / 4);
        buffer.setJitter(tg::vec4(1, 0, 1, 1));
        buffer.setSliceIndex(sliceIndex);

        mUbosAOPerPass[sliceIndex]->bind().setData(buffer.data, GL_STREAM_DRAW);
    }


    auto const fsTriPath = "glow-pipeline/internal/fullscreenTriangle.vsh";

    mShaderLinearizeDepth = Program::createFromFiles({fsTriPath, "glow-pipeline/internal/pass/ao/cssao/linearizeDepth.fsh"});
    mShaderLinearizeDepth->setUniformBuffer("uCSSAOGlobals", mUboAOGlobal);

    mShaderDeinterleavedDepth = Program::createFromFiles({fsTriPath, "glow-pipeline/internal/pass/ao/cssao/deinterleavedDepth.fsh"});
    mShaderDeinterleavedDepth->setUniformBuffer("uCSSAOGlobals", mUboAOGlobal);

    mShaderReconstructNormals = Program::createFromFiles({fsTriPath, "glow-pipeline/internal/pass/ao/cssao/reconstructNormal.fsh"});
    mShaderReconstructNormals->setUniformBuffer("uCSSAOGlobals", mUboAOGlobal);

    mShaderCoarseAO = Program::createFromFiles({"glow-pipeline/internal/pass/ao/cssao/coarseAO.fsh", "glow-pipeline/internal/pass/ao/cssao/coarseAO.gsh",
                                                "glow-pipeline/internal/pass/ao/cssao/coarseAO.vsh"});
    mShaderCoarseAO->setUniformBuffer("uCSSAOGlobals", mUboAOGlobal);

    mShaderReinterleavedAO = Program::createFromFiles({fsTriPath, "glow-pipeline/internal/pass/ao/cssao/reinterleavedAO.fsh"});

    mShaderBlurX = Program::createFromFiles({fsTriPath, "glow-pipeline/internal/pass/ao/cssao/blurX.fsh"});
    mShaderBlurX->setUniformBuffer("uCSSAOGlobals", mUboAOGlobal);

    mShaderBlurY = Program::createFromFiles({fsTriPath, "glow-pipeline/internal/pass/ao/cssao/blurY.fsh"});
    mShaderBlurY->setUniformBuffer("uCSSAOGlobals", mUboAOGlobal);

    geometry::FullscreenTriangle::init();

    // TODO: Remove this
    // There is a strange lifetime / aliasing issue going on with Samplers
    //
    // It occurs if the pipeline is created, destroyed and then re-created with one of
    // the pipeline-internal samplers surviving (in any user-side OpaquePass shader for example,
    // which stores SharedSamplers).
    //
    // The issue occurs now: One of the freshly ::create()d samplers will alias to the surviving one
    // As in, the new SharedSampler with have the same mObjectName as the old, surviving one.
    //
    // The surviving SharedSampler will then be replaced with the "new", aliased one, expiring its shared_ptr and
    // thus glDeleteSamplers() that sampler, which has the same name as the fresh one, invalidating "both" (really the same one)
    // From then on, using the fresh and now invalid sampler will result in GL_INVALID_OPERATION errors.
    //
    // This line fixes the aliasing issue, but it should get fixed in GLOW instead
    auto magicSampler = Sampler::create();

    mSamplerLinear = Sampler::create();
    mSamplerLinear->setFilter(GL_LINEAR, GL_LINEAR);

    mSamplerNearest = Sampler::create();
    mSamplerNearest->setFilter(GL_NEAREST, GL_NEAREST);
}
