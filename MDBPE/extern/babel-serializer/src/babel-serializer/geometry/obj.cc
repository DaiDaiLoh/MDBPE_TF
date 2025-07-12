#include <cstdio>
#include <type_traits> // std::is_same_v

#include <clean-core/char_predicates.hh>
#include <clean-core/from_string.hh>
#include <clean-core/stream_ref.hh>

#include "obj.hh"

/*
 * TODO:
 *  - colors (unofficial extension)
 *  - free-form stuff
 *  - materials
 *  - merging groups
 *  - smoothing groups
 *  - additional error checks (all indices must be valid)
 */

namespace
{
template <class ScalarT>
babel::obj::geometry<ScalarT> read_impl(cc::span<const std::byte> data, babel::obj::read_config const& cfg, babel::error_handler on_error)
{
    using namespace babel;

    obj::geometry<ScalarT> geometry;

    cc::vector<cc::string_view> active_groups = {"default"};
    int group_point_start = 0;
    int group_line_start = 0;
    int group_face_start = 0;

    auto const parse_int = [&](cc::string_view s) -> int
    {
        int i;
        if (cc::from_string(s, i))
        {
            return i;
        }
        else
        {
            on_error(data, cc::as_byte_span(s), "unable to parse int", severity::error);
            return 0;
        }
    };

    auto const parse_float = [&](cc::string_view s) -> ScalarT
    {
        ScalarT f;
        if (cc::from_string(s, f))
        {
            return f;
        }
        else
        {
            on_error(data, cc::as_byte_span(s), "unable to parse float", severity::error);
            return ScalarT(0);
        }
    };

    auto const parse_vertex = [&](cc::string_view line)
    {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(cc::is_space(line[1]));
        tg::pos<4, ScalarT> p;
        p.w = ScalarT(1);
        int i = 0;
        for (auto const s : line.subview(2).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 3)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse vertex", severity::error);
                break;
            }
            p[i++] = parse_float(s);
        }
        geometry.vertices.push_back(p);
    };

    auto const parse_texture_vertex = [&](cc::string_view line)
    {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(line[1] == 't');
        CC_ASSERT(cc::is_space(line[2]));
        tg::pos<3, ScalarT> p;
        p.z = ScalarT(0);
        int i = 0;
        for (auto const s : line.subview(3).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse texture vertex", severity::error);
                break;
            }
            p[i++] = parse_float(s);
        }
        geometry.tex_coords.push_back(p);
    };

    auto const parse_vertex_normal = [&](cc::string_view line)
    {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(line[1] == 'n');
        CC_ASSERT(cc::is_space(line[2]));
        tg::vec<3, ScalarT> p;
        int i = 0;
        for (auto const s : line.subview(3).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse vertex normal", severity::error);
                break;
            }
            p[i++] = parse_float(s);
        }
        geometry.normals.push_back(p);
    };

    auto const parse_parameter_space_vertex = [&](cc::string_view line)
    {
        CC_ASSERT(line[0] == 'v');
        CC_ASSERT(line[1] == 'p');
        CC_ASSERT(cc::is_space(line[2]));
        tg::pos<3, ScalarT> p;
        p.z = ScalarT(1);
        int i = 0;
        for (auto const s : line.subview(3).split(cc::is_space, cc::split_options::skip_empty))
        {
            if (i > 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse free-form vertex", severity::error);
                break;
            }
            p[i++] = parse_float(s);
        }
        geometry.parameters.push_back(p);
    };

    auto const parse_face = [&](cc::string_view line)
    {
        CC_ASSERT(line[0] == 'f');
        CC_ASSERT(cc::is_space(line[1]));
        auto const parse_entry = [&](cc::string_view s)
        {
            typename obj::geometry<ScalarT>::face_entry e;

            int i = 0;
            for (auto const seg : s.split('/', cc::split_options::keep_empty))
            {
                switch (i)
                {
                case 0: // vertex index
                {
                    if (seg.empty())
                    {
                        on_error(data, cc::as_byte_span(line), "unable to parse face entry: missing vertex index", severity::error);
                        return;
                    }
                    auto const idx = parse_int(seg);
                    if (idx < 0) // relative index
                        e.vertex_idx = int(geometry.vertices.size()) + idx;
                    else
                        e.vertex_idx = idx - 1;
                }
                break;
                case 1: // tex_coord_idx
                {
                    if (!seg.empty())
                    {
                        auto const idx = parse_int(seg);
                        if (idx < 0) // relative index
                            e.tex_coord_idx = int(geometry.tex_coords.size()) + idx;
                        else
                            e.tex_coord_idx = idx - 1;
                    }
                }
                break;
                case 2: // normal idx
                {
                    if (!seg.empty())
                    {
                        auto const idx = parse_int(seg);
                        if (idx < 0) // relative index
                            e.normal_idx = int(geometry.normals.size()) + idx;
                        else
                            e.normal_idx = idx - 1;
                    }
                }
                break;
                default:
                {
                    on_error(data, cc::as_byte_span(line), "unable to parse face entry: unknown format", severity::error);
                    return;
                }
                }
                ++i;
            }
            if (i == 0)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse face entry: unknown format", severity::error);
                return;
            }
            geometry.face_entries.push_back(e);
        };

        auto const entries_start = int(geometry.face_entries.size());
        for (auto const entry : line.subview(1).split(cc::is_space, cc::split_options::skip_empty))
        {
            parse_entry(entry);
        }

        auto const entries_count = int(geometry.face_entries.size()) - entries_start;
        if (entries_count == 0)
        {
            on_error(data, cc::as_byte_span(line), "unable to parse face: no face entries found", severity::error);
            return;
        }

        geometry.faces.push_back({entries_start, entries_count});
    };

    auto const parse_point = [&](cc::string_view s)
    {
        CC_ASSERT(s[0] == 'p');
        CC_ASSERT(cc::is_space(s[1]));
        auto const points = s.subview(2);
        for (auto const sp : points.split(cc::is_space, cc::split_options::skip_empty))
        {
            auto const idx = parse_int(sp);
            if (idx < 0) // relative index
                geometry.points.push_back({int(geometry.normals.size()) + idx});
            else
                geometry.points.push_back({idx - 1});
        }
    };

    auto const parse_line = [&](cc::string_view s)
    {
        CC_ASSERT(s[0] == 'l');
        CC_ASSERT(cc::is_space(s[1]));
        auto const segments = s.subview(2);
        auto const entries_start = int(geometry.line_entries.size());

        for (auto const seg : segments.split(cc::is_space, cc::split_options::skip_empty))
        {
            typename babel::obj::geometry<ScalarT>::line_entry e;
            int i = 0;
            for (auto const idx : seg.split('/', cc::split_options::keep_empty))
            {
                switch (i)
                {
                case 0:
                {
                    if (idx.empty())
                    {
                        on_error(data, cc::as_byte_span(s), "unable to parse line entry: missing vertex index", severity::error);
                        return;
                    }
                    auto const vertex_idx = parse_int(idx);
                    if (vertex_idx < 0)
                        e.vertex_idx = int(geometry.vertices.size()) + vertex_idx;
                    else
                        e.vertex_idx = vertex_idx - 1;
                }
                break;
                case 1:
                {
                    if (!idx.empty())
                    {
                        auto const texture_idx = parse_int(idx);
                        if (texture_idx < 0)
                            e.tex_coord_idx = int(geometry.vertices.size()) + texture_idx;
                        else
                            e.tex_coord_idx = texture_idx - 1;
                    }
                }
                break;
                default:
                {
                    on_error(data, cc::as_byte_span(s), "unable to parse line: unknown line segment format", severity::error);
                    return;
                }
                }
                ++i;
            }
            geometry.line_entries.push_back(e);
        }

        auto const entries_count = int(geometry.line_entries.size()) - entries_start;
        if (entries_count < 2)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse line: a line segment must contain at least two points", severity::error);
            return;
        }
        geometry.lines.push_back({entries_start, entries_count});
    };

    auto const handle_previous_groups = [&]()
    {
        auto const points_count = int(geometry.points.size()) - group_point_start;
        auto const lines_count = int(geometry.lines.size()) - group_line_start;
        auto const faces_count = int(geometry.faces.size()) - group_face_start;

        typename babel::obj::geometry<ScalarT>::group g;
        if (points_count > 0)
        {
            g.points_start = group_point_start;
            g.points_count = points_count;
        }
        if (lines_count > 0)
        {
            g.lines_start = group_line_start;
            g.lines_count = lines_count;
        }
        if (faces_count > 0)
        {
            g.faces_start = group_face_start;
            g.faces_count = faces_count;
        }
        if (points_count > 0 || lines_count > 0 || faces_count > 0)
            for (auto const& group : active_groups)
            {
                g.name = group;
                geometry.groups.push_back(g);
            }
    };

    auto const parse_groups = [&](cc::string_view s)
    {
        CC_ASSERT(s[0] == 'g');
        CC_ASSERT(cc::is_space(s[1]));

        handle_previous_groups();
        group_point_start = int(geometry.points.size());
        group_line_start = int(geometry.lines.size());
        group_face_start = int(geometry.faces.size());

        // now parse
        active_groups.clear();
        for (auto const g : s.subview(2).split(cc::is_space, cc::split_options::skip_empty))
            active_groups.push_back(g);
    };

    auto const data_as_string_view = cc::string_view(reinterpret_cast<char const*>(data.data()), reinterpret_cast<char const*>(data.data() + data.size()));
    for (auto line : data_as_string_view.split('\n', cc::split_options::skip_empty))
    {
        line = *line.split('#').begin(); // remove comments
        line = line.trim();

        if (line.empty() || line.starts_with('#'))
            continue;

        if (line[0] == 'v')
        {
            if (line.size() < 2)
            {
                on_error(data, cc::as_byte_span(line), "unable to parse line: starts with v but does not contain any vertex information", severity::error);
                continue;
            }
            if (cc::is_blank(line[1]))
                parse_vertex(line);
            else
            {
                if (line.size() < 3)
                {
                    on_error(data, cc::as_byte_span(line), "unable to parse line: starts with v but does not contain any vertex information", severity::error);
                    continue;
                }
                if (line[1] == 't' && cc::is_blank(line[2]))
                    parse_texture_vertex(line);
                else if (line[1] == 'n' && cc::is_blank(line[2]))
                    parse_vertex_normal(line);
                else if (line[1] == 'p' && cc::is_blank(line[2]))
                    parse_parameter_space_vertex(line);
                else if (cfg.add_unrecognized_lines)
                    geometry.unrecognized_lines.push_back(line);
            }
        }
        else if (line[0] == 'f' && line.size() >= 2 && cc::is_blank(line[1]))
            parse_face(line);
        else if (line[0] == 'p' && line.size() >= 2 && cc::is_blank(line[1]))
            parse_point(line);
        else if (line[0] == 'l' && line.size() >= 2 && cc::is_blank(line[1]))
            parse_line(line);
        else if (line[0] == 'g' && line.size() >= 2 && cc::is_blank(line[1]) && cfg.parse_groups)
            parse_groups(line);
        else if (cfg.add_unrecognized_lines)
            geometry.unrecognized_lines.push_back(line);
    }

    if (cfg.parse_groups)
        handle_previous_groups(); // the last active groups

    return geometry;
}
}

