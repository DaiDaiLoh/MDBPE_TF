#include "lz4.hh"

#include <lz4/lz4.h>

cc::vector<std::byte> babel::lz4::compress(cc::span<std::byte const> data)
{
    cc::vector<std::byte> res;
    res.resize(LZ4_compressBound(data.size()));
    auto real_size = LZ4_compress_default(reinterpret_cast<char const*>(data.data()), reinterpret_cast<char*>(res.data()), data.size_bytes(), res.size());
    res.resize(real_size); // preserves actual content
    return res;
}

bool babel::lz4::uncompress_to(cc::span<std::byte> out_data, cc::span<std::byte const> data, error_handler on_error)
{
    auto r = LZ4_decompress_safe(reinterpret_cast<char const*>(data.data()), reinterpret_cast<char*>(out_data.data()), data.size(), out_data.size());
    if (r < 0)
    {
        on_error(data, {}, "error trying to decompress data (internal lz4 error)", severity::error);
        return false;
    }
    if (r != int(out_data.size()))
    {
        on_error(data, {}, "lz4 requires the exact decompressed size", severity::error);
        return false;
    }
    return true;
}

cc::vector<std::byte> babel::lz4::uncompress(cc::span<std::byte const> data, size_t uncompressedSize, babel::error_handler on_error)
{
    cc::vector<std::byte> res;
    res.resize(uncompressedSize);
    uncompress_to(res, data, on_error);
    return res;
}
