#include "TextureRenderable.hh"

#include <sstream>

#include <glow-extras/geometry/Quad.hh>
#include <glow/common/scoped_gl.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Sampler.hh>
#include <glow/objects/Shader.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/VertexArray.hh>

void glow::viewer::TextureRenderable::renderOverlay(RenderInfo const&, const glow::vector::OGLRenderer*, const tg::isize2& res, tg::ipos2 const& offset)
{
    GLOW_SCOPED(disable, GL_CULL_FACE);
    GLOW_SCOPED(disable, GL_DEPTH_TEST);

    auto shader = mShader->use();
    shader["uSize"] = tg::vec2(res);
    shader["uOffset"] = tg::vec2(offset);
    shader.setTexture("uTex", mTexture, mSampler);

    mQuad->bind().draw();
}

void glow::viewer::TextureRenderable::init()
{
    TG_ASSERT(mTexture && "no texture set");

    mQuad = glow::geometry::make_quad();

    mSampler = glow::Sampler::create();
    mSampler->setWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
    mSampler->setBorderColor(tg::color3::black);
    mSampler->setFilter(GL_NEAREST, GL_NEAREST);

    std::stringstream code;

    code << "uniform vec2 uSize;\n";
    code << "uniform vec2 uOffset;\n";
    code << "out vec4 fColor;\n";

    switch (mTexture->getTarget())
    {
    case GL_TEXTURE_2D:
        code << "uniform sampler2D uTex;\n";
        code << "vec4 getColor(vec2 uv) { return texture(uTex, uv); }\n";
        break;
    case GL_TEXTURE_RECTANGLE:
        code << "uniform sampler2DRect uTex;\n";
        code << "vec4 getColor(vec2 uv) { return texture(uTex, uv * textureSize(uTex, 0)); }\n";
        break;
    default:
        glow::error() << "texture of type " << mTexture->getTarget() << " not supported";
        TG_ASSERT(false && "texture type not supported");
        break;
    }

    code << "void main() {\n";
    code << "vec2 ts = vec2(textureSize(uTex, 0));\n";
    code << "float tx = 0;\n";
    code << "float ty = 0;\n";
    code << "float tw = ts.x;\n";
    code << "float th = ts.y;\n";
    code << "if (tw / uSize.x > th / uSize.y) { \n"; // screen too narrow
    code << "  float f = uSize.x / tw; tw *= f; th *= f;\n";
    code << "  ty += (uSize.y - th) / 2;\n";
    code << "} else {\n"; // screen too wide
    code << "  float f = uSize.y / th; tw *= f; th *= f;\n";
    code << "  tx += (uSize.x - tw) / 2;\n";
    code << "}\n";
    code << "vec2 uv = (gl_FragCoord.xy - uOffset - vec2(tx, ty)) / vec2(tw, th); uv.y = 1 - uv.y;\n";

    code << "fColor = vec4(0,0,0,1);";
    code << "if (uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1) fColor.rgb = vec3(0.5 + 0.1 * float((int(gl_FragCoord.x / 16) + int(gl_FragCoord.y / 16)) % 2));\n";
    // code << "else fColor = pow(getColor(uv), vec4(1 / 2.224));\n";
    code << "else fColor = getColor(uv);\n";
    code << "}\n";

    auto fs = glow::Shader::createFromSource(GL_FRAGMENT_SHADER, code.str());
    auto vs = glow::Shader::createFromSource(GL_VERTEX_SHADER, R"(
                                             in vec2 aPosition;

                                             void main() {
                                                gl_Position = vec4(aPosition * 2 - 1, 0, 1);
                                             }
                                             )");
    mShader = glow::Program::create({fs, vs});
}

size_t glow::viewer::TextureRenderable::computeHash() const
{
    return mTexture->getObjectName();
}

glow::viewer::SharedTextureRenderable glow::viewer::TextureRenderable::create(glow::SharedTexture tex)
{
    TG_ASSERT(tex && "must provide texture");
    auto r = std::make_shared<TextureRenderable>();
    r->mTexture = tex;
    return r;
}
