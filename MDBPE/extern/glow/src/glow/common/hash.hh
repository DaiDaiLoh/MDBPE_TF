#pragma once

#include <cstddef>

#include <glow/common/array_view.hh>

namespace glow
{
// returns a hash of the data by executing https://github.com/Cyan4973/xxHash
[[nodiscard]] size_t hash_xxh3(array_view<std::byte const> data, size_t seed);
}
