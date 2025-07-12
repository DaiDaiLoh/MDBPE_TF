#include "DepthPreStage.hh"

#include <algorithm>
#include <memory>

#include <glow/common/scoped_gl.hh>
#include <glow/data/TextureData.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Sampler.hh>
#include <glow/objects/ShaderStorageBuffer.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow-extras/geometry/FullscreenTriangle.hh>

#include "../../RenderCallback.hh"
#include "../../RenderPipeline.hh"
#include "../../RenderScene.hh"
#include "../../lights/Light.hh"

#include "shader/glow-pipeline/internal/common/globals.hh"

namespace
{
auto constexpr maxLightsPerCluster = GLOW_PIPELINE_MAX_LIGHTS_PER_CLUSTER;

// These structs are just for sizeof() calls
struct ClusterAabb
{
    glow::std140vec4 min;
    glow::std140vec4 max;
};

struct SsboGlobalIndexCountData
{
    glow::std140uint count = 0;
};

struct SsboClusterVisibility
{
    glow::std140uint offset;
    glow::std140uint count;
};
}

void glow::pipeline::DepthPreStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback& rc)
{
    PIPELINE_PROFILE("Depth Pre Stage");

    auto const& res = ctx.camData.resolution;

    mAllocTargetDepth = ctx.pipeline.getTexPoolRect().allocAtLeast({GL_DEPTH_COMPONENT32F, {res}});

    // Light compression and upload
    {
        PIPELINE_PROFILE("Light upload");

        mGpuLights.clear();
        LightCallback callback{mGpuLights};
        rc.onGatherLights(callback);
        mSsboLightData->bind().setData(mGpuLights, GL_STREAM_DRAW);
    }

    // Depth prepass
    {
        PIPELINE_PROFILE("Depth prepass");

        auto fbo = mFboDepth->bind();
        fbo.attachDepth(mAllocTargetDepth);
        fbo.setViewport(res);

#if GLOW_PIPELINE_ENABLE_REVERSE_Z
        GLOW_SCOPED(clearDepth, 0.0);
        GLOW_SCOPED(depthFunc, GL_GREATER);
#endif

        glClear(GL_DEPTH_BUFFER_BIT);

        GLOW_SCOPED(enable, GL_CULL_FACE);
        GLOW_SCOPED(cullFace, GL_BACK);

        GLOW_SCOPED(enable, GL_DEPTH_TEST);
        GLOW_SCOPED(depthMask, GL_TRUE);

        GLOW_SCOPED(colorMask, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        rc.onRenderStage(ctx);
    }

    // Resize SSBOs on cluster amount changes
    auto const& newClusterAmount = ctx.cam.getClusterData().clusterAmount;
    if (newClusterAmount != mCachedClusterAmount)
    {
        PIPELINE_PROFILE("SSBO resize");

        mCachedClusterAmount = newClusterAmount;
        mSsboClusterAabbs->bind().setData(sizeof(ClusterAabb) * mCachedClusterAmount, nullptr, GL_STATIC_DRAW);
        mSsboClusterVisibilities->bind().setData(sizeof(SsboClusterVisibility) * mCachedClusterAmount, nullptr, GL_STATIC_DRAW);

        // The worst case for cluster-light intersections is maxLightsPerCluster * clusterAmount
        // While that amount of load is rare, this buffer costs just 4 byte per entry
        auto const maxLightIntersections = static_cast<unsigned>((maxLightsPerCluster * mCachedClusterAmount));
        mSsboLightIndexList->bind().setData(sizeof(std140uint) * maxLightIntersections, nullptr, GL_STATIC_DRAW);
    }

    // Recalculate Cluster AABBs on projection changes
    if (ctx.camData.projectionChangedThisFrame)
    {
        PIPELINE_PROFILE("AABB recalculation dispatch");

        auto shader = mComputeClusterAabbs->use();

        shader.setUniform("uProjInv", ctx.camData.cleanProjInverse);
        shader.setUniform("uNearPlane", ctx.camData.camNearPlane);
        shader.setUniform("uFarPlane", ctx.camData.camFarPlane);
        shader.setUniform("uResolution", ctx.camData.resolution);

        shader.compute(ctx.cam.getClusterData().clusterGridSize);
    }

    // Perform Light Cluster assignment
    {
        PIPELINE_PROFILE("Light assignment dispatch");

        auto shader = mComputeClusterLights->use();
        shader.setUniform("uView", ctx.camData.view);
        shader.compute(ctx.cam.getClusterData().lightAssignmentDispatch);
    }
}

void glow::pipeline::DepthPreStage::onBeginNewFrame(glow::pipeline::RenderPipeline& pipeline)
{
    if (mAllocTargetDepth)
    {
        pipeline.getTexPoolRect().free(&mAllocTargetDepth);
    }
}

glow::pipeline::DepthPreStage::DepthPreStage()
{
    mGpuLights.reserve(100);

    // Raw Depth Buffer
    mFboDepth = Framebuffer::create();

    mSsboLightIndexList = ShaderStorageBuffer::create();
    mSsboClusterVisibilities = ShaderStorageBuffer::create();
    mSsboClusterAabbs = ShaderStorageBuffer::create();
    mSsboLightData = ShaderStorageBuffer::create();
    mSsboGlobalIndexCount = ShaderStorageBuffer::create();
    mSsboGlobalIndexCount->bind().setData(SsboGlobalIndexCountData{}, GL_STATIC_DRAW);

    mComputeClusterAabbs = Program::createFromFile("glow-pipeline/internal/pass/depthpre/clusterAabbAssignment.csh");
    mComputeClusterAabbs->setShaderStorageBuffer("sClusterAABBs", mSsboClusterAabbs);

    mComputeClusterLights = Program::createFromFile("glow-pipeline/internal/pass/depthpre/clusterLightAssignment.csh");
    mComputeClusterLights->setShaderStorageBuffer("sClusterAABBs", mSsboClusterAabbs);
    mComputeClusterLights->setShaderStorageBuffer("sLightData", mSsboLightData);
    mComputeClusterLights->setShaderStorageBuffer("sClusterLightIndexList", mSsboLightIndexList);
    mComputeClusterLights->setShaderStorageBuffer("sClusterVisibilities", mSsboClusterVisibilities);
    mComputeClusterLights->setShaderStorageBuffer("sGlobalIndexCount", mSsboGlobalIndexCount);

    geometry::FullscreenTriangle::init();
}

glow::optional<tg::pos3> glow::pipeline::DepthPreStage::query3DPosition(StageCamera const& cam, tg::ipos2 pixel) const
{
    if (pixel.x >= cam.getCameraData().resolution.width || pixel.y >= cam.getCameraData().resolution.height)
        return {};

    // rescale position
    pixel.y = cam.getCameraData().resolution.height - pixel.y - 1;

    float d;

    {
        auto fb = mFboDepth->bind();
        glReadPixels(static_cast<int>(pixel.x), static_cast<int>(pixel.y), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d);
    }

    if (d < 1.f)
    {
        // unproject (with viewport coords!)
        tg::vec4 v{pixel.x / float(cam.getCameraData().resolution.width) * 2 - 1, pixel.y / float(cam.getCameraData().resolution.height) * 2 - 1, d * 2 - 1, 1.0};

        v = cam.getCameraData().projInverse * v;
        v /= v.w;
        v = cam.getCameraData().viewInverse * v;

        return tg::pos3(v);
    }
    else
    {
        return {};
    }
}
