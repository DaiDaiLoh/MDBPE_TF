#pragma once

#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

#include "cmd2D.hh"
#include "text_align.hh"

#include <typed-geometry/feature/assert.hh>
#include <typed-geometry/tg-lean.hh>

namespace glow
{
namespace vector
{
/**
 * A 2D vector image
 * (Internally represented via a low-level command buffer)
 *
 * TODO: compose images
 */
struct image2D
{
public:
    void clear() { buffer.clear(); }

    std::vector<uint32_t> const& raw_buffer() const { return buffer; }

private:
    std::vector<uint32_t> buffer;

    image2D& operator<<(cmd2D c)
    {
        buffer.push_back(uint32_t(c));
        return *this;
    }
    image2D& operator<<(uint32_t v)
    {
        buffer.push_back(v);
        return *this;
    }
    image2D& operator<<(float v)
    {
        uint32_t u;
        std::memcpy(&u, &v, sizeof(uint32_t));
        buffer.push_back(u);
        return *this;
    }
    image2D& operator<<(std::string_view v)
    {
        buffer.push_back(uint32_t(v.size()));
        auto cnt = v.size() / sizeof(uint32_t) + 1; // enough for \0 at the end
        auto i = buffer.size();
        buffer.resize(buffer.size() + cnt, uint32_t(0));
        std::memcpy(buffer.data() + i, v.data(), v.size());
        return *this;
    }

    friend struct image2D_low_level_api;

    template <class Visitor>
    friend void visit(image2D const& img, Visitor&& v);
};

// ================= LOW LEVEL API =================
// (for high-level API see graphics2D.hh)

struct paint2D
{
    enum
    {
        t_invalid,
        t_linear_gradient,
        t_box_gradient,
        t_radial_gradient,
        t_image_pattern
    } type
        = t_invalid;

    union data_t {
        struct
        {
            tg::pos2 start_pos;
            tg::pos2 end_pos;
            tg::color4 start_color;
            tg::color4 end_color;
        } linear;

        struct
        {
            tg::aabb2 bounds;
            float radius;
            float feather;
            tg::color4 inner_color;
            tg::color4 outer_color;
        } box;

        struct
        {
            tg::pos2 center;
            float inner_radius;
            float outer_radius;
            tg::color4 inner_color;
            tg::color4 outer_color;
        } radial;

        struct
        {
            tg::pos2 top_left;
            tg::size2 size;
            tg::angle angle;
            int img_handle; // TODO?
            float alpha;
        } image;

        struct
        {
            uint32_t values[14];
        } raw;

        data_t() {}
    } data;

    static_assert(sizeof(data) == sizeof(data.raw));

    static paint2D linear_gradient(tg::pos2 start, tg::pos2 end, tg::color4 start_color, tg::color4 end_color)
    {
        paint2D p;
        p.type = t_linear_gradient;
        p.data.linear = {start, end, start_color, end_color};
        return p;
    }

    static paint2D box_gradient(tg::aabb2 bounds, float radius, float feather, tg::color4 inner_color, tg::color4 outer_color)
    {
        paint2D p;
        p.type = t_box_gradient;
        p.data.box = {bounds, radius, feather, inner_color, outer_color};
        return p;
    }

    static paint2D radial_gradient(tg::pos2 center, float inner_radius, float outer_radius, tg::color4 inner_color, tg::color4 outer_color)
    {
        paint2D p;
        p.type = t_radial_gradient;
        p.data.radial = {center, inner_radius, outer_radius, inner_color, outer_color};
        return p;
    }

    static paint2D image_pattern(tg::pos2 top_left, tg::size2 size, tg::angle angle, int img_handle, float alpha = 1.0f)
    {
        paint2D p;
        p.type = t_image_pattern;
        p.data.image = {top_left, size, angle, img_handle, alpha};
        return p;
    }
};

// see https://github.com/memononen/nanovg/blob/master/src/nanovg.h
struct image2D_low_level_api
{
    explicit image2D_low_level_api(image2D& img) : img(img) {}

    void begin_path() { img << cmd2D::begin_path; }
    void close_path() { img << cmd2D::close_path; }

    void move_to(float x, float y) { img << cmd2D::move_to << x << y; }
    void line_to(float x, float y) { img << cmd2D::line_to << x << y; }
    void bezier_to(float c1x, float c1y, float c2x, float c2y, float x, float y) { img << cmd2D::bezier_to << c1x << c1y << c2x << c2y << x << y; }
    void quad_to(float cx, float cy, float x, float y) { img << cmd2D::quad_to << cx << cy << x << y; }
    void arc_to(float x1, float y1, float x2, float y2, float radius) { img << cmd2D::arc_to << x1 << y1 << x2 << y2 << radius; }

