#pragma once

#include <glow/fwd.hh>
#include <glow/objects/Program.hh>
#include <glow/util/AsyncTexture.hh>

#include "../detail/MeshAttribute.hh"
#include "../detail/MeshShaderBuilder.hh"
#include "../fwd.hh"

#include <typed-geometry/tg-lean.hh>

namespace glow
{
namespace viewer
{
class Texturing
{
    struct ExplicitTexture : AsyncTextureBase
    {
        SharedTexture mTexture;

        ExplicitTexture(SharedTexture tex) : mTexture(std::move(tex)) {}

        SharedTexture queryTexture() const override { return mTexture; }
    };

private:
    std::shared_ptr<AsyncTextureBase> mTexture;
    detail::SharedMeshAttribute mCoordsAttribute;
    tg::mat3 mTextureTransform = tg::mat3::identity;

    void buildShader(detail::MeshShaderBuilder& shader) const;
    void prepareShader(UsedProgram& shader) const;

public:
    size_t computeHash() const;

    Texturing& texture(SharedTexture t)
    {
        mTexture = std::make_shared<ExplicitTexture>(std::move(t));
        return *this;
    }
    template <class T>
    Texturing& texture(AsyncTexture<T> const& t)
    {
        mTexture = std::make_shared<AsyncTexture<T>>(t);
        return *this;
    }
    template <class T>
    Texturing& coords(T const& data)
    {
        mCoordsAttribute = detail::make_mesh_attribute("aTexCoord", data);
        return *this;
    }
    Texturing& transform(tg::mat3 const& m)
    {
        mTextureTransform = m;
        return *this;
    }
    Texturing& flip(bool flip_x = false, bool flip_y = true)
    {
        if (flip_x)
            mTextureTransform = mTextureTransform * tg::translation(1.0f, 0.0f) * tg::scaling(-1.f, 1.f);
        if (flip_y)
            mTextureTransform = mTextureTransform * tg::translation(0.0f, 1.0f) * tg::scaling(1.f, -1.f);
        return *this;
    }

    friend class MeshRenderable;
    friend class PointRenderable;
    friend class LineRenderable;
};
}
}
