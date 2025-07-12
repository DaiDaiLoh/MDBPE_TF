#pragma once

#include <iostream>
#include <string>

#include <typed-geometry/functions/basic/minmax.hh>
#include <typed-geometry/functions/basic/mix.hh>
#include <typed-geometry/tg-lean.hh>

#include "helper.hh"

namespace glow
{
namespace colors
{
enum class wcag2_size
{
    AA_large,
    AA_small,
    AAA_large,
    AAA_small
};

/// A color data type inspired by https://github.com/bgrins/TinyColor
///
/// Colors are always stored as 4 floating point values from 0..1
///
/// Notes:
/// * Colors are always in linear space!
/// * <verb>() modifies colors in-place
/// * <verb>ed() returns a modified color
/// * free functions <verb>(color) exist
///
/// TODO: analogous, triad, complement
struct color
{
    // data
public:
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    // ctor -> always take 0..1 values
public:
    color() = default;
    color(float v) : r(v), g(v), b(v), a(1.0f) {}
    color(float r, float g, float b, float a = 1.0f);
    color(tg::color3 const& v, float a = 1.0f);
    color(tg::color4 const& v);

    // factories
public:
    static color from_srgb(uint8_t r, uint8_t g, uint8_t b);
    static color from_srgb(tg::vec3 srgb); ///< 0..1^3
    static color from_srgb(uint32_t srgb); ///< 0xRRGGBB

    static color from_srgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static color from_srgba(tg::vec4 srgba); ///< 0..1^4
    static color from_srgba(uint32_t srgba); ///< 0xRRGGBBAA

    static color from_hsv(tg::vec4 hsva);                             ///< 0..360 x 0..1 x 0..1 x 0..1
    static color from_hsv(tg::vec3 hsv, float a = 1.0f);              ///< 0..360 x 0..1 x 0..1 x 0..1
    static color from_hsv(float h, float s, float v, float a = 1.0f); ///< 0..360 x 0..1 x 0..1 x 0..1

    static color from_hsl(tg::vec4 hsla);                             ///< 0..360 x 0..1 x 0..1 x 0..1
    static color from_hsl(tg::vec3 hsl, float a = 1.0f);              ///< 0..360 x 0..1 x 0..1 x 0..1
    static color from_hsl(float h, float s, float l, float a = 1.0f); ///< 0..360 x 0..1 x 0..1 x 0..1

    /// formats:
    /// - RRGGBB
    /// - RRGGBBAA
    /// - #RRGGBB
    /// - #RRGGBBAA
    /// - 0xRRGGBB
    /// - 0xRRGGBBAA
    static color from_hex(std::string const& h);

    // conversions
public:
    tg::color3 to_rgb() const;  ///< 0..1 x 0..1 x 0..1
    tg::color4 to_rgba() const; ///< 0..1 x 0..1 x 0..1 x 0..1

    tg::vec3 to_srgb() const;  ///< 0..1 x 0..1 x 0..1
    tg::vec4 to_srgba() const; ///< 0..1 x 0..1 x 0..1 x 0..1

    tg::vec3 to_hsv() const;  ///< 0..360 x 0..1 x 0..1
    tg::vec4 to_hsva() const; ///< 0..360 x 0..1 x 0..1 x 0..1

    tg::vec3 to_hsl() const;  ///< 0..360 x 0..1 x 0..1
    tg::vec4 to_hsla() const; ///< 0..360 x 0..1 x 0..1 x 0..1

    tg::vec3 to_lab() const; ///< TODO

    tg::color3 to_grayscale() const;   ///< 0..1 x 0..1 x 0..1
    tg::color4 to_grayscale_a() const; ///< 0..1 x 0..1 x 0..1 x 0..1

    // properties
public:
    float red() const { return r; }
    float green() const { return g; }
    float blue() const { return b; }
    float alpha() const { return a; }

    float brightness() const;     ///< 0..1, defined by https://www.w3.org/TR/AERT/#color-contrast
    float luminance() const;      ///< 0..1, linear space: r * .2126f + g * .7152f + b * .0722f
    float luma() const;           ///< 0..1,   srgb space: r * .2126f + g * .7152f + b * .0722f
    float hue() const;            ///< 0..360
    float saturation_hsv() const; ///< 0..1
    float saturation_hsl() const; ///< 0..1
    float value() const;          ///< 0..1
    float lightness() const;      ///< 0..1, average of r, g, b
    float chromacity() const;     ///< 0..1

    bool is_dark() const { return brightness() < 0.5f; }
    bool is_light() const { return brightness() >= 0.5f; }

