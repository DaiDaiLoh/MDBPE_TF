#if defined(IMG_HAS_FFTW_CPP)

#pragma once

#include <image/image.hh>

namespace img
{
grayscale_image to_fourier(grayscale_image const& input);
}

#endif
