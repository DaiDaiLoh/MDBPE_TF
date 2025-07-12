#pragma once

#include <vector>

#include "renderables/LabelRenderable.hh"
#include "renderables/LineRenderable.hh"
#include "renderables/MeshRenderable.hh"
#include "renderables/PointRenderable.hh"
#include "renderables/QuadricRenderable.hh"
#include "renderables/Renderable.hh"
#include "renderables/TextureRenderable.hh"
#include "renderables/Vector2DRenderable.hh"

#include "fwd.hh"
#include "objects/objects.hh"
#include "traits.hh"

#include <glow-extras/vector/fwd.hh>
#include <glow-extras/viewer/objects/other/file.hh>
#include <glow/fwd.hh>

#include <typed-geometry/tg-lean.hh>

namespace glow
{
namespace viewer
{
// view(renderable)
// identity
template <class RenderableT, class = std::enable_if_t<std::is_base_of_v<Renderable, RenderableT>>>
std::shared_ptr<RenderableT> make_renderable(std::shared_ptr<RenderableT> const& r)
{
    return r;
}

// view(pos, ...)
// automatically chooses polygons(pos), lines(pos), or points(pos), depending on mesh topology
template <class Pos3, class = std::enable_if_t<detail::is_pos3_like<Pos3>>>
SharedGeometricRenderable make_renderable(pm::vertex_attribute<Pos3> const& pos);

// view(polygons(...), ...)
SharedMeshRenderable make_renderable(builder::PolygonBuilder const& builder);
// view(points(...), ...)
SharedPointRenderable make_renderable(builder::PointBuilder const& builder);
// view(lines(...), ...)
SharedLineRenderable make_renderable(builder::LineBuilder const& builder);

// view(gv::file("path/to/mesh.file"), ...)
// automatically chooses polygons(gv::file("path/to/mesh.file")), lines(gv::file("path/to/mesh.file")), or points(gv::file("path/to/mesh.file")), depending on mesh topology
SharedGeometricRenderable make_renderable(file const& file);

// 2D vector gfx
SharedVector2DRenderable make_renderable(glow::vector::image2D const& img);

// textures
SharedTextureRenderable make_renderable(glow::SharedTexture2D const& tex);
SharedTextureRenderable make_renderable(glow::SharedTextureRectangle const& tex);

// Default interpretations for collections. view(pts, ...) (e.g. for std::vector<glm::vec3> pts)
template <class Pos3, class = std::enable_if_t<detail::is_pos3_like<Pos3>>>
SharedPointRenderable make_renderable(std::vector<Pos3> const& pos);
template <class Pos3, class = std::enable_if_t<detail::is_pos3_like<Pos3>>>
SharedLineRenderable make_renderable(std::vector<std::pair<Pos3, Pos3>> const& pos);
template <class Pos3, class = std::enable_if_t<detail::is_pos3_like<Pos3>>>
SharedLineRenderable make_renderable(std::vector<std::array<Pos3, 2>> const& pos);
template <class Pos3, int N, class = std::enable_if_t<detail::is_pos3_like<Pos3> && N >= 3>>
SharedMeshRenderable make_renderable(std::vector<std::array<Pos3, N>> const& pos);
template <class Pos3, class = std::enable_if_t<detail::is_pos3_like<Pos3>>>
SharedMeshRenderable make_renderable(std::vector<std::vector<Pos3>> const& pos);

template <class Pos3, class = std::enable_if_t<detail::is_pos3_like<Pos3>>>
SharedPointRenderable make_renderable(Pos3 const& pos);

// quadrics
SharedQuadricRenderable make_renderable(boxed_quadric const& q);
SharedQuadricRenderable make_renderable(array_view<boxed_quadric const> qs);


// labels
SharedLabelRenderable make_renderable(label const& label);
SharedLabelRenderable make_renderable(glow::array_view<label const> labels);

// ================== TG Support ==================

// tg:pos already works
template <int D, class ScalarT>
SharedLineRenderable make_renderable(tg::segment<D, ScalarT> const& seg)
{
    return make_renderable(viewer::lines(seg));
}
template <int D, class ScalarT>
SharedLineRenderable make_renderable(std::vector<tg::segment<D, ScalarT>> const& segs)
{
    return make_renderable(viewer::lines(segs));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(tg::triangle<D, ScalarT> const& tri)
{
    return make_renderable(viewer::polygons(tri));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::triangle<D, ScalarT>> const& tris)
{
    return make_renderable(viewer::polygons(tris));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(tg::quad<D, ScalarT> const& quads)
{
    return make_renderable(viewer::polygons(quads));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::quad<D, ScalarT>> const& quads)
{
    return make_renderable(viewer::polygons(quads));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(tg::aabb<D, ScalarT> const& aabb)
{
    return make_renderable(viewer::polygons(aabb));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::aabb<D, ScalarT>> const& aabbs)
{
    return make_renderable(viewer::polygons(aabbs));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(tg::box<D, ScalarT> const& box)
{
    return make_renderable(viewer::polygons(box));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::box<D, ScalarT>> const& boxes)
{
    return make_renderable(viewer::polygons(boxes));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(tg::sphere<D, ScalarT> const& sphere)
{
    return make_renderable(viewer::polygons(sphere));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::sphere<D, ScalarT>> const& spheres)
{
    return make_renderable(viewer::polygons(spheres));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(tg::cylinder<D, ScalarT> const& cylinder)
{
    return make_renderable(viewer::polygons(cylinder));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::cylinder<D, ScalarT>> const& cylinders)
{
    return make_renderable(viewer::polygons(cylinders));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(tg::cylinder_boundary_no_caps<D, ScalarT> const& tube)
{
    return make_renderable(viewer::polygons(tube));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::cylinder_boundary_no_caps<D, ScalarT>> const& tubes)
{
    return make_renderable(viewer::polygons(tubes));
}
template <class ScalarT>
SharedMeshRenderable make_renderable(tg::pyramid<tg::sphere<2, ScalarT, 3>> const& cone)
{
    return make_renderable(viewer::polygons(cone));
}
template <int D, class ScalarT>
SharedMeshRenderable make_renderable(std::vector<tg::cone<D, ScalarT>> const& cones)
{
    return make_renderable(viewer::polygons(cones));
}

// ================== Implementation ==================

template <class Pos3, class>
SharedGeometricRenderable make_renderable(polymesh::vertex_attribute<Pos3> const& pos)
{
    // mesh with faces-> MeshRenderable
    if (!pos.mesh().faces().empty())
        return make_renderable(viewer::polygons(pos));

    // mesh with edges but no faces -> LineRenderable
    else if (!pos.mesh().edges().empty())
        return make_renderable(viewer::lines(pos));

    // mesh with points but no faces/edges -> PointRenderable
    else
        return make_renderable(viewer::points(pos));
}

template <class Pos3, class>
SharedPointRenderable make_renderable(Pos3 const& pos)
{
    return make_renderable(viewer::points(std::vector<Pos3>{pos}));
}

template <class Pos3, class>
SharedPointRenderable make_renderable(std::vector<Pos3> const& pos)
{
    return make_renderable(viewer::points(pos));
}
template <class Pos3, class>
SharedLineRenderable make_renderable(std::vector<std::pair<Pos3, Pos3>> const& pos)
{
    return make_renderable(viewer::lines(pos));
}
template <class Pos3, class>
SharedLineRenderable make_renderable(std::vector<std::array<Pos3, 2>> const& pos)
{
    return make_renderable(viewer::lines(pos));
}
template <class Pos3, int N, class>
SharedMeshRenderable make_renderable(std::vector<std::array<Pos3, N>> const& pos)
{
    return make_renderable(viewer::polygons(pos));
}
template <class Pos3, class>
SharedMeshRenderable make_renderable(std::vector<std::vector<Pos3>> const& pos)
{
    return make_renderable(viewer::polygons(pos));
}

// feature test
template <class RenderableT>
auto detail_can_make_renderable(RenderableT&& r) -> decltype(make_renderable(r), std::true_type{});
template <class RenderableT>
auto detail_can_make_renderable(...) -> std::false_type;
template <class RenderableT>
static constexpr bool can_make_renderable = decltype(glow::viewer::detail_can_make_renderable<RenderableT>(std::declval<RenderableT>()))::value;
}
}
