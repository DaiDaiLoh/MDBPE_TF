#include "hash.hh"

#include <glow/detail/xxHash/xxh3.hh>

size_t glow::hash_xxh3(array_view<const std::byte> data, size_t seed)
{
    //
    return XXH3_64bits_withSeed(data.data(), data.size(), seed);
}
