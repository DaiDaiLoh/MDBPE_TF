#pragma once

#include <array>
#include <string>
#include <typeinfo>
#include <vector>

#include "../fwd.hh"

#include <polymesh/Mesh.hh>
#include <polymesh/assert.hh>

#include "uniform_info.hh"

#include <glow/common/hash.hh>
#include <glow/objects/ArrayBuffer.hh>
#include <glow/objects/Program.hh>

#include "MeshDefinition.hh"
#include "MeshInfo.hh"
#include "MeshShaderBuilder.hh"

namespace glow
{
namespace viewer
{
namespace detail
{
class MeshShaderBuilder;

// a mesh attribute is either a constant (uniform) or a per-halfedge attribute
// TODO: maybe can be read from a texture
class MeshAttribute
{
public:
    std::string const& name() const { return mName; }

    MeshAttribute(std::string name) : mName(std::move(name)) {}

    virtual SharedArrayBuffer createMeshRenderableArrayBuffer(MeshDefinition&)
    {
        throw std::logic_error("This mesh attribute cannot be used in a MeshRenderable");
    }
    virtual SharedArrayBuffer createLineRenderableArrayBuffer(MeshDefinition&)
    {
        throw std::logic_error("This mesh attribute cannot be used in a LineRenderable");
    }
    virtual SharedArrayBuffer createPointRenderableArrayBuffer(MeshDefinition&)
    {
        throw std::logic_error("This mesh attribute cannot be used in a PointRenderable");
    }

    virtual std::string typeInShader() const = 0;

    virtual void buildShader(MeshShaderBuilder& shader) = 0;
    virtual void prepareShader(UsedProgram& /*shader*/) {}

    // Returns the maximal value of this attribute, if it is convertible to a float
    virtual float computeMaxFloat() = 0;

    // Returns true, iff this attribute contains two colors per edge that should be drawn parallel to the edge
    virtual bool hasTwoColoredLines() = 0;

    size_t queryHash() const
    {
        if (!mCachedHash.has_value())
            mCachedHash = computeHash();
        return mCachedHash.value();
    }

    virtual ~MeshAttribute() = default;

protected:
    virtual size_t computeHash() const = 0;

private:
    std::string mName;
    mutable cc::optional<size_t> mCachedHash;
};

template <class T>
class ConstantMeshAttribute : public MeshAttribute
{
public:
    using data_t = T;

    T mConstant;

    ConstantMeshAttribute(std::string name, T const& constant) : MeshAttribute(std::move(name)), mConstant(constant) {}

    SharedArrayBuffer createMeshRenderableArrayBuffer(MeshDefinition&) override { return nullptr; }  // works
    SharedArrayBuffer createLineRenderableArrayBuffer(MeshDefinition&) override { return nullptr; }  // works
    SharedArrayBuffer createPointRenderableArrayBuffer(MeshDefinition&) override { return nullptr; } // works

    std::string typeInShader() const override { return uniform_info<T>::shader_type; }
    void buildShader(MeshShaderBuilder& shader) override { shader.addUniform(uniform_info<T>::shader_type, name()); }
    void prepareShader(UsedProgram& shader) override { shader.setUniform(name(), uniform_info<T>::uniform(mConstant)); }
    float computeMaxFloat() override
    {
        if constexpr (std::is_convertible_v<T, float>)
            return mConstant;
        else
            throw std::logic_error("Cannot convert T to float");
    }
    bool hasTwoColoredLines() override { return false; }

    size_t computeHash() const override { return glow::hash_xxh3(as_byte_view(mConstant), 0x412311); }
};

template <class T>
class VertexMeshAttribute : public MeshAttribute
{
public:
    using data_t = T;

    MeshInfo mMeshInfo;
    std::vector<T> mData;

    std::string typeInShader() const override { return uniform_info<T>::shader_type; }

    size_t computeHash() const override { return glow::hash_xxh3(as_byte_view(mData), 0x412313); }

    void buildShader(MeshShaderBuilder& shader) override { shader.addAttribute(uniform_info<T>::shader_type, name()); }

