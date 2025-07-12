#include "AOGlobalBuffer.hh"

#include <typed-geometry/tg.hh>

#include <cstring>

void glow::pipeline::detail::AOGlobalBuffer::setDepthLinearizationConstants(float nearPlane, float farPlane)
{
    float const invZNear = std::max(1.f / nearPlane, 1.e-6f);
    float const invZFar = std::max(1.f / farPlane, 1.e-6f);

    data.fLinearizeDepthA = invZFar - invZNear;
    data.fLinearizeDepthB = invZNear;
}

void glow::pipeline::detail::AOGlobalBuffer::setProjectionConstants(float tanHalfFovX, float tanHalfFovY)
{
    data.UVToViewA = tg::vec2(2.f * tanHalfFovX, -2.f * tanHalfFovY);
    data.UVToViewB = tg::vec2(-1.f * tanHalfFovX, 1.f * tanHalfFovY);
}

void glow::pipeline::detail::AOGlobalBuffer::setViewportConstants()
{
    data.fInverseDepthRangeA = 1.f;
    data.fInverseDepthRangeB = 0.f;
    data.f2InputViewportTopLeft = tg::vec2(0, 0);
}

void glow::pipeline::detail::AOGlobalBuffer::setBlurConstants(float sharpness, float metersToViewSpaceUnits)
{
    float baseSharpness = std::max(sharpness, 0.f);
    baseSharpness /= metersToViewSpaceUnits;

    data.fBlurSharpness0 = baseSharpness;
    data.fBlurSharpness1 = baseSharpness;
    data.fBlurViewDepth0 = 0.f;
    data.fBlurViewDepth1 = 1.f;
}

void glow::pipeline::detail::AOGlobalBuffer::setAORadiusConstants(float radius, float metersToViewSpaceUnits, float tanHalfFovY, float viewportHeight)
{
    const float R = std::max(radius, 1.e-6f) * metersToViewSpaceUnits;

    data.fR2 = R * R;
    data.fNegInvR2 = -1.f / data.fR2;

    const float TanHalfFovy = tanHalfFovY;
    data.fRadiusToScreen = R * 0.5f / TanHalfFovy * viewportHeight;

    data.fBackgroundAORadiusPixels = -1.f;
    data.fForegroundAORadiusPixels = -1.f;
}

void glow::pipeline::detail::AOGlobalBuffer::SetDepthThresholdConstants()
{
    data.fViewDepthThresholdNegInv = 0.f;
    data.fViewDepthThresholdSharpness = -1.f;
}

glow::pipeline::detail::AOGlobalBuffer::AOGlobalBuffer()
{
    memset(&data, 0, sizeof(data));
}

void glow::pipeline::detail::AOGlobalBuffer::setAOParameters(
    float radius, float metersToViewSpaceUnits, float tanHalfFovY, float viewportHeight, float sharpness, float exponent, float bias, float smallScaleAo, float largeScaleAo)
{
    setAORadiusConstants(radius, metersToViewSpaceUnits, tanHalfFovY, viewportHeight);
    setBlurConstants(sharpness, metersToViewSpaceUnits);
    SetDepthThresholdConstants();

    data.fPowExponent = tg::clamp(exponent, 1.f, 4.f);
    data.fNDotVBias = tg::clamp(bias, 0.0f, 0.5f);

    auto const AOAmountScaleFactor = 1.f / (1.f - data.fNDotVBias);
    data.fSmallScaleAOAmount = tg::clamp(smallScaleAo, 0.f, 2.f) * AOAmountScaleFactor * 2.f;
    data.fLargeScaleAOAmount = tg::clamp(largeScaleAo, 0.f, 2.f) * AOAmountScaleFactor;

    data.iDebugNormalComponent = 0;
}

void glow::pipeline::detail::AOGlobalBuffer::setDepthData(float nearPlane, float farPlane, float tanHalfFovX, float tanHalfFovY)
{
    setDepthLinearizationConstants(nearPlane, farPlane);
    setViewportConstants();
    setProjectionConstants(tanHalfFovX, tanHalfFovY);
}

void glow::pipeline::detail::AOGlobalBuffer::setResolutionConstants(const tg::isize2& viewportSize)
{
    data.invFullResolution = 1.f / tg::vec2(viewportSize);
    data.invQuarterResolution = 1.f / (tg::vec2(viewportSize) * .5f);
}