    float contrast_to(color c) const; ///< 1..21 color contrast (WCAG Version 2)
    bool is_readable_on(color c, wcag2_size size) const;

    // const operations
public:
    color clamped(color min = {0, 0, 0, 0}, color max = {1, 1, 1, 1}) const;

    color desaturated(float amount = 0.1f) const; ///< decreases saturation (in HSL)
    color saturated(float amount = 0.1f) const;   ///< increases saturation (in HSL)
    color brightened(float amount = 0.1f) const;  ///< uniformly increase rgb
    color lightened(float amount = 0.1f) const;   ///< increases lightness
    color darkened(float amount = 0.1f) const;    ///< decreases lightness
    color rotated(float hue) const;               ///< increases hue (with wrap-around)

    color mixed(color c, float t) const; /// this * (1 - t) + c * t

    color inverted() const;

    // modifying operations
public:
    void clamp(color min = {0, 0, 0, 0}, color max = {1, 1, 1, 1});

    void desaturate(float amount = 0.1f);
    void saturate(float amount = 0.1f);
    void brighten(float amount = 0.1f);
    void lighten(float amount = 0.1f);
    void darken(float amount = 0.1f);
    void rotate(float hue);

    void mix(color c, float t);

    void invert();
};

// free functions
color clamp(color c, color min = {0, 0, 0, 0}, color max = {1, 1, 1, 1});

color desaturate(color c, float amount = 0.1f);
color saturate(color c, float amount = 0.1f);
color brighten(color c, float amount = 0.1f);
color lighten(color c, float amount = 0.1f);
color darken(color c, float amount = 0.1f);
color rotate(color c, float hue);

color mix(color c1, color c2, float t);

color invert(color c);

float contrast(color c1, color c2);
bool is_readable_on(color c1, color c2, wcag2_size size);

inline std::ostream& operator<<(std::ostream& out, color const& c)
{
    out << "rgba(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ")";
    return out;
}

// =============== IMPLEMENTATION ===============

inline color::color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
inline color::color(const tg::color3& v, float a) : r(v.r), g(v.g), b(v.b), a(a) {}
inline color::color(const tg::color4& v) : r(v.r), g(v.g), b(v.b), a(v.a) {}

inline color color::from_srgb(uint8_t r, uint8_t g, uint8_t b) { return from_srgb({r / 255.0f, g / 255.0f, b / 255.0f}); }

inline color color::from_srgb(tg::vec3 srgb)
{
    return {detail::srgb_to_linear(srgb.x), detail::srgb_to_linear(srgb.y), detail::srgb_to_linear(srgb.z), 1.0f};
}

inline color color::from_srgb(uint32_t srgb)
{
    auto b = uint8_t(srgb & 0xFF);
    srgb = srgb >> 8;
    auto g = uint8_t(srgb & 0xFF);
    srgb = srgb >> 8;
    auto r = uint8_t(srgb & 0xFF);
    return from_srgb(r, g, b);
}

inline color color::from_srgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return from_srgba({r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f}); }

inline color color::from_srgba(tg::vec4 srgba)
{
    return {detail::srgb_to_linear(srgba.x), detail::srgb_to_linear(srgba.y), detail::srgb_to_linear(srgba.z), srgba.w};
}

inline color color::from_srgba(uint32_t srgba)
{
    auto a = uint8_t(srgba & 0xFF);
    srgba = srgba >> 8;
    auto b = uint8_t(srgba & 0xFF);
    srgba = srgba >> 8;
    auto g = uint8_t(srgba & 0xFF);
    srgba = srgba >> 8;
    auto r = uint8_t(srgba & 0xFF);
    return from_srgba(r, g, b, a);
}

inline color color::from_hsv(tg::vec4 hsva) { return from_hsv(hsva.x, hsva.y, hsva.z, hsva.w); }

inline color color::from_hsv(tg::vec3 hsv, float a) { return from_hsv(hsv.x, hsv.y, hsv.z, a); }

inline color color::from_hsv(float h, float s, float v, float a)
{
    color c;
    c.a = a;

    if (s <= 0)
    {
        c.r = v;
        c.g = v;
        c.b = v;
    }
    else
    {
        auto sector = std::floor(h / 60.0f);
        auto frac = h / 60.0f - sector;
        auto sec = ((int)sector % 6 + 6) % 6;
        auto o = v * (1 - s);
        auto p = v * (1 - s * frac);
        auto q = v * (1 - s * (1 - frac));

        switch (sec)
        {
        default:
        case 0:
            c.r = v;
            c.g = q;
            c.b = o;
            break;
        case 1:
            c.r = p;
            c.g = v;
            c.b = o;
            break;
        case 2:
            c.r = o;
            c.g = v;
            c.b = q;
            break;
        case 3:
            c.r = o;
            c.g = p;
            c.b = v;
            break;
        case 4:
            c.r = q;
            c.g = o;
            c.b = v;
            break;
        case 5:
            c.r = v;
            c.g = o;
            c.b = p;
            break;
        }
    }

    return c;
}

