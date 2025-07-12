#include "MeshDefinition.hh"

#include <glow/common/hash.hh>

#include <glow/objects/ElementArrayBuffer.hh>

#include <polymesh/formats.hh>
#include <polymesh/properties.hh>

#include "MeshAttribute.hh"

using namespace glow;
using namespace glow::viewer;
using namespace glow::viewer::detail;

aabb PolyMeshDefinition::computeAABB()
{
    if (mesh.vertices().empty())
        return aabb::empty();

    auto p0 = pos[mesh.vertices().first()];

    auto bb = aabb(p0, p0);

    for (auto v : mesh.vertices())
    {
        TG_ASSERT(tg::is_finite(pos[v].x));
        TG_ASSERT(tg::is_finite(pos[v].y));
        TG_ASSERT(tg::is_finite(pos[v].z));
        bb.add(pos[v]);
    }

    return bb;
}

SharedMeshAttribute PolyMeshDefinition::computePositionAttribute() { return make_mesh_attribute("aPosition", pos); }

SharedElementArrayBuffer PolyMeshDefinition::computeIndexBuffer()
{
    if (mesh.is_compact() && is_triangle_mesh(mesh))
        return nullptr;

    std::vector<int> indices;
    indices.reserve(mesh.faces().size() * 3);

    auto v_base = 0;
    for (auto f : mesh.faces())
    {
        auto idx = 0;
        for (auto h : f.halfedges())
        {
            (void)h; // unused

            if (idx >= 2)
            {
                indices.push_back(v_base + 0);
                indices.push_back(v_base + idx - 1);
                indices.push_back(v_base + idx - 0);
            }

            ++idx;
        }

        v_base += idx;
    }

    return ElementArrayBuffer::create(indices);
}

SharedMeshAttribute PolyMeshDefinition::computeFaceNormalAttribute() { return make_mesh_attribute("aNormal", face_normals(pos)); }

SharedMeshAttribute PolyMeshDefinition::computeSmoothNormalAttribute() { return make_mesh_attribute("aNormal", vertex_normals_by_area(pos)); }

size_t PolyMeshDefinition::computeHash() const
{
    auto ll = low_level_api(mesh);
    auto h = glow::hash_xxh3(as_byte_view(pos), 0x631231);
    h = glow::hash_xxh3(as_byte_view(info), h);
    if (!mesh.vertices().empty())
        h = glow::hash_xxh3(array_view(&ll.outgoing_halfedge_of(pm::vertex_index(0)), mesh.vertices().size()).as_bytes(), h);
    if (!mesh.faces().empty())
        h = glow::hash_xxh3(array_view(&ll.halfedge_of(pm::face_index(0)), mesh.faces().size()).as_bytes(), h);
    if (!mesh.halfedges().empty())
    {
        h = glow::hash_xxh3(array_view(&ll.next_halfedge_of(pm::halfedge_index(0)), mesh.halfedges().size()).as_bytes(), h);
        h = glow::hash_xxh3(array_view(&ll.prev_halfedge_of(pm::halfedge_index(0)), mesh.halfedges().size()).as_bytes(), h);
        h = glow::hash_xxh3(array_view(&ll.face_of(pm::halfedge_index(0)), mesh.halfedges().size()).as_bytes(), h);
        h = glow::hash_xxh3(array_view(&ll.to_vertex_of(pm::halfedge_index(0)), mesh.halfedges().size()).as_bytes(), h);
    }
    return h;
}

namespace glow::viewer::detail
{
std::shared_ptr<PolyMeshDefinition> make_mesh_definition(file const& file)
{
    auto pm = std::make_shared<PolyMeshDefinition>();
    load(file.filepath, pm->mesh, pm->pos);
    pm->info = make_mesh_info(pm->mesh);
    return pm;
}
}
