#include "io.hh"

#include <clean-core/string_stream.hh>
#include <clean-core/to_string.hh>

#include <typed-geometry/functions/basic/scalar_math.hh>

#include <babel-serializer/file.hh>
#include <babel-serializer/image/image.hh>

#include <rich-log/log.hh>

img::image<tg::color3> img::read_rgb_from_file(cc::string_view filepath)
{
    babel::image::read_config cfg;
    cfg.desired_bit_depth = babel::image::bit_depth::u8;
    cfg.desired_channels = babel::image::channels::rgb;

    auto const data = babel::image::read(babel::file::read_all_bytes(filepath), cfg);
    img::image<tg::color3> image(tg::isize2(data.width, data.height));
    for (auto y = 0; y < data.height; ++y)
        for (auto x = 0; x < data.width; ++x)
        {
            auto const r = float(data.bytes[(x + y * data.width) * 3 + 0]) / 255.0f;
            auto const g = float(data.bytes[(x + y * data.width) * 3 + 1]) / 255.0f;
            auto const b = float(data.bytes[(x + y * data.width) * 3 + 2]) / 255.0f;
            image(x, y) = {r, g, b};
        }
    return image;
}

// todo: make the below two a template, this is awful code duplication

void img::write(rgb_image const& input, cc::string_view filepath)
{
    babel::file::file_output_stream file(filepath);

    babel::image::data data;
    data.bit_depth = babel::image::bit_depth::u8;
    data.channels = babel::image::channels::rgb;
    data.width = input.width();
    data.height = input.height();
    data.bytes.resize(input.height() * input.width() * 3 * sizeof(std::byte));
    for (auto y = 0; y < data.height; ++y)
        for (auto x = 0; x < data.width; ++x)
        {
            auto color = input(x, y);
            if (color.r < 0.0f || 1.0f < color.r)
            {
                LOG_WARN("rgb color outside of [0, 1]: Red channel value: {}", color.r);
                color.r = tg::clamp(color.r, 0.0f, 1.0f);
            }
            if (color.g < 0.0f || 1.0f < color.g)
            {
                LOG_WARN("rgb color outside of [0, 1]: Green channel value: {}", color.g);
                color.g = tg::clamp(color.g, 0.0f, 1.0f);
            }
            if (color.b < 0.0f || 1.0f < color.b)
            {
                LOG_WARN("rgb color outside of [0, 1]: Blue channel value: {}", color.b);
                color.b = tg::clamp(color.b, 0.0f, 1.0f);
            }
            data.bytes[3 * (x + y * data.width) + 0] = std::byte(color.r * 255);
            data.bytes[3 * (x + y * data.width) + 1] = std::byte(color.g * 255);
            data.bytes[3 * (x + y * data.width) + 2] = std::byte(color.b * 255);
        }

    babel::image::write_config cfg;

    cfg.format = filepath.subview(filepath.last_index_of('.') + 1);

    babel::image::write(file, data, cfg);
}

void img::write(grayscale_image const& input, cc::string_view filepath)
{
    babel::file::file_output_stream file(filepath);

    babel::image::data data;
    data.bit_depth = babel::image::bit_depth::u8;
    data.channels = babel::image::channels::grey;
    data.width = input.width();
    data.height = input.height();
    data.bytes.resize(input.height() * input.width() * sizeof(std::byte));
    for (auto y = 0; y < data.height; ++y)
        for (auto x = 0; x < data.width; ++x)
            data.bytes[x + y * data.width] = std::byte(input(x, y) * 255);

    babel::image::write_config cfg;
    cfg.format = filepath.subview(filepath.last_index_of('.') + 1);

    babel::image::write(file, data, cfg);
}

void img::write(binary_image const& input, cc::string_view filepath)
{
    babel::file::file_output_stream file(filepath);

    babel::image::data data;
    data.bit_depth = babel::image::bit_depth::u8;
    data.channels = babel::image::channels::grey;
    data.width = input.width();
    data.height = input.height();
    data.bytes.resize(input.height() * input.width() * sizeof(std::byte));
    for (auto y = 0; y < data.height; ++y)
        for (auto x = 0; x < data.width; ++x)
            data.bytes[x + y * data.width] = input(x, y) ? std::byte(255) : std::byte(0);

    babel::image::write_config cfg;
    cfg.format = "jpg";

    babel::image::write(file, data, cfg);
}