babel::obj::geometry<float> babel::obj::read(cc::span<const std::byte> data, babel::obj::read_config const& cfg, babel::error_handler on_error)
{
    return read_impl<float>(data, cfg, on_error);
}

babel::obj::geometry<double> babel::obj::read_double(cc::span<const std::byte> data, babel::obj::read_config const& cfg, babel::error_handler on_error)
{
    return read_impl<double>(data, cfg, on_error);
}

bool babel::obj::write_simple(cc::stream_ref<std::byte> output, cc::span<tg::pos3 const> vertices, cc::span<tg::ipos3 const> triangles)
{
    char buf[256] = {};

#define BABEL_PRINT_LINE(...)                                                                         \
    do                                                                                                \
    {                                                                                                 \
        int const numChars = std::snprintf(buf, sizeof(buf), __VA_ARGS__);                            \
        output << cc::span<std::byte const>(reinterpret_cast<std::byte*>(&buf[0]), size_t(numChars)); \
    } while (0)

    BABEL_PRINT_LINE("# Babel simple OBJ Exporter\n\n#\n# object Babel_simpleObj\n#\n\n");

    for (auto const& v : vertices)
    {
        BABEL_PRINT_LINE("v  %f %f %f\n", v.x, v.y, v.z);
    }

    BABEL_PRINT_LINE("# %d vertices\n\no Babel_simpleObj\ng Babel_simpleObj\n", (int)vertices.size());

    for (auto const& t : triangles)
    {
        BABEL_PRINT_LINE("f %d %d %d \n", t.x + 1, t.y + 1, t.z + 1);
    }

    BABEL_PRINT_LINE("# %d polygons - %d triangles\n\n", (int)triangles.size(), (int)triangles.size());

#undef BABEL_PRINT_LINE

    return true;
}
