#pragma once

#include <cstddef> // std::byte
#include <cstdint>
#include <type_traits> // std::is_same_v

#include <clean-core/assert.hh>
#include <clean-core/span.hh>
#include <clean-core/strided_span.hh>
#include <clean-core/string.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

/// TODO: Iterators

/// See http://paulbourke.net/dataformats/ply/
namespace babel::ply
{
struct read_config
{
};
/// valid types that can appear in a ply file
/// signed integers are two's complement
/// float and double are IEEE 754
enum class type
{
    invalid,
    char_t,   // 1 byte
    uchar_t,  // 1 byte
    short_t,  // 2 byte
    ushort_t, // 2 byte
    int_t,    // 4 byte
    uint_t,   // 4 byte
    float_t,  // 4 byte
    double_t  // 8 byte
};

struct geometry
{
    /// A ply element definition with a 'name' and the number of entries ('count') of this type of element.
    /// Each element consists of a set of properties which are consecutively stored in 'properties'.
    /// An element's properties range from the indices 'properties_start' to 'properties_start + properties_count'
    struct element
    {
        cc::string name;
        int count = -1;
        int properties_start = -1; // index of the first property
        int properties_count = -1; // number of properties
    };

    /// A ply property definition with a 'name' and a 'type'.
    /// A property is a list if 'list_size_type' is NOT invalid
    /// A properties data is stored in 'data'
    struct property
    {
        cc::string name;
        ply::type type = ply::type::invalid;
        ply::type list_size_type = ply::type::invalid;

        constexpr bool is_list() const { return list_size_type != ply::type::invalid; }
    };

    /// To allow the use of cc::strided_span to access a propertys data, lists are stored separately in 'list_data'.
    /// This means for lists an additional layer of indirection is necessary.
    /// Therefore the content of 'data' for a list property is this list struct.
    struct list_property_entry
    {
        int start_idx = -1; // index into list_data
        int size = -1;      // the number of elements, not bytes
    };

public: // member
    cc::vector<property> properties;
    cc::vector<element> elements;
    cc::vector<std::byte> data;
    cc::vector<std::byte> list_data;

public: // api
    bool has_element(cc::string_view name) const;

    element& get_element(cc::string_view name);
    element const& get_element(cc::string_view name) const;

    bool has_property(element const& element, cc::string_view name) const;

    property& get_property(element const& element, cc::string_view name);
    property const& get_property(element const& element, cc::string_view name) const;

    cc::span<property> get_properties(cc::string_view element_name);
    cc::span<property const> get_properties(cc::string_view element_name) const;

    template <class T>
    cc::strided_span<T> get_data(element const& element, property const& property)
    {
        CC_ASSERT(property.is_list() == (std::is_same_v<T, list_property_entry>)&&"Lists must be read as 'list_property_entry'");
        return {reinterpret_cast<T*>(&(data[data_start_index(element, property)])), size_t(element.count), int64_t(size_in_bytes(element))};
    }

    template <class T>
    cc::strided_span<T const> get_data(element const& element, property const& property) const
    {
        CC_ASSERT(property.is_list() == (std::is_same_v<T, list_property_entry const>)&&"Lists must be read as 'list_property_entry'");
        return {reinterpret_cast<T const*>(&(data[data_start_index(element, property)])), size_t(element.count), int64_t(size_in_bytes(element))};
    }

    cc::strided_span<list_property_entry> get_list_entries(element const& element, property const& property)
    {
        return get_data<list_property_entry>(element, property);
    }

    cc::strided_span<list_property_entry const> get_list_entries(element const& element, property const& property) const
    {
        return get_data<list_property_entry const>(element, property);
    }

    template <class T>
    cc::span<T> get_data(list_property_entry const& list)
    {
        return {reinterpret_cast<T*>(&list_data[list.start_idx]), size_t(list.size)};
    }

    template <class T>
    cc::span<T const> get_data(list_property_entry const& list) const
    {
        return {reinterpret_cast<T const*>(&list_data[list.start_idx]), size_t(list.size)};
    }

    /// returns the sum of the byte sizes of the elements properties
    size_t size_in_bytes(element const& element) const;

    /// returns the offset of proptery into its elements in bytes
    size_t offset_of(element const& element, property const& property) const;

    /// returns the index of the first byte of the given property data
    size_t data_start_index(element const& element, property const& property) const;
};

geometry read(cc::span<const std::byte> data, babel::ply::read_config const& cfg = {}, error_handler on_error = default_error_handler);

}
