#include "view.hh"

#include <glow/common/str_utils.hh>
#include <glow/util/DefaultShaderParser.hh>

#ifdef GLOW_EXTRAS_EMBED_SHADERS
#include <glow-extras/generated/viewer_embed_shaders.hh>
#endif

namespace
{
std::vector<std::pair<std::string, std::string>> _globalViewerFonts;
}

std::vector<bool> glow::viewer::detail::raii_view_closer::sStackIsContainer;
std::vector<glow::viewer::detail::raii_config::config_func> glow::viewer::detail::raii_config::sConfigStack;
std::vector<bool*> glow::viewer::detail::raii_config::sConfigUsedStack;

void glow::viewer::global_init()
{
    static bool _isViewerInitialized = false;

    if (_isViewerInitialized)
        return;

#ifdef GLOW_EXTRAS_EMBED_SHADERS
    for (auto& virtualPair : internal_embedded_files::viewer_embed_shaders)
        DefaultShaderParser::addVirtualFile(virtualPair.first, virtualPair.second);
#else
    DefaultShaderParser::addIncludePath(util::pathOf(__FILE__) + "/../../shader");
#endif

    _isViewerInitialized = true;
}

void glow::viewer::global_add_vec2d_font(const std::string& name, const std::string& path) { _globalViewerFonts.push_back({name, path}); }
void glow::viewer::global_clear_vec2d_fonts() { _globalViewerFonts.clear(); }

const std::vector<std::pair<std::string, std::string>>& glow::viewer::detail::internal_global_get_fonts() { return _globalViewerFonts; }
