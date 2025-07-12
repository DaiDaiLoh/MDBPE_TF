#include "RenderPipeline.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/common/str_utils.hh>
#include <glow/common/thread_local.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/UniformBuffer.hh>
#include <glow/std140.hh>
#include <glow/util/DefaultShaderParser.hh>

#include "RenderCallback.hh"
#include "RenderScene.hh"
#include "Settings.hh"
#include "stages/RenderStage.hh"
#include "stages/implementations/AOStage.hh"
#include "stages/implementations/DepthPreStage.hh"
#include "stages/implementations/LightingCombinationStage.hh"
#include "stages/implementations/OITStage.hh"
#include "stages/implementations/OpaqueStage.hh"
#include "stages/implementations/OutputStage.hh"
#include "stages/implementations/PostprocessingStage.hh"
#include "stages/implementations/ShadowStage.hh"
#include "stages/implementations/TemporalResolveStage.hh"

#include "shader/glow-pipeline/internal/common/globals.hh"

#ifdef GLOW_EXTRAS_EMBED_SHADERS
#include <glow-extras/generated/pipeline_embed_shaders.hh>
#endif

namespace
{
bool _isPipelineInitialized = false;
}

namespace glow
{
namespace pipeline
{
struct GlobalConfigUboData
{
    glow::std140vec4 viewportNearFar;
    glow::std140mat4x4 projectionInverse;
    glow::std140mat4x4 viewInverse;
};

struct RenderSceneUboData
{
    glow::std140vec4 sunColor_sunIntensity;             // vec3 + float
    glow::std140vec4 sunDirection_asIntensity;          // vec3 + float
    glow::std140vec4 asColor_fogDensity;                // vec3 + float
    glow::std140vec4 asHeightFunc_bloomThreshold;       // float x4
    glow::std140vec4 bloomIntensity_exp_gamma_contrast; // float x4
    glow::std140vec4 brightness_sharpen_tonemap_void;   // float + float + bool + void
};
}
}

glow::pipeline::RenderPipeline::RenderPipeline()
{
    mUboGlobalConfig = glow::UniformBuffer::create();
    mUboGlobalConfig->bind().setData(GlobalConfigUboData{});

    mUboRenderScene = glow::UniformBuffer::create();
    mUboRenderScene->bind().setData(RenderSceneUboData{});
}

glow::pipeline::RenderPipeline::~RenderPipeline() { PIPELINE_PROFILE_OUTPUT(); }

void glow::pipeline::RenderPipeline::addStage(const glow::pipeline::SharedRenderStage& stage) { mRenderStages.push_back(stage); }

void glow::pipeline::RenderPipeline::addOutputStage(const glow::pipeline::SharedOutputStage& outputStage)
{
    mRenderStages.push_back(outputStage);
    mOutputStage = outputStage;
}

void glow::pipeline::RenderPipeline::registerDepthStage(const glow::pipeline::SharedDepthPreStage& depthPreStage) { mDepthStage = depthPreStage; }

void glow::pipeline::RenderPipeline::render(StageCamera const& stageCam, glow::pipeline::RenderScene& scene, glow::pipeline::RenderCallback& rc)
{
    PIPELINE_PROFILE("Pipeline::render");

#if GLOW_PIPELINE_ENABLE_REVERSE_Z
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
#endif

    // New Frame Event
    {
        for (auto& s : mRenderStages)
            s->beginNewFrame(*this);
    }


    // Update global UBO
    {
        mUboGlobalConfig->bind().setData(GlobalConfigUboData{tg::vec4(stageCam.getCameraData().resolution.width, stageCam.getCameraData().resolution.height,
                                                                      stageCam.getCameraData().camNearPlane, stageCam.getCameraData().camFarPlane),
                                                             stageCam.getCameraData().projInverse, stageCam.getCameraData().viewInverse},
                                         GL_STREAM_DRAW);

        mUboRenderScene->bind().setData(
            RenderSceneUboData{
                tg::vec4(scene.sun.color, scene.sun.intensity),                  //
                tg::vec4(scene.sun.direction, scene.atmoScatter.intensity),      //
                tg::vec4(scene.atmoScatter.fogColor, scene.atmoScatter.density), //
                tg::vec4(scene.atmoScatter.heightFalloffStart, scene.atmoScatter.heightFalloffEnd, scene.atmoScatter.heightFalloffExponent, scene.bloomThreshold), //
                tg::vec4(scene.bloomIntensity, scene.exposure, scene.gamma, scene.contrast),                 //
                tg::vec4(scene.brightness, scene.sharpenStrength, scene.tonemappingEnabled ? 1.f : 0.f, 0.f) //
            },
            GL_STREAM_DRAW);
    }

    // Execute
    mOutputStage->execute(stageCam, *this, scene, rc);

    // Tex Pool cleanup
    mTexPool2D.cleanUp();
    mTexPool2DArray.cleanUp();
    mTexPoolRect.cleanUp();

#if GLOW_PIPELINE_ENABLE_REVERSE_Z
    // Reset to default values
    glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
#endif
}

