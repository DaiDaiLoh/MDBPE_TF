#pragma once

#include <clean-core/optional.hh>

#include <glow-extras/viewer/objects/other/file.hh>
#include <glow/fwd.hh>
#include <iostream>
#include <polymesh/Mesh.hh>
#include <polymesh/objects/cone.hh>
#include <polymesh/objects/cube.hh>
#include <polymesh/objects/cylinder.hh>
#include <polymesh/objects/uv_sphere.hh>
#include <typed-geometry/tg-lean.hh>
#include <typed-geometry/tg.hh>

#include "../aabb.hh"
#include "../fwd.hh"
#include "../traits.hh"
#include "MeshInfo.hh"

namespace glow
{
namespace viewer
{
namespace detail
{
/// base class representing mesh topo and positions
class MeshDefinition
{
public:
    virtual ~MeshDefinition() = default;

    virtual aabb computeAABB() = 0;
    virtual SharedMeshAttribute computePositionAttribute() = 0;
    virtual glow::SharedElementArrayBuffer computeIndexBuffer() = 0;

    virtual SharedMeshAttribute computeFaceNormalAttribute() { throw std::logic_error("not supported"); }
    virtual SharedMeshAttribute computeSmoothNormalAttribute() { throw std::logic_error("not supported"); }

    virtual bool isEmpty() const = 0;

    size_t queryHash() const
    {
        if (!mCachedHash.has_value())
            mCachedHash = computeHash();
        return mCachedHash.value();
    }

protected:
    virtual size_t computeHash() const = 0;
    mutable cc::optional<size_t> mCachedHash;
};

class PolyMeshDefinition : public MeshDefinition
{
public:
    MeshInfo info;
    pm::Mesh mesh;
    pm::vertex_attribute<tg::pos3> pos{mesh};

    aabb computeAABB() override;
    SharedMeshAttribute computePositionAttribute() override;
    glow::SharedElementArrayBuffer computeIndexBuffer() override;

    SharedMeshAttribute computeFaceNormalAttribute() override;
    SharedMeshAttribute computeSmoothNormalAttribute() override;

    size_t computeHash() const override;

    bool isEmpty() const override
    {
        switch (info.type)
        {
        case MeshType::PolyMesh:
            return mesh.vertices().empty();
        case MeshType::PolygonList:
            return mesh.faces().empty();
        case MeshType::LineList:
            return mesh.edges().empty();
        case MeshType::PointList:
            return mesh.vertices().empty();
        case MeshType::Unknown:
        default:
            return true;
        }
    }
};

template <class Pos3, class = std::enable_if_t<is_pos3_like<Pos3>>>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(pm::vertex_attribute<Pos3> const& pos)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info = make_mesh_info(pos.mesh());

    pm->mesh.copy_from(pos.mesh());
    for (pm::vertex_index v : pos.mesh().vertices())
    {
        auto const& p = pos[v];
        pm->pos[v] = tg::pos3(p[0], p[1], p[2]);
    }

    return pm;
}
template <class Pos3, class = std::enable_if_t<is_pos3_like<Pos3>>>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(Pos3 const& p)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PointList;
    pm->info.vertexCount = 1;

    auto v = pm->mesh.vertices().add();
    pm->pos[v] = tg::pos3(p[0], p[1], p[2]);