inline color color::from_hsl(tg::vec4 hsla) { return from_hsl(hsla.x, hsla.y, hsla.z, hsla.w); }

inline color color::from_hsl(tg::vec3 hsl, float a) { return from_hsl(hsl.x, hsl.y, hsl.z, a); }

inline color color::from_hsl(float h, float s, float l, float a)
{
    if (s <= 0)
        return {l, l, l, a}; // achromatic

    h /= 360.0f;

    auto q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    auto p = 2 * l - q;

    auto hue_to_rgb = [&](float t) {
        if (t < 0)
            t += 1.0f;
        if (t > 1)
            t -= 1.0f;
        if (t < 1.0f / 6.0f)
            return p + (q - p) * 6 * t;
        if (t < 0.5f)
            return q;
        if (t < 2.0f / 3.0f)
            return p + (q - p) * (2.0f / 3.0f - t) * 6;
        return p;
    };

    return {
        hue_to_rgb(h + 1.0f / 3.0f), //
        hue_to_rgb(h),               //
        hue_to_rgb(h - 1.0f / 3.0f), //
        a                            //
    };
}

inline color color::from_hex(const std::string& h)
{
    auto has_a = false;

    auto s = h.c_str();
    if (s[0] == '#')
    {
        s += 1;
        has_a = h.size() > 7;
    }
    else if (s[1] == 'x')
    {
        s += 2;
        has_a = h.size() > 8;
    }
    else
    {
        has_a = h.size() > 6;
    }

    unsigned int r, g, b, a;
    a = 255;

#ifdef _MSC_VER
    if (has_a)
        sscanf_s(s, "%02x%02x%02x%02x", &r, &g, &b, &a);
    else
        sscanf_s(s, "%02x%02x%02x", &r, &g, &b);
#else
    if (has_a)
        sscanf(s, "%02x%02x%02x%02x", &r, &g, &b, &a);
    else
        sscanf(s, "%02x%02x%02x", &r, &g, &b);
#endif

    return from_srgba(uint8_t(r), uint8_t(g), uint8_t(b), uint8_t(a));
}

inline tg::color3 color::to_rgb() const { return {r, g, b}; }
inline tg::color4 color::to_rgba() const { return {r, g, b, a}; }

inline tg::vec3 color::to_srgb() const { return {detail::linear_to_srgb(r), detail::linear_to_srgb(g), detail::linear_to_srgb(b)}; }
inline tg::vec4 color::to_srgba() const { return {detail::linear_to_srgb(r), detail::linear_to_srgb(g), detail::linear_to_srgb(b), a}; }

inline tg::vec3 color::to_hsv() const { return {hue(), saturation_hsv(), value()}; }
inline tg::vec4 color::to_hsva() const { return {hue(), saturation_hsv(), value(), a}; }

inline tg::vec3 color::to_hsl() const { return {hue(), saturation_hsl(), lightness()}; }
inline tg::vec4 color::to_hsla() const { return {hue(), saturation_hsl(), lightness(), a}; }

inline tg::color3 color::to_grayscale() const
{
    auto l = lightness();
    return {l, l, l};
}

inline tg::color4 color::to_grayscale_a() const
{
    auto l = lightness();
    return {l, l, l, a};
}

inline float color::brightness() const { return r * .299f + g * .587f + b * .114f; }

inline float color::luminance() const { return r * .2126f + g * .7152f + b * .0722f; }

inline float color::luma() const
{
    auto sr = detail::linear_to_srgb(r);
    auto sg = detail::linear_to_srgb(g);
    auto sb = detail::linear_to_srgb(b);
    return sr * .2126f + sg * .7152f + sb * .0722f;
}

inline float color::hue() const
{
    auto max = tg::max(r, g, b);
    auto min = tg::min(r, g, b);

    auto d = max - min;

    if (d <= 0)
        return 0.0f; // achromatic

    if (max == r)
        return 60.0f * (g - b) / d + (g < b ? 360.0f : 0.0f);
    else if (max == g)
        return 60.0f * (b - r) / d + 120.0f;
    else
        return 60.0f * (r - g) / d + 240.0f;
}

