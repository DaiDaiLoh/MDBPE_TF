#include "ViewerApp.hh"

#include <iomanip>
#include <sstream>

#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_stdlib.h>
#include <imgui/imguizmo.h>

#include <glow-extras/debugging/imgui-util.hh>
#include <glow/common/scoped_gl.hh>

#include "view.hh"

using namespace glow;
using namespace glow::viewer;

namespace
{
thread_local tg::mat4 sCurrentViewMatrix;
thread_local tg::mat4 sCurrentProjMatrix;
thread_local bool sIsViewerActive = false;
thread_local glow::viewer::close_info sLastCloseInfo;
thread_local bool sPreserveCamera = false;
thread_local glow::viewer::SharedCameraController sPreservedCamera = nullptr;

std::string toCppString(float v)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6);
    if (v == 0.f || !tg::is_finite(v))
        ss << "0.f";
    else
        ss << v << "f";
    return ss.str();
}
std::string toCppString(tg::pos3 const& v)
{
    std::stringstream ss;
    ss << "tg::pos3(" << toCppString(v.x) << ", " << toCppString(v.y) << ", " << toCppString(v.z) << ")";
    return ss.str();
}
std::string toCppString(tg::angle32 v)
{
    std::stringstream ss;
    ss << "tg::degree(" << toCppString(v.degree()) << ")";
    return ss.str();
}

void set_close_info(int key, CameraController const& cam)
{
    sLastCloseInfo.closed_by_key = key;
    sLastCloseInfo.cam_pos = cam.getPosition();
    sLastCloseInfo.cam_target = cam.getTargetPos();
}
}

glow::viewer::close_info glow::viewer::get_last_close_info() { return sLastCloseInfo; }

tg::mat4 const& glow::viewer::current_view_matrix()
{
    TG_ASSERT(sIsViewerActive && "viewer not currently active");
    return sCurrentViewMatrix;
}
tg::mat4 const& glow::viewer::current_proj_matrix()
{
    TG_ASSERT(sIsViewerActive && "viewer not currently active");
    return sCurrentProjMatrix;
}

SubViewData& ViewerApp::getOrCreateSubViewData(tg::ipos2 start, tg::isize2 size)
{
    auto rect = tg::iaabb2(start, start + tg::vec(size));
    auto it = mSubViewDataCache.find(rect);
    if (TG_LIKELY(it != mSubViewDataCache.end()))
        return (*it).second;
    else
    {
        auto it2 = mSubViewDataCache.emplace(std::piecewise_construct, std::forward_as_tuple(rect), std::forward_as_tuple(size.width, size.height, &mRenderer));
        return it2.first->second;
    }
}

ViewerApp::ViewerApp(glow::viewer::detail::shared_command_queue queue, glow::viewer::detail::global_settings const& settings)
  : GlfwApp(Gui::ImGui), mStaticCommandQueue(std::move(queue)), mIsInteractive(detail::is_interactive(*mStaticCommandQueue)), mSettings(settings)
{
    mLayoutNodes.reserve(8);
}

cc::optional<tg::pos3> ViewerApp::query3DPosition(int x, int y)
{
    auto px = querySubViewPixel(x, y);

    if (px.is_valid())
        return mRenderer.query3DPosition(px.size, px.pos, *px.data, *px.cam);
    else
        return {};
}

cc::optional<float> ViewerApp::queryDepth(int x, int y)
{
    auto px = querySubViewPixel(x, y);

    if (px.is_valid())
        return mRenderer.queryDepth(px.size, px.pos, *px.data);
    else
        return {};
}

void ViewerApp::renderScreenshotNextFrame(std::string const& filename, int w, int h, int accumulationCount, GLenum format)
{
    mNewScreenshots.push_back({filename, w, h, accumulationCount, format});
}

void glow::viewer::ViewerApp::handlePicking(int x, int y)
{
    auto px = querySubViewPixel(x, y);

    if (!px.is_valid())
        return;

    mRenderer.handlePicking(px.size, px.pos, *px.data, *px.cam, mMouseButtonLeftPicking, mMouseButtonRightPicking);
}

