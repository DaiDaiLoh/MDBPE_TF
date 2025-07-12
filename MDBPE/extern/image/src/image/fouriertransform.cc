#if defined(IMG_HAS_FFTW_CPP)

#include "fouriertransform.hh"

#include <typed-geometry/tg.hh>

#include <fftw/dft.hh>

img::grayscale_image img::to_fourier(grayscale_image const& input)
{
    fftw::matrix_2d_real input_data(input.width(), input.height());

    for (auto x = 0; x < input.width(); ++x)
        for (auto y = 0; y < input.height(); ++y)
            input_data.set(x, y, input(x, y));

    auto transformed = fftw::discrete_fourier_transform(input_data);

    transformed.set(0, 0, {0.0, 0.0});

    auto max_value = 0.0f;
    img::grayscale_image image(input.width(), input.height());
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
        {
            auto [re, im] = transformed(x, y);
            auto value = tg::length(tg::vec2(re, im));
            //            value = tg::log(value);

            if (value > max_value)
                max_value = value;

            image(x, y) = value;
        }
    for (auto y = 0; y < input.height(); ++y)
        for (auto x = 0; x < input.width(); ++x)
            image[{x, y}] /= max_value;

    return image;
}

#endif
