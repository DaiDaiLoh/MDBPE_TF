#pragma once

#include <unordered_map>

#include "fwd.hh"

#include <typed-geometry/functions/std/hash.hh>

#include <glow-extras/glfw/GlfwApp.hh>

#include <glow-extras/vector/backend/opengl.hh>

#include "CameraController.hh"
#include "RenderInfo.hh"
#include "ViewerRenderer.hh"
#include "detail/command_queue.hh"
#include "layout.hh"

namespace glow
{
namespace viewer
{
class ViewerApp final : public glfw::GlfwApp
{
public:
    struct TerminateException : std::exception
    {
        const char* what() const noexcept { return "Global Viewer Temination"; }
    };
    // input
private:
    bool mMouseLeft = false;
    bool mMouseRight = false;
    bool mMouseMiddle = false;
    bool mKeyShift = false;
    bool mKeyCtrl = false;
    bool mKeyA = false;
    bool mKeyD = false;
    bool mKeyS = false;
    bool mKeyW = false;
    bool mKeyQ = false;
    bool mKeyE = false;
    double mLastX = 0;
    double mLastY = 0;
    // mMouseLeftButtonPressed and mMouseRightPressed true iff isRealMouseClick() && mMouseLeft (resp. mMouseRight) && Action == RELEASE
    bool mMouseButtonLeftPicking = false;
    bool mMouseButtonRightPicking = false;

    // UI
private:
    bool mShowScreenshotTool = false;
    bool mShowNonTransparentPixels = false;
    bool mScreenshotCustomRes = false;
    bool mScreenshotWithAlpha = false;
    int mScreenshotWidth = 3840;
    int mScreenshotHeight = 2160;
    std::string mScreenshotFile = "viewer_screen.png";
    int mScreenshotAccumulationCount = 1;
    bool mResetCameraNextFrame = false;
    bool mClipCameraNextFrame = false;


    struct screenshot_info
    {
        std::string file;
        int w = -1;
        int h = -1;
        int samples = 128;
        GLenum format = GL_RGB8;
    };
    std::vector<screenshot_info> mNewScreenshots;

    // gfx
private:
    ViewerRenderer mRenderer;
    SharedCameraController mCamera;

    /// The static part of the command queue, this does not change during application run
    detail::shared_command_queue const mStaticCommandQueue;
    /// Whether mStaticCommandQueue contains interactive parts. If false, the layout does not require a rebuild each frame
    bool const mIsInteractive;
    detail::global_settings const& mSettings;

    /// total elapsed time in sec
    double elapsedSeconds = 0.;

    /// A subview data cache, keys are the aabb on the screen
    std::unordered_map<tg::iaabb2, SubViewData> mSubViewDataCache;

    /// The hierarchical layout
    layout::tree_node mRootNode;

    /// The linear layout nodes
    std::vector<layout::linear_node> mLayoutNodes;

    /// Mouse movements should move 2d vector images
    tg::mat3x2 mTransform_2d = tg::mat3x2::identity;

    SubViewData& getOrCreateSubViewData(tg::ipos2 start, tg::isize2 size);

private:
    void updateCamera(float dt);
    void updateLayoutNodes(float dt, tg::isize2 size);
    void recomputeLayout(tg::isize2 size);
    void forceInteractiveExecution(float dt, tg::isize2 size);
    void renderFrame(int w, int h, bool maximizeSamples = false);
    CameraController& cameraAtPosition(tg::ipos2 mouse_pos);

public:
    ViewerApp(detail::shared_command_queue queue, detail::global_settings const& settings);

    // NOTE: xy in window coords, i.e. y is 0 at the top
    cc::optional<tg::pos3> query3DPosition(int x, int y);
    cc::optional<float> queryDepth(int x, int y);
    void renderScreenshotNextFrame(std::string const& filename, int w, int h, int accumulationCount, GLenum format);

    // picking functionality, NOTE: xy in window coords, i.e. y is 0 at the top
    void handlePicking(int x, int y);


    void resetCameraToSceneNextFrame(bool clipCam = true);
    void resetCameraToScene(bool clipCam = true);

private:
    struct SubViewPixel
    {
        tg::ipos2 pos;
        tg::isize2 size;
        SubViewData* data = nullptr;
        SharedCameraController cam;

        bool is_valid() const { return data != nullptr; }
    };
    SubViewPixel querySubViewPixel(int x, int y);

protected:
    void init() override;
    void update(float elapsedSeconds) override;
    void render(float elapsedSeconds) override;

    bool onMousePosition(double x, double y) override;
    bool onMouseButton(double x, double y, int button, int action, int mods, int clickCount) override;
    bool onKey(int key, int scancode, int action, int mods) override;
    bool onMouseScroll(double sx, double sy) override;

    bool onFileDrop(std::vector<std::string> const& files) override;

    void onResize(int w, int h) override;
    void onGui() override;
    void renderScreenshot(std::string const& filename, int w = -1, int h = -1, int accumulationCount = 1, GLenum format = GL_RGB8);
};
}
}
