#include "GlfwApp.hh"

#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
#include <AntTweakBar.h>
#endif

#ifdef GLOW_EXTRAS_HAS_IMGUI
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imguizmo.h>

#ifdef IMGUI_USE_IMPLOT
#include <imgui/implot.h>
#endif

#endif

#include <typed-geometry/tg.hh>

#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

#include <glow/gl.hh>

// NOTE: AFTER gl.hh
#include <GLFW/glfw3.h>

#ifdef GLOW_EXTRAS_HAS_AION
#include <aion/ActionAnalyzer.hh>
#endif

#include <glow/common/log.hh>
#include <glow/common/scoped_gl.hh>
#include <glow/common/str_utils.hh>
#include <glow/glow.hh>

#include <glow/util/DefaultShaderParser.hh>

#include <glow/objects/OcclusionQuery.hh>
#include <glow/objects/PrimitiveQuery.hh>
#include <glow/objects/Timestamp.hh>

#include <glow-extras/debugging/DebugOverlay.hh>
#include <glow-extras/debugging/profiling/ProfilingOverlay.hh>
#include <glow-extras/pipeline/RenderPipeline.hh>
#include <glow-extras/pipeline/RenderScene.hh>

#include "GlfwContext.hh"

using namespace glow;
using namespace glow::glfw;

static GlfwApp* sCurrApp = nullptr;

#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
static void TW_CALL GlfwAppTweakCallback(void* clientData) { (*(std::function<void()>*)clientData)(); }
#endif

[[maybe_unused]] static std::string thousandSep(size_t val)
{
    auto s = std::to_string(val);
    auto l = s.size();
    while (l > 3)
    {
        s = s.substr(0, l - 3) + "'" + s.substr(l - 3);
        l -= 3;
    }
    return s;
}

void GlfwApp::setTitle(const std::string& title)
{
    mTitle = title;
    if (mWindow)
        glfwSetWindowTitle(mWindow, title.c_str());
}

void GlfwApp::setClipboardString(const std::string& s) const { glfwSetClipboardString(mWindow, s.c_str()); }

std::string GlfwApp::getClipboardString() const
{
    auto s = glfwGetClipboardString(mWindow);
    return s ? s : "";
}

bool GlfwApp::shouldClose() const { return glfwWindowShouldClose(mWindow); }

bool GlfwApp::isFullscreen() const { return mInternalContext->isFullscreen(); }

bool GlfwApp::isMinimized() const { return mMinimized; }

void GlfwApp::requestClose() { glfwSetWindowShouldClose(mWindow, true); }

#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
void GlfwApp::tweak(int& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_INT32, &value, options.c_str());
}

void GlfwApp::tweak(bool& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_BOOLCPP, &value, options.c_str());
}

void GlfwApp::tweak(float& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_FLOAT, &value, options.c_str());
}

void GlfwApp::tweak(double& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_DOUBLE, &value, options.c_str());
}

void GlfwApp::tweak(glm::quat& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_QUAT4F, &value, options.c_str());
}

void GlfwApp::tweak(std::string& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_STDSTRING, &value, options.c_str());
}

void GlfwApp::tweak_dir(glm::vec3& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_DIR3F, &value, options.c_str());
}

void GlfwApp::tweak_color(glm::vec3& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_COLOR3F, &value, options.c_str());
}

void GlfwApp::tweak_color(glm::vec4& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_COLOR4F, &value, options.c_str());
}

void GlfwApp::tweak_color(uint32_t& value, std::string const& name, std::string const& options)
{
    TwAddVarRW(tweakbar(), name.c_str(), TW_TYPE_COLOR32, &value, options.c_str());
}

void GlfwApp::tweak_button(std::string const& name, std::function<void()> const& fun, std::string const& options)
{
    TwAddButton(tweakbar(), name.c_str(), GlfwAppTweakCallback, new std::function<void()>(fun), options.c_str());
}
#endif

