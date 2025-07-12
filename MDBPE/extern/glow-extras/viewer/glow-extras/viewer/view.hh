#pragma once

#include <functional>
#include <glow/common/nodiscard.hh>
#include <glow/common/non_copyable.hh>
#include <glow/glow.hh>
#include <utility>

#include "Scene.hh"
#include "renderables/Renderable.hh"

// the three important API headers:
#include "configure.hh"
#include "detail/command_queue.hh"
#include "make_renderable.hh"

// not really needed by often used:
#include <typed-geometry/tg-lean.hh>
#include "CameraController.hh"
#include "materials/materials.hh"
#include "objects/objects.hh"

/**
 * One-size-fits-most function for visualizing meshes, properties, ...
 *
 * view(...): creates a view and shows it
 *
 * See viewer/Readme.md for advanced usages
 */

/// creates a scoped object that automatically adds the provided configs to all views in this scope
#define GV_SCOPED_CONFIG(...) auto GLOW_MACRO_JOIN(_glow_viewer_scoped_config_, __COUNTER__) = ::glow::viewer::config(__VA_ARGS__)

#define GLOW_VIEWER_CONFIG(...) \
    auto GLOW_MACRO_JOIN(_glow_viewer_scoped_config_, __COUNTER__) = (::glow::viewer::detail::glow_viewer_config{}, ::glow::viewer::config(__VA_ARGS__))

namespace glow
{
namespace viewer
{
namespace detail
{
struct [[deprecated("use GV_SCOPED_CONFIG(...) instead")]] glow_viewer_config{};

struct raii_view_closer;
}

/// creates an empty 3D view
GLOW_NODISCARD detail::raii_view_closer view();

/// creates a 3D view, adds `obj` as renderable and calls v->configure(renderable, arg) for each arg in args
template <class ObjT, class... Args>
detail::raii_view_closer view(ObjT const& obj, Args const&... args);

/// creates an empty view with column layout
/// Usage:
///   auto c = viewer::columns();
///   c.view(pos, ...);
///   c.view(pos, ...);
///   ...
///   // show on end-of-scope
GLOW_NODISCARD detail::raii_view_closer columns();

/// creates an empty view with row layout
/// Usage:
///   auto r = viewer::rows();
///   r.view(pos, ...);
///   r.view(pos, ...);
///   ...
///   // show on end-of-scope
GLOW_NODISCARD detail::raii_view_closer rows();

/// creates an empty view with grid layout
/// Usage:
///   auto g = viewer::grid(2, 3);
///   g.view(pos, ...);
///   g.view(pos, ...);
///   ...
///   // show on end-of-scope
GLOW_NODISCARD detail::raii_view_closer grid(int cols, int rows);
GLOW_NODISCARD detail::raii_view_closer grid(float aspect = 1.0f); // aspect = w / h

// ======== Advanced usage ==========

/// closes the viewer in the next frame (interactive only)
void close_viewer();

void global_init();

/// Register a vector2d font that will be available to all viewer renderers created from now on
void global_add_vec2d_font(std::string const& name, std::string const& path);

/// sets the path used for default fonts (default "")
void global_set_default_font_path(std::string const& path);

/// Clear all globally registered vector2d fonts
void global_clear_vec2d_fonts();

/// Returns the current view matrix of the current subview
/// NOTE: only works inside interactive regions
tg::mat4 const& current_view_matrix();
/// Returns the current proj matrix of the current subview
/// NOTE: only works inside interactive regions
tg::mat4 const& current_proj_matrix();

struct close_info
{
    int closed_by_key = -1;
    tg::pos3 cam_pos;
    tg::pos3 cam_target;
};

close_info get_last_close_info();

// ======= Implementation ========

namespace detail
{
struct raii_config
{
    GLOW_RAII_CLASS(raii_config);

    using config_func = std::function<void(Renderable&)>;
    static std::vector<config_func> sConfigStack;
    static std::vector<bool*> sConfigUsedStack;

