#include <polymesh/formats.hh>

#include <glow/objects/Texture2D.hh>
#include <glow/objects/TextureRectangle.hh>

#include "make_renderable.hh"

namespace glow::viewer
{
SharedMeshRenderable make_renderable(builder::PolygonBuilder const& builder)
{
    auto r = MeshRenderable::create(builder);
    r->transform(builder.getTransform());
    return r;
}

SharedPointRenderable make_renderable(builder::PointBuilder const& builder)
{
    auto r = PointRenderable::create(builder);
    r->transform(builder.getTransform());
    return r;
}

SharedLineRenderable make_renderable(builder::LineBuilder const& builder)
{
    auto r = LineRenderable::create(builder);
    r->transform(builder.getTransform());
    return r;
}

SharedVector2DRenderable make_renderable(const vector::image2D& img) { return Vector2DRenderable::create(img); }

SharedGeometricRenderable make_renderable(file const& file)
{
    pm::Mesh m;
    pm::vertex_attribute<tg::pos3> pos(m);
    load(file.filepath, m, pos);
    return make_renderable(pos);
}

SharedTextureRenderable make_renderable(const SharedTexture2D& tex) { return TextureRenderable::create(tex); }

SharedTextureRenderable make_renderable(const SharedTextureRectangle& tex) { return TextureRenderable::create(tex); }

SharedLabelRenderable make_renderable(const label& label) { return LabelRenderable::create({label}); }

SharedLabelRenderable make_renderable(glow::array_view<const label> labels) { return LabelRenderable::create(labels); }

SharedQuadricRenderable make_renderable(const boxed_quadric& q) { return QuadricRenderable::create(array_view<boxed_quadric const>{&q, 1}); }

SharedQuadricRenderable make_renderable(array_view<const boxed_quadric> qs) { return QuadricRenderable::create(qs); }

} // namespace viewer