void ViewerApp::resetCameraToSceneNextFrame(bool clipCam)
{
    mResetCameraNextFrame = true;
    mClipCameraNextFrame = clipCam;
}

ViewerApp::SubViewPixel ViewerApp::querySubViewPixel(int x, int y)
{
    // x and y are screen coordinates
    // see https://www.glfw.org/docs/3.1/window.html#window_size
    // the layout nodes deal in framebuffer coordinates
    // we now have to transform them to framebuffer space
    int window_width;
    int window_height;
    glfwGetWindowSize(window(), &window_width, &window_height);
    int fb_width;
    int fb_height;
    glfwGetFramebufferSize(window(), &fb_width, &fb_height);
    int x_fb = int(double(fb_width) * double(x) / double(window_width));
    int y_fb = int(double(fb_height) * double(y) / double(window_height));

    for (auto& node : mLayoutNodes)
    {
        auto start = node.start;
        auto const& size = node.size;
        auto cam = node.scene.config.customCameraController ? node.scene.config.customCameraController : mCamera;

        if (x_fb >= start.x && x_fb <= start.x + size.width && y_fb >= start.y && y_fb <= start.y + size.height)
        {
            // this subview was hit
            auto const corrected_pixel = tg::ipos2{x_fb, y_fb} - tg::ivec2(start);
            start.y = fb_height - start.y - size.height - 1; // needed for subview data
            return {corrected_pixel, size, &getOrCreateSubViewData(start, size), std::move(cam)};
        }
    }

    return {};
}

void ViewerApp::init()
{
    setDumpTimingsOnShutdown(false);
    setUsePipeline(false);
    setUsePipelineConfigGui(false);
    setUseDefaultCamera(false);
    setUseDefaultCameraHandling(false);
    setCacheWindowSize(true);
    setWarnOnFrameskip(false);
    setTitle("Viewer");

    if (mSettings.headlessScreenshot)
        setStartInvisible(true);

    // set where to read and write the window position
    // (empty location -> don't read or write)
    if (mSettings.cacheWindowSizeFolder.empty())
    {
        setCacheWindowSize(false);
    }
    else
    {
        setCacheWindowSize(true);
        setCacheWindowSizeFilename(mSettings.cacheWindowSizeFolder + "glfwapp.ini");
        setCacheWindowSizeFilenameImGui(mSettings.cacheWindowSizeFolder + "imgui.ini");
    }

    sLastCloseInfo.closed_by_key = -1;

    GlfwApp::init();

    // set where to read and write the window position; part 2
    // (ImGui Context has to be initialized already)
    if (getGui() == Gui::ImGui)
    {
        if (!getCacheWindowSize())
            ImGui::GetIO().IniFilename = nullptr;
        else
        {
            ImGui::GetIO().IniFilename = getCacheWindowSizeFilenameImGui().c_str();
        }
    }

    // create camera (before initial layout)
    // (or take preserved camera)
    bool newCamera;
    if (sPreserveCamera && sPreservedCamera)
    {
        mCamera = sPreservedCamera;
        newCamera = false;
    }
    else
    {
        mCamera = CameraController::create();
        newCamera = true;
    }
    sPreserveCamera = false;

    // Initial layout
    forceInteractiveExecution(0.f, getWindowSize());

    // init cam
    if (newCamera)
    {
        resetCameraToScene();
    }

    // see if camera is reused
    for (auto const& n : mLayoutNodes)
        if (n.scene.config.reuseCamera)
        {
            if (!sPreservedCamera)
                glow::error() << "[glow-viewer] gv::reuse_camera was used without a previous viewer open (did you mean to use gv::preserve_camera?)";
            else
                mCamera = sPreservedCamera; // do NOT reset cam to scene
        }
    // see if camera should be preserved
    for (auto const& n : mLayoutNodes)
        if (n.scene.config.preserveCamera)
            sPreserveCamera = true; // force the nex viewer to reuse this camera

    sPreservedCamera = mCamera; // if the next viewer uses gv::reuse_camera

    debugging::applyGlowImguiTheme(mSettings.imguiDarkMode);

    // Dummy render
    render(0.f);
    mSubViewDataCache.clear();

    if (mSettings.headlessScreenshot)
    {
        renderScreenshot(mSettings.screenshotFilename, mSettings.screenshotResolution.x, mSettings.screenshotResolution.y,
                         mSettings.screenshotAccumulationCount, mSettings.screenshotFormat);
        requestClose();
    }
}

