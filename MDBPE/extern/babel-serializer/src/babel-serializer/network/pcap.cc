#include "pcap.hh"

#include <cstdint>

#include <clean-core/bit_cast.hh>
#include <clean-core/bits.hh>

babel::pcap::detail::packet_iterator::packet_iterator(cc::span<std::byte const> data, babel::pcap::read_config const& cfg, babel::error_handler on_error, bool swap_endianness)
  : _data{data}, _cfg{cfg}, _on_error{on_error}, _swap_endianness{swap_endianness}
{
    (void)_cfg; // currently unused
    if (!_data.empty())
        _current_packet = read_packet();
}

babel::pcap::detail::packet_iterator& babel::pcap::detail::packet_iterator::operator++()
{
    _pos += sizeof(packet_header) + _current_packet.data.size();
    if (_pos < _data.size())
        _current_packet = read_packet();
    return *this;
}

babel::pcap::packet babel::pcap::detail::packet_iterator::read_packet()
{
    packet_header ph;
    if (_pos + sizeof(ph) > _data.size())
    {
        _on_error(_data, _data.last(0), "Unexpected end of file", severity::warning);
        return {};
    }
    _data.subspan(_pos, sizeof(ph)).copy_to(cc::as_byte_span(ph));
    if (_pos + sizeof(ph) + ph.incl_len > _data.size())
    {
        _on_error(_data, _data.last(0), "Unexpected end of file", severity::warning);
        return {};
    }
    if (_swap_endianness)
    {
        ph.incl_len = cc::byteswap(ph.incl_len);
        ph.orig_len = cc::byteswap(ph.orig_len);
        ph.ts_sec = cc::byteswap(ph.ts_sec);
        ph.ts_usec = cc::byteswap(ph.ts_usec);
    }
    packet p;
    p.original_size = ph.orig_len;
    p.timestamp_usec = ph.ts_usec;
    p.timestamp_sec = ph.ts_sec;
    p.data = _data.subspan(_pos + sizeof(ph), ph.incl_len);
    return p;
}

babel::pcap::header babel::pcap::header_of(cc::span<std::byte const> data, babel::pcap::read_config const& cfg, babel::error_handler on_error)
{
    (void)cfg; // for now unused

    header h;
    if (data.size() < sizeof(h))
    {
        on_error(data, data.last(0), "Unexpected end of file", severity::error);
        return {};
    }
    data.subspan(0, sizeof(h)).copy_to(cc::as_byte_span(h));
    if (h.has_swapped_endianness())
    {
        h.version_major = cc::byteswap(h.version_major);
        h.thiszone = cc::bit_cast<int32_t>(cc::byteswap(cc::bit_cast<uint32_t>(h.thiszone)));
        h.sigfigs = cc::byteswap(h.sigfigs);
        h.snap_length = cc::byteswap(h.snap_length);
        h.network = cc::byteswap(h.network);
    }
    return h;
}

babel::pcap::detail::packet_range babel::pcap::packets_of(cc::span<std::byte const> data, babel::pcap::read_config const& cfg, babel::error_handler on_error)
{
    auto const h = header_of(data);
    return detail::packet_range(data.subspan(sizeof(header)), cfg, on_error, h.has_swapped_endianness());
}
