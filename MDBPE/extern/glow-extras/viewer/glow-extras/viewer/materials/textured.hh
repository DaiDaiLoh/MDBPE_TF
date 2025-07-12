#pragma once

#include "Texturing.hh"

#include <polymesh/Mesh.hh>

namespace glow
{
namespace viewer
{
#if GLOW_HAS_GLM
inline Texturing textured(polymesh::vertex_attribute<glm::vec2> const& uv, glow::SharedTexture const& tex)
{
    return Texturing().texture(tex).coords(uv);
}
inline Texturing textured(polymesh::halfedge_attribute<glm::vec2> const& uv, glow::SharedTexture const& tex)
{
    return Texturing().texture(tex).coords(uv);
}
template <class T>
Texturing textured(polymesh::vertex_attribute<glm::vec2> const& uv, glow::AsyncTexture<T> const& tex)
{
    return Texturing().texture(tex).coords(uv);
}
template <class T>
Texturing textured(polymesh::halfedge_attribute<glm::vec2> const& uv, glow::AsyncTexture<T> const& tex)
{
    return Texturing().texture(tex).coords(uv);
}
#endif

inline Texturing textured(polymesh::vertex_attribute<tg::pos2> const& uv, glow::SharedTexture const& tex)
{
    return Texturing().texture(tex).coords(uv);
}
inline Texturing textured(polymesh::halfedge_attribute<tg::pos2> const& uv, glow::SharedTexture const& tex)
{
    return Texturing().texture(tex).coords(uv);
}

template <class T>
Texturing textured(polymesh::vertex_attribute<tg::pos2> const& uv, glow::AsyncTexture<T> const& tex)
{
    return Texturing().texture(tex).coords(uv);
}
template <class T>
Texturing textured(polymesh::halfedge_attribute<tg::pos2> const& uv, glow::AsyncTexture<T> const& tex)
{
    return Texturing().texture(tex).coords(uv);
}
}
}
