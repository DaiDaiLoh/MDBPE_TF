#pragma once

#include "image2D.hh"

#include <string>
#include <string_view>

#include <typed-geometry/tg.hh>

namespace glow
{
namespace vector
{
struct stroke2D
{
    tg::color4 color;
    paint2D paint;
    float width = 1.0f;
    float miter_limit = 10.f;
    line_cap cap = line_cap::butt;
    line_join join = line_join::miter;

    stroke2D() = default;
    stroke2D(tg::color3 const& c) : color(c, 1.0f), width(1.0f) {}
    stroke2D(tg::color4 const& c, float w = 1.0f) : color(c), width(w) {}
    stroke2D(float r, float g, float b, float a = 1.0f) : color(r, g, b, a), width(1.0f) {}
    stroke2D(paint2D const& p, float w = 1.0f) : paint(p), width(w) {}
};
struct fill2D
{
    tg::color4 color;
    paint2D paint;

    fill2D() = default;
    fill2D(tg::color4 const& c) : color(c) {}
    fill2D(tg::color3 const& c) : color(c) {}
    fill2D(float r, float g, float b, float a = 1.0f) : color(r, g, b, a) {}
    fill2D(paint2D const& p) : paint(p) {}
};
struct font2D
{
    std::string face;
    float size = 16.f;
    text_align align = text_align::baseline_left;
    float blur = 0.f;
    float letter_spacing = 0.f;
    float line_height = 1.f;

    font2D(char const* face) : face(face) {}
    font2D(std::string face, float size = 16.f, text_align align = text_align::baseline_left) : face(std::move(face)), size(size), align(align) {}
};

/// High-level API for writing to images
/// See Vector2DSample in glow-samples
///
/// Usage:
///   auto g = graphics(img);
///   g.draw(<tg-obj>, <stroke>);
///   g.fill(<tg-obj>, <fill>);
///
/// Example:
///   g.draw(tg::pos3{...}, {1, 0, 0}); // 1px red dot
///   g.draw(tg::segment3{...}, {tg::color3::blue, 5.0f}); // 5px blue segment
///
/// TODO:
///   - infinite types like ray and line
///   - vectors
///   - think about customization points
struct graphics2D
{
    explicit graphics2D(image2D& img) : img(img) {}

    // drawing
public:
    void draw(tg::pos2 const& p, stroke2D const& stroke)
    {
        img.begin_path();
        img.rect({p - .5f, p + .5f});
        perform_draw(stroke);
    }

    void draw(tg::segment2 const& s, stroke2D const& stroke)
    {
        img.begin_path();
        img.move_to(s.pos0);
        img.line_to(s.pos1);
        perform_draw(stroke);
    }

    void draw(tg::aabb2 const& r, stroke2D const& stroke)
    {
        img.begin_path();
        img.rect(r);
        perform_draw(stroke);
    }

    void draw(tg::triangle2 const& t, stroke2D const& stroke)
    {
        img.begin_path();
        img.move_to(t.pos0);
        img.line_to(t.pos1);
        img.line_to(t.pos2);
        img.line_to(t.pos0);
        perform_draw(stroke);
    }

    void draw(tg::quad2 const& t, stroke2D const& stroke)
    {
        img.begin_path();
        img.move_to(t.pos00);
        img.line_to(t.pos10);
        img.line_to(t.pos11);
        img.line_to(t.pos01);
        img.line_to(t.pos00);
        perform_draw(stroke);
    }

    void draw(tg::sphere_boundary<2, float> const& c, stroke2D const& stroke)
    {
        img.begin_path();
        img.circle(c.center, c.radius);
        perform_draw(stroke);
    }

    void draw(tg::sphere2 const& s, stroke2D const& stroke)
    {
        img.begin_path();
        img.circle(s.center, s.radius);
        perform_draw(stroke);
    }

    // filling
public:
    void fill(tg::aabb2 const& r, fill2D const& fill)
    {
        img.begin_path();
        img.rect(r);
        perform_fill(fill);
    }

    void fill(tg::triangle2 const& t, fill2D const& fill)
    {
        img.begin_path();
        img.move_to(t.pos0);
        img.line_to(t.pos1);
        img.line_to(t.pos2);
        img.line_to(t.pos0);
        perform_fill(fill);
    }

    void fill(tg::quad2 const& t, fill2D const& fill)
    {
        img.begin_path();
        img.move_to(t.pos00);
        img.line_to(t.pos10);
        img.line_to(t.pos11);
        img.line_to(t.pos01);
        img.line_to(t.pos00);
        perform_fill(fill);
    }

    void fill(tg::sphere2 const& s, fill2D const& fill)
    {
        img.begin_path();
        img.circle(s.center, s.radius);
        perform_fill(fill);
    }

    // text
public:
    void text(tg::pos2 p, std::string_view str, font2D const& font, fill2D const& fill = fill2D(0, 0, 0))
    {
        img.begin_path();
        apply_font(font);
        if (fill.paint.type == paint2D::t_invalid)
            img.set_fill_color(fill.color);
        else
            img.set_fill_paint(fill.paint);
        img.text(p.x, p.y, str);
    }
    void text_box(tg::pos2 p, float max_width, std::string_view str, font2D const& font, fill2D const& fill = fill2D(0, 0, 0))
    {
        img.begin_path();
        apply_font(font);
        if (fill.paint.type == paint2D::t_invalid)
            img.set_fill_color(fill.color);
        else
            img.set_fill_paint(fill.paint);
        img.text_box(p.x, p.y, max_width, str);
    }

public:
    image2D_low_level_api& low_level_api() { return img; }

private:
    void apply_font(font2D const& f)
    {
        img.set_font_size(f.size);
        img.set_font_blur(f.blur);
        img.set_font_face(f.face);
        img.set_text_align(f.align);
        img.set_text_line_height(f.line_height);
        img.set_text_letter_spacing(f.letter_spacing);
    }
    void perform_draw(stroke2D const& s)
    {
        if (s.paint.type == paint2D::t_invalid)
            img.set_stroke_color(s.color);
        else
            img.set_stroke_paint(s.paint);
        img.set_stroke_width(s.width);
        img.set_miter_limit(s.miter_limit);
        img.set_line_cap(s.cap);
        img.set_line_join(s.join);
        img.stroke();
    }
    void perform_fill(fill2D const& f)
    {
        if (f.paint.type == paint2D::t_invalid)
            img.set_fill_color(f.color);
        else
            img.set_fill_paint(f.paint);
        img.fill();
    }

    image2D_low_level_api img;
};

inline graphics2D graphics(image2D& img) { return graphics2D(img); }
}
}