void ViewerApp::update(float)
{
    // empty for now
}


void ViewerApp::updateCamera(float dt)
{
    float dRight = 0;
    float dUp = 0;
    float dFwd = 0;
    float speed = mKeyShift ? 5 : 1;
    speed *= mKeyCtrl ? 0.2f : 1;

    dRight += int(mKeyD) * speed;
    dRight -= int(mKeyA) * speed;
    dFwd += int(mKeyW) * speed;
    dFwd -= int(mKeyS) * speed;
    dUp += int(mKeyE) * speed;
    dUp -= int(mKeyQ) * speed;

    auto& cam = cameraAtPosition(tg::ipos2(mLastX, mLastY));
    cam.moveCamera(dRight, dFwd, dUp, 0.f, dt);

    std::unordered_set<CameraController*> updated;
    for (auto& node : mLayoutNodes)
    {
        if (node.scene.config.customCameraController && updated.count(node.scene.config.customCameraController.get()) == 0)
        {
            node.scene.config.customCameraController->update(dt);
            updated.insert(node.scene.config.customCameraController.get());
        }
    }

    mCamera->update(dt);
    setCursorMode(mMouseRight && mSettings.rightMouseControlEnabled ? glow::glfw::CursorMode::Disabled : glow::glfw::CursorMode::Normal);
}

void ViewerApp::updateLayoutNodes(float dt, tg::isize2 size)
{
    ImGuizmo::SetRect(0, 0, size.width, size.height);
    mRootNode = layout::tree_node{};

    // for now: only works correctly with one view
    mCamera->setReverseZEnabled(mRenderer.isReverseZEnabled());
    sIsViewerActive = true;
    sCurrentViewMatrix = mCamera->computeViewMatrix();
    sCurrentProjMatrix = mCamera->computeProjMatrix();

    // calculate tree, always execute interactives
    detail::create_layout_tree(mRootNode, *mStaticCommandQueue, dt, true);
    sIsViewerActive = false;

    // calculate linear nodes
    recomputeLayout(size);
}

void ViewerApp::recomputeLayout(tg::isize2 size)
{
    mLayoutNodes.clear();
    layout::build_linear_nodes(mRootNode, {0, -1}, size, mLayoutNodes, mSettings.subviewMargin);
}

void ViewerApp::forceInteractiveExecution(float dt, tg::isize2 size)
{
    // Imgui state has to be alive so the interactive lambda doesn't crash
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    updateLayoutNodes(dt, size);
    ImGui::EndFrame();
}

void ViewerApp::renderFrame(int width, int height, bool maximizeSamples)
{
    if (mResetCameraNextFrame)
    {
        resetCameraToScene(mClipCameraNextFrame);
        mResetCameraNextFrame = false;
    }

    (void)width;
    if (mSettings.use_2d_controls())
        mRenderer.set_2d_transform(mTransform_2d);

#ifdef GLOW_EXTRAS_OPENGL_DEBUG_GROUPS
    GLOW_SCOPED(debugGroup, "Render Viewer");
#endif
    mRenderer.beginFrame(mSettings.subviewMarginColor);

    if (maximizeSamples)
        mRenderer.maximizeSamples();

    sIsViewerActive = true;
    for (auto& node : mLayoutNodes)
    {
#ifdef GLOW_EXTRAS_OPENGL_DEBUG_GROUPS
        GLOW_SCOPED(debugGroup, "SubView Render");
#endif

        auto start = node.start;
        auto const& size = node.size;
        start.y = height - start.y - size.height - 1;

        // render
        {
            GLOW_SCOPED(viewport, start, size);

            // debug settings
            node.scene.enableScreenshotDebug = mShowNonTransparentPixels;

            CameraController& cam = node.scene.config.customCameraController ? *node.scene.config.customCameraController : *mCamera;
            cam.setReverseZEnabled(mRenderer.isReverseZEnabled());

            // metadata
            node.scene.viewMatrix = cam.computeViewMatrix();
            node.scene.projMatrix = cam.computeProjMatrix();
            sCurrentViewMatrix = node.scene.viewMatrix;
            sCurrentProjMatrix = node.scene.projMatrix;

            auto& subview = getOrCreateSubViewData(start, size);
            subview.mElapsedSeconds = elapsedSeconds;
            mRenderer.renderSubview(size, start, subview, node.scene, cam);
        }
    }

    // mark nodes finished
    for (auto& node : mLayoutNodes)
        for (auto const& r : node.scene.getRenderables())
            r->onRenderFinished();

    mRenderer.endFrame(maximizeSamples ? 0.f : getLastGpuTimeMs());
    sIsViewerActive = false;
}

