#pragma once

#include <typed-geometry/tg.hh>

#include <glow/common/shared.hh>

#include <glow/objects/ArrayBuffer.hh>
#include <glow/objects/ElementArrayBuffer.hh>
#include <glow/objects/VertexArray.hh>


#include <fstream>
namespace glow
{
namespace geometry
{
/// Default type for Cube vertices
struct UVSphereVertex
{
    tg::pos3 pos;
    tg::vec3 normal;
    tg::vec3 tangent;
    tg::pos2 texCoord;

    static std::vector<ArrayBufferAttribute> attributes()
    {
        return {
            {&UVSphereVertex::pos, "aPosition"},    //
            {&UVSphereVertex::normal, "aNormal"},   //
            {&UVSphereVertex::tangent, "aTangent"}, //
            {&UVSphereVertex::texCoord, "aTexCoord"},
        };
    }
};

/// Builder for a UVSphere
/// VertexT needs a c'tor that can be called with
///   (glm::vec3 position, glm::vec3 normal, glm::vec3 tangent, glm::vec2 texCoord)
///
/// Supported built-in types:
///   UVSphereVertex
///
/// If you use your custom vertex struct you have 2 options:
///   A: specify all attributes in UVSphere(...) ctor
///   B: implement the following function in your VertexT:
///      static std::vector<ArrayBufferAttribute> attributes();
template <typename VertexT = UVSphereVertex>
class UVSphere
{
public:
    /// List of cube attributes
    std::vector<ArrayBufferAttribute> attributes;

    /// Number of segments
    int32_t segments;
    /// Number of rings
    int32_t rings;
    /// Sphere radius
    float radius;

public: // default vertex creator
    static VertexT createVertex(tg::pos3 position, tg::vec3 normal, tg::vec3 tangent, tg::pos2 texCoord)
    {
        return {position, normal, tangent, texCoord};
    }

public:
    UVSphere(std::vector<ArrayBufferAttribute> const& attrs = attributesOf(static_cast<VertexT*>(nullptr)), int32_t segments = 32, int32_t rings = 16, float radius = 1.0f)
      : attributes(attrs), segments(segments), rings(rings), radius(radius)
    {
        if (segments < 3)
        {
            this->segments = 3;
            glow::info() << "Clamping sphere segments to 3.";
        }
        if (rings < 3)
        {
            this->rings = 3;
            glow::info() << "Clamping sphere segments to 3.";
        }
    }
    /**
     * @brief generates a Vertex Array with AB and EAB
     */
    template <typename VertexCreator = decltype(createVertex)>
    SharedVertexArray generate(VertexCreator&& gen = createVertex) const
    {
        auto ab = ArrayBuffer::create();
        ab->setObjectLabel("UVSphere");
        auto eab = ElementArrayBuffer::create();
        eab->setObjectLabel("UVSphere");
        ab->defineAttributes(attributes);


        // For texturing, we need a vertical cut and may not wrap around
        std::vector<VertexT> vertices((rings + 1) * (segments + 1));

        // rings * segments quads (each consisting of 2 * 3 vertices)
        std::vector<uint32_t> indices(3 * (rings * segments * 2));


        // Vertex attributes
        auto vi = 0u;
        for (int r = 0; r < rings + 1; ++r)
            for (int s = 0; s < segments + 1; ++s)
            {
                auto theta = 180_deg / rings * r;
                auto phi = 360_deg / segments * s;

                tg::pos3 position = {cos(phi) * sin(theta) * radius, cos(theta) * radius, sin(phi) * sin(theta) * radius};
                tg::vec3 normal = normalize(position - tg::pos3::zero);
                tg::vec3 tangent = {sin(phi), 0, cos(phi)};
                tg::pos2 texcoord = {static_cast<float>(s) / segments, static_cast<float>(r) / rings};
                vertices[vi++] = gen(position, normal, tangent, texcoord);
            }

        // Indices
        auto ii = 0u;
        for (int r = 0; r < rings; ++r)
            for (int s = 0; s < segments; ++s)
            {
                uint32_t startIdxTop = (segments + 1) * r;
                uint32_t startIdxBot = (segments + 1) * (r + 1);

                indices[ii++] = startIdxTop + s;
                indices[ii++] = startIdxTop + ((s + 1));
                indices[ii++] = startIdxBot + s;

                indices[ii++] = startIdxBot + s;
                indices[ii++] = startIdxTop + ((s + 1));
                indices[ii++] = startIdxBot + ((s + 1));
            }

        ab->bind().setData(vertices);
        eab->bind().setIndices(indices);
        auto va = VertexArray::create(ab, eab, GL_TRIANGLES);
        va->setObjectLabel("UVSphere");
        return va;
    }

public: // Predefined attributes
    static std::vector<ArrayBufferAttribute> attributesOf(void*) { return VertexT::attributes(); }
};

inline SharedVertexArray make_uv_sphere(int32_t segments = 32, int32_t rings = 16, float radius = 1.0f)
{
    return UVSphere<>(UVSphere<>::attributesOf(nullptr), segments, rings, radius).generate();
}
}
}
