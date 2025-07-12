#pragma once

#include "GeometricRenderableBuilder.hh"

namespace glow
{
namespace viewer
{
namespace builder
{
class PointBuilder : public GeometricRenderableBuilder<PointBuilder>
{
public:
    enum class PointMode
    {
        Unspecified,
        Round,
        Square,
        Spheres
    };

public:
    PointBuilder& round()
    {
        if (mPointMode != PointMode::Unspecified)
            glow::warning() << "overrode point rendering mode";
        mPointMode = PointMode::Round;
        return *this;
    }
    PointBuilder& square()
    {
        if (mPointMode != PointMode::Unspecified)
            glow::warning() << "overrode point rendering mode";
        mPointMode = PointMode::Square;
        return *this;
    }
    PointBuilder& spheres()
    {
        if (mPointMode != PointMode::Unspecified)
            glow::warning() << "overrode point rendering mode";
        mPointMode = PointMode::Spheres;
        // Spheres are always camera facing
        camera_facing();
        return *this;
    }
    PointBuilder& camera_facing()
    {
        mCameraFacing = true;
        if (mOwnNormals)
            glow::warning() << "points with normals cannot be camera facing";
        return *this;
    }

    template <class T>
    PointBuilder& normals(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aNormal", val));
        mOwnNormals = true;
        if (mScreenSpaceSize)
            glow::warning() << "screen space sized points cannot have normals";
        if (mCameraFacing)
            glow::warning() << "camera facing points cannot have normals";
        return *this;
    }
    template <class T>
    PointBuilder& point_size_world(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aPointSize", val));
        mWorldSpaceSize = true;
        if (mScreenSpaceSize)
            glow::warning() << "screen space sized points cannot have a world space size";
        return *this;
    }
    template <class T>
    PointBuilder& point_size_px(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aPointSize", val));
        mScreenSpaceSize = true;
        if (mOwnNormals)
            glow::warning() << "screen space sized points cannot have normals";
        if (mWorldSpaceSize)
            glow::warning() << "world space sized points cannot have a screen space size";
        return *this;
    }

private:
    bool mCameraFacing = false;
    bool mOwnNormals = false;
    bool mWorldSpaceSize = false;
    bool mScreenSpaceSize = false;
    PointMode mPointMode = PointMode::Unspecified;

    std::shared_ptr<detail::PolyMeshDefinition> mMeshDef;

public:
    GLOW_GETTER(MeshDef);

public:
    explicit PointBuilder(std::shared_ptr<detail::PolyMeshDefinition> def) : mMeshDef(std::move(def)) {}
    friend class glow::viewer::PointRenderable;
};
}
}
}