    SharedArrayBuffer createMeshRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.vertexCount == pm->mesh.all_vertices().size() && "incompatible vertex count");

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto f : pm->mesh.faces())
                for (auto h : f.halfedges())
                    data.push_back(mData[int(h.vertex_to())]);

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }
    SharedArrayBuffer createLineRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.vertexCount == pm->mesh.all_vertices().size() && "incompatible vertex count");

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto e : pm->mesh.edges())
            {
                data.push_back(mData[int(e.vertexA())]);
                data.push_back(mData[int(e.vertexB())]);
            }

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }
    SharedArrayBuffer createPointRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.vertexCount == pm->mesh.all_vertices().size() && "incompatible vertex count");

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(mData);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }

    VertexMeshAttribute(std::string name, polymesh::vertex_attribute<T> const& data) : MeshAttribute(std::move(name))
    {
        polymesh::Mesh const& mesh = data.mesh();
        mMeshInfo = make_mesh_info(mesh);

        mData.reserve(mesh.all_vertices().size());
        for (auto v : mesh.all_vertices())
            mData.push_back(data[v]);
    }
    VertexMeshAttribute(std::string name, std::vector<T> const& data) : MeshAttribute(std::move(name))
    {
        mMeshInfo.vertexCount = data.size();
        mData = data;
    }
    VertexMeshAttribute(std::string name, std::vector<std::vector<T>> const& data) : MeshAttribute(std::move(name))
    {
        mMeshInfo.vertexCount = 0;

        for (auto const& poly : data)
            for (auto const& d : poly)
            {
                mData.push_back(d);
                mMeshInfo.vertexCount++;
            }
    }
    template <int N>
    VertexMeshAttribute(std::string name, std::vector<std::array<T, N>> const& data) : MeshAttribute(std::move(name))
    {
        mMeshInfo.vertexCount = 0;

        for (auto const& poly : data)
            for (auto const& d : poly)
            {
                mData.push_back(d);
                mMeshInfo.vertexCount++;
            }
    }

    float computeMaxFloat() override
    {
        if constexpr (std::is_convertible_v<T, float>)
        {
            auto max = std::numeric_limits<float>::min();
            for (auto f : mData)
                if (f > max)
                    max = f;

            return max;
        }
        else
            throw std::logic_error("Cannot convert T to float");
    }
    bool hasTwoColoredLines() override { return false; }
};

template <class T>
class EdgeMeshAttribute : public MeshAttribute
{
public:
    using data_t = T;

    MeshInfo mMeshInfo;
    std::vector<T> mData;

    size_t computeHash() const override { return glow::hash_xxh3(as_byte_view(mData), 0x4126313); }

    std::string typeInShader() const override { return uniform_info<T>::shader_type; }

    void buildShader(MeshShaderBuilder& shader) override { shader.addAttribute(uniform_info<T>::shader_type, name()); }

    SharedArrayBuffer createMeshRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.edgeCount == pm->mesh.all_edges().size() && "incompatible edge count");
            POLYMESH_ASSERT(int(mData.size()) == mMeshInfo.edgeCount);

            // TODO: this is kinda wrong (associates edges with halfedges and thus to-vertices)

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto f : pm->mesh.faces())
                for (auto h : f.halfedges())
                    data.push_back(mData[int(h.edge())]);

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }
    SharedArrayBuffer createLineRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.edgeCount == pm->mesh.all_edges().size() && "incompatible edge count");
            POLYMESH_ASSERT(int(mData.size()) == mMeshInfo.edgeCount);

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto e : pm->mesh.edges())
            {
                data.push_back(mData[int(e)]);
                data.push_back(mData[int(e)]);
            }

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }

    EdgeMeshAttribute(std::string name, polymesh::edge_attribute<T> const& data) : MeshAttribute(std::move(name))
    {
        polymesh::Mesh const& mesh = data.mesh();
        mMeshInfo = make_mesh_info(mesh);

        mData.reserve(mesh.all_edges().size());
        for (auto e : mesh.all_edges())
            mData.push_back(data[e]);
    }
    EdgeMeshAttribute(std::string name, std::vector<T> const& data) : MeshAttribute(std::move(name))
    {
        mMeshInfo.edgeCount = data.size();
        mData = data;
    }

    float computeMaxFloat() override
    {
        if constexpr (std::is_convertible_v<T, float>)
        {
            auto max = std::numeric_limits<float>::min();
            for (auto f : mData)
                if (f > max)
                    max = f;

            return max;
        }
        else
            throw std::logic_error("Cannot convert T to float");
    }
    bool hasTwoColoredLines() override { return false; }
};