void glow::pipeline::RenderPipeline::restrictOutputViewport(const tg::ipos2& offset, const tg::isize2& size)
{
    mOutputStage->setViewport(offset, size);
}

void glow::pipeline::RenderPipeline::clearOutputViewport() { mOutputStage->clearViewport(); }

glow::optional<tg::pos3> glow::pipeline::RenderPipeline::queryPosition3D(StageCamera const& stageCam, tg::ipos2 const& pixel) const
{
    if (!mDepthStage)
    {
        glow::error() << "[Pipeline] Querying a position requires a registered depth stage";
        return {};
    }
    else
        return mDepthStage->query3DPosition(stageCam, pixel);
}

void glow::pipeline::RenderPipeline::freeAllTargets()
{
    // New Frame Event to force target freeing
    for (auto& s : mRenderStages)
        s->beginNewFrame(*this);

    // Tex Pool cleanup with generation cutoff 0
    mTexPool2D.cleanUp(0);
    mTexPool2DArray.cleanUp(0);
    mTexPoolRect.cleanUp(0);
}

void glow::pipeline::RenderPipeline::setProjectionInfoUbo(const glow::SharedProgram& shader) const
{
    shader->setUniformBuffer("uPipelineProjectionInfoUbo", mUboGlobalConfig);
}

void glow::pipeline::RenderPipeline::setSceneUbo(const glow::SharedProgram& shader) const
{
    shader->setUniformBuffer("uPipelineSceneUbo", mUboRenderScene);
}

glow::pipeline::SharedRenderPipeline glow::pipeline::RenderPipeline::createDefaultPipeline()
{
    // Lazy init
    RenderPipeline::GlobalInit();

    SharedRenderPipeline resultingPipeline = std::make_shared<RenderPipeline>();

    SharedShadowStage shadowStage = std::make_shared<ShadowStage>();
    resultingPipeline->addStage(shadowStage);

    SharedDepthPreStage depthPreStage = std::make_shared<DepthPreStage>();
    resultingPipeline->addStage(depthPreStage);
    resultingPipeline->registerDepthStage(depthPreStage);

    SharedAOStage aoStage = std::make_shared<AOStage>(depthPreStage);
    resultingPipeline->addStage(aoStage);

    SharedOpaqueStage opaqueStage = std::make_shared<OpaqueStage>(depthPreStage, shadowStage, aoStage);
    resultingPipeline->addStage(opaqueStage);

    SharedOITStage oitStage = std::make_shared<OITStage>(depthPreStage);
    resultingPipeline->addStage(oitStage);

    SharedLightingCombinationStage lcStage = std::make_shared<LightingCombinationStage>(oitStage, opaqueStage, depthPreStage);
    resultingPipeline->addStage(lcStage);

    SharedTemporalResolveStage trStage = std::make_shared<TemporalResolveStage>(opaqueStage, lcStage);
    resultingPipeline->addStage(trStage);

    SharedPostprocessingStage postprocessingStage = std::make_shared<PostprocessingStage>(trStage);
    resultingPipeline->addStage(postprocessingStage);

    SharedOutputStage outputStage = std::make_shared<OutputStage>(postprocessingStage);
    resultingPipeline->addOutputStage(outputStage);

    return resultingPipeline;
}

