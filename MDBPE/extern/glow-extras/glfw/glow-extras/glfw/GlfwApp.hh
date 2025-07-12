#pragma once

#include <functional>
#include <string>
#include <vector>

#include <typed-geometry/tg-lean.hh>

#include <glow/common/log.hh>
#include <glow/common/property.hh>
#include <glow/common/shared.hh>
#include <glow/fwd.hh>

#include <glow-extras/camera/SmoothedCamera.hh>
#include <glow-extras/camera/controllers/LookAroundController.hh>
#include <glow-extras/camera/controllers/TargetOrbitController.hh>
#include <glow-extras/camera/controllers/WASDController.hh>
#include <glow-extras/input/InputState.hh>
#include <glow-extras/pipeline/RenderCallback.hh>
#include <glow-extras/pipeline/fwd.hh>
#include <glow-extras/timing/CpuTimer.hh>
#include <glow-extras/timing/GpuTimer.hh>

struct GLFWwindow;
struct CTwBar;
typedef struct CTwBar TwBar; // structure CTwBar is not exposed.

namespace glow
{
namespace debugging
{
GLOW_SHARED(class, DebugRenderer);
GLOW_SHARED(class, ProfilingOverlay);
}

namespace glfw
{
class GlfwContext;

enum class CursorMode
{
    /// normal behavior
    Normal,
    /// normal behavior but hardware cursor is hidden
    Hidden,
    /// virtual unrestricted cursor, real cursor is hidden and locked to center
    Disabled,
};

/**
 * @brief The GlfwApp can be used to efficiently build small sample applications based on glfw
 *
 * Derive your own class from GlfwApp and override the functions you need:
 *   - init(...): initialize and allocate all your resources and objects
 *   - update(...): called with a constant rate (default 60 Hz, configurable) before rendering
 *   - render(...): called as fast as possible (affected by vsync)
 *   - onResize(...): called when window is resized
 *   - onClose(...): called when app is closed
 *   - input: see onKey/onChar/onMouseXYZ/onFileDrop (return true if you handled the event, if base returned true, you
 *                                                    should probably return as well)
 * be sure to call base function unless you know what you do!
 *
 * Additional important functions:
 *   - setUpdateRate(...): set the update rate
 *   - window(): get the GLFW window
 *   - tweakbar(): get the AntTweakBar instance
 *   - setWindowWidth/Height(...): set initial window size before run(...)
 *
 * Render Pipeline:
 *   - setUsePipeline(true): Enable the default pipeline
 *   - Override all of the RenderCallback methods you need
 *   For a reference implementation of a GlfwApp using the Pipeline, see glow-samples/wip/rendering-pipeline
 *
 * Notes:
 *   - if you use primitive/occlusion queries, use setQueryStats(false);
 *   - overwrite onResetView if you want a different default view
 *
 * Defaults:
 *   - A GenericCamera with input handling on LMB/RMB/WASD/... (setUseDefaultXYZ to configure)
 *
 * Usage:
 * int main(int argc, char *argv[])
 * {
 *   MyGlfwApp app;
 *   return app.run(argc, argv); // automatically sets up GLOW and GLFW and everything
 * }
 */
class GlfwApp : virtual public pipeline::RenderCallback
{
public:
    enum class Gui
    {
        None,
        AntTweakBar,
        ImGui
    };

    GlfwApp(Gui gui = Gui::None) : mGui(gui) {}

private:
    std::string mTitle = "GLFW/GLOW Application"; ///< window title

    double mUpdateRate = 60;             ///< rate at which update(...) is called
    int mMaxFrameSkip = 4;               ///< maximum number of update(...) steps that are performed without rendering
    double mScheduledUpdateInterval = 0; ///< number of seconds between calls to scheduledUpdate(...). 0 means never

    GLFWwindow* mWindow = nullptr; ///< current GLFW window
    input::InputState mInputState; ///< Input state

    int mWindowWidth = 1280; ///< window width, only set before run!
    int mWindowHeight = 720; ///< window height, only set before run!

    bool mDumpTimingsOnShutdown = true; ///< if true, dumps AION timings on shutdown

    CursorMode mCursorMode = CursorMode::Normal; ///< cursor mode