template <class T>
class FaceMeshAttribute : public MeshAttribute
{
public:
    using data_t = T;

    MeshInfo mMeshInfo;
    std::vector<T> mData;

    size_t computeHash() const override { return glow::hash_xxh3(as_byte_view(mData), 0x4192313); }

    std::string typeInShader() const override { return uniform_info<T>::shader_type; }

    void buildShader(MeshShaderBuilder& shader) override { shader.addAttribute(uniform_info<T>::shader_type, name()); }

    SharedArrayBuffer createMeshRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.faceCount == pm->mesh.all_faces().size() && "incompatible face count");
            POLYMESH_ASSERT(int(mData.size()) == mMeshInfo.faceCount);

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto f : pm->mesh.faces())
                for (auto h : f.halfedges())
                {
                    (void)h;
                    data.push_back(mData[int(f)]);
                }

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }
    SharedArrayBuffer createLineRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.edgeCount == pm->mesh.all_edges().size() && "incompatible edge count");
            POLYMESH_ASSERT(mMeshInfo.faceCount == pm->mesh.all_faces().size() && "incompatible face count");
            POLYMESH_ASSERT(int(mData.size()) == mMeshInfo.faceCount);

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto e : pm->mesh.edges())
            {
                // Try to push value of face A and then of face B. If one of them is invalid due to being boundary, fill in with the opposite face value
                auto a = e.faceA();
                auto b = e.faceB();

                // Two invalid faces may happen for free edges. Default value of the data type is used here
                if (a.is_invalid() && b.is_invalid())
                {
                    data.push_back(T());
                    data.push_back(T());
                    continue;
                }

                if (a.is_valid())
                    data.push_back(mData[int(a)]);
                else // If only one face is invalid (boundary), the value from the valid face is used on both sides
                    data.push_back(mData[int(b)]);

                if (b.is_valid())
                    data.push_back(mData[int(b)]);
                else
                    data.push_back(mData[int(a)]);
            }

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }

    FaceMeshAttribute(std::string name, polymesh::face_attribute<T> const& data) : MeshAttribute(std::move(name))
    {
        polymesh::Mesh const& mesh = data.mesh();
        mMeshInfo = make_mesh_info(mesh);

        mData.reserve(mesh.all_faces().size());
        for (auto f : mesh.all_faces())
            mData.push_back(data[f]);
    }
    FaceMeshAttribute(std::string name, std::vector<T> const& data) : MeshAttribute(std::move(name))
    {
        mMeshInfo.faceCount = data.size();
        mData = data;
    }

    float computeMaxFloat() override
    {
        if constexpr (std::is_convertible_v<T, float>)
        {
            auto max = std::numeric_limits<float>::min();
            for (auto f : mData)
                if (f > max)
                    max = f;

            return max;
        }
        else
            throw std::logic_error("Cannot convert T to float");
    }
    bool hasTwoColoredLines() override { return true; }
};

template <class T>
class HalfedgeMeshAttribute : public MeshAttribute
{
public:
    using data_t = T;

    MeshInfo mMeshInfo;
    std::vector<T> mData;

    size_t computeHash() const override { return glow::hash_xxh3(as_byte_view(mData), 0x4123713); }

    std::string typeInShader() const override { return uniform_info<T>::shader_type; }

    void buildShader(MeshShaderBuilder& shader) override { shader.addAttribute(uniform_info<T>::shader_type, name()); }

