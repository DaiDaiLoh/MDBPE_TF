#pragma once

#include <typed-geometry/types/color.hh>

namespace glow::viewer
{
struct no_grid_t
{
    bool active = true;
    no_grid_t operator()(bool a) const { return {a}; }
};
struct no_shading_t
{
    bool active = true;
    no_shading_t operator()(bool a) const { return {a}; }
};
struct no_shadow_t
{
    bool active = true;
    no_shadow_t operator()(bool a) const { return {a}; }
};
struct no_backfacing_shadow_t
{
    bool active = true;
    no_backfacing_shadow_t operator()(bool a) const { return {a}; }
};
struct print_mode_t
{
    bool active = true;
    print_mode_t operator()(bool a) const { return {a}; }
};
struct no_outline_t
{
    bool active = true;
    no_outline_t operator()(bool a) const { return {a}; }
};
struct transparent_t
{
};
struct opaque_t
{
};
struct maybe_empty_t
{
};
struct no_left_mouse_control_t
{
    bool active = true;
    no_left_mouse_control_t operator()(bool a) const { return {a}; }
};
struct no_right_mouse_control_t
{
    bool active = true;
    no_right_mouse_control_t operator()(bool a) const { return {a}; }
};
struct no_viewer_ui_t
{
    bool active = true;
    no_viewer_ui_t operator()(bool a) const { return {a}; }
};
struct no_fresnel_t
{
    bool active = true;
    no_fresnel_t operator()(bool a) const { return {a}; }
};
struct infinite_accumulation_t
{
    bool active = true;
    infinite_accumulation_t operator()(bool a) const { return {a}; }
};

struct controls_2d_t
{
    bool active = true;
    controls_2d_t operator()(bool a) const { return {a}; }
};
struct dark_ui_t
{
    bool active = true;
    dark_ui_t operator()(bool a) const { return {a}; }
};
struct clear_accumulation_t
{
    bool active = true;
    clear_accumulation_t operator()(bool a) const { return {a}; }
};
struct subview_margin_t
{
    int pixels = 0;
    tg::color3 color = tg::color3(0, 0, 0);
    subview_margin_t operator()(int px) const { return {px, tg::color3(0, 0, 0)}; }
    subview_margin_t operator()(tg::color3 col) const { return {1, col}; }
    subview_margin_t operator()(int px, tg::color3 col) const { return {px, col}; }
};
struct tonemap_exposure_t
{
    float exposure = 1.f;
    tonemap_exposure_t operator()(float exposure) const { return {exposure}; }
};
struct backface_culling_t
{
    bool active = true;
    backface_culling_t operator()(bool a) const { return {a}; }
};
struct preserve_camera_t
{
    bool active = true;
    preserve_camera_t operator()(bool a) const { return {a}; }
};
struct reuse_camera_t
{
    bool active = true;
    reuse_camera_t operator()(bool a) const { return {a}; }
};
}
