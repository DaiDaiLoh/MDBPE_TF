#pragma once

#include <glow/common/array_view.hh>

#include "GeometricRenderable.hh"

namespace glow
{
namespace viewer
{
struct boxed_quadric
{
    tg::quadric3 quadric;
    tg::box3 box;
    tg::color4 color = tg::color3::white;
    bool draw_box = true;     // if false, the box part is see-through
    bool draw_surface = true; // if false, the quadric surface part is see-through

    // NOTE: also sets box
    void set_sphere(tg::sphere3 const& s)
    {
        box = aabb_of(s);
        quadric = {};
        quadric.A00 = 1;
        quadric.A11 = 1;
        quadric.A22 = 1;
        quadric.b0 = s.center.x;
        quadric.b1 = s.center.y;
        quadric.b2 = s.center.z;
        quadric.c = -s.radius * s.radius + tg::distance_sqr_to_origin(s.center);
    }
    void set_cylinder(tg::cylinder3 const& c);

    void invert() { quadric = -quadric; }

private:
    [[maybe_unused]] float _padding = 0;
};
static_assert(sizeof(boxed_quadric) == 28 * sizeof(float));

class QuadricRenderable final : public Renderable
{
private:
    aabb mBoxAABB;
    size_t mBoxQuadricHash = 0;
    int mQuadricCount = 0;

    // is lazily built
    glow::SharedVertexArray mVertexArray;
    glow::SharedProgram mForwardShader;
    glow::SharedProgram mShadowShader;
    glow::SharedShaderStorageBuffer mQuadricBuffer;

public:
    aabb computeAabb() override;
    size_t computeHash() const override;

    void renderShadow(RenderInfo const& info) override;
    void renderForward(RenderInfo const& info) override;

public:
    static SharedQuadricRenderable create(array_view<boxed_quadric const> quadrics);

private:
    void renderQuadrics(glow::SharedProgram const& program, RenderInfo const& info);
    void initFromData(array_view<boxed_quadric const> quadrics);
};
}
}
