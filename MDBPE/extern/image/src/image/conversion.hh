#pragma once

#include <image/image.hh>

namespace img
{
grayscale_image to_grayscale(rgb_image const& input);
rgb_image to_rgb(grayscale_image const& input);
}
