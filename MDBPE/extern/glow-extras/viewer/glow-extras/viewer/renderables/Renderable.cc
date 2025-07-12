#include "Renderable.hh"

#include <iostream>

void glow::viewer::Renderable::onRenderFinished() { mIsDirty = false; }
