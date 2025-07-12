#pragma once

#include <typed-geometry/tg-lean.hh>

#include <glow/common/shared.hh>

#include <glow/objects/ArrayBuffer.hh>
#include <glow/objects/ElementArrayBuffer.hh>
#include <glow/objects/VertexArray.hh>

namespace glow
{
namespace geometry
{
/// Default type for Frame vertices
struct FrameVertex
{
    tg::pos3 pos;
    tg::color3 color;

    static std::vector<ArrayBufferAttribute> attributes()
    {
        return {
            {&FrameVertex::pos, "aPosition"}, //
            {&FrameVertex::color, "aColor"},
        };
    }
};

/// Builder for a coordinate frame
/// VertexT needs a c'tor that can be called with
///   (glm::vec3 position, glm::vec3 color)
///
/// Supported built-in types:
///   FrameVertex
///
/// If you use your custom vertex struct you have 2 options:
///   A: specify all attributes in Cube(...) ctor
///   B: implemented the following function in your VertexT:
///      static std::vector<ArrayBufferAttribute> attributes();
template <typename VertexT = FrameVertex>
class CoordFrame
{
public:
    /// List of cube attributes
    std::vector<ArrayBufferAttribute> attributes;

public: // default vertex creator
    static VertexT createVertex(tg::pos3 position, tg::color3 color) { return {position, color}; }

public:
    CoordFrame(std::vector<ArrayBufferAttribute> const& attrs = attributesOf(static_cast<VertexT*>(nullptr))) : attributes(attrs) {}
    /**
     * @brief generates a Vertex Array with a single Array Buffer containing frame data
     */
    template <typename VertexCreator = decltype(createVertex)>
    SharedVertexArray generate(VertexCreator&& gen = createVertex) const
    {
        auto ab = ArrayBuffer::create();
        ab->setObjectLabel("CoordFrame");
        ab->defineAttributes(attributes);

        VertexT vertices[2 * 3] = {
            gen({0, 0, 0}, {1, 0, 0}), //
            gen({1, 0, 0}, {1, 0, 0}), //

            gen({0, 0, 0}, {0, 1, 0}), //
            gen({0, 1, 0}, {0, 1, 0}), //

            gen({0, 0, 0}, {0, 0, 1}), //
            gen({0, 0, 1}, {0, 0, 1}), //
        };

        ab->bind().setData(vertices);
        auto va = VertexArray::create(ab, nullptr, GL_LINES);
        va->setObjectLabel("CoordFrame");
        return va;
    }

public: // Predefined attributes
    static std::vector<ArrayBufferAttribute> attributesOf(void*) { return VertexT::attributes(); }
};
}
}
