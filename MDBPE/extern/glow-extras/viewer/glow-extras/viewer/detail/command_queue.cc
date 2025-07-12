#include "command_queue.hh"

#include <filesystem>
#include <variant>

#include <glow-extras/viewer/ViewerApp.hh>
#include <glow-extras/viewer/view.hh>
#include <glow/util/AsyncTextureLoader.hh>

namespace
{
std::vector<glow::viewer::detail::shared_command_queue> sCommandQueueStack;

std::vector<glow::viewer::detail::shared_command_queue> sUnusedCommandQueuePool;

glow::viewer::ViewerApp* sCurrentViewerApp = nullptr;

glow::viewer::detail::shared_command_queue createQueue()
{
    if (!sUnusedCommandQueuePool.empty())
    {
        // From pool
        auto res = sUnusedCommandQueuePool.back();
        sUnusedCommandQueuePool.pop_back();
        res->clear();
        return res;
    }
    else
    {
        // New
        auto res = std::make_shared<glow::viewer::detail::command_queue>();
        res->reserve(25u);
        return res;
    }
}

void poolQueue(glow::viewer::detail::shared_command_queue&& queue)
{
    queue->clear();
    sUnusedCommandQueuePool.push_back(std::move(queue));
}

glow::viewer::detail::global_settings sGlobalSettings{};

} // anon namespace


void glow::viewer::close_viewer()
{
    TG_ASSERT(sCurrentViewerApp && "viewer not currently active");
    sCurrentViewerApp->requestClose();
}

void glow::viewer::detail::command_queue_lazy_init()
{
    if (TG_UNLIKELY(sCommandQueueStack.empty()))
        sCommandQueueStack.push_back(createQueue());
}


void glow::viewer::detail::submit_command(glow::viewer::detail::command&& c) { sCommandQueueStack.back()->push_back(c); }

void glow::viewer::detail::on_last_command()
{
    if (sCommandQueueStack.size() == 1)
    {
        // This is the last global command queue, run viewer app with the top level queue

        // Lazily init viewer globals
        global_init();

        ViewerApp app(sCommandQueueStack.back(), sGlobalSettings);
        sCurrentViewerApp = &app;

        try
        {
            app.run();
        }
        catch (ViewerApp::TerminateException&)
        {
            // Special Shift + ESC termination
            glow::AsyncTextureLoader::shutdown();
            std::exit(0);
        }

        sCurrentViewerApp = nullptr;

        // Clear the entire command queue stack for subsequent viewer runs
        sCommandQueueStack.clear();

        // Reset global settings for subsequent viewer runs
        sGlobalSettings = global_settings{};
    }
    else
    {
        // A nested command queue has completed, do nothing
        // popping from the stack is handled at the place that is now being returned to
    }
}

bool glow::viewer::detail::is_interactive(glow::viewer::detail::command_queue const& commands)
{
    for (auto const& cmd : commands)
        if (cmd.instr == detail::command::instruction::InteractiveSubview)
            return true;

    return false;
}

void glow::viewer::detail::create_layout_tree(glow::viewer::layout::tree_node& rootNode, glow::viewer::detail::command_queue const& commands, float deltaTime, bool allowInteractiveExecute)
{
    using cmd_t = detail::command;

    layout::tree_node* currentNode = nullptr;
    for (auto const& cmd : commands)
    {
        if (cmd.instr == cmd_t::instruction::AddRenderjob)
        {
            TG_ASSERT(currentNode != nullptr);
            auto const renderable = cmd.data_renderable;
            TG_ASSERT(renderable);
            renderable->runLazyInit();
            currentNode->scene.add(renderable);
        }
        else if (cmd.instr == cmd_t::instruction::ModifyScene)
        {
            TG_ASSERT(currentNode != nullptr);
            auto const funcp = cmd.data_scene;
            TG_ASSERT(funcp);
            funcp(currentNode->scene);
        }
        else if (cmd.instr == cmd_t::instruction::BeginSubview)
        {
            // Begin a subview
            if (currentNode == nullptr)
            {
                // Begin the root subview
                currentNode = &rootNode;
            }
            else
            {
                // Start a new subview within the current one
                currentNode->children.push_back(layout::tree_node(currentNode));

                currentNode = &currentNode->children.back();
                continue;
            }
        }
        else if (cmd.instr == cmd_t::instruction::EndSubview)
        {
            TG_ASSERT(currentNode != nullptr);

            currentNode = currentNode->parent;
            if (currentNode == nullptr)
            {
                // Done, root subview has ended
                break;
            }
        }
        else if (cmd.instr == cmd_t::instruction::ModifyLayout)
        {
            TG_ASSERT(currentNode != nullptr);
            auto const layoutSettings = cmd.data_settings;
            currentNode->layoutSettings = layoutSettings;
        }
        else if (TG_LIKELY(cmd.instr == cmd_t::instruction::InteractiveSubview && allowInteractiveExecute))
        {
            // Interactive subviews still use Begin/End as usual
            TG_ASSERT(currentNode != nullptr);

            // Create an inner command queue
            auto innerCommandQueue = createQueue();

            // Run the interactive lambda to fill innerCommandQueue
            {
                sCommandQueueStack.push_back(innerCommandQueue);
                auto const funcp = cmd.data_interactive;
                TG_ASSERT(funcp);
                funcp(deltaTime);
                sCommandQueueStack.pop_back();
            }

            // Recursively populate the tree with the recorded queue
            create_layout_tree(*currentNode, *innerCommandQueue, deltaTime);

            poolQueue(std::move(innerCommandQueue));
        }
    }
}

