#include "LabelRenderable.hh"

#include <glow/common/hash.hh>

#include <glow-extras/vector/backend/opengl.hh>

#include "../RenderInfo.hh"

void glow::viewer::LabelRenderable::renderOverlay(RenderInfo const& info, vector::OGLRenderer const* oglRenderer, tg::isize2 const& res, tg::ipos2 const& /*offset*/)
{
    auto f = oglRenderer->frame(res.width, res.height);
    mImage.clear();

    {
        auto g = graphics(mImage);

        auto MVP = info.proj * info.view * transform();

        for (auto const& l : mLabels)
        {
            if (dot(l.pos - info.camPos, l.normal) > 0) // false for normal == vec3(0)
                continue;

            auto ndc = MVP * l.pos;
            auto x = (ndc.x * .5f + .5f) * res.width;
            auto y = (-ndc.y * .5f + .5f) * res.height;

            if (ndc.z < 0.f || ndc.z > 1)
                continue; // out of frustum

            auto dir = normalize(tg::vec2(x > res.width / 2 ? 1 : -1, y > res.height / 2 ? 1 : -1));
            if (l.style.override_dir != tg::vec2::zero)
                dir = normalize(l.style.override_dir);

            auto ox = x;
            auto oy = y;

            ox += dir.x * l.style.line_start_distance;
            oy += dir.y * l.style.line_start_distance;

            x += dir.x * l.style.pixel_distance;
            y += dir.y * l.style.pixel_distance;

            auto lx = x;
            auto ly = y;

            auto tbb = f.text_bounds(l.style.font, 0, 0, l.text);

            auto w = tbb.max.x - tbb.min.x + 2 * l.style.padding_x;
            auto h = tbb.max.y - tbb.min.y + 2 * l.style.padding_y;

            // TODO: anchoring

            y -= h / 2;
            if (dir.x < 0)
                x -= w;

            auto tx = x + l.style.padding_x - tbb.min.x;
            auto ty = y + l.style.padding_y - tbb.min.y;

            auto bb = tg::aabb2(tg::pos2(x, y), tg::size2(w, h));

            g.draw(tg::segment2({ox, oy}, {lx, ly}), {l.style.line_color, l.style.line_width});
            g.fill(bb, l.style.background);
            if (l.style.border_width > 0)
                g.draw(bb, {l.style.border, l.style.border_width});
            g.text({tx, ty}, l.text, l.style.font, l.style.color);
        }
    }

    f.render(mImage);
}

size_t glow::viewer::LabelRenderable::computeHash() const
{
    // NOTE: is not part of accumulated rendering
    return 0;
}

glow::viewer::SharedLabelRenderable glow::viewer::LabelRenderable::create(array_view<label const> labels)
{
    auto r = std::make_shared<LabelRenderable>();
    r->mLabels.resize(labels.size());
    for (auto i = 0; i < int(labels.size()); ++i)
        r->mLabels[i] = labels[i];
    return r;
}
