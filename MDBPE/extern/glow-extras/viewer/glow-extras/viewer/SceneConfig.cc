#include "SceneConfig.hh"

#include <glow/common/hash.hh>

size_t glow::viewer::SceneConfig::computeHash() const
{
    auto h = 0x7121231;
    auto add = [&](auto const& v) { h = glow::hash_xxh3(as_byte_view(v), h); };

    // only those that affect rendering
    add(enableGrid);
    add(enablePrintMode);
    add(enableForwardRendering);
    add(enableTransparentRendering);
    add(enableShadows);
    add(enableOutlines);
    add(infiniteAccumulation);
    add(clearAccumulation);
    add(bgColorInner);
    add(bgColorOuter);
    add(ssaoPower);
    add(ssaoRadius);
    add(enableTonemap);
    add(tonemapExposure);

    add(customBounds.has_value());
    if (customBounds.has_value())
        add(customBounds.value());

    add(customGridCenter.has_value());
    if (customGridCenter.has_value())
        add(customGridCenter.value());

    add(customGridSize.has_value());
    if (customGridSize.has_value())
        add(customGridSize.value());

    return h;
}