    bool mVSync = true;    ///< if true, enables vsync
    int mSwapInterval = 1; ///< divisor of monitor frequency

    double mOutputStatsInterval = 5.0; ///< number of seconds between stats output (0.0 for never)
    bool mPrimitiveQueryStats = false; ///< if true, queries stats (vertices, fragments, ...)
    bool mWarnOnFrameskip = true;      ///< if true, outputs a warning on frameskips

    bool mUseDefaultCamera = true;              ///< if true, uses default camera
    bool mUseDefaultCameraHandling = true;      ///< if true, implements default cam handling
    bool mUseDefaultCameraHandlingLeft = true;  ///< if true, activates left mouse button handling
    bool mUseDefaultCameraHandlingRight = true; ///< if true, activates right mouse button handling

    bool mUsePipeline = false;          ///< if true, uses rendering pipeline (requires default cam)
    bool mUsePipelineConfigGui = false; ///< if true, enables the pipeline scen configuration gui (only if pipeline is used, requires ImGui)

    bool mCacheWindowSize = false; ///< if true, saves window size and position on close, and restores on next launch
    std::string mCacheWindowSizeFilename = "glfwapp.ini";       /// Default -> cwd
    std::string mCacheWindowSizeFilenameImGui = "imgui.ini";    /// Default -> cwd

    double mCurrentTime = 0.0;            ///< current frame time (starts with 0)
    double mCurrentRenderDeltaTime = 0.0; ///< current delta time for the render(dt) call

    float mLastGpuTimeMs = 0.f; ///< The last measured GPU time, in ms
    float mLastCpuTimeMs = 0.f; ///< The last measured CPU (render dispatch) time, in ms

    double mDoubleClickTime = 0.35; ///< max number of seconds for multi clicks
    int mClickCount = 0;            ///< current click count
    int mClickButton = -1;          ///< last clicked button
    tg::pos2 mClickPos;             ///< last clicked position
    timing::CpuTimer mClickTimer;   ///< click timing
    double mMouseMoveDistance = 0;  ///< distance the curser was moved since the last click

    bool mMinimized = false; ///< is true while the application is minimized

    bool mStartInvisible = false;

    GLFWwindow* mSecondaryContext = nullptr; ///< shared OpenGL context for multithreaded loading / OpenGL object creation
    bool mCreateSecondaryContext = false;    ///< if true, creates a secondary, shared OpenGL context

    bool mIsCleaned = false;

    // extra features
private:
    Gui mGui = Gui::None;
    bool mDrawGui = true;
    bool mGuiAfterRender = false;

    // Default graphics
private:
    /// Default Camera
    camera::SharedSmoothedCamera mCamera;

    /// Default Camera handling controllers
    camera::WASDController mWASDController;
    camera::LookAroundController mLookAroundController;
    camera::TargetOrbitController mTargetOrbitController;

    /// Default pipeline
    pipeline::SharedRenderPipeline mPipeline;
    pipeline::SharedRenderScene mPipelineScene;
    pipeline::SharedStageCamera mPipelineCamera;

    SharedPrimitiveQuery mPrimitiveQuery; ///< nr of primitives per frame
    SharedOcclusionQuery mOcclusionQuery; ///< nr of pixels per frame
    timing::SharedGpuTimer mGpuTimer;

    debugging::SharedProfilingOverlay mProfilingOverlay;
    bool mProfilingOverlayVisible = false;

public:
    GLOW_PROPERTY(StartInvisible);
    GLOW_PROPERTY(UpdateRate);
    GLOW_PROPERTY(MaxFrameSkip);
    GLOW_PROPERTY(ScheduledUpdateInterval);
    GLOW_GETTER(Title);
    GLOW_PROPERTY(WindowWidth);
    GLOW_PROPERTY(WindowHeight);
    tg::isize2 getWindowSize() const { return {mWindowWidth, mWindowHeight}; }
    GLOW_PROPERTY(DumpTimingsOnShutdown);
    GLOW_PROPERTY(CursorMode);
    GLOW_PROPERTY(VSync);
    GLOW_PROPERTY(SwapInterval);
    GLOW_PROPERTY(OutputStatsInterval);
    GLOW_PROPERTY(PrimitiveQueryStats);
    GLOW_PROPERTY(WarnOnFrameskip);
    GLOW_PROPERTY(CreateSecondaryContext);
    GLOW_PROPERTY(DoubleClickTime);

