#include "opengl.hh"

#include <cstring>

#include <glow/common/log.hh>
#include <glow/common/scoped_gl.hh>
#include <glow/gl.hh>

#include "nanovg/nanovg.hh"
#include "nanovg/nanovg_gl.hh"

#include "../image2D.hh"

#include <typed-geometry/tg.hh>

using namespace glow;
using namespace glow::vector;


static int vg_cap(line_cap c)
{
    switch (c)
    {
    case line_cap::butt:
        return nvg::NVG_BUTT;
    case line_cap::round:
        return nvg::NVG_ROUND;
    case line_cap::square:
        return nvg::NVG_SQUARE;
    default:
        return 0;
    }
};
static int vg_join(line_join j)
{
    switch (j)
    {
    case line_join::bevel:
        return nvg::NVG_BEVEL;
    case line_join::miter:
        return nvg::NVG_MITER;
    case line_join::round:
        return nvg::NVG_ROUND;
    default:
        return 0;
    }
};

OGLRenderer::~OGLRenderer()
{
    if (vg)
        nvg::nvgDeleteGL3(vg);
}

ogl_context::~ogl_context()
{
    if (vg)
    {
        nvgEndFrame(vg);

        if (was_blending_enabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }
}

void ogl_context::render(const image2D& img) const
{
    using namespace nvg;

    struct nvg_renderer
    {
        NVGcontext* vg;

        NVGcolor c(tg::color4 const& cc) const { return {{{cc.r, cc.g, cc.b, cc.a}}}; }

        NVGpaint make_paint(paint2D const& p) const
        {
            switch (p.type)
            {
            case paint2D::t_linear_gradient:
            {
                auto const& d = p.data.linear;
                return nvgLinearGradient(vg, d.start_pos.x, d.start_pos.y, d.end_pos.x, d.end_pos.y, c(d.start_color), c(d.end_color));
            }
            case paint2D::t_box_gradient:
            {
                auto const& d = p.data.box;
                auto s = d.bounds.max - d.bounds.min;
                return nvgBoxGradient(vg, d.bounds.min.x, d.bounds.min.y, s.x, s.y, d.radius, d.feather, c(d.inner_color), c(d.outer_color));
            }
            case paint2D::t_radial_gradient:
            {
                auto const& d = p.data.radial;
                return nvgRadialGradient(vg, d.center.x, d.center.y, d.inner_radius, d.outer_radius, c(d.inner_color), c(d.outer_color));
            }
            case paint2D::t_image_pattern:
            {
                auto const& d = p.data.image;
                return nvgImagePattern(vg, d.top_left.x, d.top_left.y, d.size.width, d.size.height, d.angle.radians(), d.img_handle, d.alpha);
            }
            default:
                return {};
            }
        }

        void begin_path() { nvgBeginPath(vg); }
        void close_path() { nvgClosePath(vg); }

        void move_to(float x, float y) { nvgMoveTo(vg, x, y); }
        void line_to(float x, float y) { nvgLineTo(vg, x, y); }
        void bezier_to(float c1x, float c1y, float c2x, float c2y, float x, float y) { nvgBezierTo(vg, c1x, c1y, c2x, c2y, x, y); }
        void quad_to(float cx, float cy, float x, float y) { nvgQuadTo(vg, cx, cy, x, y); }
        void arc_to(float x1, float y1, float x2, float y2, float radius) { nvgArcTo(vg, x1, y1, x2, y2, radius); }

        // TODO: arc, rounded_rect_varying, ellipse
        void rect(float x, float y, float w, float h) { nvgRect(vg, x, y, w, h); }
        void rounded_rect(float x, float y, float w, float h, float r) { nvgRoundedRect(vg, x, y, w, h, r); }
        void circle(float cx, float cy, float r) { nvgCircle(vg, cx, cy, r); }

        void stroke() { nvgStroke(vg); }
        void fill() { nvgFill(vg); }

        void set_stroke_width(float s) { nvgStrokeWidth(vg, s); }
        void set_stroke_color(float r, float g, float b, float a) { nvgStrokeColor(vg, {{{r, g, b, a}}}); }
        void set_fill_color(float r, float g, float b, float a) { nvgFillColor(vg, {{{r, g, b, a}}}); }
        void set_line_cap(line_cap cap) { nvgLineCap(vg, vg_cap(cap)); }
        void set_line_join(line_join join) { nvgLineJoin(vg, vg_join(join)); }
        void set_miter_limit(float limit) { nvgMiterLimit(vg, limit); }
        void set_stroke_paint(paint2D const& p) { nvgStrokePaint(vg, make_paint(p)); }
        void set_fill_paint(paint2D const& p) { nvgFillPaint(vg, make_paint(p)); }

        void set_font_size(float s) { nvgFontSize(vg, s); }
        void set_font_blur(float s) { nvgFontBlur(vg, s); }
        void set_text_letter_spacing(float s) { nvgTextLetterSpacing(vg, s); }
        void set_text_line_height(float s) { nvgTextLineHeight(vg, s); }
        void set_text_align(text_align a) { nvgTextAlign(vg, int(a)); }
        void set_font_face(char const* f) { nvgFontFace(vg, f); }
        void text(float x, float y, char const* s) { nvgText(vg, x, y, s, nullptr); }
        void text_box(float x, float y, float max_w, char const* s) { nvgTextBox(vg, x, y, max_w, s, nullptr); }

        void clip_rect(float x, float y, float w, float h) { nvgScissor(vg, x, y, w, h); }
        void reset_clip() { nvgResetScissor(vg); }

        void global_alpha(float a) { nvgGlobalAlpha(vg, a); }

        //        void reset_transform() { nvgResetTransform(vg); }
        //        void apply_transform(float a, float b, float c, float d, float e, float f) { nvgTransform(vg, a, b, c, d, e, f); }
        //        void set_transform(float a, float b, float c, float d, float e, float f)
        //        {
        //            reset_transform();
        //            apply_transform(a, b, c, d, e, f);
        //        }
        //        void translate(float x, float y) { return nvgTranslate(vg, x, y); }
        //        void rotate(float angle) { nvgRotate(vg, angle); }
        //        void skew_x(float angle) { nvgSkewX(vg, angle); }
        //        void skew_y(float angle) { nvgSkewY(vg, angle); }
        //        void scale(float x, float y) { nvgScale(vg, x, y); }
    };

    visit(img, nvg_renderer{vg});
}

void ogl_context::apply_font(const font2D& font) const
{
    TG_ASSERT(vg && "invalid context");

    using namespace nvg;

    nvgFontSize(vg, font.size);
    nvgFontBlur(vg, font.blur);
    nvgFontFace(vg, font.face.c_str());
    nvgTextLineHeight(vg, font.line_height);
    nvgTextLetterSpacing(vg, font.letter_spacing);
    nvgTextAlign(vg, int(font.align));
}

tg::aabb2 ogl_context::text_bounds(font2D const& font, float x, float y, std::string_view str) const
{
    TG_ASSERT(vg && "invalid context");

    float bb[4];
    apply_font(font);
    nvg::nvgTextBounds(vg, x, y, str.data(), str.data() + str.size(), bb);
    return {{bb[0], bb[1]}, {bb[2], bb[3]}};
}

tg::aabb2 ogl_context::text_box_bounds(font2D const& font, float x, float y, float breakRowWidth, std::string_view str) const
{
    TG_ASSERT(vg && "invalid context");

    float bb[4];
    apply_font(font);
    nvg::nvgTextBoxBounds(vg, x, y, breakRowWidth, str.data(), str.data() + str.size(), bb);
    return {{bb[0], bb[1]}, {bb[2], bb[3]}};
}

void OGLRenderer::render(image2D const& img, float w, float h, float devicePixelRatio) const
{
    auto f = frame(w, h, devicePixelRatio);
    f.render(img);
}

ogl_context OGLRenderer::frame(float w, float h, float devicePixelRatio) const
{
    GLboolean was_blending_enabled;
    glGetBooleanv(GL_BLEND, &was_blending_enabled);
    glEnable(GL_BLEND); // not disabled by nanovg

    lazy_init();

    nvgBeginFrame(vg, w, h, devicePixelRatio);

    nvgResetTransform(vg);
    nvgTransform(vg,                     //
                 global_transform[0][0], //
                 global_transform[0][1], //
                 global_transform[1][0], //
                 global_transform[1][1], //
                 global_transform[2][0], //
                 global_transform[2][1]);

    return {vg, bool(was_blending_enabled)};
}

void OGLRenderer::loadFontFromFile(std::string const& name, std::string const& file)
{
    lazy_init();

    auto f = nvg::nvgCreateFont(vg, name.c_str(), file.c_str());
    if (f == -1)
        glow::error() << "could not create font '" << name << "' from file '" << file << "'";
}

void OGLRenderer::loadFontFromMemory(std::string const& name, array_view<const std::byte> font)
{
    lazy_init();

    unsigned char* data = static_cast<unsigned char*>(malloc(font.size()));
    std::memcpy(data, font.begin(), font.size());

    auto f = nvg::nvgCreateFontMem(vg, name.c_str(), data, int(font.size()), true);
    if (f == -1)
        glow::error() << "could not create font '" << name << "' from memory (" << font.size() << " bytes)";
}

void OGLRenderer::addFallbackFont(const std::string& baseFont, const std::string& fallbackFont)
{
    lazy_init();

    auto f = nvg::nvgAddFallbackFont(vg, baseFont.c_str(), fallbackFont.c_str());
    if (f == -1)
        glow::error() << "could not add fallback font '" << fallbackFont << "' to base font '" << baseFont << "'";
}

void OGLRenderer::lazy_init() const
{
    if (!vg)
        vg = nvg::nvgCreateGL3(nvg::NVG_ANTIALIAS | nvg::NVG_STENCIL_STROKES);
}
