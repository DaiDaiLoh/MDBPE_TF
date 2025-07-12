#pragma once

#ifdef GLOW_HAS_TG
#include <typed-geometry/tg-lean.hh>
#endif

#include <cstdint>

#include "common/alignment.hh"

/** This header implements the std140 layout functionality
 * See https://www.opengl.org/registry/specs/ARB/uniform_buffer_object.txt
 *
 * TODO: Allow file reuse in shader
 *
 * Usage: (all supported types)
 *
 * struct MyUniformBufferStruct
 * {
 *  // basic types
 *  std140bool     i;
 *  std140int      j;
 *  std140unsigned k;
 *  std140float    l;
 *
 *  // vectors
 *  std140vec2 v2;
 *  std140vec3 v3;
 *  std140vec4 v4;
 *
 *  std140ivec2 iv2;
 *  std140ivec3 iv3;
 *  std140ivec4 iv4;
 *
 *  std140bvec2 bv2;
 *  std140bvec3 bv3;
 *  std140bvec4 bv4;
 *
 *  std140dvec2 dv2;
 *  std140dvec3 dv3;
 *  std140dvec4 dv4;
 *
 *  std140uvec2 uv2;
 *  std140uvec3 uv3;
 *  std140uvec4 uv4;
 *
 *  // matrices
 *  std140mat2 m2;
 *  std140mat3 m3;
 *  std140mat4 m4;
 *
 *  std140mat2x2 m2x2;
 *  std140mat2x3 m2x3;
 *  std140mat2x4 m2x4;
 *  std140mat3x2 m3x2;
 *  std140mat3x3 m3x3;
 *  std140mat3x4 m3x4;
 *  std140mat4x2 m4x2;
 *  std140mat4x3 m4x3;
 *  std140mat4x4 m4x4;
 *
 *  std140dmat2 dm2;
 *  std140dmat3 dm3;
 *  std140dmat4 dm4;
 *
 *  std140dmat2x2 dm2x2;
 *  std140dmat2x3 dm2x3;
 *  std140dmat2x4 dm2x4;
 *  std140dmat3x2 dm3x2;
 *  std140dmat3x3 dm3x3;
 *  std140dmat3x4 dm3x4;
 *  std140dmat4x2 dm4x2;
 *  std140dmat4x3 dm4x3;
 *  std140dmat4x4 dm4x4;
 *
 *  // arrays
 *  // TODO
 *
 *  // structs
 *  // TODO
 * }
 *
 * CAUTION: even if it works for some types, you should ALWAYS use std140xyz for ALL members
 *
 * Writing and reading from these variables may be a bit slower due to awkward memory layout
 *
 * You cannot calculate directly on these wrapper types, BUT an implicit conversion exists
 */

