#pragma once

#include <cstddef> // std::byte

#include <clean-core/map.hh>
#include <clean-core/string.hh>
#include <clean-core/string_view.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <babel-serializer/errors.hh>

namespace babel::obj
{
struct read_config
{
    cc::string base_path; // for resolving mtls
    bool resolve_materials = true;

    bool parse_groups = true;
    bool add_unrecognized_lines = true;
};

// see http://paulbourke.net/dataformats/obj/
template <class ScalarT>
struct geometry
{
    using vec3_t = tg::vec<3, ScalarT>;
    using pos3_t = tg::pos<3, ScalarT>;
    using pos4_t = tg::pos<4, ScalarT>;

    struct face
    {
        int entries_start = 0;
        int entries_count = 0;
    };

    struct face_entry
    {
        int vertex_idx = -1;
        int tex_coord_idx = -1;
        int normal_idx = -1;
    };

    struct line
    {
        int entries_start = 0;
        int entries_count = 0;
    };

    struct line_entry
    {
        int vertex_idx = -1;
        int tex_coord_idx = -1;
    };

    struct point
    {
        int vertex_idx = -1;
    };

    struct group
    {
        cc::string name;

        int faces_start = 0;
        int faces_count = 0;

        int lines_start = 0;
        int lines_count = 0;

        int points_start = 0;
        int points_count = 0;

        // ... free form surfaces
        // also: can vertices be part of groups?
    };

    // vertex data
    cc::vector<pos4_t> vertices;
    cc::vector<pos3_t> tex_coords;
    cc::vector<vec3_t> normals;
    cc::vector<pos3_t> parameters;

    // elements
    cc::vector<group> groups;
    cc::vector<face> faces;
    cc::vector<line> lines;
    cc::vector<point> points;
    cc::vector<face_entry> face_entries;
    cc::vector<line_entry> line_entries;

    cc::vector<cc::string> unrecognized_lines;
};

geometry<float> read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);
geometry<double> read_double(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);

bool write_simple(cc::stream_ref<std::byte> output, cc::span<tg::pos3 const> vertices, cc::span<tg::ipos3 const> triangles);

/// creates a flat list of triangles for a given obj
/// NOTE: asserts that all faces are tris
template <class T>
[[nodiscard]] cc::vector<tg::triangle<3, T>> to_triangles(geometry<T> const& obj)
{
    cc::vector<tg::triangle<3, T>> res;
    for (auto f : obj.faces)
    {
        CC_ASSERT(f.entries_count == 3);
        auto i0 = obj.face_entries[f.entries_start + 0].vertex_idx;
        auto i1 = obj.face_entries[f.entries_start + 1].vertex_idx;
        auto i2 = obj.face_entries[f.entries_start + 2].vertex_idx;
        auto v0 = (tg::pos3)obj.vertices[i0];
        auto v1 = (tg::pos3)obj.vertices[i1];
        auto v2 = (tg::pos3)obj.vertices[i2];
        res.emplace_back(v0, v1, v2);
    }
    return res;
}
}
