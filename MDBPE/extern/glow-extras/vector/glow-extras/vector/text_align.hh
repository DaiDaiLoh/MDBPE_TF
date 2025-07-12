#pragma once

namespace glow::vector
{
enum class text_align
{
    // horizontal
    left = 1 << 0,
    center = 1 << 1,
    right = 1 << 2,

    // vertical
    top = 1 << 3,
    middle = 1 << 4,
    bottom = 1 << 5,
    baseline = 1 << 6,

    // special
    top_left = top | left,
    top_center = top | center,
    top_right = top | right,
    middle_left = middle | left,
    middle_center = middle | center,
    middle_right = middle | right,
    bottom_left = bottom | left,
    bottom_center = bottom | center,
    bottom_right = bottom | right,
    baseline_left = baseline | left,
    baseline_center = baseline | center,
    baseline_right = baseline | right,
};
}
