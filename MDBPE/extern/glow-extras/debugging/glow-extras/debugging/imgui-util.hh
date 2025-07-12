#pragma once

#ifdef GLOW_EXTRAS_HAS_IMGUI

#include <string>

#include <typed-geometry/types/span.hh>

namespace glow::debugging
{
void applyGlowImguiTheme(bool darkMode = false);

namespace imgui
{
/// ImGui::Combo for array_like<string>
bool Combo(const char* label, int* currIndex, tg::span<std::string> values);
/// ImGui::ListBox for array_like<string>
bool ListBox(const char* label, int* currIndex, tg::span<std::string> values);
}
}

#endif
