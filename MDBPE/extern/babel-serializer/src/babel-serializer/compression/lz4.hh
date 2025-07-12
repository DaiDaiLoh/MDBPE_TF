#pragma once

#include <cstddef>

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

// TODO: more of lz4's API can be exposed if required

namespace babel::lz4
{
/// compresses a range of bytes using lz4
/// NOTE: the result has more capacity than data
///       if it is stored long-term, a shrink_to_fit is advised
cc::vector<std::byte> compress(cc::span<std::byte const> data);

/// Tries to uncompress the given data
/// NOTE: lz4 does not store the uncompressed size, so it out_data must already be sized appropriately
bool uncompress_to(cc::span<std::byte> out_data, cc::span<std::byte const> data, error_handler on_error = default_error_handler);
/// same as uncompress_to but allocates target buffer
/// NOTE: lz4 does not store the uncompressed size, so it must be transmitted separately
cc::vector<std::byte> uncompress(cc::span<std::byte const> data, size_t uncompressedSize, error_handler on_error = default_error_handler);
}
