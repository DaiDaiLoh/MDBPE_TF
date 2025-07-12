#include "normalize.hh"

#include <clean-core/span.hh>

#include <typed-geometry/tg.hh>

void img::normalize(grayscale_image& image)
{
    auto data_span = cc::span(image.data_ptr(), image.data_size());
    auto min_element = tg::min_element(data_span);
    auto max_element = tg::max_element(data_span);
    auto range = max_element - min_element;
    for (auto& element : data_span)
        element = (element - min_element) / range;
}

void img::normalize(rgb_image& image)
{
    auto data_span = cc::span(image.data_ptr(), image.data_size());
    float min_value = tg::max<float>();
    float max_value = -tg::max<float>();
    for (auto const& c : data_span)
    {
        if (c.r < min_value)
            min_value = c.r;
        if (c.g < min_value)
            min_value = c.g;
        if (c.b < min_value)
            min_value = c.b;

        if (max_value < c.r)
            max_value = c.r;
        if (max_value < c.g)
            max_value = c.g;
        if (max_value < c.b)
            max_value = c.b;
    }

    auto range = max_value - min_value;
    for (auto& c : data_span)
    {
        c.r = (c.r - min_value) / range;
        c.g = (c.g - min_value) / range;
        c.b = (c.b - min_value) / range;
    }
}