namespace glow
{
#define GLOW_STD140_AUTO_TYPE(NAME, TYPE, ALIGN_IN_FLOATS) GLOW_STD140_EXPLICIT_TYPE(NAME, TYPE, TYPE, ALIGN_IN_FLOATS)
#define GLOW_STD140_EXPLICIT_TYPE(NAME, ACCESS_T, STORE_T, ALIGN_IN_FLOATS)   \
    namespace detail                                                          \
    {                                                                         \
    struct std140##NAME##_impl                                                \
    {                                                                         \
        std140##NAME##_impl() = default;                                      \
        std140##NAME##_impl(std140##NAME##_impl const&) = default;            \
        std140##NAME##_impl(std140##NAME##_impl&&) = default;                 \
                                                                              \
        std140##NAME##_impl& operator=(std140##NAME##_impl const&) = default; \
        std140##NAME##_impl& operator=(std140##NAME##_impl&&) = default;      \
                                                                              \
        std140##NAME##_impl(ACCESS_T const& t) : val(t) {}                    \
        operator ACCESS_T() const { return (ACCESS_T)val; }                   \
        std140##NAME##_impl& operator=(ACCESS_T const& t)                     \
        {                                                                     \
            val = STORE_T(t);                                                 \
            return *this;                                                     \
        }                                                                     \
        std140##NAME##_impl& operator=(ACCESS_T&& t)                          \
        {                                                                     \
            val = STORE_T(t);                                                 \
            return *this;                                                     \
        }                                                                     \
                                                                              \
    private:                                                                  \
        STORE_T val;                                                          \
    };                                                                        \
    }                                                                         \
    typedef GLOW_ALIGN_PRE(ALIGN_IN_FLOATS * 4) detail::std140##NAME##_impl std140##NAME GLOW_ALIGN_POST(ALIGN_IN_FLOATS * 4) // force ;

// scalars
GLOW_STD140_AUTO_TYPE(int, int32_t, 1);
GLOW_STD140_AUTO_TYPE(uint, uint32_t, 1);
GLOW_STD140_AUTO_TYPE(bool, bool, 1);
GLOW_STD140_AUTO_TYPE(float, float, 1);
GLOW_STD140_AUTO_TYPE(double, double, 2);

// comp-like
GLOW_STD140_AUTO_TYPE(vec2, tg::vec2, 2);
GLOW_STD140_AUTO_TYPE(vec3, tg::vec3, 4);
GLOW_STD140_AUTO_TYPE(vec4, tg::vec4, 4);

GLOW_STD140_AUTO_TYPE(ivec2, tg::ivec2, 2);
GLOW_STD140_AUTO_TYPE(ivec3, tg::ivec3, 4);
GLOW_STD140_AUTO_TYPE(ivec4, tg::ivec4, 4);

GLOW_STD140_AUTO_TYPE(uvec2, tg::uvec2, 2);
GLOW_STD140_AUTO_TYPE(uvec3, tg::uvec3, 4);
GLOW_STD140_AUTO_TYPE(uvec4, tg::uvec4, 4);

GLOW_STD140_AUTO_TYPE(dvec2, tg::dvec2, 2 * 2);
GLOW_STD140_AUTO_TYPE(dvec3, tg::dvec3, 4 * 2);
GLOW_STD140_AUTO_TYPE(dvec4, tg::dvec4, 4 * 2);


GLOW_STD140_AUTO_TYPE(dir2, tg::dir2, 2);
GLOW_STD140_AUTO_TYPE(dir3, tg::dir3, 4);
GLOW_STD140_AUTO_TYPE(dir4, tg::dir4, 4);

GLOW_STD140_AUTO_TYPE(idir2, tg::idir2, 2);
GLOW_STD140_AUTO_TYPE(idir3, tg::idir3, 4);
GLOW_STD140_AUTO_TYPE(idir4, tg::idir4, 4);

GLOW_STD140_AUTO_TYPE(ddir2, tg::ddir2, 2 * 2);
GLOW_STD140_AUTO_TYPE(ddir3, tg::ddir3, 4 * 2);
GLOW_STD140_AUTO_TYPE(ddir4, tg::ddir4, 4 * 2);


GLOW_STD140_AUTO_TYPE(pos2, tg::pos2, 2);
GLOW_STD140_AUTO_TYPE(pos3, tg::pos3, 4);
GLOW_STD140_AUTO_TYPE(pos4, tg::pos4, 4);

GLOW_STD140_AUTO_TYPE(ipos2, tg::ipos2, 2);
GLOW_STD140_AUTO_TYPE(ipos3, tg::ipos3, 4);
GLOW_STD140_AUTO_TYPE(ipos4, tg::ipos4, 4);

GLOW_STD140_AUTO_TYPE(upos2, tg::upos2, 2);
GLOW_STD140_AUTO_TYPE(upos3, tg::upos3, 4);
GLOW_STD140_AUTO_TYPE(upos4, tg::upos4, 4);

GLOW_STD140_AUTO_TYPE(dpos2, tg::dpos2, 2 * 2);
GLOW_STD140_AUTO_TYPE(dpos3, tg::dpos3, 4 * 2);
GLOW_STD140_AUTO_TYPE(dpos4, tg::dpos4, 4 * 2);


GLOW_STD140_AUTO_TYPE(comp2, tg::comp2, 2);
GLOW_STD140_AUTO_TYPE(comp3, tg::comp3, 4);
GLOW_STD140_AUTO_TYPE(comp4, tg::comp4, 4);

GLOW_STD140_AUTO_TYPE(icomp2, tg::icomp2, 2);
GLOW_STD140_AUTO_TYPE(icomp3, tg::icomp3, 4);
GLOW_STD140_AUTO_TYPE(icomp4, tg::icomp4, 4);

GLOW_STD140_AUTO_TYPE(ucomp2, tg::ucomp2, 2);
GLOW_STD140_AUTO_TYPE(ucomp3, tg::ucomp3, 4);
GLOW_STD140_AUTO_TYPE(ucomp4, tg::ucomp4, 4);

GLOW_STD140_AUTO_TYPE(dcomp2, tg::dcomp2, 2 * 2);
GLOW_STD140_AUTO_TYPE(dcomp3, tg::dcomp3, 4 * 2);
GLOW_STD140_AUTO_TYPE(dcomp4, tg::dcomp4, 4 * 2);


GLOW_STD140_AUTO_TYPE(size2, tg::size2, 2);
GLOW_STD140_AUTO_TYPE(size3, tg::size3, 4);
GLOW_STD140_AUTO_TYPE(size4, tg::size4, 4);

GLOW_STD140_AUTO_TYPE(isize2, tg::isize2, 2);
GLOW_STD140_AUTO_TYPE(isize3, tg::isize3, 4);
GLOW_STD140_AUTO_TYPE(isize4, tg::isize4, 4);

GLOW_STD140_AUTO_TYPE(usize2, tg::usize2, 2);
GLOW_STD140_AUTO_TYPE(usize3, tg::usize3, 4);
GLOW_STD140_AUTO_TYPE(usize4, tg::usize4, 4);

GLOW_STD140_AUTO_TYPE(dsize2, tg::dsize2, 2 * 2);
GLOW_STD140_AUTO_TYPE(dsize3, tg::dsize3, 4 * 2);
GLOW_STD140_AUTO_TYPE(dsize4, tg::dsize4, 4 * 2);


GLOW_STD140_AUTO_TYPE(color3, tg::color3, 4);
GLOW_STD140_AUTO_TYPE(color4, tg::color4, 4);


// matrices
GLOW_STD140_EXPLICIT_TYPE(mat4, tg::mat4, tg::mat4x4, 4);

GLOW_STD140_EXPLICIT_TYPE(mat2x4, tg::mat2x4, tg::mat2x4, 4);

GLOW_STD140_EXPLICIT_TYPE(mat3x4, tg::mat3x4, tg::mat3x4, 4);

GLOW_STD140_EXPLICIT_TYPE(mat4x4, tg::mat4x4, tg::mat4x4, 4);

GLOW_STD140_EXPLICIT_TYPE(dmat2, tg::dmat2, tg::dmat2x2, 2 * 2);
GLOW_STD140_EXPLICIT_TYPE(dmat4, tg::dmat4, tg::dmat4x4, 4 * 2);

GLOW_STD140_EXPLICIT_TYPE(dmat2x2, tg::dmat2x2, tg::dmat2x2, 2 * 2);
GLOW_STD140_EXPLICIT_TYPE(dmat2x4, tg::dmat2x4, tg::dmat2x4, 4 * 2);

GLOW_STD140_EXPLICIT_TYPE(dmat3x2, tg::dmat3x2, tg::dmat3x2, 2 * 2);
GLOW_STD140_EXPLICIT_TYPE(dmat3x4, tg::dmat3x4, tg::dmat3x4, 4 * 2);

GLOW_STD140_EXPLICIT_TYPE(dmat4x2, tg::dmat4x2, tg::dmat4x2, 2 * 2);
GLOW_STD140_EXPLICIT_TYPE(dmat4x4, tg::dmat4x4, tg::dmat4x4, 4 * 2);
}
