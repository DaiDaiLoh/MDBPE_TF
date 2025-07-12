#pragma once

#include <glow-extras/colors/colormap.hh>

#include <glow/common/log.hh>
#include <glow/fwd.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Texture1D.hh>
#include <glow/objects/Texture2D.hh>
#include <glow/objects/Texture3D.hh>

#include "../fwd.hh"

#include "../detail/MeshAttribute.hh"
#include "../detail/MeshShaderBuilder.hh"

namespace glow
{
namespace viewer
{
class ColorMapping
{
public:
    enum Dimension
    {
        None,
        Map1D,
        Map2D,
        Map3D
    };

private:
    Dimension mMode = None;
    bool mClamped = false;
    glow::SharedTexture mTexture;
    detail::SharedMeshAttribute mDataAttribute;

    tg::vec3 mDataMin = tg::vec3(0.0f);
    tg::vec3 mDataMax = tg::vec3(1.0f);

    void setMode(Dimension m)
    {
        if (mMode != None && mMode != m)
            glow::warning() << "ColorMapping already has mode " << mMode << " but is now set to " << m;
        mMode = m;
    }

    void setModeFromData(float*) { setMode(Map1D); }
    void setModeFromData(tg::vec2*) { setMode(Map2D); }
    void setModeFromData(tg::vec3*) { setMode(Map3D); }

    void buildShader(detail::MeshShaderBuilder& shader) const;
    void prepareShader(UsedProgram& shader) const;

public:
    size_t computeHash() const;

    ColorMapping& range(float min, float max)
    {
        mDataMin = tg::vec3(min);
        mDataMax = tg::vec3(max);
        return *this;
    }
    ColorMapping& range(tg::vec2 min, tg::vec2 max)
    {
        setMode(Map2D);
        mDataMin = tg::vec3(min, 0);
        mDataMax = tg::vec3(max, 1);
        return *this;
    }
    ColorMapping& range(tg::vec3 min, tg::vec3 max)
    {
        setMode(Map3D);
        mDataMin = min;
        mDataMax = max;
        return *this;
    }

    ColorMapping& linear(glow::colors::color c0 = {0, 0, 0}, glow::colors::color c1 = {1, 1, 1}, float min = 0.0f, float max = 1.0f)
    {
        range(min, max);
        return texture(glow::colors::colormap::linear(512, c0, c1).texture());
    }

    ColorMapping& clamped()
    {
        mClamped = true;
        return *this;
    }

    ColorMapping& texture(glow::SharedTexture1D const& tex)
    {
        setMode(Map1D);
        mTexture = tex;
        return *this;
    }
    ColorMapping& texture(glow::SharedTexture2D const& tex)
    {
        setMode(Map2D);
        mTexture = tex;
        return *this;
    }
    ColorMapping& texture(glow::SharedTexture3D const& tex)
    {
        setMode(Map3D);
        mTexture = tex;
        return *this;
    }

    template <class T>
    ColorMapping& data(T const& data)
    {
        if (mDataAttribute)
            glow::warning() << "ColorMapping already has a data attribute";
        auto md = detail::make_mesh_attribute("aData", data);

        // extract data type and use it to set mapping mode
        using AttrT = typename std::decay<decltype(*md)>::type;
        setModeFromData(static_cast<typename AttrT::data_t*>(nullptr));

        mDataAttribute = md;
        return *this;
    }

    friend class MeshRenderable;
    friend class PointRenderable;
    friend class LineRenderable;
};
}
}
