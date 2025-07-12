#pragma once

#include "../materials/ColorMapping.hh"
#include "../materials/Masking.hh"
#include "../materials/Texturing.hh"
#include "Renderable.hh"

namespace glow
{
namespace viewer
{
class GeometricRenderable : public Renderable
{
public:
    enum class RenderMode
    {
        Opaque,
        Transparent
    };

private:
    aabb mMeshAABB;

    std::vector<detail::SharedMeshAttribute> mAttributes;
    std::shared_ptr<ColorMapping> mColorMapping;
    std::shared_ptr<Texturing> mTexturing;
    std::shared_ptr<Masking> mMasking;
    detail::SharedMeshDefinition mMeshDefinition;

    glow::SharedElementArrayBuffer mIndexBuffer;

    RenderMode mRenderMode = RenderMode::Opaque;
    bool mFresnel = true;
    bool mBackfaceCullingEnabled = false;
    bool mShadingEnabled = true;
    tg::vec4 mFragmentClipPlane;
    tg::vec4 mPrimitiveClipPlane;

    glow::SharedTextureCubeMap mEnvMap;
    float mEnvReflectivity = 0;

public:
    GLOW_PROPERTY(RenderMode);
    GLOW_PROPERTY(Fresnel);
    GLOW_PROPERTY(BackfaceCullingEnabled);
    GLOW_PROPERTY(ShadingEnabled);
    GLOW_PROPERTY(FragmentClipPlane);
    GLOW_PROPERTY(PrimitiveClipPlane);
    GLOW_PROPERTY(EnvMap);
    GLOW_PROPERTY(EnvReflectivity);
    GLOW_GETTER(MeshAABB);
    GLOW_GETTER(MeshDefinition);
    GLOW_GETTER(Attributes);
    GLOW_GETTER(ColorMapping);
    GLOW_GETTER(Texturing);
    GLOW_GETTER(Masking);
    GLOW_GETTER(IndexBuffer);

    bool isEmpty() const override { return mMeshDefinition->isEmpty(); }

    void addAttribute(detail::SharedMeshAttribute attr);
    bool hasAttribute(std::string const& name) const;
    detail::SharedMeshAttribute getAttribute(std::string const& name) const;

    void setColorMapping(ColorMapping const& cm) { mColorMapping = std::make_shared<ColorMapping>(cm); }
    void setTexturing(Texturing const& t) { mTexturing = std::make_shared<Texturing>(t); }
    void setMasking(Masking const& m) { mMasking = std::make_shared<Masking>(m); }

protected:
    void initGeometry(detail::SharedMeshDefinition def, std::vector<detail::SharedMeshAttribute> attributes);

    // hash for all base settings
    size_t computeGenericGeometryHash() const;
};
}
}
