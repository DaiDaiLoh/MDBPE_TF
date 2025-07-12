#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <glow-extras/viewer/CameraController.hh>
#include <glow-extras/viewer/Scene.hh>
#include <glow-extras/viewer/layout.hh>
#include <glow-extras/viewer/renderables/Renderable.hh>

namespace glow::viewer::detail
{
inline auto constexpr disableGrid = [](Scene& s) { s.config.enableGrid = false; };
inline auto constexpr disableShadow = [](Scene& s) { s.config.enableShadows = false; };
inline auto constexpr enablePrintMode = [](Scene& s) { s.config.enablePrintMode = true; };
inline auto constexpr enableGrid = [](Scene& s) { s.config.enableGrid = true; };

struct global_settings
{
    bool imguiDarkMode = false;                          ///< Whether or not imgui is shown in a dark theme
    bool controls2d_enabled = false;                     ///< Whether or not controls are 2d instead of 3d
    int subviewMargin = 0;                               ///< The margin in pixels between subviews
    tg::color3 subviewMarginColor = tg::color3(0, 0, 0); ///< Color between subviews
    bool headlessScreenshot = false;                     ///< Whether to, on run, take a screenshot and immediately close
    std::string screenshotFilename = "viewer_screen.png";
    tg::ivec2 screenshotResolution = tg::ivec2(3840, 2160);
    int screenshotAccumulationCount = 64;
    GLenum screenshotFormat = GL_RGB8;
    bool leftMouseControlEnabled = true;
    bool rightMouseControlEnabled = true;
    bool enableViewerUI = true;

    std::string cacheWindowSizeFolder = "./";
    bool has3D = false; ///< True iff we have 3D content (this is set automatically)

    bool use_2d_controls() const { return controls2d_enabled || !has3D; }

    std::function<void(std::vector<std::string> const&)> onDropFiles;
};

struct command
{
    enum class instruction : uint8_t
    {
        BeginSubview,
        EndSubview,
        AddRenderjob,
        ModifyScene,
        ModifyLayout,
        InteractiveSubview
    };

    using scene_modifier_func_t = std::function<void(Scene&)>;
    using interactive_func_t = std::function<void(float)>;

    instruction const instr;
    layout::settings data_settings;
    SharedRenderable data_renderable;
    scene_modifier_func_t data_scene;
    interactive_func_t data_interactive;
    command(instruction i) : instr(i) {}

public:
    // == Command creators ==
    static command addRenderjob(SharedRenderable renderable)
    {
        command res{instruction::AddRenderjob};
        res.data_renderable = std::move(renderable);
        return res;
    }

    static command modifyLayout(layout::settings&& settings)
    {
        command res{instruction::ModifyLayout};
        res.data_settings = std::move(settings);
        return res;
    }

    static command beginSubview() { return command{instruction::BeginSubview}; }
    static command endSubview() { return command{instruction::EndSubview}; }

    static command interactive(std::function<void(float)> lambda)
    {
        command res{instruction::InteractiveSubview};
        res.data_interactive = std::move(lambda);
        return res;
    }

