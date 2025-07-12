#include "snappy.hh"

#include <snappy/snappy.h>

cc::vector<std::byte> babel::snappy::compress(cc::span<std::byte const> data)
{
    cc::vector<std::byte> res;
    res.resize(::snappy::MaxCompressedLength(data.size()));
    size_t real_size = 0;
    ::snappy::RawCompress(reinterpret_cast<char const*>(data.data()), data.size(), reinterpret_cast<char*>(res.data()), &real_size);
    res.resize(real_size); // preserves actual content
    return res;
}

cc::vector<std::byte> babel::snappy::uncompress(cc::span<std::byte const> data, babel::error_handler on_error)
{
    size_t res_size = 0;
    if (!::snappy::GetUncompressedLength(reinterpret_cast<char const*>(data.data()), data.size(), &res_size))
    {
        on_error(data, {}, "unable to get length of uncompressed data (internal snappy error)", severity::error);
        return {};
    }

    auto res = cc::vector<std::byte>::uninitialized(res_size);

    if (!::snappy::RawUncompress(reinterpret_cast<char const*>(data.data()), data.size(), reinterpret_cast<char*>(res.data())))
    {
        on_error(data, {}, "could not decompress data (internal snappy error)", severity::error);
        return {};
    }

    return res;
}