    float getCurrentTime() const { return float(mCurrentTime); }
    double getCurrentTimeD() const { return mCurrentTime; }

    float getCurrentDeltaTime() const { return float(mCurrentRenderDeltaTime); }
    double getCurrentDeltaTimeD() const { return mCurrentRenderDeltaTime; }

    float getLastGpuTimeMs() const { return mLastGpuTimeMs; }
    float getLastCpuTimeMs() const { return mLastCpuTimeMs; }

    GLOW_GETTER(Camera);

    GLOW_PROPERTY(UsePipeline);
    GLOW_PROPERTY(UsePipelineConfigGui);
    GLOW_GETTER(Pipeline);
    GLOW_GETTER(PipelineScene);
    GLOW_GETTER(PipelineCamera);

    GLOW_PROPERTY(Gui);
    GLOW_PROPERTY(DrawGui);

    GLOW_PROPERTY(UseDefaultCamera);
    GLOW_PROPERTY(UseDefaultCameraHandling);
    GLOW_PROPERTY(UseDefaultCameraHandlingLeft);
    GLOW_PROPERTY(UseDefaultCameraHandlingRight);

    GLOW_PROPERTY(CacheWindowSize);
    GLOW_PROPERTY(CacheWindowSizeFilename);
    GLOW_PROPERTY(CacheWindowSizeFilenameImGui);

    void setTitle(std::string const& title);

    GLFWwindow* window() const { return mWindow; }
    GLFWwindow* secondaryContext() const { return mSecondaryContext; }
    input::InputState const& input() const { return mInputState; }

    camera::WASDController& getWASDController() { return mWASDController; }
    camera::LookAroundController& getLookAroundController() { return mLookAroundController; }
    camera::TargetOrbitController& getTargetOrbitController() { return mTargetOrbitController; }

public:
    /// sets the current clipboard content
    void setClipboardString(std::string const& s) const;
    /// gets the current clipboard content
    std::string getClipboardString() const;

    // NOTE: the naming has changed, pressed means "down this frame, but not down last frame"
    // the deprecated isXPressed forwardings here just remain for backwards compatibility

    [[deprecated("use isMouseButtonDown instead")]] bool isMouseButtonPressed(int button) const { return mInputState.isMouseButtonDown(button); }
    bool isMouseButtonDown(int button) const { return mInputState.isMouseButtonDown(button); }

    [[deprecated("use isKeyDown instead")]] bool isKeyPressed(int key) const { return mInputState.isKeyDown(key); }
    bool isKeyDown(int key) const { return mInputState.isKeyDown(key); }

    tg::pos2 getMousePosition() const { return tg::pos2(mInputState.getMousePosition()); }

    /// check if the current click is an proper click, i.e. opposed to a mouse dragging
    bool isRealMouseClick() const { return mMouseMoveDistance < 5 && mClickTimer.elapsedMilliseconds() < 350; }

    /// Returns true iff the app should be closed
    /// Defaults to glfw window closed
    bool shouldClose() const;

    /// Returns true iff the app is in fullscren mode
    bool isFullscreen() const;
    /// Returns true iff the app is minimized
    bool isMinimized() const;

    /// Requests glfw to close the window
    void requestClose();

    // Tweakbar helper
#ifdef GLOW_EXTRAS_HAS_ANTTWEAKBAR
private:
    TwBar* mTweakbar = nullptr; ///< main tweakbar window

public:
    TwBar* tweakbar() const
    {
        if (mGui != Gui::AntTweakBar)
        {
            glow::error() << "AntTweakBar is not active. Did you forget to call GlfwApp(Gui::AntTweakBar) in the ctor?";
            TG_ASSERT(0 && "AntTweakBar not active");
            return nullptr;
        }
        return mTweakbar;
    }

    /// create read-write tweakbar entries
    void tweak(int& value, std::string const& name, std::string const& options = "");
    void tweak(bool& value, std::string const& name, std::string const& options = "");
    void tweak(float& value, std::string const& name, std::string const& options = "");
    void tweak(double& value, std::string const& name, std::string const& options = "");
    void tweak(glm::quat& value, std::string const& name, std::string const& options = "");
    void tweak(std::string& value, std::string const& name, std::string const& options = "");

