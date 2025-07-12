#include "pcd.hh"

// TODO: replace by cc stuff?
#include <cstdio>

#include <cstdint>

#include <clean-core/format.hh>
#include <clean-core/function_ptr.hh>
#include <clean-core/function_ref.hh>
#include <clean-core/string_view.hh>
#include <clean-core/utility.hh>

size_t babel::pcd::point_cloud::compute_stride() const
{
    size_t stride = 0;
    for (auto const& f : fields)
        stride += f.total_size();
    return stride;
}

bool babel::pcd::point_cloud::has_field(cc::string_view name) const
{
    for (auto const& f : fields)
        if (f.name == name)
            return true;
    return false;
}

const babel::pcd::field& babel::pcd::point_cloud::get_field(cc::string_view name) const
{
    for (auto const& f : fields)
        if (f.name == name)
            return f;

    CC_UNREACHABLE("field does not exist");
}

size_t babel::pcd::point_cloud::get_field_offset(cc::string_view name) const
{
    size_t offset = 0;
    for (auto const& f : fields)
    {
        if (f.name == name)
            return offset;

        offset += f.total_size();
    }

    CC_UNREACHABLE("field does not exist");
}

void babel::pcd::point_cloud::allocate_data()
{
    CC_ASSERT(!fields.empty() && "fields must be declared first");

    data.resize(points * compute_stride());
}

