#include "RenderStage.hh"

#include <glow/common/scoped_gl.hh>

#include "../RenderCallback.hh"
#include "../RenderPipeline.hh"

void glow::pipeline::RenderStage::registerDependency(const SharedRenderStage& stage) { mDependencies.push_back(stage); }

void glow::pipeline::RenderStage::execute(StageCamera const& cam, glow::pipeline::RenderPipeline& pipeline, glow::pipeline::RenderScene& scene, glow::pipeline::RenderCallback& rc)
{
    if (mRenderedThisFrame)
        return;

    for (auto const& dependency : mDependencies)
        dependency->execute(cam, pipeline, scene, rc);

#ifdef GLOW_EXTRAS_OPENGL_DEBUG_GROUPS
    GLOW_SCOPED(debugGroup, "RenderStage " + name());
#endif
    onExecuteStage(RenderContext{scene, *this, cam, cam.getCameraData(), pipeline}, rc);
    mRenderedThisFrame = true;
}

void glow::pipeline::RenderStage::beginNewFrame(RenderPipeline& pipeline)
{
    for (auto const& dependency : mDependencies)
        dependency->beginNewFrame(pipeline);

    mRenderedThisFrame = false;
    onBeginNewFrame(pipeline);
}

void glow::pipeline::RenderStage::registerShader(RenderContext const& ctx, const glow::SharedProgram& shader) const
{
    ctx.pipeline.setProjectionInfoUbo(shader);
    ctx.pipeline.setSceneUbo(shader);
}

void glow::pipeline::RenderStage::prepareShader(RenderContext const& ctx, const glow::SharedProgram& shader, glow::UsedProgram& usedShader) const
{
    if (mRegisteredShaders.find(shader.get()) == mRegisteredShaders.end())
    {
        registerShader(ctx, shader);
        mRegisteredShaders.insert(shader.get());
    }

    uploadShaderData(ctx, usedShader);
}