    static command sceneNoGrid()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.enableGrid = false; };
        return res;
    }

    static command scenePreserveCamera()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.preserveCamera = true; };
        return res;
    }

    static command sceneReuseCamera()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.reuseCamera = true; };
        return res;
    }

    static command sceneNoShadow()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.enableShadows = false; };
        return res;
    }

    static command sceneNoBackfacingShadow()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.enableBackfacingShadows = false; };
        return res;
    }

    static command scenePrintMode()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.enablePrintMode = true; };
        return res;
    }

    static command sceneNoOutline()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.enableOutlines = false; };
        return res;
    }

    static command sceneInfiniteAccumulation()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.infiniteAccumulation = true; };
        return res;
    }

    static command sceneBackgroundColor(tg::color3 inner, tg::color3 outer)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [inner, outer](Scene& s) {
            s.config.bgColorInner = inner;
            s.config.bgColorOuter = outer;
        };
        return res;
    }

    static command sceneBackgroundColor(tg::color3 col) { return sceneBackgroundColor(col, col); }

    static command sceneSsaoPower(float power)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [power](Scene& s) { s.config.ssaoPower = power; };
        return res;
    }

    static command sceneSsaoRadius(float radius)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [radius](Scene& s) { s.config.ssaoRadius = radius; };
        return res;
    }

    static command sceneTonemapping(float exposure)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [exposure](Scene& s) {
            s.config.enableTonemap = true;
            s.config.tonemapExposure = exposure;
        };
        return res;
    }

    static command sceneCameraOrientation(tg::angle azimuth, tg::angle altitude, float distance)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [azimuth, altitude, distance](Scene& s) {
            s.config.customCameraOrientation = true;
            s.config.cameraAzimuth = azimuth;
            s.config.cameraAltitude = altitude;
            s.config.cameraDistance = distance;
        };
        return res;
    }

    static command sceneCameraTransform(tg::pos3 pos, tg::pos3 target)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [pos, target](Scene& s) {
            s.config.customCameraPosition = true;
            s.config.cameraPosition = pos;
            s.config.cameraTarget = target;
        };
        return res;
    }

    static command sceneCameraFov(tg::horizontal_fov fov)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [fov](Scene& s) { s.config.cameraFov = fov; };
        return res;
    }

    static command sceneOrthogonalProjection(tg::aabb3 bounds)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [bounds](Scene& s) {
            s.config.enableOrthogonalProjection = true;
            s.config.orthogonalProjectionBounds = bounds;
        };
        return res;
    }

    static command sceneCustomAabb(tg::aabb3 bounds)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [bounds](Scene& s) { s.config.customBounds = bounds; };
        return res;
    }

    static command sceneCustomCameraController(SharedCameraController c)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [c = std::move(c)](Scene& s) { s.config.customCameraController = c; };
        return res;
    }

    static command sceneClearAccum()
    {
        command res{instruction::ModifyScene};
        res.data_scene = [](Scene& s) { s.config.clearAccumulation = true; };
        return res;
    }

    static command sceneCloseKeys(std::vector<int> keys)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [keys = std::move(keys)](Scene& s) { s.config.closeKeys = keys; };
        return res;
    }

    static command sceneCustomConfig(std::function<void(SceneConfig&)> f)
    {
        command res{instruction::ModifyScene};
        res.data_scene = [f = std::move(f)](Scene& s) { f(s.config); };
        return res;
    }
};

using command_queue = std::vector<command>;
using shared_command_queue = std::shared_ptr<command_queue>;

// Lazily creates the top-level command queue
void command_queue_lazy_init();
// Submits a command into the currently active command queue
void submit_command(command&& c);
// Called on the last command of any queue
void on_last_command();

void set_left_mouse_control(bool active);
void set_right_mouse_control(bool active);
void set_ui_enabled(bool active);
void set_2d_controls(bool active);
void set_has_3d_content(bool active);
void set_ui_darkmode(bool active);
void set_subview_margin(int margin);
void set_subview_margin_color(tg::color3 color);
void set_headless_screenshot(tg::ivec2 resolution, int accum, std::string const& filename, GLenum format);
void set_file_drop_handler(std::function<void(std::vector<std::string> const&)> callback);
void set_cache_window_size_folder(std::string_view s);

// Returns true if the given command list contains one or more interactive instructions (and whose tree therefore must be rebuilt each frame)
bool is_interactive(command_queue const& commands);

// Builds a layout tree from a given list of commands
// deltaTime required to call interactive lambdas
void create_layout_tree(layout::tree_node& rootNode, command_queue const& commands, float deltaTime, bool allowInteractiveExecute = true);

cc::optional<tg::pos3> query_current_viewer_3d_position(int x, int y);
cc::optional<float> query_current_viewer_depth(int x, int y);
tg::isize2 query_current_viewer_window_size();
tg::pos2 query_mouse_position();
void reset_camera_to_scene(bool clip_cam);
bool is_fullscreen();
bool is_in_viewer();
void toggle_fullscreen();

void make_screenshot(std::string const& filename, int w, int h, int accumulationCount, GLenum format);
}
