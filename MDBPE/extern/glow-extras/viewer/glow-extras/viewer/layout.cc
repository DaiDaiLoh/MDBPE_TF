#include "layout.hh"

namespace
{
template <int D>
constexpr tg::comp<D, int> tg_ifloor(tg::comp<D, float> const& v)
{
    tg::comp<D, int> res;
    for (auto i = 0; i < D; ++i)
        res[i] = tg::ifloor(v[i]);

    return res;
}
}

void glow::viewer::layout::build_linear_nodes(
    glow::viewer::layout::tree_node& root, tg::ipos2 rootStart, tg::isize2 rootSize, std::vector<glow::viewer::layout::linear_node>& out, int margin)
{
    auto scnt = int(root.children.size());

    if (scnt == 0)
    {
        // Leaf, write to out
        out.emplace_back(rootStart, rootSize, root.scene);
        return;
    }

    // copy intended
    auto rows = root.layoutSettings.rows;
    auto cols = root.layoutSettings.cols;

    // auto-grid
    if (rows == -1 && cols == -1)
    {
        auto bestVal = std::numeric_limits<float>::max();

        for (auto ir = 1; ir <= scnt; ++ir)
        {
            auto r = ir;
            auto c = (scnt + r - 1) / r;
            while ((r - 1) * c >= scnt)
                --r;

            TG_ASSERT(r * c >= scnt);
            TG_ASSERT((r - 1) * c <= scnt);
            TG_ASSERT(r * (c - 1) <= scnt);
            TG_ASSERT(r >= 1);
            TG_ASSERT(c >= 1);

            auto ar = (rootSize.width / float(c)) / (rootSize.height / float(r));
            auto val = tg::max(root.layoutSettings.aspectRatio / ar, ar / root.layoutSettings.aspectRatio);
            if (val < bestVal)
            {
                bestVal = val;
                rows = r;
                cols = c;
            }
        }
    }
    else if (rows == -1) // fixed cols
    {
        TG_ASSERT(cols > 0);
        rows = (scnt + cols - 1) / cols;
    }
    else if (cols == -1) // fixed rows
    {
        TG_ASSERT(rows > 0);
        cols = (scnt + rows - 1) / rows;
    }

    TG_ASSERT(rows * cols >= scnt);
    TG_ASSERT(rows >= 1);
    TG_ASSERT(cols >= 1);

    for (auto i = 0; i < scnt; ++i)
    {
        auto& child = root.children[unsigned(i)];

        auto const c = i % cols;
        auto const r = i / cols;
        auto const start = rootStart + tg::ivec2(tg_ifloor(tg::comp2(rootSize) * tg::comp2(c, r) / tg::comp2(cols, rows)));
        auto const end = rootStart + tg::ivec2(tg_ifloor(tg::comp2(rootSize) * tg::comp2(c + 1, r + 1) / tg::comp2(cols, rows)));
        auto const size = end - start - tg::ivec2(margin);

        // Descend
        build_linear_nodes(child, start, tg::isize2(size), out, margin);
    }
}
