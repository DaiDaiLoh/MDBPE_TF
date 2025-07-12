#include "configure.hh"

#include <cstring>
#include <memory>
#include <typed-geometry/tg.hh>

#include <glow/objects/TextureCubeMap.hh>

#include "Scene.hh"
#include "detail/command_queue.hh"
#include "materials/envmap.hh"
#include "renderables/GeometricRenderable.hh"
#include "renderables/LineRenderable.hh"
#include "renderables/MeshRenderable.hh"
#include "renderables/PointRenderable.hh"
#include "renderables/Renderable.hh"

namespace glow::viewer
{
void configure(Renderable& r, tg::mat4 const& transform)
{
    if (r.isNullRenderable())
        glow::warning() << "v.configure(mat4) does not work as there is no renderable to transform";
    if (r.transform() != transform)
    {
        r.transform(transform);
        r.clearHash();
    }
}
#ifdef GLOW_HAS_GLM
void configure(Renderable& r, glm::mat4 const& transform)
{
    tg::mat4 m;
    std::memcpy(data_ptr(m), &transform[0][0], sizeof(m));
    r.transform(m);
    r.clearHash();
}
#endif

void configure(GeometricRenderable& r, glow::colors::color const& c)
{
    r.addAttribute(detail::make_mesh_attribute("aColor", c));
    if (c.a < 1)
        r.setRenderMode(GeometricRenderable::RenderMode::Transparent);
    r.clearHash();
}
void configure(GeometricRenderable& r, ColorMapping const& cm)
{
    r.setColorMapping(cm);
    r.clearHash();
}
void configure(GeometricRenderable& r, Texturing const& t)
{
    r.setTexturing(t);
    r.clearHash();
}
void configure(GeometricRenderable& r, Masking const& m)
{
    r.setMasking(m);
    r.clearHash();
}

void configure(Renderable&, no_grid_t b)
{
    if (b.active)
        detail::submit_command(detail::command::sceneNoGrid());
}
void configure(Renderable&, no_shadow_t s)
{
    if (s.active)
        detail::submit_command(detail::command::sceneNoShadow());
}
void configure(Renderable&, no_backfacing_shadow_t s)
{
    if (s.active)
        detail::submit_command(detail::command::sceneNoBackfacingShadow());
}
void configure(Renderable&, print_mode_t b)
{
    if (b.active)
        detail::submit_command(detail::command::scenePrintMode());
}
void configure(Renderable&, no_outline_t b)
{
    if (b.active)
        detail::submit_command(detail::command::sceneNoOutline());
}
void configure(Renderable&, infinite_accumulation_t b)
{
    if (b.active)
        detail::submit_command(detail::command::sceneInfiniteAccumulation());
}
void configure(Renderable& r, maybe_empty_t)
{
    if (r.isNullRenderable())
        glow::warning() << "v.configure(maybe_empty) does not work as there is no renderable to configure";
    r.setCanBeEmpty();
}

void configure(Renderable&, no_left_mouse_control_t b) { detail::set_left_mouse_control(!b.active); }
void configure(Renderable&, no_right_mouse_control_t b) { detail::set_right_mouse_control(!b.active); }
void configure(Renderable&, no_viewer_ui_t b) { detail::set_ui_enabled(!b.active); }

void configure(Renderable&, controls_2d_t b) { detail::set_2d_controls(b.active); }
void configure(Renderable&, dark_ui_t b) { detail::set_ui_darkmode(b.active); }
void configure(Renderable&, background_color const& b) { detail::submit_command(detail::command::sceneBackgroundColor(b.inner, b.outer)); }
void configure(Renderable&, ssao_power const& b) { detail::submit_command(detail::command::sceneSsaoPower(b.power)); }
void configure(Renderable&, ssao_radius const& b) { detail::submit_command(detail::command::sceneSsaoRadius(b.radius)); }
void configure(Renderable&, const tonemap_exposure_t& b) { detail::submit_command(detail::command::sceneTonemapping(b.exposure)); }
void configure(Renderable&, const camera_orientation& b)
{
    detail::submit_command(detail::command::sceneCameraOrientation(b.azimuth, b.altitude, b.distance));
}
void configure(Renderable&, const camera_transform& b) { detail::submit_command(detail::command::sceneCameraTransform(b.pos, b.target)); }
void configure(Renderable&, const camera_fov& b) { detail::submit_command(detail::command::sceneCameraFov(b.fov)); }

void configure(Renderable& r, const char* name)
{
    if (r.isNullRenderable())
        glow::warning() << "v.configure(string) does not work as there is no renderable to name";
    r.name(name);
    // name change does not clear hash
}
void configure(Renderable& r, std::string_view name)
{
    if (r.isNullRenderable())
        glow::warning() << "v.configure(string) does not work as there is no renderable to name";
    r.name(std::string(name));
    // name change does not clear hash
}
void configure(Renderable& r, const std::string& name)
{
    if (r.isNullRenderable())
        glow::warning() << "v.configure(string) does not work as there is no renderable to name";
    r.name(name);
    // name change does not clear hash
}

void configure(GeometricRenderable& r, transparent_t)
{
    r.setRenderMode(GeometricRenderable::RenderMode::Transparent);
    r.clearHash();
}
void configure(GeometricRenderable& r, opaque_t)
{
    r.setRenderMode(GeometricRenderable::RenderMode::Opaque);
    r.clearHash();
}
void configure(GeometricRenderable& r, no_fresnel_t)
{
    r.setFresnel(false);
    r.clearHash();
}

void configure(GeometricRenderable& r, backface_culling_t b)
{
    r.setBackfaceCullingEnabled(b.active);
    r.clearHash();
}

void configure(Renderable&, clear_accumulation_t b)
{
    if (b.active)
        detail::submit_command(detail::command::sceneClearAccum());
}

void configure(Renderable&, subview_margin_t b)
{
    detail::set_subview_margin(b.pixels);
    detail::set_subview_margin_color(b.color);
}

void configure(Renderable&, const headless_screenshot& b)
{
    detail::set_headless_screenshot(b.resolution, b.accumulationCount, b.filename, b.format);
}

void configure(Renderable&, SharedRenderable const& rj) { submit_command(detail::command::addRenderjob(rj)); }

void configure(Renderable&, const close_keys& k) { submit_command(detail::command::sceneCloseKeys(k.keys)); }

void configure(Renderable&, const orthogonal_projection& p) { detail::submit_command(detail::command::sceneOrthogonalProjection(p.bounds)); }

void configure(GeometricRenderable& r, no_shading_t s) { r.setShadingEnabled(!s.active); }

void configure(Renderable&, preserve_camera_t b)
{
    if (b.active)
        detail::submit_command(detail::command::scenePreserveCamera());
}

void configure(Renderable&, reuse_camera_t b)
{
    if (b.active)
        detail::submit_command(detail::command::sceneReuseCamera());
}

void configure(Renderable&, SharedCameraController c) { detail::submit_command(detail::command::sceneCustomCameraController(std::move(c))); }

void configure(Renderable&, const tg::aabb3& bounds) { detail::submit_command(detail::command::sceneCustomAabb(bounds)); }

void configure(Renderable&, std::function<void(SceneConfig&)> f) { detail::submit_command(detail::command::sceneCustomConfig(std::move(f))); }

void configure(Renderable&, const grid_size& v)
{
    detail::submit_command(detail::command::sceneCustomConfig([s = v.size](SceneConfig& cfg) { cfg.customGridSize = s; }));
}
void configure(Renderable&, const grid_center& v)
{
    detail::submit_command(detail::command::sceneCustomConfig([c = v.center](SceneConfig& cfg) { cfg.customGridCenter = c; }));
}

void configure(Renderable&, const shadow_screen_fadeout_distance& v)
{
    detail::submit_command(detail::command::sceneCustomConfig([r = v.radius](SceneConfig& cfg) { cfg.shadowScreenFadeoutDistance = r; }));
}
void configure(Renderable&, const shadow_world_fadeout_factor& v)
{
    detail::submit_command(detail::command::sceneCustomConfig(
        [i = v.inner_factor, o = v.outer_factor](SceneConfig& cfg)
        {
            cfg.shadowWorldFadeoutFactorInner = i;
            cfg.shadowWorldFadeoutFactorOuter = o;
        }));
}
void configure(Renderable&, const shadow_strength& v)
{
    detail::submit_command(detail::command::sceneCustomConfig([s = v.strength](SceneConfig& cfg) { cfg.shadowStrength = s; }));
}
void configure(Renderable&, const sun_offset_factor& v)
{
    detail::submit_command(detail::command::sceneCustomConfig([f = v.factor](SceneConfig& cfg) { cfg.sunOffsetFactor = f; }));
}
void configure(Renderable&, const sun_scale_factor& v)
{
    detail::submit_command(detail::command::sceneCustomConfig([f = v.factor](SceneConfig& cfg) { cfg.sunScaleFactor = f; }));
}
void configure(Renderable&, const total_shadow_samples& v)
{
    detail::submit_command(detail::command::sceneCustomConfig([s = v.samples](SceneConfig& cfg) { cfg.maxShadowSamples = s; }));
}

void configure(GeometricRenderable& r, const clip_plane& p) { r.setFragmentClipPlane(tg::vec4(p.normal, dot(p.normal, p.pos))); }

void configure(GeometricRenderable& r, const primitive_clip_plane& p) { r.setPrimitiveClipPlane(tg::vec4(p.plane.normal, p.plane.dis)); }

void configure(GeometricRenderable& r, const EnvMap& em)
{
    r.setEnvMap(em.texture);
    r.setEnvReflectivity(em.reflectivity);
}

void configure(GeometricRenderable& r, Picker p)
{
    // user-defined IDs
    if (auto const& vert_attrib = p.getUserDefinedVertexIds())
        r.addAttribute(detail::make_mesh_attribute("aPickID", vert_attrib.value()));
    else if (auto const& edge_attrib = p.getUserDefinedEdgeIds())
        r.addAttribute(detail::make_mesh_attribute("aPickID", edge_attrib.value()));
    else if (auto const& face_attrib = p.getUserDefinedFaceIds())
        r.addAttribute(detail::make_mesh_attribute("aPickID", face_attrib.value()));
    r.setPicker(std::move(p));
}

void configure(Renderable&, file_drop_handler const& h) { detail::set_file_drop_handler(h.callback); }

void configure(Renderable&, cache_window_size const& s) { detail::set_cache_window_size_folder(s.folder); }



} // namespace glow::viewer
