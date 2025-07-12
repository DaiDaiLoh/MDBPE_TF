#pragma once

#include <cstddef>

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

// TODO: more of zstd's API can be exposed if required

namespace babel::zstd
{
/// compresses a range of bytes using facebook's zstd
/// NOTE: the result has more capacity than data
///       if it is stored long-term, a shrink_to_fit is advised
/// NOTE: a compression level of 0 means "use default" (for other values please consult zstd)
cc::vector<std::byte> compress(cc::span<std::byte const> data, int compression_level = 0);

/// Tries to uncompress the given data
cc::vector<std::byte> uncompress(cc::span<std::byte const> data, error_handler on_error = default_error_handler);
}
