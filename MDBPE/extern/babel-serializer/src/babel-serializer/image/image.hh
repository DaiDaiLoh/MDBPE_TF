#pragma once

#include <clean-core/array.hh>
#include <clean-core/stream_ref.hh>

#include <babel-serializer/errors.hh>

// general-purpose image loading
// unified but simplified interface for many image types
// NOTE: currently does NOT support images above 2GB (stb limitation)
namespace babel::image
{
enum class channels
{
    invalid = 0,

    grey = 1,
    grey_alpha = 2,
    rgb = 3,
    rgb_alpha = 4,
};

enum class bit_depth
{
    invalid,

    u8,
    u16,
    f32
};

struct read_config
{
    // TODO: format hint?

    /// if not invalid, converts the result to the given channel number
    channels desired_channels = channels::invalid;

    /// if not invalid, converts the result to the given bit depth
    bit_depth desired_bit_depth = bit_depth::invalid;
};

struct write_config
{
    // TODO: compression settings

    /// desired format (case INsensitive)
    /// currently supported: png, bmp, tga, jpg, hdr
    cc::string_view format;

    /// if true, flips the image vertically on write
    bool flip_vertically = false;

    /// quality parameter if jpg is used
    int jpg_quality = 90;
};

struct data_header
{
    image::channels channels = channels::invalid;
    image::bit_depth bit_depth = bit_depth::invalid;
    int width = 1;
    int height = 1;
    int depth = 1;

    bool is_valid() const { return channels != channels::invalid; }
};

/// NOTE: the data must be in "natural stride", row-by-row with no padding
struct data : data_header
{
    // header via inheritance
    cc::array<std::byte> bytes;
};

/// reads an image from memory
data read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);

/// writes an image to a bytestream
/// returns true on success
bool write(cc::stream_ref<std::byte> output, data const& img, write_config const& cfg, error_handler on_error = default_error_handler);
bool write(cc::stream_ref<std::byte> output, data_header const& img, cc::span<std::byte const> data, write_config const& cfg, error_handler on_error = default_error_handler);
}