    static void applyConfigStack(Renderable& r)
    {
        for (auto b : detail::raii_config::sConfigUsedStack)
            *b = true;
        for (auto const& cf : detail::raii_config::sConfigStack)
            cf(r);
    }

    bool was_used = false;

public:
    raii_config(std::vector<config_func> const& funcs) : mNumConfigs(int(funcs.size()))
    {
        sConfigStack.insert(sConfigStack.end(), funcs.begin(), funcs.end());
        sConfigUsedStack.push_back(&was_used);
    }
    ~raii_config()
    {
        TG_ASSERT(sConfigStack.size() >= unsigned(mNumConfigs) && "corrupted config stack");
        TG_ASSERT(!sConfigUsedStack.empty() && sConfigUsedStack.back() == &was_used && "corrupted config stack");

        if (!*sConfigUsedStack.back())
            glow::warning() << "GLOW_VIEWER_CONFIG(...) was not used by any gv::view(...). did you accidentally put it into a too narrow scope?";

        sConfigStack.resize(sConfigStack.size() - unsigned(mNumConfigs));
        sConfigUsedStack.pop_back();
    }

    template <class T>
    static config_func packConfigFunc(T const& arg)
    {
        return config_func{[arg](Renderable& r) { ::glow::viewer::configure(r, arg); }};
    }

private:
    int const mNumConfigs;
};

struct raii_view_closer
{
private:
    bool const mMustCloseSubview; ///< If false, this view is inactive and integrates into the parent view

    bool isValid = true;

public:
    GLOW_RAII_CLASS(raii_view_closer);
    static std::vector<bool> sStackIsContainer;

    // Regular ctor
    explicit raii_view_closer(bool active) : mMustCloseSubview(active)
    {
        // No subview starts out as a container initially
        sStackIsContainer.push_back(false);
    }

    raii_view_closer(raii_view_closer&& rhs) noexcept : mMustCloseSubview(rhs.mMustCloseSubview), isValid(rhs.isValid) { rhs.isValid = false; }
    ~raii_view_closer() { show(); }
    void show()
    {
        if (!isValid)
            return;

        TG_ASSERT(!sStackIsContainer.empty() && "corrupted subview stack");

        // remove self from subview stack
        sStackIsContainer.pop_back();

        if (mMustCloseSubview)
            submit_command(command::endSubview());

        if (sStackIsContainer.empty())
        {
            on_last_command();
        }
        isValid = false;
    }

    static void convertCurrentSubviewToLayout(layout::settings&& settings)
    {
        TG_ASSERT(!sStackIsContainer.empty() && "corrupted subview stack");
        sStackIsContainer.back() = true;
        submit_command(command::modifyLayout(std::move(settings)));
    }

    template <class... Args>
    void configure(Args&&... args)
    {
        static_assert(sizeof...(args) > 0, "supply at least one argument");

        NullRenderable r;
        (glow::viewer::execute_configure(r, args), ...);
    }

public:
    /// Tag to construct a raii_view_closer that does nothing
    struct blank_tag
    {
    };