    return pm;
}
template <class Pos3, class = std::enable_if_t<is_pos3_like<Pos3>>>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<Pos3> const& pos)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PointList;
    pm->info.vertexCount = int(pos.size());

    for (auto const& p : pos)
    {
        auto v = pm->mesh.vertices().add();
        pm->pos[v] = tg::pos3(p[0], p[1], p[2]);
    }

    return pm;
}
template <class Pos3, class = std::enable_if_t<is_pos3_like<Pos3>>>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<std::vector<Pos3>> const& pos)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;
    pm->info.vertexCount = 0;
    pm->info.faceCount = int(pos.size());

    std::vector<pm::vertex_index> face;
    for (auto const& poly : pos)
    {
        face.clear();

        for (auto const& p : poly)
        {
            auto v = pm->mesh.vertices().add();
            pm->pos[v] = tg::pos3(p[0], p[1], p[2]);
            face.push_back(v);
            pm->info.vertexCount++;
        }

        pm->mesh.faces().add(face);
    }

    return pm;
}
template <class Pos3, class = std::enable_if_t<is_pos3_like<Pos3>>>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<std::array<Pos3, 2>> const& pos)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::LineList;
    pm->info.vertexCount = int(2 * pos.size());
    pm->info.edgeCount = int(pos.size());
    pm->info.faceCount = 0;

    for (auto const& seg : pos)
    {
        auto v0 = pm->mesh.vertices().add();
        auto v1 = pm->mesh.vertices().add();
        pm->mesh.edges().add_or_get(v0, v1);

        pm->pos[v0] = tg::pos3(seg[0][0], seg[0][1], seg[0][2]);
        pm->pos[v1] = tg::pos3(seg[1][0], seg[1][1], seg[1][2]);
    }

    return pm;
}
template <class Pos3, int N, class = std::enable_if_t<is_pos3_like<Pos3>>>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<std::array<Pos3, N>> const& pos)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;
    pm->info.vertexCount = 0;
    pm->info.faceCount = int(pos.size());

    std::vector<pm::vertex_index> face;
    for (auto const& poly : pos)
    {
        face.clear();

        for (auto const& p : poly)
        {
            auto v = pm->mesh.vertices().add();
            pm->pos[v] = tg::pos3(p[0], p[1], p[2]);
            face.push_back(v);
            pm->info.vertexCount++;
        }

        pm->mesh.faces().add(face);
    }

    return pm;
}


