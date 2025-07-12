#include "Vector2DRenderable.hh"

#include <glow/common/hash.hh>

#include <glow-extras/vector/backend/opengl.hh>

#include "../RenderInfo.hh"

void glow::viewer::Vector2DRenderable::renderOverlay(RenderInfo const&, vector::OGLRenderer const* oglRenderer, tg::isize2 const& res, tg::ipos2 const& /*offset*/)
{
    oglRenderer->render(mImage, res.width, res.height);
}

size_t glow::viewer::Vector2DRenderable::computeHash() const { return glow::hash_xxh3(glow::as_byte_view(mImage.raw_buffer()), 0x231241); }

glow::viewer::SharedVector2DRenderable glow::viewer::Vector2DRenderable::create(const glow::vector::image2D& img)
{
    auto r = std::make_shared<Vector2DRenderable>();
    r->mImage = img; // copy
    return r;
}