    // TODO: arc, rounded_rect_varying, ellipse
    void rect(float x, float y, float w, float h) { img << cmd2D::rect << x << y << w << h; }
    void rounded_rect(float x, float y, float w, float h, float r) { img << cmd2D::rounded_rect << x << y << w << h << r; }
    void circle(float cx, float cy, float r) { img << cmd2D::circle << cx << cy << r; }

    void stroke() { img << cmd2D::stroke; }
    void fill() { img << cmd2D::fill; }

    void set_stroke_width(float s) { img << cmd2D::set_stroke_width << s; }
    void set_stroke_color(float r, float g, float b, float a) { img << cmd2D::set_stroke_color << r << g << b << a; }
    void set_fill_color(float r, float g, float b, float a) { img << cmd2D::set_fill_color << r << g << b << a; }
    void set_line_cap(line_cap cap) { img << cmd2D::set_line_cap << uint32_t(cap); }
    void set_line_join(line_join join) { img << cmd2D::set_line_join << uint32_t(join); }
    void set_miter_limit(float limit) { img << cmd2D::set_miter_limit << limit; }

    void set_stroke_paint(paint2D const& p)
    {
        img << cmd2D::set_stroke_paint;
        img << uint32_t(p.type);
        for (auto i = 0u; i < sizeof(p.data.raw) / sizeof(uint32_t); ++i)
            img << p.data.raw.values[i];
    }
    void set_fill_paint(paint2D const& p)
    {
        img << cmd2D::set_fill_paint;
        img << uint32_t(p.type);
        for (auto i = 0u; i < sizeof(p.data.raw) / sizeof(uint32_t); ++i)
            img << p.data.raw.values[i];
    }

    void clip_rect(float x, float y, float w, float h) { img << cmd2D::clip_rect << x << y << w << h; }
    void reset_clip() { img << cmd2D::reset_clip; }

    // tg versions:
    void move_to(tg::pos2 const& p) { move_to(p.x, p.y); }
    void line_to(tg::pos2 const& p) { line_to(p.x, p.y); }
    void bezier_to(tg::pos2 const& c1, tg::pos2 const& c2, tg::pos2 const& p) { bezier_to(c1.x, c1.y, c2.x, c2.y, p.x, p.y); }
    void quad_to(tg::pos2 const& c, tg::pos2 const& p) { quad_to(c.x, c.y, p.x, p.y); }
    void arc_to(tg::pos2 const& p1, tg::pos2 const& p2, float radius) { arc_to(p1.x, p1.y, p2.x, p2.y, radius); }

    // TODO: arc, rounded_rect_varying, ellipse
    void rect(tg::aabb2 const& r) { rect(r.min.x, r.min.y, r.max.x - r.min.x, r.max.y - r.min.y); }
    void rounded_rect(tg::aabb2 const& r, float radius) { rounded_rect(r.min.x, r.min.y, r.max.x - r.min.x, r.max.y - r.min.y, radius); }
    void circle(tg::pos2 const& p, float r) { circle(p.x, p.y, r); }

    void set_stroke_color(tg::color4 const& c) { set_stroke_color(c.r, c.g, c.b, c.a); }
    void set_fill_color(tg::color4 const& c) { set_fill_color(c.r, c.g, c.b, c.a); }

    void clip_rect(tg::aabb2 const& r) { clip_rect(r.min.x, r.min.y, r.max.x - r.min.x, r.max.y - r.min.y); }

    void set_font_size(float s) { img << cmd2D::set_font_size << s; }
    void set_font_blur(float s) { img << cmd2D::set_font_blur << s; }
    void set_text_letter_spacing(float s) { img << cmd2D::set_text_letter_spacing << s; }
    void set_text_line_height(float s) { img << cmd2D::set_text_line_height << s; }
    void set_text_align(text_align a) { img << cmd2D::set_text_align << uint32_t(a); }
    void set_font_face(std::string_view f) { img << cmd2D::set_font_face << f; }
    void text(float x, float y, std::string_view s) { img << cmd2D::text << x << y << s; }
    void text_box(float x, float y, float max_w, std::string_view s) { img << cmd2D::text_box << x << y << max_w << s; }