    void tweak_dir(glm::vec3& value, std::string const& name, std::string const& options = "");
    void tweak_color(glm::vec3& value, std::string const& name, std::string const& options = "");
    void tweak_color(glm::vec4& value, std::string const& name, std::string const& options = "");
    void tweak_color(uint32_t& value, std::string const& name, std::string const& options = "");

    // NOTE: function will currently be leaked!
    void tweak_button(std::string const& name, std::function<void()> const& fun, std::string const& options = "");
#endif

#ifdef GLOW_EXTRAS_HAS_IMGUI // imgui support
private:
    bool mEnableDebugOverlay = true; ///< if true, enables debugging::DebugOverlay (requires ImGui)

public:
    GLOW_PROPERTY(EnableDebugOverlay);
#endif

    /// if called, performs onGui after render()
    void performGuiAfterRender() { mGuiAfterRender = true; }
    /// if called, performs onGui before render()
    void performGuiBeforeRender() { mGuiAfterRender = false; }

protected:
    /// Called once GLOW is initialized. Allocated your resources and init your logic here.
    virtual void init();
    /// Called with at 1 / getUpdateRate() Hz (timestep)
    virtual void update(float elapsedSeconds);
    /// Called as fast as possible for rendering (elapsedSeconds is not fixed here)
    virtual void render(float elapsedSeconds);
    /// Called once every getScheduledUpdateInterval() seconds (0 means never), for tasks that only have to happen rarely
    virtual void scheduledUpdate();
    /// Called once in the beginning after (init) and whenever the window size changed
    virtual void onResize(int w, int h);
    /// Called at the end, when application is closed
    virtual void onClose();
    /// Called when the gui is handled (currently only for imgui)
    virtual void onGui();

    /// Called whenever a key is pressed
    virtual bool onKey(int key, int scancode, int action, int mods);
    /// Called whenever a character is entered (unicode)
    virtual bool onChar(unsigned int codepoint, int mods);
    /// Called whenever the mouse position changes
    virtual bool onMousePosition(double x, double y);
    /// Called whenever a mouse button is pressed (clickCount is 1 for single clicks, 2 for double, 3+ for multi)
    virtual bool onMouseButton(double x, double y, int button, int action, int mods, int clickCount);
    /// Called whenever the mouse is scrolled
    virtual bool onMouseScroll(double sx, double sy);
    /// Called whenever the mouse enters the window
    virtual bool onMouseEnter();
    /// Called whenever the mouse leaves the window
    virtual bool onMouseExit();
    /// Called whenever the window gets focus
    virtual bool onFocusGain();
    /// Called whenever the window loses focus
    virtual bool onFocusLost();
    /// Called whenever files are dropped (drag'n'drop), parameters is file paths
    virtual bool onFileDrop(std::vector<std::string> const& files);

    /// Called when view should be reset
    virtual void onResetView();

    /// Blocking call that executes the complete main loop
    virtual void mainLoop();

private:
    glfw::GlfwContext* mInternalContext = nullptr;
    bool mInternalContextOwner = false;

private:
    void internalInit();
    void internalCleanUp();

    void internalOnMouseButton(double x, double y, int button, int action, int mods);
    void internalOnGui();
    void internalPerformGui();

protected:
    /// performs glfw polling
    void updateInput();

    /// should be called before rendering
    void beginRender();
    /// should be called after rendering
    /// calls swapBuffers
    void endRender();

    /// Blocks the thread for a given number of seconds
    void sleepSeconds(double seconds) const;

public:
    /// Initializes GLFW and GLOW, and runs until window is closed
    void run();

    /// Initializes GLFW and GLOW but does not create a window
    void startHeadless();

    /// Toggle fullscreen mode
    void toggleFullscreen();

    /// Toggle profiling overlay
    void toggleProfilingOverlay();

#ifdef GLOW_EXTRAS_HAS_IMGUI // imgui support
    /// Toggle OpenGL debug overlay
    void toggleDebugOverlay();
#endif

public:
    virtual ~GlfwApp(); // virtual dtor
};

namespace detail
{
GlfwApp* current_app();
}
}
}
