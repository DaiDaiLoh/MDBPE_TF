#pragma once

#if GLOW_HAS_GLM
#include <glm/glm.hpp>
#endif

#include <glow-extras/colors/color.hh>

#if GLOW_HAS_TG
#include <typed-geometry/tg-lean.hh>
#endif

namespace glow
{
namespace viewer
{
namespace detail
{
template <class T>
struct uniform_info
{
    static constexpr bool allowed = false;
    // static constexpr char const* shader_type = "<unsupported>";

    static T uniform(T const& v) { return v; }
};

#define GLOW_VIEWER_UNIFORM_PRIMITIVE_TYPE(name)          \
    template <>                                           \
    struct uniform_info<name>                             \
    {                                                     \
        static constexpr bool allowed = true;             \
        static constexpr char const* shader_type = #name; \
        static name uniform(name const& v) { return v; }  \
    } // force ;

#define GLOW_VIEWER_UNIFORM_MATH_TYPE(ns, name)                  \
    template <>                                                  \
    struct uniform_info<ns::name>                                \
    {                                                            \
        static constexpr bool allowed = true;                    \
        static constexpr char const* shader_type = #name;        \
        static ns::name uniform(ns::name const& v) { return v; } \
    } // force ;

#define GLOW_VIEWER_UNIFORM_MATH_TYPE2(ns, name, shadert)        \
    template <>                                                  \
    struct uniform_info<ns::name>                                \
    {                                                            \
        static constexpr bool allowed = true;                    \
        static constexpr char const* shader_type = #shadert;     \
        static ns::name uniform(ns::name const& v) { return v; } \
    } // force ;

GLOW_VIEWER_UNIFORM_PRIMITIVE_TYPE(bool);
GLOW_VIEWER_UNIFORM_PRIMITIVE_TYPE(int);
GLOW_VIEWER_UNIFORM_PRIMITIVE_TYPE(float);

#if GLOW_HAS_GLM
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, vec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, vec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, vec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, ivec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, ivec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, ivec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, uvec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, uvec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, uvec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, bvec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, bvec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, bvec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, mat2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, mat3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(glm, mat4);
#endif

GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, vec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, vec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, vec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, ivec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, ivec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, ivec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, uvec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, uvec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, uvec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, pos2, vec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, pos3, vec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, pos4, vec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, ipos2, ivec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, ipos3, ivec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, ipos4, ivec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, upos2, uvec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, upos3, uvec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, upos4, uvec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, dir2, vec2);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, dir3, vec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, dir4, vec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, color3, vec3);
GLOW_VIEWER_UNIFORM_MATH_TYPE2(tg, color4, vec4);

GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, mat2);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, mat3);
GLOW_VIEWER_UNIFORM_MATH_TYPE(tg, mat4);

template <>
struct uniform_info<colors::color>
{
    static constexpr bool allowed = true;
    static constexpr char const* shader_type = "vec4";
    static tg::color4 uniform(colors::color const& v) { return v.to_rgba(); }
    // TODO
};
}
}
}