    SharedArrayBuffer createMeshRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.edgeCount == pm->mesh.all_edges().size() && "incompatible edge count");
            POLYMESH_ASSERT(int(mData.size()) == mMeshInfo.edgeCount * 2);

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto f : pm->mesh.faces())
                for (auto h : f.halfedges())
                    data.push_back(mData[int(h)]);

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }
    SharedArrayBuffer createLineRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.edgeCount == pm->mesh.all_edges().size() && "incompatible edge count");
            POLYMESH_ASSERT(int(mData.size()) == mMeshInfo.edgeCount * 2);

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            for (auto e : pm->mesh.edges())
            {
                data.push_back(mData[int(e.halfedgeA())]);
                data.push_back(mData[int(e.halfedgeB())]);
            }

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }

    HalfedgeMeshAttribute(std::string name, polymesh::halfedge_attribute<T> const& data) : MeshAttribute(std::move(name))
    {
        polymesh::Mesh const& mesh = data.mesh();
        mMeshInfo = make_mesh_info(mesh);

        mData.reserve(mesh.all_halfedges().size());
        for (auto h : mesh.all_halfedges())
            mData.push_back(data[h]);
    }
    HalfedgeMeshAttribute(std::string name, std::vector<T> const& data) : MeshAttribute(std::move(name))
    {
        mMeshInfo.edgeCount = data.size() / 2;
        mData = data;
    }

    float computeMaxFloat() override
    {
        if constexpr (std::is_convertible_v<T, float>)
        {
            auto max = std::numeric_limits<float>::min();
            for (auto f : mData)
                if (f > max)
                    max = f;

            return max;
        }
        else
            throw std::logic_error("Cannot convert T to float");
    }
    bool hasTwoColoredLines() override { return false; }
};

template <class T>
class RawMeshAttribute : public MeshAttribute
{
public:
    using data_t = T;

    MeshInfo mMeshInfo;
    std::vector<T> mData;
    bool twoColoredLines;

    size_t computeHash() const override { return glow::hash_xxh3(as_byte_view(mData), 0x41991 + int(twoColoredLines)); }

    std::string typeInShader() const override { return uniform_info<T>::shader_type; }

    void buildShader(MeshShaderBuilder& shader) override { shader.addAttribute(uniform_info<T>::shader_type, name()); }

    SharedArrayBuffer createMeshRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            switch (pm->info.type)
            {
            case MeshType::PolyMesh:
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_faces().size() && "Raw attribute when used with PolyMesh must be one value per face");
                for (auto f : pm->mesh.faces())
                    for (auto h : f.halfedges())
                    {
                        (void)h;
                        data.push_back(mData[int(f)]);
                    }
                break;
            case MeshType::PointList:
                POLYMESH_ASSERT(0 && "how can a point list be used for a mesh renderable?");
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_vertices().size() && "Raw attribute for PointList must be one value per point");
                for (auto f : pm->mesh.faces())
                    for (auto h : f.halfedges())
                        data.push_back(mData[int(h.vertex_to())]);
                break;
            case MeshType::PolygonList:
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_faces().size() && "Raw attribute for PolygonList must be one value per polygon");
                for (auto f : pm->mesh.faces())
                    for (auto h : f.halfedges())
                    {
                        (void)h;
                        data.push_back(mData[int(f)]);
                    }
                break;
            case MeshType::LineList:
                POLYMESH_ASSERT(0 && "how can a line list be used for a mesh renderable?");
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_edges().size() && "Raw attribute for LineList must be one value per line");
                for (auto f : pm->mesh.faces())
                    for (auto h : f.halfedges())
                        data.push_back(mData[int(h.edge())]);
                break;
            default:
                throw std::logic_error("Cannot attach raw attribute because mesh type is unknown/unsupported");
            }

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }
    SharedArrayBuffer createLineRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(mMeshInfo.vertexCount == pm->mesh.all_vertices().size() && "incompatible vertex count");

            std::vector<T> data;
            data.reserve(pm->mesh.halfedges().size());
            switch (pm->info.type)
            {
            case MeshType::PolyMesh:
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_edges().size() && "Raw attribute when used with PolyMesh must be one value per edge");
                for (auto e : pm->mesh.edges())
                {
                    data.push_back(mData[int(e)]);
                    data.push_back(mData[int(e)]);
                }
                break;
            case MeshType::PointList:
                POLYMESH_ASSERT(0 && "how can a point list be used for a line renderable?");
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_vertices().size() && "Raw attribute for PointList must be one value per point");
                for (auto e : pm->mesh.edges())
                {
                    data.push_back(mData[int(e.vertexA())]);
                    data.push_back(mData[int(e.vertexB())]);
                }
                break;
            case MeshType::PolygonList:
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_faces().size() && "Raw attribute for PolygonList must be one value per polygon");
                for (auto e : pm->mesh.edges())
                {
                    // Try to push value of face A and then of face B. If one of them is invalid due to being boundary, fill in with the opposite face value
                    auto a = e.faceA();
                    auto b = e.faceB();

                    // Two invalid faces may happen for free edges. Default value of the data type is used here
                    if (a.is_invalid() && b.is_invalid())
                    {
                        data.push_back(T());
                        data.push_back(T());
                        continue;
                    }

                    if (a.is_valid())
                        data.push_back(mData[int(a)]);
                    else // If only one face is invalid (boundary), the value from the valid face is used on both sides
                        data.push_back(mData[int(b)]);

                    if (b.is_valid())
                        data.push_back(mData[int(b)]);
                    else
                        data.push_back(mData[int(a)]);
                }
                twoColoredLines = true;
                break;
            case MeshType::LineList:
                POLYMESH_ASSERT(int(mData.size()) == pm->mesh.all_edges().size() && "Raw attribute for LineList must be one value per line");
                for (auto e : pm->mesh.edges())
                {
                    data.push_back(mData[int(e)]);
                    data.push_back(mData[int(e)]);
                }
                break;
            default:
                throw std::logic_error("Cannot attach raw attribute because mesh type is unknown/unsupported");
            }

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(data);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }
    SharedArrayBuffer createPointRenderableArrayBuffer(MeshDefinition& mesh) override
    {
        auto pm = dynamic_cast<PolyMeshDefinition*>(&mesh);

        if (pm)
        {
            POLYMESH_ASSERT(int(mData.size()) == pm->mesh.vertices().size() && "incompatible vertex count");

            auto ab = ArrayBuffer::create();
            ab->defineAttribute<T>(name());
            ab->bind().setData(mData);
            return ab;
        }
        else
            throw std::logic_error("Mesh type " + std::string(typeid(mesh).name()) + " not supported");
    }

    RawMeshAttribute(std::string name, std::vector<T> const& data) : MeshAttribute(std::move(name)) { mData = data; }

    float computeMaxFloat() override
    {
        if constexpr (std::is_convertible_v<T, float>)
        {
            auto max = std::numeric_limits<float>::min();
            for (auto f : mData)
                if (f > max)
                    max = f;

            return max;
        }
        else
            throw std::logic_error("Cannot convert T to float");
    }
    bool hasTwoColoredLines() override { return twoColoredLines; }
};

