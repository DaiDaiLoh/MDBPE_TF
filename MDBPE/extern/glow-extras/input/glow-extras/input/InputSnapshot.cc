#include "InputSnapshot.hh"

#include <typed-geometry/tg.hh>

void glow::input::InputSnapshot::addMouseDelta(const tg::dvec2& delta) { mousePosition += delta; }

void glow::input::InputSnapshot::addScrollDelta(const tg::vec2& delta) { scrollPosition += delta; }

tg::dvec2 glow::input::InputSnapshot::getMouseDelta(const glow::input::InputSnapshot& prev) const { return mousePosition - prev.mousePosition; }

tg::vec2 glow::input::InputSnapshot::getScrollDelta(const glow::input::InputSnapshot& prev) const { return scrollPosition - prev.scrollPosition; }