    void global_alpha(float a) { img << cmd2D::global_alpha << a; }

private:
    image2D& img;
};

inline image2D_low_level_api low_level_api(image2D& img) { return image2D_low_level_api(img); }

template <class Visitor>
void visit(image2D const& img, Visitor&& v)
{
    int idx = 0;
    auto buffer_size = int(img.buffer.size());
    uint32_t const* data = img.buffer.data();
    paint2D paint;

    auto get_u32 = [&]() -> uint32_t {
        TG_ASSERT(idx < buffer_size);
        return data[idx++];
    };
    auto get_cmd = [&]() -> cmd2D { return static_cast<cmd2D>(get_u32()); };
    auto get_f32 = [&]() -> float {
        auto u = get_u32();
        float f;
        std::memcpy(&f, &u, sizeof(float));
        return f;
    };
    auto get_str = [&]() -> char const* {
        auto ss = get_u32();
        auto s = reinterpret_cast<char const*>(data + idx);
        auto bc = ss / sizeof(uint32_t) + 1; // enough for \0
        idx += int(bc);
        return s;
    };

    float f1, f2, f3, f4, f5;
    char const* str;
    while (idx < buffer_size)
    {
        auto cmd = get_cmd();
        switch (cmd)
        {
        case cmd2D::begin_path:
            v.begin_path();
            break;
        case cmd2D::close_path:
            v.close_path();
            break;

            // TODO: bezier_to, quad_to, arc_to
        case cmd2D::move_to:
            f1 = get_f32();
            f2 = get_f32();
            v.move_to(f1, f2);
            break;
        case cmd2D::line_to:
            f1 = get_f32();
            f2 = get_f32();
            v.line_to(f1, f2);
            break;

            // TODO: arc, rounded_rect_varying, ellipse
        case cmd2D::rect:
            f1 = get_f32();
            f2 = get_f32();
            f3 = get_f32();
            f4 = get_f32();
            v.rect(f1, f2, f3, f4);
            break;
        case cmd2D::rounded_rect:
            f1 = get_f32();
            f2 = get_f32();
            f3 = get_f32();
            f4 = get_f32();
            f5 = get_f32();
            v.rounded_rect(f1, f2, f3, f4, f5);
            break;
        case cmd2D::circle:
            f1 = get_f32();
            f2 = get_f32();
            f3 = get_f32();
            v.circle(f1, f2, f3);
            break;

        case cmd2D::stroke:
            v.stroke();
            break;
        case cmd2D::fill:
            v.fill();
            break;

        case cmd2D::set_stroke_width:
            f1 = get_f32();
            v.set_stroke_width(f1);
            break;
        case cmd2D::set_stroke_color:
            f1 = get_f32();
            f2 = get_f32();
            f3 = get_f32();
            f4 = get_f32();
            v.set_stroke_color(f1, f2, f3, f4);
            break;
        case cmd2D::set_fill_color:
            f1 = get_f32();
            f2 = get_f32();
            f3 = get_f32();
            f4 = get_f32();
            v.set_fill_color(f1, f2, f3, f4);
            break;
        case cmd2D::set_line_cap:
            v.set_line_cap(line_cap(get_u32()));
            break;
        case cmd2D::set_line_join:
            v.set_line_join(line_join(get_u32()));
            break;
        case cmd2D::set_miter_limit:
            f1 = get_f32();
            v.set_miter_limit(f1);
            break;
        case cmd2D::set_stroke_paint:
            paint.type = decltype(paint.type)(get_u32());
            for (auto i = 0u; i < sizeof(paint.data.raw) / sizeof(uint32_t); ++i)
                paint.data.raw.values[i] = get_u32();
            v.set_stroke_paint(paint);
            break;
        case cmd2D::set_fill_paint:
            paint.type = decltype(paint.type)(get_u32());
            for (auto i = 0u; i < sizeof(paint.data.raw) / sizeof(uint32_t); ++i)
                paint.data.raw.values[i] = get_u32();
            v.set_fill_paint(paint);
            break;

        case cmd2D::clip_rect:
            f1 = get_f32();
            f2 = get_f32();
            f3 = get_f32();
            f4 = get_f32();
            v.clip_rect(f1, f2, f3, f4);
            break;
        case cmd2D::reset_clip:
            v.reset_clip();
            break;

        case cmd2D::set_font_size:
            f1 = get_f32();
            v.set_font_size(f1);
            break;
        case cmd2D::set_font_blur:
            f1 = get_f32();
            v.set_font_blur(f1);
            break;
        case cmd2D::set_text_letter_spacing:
            f1 = get_f32();
            v.set_text_letter_spacing(f1);
            break;
        case cmd2D::set_text_line_height:
            f1 = get_f32();
            v.set_text_line_height(f1);
            break;
        case cmd2D::set_text_align:
            v.set_text_align(text_align(get_u32()));
            break;
        case cmd2D::set_font_face:
            str = get_str();
            v.set_font_face(str);
            break;
        case cmd2D::text:
            f1 = get_f32();
            f2 = get_f32();
            str = get_str();
            v.text(f1, f2, str);
            break;
        case cmd2D::text_box:
            f1 = get_f32();
            f2 = get_f32();
            f3 = get_f32();
            str = get_str();
            v.text_box(f1, f2, f3, str);
            break;

        case cmd2D::global_alpha:
            v.global_alpha(get_f32());
            break;

        default:
            TG_ASSERT(false && "unsupported command");
            return;
        }
    }
}
}
}
