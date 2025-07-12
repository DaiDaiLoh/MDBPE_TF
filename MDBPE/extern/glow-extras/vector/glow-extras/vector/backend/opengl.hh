#pragma once

#include <string>
#include <string_view>

#include <glow/common/array_view.hh>
#include <glow/common/non_copyable.hh>

#include <typed-geometry/tg-lean.hh>

#include "../graphics2D.hh"

namespace nvg
{
struct NVGcontext;
}

namespace glow
{
namespace vector
{
struct image2D;

struct ogl_context
{
    GLOW_RAII_CLASS(ogl_context);

    ogl_context(nvg::NVGcontext* vg, bool was_blending_enabled) : vg(vg), was_blending_enabled(was_blending_enabled) {}
    ogl_context(ogl_context&& rhs) : vg(rhs.vg), was_blending_enabled(rhs.was_blending_enabled) { rhs.vg = nullptr; }
    ~ogl_context();

    void render(image2D const& img) const;

    tg::aabb2 text_bounds(font2D const& font, float x, float y, std::string_view str) const;
    tg::aabb2 text_box_bounds(font2D const& font, float x, float y, float breakRowWidth, std::string_view str) const;


private:
    nvg::NVGcontext* vg;
    bool was_blending_enabled;

    void apply_font(font2D const& font) const;
};

class OGLRenderer
{
    GLOW_NON_COPYABLE(OGLRenderer);

public:
    OGLRenderer() = default;
    ~OGLRenderer();

    /// renders the image to the current bound framebuffer
    /// NOTE: must have stencil buffer!
    ///
    /// shortcut for:
    ///   auto f = frame(...);
    ///   f.render(img);
    void render(image2D const& img, float w, float h, float devicePixelRatio = 1.0f) const;

    /// returns a RAII object in which images can be drawn
    /// NOTE: must have stencil buffer!
    ogl_context frame(float w, float h, float devicePixelRatio = 1.0f) const;

    /// loads a font and makes it available under "name"
    void loadFontFromFile(std::string const& name, std::string const& file);
    void loadFontFromMemory(std::string const& name, array_view<const std::byte> font);

    void addFallbackFont(std::string const& baseFont, std::string const& fallbackFont);

    void set_global_transform(tg::mat3x2 const& global_transform) { this->global_transform = global_transform; }

private:
    void lazy_init() const;

    tg::mat3x2 global_transform = tg::mat3x2::identity;
    mutable nvg::NVGcontext* vg = nullptr;
};
}
}
