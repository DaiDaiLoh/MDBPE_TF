#include "zstd.hh"

#include <clean-core/format.hh>
#include <clean-core/to_string.hh>

#include <zstd/zstd.h>

cc::vector<std::byte> babel::zstd::compress(cc::span<std::byte const> data, int compression_level)
{
    cc::vector<std::byte> res;
    res.resize(ZSTD_compressBound(data.size()));
    auto real_size = ZSTD_compress(res.data(), res.size(), data.data(), data.size(), compression_level);
    res.resize(real_size); // preserves actual content
    return res;
}

cc::vector<std::byte> babel::zstd::uncompress(cc::span<std::byte const> data, babel::error_handler on_error)
{
    auto res_size = ZSTD_getFrameContentSize(data.data(), data.size());

    if (res_size == ZSTD_CONTENTSIZE_UNKNOWN)
    {
        on_error(data, {}, "cannot determine length of uncompressed data (internal zstd error)", severity::error);
        return {};
    }
    if (res_size == ZSTD_CONTENTSIZE_ERROR)
    {
        on_error(data, {}, "unable to get length of uncompressed data (internal zstd error)", severity::error);
        return {};
    }

    auto res = cc::vector<std::byte>::uninitialized(res_size);

    auto real_size = ZSTD_decompress(res.data(), res.size(), data.data(), data.size());

    if (ZSTD_isError(real_size))
    {
        on_error(data, {}, cc::format("could not decompress data (internal zstd error: {})", ZSTD_getErrorName(real_size)), severity::error);
        return {};
    }

    res.resize(real_size);

    return res;
}
