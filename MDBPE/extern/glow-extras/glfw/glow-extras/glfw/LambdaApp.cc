#include "LambdaApp.hh"

namespace glow
{
namespace glfw
{
LambdaApp::LambdaApp(std::function<void(LambdaApp *)> fInit, //
                     std::function<void(LambdaApp *, float)> fRender,
                     std::function<void(LambdaApp *, float)> fUpdate)
  : mInitFunc(fInit), //
    mRenderFunc(fRender),
    mUpdateFunc(fUpdate)
{
}

void LambdaApp::init()
{
    GlfwApp::init();

    if (mInitFunc)
        mInitFunc(this);
}

void LambdaApp::update(float elapsedSeconds)
{
    GlfwApp::update(elapsedSeconds);

    if (mUpdateFunc)
        mUpdateFunc(this, elapsedSeconds);
}

void LambdaApp::render(float elapsedSeconds)
{
    GlfwApp::render(elapsedSeconds);

    if (mRenderFunc)
        mRenderFunc(this, elapsedSeconds);
}

SharedLambdaApp make_app(std::function<void(LambdaApp *, float)> fRender, std::function<void(LambdaApp *, float)> fUpdate)
{
    return std::make_shared<LambdaApp>(nullptr, fRender, fUpdate);
}
}
}
