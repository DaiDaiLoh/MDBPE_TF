#pragma once

#include <vector>

#include <glow/common/shared.hh>

#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow/util/TexturePool.hh>

#include <glow-extras/geometry/Quad.hh>
#include <glow-extras/vector/backend/opengl.hh>
#include <glow-extras/vector/image2D.hh>

#include <typed-geometry/tg.hh>

#include "CameraController.hh"
#include "RenderInfo.hh"
#include "renderables/Renderable.hh"

namespace glow::viewer
{
struct SubViewData;

class ViewerRenderer
{
private:
    // == Shaders ==
    SharedProgram mShaderBackground;
    SharedProgram mShaderOutline;
    SharedProgram mShaderSSAO;
    SharedProgram mShaderAccum;
    SharedProgram mShaderOutput;
    SharedProgram mShaderGround;
    SharedProgram mShaderShadow;
    SharedProgram mShaderPickingVis;

    // == Mesh ==
    SharedVertexArray mMeshQuad;

    // == Framebuffers ==
    SharedFramebuffer mFramebuffer;
    SharedFramebuffer mFramebufferColor;
    SharedFramebuffer mFramebufferColorOverlay;
    SharedFramebuffer mFramebufferSSAO;
    SharedFramebuffer mFramebufferOutput;
    SharedFramebuffer mFramebufferShadow;
    SharedFramebuffer mFramebufferShadowSoft;
    SharedFramebuffer mFramebufferPicking;
    SharedFramebuffer mFramebufferVisPicking;

    // == Other ==
    tg::rng mRng;
    vector::OGLRenderer mVectorRenderer;
    vector::image2D mVectorImage;

    // == Config ==
    int const mMinAccumCnt = 128;
    int const mMinSSAOCnt = 8196;
    int const mAccumPerFrame = 1;
    float const mDepthThresholdFactor = 1.0f;
    float const mNormalThreshold = 0.9f;
    float const mGroundOffsetFactor = 0.001f;

    int mSSAOSamples = 16;
    int mShadowSamplesPerFrame = 8;
    bool mIsCurrentFrameFullyConverged = true;

    bool mReverseZEnabled = true;

    // == Picking ==
    std::vector<SharedRenderable> mAllPickableRenderables;
    SharedRenderable mLastPickedRenderable;

public:
    TexturePool<TextureRectangle> texturePoolRect;
    TexturePool<Texture2D> texturePool2D;

    picking_result pickRes;
    std::optional<tg::ivec2> current_picked_ID; // (RenderableID, FaceID)
    std::optional<tg::ipos2> normal_check_values;
    std::optional<tg::vec3> current_picked_normal;

    ViewerRenderer();

    void beginFrame(tg::color3 const& clearColor, bool bSkipOutput = false);
    void renderSubview(tg::isize2 const& res, tg::ipos2 const& offset, SubViewData& subViewData, Scene const& scene, CameraController& cam, bool bSkipOutput = false);
    void endFrame(float approximateRenderTime = 0.f);
    void maximizeSamples();
    void set_2d_transform(tg::mat3x2 const& transform) { mVectorRenderer.set_global_transform(transform); }

    bool isReverseZEnabled() const { return mReverseZEnabled; }

    cc::optional<tg::pos3> query3DPosition(tg::isize2 resolution, tg::ipos2 pixel, SubViewData const& subViewData, CameraController const& cam) const;
    cc::optional<float> queryDepth(tg::isize2 resolution, tg::ipos2 pixel, SubViewData const& subViewData) const;

    void handlePicking(tg::isize2 resolution, tg::ipos2 pixel, SubViewData& subViewData, CameraController const& cam, bool mouse_left, bool mouse_right);
};


// Cross-frame persistent data per subview
struct SubViewData
{
    static auto constexpr shadowMapSize = tg::isize2(2048, 2048);

private:
    // For pooled texture allocation
    ViewerRenderer* const renderer;
    GLOW_NON_COPYABLE(SubViewData);

public:
    SharedTextureRectangle targetAccumWrite;
    SharedTextureRectangle targetAccumRead;
    SharedTextureRectangle targetDepth;
    SharedTextureRectangle targetOutput;
    SharedTextureRectangle targetSsao;
    SharedTexture2D shadowMapSoft;
    SharedTexture2D targetOutput2D;

    SharedTextureRectangle targetPicking;

    int accumCount = 0;
    int ssaoSampleCount = 0;
    int shadowSampleCount = 0;

    double mElapsedSeconds = 0.;

    tg::mat4 lastView = tg::mat4::identity;
    tg::pos3 lastPos = tg::pos3::zero;

    size_t lastHash = 0;

    SubViewData(int w, int h, ViewerRenderer* r, bool bOutput2D = false) : renderer(r)
    {
        auto const size = TextureRectangle::SizeT(w, h);

        targetAccumRead = renderer->texturePoolRect.alloc({GL_RGBA32F, size});
        targetAccumWrite = renderer->texturePoolRect.alloc({GL_RGBA32F, size});

        if (renderer->isReverseZEnabled())
            targetDepth = renderer->texturePoolRect.alloc({GL_DEPTH_COMPONENT32F, size});
        else
            targetDepth = renderer->texturePoolRect.alloc({GL_DEPTH_COMPONENT32, size});

        targetOutput = renderer->texturePoolRect.alloc({GL_RGBA16F, size});

        if (bOutput2D)
        {
            targetOutput2D = renderer->texturePool2D.alloc({GL_RGBA16F, size});
        }

        targetSsao = renderer->texturePoolRect.alloc({GL_R32F, size});

        targetPicking = renderer->texturePoolRect.alloc({GL_RG32I, size});
        targetPicking->bind().setMagFilter(GL_NEAREST);
        targetPicking->bind().setMinFilter(GL_NEAREST);

        shadowMapSoft = renderer->texturePool2D.alloc({GL_R32F, shadowMapSize, 0});
        shadowMapSoft->bind().setAnisotropicFiltering(16);
    }

    ~SubViewData()
    {
        renderer->texturePoolRect.free(&targetAccumRead);
        renderer->texturePoolRect.free(&targetAccumWrite);
        renderer->texturePoolRect.free(&targetDepth);
        renderer->texturePoolRect.free(&targetOutput);
        renderer->texturePoolRect.free(&targetSsao);
        renderer->texturePoolRect.free(&targetPicking);
        renderer->texturePool2D.free(&shadowMapSoft);
        if (targetOutput2D)
        {
            renderer->texturePool2D.free(&targetOutput2D);
        }
    }

    void clearAccumBuffer()
    {
        accumCount = 0;
        ssaoSampleCount = 0;
    }

    void clearShadowMap() { shadowSampleCount = 0; }

    ViewerRenderer* getRenderer() const { return renderer; }
};
}