void GlfwApp::init()
{
    if (!mUseDefaultCamera && mUsePipeline)
        glow::error() << "Cannot use pipeline without default camera in GlfwApp.";

    if (mUseDefaultCamera || mUsePipeline)
    {
        // create camera with some sensible defaults
        mCamera = camera::SmoothedCamera::create();
        mCamera->handle.setLookAt(tg::pos3{2}, tg::pos3{0});
    }

    if (mUsePipeline)
    {
        // set up ng pipeline
        pipeline::RenderPipeline::GlobalInit();
        mPipeline = pipeline::RenderPipeline::createDefaultPipeline();
        mPipelineScene = pipeline::RenderScene::create();
        mPipelineCamera = pipeline::StageCamera::create();

        if (mUsePipelineConfigGui)
        {
            if (mGui != Gui::ImGui)
            {
                glow::error() << "Pipeline config gui requires ImGui";
                mUsePipelineConfigGui = false;
            }
        }
    }
    else
    {
        mUsePipelineConfigGui = false;
    }

    mPrimitiveQuery = PrimitiveQuery::create();
    mOcclusionQuery = OcclusionQuery::create();

    mGpuTimer = timing::GpuTimer::create();

#ifdef GLOW_EXTRAS_HAS_IMGUI
    mProfilingOverlay = debugging::ProfilingOverlay::create();
#endif

    // init UI
    switch (mGui)
    {
    // anttweakbar
    case Gui::AntTweakBar:
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
        TG_ASSERT(mTweakbar == nullptr);
        TwInit(TW_OPENGL_CORE, nullptr); // for core profile
        TwWindowSize(mWindowWidth, mWindowHeight);
        mTweakbar = TwNewBar("Tweakbar");
#else
        glow::warning() << "AntTweakBar GUI not supported (are you missing a dependency?)";
#endif
        break;

    // imgui
    case Gui::ImGui:
#ifdef GLOW_EXTRAS_HAS_IMGUI
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
#ifdef IMGUI_USE_IMPLOT
        ImPlot::CreateContext();
#endif
        ImGui_ImplGlfw_InitForOpenGL(window(), false);
        char* glsl_version = nullptr;
#ifdef __APPLE__
        glsl_version = "#version 150";
#endif
        ImGui_ImplOpenGL3_Init(glsl_version);
    }
#else
        glow::warning() << "ImGUI not supported (are you missing a dependency?)";
#endif
    break;

    case Gui::None:
        break;
    }

    // show window
    {
        bool setPositionFromCache = false;

        if (mCacheWindowSize)
        {
            // Restore window size
            std::ifstream file(mCacheWindowSizeFilename);
            if (file.good())
            {
                int w, h, x, y;
                file >> w >> h >> x >> y;

                if (w == 0 && h == 0)
                {
                    mInternalContext->enterFullscreen();

                    // Set reasonable position and size for leaving fullscreen
                    mInternalContext->setCachedPosition(50, 50);
                    mInternalContext->setCachedSize(mWindowWidth, mWindowHeight);
                }
                else
                {
                    mInternalContext->resize(w, h);
                    mInternalContext->reposition(x, y);
                }

                setPositionFromCache = true;
            }
        }

        if (!setPositionFromCache)
        {
            mInternalContext->resize(mWindowWidth, mWindowHeight);
        }

        if (!mStartInvisible)
            mInternalContext->show();
    }
}

void GlfwApp::update(float) {}

void GlfwApp::render(float)
{
    if (mUsePipeline)
    {
        GLOW_SCOPED(debugGroup, "GlfwApp RenderPipeline");
        TG_ASSERT(mPipeline && "did you forgot to call GlfwApp::init() in your init?");
        mPipelineCamera->onNewFrame(*mCamera);
        mPipeline->render(*mPipelineCamera, *mPipelineScene, *this);
    }
}

void GlfwApp::scheduledUpdate() {}

void GlfwApp::onResize(int w, int h)
{
    if (mCamera)
        mCamera->setViewportSize(w, h);
}

void GlfwApp::onClose() {}

void GlfwApp::onGui() {}

bool GlfwApp::onKey(int key, int scancode, int action, int mods)
{
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
    if (mGui == Gui::AntTweakBar && TwEventKeyGLFW(mWindow, key, scancode, action, mods))
        return true;
#endif

#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mGui == Gui::ImGui)
    {
        ImGui_ImplGlfw_KeyCallback(window(), key, scancode, action, mods);

        if (ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantTextInput)
            return true;
    }

#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (key == GLFW_KEY_F9 && action == GLFW_PRESS)
    {
        toggleProfilingOverlay();
        return true;
    }
#endif

    if (mEnableDebugOverlay)
    {
        if (key == GLFW_KEY_F10 && action == GLFW_PRESS)
        {
            toggleDebugOverlay();
            return true;
        }
    }
#endif

    if (key == GLFW_KEY_HOME && action != GLFW_RELEASE)
    {
        onResetView();
        return true;
    }

    return false;
}

