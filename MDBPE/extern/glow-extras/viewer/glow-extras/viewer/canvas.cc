#include "canvas.hh"

#include <string>

#include <glow-extras/colors/color.hh>

// TODO: more performant from_hex

void glow::viewer::canvas_data::set_color(std::string_view hex)
{
    auto c = glow::colors::color::from_hex(std::string(hex));
    set_color(c.r, c.g, c.b, c.a);
}

glow::viewer::material::material(std::string_view color_str) : type(material_type::diffuse)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    color = {c.r, c.g, c.b, c.a};
}

glow::viewer::material::material(const char* color_str) : type(material_type::diffuse)
{
    auto c = glow::colors::color::from_hex(color_str);
    color = {c.r, c.g, c.b, c.a};
}

glow::viewer::canvas_t::point_ref& glow::viewer::canvas_t::point_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

glow::viewer::canvas_t::splat_ref& glow::viewer::canvas_t::splat_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

glow::viewer::canvas_t::line_ref& glow::viewer::canvas_t::line_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

glow::viewer::canvas_t::triangle_ref& glow::viewer::canvas_t::triangle_ref::color(std::string_view color_str)
{
    auto c = glow::colors::color::from_hex(std::string(color_str));
    return color(tg::color4(c.r, c.g, c.b, c.a));
}

void glow::viewer::canvas_data::add_label(const glow::viewer::label& label) { _labels.push_back(label); }

void glow::viewer::canvas_data::add_labels(glow::array_view<const glow::viewer::label> labels)
{
    for (auto l : labels)
        _labels.push_back(l);
}

void glow::viewer::canvas_data::add_arrow(tg::pos3 from, tg::pos3 to, float world_size, tg::color3 color, const glow::viewer::arrow_style& style)
{
    auto mat = material(color);

    auto const length = distance(to, from);
    auto const dir = normalize_safe(to - from);
    tg::pos3 center;

    // compute 3 pos
    {
        auto const length_arrow = world_size * style.length_factor;
        auto const length_shaft = world_size * style.shaft_min_length_factor;
        auto const margin_arrow = world_size * style.margin_arrow_factor;
        auto const margin_shaft = world_size * style.margin_shaft_factor;

        if (length_arrow + length_shaft + margin_arrow + margin_shaft <= length) // enough space
        {
            to -= dir * margin_arrow;
            center = to - dir * length_arrow;
            from += dir * margin_shaft;
        }
        else // not enough space: start from to and go backwards
        {
            to -= dir * margin_arrow;
            center = to - dir * length_arrow;
            from = center - dir * length_shaft;
        }
    }

    auto const TA = tg::any_normal(dir);
    auto const TB = normalize_safe(-cross(TA, dir));

    auto const r_shaft = world_size / 2;
    auto const r_arrow = world_size * style.radius_factor / 2;

    for (auto i = 0; i < style.segments; ++i)
    {
        auto a0 = 360_deg * i / style.segments;
        auto a1 = 360_deg * (i == style.segments - 1 ? 0 : i + 1) / style.segments;

        auto [s0, c0] = tg::sin_cos(a0);
        auto [s1, c1] = tg::sin_cos(a1);

        auto n0 = TA * s0 + TB * c0;
        auto n1 = TA * s1 + TB * c1;

        auto const from_shaft_0 = from + n0 * r_shaft;
        auto const from_shaft_1 = from + n1 * r_shaft;

        auto const center_shaft_0 = center + n0 * r_shaft;
        auto const center_shaft_1 = center + n1 * r_shaft;

        auto const center_arrow_0 = center + n0 * r_arrow;
        auto const center_arrow_1 = center + n1 * r_arrow;

        // shaft end
        _add_triangle(from, from_shaft_0, from_shaft_1, -dir, -dir, -dir, mat);

        // shaft mantle
        _add_triangle(from_shaft_0, center_shaft_0, center_shaft_1, n0, n0, n1, mat);
        _add_triangle(from_shaft_0, center_shaft_1, from_shaft_1, n0, n1, n1, mat);

        // arrow end
        _add_triangle(center, center_arrow_0, center_arrow_1, -dir, -dir, -dir, mat);

        // arrow mantle
        // TODO: normal?
        _add_triangle(center_arrow_0, to, center_arrow_1, n0, n0, n1, mat);
    }
}

void glow::viewer::canvas_data::add_arrow(tg::pos3 from_pos, tg::vec3 extent, float world_size, tg::color3 color, const glow::viewer::arrow_style& style)
{
    add_arrow(from_pos, from_pos + extent, world_size, color, style);
}

void glow::viewer::canvas_data::add_arrow(tg::vec3 extent, tg::pos3 to_pos, float world_size, tg::color3 color, const glow::viewer::arrow_style& style)
{
    add_arrow(to_pos - extent, to_pos, world_size, color, style);
}

