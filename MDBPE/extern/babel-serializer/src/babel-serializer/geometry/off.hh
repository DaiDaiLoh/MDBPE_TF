#pragma once

#include <cstddef> // std::byte

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <babel-serializer/errors.hh>

// http://www.geomview.org/docs/html/OFF.html
namespace babel::off
{
struct read_config
{
};
struct geometry
{
    // a face consists of a set of
    struct face
    {
        int vertices_start = -1;
        int vertices_count = -1;
    };

    cc::vector<tg::pos4> vertices;
    cc::vector<int> face_vertices;
    cc::vector<face> faces;

    // arrays parallel to vertices
    cc::vector<tg::vec3> normals;
    cc::vector<tg::pos2> tex_coords;
    cc::vector<tg::color4> vertex_colors;

    // array parallel to faces
    cc::vector<tg::color4> face_colors;
};
geometry read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);
}
