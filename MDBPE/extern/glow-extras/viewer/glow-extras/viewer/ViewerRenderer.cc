#include "ViewerRenderer.hh"

#include <glow-extras/vector/fonts.hh>
#include <glow-extras/vector/graphics2D.hh>
#include <glow/common/non_copyable.hh>
#include <glow/common/scoped_gl.hh>
#include <glow/glow.hh>

#include "Scene.hh"
#include "view.hh"

namespace
{
std::string sDefaultFontPath;

// TODO: Add viewer-global UBO with this kind of layout
// struct ShaderGlobalUbo
//{
//    glow::std140mat4x4 view;
//    glow::std140mat4x4 proj;
//    glow::std140mat4x4 invView;
//    glow::std140mat4x4 invProj;
//    glow::std140mat4x4 cleanVp;
//    glow::std140mat4x4 prevCleanVp;

//    glow::std140vec3 camPos;
//    glow::std140vec3 sunDirection;
//    glow::std140vec3 sunColor;
//};

struct AccumRenderPoolTargets
{
private:
    glow::TexturePool<glow::TextureRectangle>* const mPool;
    GLOW_RAII_CLASS(AccumRenderPoolTargets);

public:
    glow::SharedTextureRectangle shadowMap;
    glow::SharedTextureRectangle color;
    glow::SharedTextureRectangle normal;
    glow::SharedTextureRectangle colorOverlay;
    glow::SharedTextureRectangle colorTransparent;
    glow::SharedTextureRectangle normalTransparent;
    glow::SharedTextureRectangle depthTransparent;

    AccumRenderPoolTargets(glow::TexturePool<glow::TextureRectangle>* pool, tg::isize2 size, bool reverse_z_enabled) : mPool(pool)
    {
        if (reverse_z_enabled)
        {
            shadowMap = mPool->allocAtLeast({GL_DEPTH_COMPONENT32F, {2048, 2048}});
            depthTransparent = mPool->allocAtLeast({GL_DEPTH_COMPONENT32F, size});
        }
        else
        {
            shadowMap = mPool->allocAtLeast({GL_DEPTH_COMPONENT32, {2048, 2048}});
            depthTransparent = mPool->allocAtLeast({GL_DEPTH_COMPONENT32, size});
        }

        color = mPool->allocAtLeast({GL_RGBA16F, size});
        normal = mPool->allocAtLeast({GL_RGBA16F, size});
        colorOverlay = mPool->allocAtLeast({GL_RGBA16F, size});
        colorTransparent = mPool->allocAtLeast({GL_RGBA16F, size});
        normalTransparent = mPool->allocAtLeast({GL_RGBA16F, size});
    }

    ~AccumRenderPoolTargets()
    {
        mPool->free(&shadowMap);
        mPool->free(&color);
        mPool->free(&normal);
        mPool->free(&colorOverlay);
        mPool->free(&colorTransparent);
        mPool->free(&normalTransparent);
        mPool->free(&depthTransparent);
    }
};
}

void glow::viewer::global_set_default_font_path(const std::string& path)
{
    sDefaultFontPath = path;
    if (path.size() > 0 && path.back() != '/' && path.back() != '\\')
        sDefaultFontPath += "/";
}

glow::viewer::ViewerRenderer::ViewerRenderer()
{
    mReverseZEnabled = glow::OGLVersion.total >= 45;

    // Load shaders
    mShaderSSAO = glow::Program::createFromFile("glow-viewer/pp.ssao");
    mShaderOutline = glow::Program::createFromFile("glow-viewer/pp.outline");
    mShaderOutput = glow::Program::createFromFile("glow-viewer/pp.output");
    mShaderBackground = glow::Program::createFromFile("glow-viewer/pp.bg");
    mShaderGround = glow::Program::createFromFile("glow-viewer/pp.ground");
    mShaderAccum = glow::Program::createFromFile("glow-viewer/pp.accum");
    mShaderShadow = glow::Program::createFromFile("glow-viewer/pp.shadow");
    mShaderPickingVis = glow::Program::createFromFile("glow-viewer/pp.pickvis");

    // Load meshes
    mMeshQuad = glow::geometry::make_quad();

    // Create framebuffers
    mFramebuffer = Framebuffer::create();
    mFramebufferColor = Framebuffer::create();
    mFramebufferColorOverlay = Framebuffer::create();
    mFramebufferSSAO = Framebuffer::create();
    mFramebufferOutput = Framebuffer::create();
    mFramebufferShadow = Framebuffer::create();
    mFramebufferShadowSoft = Framebuffer::create();
    mFramebufferPicking = Framebuffer::create();
    mFramebufferVisPicking = Framebuffer::create();

    // Register global fonts
    auto const& globalFonts = detail::internal_global_get_fonts();
#ifdef GLOW_EXTRAS_DEFAULT_FONTS
    mVectorRenderer.loadFontFromMemory("sans", glow::vector::getDefaultFontSans());
    mVectorRenderer.loadFontFromMemory("mono", glow::vector::getDefaultFontMono());
#endif
    for (auto const& font : globalFonts)
        mVectorRenderer.loadFontFromFile(font.first, font.second);
}

