#pragma once

#include <cstddef> // std::byte
#include <cstdint>

#include <clean-core/span.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <babel-serializer/errors.hh>

namespace babel::stl
{
enum class file_type
{
    autodetect,
    binary,
    ascii
};
struct read_config
{
    babel::stl::file_type file_type = file_type::autodetect;
    // Iff true, the reader will emit a warning, if any position or normal values exceed float range but fit into a double.
    // The result will nevertheless be rounded to float.
    bool warn_on_double_values = true;
};

/// See: https://en.wikipedia.org/wiki/STL_(file_format)
/// We accept colors in VisCAM and SolidView, as well as Materialise Magics format
struct geometry
{
    /// right-hand-rule triangles with normal information
    struct triangle
    {
        tg::pos3 v0;
        tg::pos3 v1;
        tg::pos3 v2;
        tg::vec3 normal;
    };

    struct color
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };

    /// The name is optional and always empty for binary STL files
    cc::string name;
    cc::vector<triangle> triangles;
    cc::vector<color> triangle_color;
};

geometry read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);
}