void ViewerApp::resetCameraToScene(bool clipCam)
{
    bool has_3d_geometry = false;
    for (auto const& node : mLayoutNodes)
    {
        auto const& bounding = node.scene.getBoundingInfo();
        if (node.scene.has_3d_renderables())
            has_3d_geometry = true;

        auto& camera = node.scene.config.customCameraController ? *node.scene.config.customCameraController : *mCamera;

        if (node.scene.config.cameraFov.has_value())
            camera.s.HorizontalFov = node.scene.config.cameraFov.value();

        camera.setupMesh(bounding.diagonal, bounding.center);

        if (node.scene.config.customCameraOrientation)
            camera.setOrbit(node.scene.config.cameraAzimuth, node.scene.config.cameraAltitude, node.scene.config.cameraDistance);
        if (node.scene.config.customCameraPosition)
            camera.setTransform(node.scene.config.cameraPosition, node.scene.config.cameraTarget);
        if (node.scene.config.enableOrthogonalProjection)
            camera.enableOrthographicMode(node.scene.config.orthogonalProjectionBounds);
    }

    detail::set_has_3d_content(has_3d_geometry);

    if (clipCam)
    {
        mCamera->clipCamera();
        for (auto const& node : mLayoutNodes)
            if (node.scene.config.customCameraController)
                node.scene.config.customCameraController->clipCamera();
    }
}

CameraController& ViewerApp::cameraAtPosition(tg::ipos2 mouse_pos)
{
    for (auto& node : mLayoutNodes)
    {
        auto const bb = tg::iaabb2(node.start, node.size);
        if (node.scene.config.customCameraController && contains(bb, mouse_pos))
            return *node.scene.config.customCameraController;
    }
    return *mCamera;
}

void ViewerApp::render(float dt)
{
    updateCamera(dt);
    renderFrame(getWindowWidth(), getWindowHeight());
    handlePicking(mLastX, mLastY);
    mMouseButtonLeftPicking = false;
    mMouseButtonRightPicking = false;

    for (auto const& s : mNewScreenshots)
        renderScreenshot(s.file, s.w, s.h, s.samples, s.format);
    mNewScreenshots.clear();

    elapsedSeconds += dt;
}

bool ViewerApp::onMousePosition(double x, double y)
{
    if (GlfwApp::onMousePosition(x, y))
        return true;

    auto dx = x - mLastX;
    auto dy = y - mLastY;

    if (!isRealMouseClick())
    {
        if (mSettings.use_2d_controls() && mMouseMiddle)
        {
            mTransform_2d[2][0] += float(dx);
            mTransform_2d[2][1] += float(dy);
        }
        else
        {
            auto& cam = cameraAtPosition(tg::ipos2(mLastX, mLastY));

            auto minS = double(tg::min(getWindowWidth(), getWindowHeight())); // correct?
            auto relDx = dx / minS;
            auto relDy = dy / minS;

            if (mMouseRight && mSettings.rightMouseControlEnabled)
                cam.fpsStyleLookaround(float(relDx), float(relDy));
            else if (mMouseLeft && mSettings.leftMouseControlEnabled)
                cam.targetLookaround(float(relDx), float(relDy));
        }
    }

    mLastX = x;
    mLastY = y;

    return false;
}

