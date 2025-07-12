#include "image.hh"

#include <clean-core/assertf.hh>
#include <clean-core/string.hh>

#include <babel-serializer/detail/stb_image.hh>
#include <babel-serializer/detail/stb_image_write.hh>

static int bit_depth_byte_size(babel::image::bit_depth d)
{
    switch (d)
    {
    case babel::image::bit_depth::u8:
        return 1;
    case babel::image::bit_depth::u16:
        return 2;
    case babel::image::bit_depth::f32:
        return 4;
    default:
        CC_UNREACHABLE("unsupported bit depth");
    }
}

babel::image::data babel::image::read(cc::span<const std::byte> data, read_config const& cfg, error_handler on_error)
{
    babel::image::data d;

    // stb types
    auto buffer_data = (babel_stbi_uc*)(data.data());
    auto buffer_size = int(data.size());

    // fixed bit depth
    if (cfg.desired_bit_depth != bit_depth::invalid)
        d.bit_depth = cfg.desired_bit_depth;
    else if (babel_stbi_is_hdr_from_memory(buffer_data, buffer_size))
        d.bit_depth = bit_depth::f32;
    else if (babel_stbi_is_16_bit_from_memory(buffer_data, buffer_size))
        d.bit_depth = bit_depth::u16;
    else
        d.bit_depth = bit_depth::u8;

    // load
    int w, h;
    int stored_channels;
    std::byte* ptr;
    switch (d.bit_depth)
    {
    case bit_depth::u8:
        ptr = reinterpret_cast<std::byte*>(babel_stbi_load_from_memory(buffer_data, buffer_size, &w, &h, &stored_channels, int(cfg.desired_channels)));
        break;
    case bit_depth::u16:
        ptr = reinterpret_cast<std::byte*>(babel_stbi_load_16_from_memory(buffer_data, buffer_size, &w, &h, &stored_channels, int(cfg.desired_channels)));
        break;
    case bit_depth::f32:
        ptr = reinterpret_cast<std::byte*>(babel_stbi_loadf_from_memory(buffer_data, buffer_size, &w, &h, &stored_channels, int(cfg.desired_channels)));
        break;
    default:
        CC_UNREACHABLE("unsupported bit depth");
    }

    // channels
    d.channels = cfg.desired_channels != channels::invalid ? cfg.desired_channels : channels(stored_channels);

    if (ptr)
    {
        d.width = w;
        d.height = h;
        d.depth = 1;
        d.bytes = cc::array<std::byte>::uninitialized(w * h * int(d.channels) * bit_depth_byte_size(d.bit_depth));
        std::memcpy(d.bytes.data(), ptr, d.bytes.size());
        babel_stbi_image_free(ptr);
    }
    else // error
    {
        on_error(data, data, babel_stbi_failure_reason(), severity::error);
    }

    return d;
}

bool babel::image::write(cc::stream_ref<std::byte> output,
                         babel::image::data_header const& img,
                         cc::span<const std::byte> data,
                         babel::image::write_config const& cfg,
                         babel::error_handler on_error)
{
    CC_ASSERT(!cfg.format.empty() && "must provide a format");
    CC_ASSERT(img.is_valid());

    auto channel_cnt = int(img.channels);

    auto expected_size = img.width * img.height * img.depth * bit_depth_byte_size(img.bit_depth) * channel_cnt;
    CC_ASSERTF(expected_size == int(data.size()), "image data size is not what is expected (expected {}, got {})", expected_size, data.size());

    auto write_func = +[](void* ctx, void* data, int size)
    { *reinterpret_cast<cc::stream_ref<std::byte>*>(ctx) << cc::span<std::byte const>(reinterpret_cast<std::byte const*>(data), size); };

    CC_ASSERT(img.depth == 1 && "3D / layered images not supported");

    auto fmt = cc::string(cfg.format).to_lower();
    if (cfg.format == "png")
    {
        CC_ASSERT(img.bit_depth == bit_depth::u8 && "currently only 8bit supported");
        if (!babel_stbi_write_png_to_func(write_func, &output, img.width, img.height, channel_cnt, data.data(),
                                          channel_cnt * img.width * bit_depth_byte_size(img.bit_depth)))
        {
            on_error({}, {}, babel_stbi_failure_reason(), severity::error);
            return false;
        }
    }
    else if (cfg.format == "bmp")
    {
        CC_ASSERT(img.bit_depth == bit_depth::u8 && "currently only 8bit supported");
        if (!babel_stbi_write_bmp_to_func(write_func, &output, img.width, img.height, channel_cnt, data.data()))
        {
            on_error({}, {}, babel_stbi_failure_reason(), severity::error);
            return false;
        }
    }
    else if (cfg.format == "tga")
    {
        CC_ASSERT(img.bit_depth == bit_depth::u8 && "currently only 8bit supported");
        if (!babel_stbi_write_tga_to_func(write_func, &output, img.width, img.height, channel_cnt, data.data()))
        {
            on_error({}, {}, babel_stbi_failure_reason(), severity::error);
            return false;
        }
    }
    else if (cfg.format == "jpg")
    {
        CC_ASSERT(img.bit_depth == bit_depth::u8 && "currently only 8bit supported");
        if (!babel_stbi_write_jpg_to_func(write_func, &output, img.width, img.height, channel_cnt, data.data(), cfg.jpg_quality))
        {
            on_error({}, {}, babel_stbi_failure_reason(), severity::error);
            return false;
        }
    }
    else if (cfg.format == "hdr")
    {
        CC_ASSERT(img.bit_depth == bit_depth::f32 && "currently only 32bit float supported");
        if (!babel_stbi_write_hdr_to_func(write_func, &output, img.width, img.height, channel_cnt, reinterpret_cast<float const*>(data.data())))
        {
            on_error({}, {}, babel_stbi_failure_reason(), severity::error);
            return false;
        }
    }
    else
    {
        CC_ASSERTF(false, "unsupported format {}", cfg.format);
    }

    return true;
}

bool babel::image::write(cc::stream_ref<std::byte> output, data const& img, write_config const& cfg, error_handler on_error)
{
    return write(output, img, img.bytes, cfg, on_error);
}
