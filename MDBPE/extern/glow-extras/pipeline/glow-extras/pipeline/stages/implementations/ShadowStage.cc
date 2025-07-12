#include "ShadowStage.hh"

#include <typed-geometry/tg.hh>

#include <glow/common/scoped_gl.hh>
#include <glow/data/TextureData.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture2DArray.hh>
#include <glow/objects/UniformBuffer.hh>

#include "../../RenderCallback.hh"
#include "../../RenderScene.hh"
#include "../StageCamera.hh"

void glow::pipeline::ShadowStage::updateShadowData(StageCamera const& cam, const tg::vec3& lightDirection, float cascadeLambda)
{
    // Based on shadowmappingcascade.cpp from Sascha Willem's Vulkan samples

    std::array<float, GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT> cascadeSplits;
    auto const& nearClip = cam.getCameraData().camNearPlane;
    auto const farClip = cam.getCameraData().camFarPlane; // If in reverse-Z, this is the simulated far plane, effectively the shadow range
    auto const clipRange = farClip - nearClip;

    auto minZ = nearClip;
    auto maxZ = nearClip + clipRange;

    auto range = maxZ - minZ;
    auto ratio = maxZ / minZ;

    // Based on https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (auto i = 0u; i < GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT; ++i)
    {
        auto p = (i + 1) / static_cast<float>(GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT);
        auto log = minZ * std::pow(ratio, p);
        auto uniform = minZ + range * p;
        auto d = cascadeLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearClip) / clipRange;
    }

    float lastSplitDist = 0.f;
    for (auto i = 0u; i < GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT; ++i)
    {
        auto const& splitDist = cascadeSplits[i];

        tg::vec3 frustumCorners[8] = {
            tg::vec3(-1, 1, -1), tg::vec3(1, 1, -1), tg::vec3(1, -1, -1), tg::vec3(-1, -1, -1),
            tg::vec3(-1, 1, 1),  tg::vec3(1, 1, 1),  tg::vec3(1, -1, 1),  tg::vec3(-1, -1, 1),
        };

        // Project frustum corners into world space
#if GLOW_PIPELINE_ENABLE_REVERSE_Z
        tg::mat4 invCam = tg::inverse(cam.getCameraData().simulatedCleanProj * cam.getCameraData().view);
#else
        tg::mat4 invCam = tg::inverse(cam.getCameraData().cleanVp);
#endif
        for (auto i = 0; i < 8; ++i)
        {
            tg::vec4 invCorner = invCam * tg::vec4(frustumCorners[i], 1.0f);
            frustumCorners[i] = tg::vec3(invCorner / invCorner.w);
        }

        for (auto i = 0; i < 4; ++i)
        {
            tg::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
            frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
            frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
        }

        // Get frustum center
        tg::vec3 frustumCenter = tg::vec3(0.0f);
        for (auto i = 0; i < 8; ++i)
        {
            frustumCenter += frustumCorners[i];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;
        for (auto i = 0; i < 8; ++i)
        {
            float distance = tg::length(frustumCorners[i] - frustumCenter);
            radius = tg::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        tg::vec3 maxExtents = tg::vec3(radius);
        tg::vec3 minExtents = -maxExtents;

        tg::mat4 lightViewMatrix = tg::look_at_opengl(tg::pos3(frustumCenter) + lightDirection * -minExtents.z, frustumCenter, tg::vec3(0.0f, 1.0f, 0.0f));
        tg::mat4 lightOrthoMatrix = tg::orthographic(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

        static_assert(4 /*tg::vec4::length()*/ == GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT, "This cascade amount requires a different split depth "
                                                                                       "container");
        // Store split distance and matrix in cascade
        mSplitDepths[static_cast<int>(i)] = (nearClip + splitDist * clipRange) * -1.0f;
        mLightSpaceVps[i] = lightOrthoMatrix * lightViewMatrix;

        lastSplitDist = cascadeSplits[i];
    }
}

void glow::pipeline::ShadowStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback& rc)
{
    PIPELINE_PROFILE("Shadow Stage");


    bool drawShadows = ctx.scene.shadow.mode == ShadowMode::UpdateAlways;

    if (ctx.scene.shadow.mode == ShadowMode::UpdateOnce)
    {
        drawShadows = true;
        ctx.scene.shadow.mode = ShadowMode::DontUpdate;
    }

    // Update Shadow Data
    {
        updateShadowData(ctx.cam, ctx.scene.sun.direction, ctx.scene.shadow.cascadeSplitLambda);
        mUboLightVPs->bind().setData(mLightSpaceVps, GL_STREAM_DRAW);
        mUboSquaredCascadeLimits->bind().setData(mSplitDepths, GL_STREAM_DRAW);
    }

    if (drawShadows)
    {
        // Create shadow cascades if necessary
        if (!ctx.scene.shadow.cascades)
        {
            SharedTextureData shadowCascadeFormat = std::make_shared<TextureData>();
            shadowCascadeFormat->setWidth(GLOW_PIPELINE_SHADOW_MAP_SIZE);
            shadowCascadeFormat->setHeight(GLOW_PIPELINE_SHADOW_MAP_SIZE);
            shadowCascadeFormat->setPreferredInternalFormat(GL_DEPTH_COMPONENT32F);
            shadowCascadeFormat->setMagFilter(GL_LINEAR);
            shadowCascadeFormat->setMinFilter(GL_LINEAR);
            shadowCascadeFormat->setWrapR(GL_CLAMP_TO_BORDER);
            shadowCascadeFormat->setWrapS(GL_CLAMP_TO_BORDER);
            shadowCascadeFormat->setWrapT(GL_CLAMP_TO_BORDER);
            shadowCascadeFormat->setCompareMode(GL_COMPARE_R_TO_TEXTURE_ARB);
            shadowCascadeFormat->setCompareFunction(GL_LEQUAL);
            shadowCascadeFormat->setBorderColor(tg::color4(1, 1, 1, 1));
            shadowCascadeFormat->setAnisotropicFiltering(1);

            ctx.scene.shadow.cascades = Texture2DArray::createFromData(shadowCascadeFormat);
            ctx.scene.shadow.cascades->bind().makeStorageImmutable(GLOW_PIPELINE_SHADOW_MAP_SIZE, GLOW_PIPELINE_SHADOW_MAP_SIZE,
                                                                   GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT, GL_DEPTH_COMPONENT32F, 1);
        }

        // Render Shadow Pass
        {
#if GLOW_PIPELINE_ENABLE_REVERSE_Z
            // Reset to default values
            // This is required so the default glm::orthoRH_NO projection does not cut off geometry at depth < 0.5
            // It would no longer be required if a glm::orthoRH_ZO projection is used instead, but the transformation
            // from world- to light space does not work correctly in that case (in the opaque pass shader, shadowing.glsl)
            glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
#endif

            auto fbo = mFboShadows->bind();
            fbo.attachDepth(ctx.scene.shadow.cascades, 0, -1);

            glClear(GL_DEPTH_BUFFER_BIT);

            GLOW_SCOPED(enable, GL_DEPTH_TEST);

            // Frontface culling
            GLOW_SCOPED(enable, GL_CULL_FACE);
            GLOW_SCOPED(cullFace, GL_FRONT);

            // Non-instanced
            {
                auto shader = mShaderShadow->use();
                rc.onPerformShadowPass(ctx, shader);
            }

            // Instanced
            {
                auto shader = mShaderShadowInstancing->use();
                rc.onPerformShadowPassInstanced(ctx, shader);
            }

            // Custom
            rc.onRenderStage(ctx);


#if GLOW_PIPELINE_ENABLE_REVERSE_Z
            // Re-enable reverse Z for the remaining pipeline
            glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
#endif
        }
    }
}

glow::pipeline::ShadowStage::ShadowStage()
{
    mFboShadows = Framebuffer::create();

    mUboLightVPs = UniformBuffer::create();
    mUboSquaredCascadeLimits = UniformBuffer::create();

    mUboLightVPs->bind().setData(mLightSpaceVps, GL_STREAM_DRAW);
    mUboSquaredCascadeLimits->bind().setData(tg::vec4(0.f), GL_STREAM_DRAW);

    mShaderShadow = Program::createFromFiles({"glow-pipeline/internal/pass/shadow/geometryDepthOnly.vsh",
                                              "glow-pipeline/internal/pass/shadow/shadowCascades.gsh", "glow-pipeline/internal/utility/blank.fsh"});

    mShaderShadow->setUniformBuffer("uShadowLightVPUBO", mUboLightVPs);


    mShaderShadowInstancing = Program::createFromFiles({"glow-pipeline/internal/pass/shadow/geometryDepthOnlyInstanced.vsh",
                                                        "glow-pipeline/internal/pass/shadow/shadowCascades.gsh", "glow-pipeline/internal/utility/blank.fsh"});

    mShaderShadowInstancing->setUniformBuffer("uShadowLightVPUBO", mUboLightVPs);
}

void glow::pipeline::ShadowStage::registerShader(RenderContext const&, const glow::SharedProgram& shader) const
{
    shader->setUniformBuffer("uShadowVPUBO", mUboLightVPs);
}
