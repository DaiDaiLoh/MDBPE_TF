#pragma once

#include <image/image.hh>
#include <typed-geometry/types/angle.hh>
#include <typed-geometry/types/dir.hh>

namespace img
{
image<tg::vec2> gradient_of(image<float> const& input);

image<tg::angle32> angles_of(image<tg::vec2> const& gradient_image);

grayscale_image amplitudes_of(image<tg::vec2> const& gradient_image);
}
