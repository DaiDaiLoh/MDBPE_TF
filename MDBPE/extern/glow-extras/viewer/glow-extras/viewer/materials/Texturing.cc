#include "Texturing.hh"

#include <glow/objects/Texture.hh>

#include <glow/common/hash.hh>

using namespace glow;
using namespace glow::viewer;

void Texturing::buildShader(glow::viewer::detail::MeshShaderBuilder& shader) const
{
    shader.addPassthrough(mCoordsAttribute->typeInShader(), "TexCoord");

    // TODO: other sampler types
    shader.addUniform("sampler2D", "uTexture");
    shader.addUniform("mat3", "uTextureTransform");

    shader.addFragmentShaderCode("vec3 uv = uTextureTransform * vec3(vTexCoord, 1);");
    shader.addFragmentShaderCode("vColor *= texture(uTexture, uv.xy / uv.z);");
}

void Texturing::prepareShader(glow::UsedProgram& shader) const
{
    TG_ASSERT(mTexture && "no texture set");
    shader.setTexture("uTexture", mTexture->queryTexture());
    shader["uTextureTransform"] = mTextureTransform;
}

size_t Texturing::computeHash() const
{
    // TODO: clear accum when texture is loaded
    auto h = mCoordsAttribute->queryHash();
    h = glow::hash_xxh3(as_byte_view(mTextureTransform), h);
    if (mTexture)
    {
        auto t = mTexture->queryTexture();
        if (t)
            h = glow::hash_xxh3(as_byte_view(t->getObjectName()), h);
    }
    return h;
}
