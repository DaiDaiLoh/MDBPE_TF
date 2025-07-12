#include "Masking.hh"

#include <glow/common/hash.hh>

using namespace glow;
using namespace glow::viewer;

void Masking::buildShader(glow::viewer::detail::MeshShaderBuilder& shader) const
{
    shader.addUniform("float", "uMaskingThreshold");
    shader.addPassthrough(mDataAttribute->typeInShader(), "Visible");

    shader.addFragmentShaderCode("    if(vIn.Visible < uMaskingThreshold) discard;");
}

void Masking::prepareShader(glow::UsedProgram& shader) const
{
    shader.setUniform("uMaskingThreshold", mThreshold);
    mDataAttribute->prepareShader(shader);
}

size_t Masking::computeHash() const
{
    auto h = mDataAttribute->queryHash();
    h = glow::hash_xxh3(as_byte_view(mThreshold), h);
    return h;
}
