#pragma once

#include "GeometricRenderableBuilder.hh"

namespace glow
{
namespace viewer
{
namespace builder
{
class LineBuilder : public GeometricRenderableBuilder<LineBuilder>
{
public:
    // Caps setting, always exactly one of the three, independent of the other settings
    LineBuilder& round_caps()
    {
        mRoundCaps = true;
        if (mSquareCaps)
            glow::warning() << "square caps cannot be round";
        if (mNoCaps)
            glow::warning() << "no caps cannot be round";
        return *this;
    }
    LineBuilder& square_caps()
    {
        mSquareCaps = true;
        if (mRoundCaps)
            glow::warning() << "round caps cannot be square";
        if (mNoCaps)
            glow::warning() << "no caps cannot be square";
        return *this;
    }
    LineBuilder& no_caps()
    {
        mNoCaps = true;
        if (mRoundCaps)
            glow::warning() << "round caps cannot be none";
        if (mSquareCaps)
            glow::warning() << "square caps cannot be none";
        if (mExtrapolate)
            glow::warning() << "extrapolated caps cannot be none";
        return *this;
    }

    // Extrapolation setting, independent of the other settings
    LineBuilder& extrapolate(bool enabled = true)
    {
        if (enabled)
        {
            mExtrapolate = true;
            if (mNoExtrapolation)
                glow::warning() << "disabled and enabled extrapolation again";
        }
        else
        {
            mNoExtrapolation = true;
            if (mExtrapolate)
                glow::warning() << "enabled and disabled extrapolation again";
        }
        if (mNoCaps)
            glow::warning() << "no caps cannot have extrapolation settings";
        return *this;
    }
    LineBuilder& no_extrapolation()
    {
        mNoExtrapolation = true;
        if (mExtrapolate)
            glow::warning() << "enabled and disabled extrapolation again";
        if (mNoCaps)
            glow::warning() << "no caps cannot have extrapolation settings";
        return *this;
    }

    // Render mode camera facing, excludes 3D and normals
    LineBuilder& camera_facing()
    {
        mCameraFacing = true;
        if (mOwnNormals)
            glow::warning() << "lines with normals cannot be camera facing";
        if (mForce3D)
            glow::warning() << "3D lines cannot be camera facing";
        return *this;
    }

    // Render mode normals, excludes 3D and camera facing modes as well as screen space size
    template <class T>
    LineBuilder& normals(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aNormal", val));
        mOwnNormals = true;
        if (mCameraFacing)
            glow::warning() << "camera facing lines cannot have normals";
        return *this;
    }

    // Makes the lines two-colored (i.e. left-right separate color)
    // Requires normals in the 3D case
    template <class T>
    LineBuilder& two_colored(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aColor", val));
        mForceTwoColored = true;
        return *this;
    }

    // Force 3D render mode, even though setting normals would create flat normal aligned lines. Default is always 3D
    LineBuilder& force3D()
    {
        mForce3D = true;
        if (mCameraFacing)
            glow::warning() << "camera facing lines cannot be 3D";
        // TODO: Caps?
        return *this;
    }

    // Screen space or world space size. Screen space excludes normals
    template <class T>
    LineBuilder& line_width_world(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aLineWidth", val));
        mWorldSpaceSize = true;
        if (mScreenSpaceSize)
            glow::warning() << "lines with screen space width cannot have a world space width";
        return *this;
    }
    template <class T>
    LineBuilder& line_width_px(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aLineWidth", val));
        mScreenSpaceSize = true;
        if (mOwnNormals)
            glow::warning() << "screen space lines cannot have normals";
        if (mWorldSpaceSize)
            glow::warning() << "lines with world space width cannot have a screen space width";
        return *this;
    }

    template <class T>
    LineBuilder& dash_size_world(T const& val)
    {
        addAttribute(detail::make_mesh_attribute("aDashSize", val));
        mDashSizeWorld = true;
        if (!mWorldSpaceSize)
            glow::warning() << "lines with dash size currently need world space sizes";
        return *this;
    }


private:
    bool mSquareCaps = false;
    bool mRoundCaps = false;
    bool mNoCaps = false;
    bool mExtrapolate = false;
    bool mNoExtrapolation = false;
    bool mCameraFacing = false;
    bool mOwnNormals = false;
    bool mForce3D = false;
    bool mWorldSpaceSize = false;
    bool mScreenSpaceSize = false;
    bool mDashSizeWorld = false;
    bool mForceTwoColored = false;

    std::shared_ptr<detail::PolyMeshDefinition> mMeshDef;

public:
    GLOW_GETTER(MeshDef);

public:
    explicit LineBuilder(std::shared_ptr<detail::PolyMeshDefinition> def) : mMeshDef(std::move(def)) {}
    friend class glow::viewer::LineRenderable;
};
}
}
}
