#include "gradient.hh"

#include <rich-log/log.hh>

#include <typed-geometry/functions/basic/scalar_math.hh>

#include <typed-geometry/functions/vector/dot.hh>

#include <typed-geometry/functions/vector/length.hh>

img::image<tg::vec2> img::gradient_of(grayscale_image const& input)
{
    image<tg::vec2> result(input.width(), input.height());

    auto const reflect = [&](int v, int dimension_size)
    {
        if (v < 0)
            return -v - 1;
        if (v >= dimension_size)
            return 2 * dimension_size - v - 1;
        return v;
    };

    auto const get = [&](int x, int y)
    {
        auto const xx = reflect(x, input.width());
        auto const yy = reflect(y, input.height());
        return input(xx, yy);
    };

    // todo: can do x and y in one step

    // x direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            auto value = 0.0f;

            value += get(x - 1, y + 1);
            value += 2 * get(x - 1, y);
            value += get(x - 1, y - 1);

            value -= get(x + 1, y + 1);
            value -= 2 * get(x + 1, y);
            value -= get(x + 1, y - 1);

            result(x, y).x = value;
        }

// y directoin
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            auto value = 0.0f;
            value += get(x + 1, y - 1);
            value += 2 * get(x, y - 1);
            value += get(x - 1, y - 1);

            value -= get(x + 1, y + 1);
            value -= 2 * get(x, y + 1);
            value -= get(x - 1, y + 1);

            result(x, y).y = value;
        }
    return result;
}

img::image<tg::angle32> img::angles_of(image<tg::vec2> const& gradient_image)
{
    image<tg::angle32> result(gradient_image.width(), gradient_image.height());

#pragma omp parallel for
    for (auto y = 0; y < gradient_image.height(); ++y)
        for (auto x = 0; x < gradient_image.width(); ++x)
        {
            auto g = gradient_image(x, y);
            result(x, y) = tg::atan2(g.y, g.x);
        }
    return result;
}

img::grayscale_image img::amplitudes_of(image<tg::vec2> const& gradient_image)
{
    img::grayscale_image output(gradient_image.width(), gradient_image.height());

#pragma omp parallel for
    for (auto y = 0; y < gradient_image.height(); ++y)
        for (auto x = 0; x < gradient_image.width(); ++x)
        {
            auto g = gradient_image(x, y);
            auto l = length(g);
            output(x, y) = l;
        }

    return output;
}
