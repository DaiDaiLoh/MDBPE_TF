#pragma once

#include <typed-geometry/types/vec.hh>

namespace tp
{
/// constellation of one token class pair and their ancor offset vector
struct constellation
{
    int source_class_id = -1;
    int target_class_id = -1;
    tg::ivec2 ancor_offset; // offset from the pixel position

    bool operator==(constellation const&) const = default;
};
}