void glow::viewer::ViewerRenderer::beginFrame(tg::color3 const& clearColor, bool bSkipOutput)
{
    if (mReverseZEnabled)
    {
        // set up reverse-Z depth test (1 is near, 0 is far)
        // see http://www.reedbeta.com/blog/depth-precision-visualized/
        // see https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    }

    if (!bSkipOutput)
    {
        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    mIsCurrentFrameFullyConverged = true;

    mAllPickableRenderables.clear();
}

void glow::viewer::ViewerRenderer::endFrame(float approximateRenderTime)
{
    if (mReverseZEnabled)
    {
        // restore default OpenGL conventions
        glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
    }

    if (mIsCurrentFrameFullyConverged)
    {
        // If all subviews rendered this frame were converged, the approximate render time is meaningless
        // To prevent lags on the next "wake-up" draw, cap sample counts to the minimum
        mSSAOSamples = 12;
        mShadowSamplesPerFrame = 1;
    }
    else if (approximateRenderTime > 0.f)
    {
        auto multiply = [&](float mult)
        {
            mSSAOSamples *= mult;
            mShadowSamplesPerFrame *= mult;
        };

        auto add = [&](int val)
        {
            mSSAOSamples += val;
            mShadowSamplesPerFrame += val;
        };

        if (approximateRenderTime < 5)
            multiply(2);
        else if (approximateRenderTime < 10)
            add(1);
        else if (approximateRenderTime > 15)
            multiply(.5f);
        else if (approximateRenderTime > 13)
            add(-1);

        mSSAOSamples = tg::clamp(mSSAOSamples, 12, 64);
        mShadowSamplesPerFrame = tg::clamp(mShadowSamplesPerFrame, 1, 32);
    }

    texturePoolRect.cleanUp();
    texturePool2D.cleanUp();
}

void glow::viewer::ViewerRenderer::maximizeSamples()
{
    mShadowSamplesPerFrame = 32;
    mSSAOSamples = 64;
}

cc::optional<tg::pos3> glow::viewer::ViewerRenderer::query3DPosition(tg::isize2 resolution, tg::ipos2 pixel, const SubViewData& subViewData, const CameraController& cam) const
{
    if (pixel.x >= resolution.width || pixel.y >= resolution.height)
        return {};

    // remap
    pixel.y = resolution.height - pixel.y - 1;

    float d;

    {
        auto fb = mFramebuffer->bind();
        fb.attachDepth(subViewData.targetDepth);
        glReadPixels(pixel.x, pixel.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d);
    }

    if (mReverseZEnabled && d <= 0.0f)
        return {};
    if (!mReverseZEnabled && d >= 1.0f)
        return {};

    tg::pos3 pos_ndc;
    pos_ndc.x = pixel.x / float(resolution.width - 1) * 2 - 1;
    pos_ndc.y = pixel.y / float(resolution.height - 1) * 2 - 1;

    if (mReverseZEnabled)
        pos_ndc.z = d;
    else
        pos_ndc.z = d * 2.0f - 1.0f;

    auto pos_vs = tg::inverse(cam.computeProjMatrix()) * pos_ndc;
    return tg::inverse(cam.computeViewMatrix()) * pos_vs;
}

cc::optional<float> glow::viewer::ViewerRenderer::queryDepth(tg::isize2 resolution, tg::ipos2 pixel, const SubViewData& subViewData) const
{
    if (pixel.x >= resolution.width || pixel.y >= resolution.height)
        return {};

    // remap
    pixel.y = resolution.height - pixel.y - 1;

    float d;

    {
        auto fb = mFramebuffer->bind();
        fb.attachDepth(subViewData.targetDepth);
        glReadPixels(pixel.x, pixel.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d);
    }

    return d;
}

void glow::viewer::ViewerRenderer::handlePicking(tg::isize2 resolution, tg::ipos2 pixel, SubViewData& subViewData, CameraController const& cam, bool left_mouse, bool right_mouse)
{
    if (pixel.x >= resolution.width || pixel.y >= resolution.height || pixel.x < 0 || pixel.y < 0)
        return;

    // remap
    pixel.y = resolution.height - pixel.y - 1;

    // store pixel position in ViewerRenderer (each frame)
    normal_check_values = pixel;

    tg::ivec2 rg;
    float d;

    {
        auto fb = mFramebufferPicking->bind();
        fb.attachColor("fColor", subViewData.targetPicking);
        fb.attachDepth(subViewData.targetDepth);

        glReadPixels(pixel.x, pixel.y, 1, 1, GL_RG_INTEGER, GL_INT, &rg);
        glReadPixels(pixel.x, pixel.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d);
    }

    // ############## WORLD POS ########################
    tg::pos3 pos_ndc;
    pos_ndc.x = pixel.x / float(resolution.width - 1) * 2 - 1;
    pos_ndc.y = pixel.y / float(resolution.height - 1) * 2 - 1;

    if (mReverseZEnabled)
        pos_ndc.z = d;
    else
        pos_ndc.z = d * 2.0f - 1.0f;

    auto pos_vs = tg::inverse(cam.computeProjMatrix()) * pos_ndc;
    auto pos_world = tg::inverse(cam.computeViewMatrix()) * pos_vs;
    // ############ WORLD POS END ########################

    if (rg.x < 0)
        return;

    if (mAllPickableRenderables.empty())
        return; // first frame

    auto const& ren = mAllPickableRenderables[rg.x];

    if (!ren->hasPicker())
    {
        std::cout << "[ViewerApp] WARNING: This renderable does not have a Picker. Call gv::view() with gv::pick()" << std::endl;
        return;
    }

    tg::vec3 normal = tg::vec3(0, 0, 0);
    auto& p = ren->getPicker();
    if (current_picked_normal.has_value())
    {
        normal = current_picked_normal.value();
    }

    auto const res = p.handlePicking(pickRes, pos_world, rg, normal, left_mouse, right_mouse);

    if (res.successfulPick && rg.x >= 0 && rg.y >= 0)
    {
        pickRes = res;
        if (mLastPickedRenderable != nullptr && mLastPickedRenderable != ren)
        {
            mLastPickedRenderable->setDirty();
        }
        current_picked_ID = rg;
        subViewData.clearAccumBuffer();
        mLastPickedRenderable = ren;
    }
}

void glow::viewer::ViewerRenderer::renderSubview(
    tg::isize2 const& res, tg::ipos2 const& offset, SubViewData& subViewData, Scene const& scene, glow::viewer::CameraController& cam, bool bSkipOutput)
{
    if (scene.shouldBeCleared() || scene.queryHash() != subViewData.lastHash)
    {
        subViewData.clearAccumBuffer();
        subViewData.clearShadowMap();
        subViewData.lastHash = scene.queryHash();
    }

    auto const& renderables = scene.getRenderables();
    auto const& boundingInfo = scene.getBoundingInfo();

    auto sunPos = boundingInfo.center
                  + tg::vec3::unit_y * (0.5f * (boundingInfo.aabb.max.y - boundingInfo.aabb.min.y) + scene.config.sunOffsetFactor * boundingInfo.diagonal);

    // pass lambdas
    auto renderVectorOverlay = [&]()
    {
        RenderInfo info{cam.computeViewMatrix(),
                        cam.computeProjMatrix(),
                        sunPos,
                        res,
                        cam.getPosition(),
                        normalize(cam.getForwardDir()),
                        normalize(cam.getUpDir()),
                        normalize(cam.getRightDir()),
                        subViewData.accumCount,
                        mReverseZEnabled,
                        subViewData.mElapsedSeconds};
        for (auto const& r : renderables)
            r->renderOverlay(info, &mVectorRenderer, res, offset);

        mVectorImage.clear();
        {
            auto g = graphics(mVectorImage);

            auto const col_fg = scene.config.enablePrintMode ? tg::color3::black : tg::color3::white;
            auto const col_bg = scene.config.enablePrintMode ? tg::color3::white : tg::color3::black;

            std::string_view name;
            auto has_unique_name = true;
            for (auto const& r : renderables)
            {
                if (r->name().empty())
                    continue;

                if (!name.empty() && name != r->name())
                    has_unique_name = false;
                name = r->name();
            }

            if (!name.empty() && has_unique_name)
            {
                auto f = glow::vector::font2D("sans", 24);
                auto const y = res.height - 8.f;
                f.blur = 4;
                g.text({8, y}, name, f, col_bg);
                f.blur = 1;
                g.text({8, y}, name, f, col_bg);
                f.blur = 0;
                g.text({8, y}, name, f, col_fg);
            }
        }
        mVectorRenderer.render(mVectorImage, res.width, res.height);
    };

    auto performOutput = [&]()
    {
        if (bSkipOutput)
            return;

        // output
        {
            GLOW_SCOPED(debugGroup, "output");
            auto shader = mShaderOutput->use();
            shader["uTexOutput"] = subViewData.targetOutput;
            shader["uAccumCnt"] = subViewData.accumCount;
            shader["uViewportOffset"] = offset;
            shader["uDebugPixels"] = scene.enableScreenshotDebug;
            mMeshQuad->bind().draw();
        }

        // glow-extras-vector overlay
        renderVectorOverlay();
    };

    // allocate pool targets
    auto targets = AccumRenderPoolTargets(&texturePoolRect, res, mReverseZEnabled);

    // adjust camera size
    cam.resize(res.width, res.height);

    if (mReverseZEnabled)
        glClearDepth(0);
    else
        glClearDepth(1);

    GLOW_SCOPED(depthFunc, mReverseZEnabled ? GL_GREATER : GL_LESS);

    auto camPos = cam.getPosition();
    auto view = cam.computeViewMatrix();

    // accumulation and jittering
    auto dis = 0.0f;
    for (auto x = 0; x < 3; ++x)
        for (auto y = 0; y < 3; ++y)
            dis += tg::abs(view[x][y] - subViewData.lastView[x][y]);
    if (dis > 0.01f || distance(subViewData.lastPos, camPos) > boundingInfo.diagonal / 5000.f)
    {
        // Camera changed beyond epsilon, reset accumulation
        subViewData.clearAccumBuffer();

        subViewData.lastView = view;
        subViewData.lastPos = camPos;
    }
    else // clip to last
    {
        view = subViewData.lastView;
        camPos = subViewData.lastPos;
    }

    // early out if too many samples, and no infinite accumulation configured
    if (subViewData.ssaoSampleCount > mMinSSAOCnt && subViewData.accumCount > mMinAccumCnt && !scene.config.infiniteAccumulation)
    {
        performOutput();
        return;
    }
    else
    {
        mIsCurrentFrameFullyConverged = false;
    }

    // compute sun
    if (distance(boundingInfo.center, sunPos) <= boundingInfo.diagonal / 10000)
        sunPos += tg::vec3::unit_y * tg::max(1.0f, distance_to_origin(sunPos) / 50);

    auto groundY = (boundingInfo.aabb.min.y - 1e-4f) - mGroundOffsetFactor * boundingInfo.diagonal;

    auto const groundShadowAabb = tg::aabb3(tg::pos3(boundingInfo.center.x, groundY, boundingInfo.center.z) - tg::vec3(1, 0, 1) * boundingInfo.diagonal * 1,
                                            tg::pos3(boundingInfo.center.x, groundY, boundingInfo.center.z) + tg::vec3(1, 0, 1) * boundingInfo.diagonal * 1);

    for (auto _ = 0; _ < mShadowSamplesPerFrame && subViewData.shadowSampleCount < scene.config.maxShadowSamples; ++_)
    {
        auto sunPosJitter = sunPos;
        if (subViewData.shadowSampleCount > 0)
        {
            auto v = uniform_vec(mRng, tg::sphere3::unit) * boundingInfo.diagonal * scene.config.sunScaleFactor / 3.f;
            v.y = 0;
            sunPosJitter += v;
        }
        auto sunDir = normalize(boundingInfo.center - sunPosJitter);
        auto sunFov = 1_deg;
        if (volume_of(boundingInfo.aabb) > 0)
            for (auto x : {0, 1})
                for (auto y : {0, 1})
                    for (auto z : {0, 1})
                    {
                        auto s = boundingInfo.aabb.max - boundingInfo.aabb.min;
                        auto p = boundingInfo.aabb.min;
                        p.x += s.x * x;
                        p.y += s.y * y;
                        p.z += s.z * z;
                        auto pd = normalize(p - sunPosJitter);
                        auto dot = tg::dot(sunDir, pd);
                        // assure dot is in [-1, 1] so that acos is well defined.
                        // even though the dot product of two normalized vectors should be in [-1, 1],
                        // numerical errors can lead to values slightly outside this range.
                        dot = tg::clamp(dot, -1.0f, 1.0f);
                        sunFov = tg::max(tg::acos(dot) * 2, sunFov);
                    }

        auto sunView = tg::look_at_opengl(sunPosJitter, boundingInfo.center, tg::vec3::unit_x);
        tg::mat4 sunProj;
        if (mReverseZEnabled)
            sunProj = tg::perspective_reverse_z_opengl(tg::horizontal_fov(sunFov), 1.0f, cam.getNearPlane());
        else
            sunProj = tg::perspective_opengl(tg::horizontal_fov(sunFov), 1.0f, cam.getNearPlane(), cam.getFarPlane());

        // draw shadow map
        {
            GLOW_SCOPED(debugGroup, "Draw shadow map");
            auto fb = mFramebufferShadow->bind();
            fb.attachDepth(targets.shadowMap);

            GLOW_SCOPED(enable, GL_DEPTH_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);

            auto const shadowMapBiasedRes = SubViewData::shadowMapSize / 2;
            RenderInfo info{sunView,
                            sunProj,
                            sunPos,
                            shadowMapBiasedRes,
                            tg::pos3::zero,
                            tg::dir3::neg_z,
                            tg::dir3::pos_y,
                            tg::dir3::pos_x,
                            subViewData.shadowSampleCount,
                            mReverseZEnabled,
                            subViewData.mElapsedSeconds};

            for (auto const& r : renderables)
                r->renderShadow(info);
        }

        // accum soft shadow map
        {
            GLOW_SCOPED(debugGroup, "Accumulate soft shadow map");
            auto fb = mFramebufferShadowSoft->bind();
            fb.attachColor("fShadow", subViewData.shadowMapSoft);

            if (subViewData.shadowSampleCount == 0)
            {
                GLOW_SCOPED(clearColor, 0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            GLOW_SCOPED(enable, GL_BLEND);
            GLOW_SCOPED(blendFunc, GL_ONE, GL_ONE);

            auto shader = mShaderShadow->use();
            shader["uSunView"] = sunView;
            shader["uSunProj"] = sunProj;
            shader["uGroundShadowMin"] = groundShadowAabb.min;
            shader["uGroundShadowMax"] = groundShadowAabb.max;
            shader["uShadowMap"] = targets.shadowMap;
            shader["uReverseZEnabled"] = mReverseZEnabled;
            mMeshQuad->bind().draw();
        }

        ++subViewData.shadowSampleCount;
    }
    // update mipmaps
    subViewData.shadowMapSoft->bind().generateMipmaps();

    auto const ssaoEnabled = bool(scene.config.ssaoPower > 0.f);

    // accumulate multiple frames per frame
    for (auto _ = 0; _ < mAccumPerFrame; ++_)
    {
        // jittering
        auto jitter_x = uniform(mRng, -1.0f, 1.0f);
        auto jitter_y = uniform(mRng, -1.0f, 1.0f);
        if (subViewData.accumCount == 0)
            jitter_x = jitter_y = 0;
        auto proj = tg::translation(tg::vec3(jitter_x / res.width, jitter_y / res.height, 0)) * cam.computeProjMatrix();

        // main rendering
        {
            GLOW_SCOPED(debugGroup, "main render");
            auto fb = mFramebuffer->bind();
            fb.attachColor("fColor", targets.color);
            fb.attachColor("fNormal", targets.normal);
            fb.attachDepth(subViewData.targetDepth);

            GLOW_SCOPED(enable, GL_DEPTH_TEST);
            GLOW_SCOPED(clearColor, 0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // bg
            {
                GLOW_SCOPED(disable, GL_DEPTH_TEST);
                GLOW_SCOPED(disable, GL_CULL_FACE);
                auto shader = mShaderBackground->use();
                shader["uInnerColor"] = tg::vec3(scene.config.bgColorInner);
                shader["uOuterColor"] = tg::vec3(scene.config.bgColorOuter);
                shader["uPrintMode"] = scene.config.enablePrintMode;
                shader["uTexEnvMap"] = scene.config.bgEnvmap;
                shader["uUseEnvMap"] = scene.config.bgEnvmap != nullptr;
                shader["uInvProj"] = inverse(proj);
                shader["uInvView"] = inverse(view);
                mShaderBackground->setWarnOnUnchangedUniforms(false); // uTexEnvMap
                mMeshQuad->bind().draw();
            }

            // renderjobs
            if (scene.config.enableForwardRendering)
            {
                RenderInfo info{view,
                                proj,
                                sunPos,
                                res,
                                cam.getPosition(),
                                normalize(cam.getForwardDir()),
                                normalize(cam.getUpDir()),
                                normalize(cam.getRightDir()),
                                subViewData.accumCount,
                                mReverseZEnabled,
                                subViewData.mElapsedSeconds};
                for (auto const& r : renderables)
                    r->renderForward(info);
            }
        }

        // picking rendering
        {
            auto fb = mFramebufferPicking->bind();
            fb.attachColor("fColor", subViewData.targetPicking);
            fb.attachDepth(subViewData.targetDepth);

            GLOW_SCOPED(depthFunc, mReverseZEnabled ? GL_GEQUAL : GL_LEQUAL);
            GLOW_SCOPED(enable, GL_DEPTH_TEST);
            int const clear_color[] = {-1, -1, -1, -1};
            glClearBufferiv(GL_COLOR, 0, clear_color);

            RenderInfo info{view,
                            proj,
                            sunPos,
                            res,
                            cam.getPosition(),
                            normalize(cam.getForwardDir()),
                            normalize(cam.getUpDir()),
                            normalize(cam.getRightDir()),
                            subViewData.accumCount,
                            mReverseZEnabled,
                            subViewData.mElapsedSeconds};

            for (auto& r : renderables)
            {
                if (r->hasPicker())
                {
                    auto const renderableId = int(mAllPickableRenderables.size());
                    mAllPickableRenderables.push_back(r);
                    r->renderPicking(info, renderableId);
                }
            }
        }

        // ground
        {
            GLOW_SCOPED(debugGroup, "ground");
            auto fb = mFramebuffer->bind();

            GLOW_SCOPED(enable, GL_BLEND);
            GLOW_SCOPED(enable, GL_DEPTH_TEST);
            GLOW_SCOPED(depthMask, GL_FALSE);
            GLOW_SCOPED(depthFunc, GL_ALWAYS);
            GLOW_SCOPED(disable, GL_CULL_FACE);
            GLOW_SCOPED(blendFunc, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            auto shader = mShaderGround->use();
            shader["uReverseZEnabled"] = mReverseZEnabled;
            shader["uProj"] = proj;
            shader["uView"] = view;
            shader["uInvProj"] = inverse(proj);
            shader["uInvView"] = inverse(view);
            shader["uGroundY"] = scene.config.customGridCenter.has_value() ? scene.config.customGridCenter.value().y : groundY;
            shader["uCamPos"] = camPos;
            shader["uGridCenter"] = scene.config.customGridCenter.has_value() ? scene.config.customGridCenter.value() : tg::pos3();
            shader["uGridSize"] = scene.config.customGridSize.has_value() ? scene.config.customGridSize.value() : boundingInfo.diagonal / 3;
            shader["uMeshDiag"] = boundingInfo.diagonal;
            shader["uMeshCenter"] = boundingInfo.center;
            shader["uShadowStrength"] = scene.config.enableShadows ? scene.config.shadowStrength : 0.f;
            shader["uGroundShadowMin"] = groundShadowAabb.min;
            shader["uGroundShadowMax"] = groundShadowAabb.max;
            shader["uShadowSamples"] = float(subViewData.shadowSampleCount);
            shader["uShadowMapSoft"] = subViewData.shadowMapSoft;
            shader["uShadowScreenFadeoutDistance"] = scene.config.shadowScreenFadeoutDistance;
            shader["uShadowWorldFadeoutFactorInner"] = scene.config.shadowWorldFadeoutFactorInner;
            shader["uShadowWorldFadeoutFactorOuter"] = scene.config.shadowWorldFadeoutFactorOuter;
            shader["uShowGrid"] = scene.config.enableGrid;
            shader["uShowBackfacingShadows"] = scene.config.enableBackfacingShadows;
            shader["uTexDepth"] = subViewData.targetDepth;
            mMeshQuad->bind().draw();
        }

        // transparencies
        {
            GLOW_SCOPED(debugGroup, "transparency");

            auto fb = mFramebuffer->bind();
            fb.attachColor("fColor", targets.colorTransparent);
            fb.attachColor("fNormal", targets.normalTransparent);
            fb.attachDepth(targets.depthTransparent);

            GLOW_SCOPED(enable, GL_DEPTH_TEST);
            GLOW_SCOPED(clearColor, 0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // renderjobs
            if (scene.config.enableTransparentRendering)
            {
                RenderInfo info{view,
                                proj,
                                sunPos,
                                res,
                                cam.getPosition(),
                                normalize(cam.getForwardDir()),
                                normalize(cam.getUpDir()),
                                normalize(cam.getRightDir()),
                                subViewData.accumCount,
                                mReverseZEnabled,
                                subViewData.mElapsedSeconds};
                for (auto const& r : renderables)
                    r->renderTransparent(info);
            }
        }

        // update normal according to mouse position
        if (normal_check_values.has_value())
        {
            GLOW_SCOPED(debugGroup, "normal_update");
            auto const pixel_pos = normal_check_values.value();
            tg::vec4 n;
            auto fb = mFramebufferVisPicking->bind();
            fb.attachColor("fNormal", targets.normal);
            glReadPixels(pixel_pos.x, pixel_pos.y, 1, 1, GL_RGBA, GL_FLOAT, &n);
            current_picked_normal = tg::vec3(n.x, n.y, n.z);
        }

        // overlay
        {
            GLOW_SCOPED(debugGroup, "overlay");
            auto fb = mFramebufferColorOverlay->bind();
            fb.attachColor("fColor", targets.colorOverlay);
            GLOW_SCOPED(clearColor, 0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        // outline
        if (scene.config.enableOutlines)
        {
            GLOW_SCOPED(debugGroup, "outline");
            auto fb = mFramebufferColorOverlay->bind();
            GLOW_SCOPED(disable, GL_DEPTH_TEST);
            GLOW_SCOPED(enable, GL_BLEND);
            GLOW_SCOPED(blendFunc, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            auto shader = mShaderOutline->use();
            shader["uReverseZEnabled"] = mReverseZEnabled;
            shader["uTexDepth"] = subViewData.targetDepth;
            shader["uTexNormal"] = targets.normal;
            shader["uNearPlane"] = cam.getNearPlane();
            shader["uFarPlane"] = cam.getFarPlane();
            shader["uNormalThreshold"] = mNormalThreshold;
            shader["uInvProj"] = inverse(proj);
            shader["uInvView"] = inverse(view);
            shader["uCamPos"] = cam.getPosition();
            shader["uViewportOffset"] = offset;
            shader["uDepthThreshold"] = mDepthThresholdFactor * boundingInfo.diagonal / 50.f;
            mMeshQuad->bind().draw();
        }

        // picking visualization
        if (current_picked_ID.has_value())
        {
            GLOW_SCOPED(debugGroup, "picking_visualization");
            auto fb = mFramebufferColorOverlay->bind();
            GLOW_SCOPED(disable, GL_DEPTH_TEST);
            GLOW_SCOPED(enable, GL_BLEND);
            GLOW_SCOPED(blendFunc, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

            auto shader = mShaderPickingVis->use();
            shader["uTexPick"] = subViewData.targetPicking;
            shader["uFragID"] = current_picked_ID.value().y;
            shader["uRenderableID"] = current_picked_ID.value().x;
            shader["uNeighborhoodSize"] = pickRes.borderWidth;
            shader["uColor"] = pickRes.pickingColor;
            shader["uColorBorder"] = pickRes.borderColor;

            mMeshQuad->bind().draw();
        }

        // ssao
        if (ssaoEnabled)
        {
            GLOW_SCOPED(debugGroup, "ssao");
            auto fb = mFramebufferSSAO->bind();
            fb.attachColor("fSSAO", subViewData.targetSsao);

            if (subViewData.ssaoSampleCount == 0)
            {
                GLOW_SCOPED(clearColor, 0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            GLOW_SCOPED(enable, GL_BLEND);
            GLOW_SCOPED(blendFunc, GL_ONE, GL_ONE);

            auto shader = mShaderSSAO->use();
            shader["uReverseZEnabled"] = mReverseZEnabled;
            shader["uTexDepth"] = subViewData.targetDepth;
            shader["uTexNormal"] = targets.normal;
            shader["uView"] = view;
            shader["uProj"] = proj;
            shader["uInvProj"] = inverse(proj);
            shader["uScreenSize"] = tg::size2(res);
            shader["uRadius"] = scene.config.ssaoRadius * boundingInfo.diagonal / 30.f;
            shader["uSeed"] = uint32_t(mRng());
            shader["uSamples"] = int(mSSAOSamples);
            shader["uViewportOffset"] = offset;

            mMeshQuad->bind().draw();

            subViewData.ssaoSampleCount += mSSAOSamples;
        }
        else
        {
            // Jump to min + 1 to allow early outs
            subViewData.ssaoSampleCount = mMinSSAOCnt + 1;
        }

        // accum
        {
            GLOW_SCOPED(debugGroup, "accum");
            subViewData.targetAccumRead.swap(subViewData.targetAccumWrite);

            auto shader = mShaderAccum->use();
            shader["uReverseZEnabled"] = mReverseZEnabled;
            shader["uTexColor"] = targets.color;
            shader["uTexColorOverlay"] = targets.colorOverlay;
            shader["uTexSSAO"] = subViewData.targetSsao;
            shader["uTexDepth"] = subViewData.targetDepth;
            shader["uTexColorTransparent"] = targets.colorTransparent;
            shader["uTexDepthTransparent"] = targets.depthTransparent;
            shader["uTexAccum"] = subViewData.targetAccumRead;

            shader["uAccumCnt"] = subViewData.accumCount;
            shader["uSSAOSamples"] = subViewData.ssaoSampleCount;
            shader["uEnableSSAO"] = ssaoEnabled;
            shader["uSSAOPower"] = scene.config.ssaoPower;
            shader["uForceAlphaOne"] = false;

            shader["uEnableTonemap"] = scene.config.enableTonemap;
            shader["uTonemapExposure"] = scene.config.tonemapExposure;

            {
                auto fb = mFramebufferOutput->bind();
                fb.attachColor("fOutput", subViewData.targetOutput);
                fb.attachColor("fAccum", subViewData.targetAccumWrite);
                mMeshQuad->bind().draw();
            }

            if (subViewData.targetOutput2D)
            {
                shader["uForceAlphaOne"] = true;

                auto fb = mFramebufferOutput->bind();
                fb.attachColor("fOutput", subViewData.targetOutput2D);
                mMeshQuad->bind().draw();
            }
        }

        ++subViewData.accumCount;
    }

    if (bSkipOutput && subViewData.targetOutput2D)
    {
        auto fb = mFramebufferOutput->bind();
        fb.attachColor("fOutput", subViewData.targetOutput2D);
        renderVectorOverlay();
    }

    performOutput();
}