template <class Pos3, class = std::enable_if_t<is_pos3_like<Pos3>>>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<std::pair<Pos3, Pos3>> const& pos)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::LineList;
    pm->info.vertexCount = int(2 * pos.size());
    pm->info.edgeCount = int(pos.size());
    pm->info.faceCount = 0;

    for (auto const& seg : pos)
    {
        auto v0 = pm->mesh.vertices().add();
        auto v1 = pm->mesh.vertices().add();
        pm->mesh.edges().add_or_get(v0, v1);

        pm->pos[v0] = tg::pos3(seg.first[0], seg.first[1], seg.first[2]);
        pm->pos[v1] = tg::pos3(seg.second[0], seg.second[1], seg.second[2]);
    }

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::segment<D, ScalarT>> const& segs)
{
    static_assert(D == 2 || D == 3, "currently only 2D and 3D supported");

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::LineList;
    pm->info.vertexCount = 2 * int(segs.size());
    pm->info.edgeCount = int(segs.size());

    for (auto const& seg : segs)
    {
        auto v0 = pm->mesh.vertices().add();
        auto v1 = pm->mesh.vertices().add();
        pm->mesh.edges().add_or_get(v0, v1);

        switch (D)
        {
        case 2:
            pm->pos[v0] = tg::pos3(seg.pos0[0], 0, seg.pos0[1]);
            pm->pos[v1] = tg::pos3(seg.pos1[0], 0, seg.pos1[1]);
            break;
        case 3:
            pm->pos[v0] = tg::pos3(seg.pos0[0], seg.pos0[1], seg.pos0[2]);
            pm->pos[v1] = tg::pos3(seg.pos1[0], seg.pos1[1], seg.pos1[2]);
            break;
        }
    }

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::segment<D, ScalarT> const& seg)
{
    return make_mesh_definition(std::vector<tg::segment<D, ScalarT>>{seg});
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::triangle<D, ScalarT>> const& tris)
{
    static_assert(D == 2 || D == 3, "currently only 2D and 3D supported");

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;
    pm->info.vertexCount = 3 * int(tris.size());
    pm->info.edgeCount = 3 * int(tris.size());
    pm->info.faceCount = int(tris.size());

    for (auto const& tri : tris)
    {
        auto v0 = pm->mesh.vertices().add();
        auto v1 = pm->mesh.vertices().add();
        auto v2 = pm->mesh.vertices().add();
        pm->mesh.faces().add(v0, v1, v2);

        switch (D)
        {
        case 2:
            pm->pos[v0] = tg::pos3(tri.pos0[0], 0, tri.pos0[1]);
            pm->pos[v1] = tg::pos3(tri.pos1[0], 0, tri.pos1[1]);
            pm->pos[v2] = tg::pos3(tri.pos2[0], 0, tri.pos2[1]);
            break;
        case 3:
            pm->pos[v0] = tg::pos3(tri.pos0[0], tri.pos0[1], tri.pos0[2]);
            pm->pos[v1] = tg::pos3(tri.pos1[0], tri.pos1[1], tri.pos1[2]);
            pm->pos[v2] = tg::pos3(tri.pos2[0], tri.pos2[1], tri.pos2[2]);
            break;
        }
    }

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::triangle<D, ScalarT> const& tri)
{
    return make_mesh_definition(std::vector<tg::triangle<D, ScalarT>>{tri});
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::quad<D, ScalarT>> const& quads)
{
    static_assert(D == 2 || D == 3, "currently only 2D and 3D supported");

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;
    pm->info.vertexCount = 4 * int(quads.size());
    pm->info.edgeCount = 4 * int(quads.size());
    pm->info.faceCount = int(quads.size());

    for (auto const& q : quads)
    {
        auto v00 = pm->mesh.vertices().add();
        auto v01 = pm->mesh.vertices().add();
        auto v10 = pm->mesh.vertices().add();
        auto v11 = pm->mesh.vertices().add();
        pm->mesh.faces().add(v00, v10, v11, v01);

        auto to_pos = [](tg::pos<D, ScalarT> p) -> tg::pos3 {
            if constexpr (D == 2)
                return tg::pos3(p.x, 0, p.y);
            else
                return tg::pos3(p.x, p.y, p.z);
        };

        pm->pos[v00] = to_pos(q.pos00);
        pm->pos[v01] = to_pos(q.pos01);
        pm->pos[v10] = to_pos(q.pos10);
        pm->pos[v11] = to_pos(q.pos11);
    }

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::quad<D, ScalarT> const& quads)
{
    return make_mesh_definition(std::vector<tg::quad<D, ScalarT>>{quads});
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::aabb<D, ScalarT>> const& aabbs)
{
    static_assert(D == 3, "currently only 3D supported");

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;
    pm->info.vertexCount = 8 * int(aabbs.size());
    pm->info.edgeCount = 12 * int(aabbs.size());
    pm->info.faceCount = 6 * int(aabbs.size());

    auto mix3 = [](tg::pos3 a, tg::pos3 b, tg::vec3 t) {
        return tg::pos3(tg::mix(a.x, b.x, t.x), //
                        tg::mix(a.y, b.y, t.y), //
                        tg::mix(a.z, b.z, t.z));
    };

    for (auto const& b : aabbs)
    {
        auto v000 = pm->mesh.vertices().add();
        auto v001 = pm->mesh.vertices().add();
        auto v010 = pm->mesh.vertices().add();
        auto v011 = pm->mesh.vertices().add();
        auto v100 = pm->mesh.vertices().add();
        auto v101 = pm->mesh.vertices().add();
        auto v110 = pm->mesh.vertices().add();
        auto v111 = pm->mesh.vertices().add();

        pm->mesh.faces().add(v000, v010, v110, v100);
        pm->mesh.faces().add(v000, v100, v101, v001);
        pm->mesh.faces().add(v000, v001, v011, v010);

        pm->mesh.faces().add(v001, v101, v111, v011);
        pm->mesh.faces().add(v010, v011, v111, v110);
        pm->mesh.faces().add(v100, v110, v111, v101);

        pm->pos[v000] = mix3(b.min, b.max, {0, 0, 0});
        pm->pos[v001] = mix3(b.min, b.max, {0, 0, 1});
        pm->pos[v010] = mix3(b.min, b.max, {0, 1, 0});
        pm->pos[v011] = mix3(b.min, b.max, {0, 1, 1});
        pm->pos[v100] = mix3(b.min, b.max, {1, 0, 0});
        pm->pos[v101] = mix3(b.min, b.max, {1, 0, 1});
        pm->pos[v110] = mix3(b.min, b.max, {1, 1, 0});
        pm->pos[v111] = mix3(b.min, b.max, {1, 1, 1});
    }

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::aabb<D, ScalarT> const& aabb)
{
    return make_mesh_definition(std::vector<tg::aabb<D, ScalarT>>{aabb});
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::box<D, ScalarT>> const& aabbs)
{
    static_assert(D == 3, "currently only 3D supported");

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;
    pm->info.vertexCount = 8 * int(aabbs.size());
    pm->info.edgeCount = 12 * int(aabbs.size());
    pm->info.faceCount = 6 * int(aabbs.size());

    for (auto const& b : aabbs)
    {
        auto v000 = pm->mesh.vertices().add();
        auto v001 = pm->mesh.vertices().add();
        auto v010 = pm->mesh.vertices().add();
        auto v011 = pm->mesh.vertices().add();
        auto v100 = pm->mesh.vertices().add();
        auto v101 = pm->mesh.vertices().add();
        auto v110 = pm->mesh.vertices().add();
        auto v111 = pm->mesh.vertices().add();

        pm->mesh.faces().add(v000, v010, v110, v100);
        pm->mesh.faces().add(v000, v100, v101, v001);
        pm->mesh.faces().add(v000, v001, v011, v010);

        pm->mesh.faces().add(v001, v101, v111, v011);
        pm->mesh.faces().add(v010, v011, v111, v110);
        pm->mesh.faces().add(v100, v110, v111, v101);

        pm->pos[v000] = (b.center - b.half_extents[0] - b.half_extents[1] - b.half_extents[2]);
        pm->pos[v001] = (b.center - b.half_extents[0] - b.half_extents[1] + b.half_extents[2]);
        pm->pos[v010] = (b.center - b.half_extents[0] + b.half_extents[1] - b.half_extents[2]);
        pm->pos[v011] = (b.center - b.half_extents[0] + b.half_extents[1] + b.half_extents[2]);
        pm->pos[v100] = (b.center + b.half_extents[0] - b.half_extents[1] - b.half_extents[2]);
        pm->pos[v101] = (b.center + b.half_extents[0] - b.half_extents[1] + b.half_extents[2]);
        pm->pos[v110] = (b.center + b.half_extents[0] + b.half_extents[1] - b.half_extents[2]);
        pm->pos[v111] = (b.center + b.half_extents[0] + b.half_extents[1] + b.half_extents[2]);
    }

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::box<D, ScalarT> const& b)
{
    return make_mesh_definition(std::vector<tg::box<D, ScalarT>>{b});
}

