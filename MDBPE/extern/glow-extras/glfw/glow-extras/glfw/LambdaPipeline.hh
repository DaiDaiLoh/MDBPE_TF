#pragma once

#include <functional>

#include "GlfwApp.hh"

namespace glow
{
namespace glfw
{
GLOW_SHARED(class, LambdaPipeline);
class LambdaPipeline : public GlfwApp
{
public:
    using RenderPassFunction
        = std::function<void(LambdaPipeline*, glow::pipeline::RenderScene const&, glow::pipeline::RenderStage const&, glow::pipeline::CameraData const&)>;

    LambdaPipeline(std::function<void(LambdaPipeline*)> fInit,
                   RenderPassFunction fOpaque,
                   RenderPassFunction fTransparent = nullptr,
                   std::function<void(LambdaPipeline*, float)> fUpdate = nullptr);

    void init() override;
    void update(float elapsedSeconds) override;

protected:
    void render(float dt) override;

    void onPerformShadowPass(glow::pipeline::RenderScene const& p,
                             glow::pipeline::RenderStage const& s,
                             glow::pipeline::CameraData const& d,
                             glow::UsedProgram& shader) override;
    void onRenderDepthPrePass(glow::pipeline::RenderScene const& p,
                              glow::pipeline::RenderStage const& s,
                              glow::pipeline::CameraData const& d) override;
    void onRenderOpaquePass(glow::pipeline::RenderScene const& p,
                            glow::pipeline::RenderStage const& s,
                            glow::pipeline::CameraData const& d) override;
    void onRenderTransparentPass(glow::pipeline::RenderScene const& p,
                                 glow::pipeline::RenderStage const& s,
                                 glow::pipeline::CameraData const& d) override;

    bool onKey(int key, int scancode, int action, int mods) override;

    bool onMouseButton(double x, double y, int button, int action, int mods, int clickCount) override;

private:
    std::function<void(LambdaPipeline*)> mInitFunc = nullptr;
    std::function<void(LambdaPipeline*, float)> mUpdateFunc = nullptr;
    std::function<void(LambdaPipeline*, float)> mPreRenderFunc = nullptr;
    std::function<void(LambdaPipeline*, int, int, int, int)> mKeyHandleFunc = nullptr;
    std::function<void(LambdaPipeline*, double, double, int, int, int, int)> mMouseHandleFunc = nullptr;

    RenderPassFunction mDepthPreFunc = nullptr;
    RenderPassFunction mOpaqueFunc = nullptr;
    RenderPassFunction mTransparentFunc = nullptr;

public:
    GLOW_PROPERTY(InitFunc);
    GLOW_PROPERTY(UpdateFunc);
    GLOW_PROPERTY(PreRenderFunc);
    GLOW_PROPERTY(KeyHandleFunc);
    GLOW_PROPERTY(MouseHandleFunc);

    GLOW_PROPERTY(DepthPreFunc);
    GLOW_PROPERTY(OpaqueFunc);
    GLOW_PROPERTY(TransparentFunc);
};

/// creates a lambda pipeline
SharedLambdaPipeline make_pipeline(LambdaPipeline::RenderPassFunction fOpaque,
                                   LambdaPipeline::RenderPassFunction fTransparent = nullptr,
                                   std::function<void(LambdaPipeline*, float)> fUpdate = nullptr);

/// forwards all arguments to make_pipeline(...) and shows the result
template <typename... Args>
void show_pipeline(Args&&... args)
{
    auto v = make_pipeline(std::forward<Args>(args)...);
    v->run();
}

}
}
