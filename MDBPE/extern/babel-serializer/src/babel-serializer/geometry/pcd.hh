#pragma once

#include <clean-core/strided_span.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <babel-serializer/errors.hh>

namespace babel::pcd
{
struct read_config
{
    // empty for now
};

struct viewpoint_t
{
    tg::pos3 position;
    tg::quat rotation;
};

struct field
{
    cc::string name;
    int size = 0;  ///< component size in bytes
    char type = 0; ///< component type (I, U, F)
    int count = 0; ///< number of components per entry

    /// returns the total field size in bytes
    size_t total_size() const { return size * count; }
};

/// PCD data: https://vml.sakura.ne.jp/koeda/PCL/tutorials/html/pcd_file_format.html
struct point_cloud
{
    cc::string version = "0.7";
    size_t width = 0;
    size_t height = 0;
    size_t points = 0;
    viewpoint_t viewpoint;

    cc::vector<field> fields;
    cc::vector<std::byte> data;

    // API
public:
    /// computes the expected per-point stride (in bytes)
    size_t compute_stride() const;

    /// returns true if a field with the name exists
    bool has_field(cc::string_view name) const;

    /// returns the field for the given name
    /// NOTE: field must exist
    field const& get_field(cc::string_view name) const;

    /// returns the field offset in bytes
    /// NOTE: field must exist
    size_t get_field_offset(cc::string_view name) const;

    /// returns a strided span for accessing the point data of this field
    /// NOTE: - field must exist
    ///       - field total_size must match sizeof(T)
    template <class T>
    cc::strided_span<T> get_data(cc::string_view name)
    {
        auto const& f = this->get_field(name);
        CC_ASSERT(sizeof(T) == f.total_size() && "stride mismatch");
        return cc::strided_span<T>(reinterpret_cast<T*>(data.data() + get_field_offset(name)), points, compute_stride());
    }
    template <class T>
    cc::strided_span<T const> get_data(cc::string_view name) const
    {
        auto const& f = this->get_field(name);
        CC_ASSERT(sizeof(T) == f.total_size() && "stride mismatch");
        return cc::strided_span<T const>(reinterpret_cast<T const*>(data.data() + get_field_offset(name)), points, compute_stride());
    }

    /// resizes the data vector to match the expected size (useful before writing points)
    /// NOTE: requires at least one field
    ///       fields must not be changed afterwards
    void allocate_data();
};

/// reads a point cloud from memory
/// TODO: - stream-based interface
///       - control over pcd output data?
point_cloud read(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);
}