template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::sphere<D, ScalarT>> const& spheres)
{
    static_assert(D == 3, "currently only 3D supported");

    // TODO: proper ray-traced spheres

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;

    for (auto const& s : spheres)
    {
        pm::objects::add_uv_sphere(
            pm->mesh,
            [&](pm::vertex_handle v, float x, float y) {
                auto [sx, cx] = tg::sin_cos(x * 360_deg);
                auto [sy, cy] = tg::sin_cos(y * 180_deg);
                auto& p = pm->pos[v];
                p.x = s.center.x + s.radius * sy * sx;
                p.y = s.center.y + s.radius * cy;
                p.z = s.center.z + s.radius * sy * cx;
            },
            32, 32);
    }

    POLYMESH_ASSERT(pm->mesh.is_compact());
    pm->info.vertexCount = pm->mesh.vertices().size();
    pm->info.edgeCount = pm->mesh.edges().size();
    pm->info.faceCount = pm->mesh.faces().size();

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::sphere<D, ScalarT> const& s)
{
    return make_mesh_definition(std::vector<tg::sphere<D, ScalarT>>{s});
}

template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::cylinder<D, ScalarT>> const& cylinders)
{
    static_assert(D == 3, "currently only 3D supported");

    // TODO: proper ray-traced cylinders

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;

    for (auto const& c : cylinders)
    {
        auto dir = normalize(c.axis.pos1 - c.axis.pos0);
        auto t0 = any_normal(dir);
        auto t1 = cross(dir, t0);

        pm::objects::add_cylinder(
            pm->mesh,
            [&](pm::vertex_handle v, float x, float y) {
                auto [sx, cx] = tg::sin_cos(x * 360_deg);
                auto& p = pm->pos[v];
                p = c.axis[y] + (t0 * sx + t1 * cx) * c.radius;
            },
            32);
    }

    POLYMESH_ASSERT(pm->mesh.is_compact());
    pm->info.vertexCount = pm->mesh.vertices().size();
    pm->info.edgeCount = pm->mesh.edges().size();
    pm->info.faceCount = pm->mesh.faces().size();

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::cylinder<D, ScalarT> const& s)
{
    return make_mesh_definition(std::vector<tg::cylinder<D, ScalarT>>{s});
}

