#pragma once

#include <cstdint>

#include <clean-core/span.hh>

#include <babel-serializer/errors.hh>

namespace babel::pcap
{
namespace detail
{
struct packet_range; // fwd
}

/// This is the header for an entire pcap file
struct header
{
    uint32_t magic_number;  ///< magic number
    uint16_t version_major; ///< major version number
    uint16_t version_minor; ///< minor version number
    int32_t thiszone;       ///< GMT to local correction
    uint32_t sigfigs;       ///< accuracy of timestamps
    uint32_t snap_length;   ///< max length of captured packets, in octets
    uint32_t network;       ///< data link type

    /// returns true, if the endianness of all data must be swapped because the system that produced this
    /// pcap file has opposite endianness than the system that is now reading the file
    bool has_swapped_endianness() const { return magic_number == 0xd4c3b2a1u || magic_number == 0x4d3cb2a1u; }

    /// returns true, if the packet timestamps are nanoseconds instead of microseconds
    bool has_nanosecond_timestamps() const { return magic_number == 0xa1b23c4du || magic_number == 0x4d3cb2a1u; }
};

/// this represents a single packet of a pcap file
struct packet
{
    uint32_t timestamp_sec = 0;     ///< timestamp seconds
    uint32_t timestamp_usec = 0;    ///< timestamp microseconds (or nanoseconds, see header). Offset to seconds.
    uint32_t original_size = 0;     ///< original length of packet (NOT the stored one)
    cc::span<std::byte const> data; ///< raw data of the packet
};

struct read_config
{
};

/// usage:
/// auto h = header_of(my_data);
header header_of(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);

/// usage:
/// auto data = ...;
/// for(auto p : packets_of(data))
///     my_process_data(p.data());
///
/// IMPORTANT: data MUST survive the packet_range and its iterators!
/// IMPORTANT: on_error MUST survive the packet_range and its iterators!
///
/// Usage with a custom error handler:
/// auto data = ...;
/// auto my_error_handler = [&](auto data, auto pos, auto message, auto severity){...};
/// auto cfg = babel::pcap::read_config();
/// for(auto p : packets_of(data, cfg, my_error_handler))
///     my_process_data(p.data());
detail::packet_range packets_of(cc::span<std::byte const> data, read_config const& cfg = {}, error_handler on_error = default_error_handler);

namespace detail
{
struct packet_iterator
{
    struct packet_header
    {
        uint32_t ts_sec;   /* timestamp seconds */
        uint32_t ts_usec;  /* timestamp microseconds */
        uint32_t incl_len; /* number of octets of packet saved in file */
        uint32_t orig_len; /* actual length of packet */
    };

    packet_iterator(cc::span<std::byte const> data, read_config const& cfg, error_handler on_error, bool swap_endianness);
    packet operator*() { return _current_packet; }
    packet_iterator& operator++();
    packet_iterator operator++(int)
    {
        auto i = *this;
        operator++();
        return i;
    }
    bool operator!=(cc::sentinel) const { return _pos < _data.size(); }

private:
    packet read_packet();
    cc::span<std::byte const> _data;
    read_config _cfg; // currently unused
    error_handler _on_error;
    bool _swap_endianness;
    packet _current_packet;
    size_t _pos = 0;
};

struct packet_range
{
    cc::span<std::byte const> data; ///< data without the header
    read_config cfg;
    error_handler on_error;
    bool swap_endianness;

    packet_range(cc::span<std::byte const> data, read_config const& cfg, error_handler on_error, bool swap_endianness)
      : data{data}, cfg{cfg}, on_error{on_error}, swap_endianness{swap_endianness}
    {
    }

    packet_iterator begin() const { return {data, cfg, on_error, swap_endianness}; }
    cc::sentinel end() const { return {}; }
};
}
}
