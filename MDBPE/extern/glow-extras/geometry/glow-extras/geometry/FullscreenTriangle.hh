#pragma once

#include <glow/objects/VertexArray.hh>

namespace glow
{
namespace geometry
{
/// Helper for drawing a single triangle for fullscreen passes with
/// the oversized triangle optimization trick
///
/// Usage:
/// // Somewhere
/// FullscreenTriangle::init();
///
/// // Render
/// FullscreenTriangle::draw();
///
/// Sample Vertex Shader:
///
/// void main() {
///     vec2 uv = vec2(((gl_VertexID << 1) & 2) * 2 - 1, (gl_VertexID & 2) * 2 - 1);
///     gl_Position = vec4(uv, 0, 1);
/// }
///
class FullscreenTriangle
{
private:
    static SharedVertexArray sBlankVao;

public:
    /// Initializes the internally required blank VAO
    /// Calls beyond the first one do nothing
    static void init()
    {
        if (!sBlankVao)
            sBlankVao = VertexArray::create();
    }

    /// Binds the blank VAO and draws the triangle
    static void draw()
    {
        auto boundVao = sBlankVao->bind();
        boundVao.negotiateBindings();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    /// Advanced usage
    ///
    /// auto vao = FullscreenTriangle::bindBlankVao();
    ///
    /// for (auto i = 0; i < 10; ++i)
    ///      FullscreenTriangle::drawTriangle();
    ///
    /// for (auto i = 0; i < 50; ++i)
    ///     FullscreenTriangle::drawPoint();
    ///
    GLOW_NODISCARD static BoundVertexArray bindBlankVao() { return sBlankVao->bind(); }
    static void drawTriangle() { glDrawArrays(GL_TRIANGLES, 0, 3); }
    static void drawPoint() { glDrawArrays(GL_POINTS, 0, 1); }
};
}
}
