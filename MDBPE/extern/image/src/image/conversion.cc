#include "conversion.hh"

img::grayscale_image img::to_grayscale(rgb_image const& input)
{
    grayscale_image result(input.width(), input.height());
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            auto c = input(x, y);
            result(x, y) = (c.r + c.g + c.b) / 3;
        }
    return result;
}

img::rgb_image img::to_rgb(grayscale_image const& input)
{
    rgb_image result(input.extents());
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            auto c = input(x, y);
            result(x, y) = {c, c, c};
        }
    return result;
}