// make_mesh_attribute for creating attributes

inline std::shared_ptr<ConstantMeshAttribute<float>> make_mesh_attribute(std::string const& name, double const& val)
{
    return std::make_shared<ConstantMeshAttribute<float>>(name, float(val));
}
template <class T, class = typename std::enable_if<uniform_info<T>::allowed>::type>
std::shared_ptr<ConstantMeshAttribute<T>> make_mesh_attribute(std::string const& name, T const& val)
{
    return std::make_shared<ConstantMeshAttribute<T>>(name, val);
}
template <class T>
std::shared_ptr<VertexMeshAttribute<T>> make_mesh_attribute(std::string const& name, polymesh::vertex_attribute<T> const& val)
{
    return std::make_shared<VertexMeshAttribute<T>>(name, val);
}
template <class T>
std::shared_ptr<FaceMeshAttribute<T>> make_mesh_attribute(std::string const& name, polymesh::face_attribute<T> const& val)
{
    return std::make_shared<FaceMeshAttribute<T>>(name, val);
}
template <class T>
std::shared_ptr<EdgeMeshAttribute<T>> make_mesh_attribute(std::string const& name, polymesh::edge_attribute<T> const& val)
{
    return std::make_shared<EdgeMeshAttribute<T>>(name, val);
}
template <class T>
std::shared_ptr<HalfedgeMeshAttribute<T>> make_mesh_attribute(std::string const& name, polymesh::halfedge_attribute<T> const& val)
{
    return std::make_shared<HalfedgeMeshAttribute<T>>(name, val);
}
template <class T>
std::shared_ptr<RawMeshAttribute<T>> make_mesh_attribute(std::string const& name, std::vector<T> const& val)
{
    return std::make_shared<RawMeshAttribute<T>>(name, val);
}
template <class T>
std::shared_ptr<VertexMeshAttribute<T>> make_mesh_attribute(std::string const& name, std::vector<std::vector<T>> const& val)
{
    return std::make_shared<VertexMeshAttribute<T>>(name, val);
}
template <class T, int N>
std::shared_ptr<VertexMeshAttribute<T>> make_mesh_attribute(std::string const& name, std::vector<std::array<T, N>> const& val)
{
    return std::make_shared<VertexMeshAttribute<T>>(name, val);
}
}
}
}