bool ViewerApp::onMouseButton(double x, double y, int button, int action, int mods, int clickCount)
{
    if (GlfwApp::onMouseButton(x, y, button, action, mods, clickCount))
        return true;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && clickCount >= 2 && mods == 0)
    {
        if (auto p = query3DPosition(int(x), int(y)); p.has_value())
            cameraAtPosition({int(x), int(y)}).focusOnSelectedPoint(p.value());
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS && clickCount >= 2 && mods == 0)
        resetCameraToScene(false);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mMouseLeft = true;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        if (isRealMouseClick())
        {
            mMouseButtonLeftPicking = true;
        }
        mMouseLeft = false;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        mMouseRight = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        if (isRealMouseClick())
        {
            mMouseButtonRightPicking = true;
        }
        mMouseRight = false;
    }

    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
        mMouseMiddle = true;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
        mMouseMiddle = false;

    return false;
}

bool ViewerApp::onKey(int key, int scancode, int action, int mods)
{
    if (GlfwApp::onKey(key, scancode, action, mods))
        return true;

    // Close
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            if (input().isKeyDown(GLFW_KEY_LEFT_SHIFT))
            {
                // Terminate on Shift + ESC
                throw TerminateException();
            }
            else
            {
                // Close on ESC
                set_close_info(key, *mCamera);
                requestClose();
            }
            return true;
        }

        if (key == GLFW_KEY_F2)
            renderScreenshot(mScreenshotFile);

        if (key == GLFW_KEY_F3)
            resetCameraToScene();

        // additional close keys
        for (auto const& node : mLayoutNodes)
            for (auto k : node.scene.config.closeKeys)
                if (key == k)
                {
                    set_close_info(key, *mCamera);
                    requestClose();
                    return true;
                }
    }

    { // camera
        auto& camera = cameraAtPosition(tg::ipos2(mLastX, mLastY));

        // Numpad+- - camera speed
        if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS)
        {
            camera.changeCameraSpeed(1);
            return true;
        }
        if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS)
        {
            camera.changeCameraSpeed(-1);
            return true;
        }

        // Numpad views:
        if (key == GLFW_KEY_KP_8 && action == GLFW_PRESS)
        {
            camera.rotate(0, 1);
            return true;
        }
        if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS)
        {
            camera.rotate(0, -1);
            return true;
        }
        if (key == GLFW_KEY_KP_6 && action == GLFW_PRESS)
        {
            camera.rotate(1, 0);
            return true;
        }
        if (key == GLFW_KEY_KP_4 && action == GLFW_PRESS)
        {
            camera.rotate(-1, 0);
            return true;
        }
        if (key == GLFW_KEY_KP_0 && action == GLFW_PRESS)
        {
            camera.resetView();
            return true;
        }
        if (key == GLFW_KEY_KP_5 && action == GLFW_PRESS)
        {
            camera.setOrbit({1, 0, 0}, {0, 1, 0});
            return true;
        }
        if (key == GLFW_KEY_KP_9 && action == GLFW_PRESS)
        {
            camera.setOrbit({0, 1, 0}, {1, 0, 0});
            return true;
        }
        if (key == GLFW_KEY_KP_3 && action == GLFW_PRESS)
        {
            camera.setOrbit({0, -1, 0}, {-1, 0, 0});
            return true;
        }

        // movement
        if (key == GLFW_KEY_A && action == GLFW_PRESS)
            mKeyA = true;
        if (key == GLFW_KEY_A && action == GLFW_RELEASE)
            mKeyA = false;

        if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
            mKeyShift = true;
        if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
            mKeyShift = false;

        if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS)
            mKeyCtrl = true;
        if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE)
            mKeyCtrl = false;

        if (key == GLFW_KEY_D && action == GLFW_PRESS)
            mKeyD = true;
        if (key == GLFW_KEY_D && action == GLFW_RELEASE)
            mKeyD = false;

        if (key == GLFW_KEY_S && action == GLFW_PRESS)
            mKeyS = true;
        if (key == GLFW_KEY_S && action == GLFW_RELEASE)
            mKeyS = false;

        if (key == GLFW_KEY_W && action == GLFW_PRESS)
            mKeyW = true;
        if (key == GLFW_KEY_W && action == GLFW_RELEASE)
            mKeyW = false;

        if (key == GLFW_KEY_Q && action == GLFW_PRESS)
            mKeyQ = true;
        if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
            mKeyQ = false;

        if (key == GLFW_KEY_E && action == GLFW_PRESS)
            mKeyE = true;
        if (key == GLFW_KEY_E && action == GLFW_RELEASE)
            mKeyE = false;
    }

    return false;
}

