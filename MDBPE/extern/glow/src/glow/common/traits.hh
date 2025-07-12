#pragma once

#ifdef GLOW_HAS_GLM
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#endif

#ifdef GLOW_HAS_TG
#include <typed-geometry/types/color.hh>
#endif

namespace glow
{
namespace detail
{
#ifdef GLOW_HAS_GLM
inline tg::color4 get_color4(glm::vec3 v) { return {v.x, v.y, v.z, 1}; }
inline tg::color4 get_color4(glm::vec4 v) { return {v.x, v.y, v.z, v.w}; }
#endif
#ifdef GLOW_HAS_TG
inline tg::color4 get_color4(tg::color3 c) { return {c.r, c.g, c.b, 1}; }
inline tg::color4 get_color4(tg::color4 c) { return {c.r, c.g, c.b, c.a}; }
#endif
}
}