void glow::pipeline::RenderPipeline::GlobalInit()
{
    if (_isPipelineInitialized)
        return;

    DefaultShaderParser::registerCustomPragma("pipeline", [](std::string const& instruction, DefaultShaderParser::CallbackShaderWriter& writer) -> bool {
        // == Passes ==
        if (instruction == "depthPre")
        {
            writer.emitText(R"(
                out float fOut;
            )");
            return true;
        }

        if (instruction == "transparent")
        {
            writer.emitText(R"(
                out vec4 fAccuA;
                out float fAccuB;
                out vec3 fDistortion;
            )");
            return true;
        }

        if (instruction == "opaque")
        {
            writer.emitText(R"(
                out vec3 fHdr;
                out vec2 fVelocity;
            )");
            return true;
        }

        // == Internal UBOs ==
        if (instruction == "internal_projectionInfoUbo")
        {
            writer.emitText(R"(
                    #ifndef _PIPELINE_UBO_PROJECTION_INFO
                    #define _PIPELINE_UBO_PROJECTION_INFO
                    layout (std140) uniform uPipelineProjectionInfoUbo
                    {
                        vec4 viewportNearFar;
                        mat4 projectionInverse;
                        mat4 viewInverse;
                    } uPipelineProjectionInfo;
                    #endif
                )");
            return true;
        }

        if (instruction == "sceneInfo")
        {
            writer.emitText(R"(
                    #ifndef _PIPELINE_UBO_SCENE
                    #define _PIPELINE_UBO_SCENE
                    layout (std140) uniform uPipelineSceneUbo
                    {
                        vec4 sunColor_sunIntensity;             // vec3 + float
                        vec4 sunDirection_asIntensity;          // vec3 + float
                        vec4 asColor_fogDensity;                // vec3 + float
                        vec4 asHeightFunc_bloomThreshold;       // float x4
                        vec4 bloomIntensity_exp_gamma_contrast; // float x4
                        vec4 brightness_sharpen_tonemap_void;   // float + float + bool + void
                    } uPipelineScenePacked;

                    struct PipelineSceneUnpacked
                    {
                        vec3 sunColor;
                        float sunIntensity;
                        vec3 sunDirection;
                        float atmoScatterIntensity;
                        vec3 atmoScatterColor;
                        float fogDensity;
                        float atmoScatterFalloffStart;
                        float atmoScatterFalloffEnd;
                        float atmoScatterFalloffExponent;
                        float bloomThreshold;
                        float bloomIntensity;
                        float exposure;
                        float gamma;
                        float contrast;
                        float brightness;
                        float sharpenStrength;
                        float tonemapEnabled;
                    };

                    PipelineSceneUnpacked unpackPipelineSceneUbo() {
                        PipelineSceneUnpacked res;
                        res.sunColor = uPipelineScenePacked.sunColor_sunIntensity.xyz;
                        res.sunIntensity = uPipelineScenePacked.sunColor_sunIntensity.w;
                        res.sunDirection = uPipelineScenePacked.sunDirection_asIntensity.xyz;
                        res.atmoScatterIntensity = uPipelineScenePacked.sunDirection_asIntensity.w;
                        res.atmoScatterColor = uPipelineScenePacked.asColor_fogDensity.xyz;
                        res.fogDensity = uPipelineScenePacked.asColor_fogDensity.w;
                        res.atmoScatterFalloffStart = uPipelineScenePacked.asHeightFunc_bloomThreshold.x;
                        res.atmoScatterFalloffEnd = uPipelineScenePacked.asHeightFunc_bloomThreshold.y;
                        res.atmoScatterFalloffExponent = uPipelineScenePacked.asHeightFunc_bloomThreshold.z;
                        res.bloomThreshold = uPipelineScenePacked.asHeightFunc_bloomThreshold.w;
                        res.bloomIntensity = uPipelineScenePacked.bloomIntensity_exp_gamma_contrast.x;
                        res.exposure = uPipelineScenePacked.bloomIntensity_exp_gamma_contrast.y;
                        res.gamma = uPipelineScenePacked.bloomIntensity_exp_gamma_contrast.z;
                        res.contrast = uPipelineScenePacked.bloomIntensity_exp_gamma_contrast.w;
                        res.brightness = uPipelineScenePacked.brightness_sharpen_tonemap_void.x;
                        res.sharpenStrength = uPipelineScenePacked.brightness_sharpen_tonemap_void.y;
                        res.tonemapEnabled = uPipelineScenePacked.brightness_sharpen_tonemap_void.z;
                        return res;
                    }

                    PipelineSceneUnpacked gPipelineScene = unpackPipelineSceneUbo();
                    #endif
                )");
            return true;
        }

        return false;
    });

#ifdef GLOW_EXTRAS_EMBED_SHADERS
    for (auto& virtualPair : internal_embedded_files::pipeline_embed_shaders)
        DefaultShaderParser::addVirtualFile(virtualPair.first, virtualPair.second);
#else
    DefaultShaderParser::addIncludePath(util::pathOf(__FILE__) + "/../../shader");
#endif

    _isPipelineInitialized = true;
}
