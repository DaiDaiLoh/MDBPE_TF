#pragma once

#ifdef GLOW_EXTRAS_HAS_POLYMESH

#include <glow/objects/ArrayBuffer.hh>
#include <glow/objects/ElementArrayBuffer.hh>
#include <glow/objects/VertexArray.hh>

#include <polymesh/Mesh.hh>
#include <polymesh/properties.hh>

namespace glow
{
namespace geometry
{
/**
 * Usage:
 *  auto vao = MeshBuilder(m)
 *      .add("aPosition", pos)
 *      .add("aNormal", normals)
 *      ...
 *      .make()
 *
 * NOTE:
 *  for now it will always generate non-indexed TRIANGLE_LISTs
 *  will generate naively triangulate mesh for non-triangular ones
 */
class MeshBuilder final
{
    // ctors
public:
    MeshBuilder(polymesh::Mesh const& m) : mesh(m)
    {
        // build index buffer if not triangular
        if (!is_triangle_mesh(m))
        {
            std::vector<int> indices;
            indices.reserve(m.faces().size() * 3);

            auto v_base = 0;
            for (auto f : m.faces())
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

            mIndexBuffer = ElementArrayBuffer::create(indices);
        }
    }

    SharedVertexArray make() const { return VertexArray::create(mBuffers, mIndexBuffer); }

    // adding attributes
public:
    template <class T>
    MeshBuilder& add(std::string const& attrName, polymesh::vertex_attribute<T> const& attr)
    {
        std::vector<T> data;
        data.reserve(mesh.faces().size() * 3);
        for (auto f : mesh.faces())
            for (auto h : f.halfedges())
                data.push_back(attr[h.vertex_to()]);
        auto ab = ArrayBuffer::create();
        ab->defineAttribute<T>(attrName);
        ab->bind().setData(data);
        mBuffers.push_back(ab);
        return *this;
    }

    template <class T>
    MeshBuilder& add(std::string const& attrName, polymesh::face_attribute<T> const& attr)
    {
        std::vector<T> data;
        data.reserve(mesh.faces().size() * 3);
        for (auto f : mesh.faces())
            for (auto h : f.halfedges())
            {
                (void)h; // unused!
                data.push_back(attr[f]);
            }
        auto ab = ArrayBuffer::create();
        ab->defineAttribute<T>(attrName);
        ab->bind().setData(data);
        mBuffers.push_back(ab);
        return *this;
    }

    template <class T>
    MeshBuilder& add(std::string const& attrName, polymesh::edge_attribute<T> const& attr)
    {
        std::vector<T> data;
        data.reserve(mesh.faces().size() * 3);
        for (auto f : mesh.faces())
            for (auto h : f.halfedges())
                data.push_back(attr[h]);
        auto ab = ArrayBuffer::create();
        ab->defineAttribute<T>(attrName);
        ab->bind().setData(data);
        mBuffers.push_back(ab);
        return *this;
    }

    template <class T>
    MeshBuilder& add(std::string const& attrName, polymesh::halfedge_attribute<T> const& attr)
    {
        std::vector<T> data;
        data.reserve(mesh.faces().size() * 3);
        for (auto f : mesh.faces())
            for (auto h : f.halfedges())
                data.push_back(attr[h]);
        auto ab = ArrayBuffer::create();
        ab->defineAttribute<T>(attrName);
        ab->bind().setData(data);
        mBuffers.push_back(ab);
        return *this;
    }

private:
    std::vector<SharedArrayBuffer> mBuffers;
    SharedElementArrayBuffer mIndexBuffer;

    polymesh::Mesh const& mesh;
};
}
}

#endif
