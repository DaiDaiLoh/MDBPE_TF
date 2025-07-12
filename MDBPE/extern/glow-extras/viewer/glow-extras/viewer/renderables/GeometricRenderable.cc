#include "GeometricRenderable.hh"

#include <glow/common/hash.hh>

using namespace glow;
using namespace glow::viewer;

void GeometricRenderable::addAttribute(viewer::detail::SharedMeshAttribute attr)
{
    if (hasAttribute(attr->name()))
    {
        glow::error() << "Attribute " << attr->name() << " already added";
        return;
    }
    mAttributes.emplace_back(std::move(attr));
}

bool GeometricRenderable::hasAttribute(const std::string& name) const
{
    for (auto const& attr : mAttributes)
        if (attr->name() == name)
            return true;

    return false;
}

viewer::detail::SharedMeshAttribute GeometricRenderable::getAttribute(const std::string& name) const
{
    for (auto const& attr : mAttributes)
        if (attr->name() == name)
            return attr;

    return nullptr;
}

void GeometricRenderable::initGeometry(viewer::detail::SharedMeshDefinition def, std::vector<viewer::detail::SharedMeshAttribute> attributes)
{
    TG_ASSERT(def != nullptr);

    mAttributes = std::move(attributes);

    addAttribute(def->computePositionAttribute());

    mIndexBuffer = def->computeIndexBuffer();
    mMeshAABB = def->computeAABB();
    mMeshDefinition = std::move(def);
}

size_t GeometricRenderable::computeGenericGeometryHash() const
{
    size_t h = 0x741231;
    h = glow::hash_xxh3(as_byte_view(transform()), h);
    h = glow::hash_xxh3(as_byte_view(mRenderMode), h);
    h = glow::hash_xxh3(as_byte_view(mPrimitiveClipPlane), h);
    h = glow::hash_xxh3(as_byte_view(mFragmentClipPlane), h);
    h = glow::hash_xxh3(as_byte_view(mFresnel), h);
    h = glow::hash_xxh3(as_byte_view(mBackfaceCullingEnabled), h);
    h = glow::hash_xxh3(as_byte_view(mShadingEnabled), h);
    h = glow::hash_xxh3(as_byte_view(mMeshDefinition->queryHash()), h);
    if (mColorMapping)
        h = glow::hash_xxh3(as_byte_view(mColorMapping->computeHash()), h);
    if (mTexturing)
        h = glow::hash_xxh3(as_byte_view(mTexturing->computeHash()), h);
    if (mMasking)
        h = glow::hash_xxh3(as_byte_view(mMasking->computeHash()), h);
    for (auto const& a : mAttributes)
        h = glow::hash_xxh3(as_byte_view(a->queryHash()), h);
    return h;
}
