#pragma once

#include "Renderable.hh"

namespace glow::viewer
{
class TextureRenderable final : public Renderable
{
private:
    glow::SharedTexture mTexture;
    glow::SharedProgram mShader;
    glow::SharedVertexArray mQuad;
    glow::SharedSampler mSampler;

public:
    aabb computeAabb() override { return aabb::empty(); }

    void renderOverlay(RenderInfo const&, vector::OGLRenderer const*, tg::isize2 const& res,tg::ipos2 const& offset) override;

    void init() override;
    size_t computeHash() const override;

public:
    static SharedTextureRenderable create(glow::SharedTexture tex);
};
}
