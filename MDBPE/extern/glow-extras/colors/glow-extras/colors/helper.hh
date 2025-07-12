#pragma once

#include <cmath>

namespace glow
{
namespace colors
{
namespace detail
{
// https://www.w3.org/TR/2008/REC-WCAG20-20081211/#relativeluminancedef
inline float srgb_to_linear(float l) { return l < 0.04045f ? l / 12.92f : std::pow((l + 0.055f) / 1.055f, 2.4f); }
inline float linear_to_srgb(float l) { return l < 0.0031308f ? l * 12.92f : std::pow(l, 1.0f / 2.4f) * 1.055f - 0.055f; }
}
}
}