inline float color::saturation_hsv() const
{
    auto max = tg::max(r, g, b);
    auto min = tg::min(r, g, b);

    return max <= 0 ? 0.0f : (max - min) / max;
}

inline float color::saturation_hsl() const
{
    auto max = tg::max(r, g, b);
    auto min = tg::min(r, g, b);

    auto d = max - min;
    auto l = (max + min) * 0.5f;

    return l > 0.5f ? d / (2 - max - min) : d / (max + min);
}

inline float color::value() const { return tg::max(r, g, b); }

inline float color::lightness() const { return (r + g + b) / 3.0f; }

inline float color::contrast_to(color c) const
{
    auto l0 = luminance();
    auto l1 = c.luminance();

    return (tg::max(l0, l1) + 0.05f) / (tg::min(l0, l1) + 0.05f);
}

inline bool color::is_readable_on(color c, wcag2_size size) const
{
    auto r = contrast_to(c);

    switch (size)
    {
    case wcag2_size::AA_small:
    case wcag2_size::AAA_large:
        return r >= 4.5f;
    case wcag2_size::AA_large:
        return r >= 3;
    case wcag2_size::AAA_small:
        return r >= 7;
    default:
        return false;
    }
}

inline color color::clamped(color min, color max) const
{
    return {
        tg::clamp(r, min.r, max.r), //
        tg::clamp(g, min.g, max.g), //
        tg::clamp(b, min.b, max.b), //
        tg::clamp(a, min.a, max.a)  //
    };
}

inline color color::desaturated(float amount) const
{
    auto hsla = to_hsla();
    hsla.y = tg::clamp(hsla.y - amount, 0.0f, 1.0f);
    return from_hsl(hsla);
}

inline color color::saturated(float amount) const
{
    auto hsla = to_hsla();
    hsla.y = tg::clamp(hsla.y + amount, 0.0f, 1.0f);
    return from_hsl(hsla);
}

inline color color::brightened(float amount) const
{
    auto c = *this;
    c.r += amount;
    c.g += amount;
    c.b += amount;
    return c.clamped();
}

inline color color::lightened(float amount) const
{
    auto hsla = to_hsla();
    hsla.z = tg::clamp(hsla.z + amount, 0.0f, 1.0f);
    return from_hsl(hsla);
}

inline color color::darkened(float amount) const
{
    auto hsla = to_hsla();
    hsla.z = tg::clamp(hsla.z - amount, 0.0f, 1.0f);
    return from_hsl(hsla);
}

inline color color::rotated(float hue) const
{
    auto hsla = to_hsla();
    auto new_h = std::fmod(hsla.x + hue, 360.0f);
    hsla.x = new_h < 0 ? 360.0f + new_h : new_h;
    return from_hsl(hsla);
}

inline color color::mixed(color c, float t) const
{
    return {
        tg::mix(r, c.r, t), //
        tg::mix(g, c.g, t), //
        tg::mix(b, c.b, t), //
        tg::mix(a, c.a, t)  //
    };
}

inline color color::inverted() const { return {1 - r, 1 - g, 1 - b, 1 - a}; }

inline void color::clamp(color min, color max) { *this = clamped(min, max); }
inline void color::desaturate(float amount) { *this = desaturated(amount); }
inline void color::saturate(float amount) { *this = saturated(amount); }
inline void color::brighten(float amount) { *this = brightened(amount); }
inline void color::lighten(float amount) { *this = lightened(amount); }
inline void color::darken(float amount) { *this = darkened(amount); }
inline void color::rotate(float hue) { *this = rotated(hue); }
inline void color::mix(color c, float t) { *this = mixed(c, t); }
inline void color::invert() { *this = inverted(); }

inline color clamp(color c, color min, color max) { return c.clamped(min, max); }
inline color desaturate(color c, float amount) { return c.desaturated(amount); }
inline color saturate(color c, float amount) { return c.saturated(amount); }
inline color brighten(color c, float amount) { return c.brightened(amount); }
inline color lighten(color c, float amount) { return c.lightened(amount); }
inline color darken(color c, float amount) { return c.darkened(amount); }
inline color rotate(color c, float hue) { return c.rotated(hue); }
inline color mix(color c1, color c2, float t) { return c1.mixed(c2, t); }
inline color invert(color c) { return c.inverted(); }
inline float contrast(color c1, color c2) { return c1.contrast_to(c2); }
inline bool is_readable_on(color c1, color c2, wcag2_size size) { return c1.is_readable_on(c2, size); }
} // namespace colors
} // namespace glow
