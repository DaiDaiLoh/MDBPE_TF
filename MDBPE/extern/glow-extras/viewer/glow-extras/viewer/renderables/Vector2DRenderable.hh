#pragma once

#include "Renderable.hh"

#include <glow-extras/vector/image2D.hh>

namespace glow
{
namespace viewer
{
// TODO: move and scale image
class Vector2DRenderable final : public Renderable
{
private:
    vector::image2D mImage;

public:
    aabb computeAabb() override { return aabb::empty(); }

    void renderOverlay(RenderInfo const&, vector::OGLRenderer const* oglRenderer, tg::isize2 const& res, tg::ipos2 const&) override;
    size_t computeHash() const override;

public:
    static SharedVector2DRenderable create(vector::image2D const& img);
};
}
}
