#pragma once

#include <glow/gl.hh>

#include <cstdint>
#include <string>

namespace glow
{
namespace detail
{
enum class glBaseType
{
    Float,
    Int,
    UInt,
    Double
};
}

/** Supported types:
 *
 * C++ basic types:
 *   * [u]int[8/16/32]_t
 *   * float
 *   * double
 *
 * Glm types:
 *   * [ iud]vec[234]
 *
 * IMPORTANT: if you get linker errors, this means the type is not supported (or missing implementation)
 */

/// Compile-time deduction of the OpenGL type equivalant of a given data type
/// e.g. "type" is GL_FLOAT for float and GL_INT for glm::ivec3
/// e.g. "size" is 1 for float and 3 for glm::ivec3
/// e.g. "format" is GL_RGBA for glm::vec4
template <typename DataT>
struct glTypeOf
{
    static GLenum type;
    static GLenum format; // NOT SUPPORTED FOR DOUBLE
    static GLint size;
    static detail::glBaseType basetype; // SUPPORTED FOR UNIFORM-TYPES
};

/// Converts a uniform type to string
std::string glUniformTypeToString(GLenum type);
std::string glUniformTypeToString(GLenum type, GLint size);
}