babel::pcd::point_cloud babel::pcd::read(cc::span<const std::byte> data, read_config const&, error_handler on_error)
{
    point_cloud pts;

    // TODO: error positions are wrong

    size_t prev_pos = 0;
    size_t pos = 0;
    auto get_line = [&]() -> cc::string_view {
        prev_pos = pos;
        if (pos >= data.size())
        {
            on_error(data, data.last(0), "expected line", severity::error);
            return "";
        }

        // skip comment lines
        while (char(data[pos]) == '#')
        {
            while (pos < data.size() && char(data[pos]) != '\n')
                ++pos;
            if (pos < data.size() && char(data[pos]) == '\n')
                ++pos;
        }

        auto p_start = reinterpret_cast<char const*>(&data[pos]);
        while (pos < data.size())
        {
            auto c = char(data[pos]);
            if (c == '\n' || c == '#')
                break;

            ++pos;
        }
        auto p_end = reinterpret_cast<char const*>(data.data() + pos);

        // ignore rest of comment
        while (pos < data.size())
        {
            auto c = char(data[pos]);
            if (c == '\n')
                break;

            ++pos;
        }

        // point behind newline
        if (pos < data.size() && char(data[pos]) == '\n')
            ++pos;

        return cc::string_view(p_start, p_end);
    };

    auto parse_int = [&](cc::string_view s) -> int {
        cc::string ss = s; // null-terminated
        int i;
        auto r = std::sscanf(ss.c_str(), "%d", &i);
        if (r == 0)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse int", severity::error);
            return 0;
        }
        return i;
    };
    auto parse_int64 = [&](cc::string_view s) -> long long int {
        cc::string ss = s; // null-terminated
        long long int i;
        auto r = std::sscanf(ss.c_str(), "%lld", &i);
        if (r == 0)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse int", severity::error);
            return 0;
        }
        return i;
    };
    auto parse_uint64 = [&](cc::string_view s) -> unsigned long long int {
        cc::string ss = s; // null-terminated
        unsigned long long int i;
        auto r = std::sscanf(ss.c_str(), "%llu", &i);
        if (r == 0)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse uint", severity::error);
            return 0;
        }
        return i;
    };
    auto parse_float = [&](cc::string_view s) -> float {
        cc::string ss = s; // null-terminated
        float f;
        auto r = std::sscanf(ss.c_str(), "%f", &f);
        if (r == 0)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse float", severity::error);
            return 0;
        }
        return f;
    };
    auto parse_double = [&](cc::string_view s) -> double {
        cc::string ss = s; // null-terminated
        double f;
        auto r = std::sscanf(ss.c_str(), "%lf", &f);
        if (r == 0)
        {
            on_error(data, cc::as_byte_span(s), "unable to parse double", severity::error);
            return 0;
        }
        return f;
    };

    // version
    {
        auto s = get_line();
        if (!s.starts_with("VERSION "))
        {
            on_error(data, cc::as_byte_span(s), "expected VERSION line", severity::error);
            return {};
        }

        pts.version = s.subview(8).trim();
        // TODO: make me into some warning feature
        if (pts.version != "0.7" && pts.version != ".7" && pts.version != "7")
        {
            on_error(data, cc::as_byte_span(s), "expected VERSION 0.7", severity::warning);
        }
    }

    // fields
    {
        auto s = get_line();
        if (!s.starts_with("FIELDS "))
        {
            on_error(data, cc::as_byte_span(s), "expected FIELDS line", severity::error);
            return {};
        }
        s = s.subview(7).trim();

        for (auto name : s.split())
        {
            auto& f = pts.fields.emplace_back();
            f.name = name;
        }
    }

    // size
    {
        auto s = get_line();
        if (!s.starts_with("SIZE "))
        {
            on_error(data, cc::as_byte_span(s), "expected SIZE line", severity::error);
            return {};
        }
        s = s.subview(5).trim();

        size_t i = 0;
        for (auto v : s.split())
        {
            if (i >= pts.fields.size())
            {
                on_error(data, cc::as_byte_span(s), "too many entries", severity::warning);
                break;
            }

            auto si = parse_int(v);
            if (si != 1 && si != 2 && si != 4 && si != 8)
                on_error(data, cc::as_byte_span(v), "unknown size", severity::warning);
            pts.fields[i].size = si;

            ++i;
        }
        if (i != pts.fields.size())
        {
            on_error(data, cc::as_byte_span(s), "too few entries", severity::error);
            return {};
        }
    }

    // type
    {
        auto s = get_line();
        if (!s.starts_with("TYPE "))
        {
            on_error(data, cc::as_byte_span(s), "expected TYPE line", severity::error);
            return {};
        }
        s = s.subview(5).trim();

        size_t i = 0;
        for (auto v : s.split())
        {
            if (i >= pts.fields.size())
            {
                on_error(data, cc::as_byte_span(v), "too many entries", severity::warning);
                break;
            }

            if (v != "I" && v != "U" && v != "F")
                on_error(data, cc::as_byte_span(v), "unknown type", severity::warning);

            if (v == "F" && pts.fields[i].size != 4 && pts.fields[i].size != 8)
                on_error(data, cc::as_byte_span(v), "float field must be 4 or 8 bytes", severity::warning);

            pts.fields[i].type = v[0];

            ++i;
        }
        if (i != pts.fields.size())
        {
            on_error(data, cc::as_byte_span(s), "too few entries", severity::error);
            return {};
        }
    }

    // count
    {
        auto s = get_line();
        if (!s.starts_with("COUNT "))
        {
            on_error(data, cc::as_byte_span(s), "expected COUNT line", severity::error);
            return {};
        }
        s = s.subview(5).trim();

        size_t i = 0;
        for (auto v : s.split())
        {
            if (i >= pts.fields.size())
            {
                on_error(data, cc::as_byte_span(v), "too many entries", severity::warning);
                break;
            }

            pts.fields[i].count = parse_int(v);

            ++i;
        }
        if (i != pts.fields.size())
        {
            on_error(data, cc::as_byte_span(s), "too few entries", severity::error);
            return {};
        }
    }

    // width
    {
        auto s = get_line();
        if (!s.starts_with("WIDTH "))
        {
            on_error(data, cc::as_byte_span(s), "expected WIDTH line", severity::error);
            return {};
        }
        pts.width = parse_int(s.subview(6).trim());
    }
    // height
    {
        auto s = get_line();
        if (!s.starts_with("HEIGHT "))
        {
            on_error(data, cc::as_byte_span(s), "expected HEIGHT line", severity::error);
            return {};
        }
        pts.height = parse_int(s.subview(7).trim());
    }

    // viewpoint
    {
        auto s = get_line();
        if (!s.starts_with("VIEWPOINT "))
        {
            on_error(data, cc::as_byte_span(s), "expected VIEWPOINT line", severity::error);
            return {};
        }
        s = s.subview(10).trim();

        cc::vector<float> vp;
        for (auto v : s.split())
            vp.push_back(parse_float(v));
        if (vp.size() != 3 + 4)
        {
            on_error(data, cc::as_byte_span(s), "expected 7 floats", severity::warning);
        }
        else
        {
            pts.viewpoint.position = {vp[0], vp[1], vp[2]};
            pts.viewpoint.rotation = {vp[3], vp[4], vp[5], vp[6]};
        }
    }

    // points
    {
        auto s = get_line();
        if (!s.starts_with("POINTS "))
        {
            on_error(data, cc::as_byte_span(s), "expected POINTS line", severity::error);
            return {};
        }
        pts.points = parse_int(s.subview(7).trim());

        if (pts.points != pts.width * pts.height)
            on_error(data, cc::as_byte_span(s),
                     cc::format("expected {} x {} = {} points but got {} points", pts.width, pts.height, pts.width * pts.height, pts.points), severity::warning);
    }

    // data
    {
        auto s = get_line();
        if (!s.starts_with("DATA "))
        {
            on_error(data, cc::as_byte_span(s), "expected DATA line", severity::error);
            return {};
        }
        s = s.subview(5).trim();

        pts.allocate_data();

        if (s == "binary")
        {
            if (data.size() - pos != pts.data.size())
                on_error(data, cc::as_byte_span(s), cc::format("DATA size mismatch, expected {} B, got {} B", pts.data.size(), data.size() - pos),
                         severity::warning);

            // just a raw copy
            std::memcpy(pts.data.data(), data.data() + pos, cc::min(pts.data.size(), data.size() - pos));
        }
        else if (s == "ascii")
        {
            // build deserializers
            cc::vector<size_t> offsets;
            cc::vector<cc::function_ref<void(std::byte*, cc::string_view)>> parsers;
            {
                size_t offset = 0;
                for (auto const& f : pts.fields)
                {
                    for (auto i = 0; i < f.count; ++i)
                    {
                        offsets.push_back(offset);
                        switch (f.type)
                        {
                        case 'I':
                            switch (f.size)
                            {
                            case 1:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int8_t*>(d) = parse_int(s);
                                });
                                break;
                            case 2:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int16_t*>(d) = parse_int(s);
                                });
                                break;
                            case 4:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int32_t*>(d) = parse_int(s);
                                });
                                break;
                            case 8:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<int64_t*>(d) = parse_int64(s);
                                });
                                break;
                            default:
                                parsers.push_back([&](std::byte*, cc::string_view) {});
                                break;
                            }
                            break;
                        case 'U':
                            switch (f.size)
                            {
                            case 1:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint8_t*>(d) = parse_uint64(s);
                                });
                                break;
                            case 2:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint16_t*>(d) = parse_uint64(s);
                                });
                                break;
                            case 4:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint32_t*>(d) = parse_uint64(s);
                                });
                                break;
                            case 8:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<uint64_t*>(d) = parse_uint64(s);
                                });
                                break;
                            default:
                                parsers.push_back([&](std::byte*, cc::string_view) {});
                                break;
                            }
                            break;
                        case 'F':
                            switch (f.size)
                            {
                            case 4:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<float*>(d) = parse_float(s);
                                });
                                break;
                            case 8:
                                parsers.push_back([&](std::byte* d, cc::string_view s) { //
                                    *reinterpret_cast<double*>(d) = parse_double(s);
                                });
                                break;
                            default:
                                parsers.push_back([&](std::byte*, cc::string_view) {});
                                break;
                            }
                            break;
                        default:
                            CC_UNREACHABLE("unknown type");
                        }

                        offset += f.size;
                    }
                }
                CC_ASSERT(offsets.size() == parsers.size() && "invalid type somewhere");
            }

            // each point line by line
            for (size_t p = 0; p < pts.points; ++p)
            {
                s = get_line();
                auto stride = pts.compute_stride();

                size_t i = 0;
                for (auto v : s.split())
                {
                    if (i >= parsers.size())
                    {
                        on_error(data, cc::as_byte_span(v), "too many entries", severity::warning);
                        break;
                    }

                    auto d = pts.data.data() + stride * p + offsets[i];
                    parsers[i](d, v);

                    ++i;
                }
                if (i != parsers.size())
                    on_error(data, cc::as_byte_span(s), "too few entries", severity::warning);
            }
        }
        else
        {
            on_error(data, cc::as_byte_span(s), "unexpected DATA format", severity::error);
            return {};
        }
    }

    return pts;
}