std::vector<glow::viewer::SharedRenderable> glow::viewer::canvas_data::create_renderables() const
{
    std::vector<glow::viewer::SharedRenderable> res;

    pm::Mesh m;
    auto pos = m.vertices().make_attribute<tg::pos3>();
    auto normals = m.vertices().make_attribute<tg::vec3>();
    auto size = m.vertices().make_attribute<float>();
    auto color3 = m.vertices().make_attribute<tg::color3>();
    auto color4 = m.vertices().make_attribute<tg::color4>();
    auto dash_size = m.edges().make_attribute<float>();

    // lazily initialized attributes for picking ids
    pm::vertex_attribute<int32_t> vertex_pick_ids_data;
    pm::face_attribute<int32_t> face_pick_ids_data;
    pm::edge_attribute<int32_t> edge_pick_ids_data;
    auto set_vertex_pick_id = [&](pm::vertex_handle v, int32_t id)
    {
        if (id < 0)
            return;
        if (!vertex_pick_ids_data.is_valid())
            vertex_pick_ids_data = m.vertices().make_attribute<int32_t>(-1);
        vertex_pick_ids_data[v] = id;
    };
    auto set_face_pick_id = [&](pm::face_handle f, int32_t id)
    {
        if (id < 0)
            return;
        if (!face_pick_ids_data.is_valid())
            face_pick_ids_data = m.faces().make_attribute<int32_t>(-1);
        face_pick_ids_data[f] = id;
    };
    auto set_edge_pick_id = [&](pm::edge_handle e, int32_t id)
    {
        if (id < 0)
            return;
        if (!edge_pick_ids_data.is_valid())
            edge_pick_ids_data = m.edges().make_attribute<int32_t>(-1);
        edge_pick_ids_data[e] = id;
    };

    auto const attach_picker = [&](auto& r, auto const& ids)
    {
        Picker pick;
        pick.setCustomIds(ids);
        if (!_pick_functions->empty())
        {
            pick.onLeftClick(
                [functions = _pick_functions](int32_t id, tg::pos3 const& p, tg::vec3 const& n)
                {
                    if (id >= 0 && (*functions)[id].on_left_click)
                        return (*functions)[id].on_left_click(p, n);
                    else
                        return picking_result::noSuccess();
                });
            pick.onRightClick(
                [functions = _pick_functions](int32_t id, tg::pos3 const& p, tg::vec3 const& n)
                {
                    if (id >= 0 && (*functions)[id].on_right_click)
                        return (*functions)[id].on_right_click(p, n);
                    else
                        return picking_result::noSuccess();
                });
            pick.onHover(
                [functions = _pick_functions](int32_t id, tg::pos3 const& p, tg::vec3 const& n)
                {
                    if (id >= 0 && (*functions)[id].on_hover)
                        return (*functions)[id].on_hover(p, n);
                    else
                        return picking_result::noSuccess();
                });
        }
        gv::configure(*r, pick);
    };


    //
    // points
    //
    if (!_points_px.empty())
    {
        auto has_transparent = false;
        auto has_picker = false;
        m.clear();
        m.vertices().reserve(_points_px.size());
        for (auto const& p : _points_px)
        {
            if (p.color.a <= 0)
                continue;

            if (p.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v = m.vertices().add();
            pos[v] = p.pos;
            color3[v] = tg::color3(p.color);
            size[v] = p.size;
            set_vertex_pick_id(v, p.pick_id);

            if (p.pick_id >= 0)
                has_picker = true;
        }

        if (!m.vertices().empty())
        {
            auto r = gv::make_and_configure_renderable(gv::points(pos).point_size_px(size), color3);

            if (has_picker)
                attach_picker(r, vertex_pick_ids_data);

            res.push_back(r);
        }

        has_picker = false;
        if (has_transparent)
        {
            m.clear();
            for (auto const& p : _points_px)
            {
                if (p.color.a <= 0 || p.color.a >= 1)
                    continue;

                auto v = m.vertices().add();
                pos[v] = p.pos;
                color4[v] = p.color;
                size[v] = p.size;
                set_vertex_pick_id(v, p.pick_id);

                if (p.pick_id >= 0)
                    has_picker = true;
            }

            auto r = gv::make_and_configure_renderable(gv::points(pos).point_size_px(size), color4, gv::transparent);
            if (has_picker)
                attach_picker(r, vertex_pick_ids_data);
            res.push_back(r);
        }
    }
    if (!_points_world.empty())
    {
        auto has_transparent = false;
        auto has_picker = false;
        m.clear();
        m.vertices().reserve(_points_world.size());
        for (auto const& p : _points_world)
        {
            if (p.color.a <= 0)
                continue;

            if (p.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v = m.vertices().add();
            pos[v] = p.pos;
            color3[v] = tg::color3(p.color);
            size[v] = p.size;
            set_vertex_pick_id(v, p.pick_id);

            if (p.pick_id >= 0)
                has_picker = true;
        }

        if (!m.vertices().empty())
        {
            auto r = gv::make_and_configure_renderable(gv::points(pos).point_size_world(size), color3);

            if (has_picker)
                attach_picker(r, vertex_pick_ids_data);

            res.push_back(r);
        }

        has_picker = false;
        if (has_transparent)
        {
            m.clear();
            for (auto const& p : _points_world)
            {
                if (p.color.a <= 0 || p.color.a >= 1)
                    continue;

                auto v = m.vertices().add();
                pos[v] = p.pos;
                color4[v] = p.color;
                size[v] = p.size;
                set_vertex_pick_id(v, p.pick_id);

                if (p.pick_id >= 0)
                    has_picker = true;
            }

            auto r = gv::make_and_configure_renderable(gv::points(pos).point_size_world(size), color4, gv::transparent);
            if (has_picker)
                attach_picker(r, vertex_pick_ids_data);
            res.push_back(r);
        }
    }

    //
    // splats
    //
    if (!_splats.empty())
    {
        auto has_transparent = false;
        auto has_picker = false;
        m.clear();
        m.vertices().reserve(_splats.size());
        for (auto const& p : _splats)
        {
            TG_ASSERT(p.size >= 0 && "no splat size set (forgot to call .set_splat_size() or .add_splats(...).size(...)?)");

            if (p.color.a <= 0)
                continue;

            if (p.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v = m.vertices().add();
            pos[v] = p.pos;
            color3[v] = tg::color3(p.color);
            size[v] = p.size;
            normals[v] = p.normal;
            set_vertex_pick_id(v, p.pick_id);

            if (p.pick_id >= 0)
                has_picker = true;
        }

        if (!m.vertices().empty())
        {
            auto r = gv::make_and_configure_renderable(gv::points(pos).round().normals(normals).point_size_world(size), color3);
            if (has_picker)
                attach_picker(r, vertex_pick_ids_data);
            res.push_back(r);
        }

        has_picker = false;
        if (has_transparent)
        {
            m.clear();
            for (auto const& p : _splats)
            {
                if (p.color.a <= 0 || p.color.a >= 1)
                    continue;

                auto v = m.vertices().add();
                pos[v] = p.pos;
                color4[v] = p.color;
                size[v] = p.size;
                normals[v] = p.normal;
                set_vertex_pick_id(v, p.pick_id);

                if (p.pick_id >= 0)
                    has_picker = true;
            }

            auto r = gv::make_and_configure_renderable(gv::points(pos).round().normals(normals).point_size_world(size), color4, gv::transparent);
            if (has_picker)
                attach_picker(r, vertex_pick_ids_data);
            res.push_back(r);
        }
    }

    //
    // lines
    //

    // two common functions for lines_px and lines_world:
    auto add_line_to_mesh = [&](auto const& l, bool& has_picker, bool& has_normals, auto transparency)
    {
        auto v0 = m.vertices().add();
        auto v1 = m.vertices().add();
        pos[v0] = l.p0.pos;
        pos[v1] = l.p1.pos;
        if constexpr (!transparency)
        {
            color3[v0] = tg::color3(l.p0.color);
            color3[v1] = tg::color3(l.p1.color);
        }
        else
        {
            color4[v0] = tg::color4(l.p0.color);
            color4[v1] = tg::color4(l.p1.color);
        }
        size[v0] = l.p0.size;
        size[v1] = l.p1.size;
        normals[v0] = l.p0.normal;
        normals[v1] = l.p1.normal;
        has_normals |= normals[v0] != tg::vec3() || normals[v1] != tg::vec3();
        auto e = m.edges().add_or_get(v0, v1);
        dash_size[e] = l.dash_size;
        set_edge_pick_id(e, l.pick_id);

        if (l.pick_id >= 0)
            has_picker = true;
    };

    // create a renderable from m
    auto add_line_renderable = [&](bool has_picker, bool has_normals, auto has_transparent, auto world_size, auto const& color_attr)
    {
        auto lines = gv::lines(pos);
        if constexpr (world_size)
            lines.line_width_world(size).dash_size_world(dash_size);
        else // TODO: screen space dash size
            lines.line_width_px(size);
        if (has_normals)
            lines.normals(normals);
        if (_state.two_colored_lines && !has_normals)
            glow::warning() << "two-colored lines require normals";

        auto r = _state.two_colored_lines && has_normals ? gv::make_and_configure_renderable(lines.two_colored(color_attr).force3D()) //
                                                         : gv::make_and_configure_renderable(lines, color_attr);

        if constexpr (has_transparent)
            gv::configure(*r, gv::transparent);

        if (has_picker)
            attach_picker(r, edge_pick_ids_data);

        res.push_back(r);
    };

    auto add_line_renderables = [&](std::vector<line> const& lines, auto world_size)
    {
        if (lines.empty())
            return;
        auto has_transparent = false;
        auto has_picker = false;
        bool has_normals = false;
        m.clear();
        m.vertices().reserve(lines.size());

        for (auto const& l : lines)
        {
            if (l.p0.color.a <= 0 && l.p1.color.a <= 0)
                continue;

            if (l.p0.color.a < 1 || l.p1.color.a < 1)
            {
                has_transparent = true;
                continue;
            }

            add_line_to_mesh(l, has_picker, has_normals, std::false_type());
        }

        if (!m.vertices().empty())
            add_line_renderable(has_picker, has_normals, std::false_type(), world_size, color3);

        has_picker = false;
        if (has_transparent)
        {
            m.clear();
            for (auto const& l : lines)
            {
                if (l.p0.color.a <= 0 && l.p1.color.a <= 0)
                    continue;

                if (l.p0.color.a >= 1 && l.p1.color.a >= 1)
                    continue;

                add_line_to_mesh(l, has_picker, has_normals, std::true_type());
            }

            add_line_renderable(has_picker, has_normals, std::false_type(), world_size, color4);
        }
    };

    add_line_renderables(_lines_px, std::false_type());
    add_line_renderables(_lines_world, std::true_type());


    //
    // faces
    //
    if (!_triangles.empty())
    {
        auto has_transparent = false;
        auto has_picker = false;
        m.clear();
        m.vertices().reserve(_triangles.size() * 3);
        for (auto const& t : _triangles)
        {
            if (t.color[0].a <= 0 && t.color[1].a <= 0 && t.color[2].a <= 0)
                continue;

            if (t.color[0].a < 1 || t.color[1].a < 1 || t.color[2].a < 1)
            {
                has_transparent = true;
                continue;
            }

            auto v0 = m.vertices().add();
            auto v1 = m.vertices().add();
            auto v2 = m.vertices().add();
            pos[v0] = t.pos[0];
            pos[v1] = t.pos[1];
            pos[v2] = t.pos[2];
            normals[v0] = t.normal[0];
            normals[v1] = t.normal[1];
            normals[v2] = t.normal[2];
            color3[v0] = tg::color3(t.color[0]);
            color3[v1] = tg::color3(t.color[1]);
            color3[v2] = tg::color3(t.color[2]);
            auto f = m.faces().add(v0, v1, v2);
            set_face_pick_id(f, t.pick_id);
            set_face_pick_id(f, t.pick_id);
            if (t.pick_id >= 0)
                has_picker = true;
        }

        if (!m.vertices().empty())
        {
            auto r = gv::make_and_configure_renderable(gv::polygons(pos).normals(normals), color3);
            if (has_picker)
                attach_picker(r, face_pick_ids_data);
            res.push_back(r);
        }

        has_picker = false;
        if (has_transparent)
        {
            m.clear();
            for (auto const& t : _triangles)
            {
                if (t.color[0].a <= 0 && t.color[1].a <= 0 && t.color[2].a <= 0)
                    continue;

                if (t.color[0].a >= 1 && t.color[1].a >= 1 && t.color[2].a >= 1)
                    continue;

                auto v0 = m.vertices().add();
                auto v1 = m.vertices().add();
                auto v2 = m.vertices().add();
                pos[v0] = t.pos[0];
                pos[v1] = t.pos[1];
                pos[v2] = t.pos[2];
                normals[v0] = t.normal[0];
                normals[v1] = t.normal[1];
                normals[v2] = t.normal[2];
                color4[v0] = t.color[0];
                color4[v1] = t.color[1];
                color4[v2] = t.color[2];
                auto f = m.faces().add(v0, v1, v2);
                set_face_pick_id(f, t.pick_id);
                if (t.pick_id >= 0)
                    has_picker = true;
            }

            auto r = gv::make_and_configure_renderable(gv::polygons(pos).normals(normals), color4, gv::transparent);
            if (has_picker)
                attach_picker(r, face_pick_ids_data);
            res.push_back(r);
        }
    }

    //
    // labels
    //
    if (!_labels.empty())
        res.push_back(gv::make_and_configure_renderable(_labels));

    //
    // quadrics
    //
    if (!_quadrics.empty())
        res.push_back(gv::make_renderable(_quadrics));

    // extras
    for (auto& r : res)
        r->name(_state.name);

    for (auto& r : res)
        r->transform(_transform);

    return res;
}

glow::viewer::canvas_t::~canvas_t()
{
    auto v = gv::view();
    for (auto const& r : create_renderables())
        gv::view(r);
}