bool GlfwApp::onChar(unsigned int codepoint, int /*mods*/)
{
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
    if (mGui == Gui::AntTweakBar && TwEventCharGLFW(mWindow, codepoint))
        return true;
#endif

#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mGui == Gui::ImGui)
    {
        ImGui_ImplGlfw_CharCallback(window(), codepoint);

        if (ImGui::GetIO().WantTextInput)
            return true;
    }
#endif

    return false;
}

bool GlfwApp::onMousePosition(double x, double y)
{
    mMouseMoveDistance += length(mInputState.getMouseDelta());
    (void)x;
    (void)y;
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
    if (mGui == Gui::AntTweakBar && TwEventMousePosGLFW(mWindow, x, y))
        return true;
#endif

#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mGui == Gui::ImGui)
    {
        if (ImGui::GetIO().WantCaptureMouse)
            return true;

        if (ImGuizmo::IsUsing())
            return true;
    }
#endif

    return false;
}

bool GlfwApp::onMouseButton(double x, double y, int button, int action, int mods, int clickCount)
{
    (void)x;
    (void)y;
    (void)clickCount;

#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
    if (mGui == Gui::AntTweakBar && mCursorMode == glfw::CursorMode::Normal && TwEventMouseButtonGLFW(mWindow, button, action, mods))
        return true;
#endif

#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mGui == Gui::ImGui)
    {
        ImGui_ImplGlfw_MouseButtonCallback(window(), button, action, mods);

        if (ImGui::GetIO().WantCaptureMouse)
            return true;

        if (ImGuizmo::IsUsing())
            return true;
    }
#endif

    return false;
}

bool GlfwApp::onMouseScroll(double sx, double sy)
{
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
    if (mGui == Gui::AntTweakBar && TwEventMouseWheelGLFW(mWindow, sx, sy))
        return true;
#endif

#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mGui == Gui::ImGui)
    {
        ImGui_ImplGlfw_ScrollCallback(window(), sx, sy);

        if (ImGui::GetIO().WantCaptureMouse)
            return true;

        if (ImGuizmo::IsUsing())
            return true;
    }
#endif

    return false;
}

bool GlfwApp::onMouseEnter() { return false; }

bool GlfwApp::onMouseExit() { return false; }

bool GlfwApp::onFocusGain() { return false; }

bool GlfwApp::onFocusLost() { return false; }

bool GlfwApp::onFileDrop(const std::vector<std::string>&) { return false; }

void GlfwApp::onResetView()
{
    if (mUseDefaultCamera)
        mCamera->setLookAt(tg::pos3::zero - glow::transform::Forward() * mCamera->getLookAtDistance(), {0, 0, 0});
}

