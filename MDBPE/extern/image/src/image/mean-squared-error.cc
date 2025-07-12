#include "mean-squared-error.hh"

#include <rich-log/log.hh>

#include <typed-geometry/tg.hh>

float img::mean_squared_error(rgb_image const& image_a, rgb_image const& image_b)
{
    CC_ASSERT(image_a.extents() == image_b.extents() && "images must have same dimensions");
    auto const total_pixel = image_a.width() * image_a.height();
    double error = 0.0;
#pragma omp parallel for reduction(+ : error)
    for (auto y = 0; y < image_a.height(); ++y)
        for (auto x = 0; x < image_a.width(); ++x)
        {
            auto const a = tg::pos3(image_a(x, y)) * 255;
            auto const b = tg::pos3(image_b(x, y)) * 255;
            error += tg::distance_sqr(a, b);
        }
    return error / (3 * total_pixel);
}