bool ViewerApp::onMouseScroll(double sx, double sy)
{
    if (GlfwApp::onMouseScroll(sx, sy))
        return true;

    if (mSettings.use_2d_controls())
    {
        auto const speed = mKeyCtrl ? 0.2f : 0.1f;
        auto scale = (sy < 0) ? 1 - speed : 1 + speed;

        mTransform_2d[0][0] *= scale;
        mTransform_2d[1][1] *= scale;
        mTransform_2d[2][0] = scale * (mTransform_2d[2][0] - float(mLastX)) + float(mLastX);
        mTransform_2d[2][1] = scale * (mTransform_2d[2][1] - float(mLastY)) + float(mLastY);
    }
    else
    {
        auto& camera = cameraAtPosition(tg::ipos2(mLastX, mLastY));
        camera.zoom(float(sy));
    }

    return false;
}

bool ViewerApp::onFileDrop(const std::vector<std::string>& files)
{
    if (mSettings.onDropFiles)
    {
        mSettings.onDropFiles(files);
        return true;
    }
    return false;
}

void ViewerApp::onResize(int w, int h)
{
    mSubViewDataCache.clear();
    if (!mIsInteractive)
    {
        // Non-interactive viewers dont rebuild the layout each frame, do it now
        updateLayoutNodes(0.f, {w, h});
        sIsViewerActive = false;
    }
}

