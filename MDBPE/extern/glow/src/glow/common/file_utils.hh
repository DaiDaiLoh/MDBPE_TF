#pragma once

#include <cstdint>
#include <string>

namespace glow
{
namespace util
{
/// Returns a 64bit representing the file modification time of a given file
/// Returns 0 for non-existing file
/// Gets bigger with progressing time
int64_t fileModificationTime(std::string const& filename);
}
}
