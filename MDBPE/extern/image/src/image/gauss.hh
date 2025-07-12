#pragma once

#include <image/image.hh>

#include <typed-geometry/types/dir.hh>

namespace img
{
grayscale_image gauss(grayscale_image const& input);

rgb_image gauss(rgb_image const& input);

/// like above, but will keep pixel where mask is false constant
rgb_image gauss(rgb_image const& input, binary_image const& mask);

image<tg::dir2> gauss(image<tg::dir2> const& input);
}
