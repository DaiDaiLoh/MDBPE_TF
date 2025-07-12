#pragma once

#include <clean-core/vector.hh>

#include <typed-geometry/types/pos.hh>

namespace tp
{
/// data for a single token class
struct token_data
{
    cc::vector<tg::ipos2> positions; // relative to ancor at (0,0)
    cc::vector<int> position_class;  // original token class off position_class[i] at positions[i]
    int class_id = -1;
};

}