void ViewerApp::onGui()
{
    // this gets called each frame, before ::render

    // if interactive, perform layouting now so the interactive lambda can use imgui
    if (mIsInteractive)
        updateLayoutNodes(getCurrentDeltaTime(), getWindowSize());

    // perform viewer-global imgui
    if (mSettings.enableViewerUI && ImGui::BeginMainMenuBar())
    {
        ImGui::PushID("glow_viewer_menu");

        ImGui::MenuItem("Viewer", nullptr, false, false);

        if (ImGui::BeginMenu("Actions"))
        {
            if (ImGui::MenuItem("Take Screenshot", "F2"))
                renderScreenshot(mScreenshotFile);

            if (ImGui::MenuItem("Reset Camera", "F3"))
                resetCameraToScene();

            if (ImGui::MenuItem("Quit", "Esc"))
                requestClose();

            if (ImGui::MenuItem("Quit All", "Shift + Esc"))
                throw TerminateException();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Screenshot Tool", nullptr, &mShowScreenshotTool);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Info"))
        {
            auto const camOrientation = mCamera->getSphericalCoordinates();
            ImGui::Text("Cam Orientation: Azimuth: %.3f, Altitude: %.3f", camOrientation.comp0.degree(), camOrientation.comp1.degree());
            ImGui::Text("Cam Distance: %.3f", mCamera->getTargetDistance());
            auto const camPos = mCamera->getPosition();
            ImGui::Text("Cam Position: x: %.3f, y: %.3f, z: %.3f", camPos.x, camPos.y, camPos.z);
            auto const camTarget = mCamera->getTargetPos();
            ImGui::Text("Cam Target: x: %.3f, y: %.3f, z: %.3f", camTarget.x, camTarget.y, camTarget.z);

            if (ImGui::BeginMenu("Print cam configuration"))
            {
                if (ImGui::MenuItem("Orientation"))
                {
                    glow::info() << "Cam config:\nGLOW_VIEWER_CONFIG(glow::viewer::camera_orientation(" << toCppString(camOrientation.comp0) << ", "
                                 << toCppString(camOrientation.comp1) << ", " << toCppString(mCamera->getTargetDistance()) << "));";
                }

                if (ImGui::MenuItem("Transform (Full)"))
                {
                    glow::info() << "Cam config:\nGLOW_VIEWER_CONFIG(glow::viewer::camera_transform(" << toCppString(mCamera->getPosition()) << ", "
                                 << toCppString(mCamera->getTargetPos()) << "));";
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Overlays"))
        {
            if (ImGui::MenuItem("Profiling", "F9"))
                toggleProfilingOverlay();

            if (ImGui::MenuItem("OpenGL Log", "F10"))
                toggleDebugOverlay();


            ImGui::EndMenu();
        }

        ImGui::PopID();
        ImGui::EndMainMenuBar();
    }

    // screenshot tool
    if (mShowScreenshotTool)
    {
        if (ImGui::Begin("Screenshot Tool", &mShowScreenshotTool, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushID("screenshot_tool");

            ImGui::InputText("Output Path", &mScreenshotFile);

            if (ImGui::Button("Render Screenshot"))
            {
                auto format = mScreenshotWithAlpha ? GL_RGBA8 : GL_RGB8;
                if (mScreenshotCustomRes)
                    renderScreenshot(mScreenshotFile, mScreenshotWidth, mScreenshotHeight, mScreenshotAccumulationCount, format);
                else
                    renderScreenshot(mScreenshotFile, -1, -1, mScreenshotAccumulationCount, format);
            }

            ImGui::Separator();

            ImGui::Checkbox("Show non-transparent pixels", &mShowNonTransparentPixels);

            ImGui::SliderInt("Accumulation Count", &mScreenshotAccumulationCount, 1, 256);

            ImGui::Checkbox("Output with Alpha", &mScreenshotWithAlpha);

            ImGui::Checkbox("Custom Resolution", &mScreenshotCustomRes);
            if (ImGui::SliderInt("Screenshot Width", &mScreenshotWidth, 1, 16000))
                mScreenshotCustomRes = true;
            if (ImGui::SliderInt("Screenshot Height", &mScreenshotHeight, 1, 16000))
                mScreenshotCustomRes = true;

            ImGui::PopID();
        }
        ImGui::End();
    }

    // perform imgui for each renderable
    sIsViewerActive = true;
    for (auto const& node : mLayoutNodes)
    {
        ImGuizmo::SetRect(node.start.x, node.start.y, node.size.width, node.size.height);
        for (auto const& r : node.scene.getRenderables())
        {
            sCurrentViewMatrix = node.scene.viewMatrix;
            sCurrentProjMatrix = node.scene.projMatrix;
            r->onGui();
        }
    }
    ImGuizmo::SetRect(0, 0, getWindowWidth(), getWindowHeight());
    sIsViewerActive = false;
}

void ViewerApp::renderScreenshot(const std::string& filename, int w, int h, int accumulationCount, GLenum format)
{
    auto customRes = w > 0 && h > 0;
    auto screenW = customRes ? w : getWindowWidth();
    auto screenH = customRes ? h : getWindowHeight();

    auto target = glow::TextureRectangle::create(screenW, screenH, format);
    auto temporaryFbo = glow::Framebuffer::create("fOut", target);

    if (customRes)
    {
        onResize(screenW, screenH);
        recomputeLayout({screenW, screenH});
    }

    {
        auto fbo = temporaryFbo->bind();
        for (auto _ = 0; _ < accumulationCount; ++_)
            renderFrame(screenW, screenH, true);
    }

    target->bind().writeToFile(filename);
    glow::info() << "Saved screenshot as " << filename;

    if (customRes)
    {
        onResize(getWindowWidth(), getWindowHeight());
        recomputeLayout(getWindowSize());
    }
}
