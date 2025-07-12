#pragma once

namespace glow
{
namespace vector
{
// basically taken from https://github.com/memononen/nanovg/blob/master/src/nanovg.h
enum class cmd2D
{
    begin_path,
    close_path,

    move_to,
    line_to,
    bezier_to,
    quad_to,
    arc_to,

    arc,
    rect,
    rounded_rect,
    rounded_rect_varying,
    circle,
    ellipse,

    stroke,
    fill,

    set_stroke_width,
    set_stroke_color,
    set_fill_color,
    set_line_cap,
    set_line_join,
    set_miter_limit,
    set_fill_paint,
    set_stroke_paint,

    clip_rect,
    reset_clip,

    set_font_size,
    set_font_blur,
    set_text_letter_spacing,
    set_text_line_height,
    set_text_align,
    set_font_face, // TODO: id version
    text,
    text_box,
    // TODO: measures

    global_alpha

    // TODO: transform
    // TODO: images
};

enum class line_cap
{
    butt,
    round,
    square
};
enum class line_join
{
    miter,
    round,
    bevel
};
}
}
