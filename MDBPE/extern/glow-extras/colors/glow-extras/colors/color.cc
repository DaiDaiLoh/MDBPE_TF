#include "color.hh"

#include <glow/common/gltypeinfo.hh>

namespace glow
{
template <>
GLenum glTypeOf<colors::color>::type = GL_FLOAT;
template <>
GLenum glTypeOf<colors::color>::format = GL_RGBA;
template <>
GLint glTypeOf<colors::color>::size = 4;
template <>
detail::glBaseType glTypeOf<colors::color>::basetype = detail::glBaseType::Float;
}
