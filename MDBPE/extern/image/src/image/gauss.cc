#include "gauss.hh"

#include <typed-geometry/tg.hh>

#include <rich-log/log.hh>

img::grayscale_image img::gauss(grayscale_image const& input)
{
    float coefficients[] = {0.0545f, 0.2442f, 0.4026f, 0.2442f, 0.0545f};
    auto const reflect = [&](int v, int dimension_size)
    {
        if (v < 0)
            return -v - 1;
        if (v >= dimension_size)
            return 2 * dimension_size - v - 1;
        return v;
    };

    auto const get = [&](int x, int y, img::grayscale_image const& from)
    {
        auto const xx = reflect(x, input.width());
        auto const yy = reflect(y, input.height());
        return from(xx, yy);
    };

    img::grayscale_image tmp(input.width(), input.height());
    img::grayscale_image output(input.width(), input.height());

    // x direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            tmp(x, y) = coefficients[0] * get(x - 2, y, input);
            tmp(x, y) += coefficients[1] * get(x - 1, y, input);
            tmp(x, y) += coefficients[2] * get(x, y, input);
            tmp(x, y) += coefficients[3] * get(x + 1, y, input);
            tmp(x, y) += coefficients[4] * get(x + 2, y, input);
        }

        // y direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            output(x, y) = coefficients[0] * get(x, y - 2, tmp);
            output(x, y) += coefficients[1] * get(x, y - 1, tmp);
            output(x, y) += coefficients[2] * get(x, y, tmp);
            output(x, y) += coefficients[3] * get(x, y + 1, tmp);
            output(x, y) += coefficients[4] * get(x, y + 2, tmp);
        }
    return output;
}

img::rgb_image img::gauss(rgb_image const& input)
{
    float coefficients[] = {0.0545f, 0.2442f, 0.4026f, 0.2442f, 0.0545f};
    auto const reflect = [&](int v, int dimension_size)
    {
        if (v < 0)
            return -v - 1;
        if (v >= dimension_size)
            return 2 * dimension_size - v - 1;
        return v;
    };

    auto const get = [&](int x, int y, img::rgb_image const& from)
    {
        auto const xx = reflect(x, input.width());
        auto const yy = reflect(y, input.height());
        return from(xx, yy);
    };

    img::rgb_image tmp(input.width(), input.height());
    img::rgb_image output(input.width(), input.height());

    // x direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            tmp(x, y) = tg::color3(coefficients[0] * tg::comp(get(x - 2, y, input)));
            tmp(x, y) += coefficients[1] * tg::comp3(get(x - 1, y, input));
            tmp(x, y) += coefficients[2] * tg::comp3(get(x, y, input));
            tmp(x, y) += coefficients[3] * tg::comp3(get(x + 1, y, input));
            tmp(x, y) += coefficients[4] * tg::comp3(get(x + 2, y, input));
        }

        // y direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            output(x, y) = tg::color3(coefficients[0] * tg::comp3(get(x, y - 2, tmp)));
            output(x, y) += coefficients[1] * tg::comp3(get(x, y - 1, tmp));
            output(x, y) += coefficients[2] * tg::comp3(get(x, y, tmp));
            output(x, y) += coefficients[3] * tg::comp3(get(x, y + 1, tmp));
            output(x, y) += coefficients[4] * tg::comp3(get(x, y + 2, tmp));
        }
    return output;
}

img::rgb_image img::gauss(rgb_image const& input, binary_image const& mask)
{
    float coefficients[] = {0.0545f, 0.2442f, 0.4026f, 0.2442f, 0.0545f};
    auto const reflect = [&](int v, int dimension_size)
    {
        if (v < 0)
            return -v - 1;
        if (v >= dimension_size)
            return 2 * dimension_size - v - 1;
        return v;
    };

    auto const get = [&](int x, int y, img::rgb_image const& from)
    {
        auto const xx = reflect(x, input.width());
        auto const yy = reflect(y, input.height());
        return from(xx, yy);
    };

    img::rgb_image tmp(input.width(), input.height());
    img::rgb_image output(input.width(), input.height());

    // x direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            if (mask(x, y))
            {
                tmp(x, y) = tg::color3(coefficients[0] * tg::comp(get(x - 2, y, input)));
                tmp(x, y) += coefficients[1] * tg::comp3(get(x - 1, y, input));
                tmp(x, y) += coefficients[2] * tg::comp3(get(x, y, input));
                tmp(x, y) += coefficients[3] * tg::comp3(get(x + 1, y, input));
                tmp(x, y) += coefficients[4] * tg::comp3(get(x + 2, y, input));
            }
            else
            {
                tmp(x, y) = input(x, y);
            }
        }

        // y direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            if (mask(x, y))
            {
                output(x, y) = tg::color3(coefficients[0] * tg::comp3(get(x, y - 2, tmp)));
                output(x, y) += coefficients[1] * tg::comp3(get(x, y - 1, tmp));
                output(x, y) += coefficients[2] * tg::comp3(get(x, y, tmp));
                output(x, y) += coefficients[3] * tg::comp3(get(x, y + 1, tmp));
                output(x, y) += coefficients[4] * tg::comp3(get(x, y + 2, tmp));
            }
            else
            {
                output(x, y) = tmp(x, y);
            }
        }
    return output;
}

img::image<tg::dir2> img::gauss(image<tg::dir2> const& input)
{
    float coefficients[] = {0.0545f, 0.2442f, 0.4026f, 0.2442f, 0.0545f};
    auto const reflect = [&](int v, int dimension_size)
    {
        if (v < 0)
            return -v - 1;
        if (v >= dimension_size)
            return 2 * dimension_size - v - 1;
        return v;
    };

    auto const get = [&](int x, int y, auto const& from)
    {
        auto const xx = reflect(x, input.width());
        auto const yy = reflect(y, input.height());
        return from(xx, yy);
    };

    img::image<tg::vec2> tmp(input.width(), input.height());
    img::image<tg::vec2> output(input.width(), input.height());

    // x direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            tmp(x, y) = coefficients[0] * get(x - 2, y, input);
            tmp(x, y) += coefficients[1] * get(x - 1, y, input);
            tmp(x, y) += coefficients[2] * get(x, y, input);
            tmp(x, y) += coefficients[3] * get(x + 1, y, input);
            tmp(x, y) += coefficients[4] * get(x + 2, y, input);
        }

        // y direction
#pragma omp parallel for
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            output(x, y) = coefficients[0] * get(x, y - 2, tmp);
            output(x, y) += coefficients[1] * get(x, y - 1, tmp);
            output(x, y) += coefficients[2] * get(x, y, tmp);
            output(x, y) += coefficients[3] * get(x, y + 1, tmp);
            output(x, y) += coefficients[4] * get(x, y + 2, tmp);
        }

    img::image<tg::dir2> normalized_output(input.extents());
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
            normalized_output(x, y) = normalize(output(x, y));

    return normalized_output;
}
