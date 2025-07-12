#include "fonts.hh"

#include <glow/common/log.hh>

// defined in fonts/FontFiraSans.cc and fonts/FontFiraMono.cc
namespace glow::vector::detail
{
std::vector<std::byte> getFontFiraSans();
std::vector<std::byte> getFontFiraMono();
}

std::vector<std::byte> glow::vector::getDefaultFontSans()
{
#ifdef GLOW_EXTRAS_DEFAULT_FONTS
    return detail::getFontFiraSans();
#else
    glow::error() << "[glow-extas] GLOW_EXTRAS_DEFAULT_FONTS must be enabled to get default fonts";
    return {};
#endif
}

std::vector<std::byte> glow::vector::getDefaultFontMono()
{
#ifdef GLOW_EXTRAS_DEFAULT_FONTS
    return detail::getFontFiraMono();
#else
    glow::error() << "[glow-extas] GLOW_EXTRAS_DEFAULT_FONTS must be enabled to get default fonts";
    return {};
#endif
}