void GlfwApp::mainLoop()
{
    // Loop until the user closes the window

    [[maybe_unused]] unsigned frames = 0;
    double lastTime = glfwGetTime();
    [[maybe_unused]] double lastStatsTime = lastTime;
    double lastScheduledUpdateTime = lastTime;
    double timeAccum = 0.000001;
    mCurrentTime = 0.0;
    [[maybe_unused]] size_t primitives = 0;
    [[maybe_unused]] size_t fragments = 0;
    [[maybe_unused]] double cpuRenderTime = 0;
    int updatesPerFrame = 1;
    mCurrentRenderDeltaTime = 1.0 / mUpdateRate; // tracks time between renders, init is for first frame only
    while (!shouldClose())
    {
        updateInput();
        float cpuRenderDispatchTime = 0.f;

        // Update
        {
            double const dt = 1.0 / mUpdateRate;

            // # of updates
            auto updates = updatesPerFrame;
            if (timeAccum > updatesPerFrame * dt) // lags one behind: do one more
                ++updates;
            if (timeAccum < -dt) // is more than one ahead: skip one
                --updates;

            // do updates
            for (auto i = 0; i < updates; ++i)
            {
                update(float(dt));
                timeAccum -= dt;
                mCurrentTime += dt;
            }

            // update adjustment (AFTER updates! timeAccum should be within -dt..dt now)
            if (timeAccum > 2.5 * dt)
            {
                ++updatesPerFrame;
                // glow::info() << "increasing frames per sec";
            }
            else if (timeAccum < -2.5 * dt)
            {
                if (updatesPerFrame > 0)
                    --updatesPerFrame;
                // glow::info() << "decreasing frames per sec";
            }

            // frameskip
            if (timeAccum > mMaxFrameSkip * dt)
            {
                if (mWarnOnFrameskip)
                    glow::warning() << "Too many updates queued, frame skip of " << timeAccum << " secs";
                timeAccum = mMaxFrameSkip * dt * 0.5;
            }

            // glow::info() << updates << ", " << timeAccum / dt;
        }

        // Camera Update
        if (mCamera && mUseDefaultCameraHandling)
        {
            auto doCameraHandling = true;
#ifdef GLOW_EXTRAS_HAS_IMGUI
            if (mGui == Gui::ImGui)
            {
                if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard)
                    doCameraHandling = false;
            }
#endif

            if (doCameraHandling)
                mCamera->update(static_cast<float>(mCurrentRenderDeltaTime), mInputState, mWASDController, mLookAroundController, mTargetOrbitController);
        }

        if (!mMinimized)
        {
            beginRender();

            // GUI (before)
            if (!mGuiAfterRender)
                internalPerformGui();

            // Render here
            {
                if (mPrimitiveQueryStats)
                {
                    mPrimitiveQuery->begin();
                    mOcclusionQuery->begin();
                }

                {
                    auto const cpuStart = glfwGetTime();
                    auto timerScope = mGpuTimer->scope();

                    render(static_cast<float>(mCurrentRenderDeltaTime));

                    cpuRenderDispatchTime = static_cast<float>(glfwGetTime() - cpuStart);
                    cpuRenderTime += cpuRenderDispatchTime;
                }

                if (mPrimitiveQueryStats)
                {
                    mPrimitiveQuery->end();
                    mOcclusionQuery->end();
                }
            }

            // GUI (after)
            if (mGuiAfterRender)
                internalPerformGui();

            endRender();
        }

        // timing
        auto now = glfwGetTime();
        mCurrentRenderDeltaTime = now - lastTime;
        timeAccum += now - lastTime;
        lastTime = now;
        ++frames;

        if (mPrimitiveQueryStats)
        {
            primitives += mPrimitiveQuery->getResult64();
            fragments += mOcclusionQuery->getResult64();
        }

        if (mScheduledUpdateInterval > 0 && lastTime > lastScheduledUpdateTime + mScheduledUpdateInterval)
        {
            scheduledUpdate();
            lastScheduledUpdateTime = lastTime;
        }

        {
            mLastGpuTimeMs = mGpuTimer->elapsedSeconds() * 1000;
            mLastCpuTimeMs = cpuRenderDispatchTime * 1000;

#ifdef GLOW_EXTRAS_HAS_IMGUI
            mProfilingOverlay->onFrame(mLastGpuTimeMs, mLastCpuTimeMs);
#else
            if (mOutputStatsInterval > 0 && lastTime > lastStatsTime + mOutputStatsInterval)
            {
                double fps = frames / (lastTime - lastStatsTime);
                std::ostringstream ss;
                ss << std::setprecision(3);
                ss << "FPS: " << fps;
                ss << ", frametime: " << 1000.0 / fps << " ms";
                ss << ", CPU: " << mLastCpuTimeMs << " ms";
                ss << ", GPU: " << mLastGpuTimeMs << " ms";
                if (mPrimitiveQueryStats)
                {
                    ss << ", primitives: " << thousandSep(primitives / frames);
                    ss << ", frags: " << thousandSep(fragments / frames);
                }
                info() << ss.str();

                lastStatsTime = lastTime;
                frames = 0;
                primitives = 0;
                fragments = 0;
                cpuRenderTime = 0;
            }
#endif
        }
    }

    // clean up
    internalCleanUp();
}

