#pragma once

#include <memory>
#include <vector>

#include <glow/common/optional.hh>
#include <glow/fwd.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/Texture2DArray.hh>
#include <glow/objects/TextureRectangle.hh>
#include <glow/util/TexturePool.hh>

#include <glow-extras/camera/CameraBase.hh>

#include "fwd.hh"
#include "stages/StageCamera.hh"

namespace glow
{
namespace pipeline
{
GLOW_SHARED(class, RenderPipeline);
class RenderPipeline
{
private:
    // == Texture Pools ==
    TexturePool<Texture2D> mTexPool2D;
    TexturePool<Texture2DArray> mTexPool2DArray;
    TexturePool<TextureRectangle> mTexPoolRect;

    // == UBOs ==
    SharedUniformBuffer mUboGlobalConfig;
    SharedUniformBuffer mUboRenderScene;

    // == Stages ==
    std::vector<SharedRenderStage> mRenderStages;
    SharedOutputStage mOutputStage;
    SharedDepthPreStage mDepthStage = nullptr;

public:
    RenderPipeline();
    ~RenderPipeline();

    void addStage(SharedRenderStage const& stage);
    void addOutputStage(SharedOutputStage const& outputStage);

    /// Optionally registers a depth stage
    /// This is required for depth queries
    void registerDepthStage(SharedDepthPreStage const& depthPreStage);

    void setProjectionInfoUbo(SharedProgram const& shader) const;
    void setSceneUbo(SharedProgram const& shader) const;

    /// Renders and presents a frame
    void render(StageCamera const& stageCam, RenderScene& scene, RenderCallback& rc);

    /// Restricts the output to a given viewport
    void restrictOutputViewport(tg::ipos2 const& offset, tg::isize2 const& size);

    /// Removes the output viewport restriction
    void clearOutputViewport();

    /// Queries a position in the currently rendered depth buffer
    /// Requires a registered depth stage (via registerDepthStage)
    optional<tg::pos3> queryPosition3D(StageCamera const& stageCam, tg::ipos2 const& pixel) const;

    // -- Getters --
    TexturePool<Texture2D>& getTexPool2D() { return mTexPool2D; }
    TexturePool<Texture2DArray>& getTexPool2DArray() { return mTexPool2DArray; }
    TexturePool<TextureRectangle>& getTexPoolRect() { return mTexPoolRect; }

    /// Deallocate all texture pool targets
    void freeAllTargets();

public:
    static SharedRenderPipeline createDefaultPipeline();

    static void GlobalInit();
};
}
}
