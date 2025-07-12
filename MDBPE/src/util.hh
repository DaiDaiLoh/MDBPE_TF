#pragma once

#include <clean-core/vector.hh>

#include <typed-geometry/types/color.hh>

namespace tp
{
/// generate a set of 'count' colors on the HSL color wheel
cc::vector<tg::color3> generate_colors(int count);

}
