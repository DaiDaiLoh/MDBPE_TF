#pragma once

#include <functional>

#include "GlfwApp.hh"

namespace glow
{
namespace glfw
{
GLOW_SHARED(class, LambdaApp);

/// creates a lambda app
SharedLambdaApp make_app(std::function<void(LambdaApp*, float)> fRender, //
                         std::function<void(LambdaApp*, float)> fUpdate = nullptr);

/// forwards all arguments to make_app(...) and shows the result
template <typename... Args>
void show_app(Args&&... args)
{
    auto v = make_app(std::forward<Args>(args)...);
    v->run();
}

class LambdaApp : public GlfwApp
{
public:
    LambdaApp(std::function<void(LambdaApp*)> fInit,          //
              std::function<void(LambdaApp*, float)> fRender, //
              std::function<void(LambdaApp*, float)> fUpdate);

    void init() override;
    void update(float elapsedSeconds) override;
    void render(float elapsedSeconds) override;

private:
    std::function<void(LambdaApp*)> mInitFunc;
    std::function<void(LambdaApp*, float)> mRenderFunc;
    std::function<void(LambdaApp*, float)> mUpdateFunc;

public:
    GLOW_PROPERTY(InitFunc);
    GLOW_PROPERTY(RenderFunc);
    GLOW_PROPERTY(UpdateFunc);
};
}
}
