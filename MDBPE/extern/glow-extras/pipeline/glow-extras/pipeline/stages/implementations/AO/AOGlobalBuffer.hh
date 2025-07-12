#pragma once

#include <algorithm>

#include <glow/std140.hh>

namespace glow
{
namespace pipeline
{
namespace detail
{
struct AOGlobalsUBO
{
    std140uvec4 buildVersion;

    std140vec2 invQuarterResolution;
    std140vec2 invFullResolution;

    std140vec2 UVToViewA;
    std140vec2 UVToViewB;

    std140float fRadiusToScreen;
    std140float fR2;
    std140float fNegInvR2;
    std140float fNDotVBias;

    std140float fSmallScaleAOAmount;
    std140float fLargeScaleAOAmount;
    std140float fPowExponent;
    std140int iUnused;

    std140float fBlurViewDepth0;
    std140float fBlurViewDepth1;
    std140float fBlurSharpness0;
    std140float fBlurSharpness1;

    std140float fLinearizeDepthA;
    std140float fLinearizeDepthB;
    std140float fInverseDepthRangeA;
    std140float fInverseDepthRangeB;

    std140vec2 f2InputViewportTopLeft;
    std140float fViewDepthThresholdNegInv;
    std140float fViewDepthThresholdSharpness;

    std140float fBackgroundAORadiusPixels;
    std140float fForegroundAORadiusPixels;
    std140int iDebugNormalComponent;
};

class AOGlobalBuffer
{
private:
    void setDepthLinearizationConstants(float nearPlane, float farPlane);
    void setProjectionConstants(float tanHalfFovX, float tanHalfFovY);
    void setViewportConstants();

    void setBlurConstants(float sharpness, float metersToViewSpaceUnits);
    void setAORadiusConstants(float radius, float metersToViewSpaceUnits, float tanHalfFovY, float viewportHeight);
    void SetDepthThresholdConstants();

public:
    AOGlobalsUBO data;

    AOGlobalBuffer();

    void setAOParameters(float radius, float metersToViewSpaceUnits, float tanHalfFovY, float viewportHeight, float sharpness, float exponent, float bias, float smallScaleAo, float largeScaleAo);

    void setDepthData(float nearPlane, float farPlane, float tanHalfFovX, float tanHalfFovY);

    void setResolutionConstants(tg::isize2 const& viewportSize);
};
}
}
}
