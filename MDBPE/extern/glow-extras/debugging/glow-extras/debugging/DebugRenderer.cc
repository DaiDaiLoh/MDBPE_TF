#include "DebugRenderer.hh"

#include <typed-geometry/tg.hh>

#include <glow/common/scoped_gl.hh>
#include <glow/common/str_utils.hh>
#include <glow/objects/Program.hh>
#include <glow/util/DefaultShaderParser.hh>

#include <glow-extras/geometry/Cube.hh>
#include <glow-extras/geometry/Line.hh>
#include <glow-extras/geometry/Quad.hh>

#ifdef GLOW_EXTRAS_EMBED_SHADERS
#include <glow-extras/generated/debugging_embed_shaders.hh>
#endif

using namespace glow;
using namespace glow::debugging;

namespace
{
bool _isDebugRendererInitialized = false;
}

DebugRenderer::DebugRenderer()
{
    DebugRenderer::GlobalInit();

    mQuad = geometry::Quad<PrimitiveVertex>().generate();
    mCube = geometry::Cube<PrimitiveVertex>().generate();
    mLine = geometry::Line<PrimitiveVertex>().generate();

    mShaderOpaque = Program::createFromFiles({"glow-debugging/primitive.opaque.fsh", "glow-debugging/primitive.vsh"});
}

void DebugRenderer::clear()
{
    mPrimitives.clear();
}

void DebugRenderer::render(tg::mat4 const& vp) const
{
    auto shader = mShaderOpaque->use();
    shader.setUniform("uVP", vp);

    // While helping legibility of lines, this feature was removed
    // from OpenGL core and causes errors on non-NVidia platforms
    // GLOW_SCOPED(enable, GL_CONSERVATIVE_RASTERIZATION_NV);

    for (auto const& p : mPrimitives)
    {
        shader.setUniform("uColor", p.color);
        shader.setUniform("uModel", p.modelMatrix);

        p.vao->bind().draw();
    }
}

void DebugRenderer::GlobalInit()
{
    if (_isDebugRendererInitialized)
        return;

#ifdef GLOW_EXTRAS_EMBED_SHADERS
    for (auto& virtualPair : internal_embedded_files::debugging_embed_shaders)
        DefaultShaderParser::addVirtualFile(virtualPair.first, virtualPair.second);
#else
    DefaultShaderParser::addIncludePath(util::pathOf(__FILE__) + "/../../shader");
#endif

    _isDebugRendererInitialized = true;
}

void DebugRenderer::renderLine(tg::pos3 start, tg::pos3 end, tg::color3 color)
{
    auto m = tg::translation(start) * tg::mat4(tg::mat3::from_cols(end - start, tg::vec3(0, 1, 0), tg::vec3(0, 0, 1)));
    mPrimitives.push_back({mLine, color, m});
}

void DebugRenderer::renderAABB(tg::pos3 start, tg::pos3 end, tg::color3 color, bool wireframe)
{
    if (start.x > end.x)
        std::swap(start.x, end.x);
    if (start.y > end.y)
        std::swap(start.y, end.y);
    if (start.z > end.z)
        std::swap(start.z, end.z);

    if (!wireframe)
    {
        auto m = tg::translation((start + end) / 2.0f) * tg::scaling(tg::size(end - start) / 2.0f);
        mPrimitives.push_back({mCube, color, m});
    }
    else
    {
        auto s = start;
        auto e = end;

        renderLine({s.x, s.y, s.z}, {e.x, s.y, s.z}, color);
        renderLine({s.x, s.y, e.z}, {e.x, s.y, e.z}, color);
        renderLine({s.x, e.y, s.z}, {e.x, e.y, s.z}, color);
        renderLine({s.x, e.y, e.z}, {e.x, e.y, e.z}, color);

        renderLine({s.x, s.y, s.z}, {s.x, e.y, s.z}, color);
        renderLine({s.x, s.y, e.z}, {s.x, e.y, e.z}, color);
        renderLine({e.x, s.y, s.z}, {e.x, e.y, s.z}, color);
        renderLine({e.x, s.y, e.z}, {e.x, e.y, e.z}, color);

        renderLine({s.x, s.y, s.z}, {s.x, s.y, e.z}, color);
        renderLine({e.x, s.y, s.z}, {e.x, s.y, e.z}, color);
        renderLine({s.x, e.y, s.z}, {s.x, e.y, e.z}, color);
        renderLine({e.x, e.y, s.z}, {e.x, e.y, e.z}, color);
    }
}

void DebugRenderer::renderAABBCentered(tg::pos3 center, tg::vec3 size, tg::color3 color, bool wireframe)
{
    renderAABB(center - size / 2.0f, center + size / 2.0f, color, wireframe);
}