void GlfwApp::internalInit()
{
    TG_ASSERT(sCurrApp == nullptr && "cannot run multiple apps simulatenously");
    sCurrApp = this;

    TG_ASSERT(mWindow == nullptr);

    // create and get glfw context
    mInternalContext = GlfwContext::current();
    mInternalContextOwner = mInternalContext == nullptr;
    if (mInternalContextOwner)
        mInternalContext = new GlfwContext;

    mWindow = mInternalContext->window();

    // create secondary context
    if (mCreateSecondaryContext)
    {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        mSecondaryContext = glfwCreateWindow(15, 15, "Loader Thread Context", nullptr, mWindow);
    }

    // unbind any ogl object (especially from AntTweakBar)
    glow::unbindOpenGLObjects();

    // init app
    init();

    // input callbacks
    {
        glfwSetKeyCallback(mWindow,
                           [](GLFWwindow*, int key, int scancode, int action, int mods)
                           {
                               if (action == GLFW_PRESS)
                                   sCurrApp->mInputState.onKeyPress(key);
                               else if (action == GLFW_RELEASE)
                                   sCurrApp->mInputState.onKeyRelease(key);

                               sCurrApp->onKey(key, scancode, action, mods);
                           });
        glfwSetCharModsCallback(mWindow, [](GLFWwindow*, unsigned int codepoint, int mods) { sCurrApp->onChar(codepoint, mods); });
        glfwSetMouseButtonCallback(mWindow,
                                   [](GLFWwindow*, int button, int action, int mods)
                                   {
                                       if (action == GLFW_PRESS)
                                           sCurrApp->mInputState.onMouseButtonPress(button);
                                       else if (action == GLFW_RELEASE)
                                           sCurrApp->mInputState.onMouseButtonRelease(button);

                                       sCurrApp->internalOnMouseButton(sCurrApp->mInputState.getMousePosition().x,
                                                                       sCurrApp->mInputState.getMousePosition().y, button, action, mods);
                                   });
        glfwSetCursorEnterCallback(mWindow,
                                   [](GLFWwindow*, int entered)
                                   {
                                       if (entered)
                                           sCurrApp->onMouseEnter();
                                       else
                                           sCurrApp->onMouseExit();
                                   });

        glfwSetCursorPosCallback(mWindow,
                                 [](GLFWwindow*, double x, double y)
                                 {
                                     sCurrApp->mInputState.onMousePos(x, y);
                                     sCurrApp->onMousePosition(x, y);
                                 });
        glfwSetScrollCallback(mWindow,
                              [](GLFWwindow*, double sx, double sy)
                              {
                                  sCurrApp->mInputState.onScroll(static_cast<float>(sx), static_cast<float>(sy));
                                  sCurrApp->onMouseScroll(sx, sy);
                              });
        glfwSetFramebufferSizeCallback(mWindow,
                                       [](GLFWwindow*, int w, int h)
                                       {
                                           sCurrApp->mWindowWidth = w;
                                           sCurrApp->mWindowHeight = h;

                                           sCurrApp->onResize(w, h);
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
                                           if (sCurrApp->mGui == Gui::AntTweakBar)
                                               TwWindowSize(w, h);
#endif
                                       });
        glfwSetWindowFocusCallback(mWindow,
                                   [](GLFWwindow*, int focused)
                                   {
                                       if (focused)
                                           sCurrApp->onFocusGain();
                                       else
                                           sCurrApp->onFocusLost();
                                   });
        glfwSetDropCallback(mWindow,
                            [](GLFWwindow*, int count, const char** paths)
                            {
                                std::vector<std::string> files;
                                for (auto i = 0; i < count; ++i)
                                    files.push_back(paths[i]);
                                sCurrApp->onFileDrop(files);
                            });
        glfwSetWindowIconifyCallback(mWindow, [](GLFWwindow*, int iconified) { sCurrApp->mMinimized = static_cast<bool>(iconified); });
    }

#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mEnableDebugOverlay && mGui == Gui::ImGui)
    {
        debugging::DebugOverlay::Init();
    }
#endif

    glfwGetFramebufferSize(mWindow, &mWindowWidth, &mWindowHeight);
    onResize(mWindowWidth, mWindowHeight);
}

void GlfwApp::internalCleanUp()
{
    if (mIsCleaned)
        return;
    mIsCleaned = true;

    if (mCacheWindowSize)
    {
        // Save window size to disk
        std::ofstream file(mCacheWindowSizeFilename);
        if (file.good())
        {
            if (mInternalContext->isFullscreen())
                file << "0 0 0 0\n";
            else
            {
                int x, y, w, h;
                glfwGetWindowPos(mWindow, &x, &y);
                glfwGetWindowSize(mWindow, &w, &h);
                file << w << " " << h << " " << x << " " << y << "\n";
            }
        }
    }

    onClose();

    // cleanup UI
    switch (mGui)
    {
    // anttweakbar
    case Gui::AntTweakBar:
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
        TwTerminate();
#endif
        break;

    // imgui
    case Gui::ImGui:
#ifdef GLOW_EXTRAS_HAS_IMGUI
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#ifdef IMGUI_USE_IMPLOT
        ImPlot::DestroyContext();
#endif
#endif
        break;

    case Gui::None:
        break;
    }

    // aion dump
#ifdef GLOW_EXTRAS_HAS_AION
    if (mDumpTimingsOnShutdown)
        aion::ActionAnalyzer::dumpSummary(std::cout, false);
#endif

    // remove close flag
    glfwSetWindowShouldClose(mWindow, GLFW_FALSE);

    // clear callbacks
    glfwSetKeyCallback(mWindow, nullptr);
    glfwSetCharModsCallback(mWindow, nullptr);
    glfwSetMouseButtonCallback(mWindow, nullptr);
    glfwSetCursorEnterCallback(mWindow, nullptr);

    glfwSetCursorPosCallback(mWindow, nullptr);
    glfwSetScrollCallback(mWindow, nullptr);
    glfwSetFramebufferSizeCallback(mWindow, nullptr);
    glfwSetWindowFocusCallback(mWindow, nullptr);
    glfwSetDropCallback(mWindow, nullptr);

    // release context
    if (mInternalContextOwner)
        delete mInternalContext;
    else
        mInternalContext->hide();

    sCurrApp = nullptr;
}