template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::cylinder_boundary_no_caps<D, ScalarT>> const& tubes)
{
    static_assert(D == 3, "currently only 3D supported");

    // TODO: proper ray-traced tubes

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;

    for (auto const& c : tubes)
    {
        auto dir = normalize(c.axis.pos1 - c.axis.pos0);
        auto t0 = any_normal(dir);
        auto t1 = cross(dir, t0);

        pm::objects::add_cylinder(
            pm->mesh,
            [&](pm::vertex_handle v, float x, float y) {
                auto [sx, cx] = tg::sin_cos(x * 360_deg);
                auto& p = pm->pos[v];
                p = c.axis[y] + (t0 * sx + t1 * cx) * c.radius;
            },
            32, false);
    }

    POLYMESH_ASSERT(pm->mesh.is_compact());
    pm->info.vertexCount = pm->mesh.vertices().size();
    pm->info.edgeCount = pm->mesh.edges().size();
    pm->info.faceCount = pm->mesh.faces().size();

    return pm;
}
template <int D, class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::cylinder_boundary_no_caps<D, ScalarT> const& s)
{
    return make_mesh_definition(std::vector<tg::cylinder_boundary_no_caps<D, ScalarT>>{s});
}

template <class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(std::vector<tg::cone<3, ScalarT>> const& cones)
{
    // TODO: proper ray-traced cones

    auto pm = std::make_shared<PolyMeshDefinition>();
    pm->info.type = MeshType::PolygonList;

    for (auto const& c : cones)
    {
        auto dir = c.base.normal;
        auto t0 = any_normal(dir);
        auto t1 = cross(dir, t0);

        pm::objects::add_cone(
            pm->mesh,
            [&](pm::vertex_handle v, float x, float y) {
                auto [sx, cx] = tg::sin_cos(x * 360_deg);
                auto& p = pm->pos[v];
                if (y == 0)
                    p = c.base.center + (t0 * sx + t1 * cx) * c.base.radius;
                else
                    p = c.base.center + c.base.normal * c.height;
            },
            32);
    }

    POLYMESH_ASSERT(pm->mesh.is_compact());
    pm->info.vertexCount = pm->mesh.vertices().size();
    pm->info.edgeCount = pm->mesh.edges().size();
    pm->info.faceCount = pm->mesh.faces().size();

    return pm;
}
template <class ScalarT>
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(tg::cone<3, ScalarT> const& s)
{
    return make_mesh_definition(std::vector<tg::cone<3, ScalarT>>{s});
}

std::shared_ptr<PolyMeshDefinition> make_mesh_definition(file const& file);
}
}
}