void glow::viewer::detail::set_left_mouse_control(bool active) { sGlobalSettings.leftMouseControlEnabled = active; }
void glow::viewer::detail::set_right_mouse_control(bool active) { sGlobalSettings.rightMouseControlEnabled = active; }
void glow::viewer::detail::set_ui_enabled(bool active) { sGlobalSettings.enableViewerUI = active; }
void glow::viewer::detail::set_2d_controls(bool active) { sGlobalSettings.controls2d_enabled = active; }
void glow::viewer::detail::set_has_3d_content(bool active) { sGlobalSettings.has3D = active; }
void glow::viewer::detail::set_ui_darkmode(bool active) { sGlobalSettings.imguiDarkMode = active; }
void glow::viewer::detail::set_subview_margin(int margin) { sGlobalSettings.subviewMargin = margin; }
void glow::viewer::detail::set_subview_margin_color(tg::color3 color) { sGlobalSettings.subviewMarginColor = color; }

void glow::viewer::detail::set_headless_screenshot(tg::ivec2 resolution, int accum, std::string const& filename, GLenum format)
{
    sGlobalSettings.headlessScreenshot = true;
    sGlobalSettings.screenshotFilename = filename;
    sGlobalSettings.screenshotAccumulationCount = accum;
    sGlobalSettings.screenshotResolution = resolution;
    sGlobalSettings.screenshotFormat = format;
}

void glow::viewer::detail::make_screenshot(std::string const& filename, int w, int h, int accumulationCount, GLenum format)
{
    if (sCurrentViewerApp == nullptr)
        glow::warning() << "Only use inside interactive viewers!";
    else
        return sCurrentViewerApp->renderScreenshotNextFrame(filename, w, h, accumulationCount, format);
}

cc::optional<tg::pos3> glow::viewer::detail::query_current_viewer_3d_position(int x, int y)
{
    if (sCurrentViewerApp == nullptr)
    {
        glow::warning() << "Only use inside interactive viewers!";
        return {};
    }
    else
        return sCurrentViewerApp->query3DPosition(x, y);
}

cc::optional<float> glow::viewer::detail::query_current_viewer_depth(int x, int y)
{
    if (sCurrentViewerApp == nullptr)
    {
        glow::warning() << "Only use inside interactive viewers!";
        return {};
    }
    else
        return sCurrentViewerApp->queryDepth(x, y);
}


tg::isize2 glow::viewer::detail::query_current_viewer_window_size()
{
    if (nullptr == sCurrentViewerApp)
    {
        glow::warning() << "Only use inside interactive viewers!";
        return {};
    }
    else
        return sCurrentViewerApp->getWindowSize();
}

tg::pos2 glow::viewer::detail::query_mouse_position()
{
    if (nullptr == sCurrentViewerApp)
    {
        glow::warning() << "Only use inside interactive viewers!";
        return {};
    }
    else
        return sCurrentViewerApp->getMousePosition();
}

void glow::viewer::detail::reset_camera_to_scene(bool clip_cam)
{
    if (nullptr == sCurrentViewerApp)
        glow::warning() << "Only use inside interactive viewers!";
    else
        sCurrentViewerApp->resetCameraToSceneNextFrame(clip_cam);
}

bool glow::viewer::detail::is_fullscreen()
{
    if (nullptr == sCurrentViewerApp)
    {
        glow::warning() << "Only use inside interactive viewers!";
        return false;
    }
    else
        return sCurrentViewerApp->isFullscreen();
}

bool glow::viewer::detail::is_in_viewer() { return sCurrentViewerApp != nullptr; }

void glow::viewer::detail::toggle_fullscreen()
{
    if (nullptr == sCurrentViewerApp)
        glow::warning() << "Only use inside interactive viewers!";
    else
        return sCurrentViewerApp->toggleFullscreen();
}

void glow::viewer::detail::set_file_drop_handler(std::function<void(std::vector<std::string> const&)> callback)
{
    sGlobalSettings.onDropFiles = std::move(callback);
}

void glow::viewer::detail::set_cache_window_size_folder(std::string_view s)
{
    auto& folder = sGlobalSettings.cacheWindowSizeFolder;
    folder = s;
    if (folder.empty())
        return;

    if (folder.back() != '/')
        folder.append("/");
    if (!std::filesystem::exists(folder))
    {
        glow::warning() << "Target folder for window size caching does not exist! Defaulting to working directory.";
        folder = "./";
    }
}
