#include "OutputStage.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/objects/Framebuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture3D.hh>
#include <glow/objects/TextureRectangle.hh>

#include <glow-extras/geometry/FullscreenTriangle.hh>

#include "../../RenderCallback.hh"
#include "../../RenderScene.hh"
#include "PostprocessingStage.hh"

glow::SharedTexture3D glow::pipeline::OutputStage::generateNeutralLut()
{
    auto res = Texture3D::create(colorLutDimensions, colorLutDimensions, colorLutDimensions, GL_RGB);

    {
        auto constexpr dimF = static_cast<float>(colorLutDimensions - 1);
        GLubyte lutData[colorLutDimensions][colorLutDimensions][colorLutDimensions][3];

        for (auto r = 0u; r < colorLutDimensions; ++r)
            for (auto g = 0u; g < colorLutDimensions; ++g)
                for (auto b = 0u; b < colorLutDimensions; ++b)
                {
                    lutData[r][g][b][2] = static_cast<GLubyte>((r / dimF) * 255);
                    lutData[r][g][b][1] = static_cast<GLubyte>((g / dimF) * 255);
                    lutData[r][g][b][0] = static_cast<GLubyte>((b / dimF) * 255);
                }

        {
            auto tex = res->bind();
            tex.setWrap(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            tex.setFilter(GL_LINEAR, GL_NEAREST);
            tex.setData(GL_RGB8, colorLutDimensions, colorLutDimensions, colorLutDimensions, GL_RGB, GL_UNSIGNED_BYTE, lutData);
        }
    }

    res->setMipmapsGenerated(true);
    return res;
}

void glow::pipeline::OutputStage::onExecuteStage(RenderContext const& ctx, glow::pipeline::RenderCallback&)
{
    PIPELINE_PROFILE("Output Stage");

    if (!mShaderRegistered)
    {
        registerShader(ctx, mShaderOutput);
        mShaderRegistered = true;
    }

    auto shader = mShaderOutput->use();
    shader.setTexture("uInput", mStagePostprocessing->getTargetLdr());
    shader.setTexture("uColorLut", ctx.scene.colorLut ? ctx.scene.colorLut : mNeutralColorLut);

    shader.setUniform("uViewportOffset", mViewportOffset);

    if (mViewportEnabled)
        glViewport(mViewportOffset.x, mViewportOffset.y, mViewportSize.width, mViewportSize.height);

    geometry::FullscreenTriangle::draw();
}

glow::pipeline::OutputStage::OutputStage(SharedPostprocessingStage const& postprocessingStage) : mStagePostprocessing(postprocessingStage)
{
    registerDependency(mStagePostprocessing);

    mShaderOutput = Program::createFromFiles({"glow-pipeline/internal/pass/output/output.fsh", "glow-pipeline/internal/"
                                                                                               "fullscreenTriangle.vsh"});

    geometry::FullscreenTriangle::init();
    mNeutralColorLut = generateNeutralLut();
}

void glow::pipeline::OutputStage::setViewport(const tg::ipos2& offset, const tg::isize2& size)
{
    mViewportOffset = offset;
    mViewportSize = size;
    mViewportEnabled = true;
}

void glow::pipeline::OutputStage::clearViewport()
{
    mViewportEnabled = false;
    mViewportOffset = tg::ipos2(0);
    mViewportSize = tg::isize2(0);
}
