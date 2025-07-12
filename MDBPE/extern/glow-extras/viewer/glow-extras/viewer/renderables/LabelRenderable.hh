#pragma once

#include "Renderable.hh"

#include <glow/common/array_view.hh>
#include <glow-extras/viewer/objects/other/labels.hh>
#include <glow-extras/vector/image2D.hh>

namespace glow
{
namespace viewer
{
class LabelRenderable final : public Renderable
{
private:
    vector::image2D mImage;
    std::vector<label> mLabels;

public:
    aabb computeAabb() override { return aabb::empty(); }

    void renderOverlay(RenderInfo const&, vector::OGLRenderer const* oglRenderer, tg::isize2 const& res, tg::ipos2 const&) override;
    size_t computeHash() const override;

public:
    static SharedLabelRenderable create(array_view<label const> labels);
};
}
}