    explicit raii_view_closer(blank_tag) : mMustCloseSubview(false), isValid(false) {}
};

inline void initAndBeginSubview()
{
    command_queue_lazy_init();
    submit_command(command::beginSubview());
}


inline raii_view_closer getOrCreateView()
{
    TG_ASSERT(glow::isInitialized() && "glow is not initialized. maybe a 'glow::glfw::GlfwContext ctx;' is missing? (via #include <glow-extras/glfw/GlfwContext.hh>)");

    // if current view is not container, it will be used to place renderables in it
    if (!detail::raii_view_closer::sStackIsContainer.empty() && !detail::raii_view_closer::sStackIsContainer.back())
    {
        // no operation, this is implicit in the command queue, we just don't begin another subview
        // the view closer has to do nothing
        return raii_view_closer{false};
    }

    // otherwise a new subview is created
    initAndBeginSubview();
    // the view closer has to close this subview again
    return raii_view_closer{true};
}

} // namespace detail

template <class... Args>
GLOW_NODISCARD detail::raii_config config(Args const&... args)
{
    std::vector<detail::raii_config::config_func> funcs;
    funcs.reserve(sizeof...(args));

    int appendHelper[] = {0, (funcs.push_back(detail::raii_config::packConfigFunc(args)), 0)...};
    (void)appendHelper;

    return detail::raii_config(funcs);
}

inline detail::raii_view_closer view()
{
    auto v = detail::getOrCreateView();

    NullRenderable r;
    detail::raii_config::applyConfigStack(r);

    return v;
}

template <class ObjT, class... Args>
detail::raii_view_closer view(ObjT const& obj, Args const&... args)
{
    auto v = detail::getOrCreateView();

    static_assert(can_make_renderable<ObjT const&>, "no suitable make_renderable(obj) found. see \"make_renderable.hh\" for available options.");

    auto r = make_renderable(obj);
    submit_command(detail::command::addRenderjob(r));

    detail::raii_config::applyConfigStack(*r);

    (glow::viewer::execute_configure(*r, args), ...);

    if (!r->canBeEmpty())
        TG_ASSERT(!r->isEmpty() && "empty renderables are usually a bug. otherwise, use gv::view(..., gv::maybe_empty).");

    return v;
}

/// similar to view(obj, args...) but returns the created renderable and does NOT apply config stack
template <class ObjT, class... Args>
auto make_and_configure_renderable(ObjT const& obj, Args const&... args)
{
    static_assert(can_make_renderable<ObjT const&>, "no suitable make_renderable(obj) found. see \"make_renderable.hh\" for available options.");

    auto r = make_renderable(obj);
    (glow::viewer::execute_configure(*r, args), ...);

    return r;
}

inline detail::raii_view_closer columns() { return grid(-1, 1); }

inline detail::raii_view_closer rows() { return grid(1, -1); }

inline detail::raii_view_closer grid(int cols, int rows)
{
    auto v = detail::getOrCreateView();
    detail::raii_view_closer::convertCurrentSubviewToLayout({cols, rows});
    return v;
}

inline detail::raii_view_closer grid(float aspect)
{
    auto v = detail::getOrCreateView();
    detail::raii_view_closer::convertCurrentSubviewToLayout({-1, -1, aspect});
    return v;
}

inline detail::raii_view_closer nothing() { return detail::raii_view_closer{detail::raii_view_closer::blank_tag{}}; }

/// F: () -> void
/// or (float dt) -> void
template <class F>
detail::raii_view_closer interactive(F&& func)
{
    // Never integrates, always a new top-level subview
    detail::initAndBeginSubview();
    if constexpr (std::is_invocable_v<F, float>)
        detail::submit_command(detail::command::interactive(func));
    else if constexpr (std::is_invocable_v<F>)
        detail::submit_command(detail::command::interactive([func = std::forward<F>(func)](float) { func(); }));
    else
        static_assert(tg::always_false<F>, "func must be () -> void or (float dt) -> void");
    return detail::raii_view_closer{true};
}

// A view that clears its accumulation each time
template <class... Args>
detail::raii_view_closer view_cleared(Args&&... args)
{
    return ::glow::viewer::view(std::forward<Args>(args)..., clear_accumulation);
}

// Clears all accumulation of the current topmost view
// Only functional inside ::interactive
inline void view_clear_accumulation() { detail::submit_command(detail::command::sceneClearAccum()); }

/// Makes a screenshot of the current scene, only usable in interactive viewers
inline void make_screenshot(std::string const& filename, int w = -1, int h = -1, int accumulationCount = 128, GLenum format = GL_RGB8)
{
    detail::make_screenshot(filename, w, h, accumulationCount, format);
}


namespace detail
{
/// Internal use only, returns all currently registered fonts, {name, path}
std::vector<std::pair<std::string, std::string>> const& internal_global_get_fonts();
}
}
}

namespace gv = glow::viewer;
