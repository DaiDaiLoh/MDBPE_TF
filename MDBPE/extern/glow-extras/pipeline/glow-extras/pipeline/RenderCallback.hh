#pragma once

#include <vector>

#include <glow/common/shared.hh>
#include <glow/fwd.hh>
#include <glow/objects/Program.hh>

#include "fwd.hh"
#include "lights/Light.hh"
#include "stages/RenderStage.hh"

namespace glow
{
namespace pipeline
{
struct CameraData;

struct RenderContext
{
    RenderScene& scene;
    RenderStage const& stage;  ///< The currently active render stage
    StageCamera const& cam;    ///< The pipeline-wide camera
    CameraData const& camData; ///< Fast access to cam.getCameraData()
    RenderPipeline& pipeline;  ///< The pipeline invoking the current render task

    /// Shorthand to use a shader and properly prepare it within the current stage and pipeline
    UsedProgram useProgram(glow::SharedProgram const& program) const
    {
        auto shader = program->use();
        stage.prepareShader(*this, program, shader);
        return shader;
    }
};

struct LightCallback
{
    std::vector<Light>& lights;

    /// Tube light
    void addLight(tg::pos3 const& posA, tg::pos3 const& posB, tg::color3 const& color, float size, float radius, int maskId = 0)
    {
        lights.emplace_back(posA, posB, color, size, radius, maskId);
    }

    /// Sphere light
    void addLight(tg::pos3 const& pos, tg::color3 const& color, float size, float radius, int maskId = 0)
    {
        lights.emplace_back(pos, color, size, radius, maskId);
    }

    /// Point light
    void addLight(tg::pos3 const& pos, tg::color3 const& color, float radius, int maskId = 0)
    {
        lights.emplace_back(pos, color, radius, maskId);
    }
};

class RenderCallback
{
public:
    // -- Stages --

    /// Render the given stage
    /// This method can be overriden to customize interactions with specific stages,
    /// but it is recommended to only override the onRenderPass methods below instead
    virtual void onRenderStage(RenderContext const& ctx)
    {
        switch (ctx.stage.getType())
        {
        case StageType::DepthPre:
            onRenderDepthPrePass(ctx);
            break;
        case StageType::Shadow:
            onRenderShadowPass(ctx);
            break;
        case StageType::Opaque:
            onRenderOpaquePass(ctx);
            break;
        case StageType::Transparent:
            onRenderTransparentPass(ctx);
            break;
        default:
            break;
        }
    }

    /// Render the depth pre pass
    ///
    /// No fragment shader required.
    /// Optional shader usage:
    ///
    /// #glow pipeline depthPre
    /// #include <glow-pipeline/pass/depthpre/depthPre.glsl>
    ///
    /// outputDepthPreGeometry(fragmentDepth);
    ///
    virtual void onRenderDepthPrePass(RenderScene const&, RenderStage const&, CameraData const&) {}
    virtual void onRenderDepthPrePass(RenderContext const& ctx) { onRenderDepthPrePass(ctx.scene, ctx.stage, ctx.camData); }

    /// Render the shadow pass
    /// Note: If you have geometry that does not require special shaders, consider using
    /// the simpler onPerformShadowPass() and onPerformShadowPassInstanced() instead
    ///
    /// Shader usage:
    ///
    /// #include <glow-pipeline/pass/shadow/shadow.glsl>
    ///
    /// outputShadowVertex(modelSpacePosition, cascadeIndex);
    ///
    /// (Optionally, add a discarding fragment shader)
    /// (Note that the code above can be a vertex or geometry shader)
    ///
    virtual void onRenderShadowPass(RenderScene const&, RenderStage const&, CameraData const&) {}
    virtual void onRenderShadowPass(RenderContext const& ctx) { onRenderShadowPass(ctx.scene, ctx.stage, ctx.camData); }


    /// Render the forward opaque pass
    /// See opaquePass.glsl for useful helpers and options
    ///
    /// Shader usage:
    ///
    /// #glow pipeline opaque
    /// #include <glow-pipeline/pass/opaque/opaquePass.glsl>
    ///
    /// FOREACH_LIGHT(fragmentDepth, i) {
    ///     LightData light = getLight(i);
    ///     // .. perform lighting ..
    /// }
    ///
    /// outputOpaqueGeometry(finalColor, [velocity]);
    ///
    virtual void onRenderOpaquePass(RenderScene const&, RenderStage const&, CameraData const&) {}
    virtual void onRenderOpaquePass(RenderContext const& ctx) { onRenderOpaquePass(ctx.scene, ctx.stage, ctx.camData); }

    /// Register all lights that are visible this frame
    virtual void onGatherLights(LightCallback&) {}

    /// Render the transparent pass
    /// See oitPass.glsl for useful helpers
    ///
    /// Shader usage:
    ///
    /// #glow pipeline transparent
    /// #include <glow-pipeline/pass/transparent/oitPass.glsl>
    ///
    /// outputOitGeometry(fragmentZ, color, alpha, offset, [blurriness]);
    ///
    virtual void onRenderTransparentPass(RenderScene const&, RenderStage const&, CameraData const&) {}
    virtual void onRenderTransparentPass(RenderContext const& ctx) { onRenderTransparentPass(ctx.scene, ctx.stage, ctx.camData); }

public:
    // -- Simple shadow interface --

    /// Perform the shadow pass
    /// Draw all geometry that should cast shadows, use the provided shader and set the uModel uniform to the models
    /// model matrix The vertex array must provide the aPosition attribute
    ///
    /// Usage:
    ///
    /// onPerformShadowPass(..., shader)
    /// {
    ///     for (model : models)
    ///     {
    ///         shader.setUniform("uModel", model.modelMatrix);
    ///         model->bind().draw();
    ///     }
    /// }
    virtual void onPerformShadowPass(RenderScene const&, RenderStage const&, CameraData const&, UsedProgram&) {}
    virtual void onPerformShadowPass(RenderContext const& ctx, UsedProgram& shader)
    {
        onPerformShadowPass(ctx.scene, ctx.stage, ctx.camData, shader);
    }

    /// Draw all instanced geometry that should cast shadows, use the provided shader and draw a vertex array with column-divided model matrices
    /// The vertex array must have the divisor set to 1 and provide the aPosition attribute
    /// The per-instance model matrix column attributes are named aModelC0, .., aModelC3
    ///
    /// The exact specification can be gathered from shader/glow-pipeline/internal/pass/shadow/geometryDepthOnlyInstanced.vsh
    virtual void onPerformShadowPassInstanced(RenderScene const&, RenderStage const&, CameraData const&, UsedProgram&) {}
    virtual void onPerformShadowPassInstanced(RenderContext const& ctx, UsedProgram& shader)
    {
        onPerformShadowPassInstanced(ctx.scene, ctx.stage, ctx.camData, shader);
    }

public:
    virtual ~RenderCallback() {}
};

}
}
