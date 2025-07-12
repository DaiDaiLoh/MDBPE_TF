#pragma once

#include <typed-geometry/tg-lean.hh>

#include <glow/fwd.hh>

namespace glow
{
namespace shader
{
class PostProcessing
{
private:
    bool mInitialized = false;

    // shared members
private:
    glow::SharedVertexArray mQuad;

    // gradient
private:
    glow::SharedProgram mProgramGradient;
    glow::SharedProgram mProgramTextureRect;
    glow::SharedProgram mProgramOutput;
    glow::SharedProgram mProgramCopy;

public:
    void gradientRadial(tg::color3 colorInner, tg::color3 colorOuter, tg::pos2 center = {0.5f, 0.5f}, float radius = 0.5f);

    void drawTexture(SharedTextureRectangle const& tex, tg::pos2 start, tg::pos2 end, tg::mat4 const& colorMatrix = tg::mat4::identity);

    // performs gamma correction, dithering, and fxaa
    void output(SharedTextureRectangle const& hdr, bool dithering = true, bool fxaa = true, bool gamma_correction = true);

    void copyFrom(SharedTextureRectangle const& src);

public:
    /// initializes the PP shaders if not already done
    /// (is optional)
    void init();
};
}
}
