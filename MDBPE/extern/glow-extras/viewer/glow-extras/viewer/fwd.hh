#pragma once

#include <glow/common/shared.hh>

namespace glow
{
namespace viewer
{
namespace detail
{
struct raii_view_closer;
GLOW_SHARED(class, MeshAttribute);
GLOW_SHARED(class, MeshDefinition);
class MeshShaderBuilder;
}

GLOW_SHARED(class, Scene);
struct SceneConfig;

GLOW_SHARED(class, Renderable);
GLOW_SHARED(class, GeometricRenderable);
GLOW_SHARED(class, MeshRenderable);
GLOW_SHARED(class, PointRenderable);
GLOW_SHARED(class, LineRenderable);
GLOW_SHARED(class, Vector2DRenderable);
GLOW_SHARED(class, TextureRenderable);
GLOW_SHARED(class, LabelRenderable);
GLOW_SHARED(class, QuadricRenderable);

class ColorMapping;
class Texturing;
class Masking;
class ViewerApp;
struct EnvMap;

struct label;
}

namespace colors
{
struct color;
}
}

namespace polymesh
{
class Mesh;
template <class AttrT>
struct vertex_attribute;
template <class AttrT>
struct face_attribute;
template <class AttrT>
struct edge_attribute;
template <class AttrT>
struct halfedge_attribute;
}
