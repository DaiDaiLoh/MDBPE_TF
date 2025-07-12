#pragma once

#include <clean-core/string_view.hh>

#include <image/image.hh>

namespace img
{
image<tg::color3> read_rgb_from_file(cc::string_view filepath);

void write(rgb_image const& input, cc::string_view filepath);
void write(grayscale_image const& input, cc::string_view filepath);
void write(binary_image const& input, cc::string_view filepath);
}
