#include "util.hh"

#include <typed-geometry/feature/colors.hh>
#include <typed-geometry/functions/random/shuffle.hh>

cc::vector<tg::color3> tp::generate_colors(int count)
{
    cc::vector<tg::color3> colors;
    for (auto i = 0; i < count; ++i)
    {
        auto alpha = float(i) / count;
        auto hsl = tg::hsl(alpha, 1.0, 0.5);
        auto rgb = tg::convert_color_to<tg::color3>(hsl);
        rgb.r = tg::clamp(rgb.r, 0.0, 1.0);
        rgb.g = tg::clamp(rgb.g, 0.0, 1.0);
        rgb.b = tg::clamp(rgb.b, 0.0, 1.0);
        colors.push_back(rgb);
    }
    tg::rng rng;
    tg::shuffle(rng, colors);
    return colors;
}
