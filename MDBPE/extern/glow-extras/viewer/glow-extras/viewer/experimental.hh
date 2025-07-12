#pragma once

#include <glow-extras/viewer/view.hh>

namespace glow::viewer::experimental
{
/// Query the 3D position at the given pixel, only usable in interactive viewers
inline cc::optional<tg::pos3> interactive_get_position(int x, int y) { return detail::query_current_viewer_3d_position(x, y); }

/// Query the 3D position at the given pixel, only usable in interactive viewers
inline cc::optional<tg::pos3> interactive_get_position(tg::pos2 const pos) { return interactive_get_position(pos.x, pos.y); }

/// Get the window size, only usable in interactive viewers
inline tg::isize2 interactive_get_window_size() { return detail::query_current_viewer_window_size(); }

/// Get the last valid mouse position, only usable in interactive viewers.
inline tg::pos2 interactive_get_mouse_position() { return detail::query_mouse_position(); }

/// reset the camera to the current geometry of the viewer
inline void interactive_reset_camera(bool clip_cam = false) { return detail::reset_camera_to_scene(clip_cam); }

/// Returns true iff the window is in fullscreen, only usable in interactive viewers.
inline bool interactive_is_fullscreen() { return detail::is_fullscreen(); }

/// Toggles fullscreen mode of the viewer, only usable in interactive viewers.
inline void interactive_toggle_fullscreen() { return detail::toggle_fullscreen(); }

/// Returns true iff we are currently inside an interactive viewer
inline bool is_in_interactive() { return detail::is_in_viewer(); }
}
