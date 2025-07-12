#pragma once

#include <vector>

#include <typed-geometry/tg-lean.hh>

#include <glow/common/shared.hh>

#include <glow-extras/viewer/Scene.hh>

namespace glow::viewer::layout
{
struct settings
{
    int cols = -1;
    int rows = -1;
    float aspectRatio = 1.f;
};

// layout tree node
// intermediate data structure to build linear_node arrays
struct tree_node
{
    tree_node* parent;
    std::vector<tree_node> children;

    settings layoutSettings;
    Scene scene;

    explicit tree_node(tree_node* p = nullptr) : parent(p) {}
};

// linear node
// layouted, used in rendering
struct linear_node
{
    tg::ipos2 start;
    tg::isize2 size;
    Scene scene;

    linear_node(tg::ipos2 start, tg::isize2 size, Scene scene) : start(start), size(size), scene(std::move(scene)) {}
};

// Transforms a given tree into a vector of correctly layouted linear nodes
void build_linear_nodes(tree_node& root, tg::ipos2 rootStart, tg::isize2 rootSize, std::vector<linear_node>& out, int margin);
}
