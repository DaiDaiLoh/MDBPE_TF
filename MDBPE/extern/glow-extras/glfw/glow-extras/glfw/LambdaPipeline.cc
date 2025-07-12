#include "LambdaPipeline.hh"

#include <glow-extras/camera/CameraBase.hh>
#include <glow-extras/pipeline/RenderPipeline.hh>

namespace glow
{
namespace glfw
{
LambdaPipeline::LambdaPipeline(std::function<void(LambdaPipeline*)> fInit,
                               LambdaPipeline::RenderPassFunction fOpaque,
                               LambdaPipeline::RenderPassFunction fTransparent,
                               std::function<void(LambdaPipeline*, float)> fUpdate)
  : mInitFunc(fInit), mUpdateFunc(fUpdate), mOpaqueFunc(fOpaque), mTransparentFunc(fTransparent)
{
}

void LambdaPipeline::init()
{
    setUsePipeline(true);
    setGui(GlfwApp::Gui::ImGui);
    setUsePipelineConfigGui(true);
    GlfwApp::init();

    if (mInitFunc)
        mInitFunc(this);
}

void LambdaPipeline::update(float elapsedSeconds)
{
    GlfwApp::update(elapsedSeconds);

    if (mUpdateFunc)
        mUpdateFunc(this, elapsedSeconds);
}

void LambdaPipeline::render(float dt)
{
    if (mPreRenderFunc)
        mPreRenderFunc(this, dt);

    GlfwApp::render(dt);
}

void LambdaPipeline::onPerformShadowPass(const pipeline::RenderScene&, const pipeline::RenderStage&, const pipeline::CameraData&, UsedProgram&) {}

void LambdaPipeline::onRenderDepthPrePass(const pipeline::RenderScene& p, const pipeline::RenderStage& s, const pipeline::CameraData& d)
{
    if (mDepthPreFunc)
        mDepthPreFunc(this, p, s, d);
}

void LambdaPipeline::onRenderOpaquePass(const pipeline::RenderScene& p, const pipeline::RenderStage& s, const pipeline::CameraData& d)
{
    if (mOpaqueFunc)
        mOpaqueFunc(this, p, s, d);
}

void LambdaPipeline::onRenderTransparentPass(const pipeline::RenderScene& p, const pipeline::RenderStage& s, const pipeline::CameraData& d)
{
    if (mTransparentFunc)
        mTransparentFunc(this, p, s, d);
}

bool LambdaPipeline::onKey(int key, int scancode, int action, int mods)
{
    if (GlfwApp::onKey(key, scancode, action, mods))
        return true;

    if (mKeyHandleFunc)
    {
        mKeyHandleFunc(this, key, scancode, action, mods);
        return true;
    }
    else
        return false;
}

bool LambdaPipeline::onMouseButton(double x, double y, int button, int action, int mods, int clickCount)
{
    if (GlfwApp::onMouseButton(x, y, button, action, mods, clickCount))
        return true;

    if (mMouseHandleFunc)
    {
        mMouseHandleFunc(this, x, y, button, action, mods, clickCount);
        return true;
    }
    else
        return false;
}

SharedLambdaPipeline make_pipeline(LambdaPipeline::RenderPassFunction fOpaque,
                                   LambdaPipeline::RenderPassFunction fTransparent,
                                   std::function<void(LambdaPipeline*, float)> fUpdate)
{
    return std::make_shared<LambdaPipeline>(nullptr, fOpaque, fTransparent, fUpdate);
}

}
}
