#pragma once

#include <cstddef>

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <babel-serializer/errors.hh>

// TODO: more of snappy's API can be exposed if required

namespace babel::snappy
{
/// compresses a range of bytes using google's snappy
/// NOTE: the result has more capacity than data
///       if it is stored long-term, a shrink_to_fit is advised
cc::vector<std::byte> compress(cc::span<std::byte const> data);

/// Tries to uncompress the given data
cc::vector<std::byte> uncompress(cc::span<std::byte const> data, error_handler on_error = default_error_handler);
}
