#pragma once

#include <typed-geometry/feature/assert.hh>

#include "color.hh"

namespace glow
{
namespace colors
{
namespace material
{
color material(color c, int variant = 500, bool accent = false);

inline color red(int variant = 500, bool accent = false) { return material(color::from_hex("#f44336"), variant, accent); }
inline color pink(int variant = 500, bool accent = false) { return material(color::from_hex("#e91e63"), variant, accent); }
inline color purple(int variant = 500, bool accent = false) { return material(color::from_hex("#9c27b0"), variant, accent); }
inline color deep_purple(int variant = 500, bool accent = false) { return material(color::from_hex("#673ab7"), variant, accent); }
inline color indigo(int variant = 500, bool accent = false) { return material(color::from_hex("#3f51b5"), variant, accent); }
inline color blue(int variant = 500, bool accent = false) { return material(color::from_hex("#2196f3"), variant, accent); }
inline color light_blue(int variant = 500, bool accent = false) { return material(color::from_hex("#03a9f4"), variant, accent); }
inline color cyan(int variant = 500, bool accent = false) { return material(color::from_hex("#00bcd4"), variant, accent); }
inline color teal(int variant = 500, bool accent = false) { return material(color::from_hex("#009688"), variant, accent); }
inline color green(int variant = 500, bool accent = false) { return material(color::from_hex("#4caf50"), variant, accent); }
inline color light_green(int variant = 500, bool accent = false) { return material(color::from_hex("#8bc34a"), variant, accent); }
inline color lime(int variant = 500, bool accent = false) { return material(color::from_hex("#cddc39"), variant, accent); }
inline color yellow(int variant = 500, bool accent = false) { return material(color::from_hex("#ffeb3b"), variant, accent); }
inline color amber(int variant = 500, bool accent = false) { return material(color::from_hex("#ffc107"), variant, accent); }
inline color orange(int variant = 500, bool accent = false) { return material(color::from_hex("#ff9800"), variant, accent); }
inline color deep_orange(int variant = 500, bool accent = false) { return material(color::from_hex("#ff5722"), variant, accent); }

inline color brown(int variant = 500) { return material(color::from_hex("#795548"), variant); }
inline color grey(int variant = 500) { return material(color::from_hex("#9e9e9e"), variant); }
inline color blue_grey(int variant = 500) { return material(color::from_hex("#607d8b"), variant); }

// =============== IMPLEMENTATION ===============

inline color material(color c, int variant, bool accent)
{
    // from https://github.com/mbitson/mcg/blob/master/scripts/controllers/ColorGeneratorCtrl.js

    if (accent)
    {
        switch (variant)
        {
        case 100:
            return c.lightened(.50f).saturated(.30f);
        case 200:
            return c.lightened(.30f).saturated(.30f);
        case 400:
            return c.lightened(.10f).saturated(.15f);
        case 700:
            return c.lightened(.05f).saturated(.05f);
        default:
            TG_ASSERT(0 && "invalid variant");
            return {};
        }
    }
    else
    {
        switch (variant)
        {
        case 50:
            return c.lightened(.52f);
        case 10:
            return c.lightened(.37f);
        case 200:
            return c.lightened(.26f);
        case 300:
            return c.lightened(.12f);
        case 400:
            return c.lightened(.06f);
        case 500:
            return c;
        case 600:
            return c.darkened(.06f);
        case 700:
            return c.darkened(.12f);
        case 800:
            return c.darkened(.18f);
        case 900:
            return c.darkened(.24f);
        default:
            TG_ASSERT(0 && "invalid variant");
            return {};
        }
    }
}
}
}
}
