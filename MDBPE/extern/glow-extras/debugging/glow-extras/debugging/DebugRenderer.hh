#pragma once

#include <vector>

#include <glow/common/property.hh>
#include <glow/common/shared.hh>

#include <glow/objects/ArrayBufferAttribute.hh>

#include <glow/fwd.hh>

#include <typed-geometry/tg-lean.hh>

namespace glow
{
namespace debugging
{
/**
 * @brief The DebugRenderer is a easy-to-use (but not so performant) interface for easy debug rendering
 *
 * The DebugRenderer provides a set of functions for drawing primitives:
 *   - drawLine(...)
 *   - drawCube(...)
 *
 * The DebugRenderer accumulates all primitives, unless DebugRenderer::clear() is called
 */
class DebugRenderer
{
private:
    struct PrimitiveVertex
    {
        tg::pos3 pos;

        PrimitiveVertex() = default;
        PrimitiveVertex(float u, float v) : pos(u, v, 0.0) {}
        PrimitiveVertex(tg::pos3 pos) : pos(pos) {}
        PrimitiveVertex(tg::pos3 position, tg::vec3, tg::vec3, tg::pos2) : pos(position) {}
        static std::vector<ArrayBufferAttribute> attributes()
        {
            return {
                {&PrimitiveVertex::pos, "aPosition"}, //
            };
        }
    };

    struct Primitive
    {
        SharedVertexArray vao;
        tg::color3 color;
        tg::mat4 modelMatrix;
    };

    SharedVertexArray mQuad;
    SharedVertexArray mCube;
    SharedVertexArray mLine;
    SharedProgram mShaderOpaque;

    std::vector<Primitive> mPrimitives;

public:
    DebugRenderer();

    /// Clears all stored primitives
    void clear();

    /// Renders all stored primitives
    void render(tg::mat4 const& vp) const;

    static void GlobalInit();

    // render functions
public:
    void renderLine(tg::pos3 start, tg::pos3 end, tg::color3 color = tg::color3::white);
    void renderAABB(tg::pos3 start, tg::pos3 end, tg::color3 color = tg::color3::white, bool wireframe = true);
    void renderAABBCentered(tg::pos3 center, tg::vec3 size, tg::color3 color = tg::color3::white, bool wireframe = true);
};
GLOW_SHARED(class, DebugRenderer);
}
}