void GlfwApp::internalOnMouseButton(double x, double y, int button, int action, int mods)
{
    // check double click
    if (action == GLFW_PRESS)
    {
        if (distance(mClickPos, tg::pos2(x, y)) > 5) // too far
            mClickCount = 0;
        if (mClickTimer.elapsedSecondsD() > mDoubleClickTime) // too slow
            mClickCount = 0;
        if (mClickButton != button) // wrong button
            mClickCount = 0;
        mClickCount++;
        mMouseMoveDistance = 0;
        mClickTimer.restart();
        mClickButton = button;
        mClickPos = tg::pos2(x, y);
    }

    onMouseButton(x, y, button, action, mods, mClickCount);
}

void GlfwApp::internalOnGui()
{
#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mEnableDebugOverlay)
        debugging::DebugOverlay::OnGui();

    if (mUsePipelineConfigGui)
        mPipelineScene->imguiConfigWindow();

    if (mProfilingOverlayVisible)
        mProfilingOverlay->onGui();
#endif
}

void GlfwApp::internalPerformGui()
{
#ifdef GLOW_EXTRAS_HAS_IMGUI
    if (mGui == Gui::ImGui)
    {
        GLOW_SCOPED(debugGroup, "ImGui");
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuizmo::BeginFrame();
        ImGuizmo::SetRect(0, 0, mWindowWidth, mWindowHeight);

        onGui();
        internalOnGui();

        ImGui::EndFrame();
    }
#endif
}

void GlfwApp::updateInput()
{
    // update cursor mode
    switch (mCursorMode)
    {
    case CursorMode::Normal:
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    case CursorMode::Hidden:
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        break;
    case CursorMode::Disabled:
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    }

    // Poll for and process events
    mInputState.swap();
    glfwPollEvents();
}

void GlfwApp::beginRender()
{
    // vsync
    glfwSwapInterval(mVSync ? mSwapInterval : 0);

    // viewport
    glViewport(0, 0, mWindowWidth, mWindowHeight);
}

void GlfwApp::endRender()
{
    if (mDrawGui)
    {
        // draw the tweak bar(s)
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
        if (mGui == Gui::AntTweakBar)
        {
            TwDraw();

            // unbind TweakBar stuff
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glUseProgram(0);
            glBindVertexArray(0);
        }
#endif

        // draw imgui
#ifdef GLOW_EXTRAS_HAS_IMGUI
        if (mGui == Gui::ImGui)
        {
            ImGui::Render();
            glViewport(0, 0, getWindowWidth(), getWindowHeight());
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
#endif
    }

    // Swap front and back buffers
    glfwSwapBuffers(mWindow);
}

void GlfwApp::sleepSeconds(double seconds) const
{
    if (seconds <= 0.0)
        return;

    std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int64_t>(seconds * 1000 * 1000)));
}

void GlfwApp::toggleFullscreen() { mInternalContext->toggleFullscreen(); }

void GlfwApp::toggleProfilingOverlay() { mProfilingOverlayVisible = !mProfilingOverlayVisible; }

#ifdef GLOW_EXTRAS_HAS_IMGUI // imgui support
void GlfwApp::toggleDebugOverlay() { debugging::DebugOverlay::ToggleVisibility(); }
#endif

void GlfwApp::run()
{
    internalInit();
    mainLoop();
}

void GlfwApp::startHeadless() { internalInit(); }

GlfwApp::~GlfwApp() { internalCleanUp(); }

GlfwApp* glfw::detail::current_app() { return sCurrApp; }
