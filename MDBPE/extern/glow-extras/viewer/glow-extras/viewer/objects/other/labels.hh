#pragma once

#include <glow-extras/vector/graphics2D.hh>

namespace glow::viewer
{
struct label_style
{
    tg::color3 background = tg::color3::white;
    tg::color3 border = tg::color3::black;
    float border_width = 2;
    int padding_x = 8;
    int padding_y = 4;

    // if non-zero, this overrides the label dir
    tg::vec2 override_dir;

    float line_width = 1.f;
    tg::color3 line_color = tg::color3::black;

    // how far away the label is from "pos" (in screen space)
    float pixel_distance = 40.f;

    // when the line starts
    float line_start_distance = 4.f;

    // in relative screen space, where does the line connect
    tg::vec2 anchor = {0, 1};

    glow::vector::font2D font = {"sans", 20};
    glow::vector::fill2D color = tg::color3::black;
};

struct label
{
    std::string text;
    label_style style;

    // world position
    tg::pos3 pos;

    // if non-zero and points away from camera, then label is not visible
    tg::vec3 normal;
};
}
