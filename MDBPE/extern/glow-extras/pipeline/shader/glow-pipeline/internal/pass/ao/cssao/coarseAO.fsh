layout(std140) uniform uCSSAOGlobals {
    uvec4 GlobalConstantBuffer_0; // std140uvec4 buildVersion;

    vec2 GlobalConstantBuffer_1; // std140vec2 invQuarterResolution;
    vec2 GlobalConstantBuffer_2; // std140vec2 invFullResolution;
    vec2 GlobalConstantBuffer_3; // std140vec2 UVToViewA;
    vec2 GlobalConstantBuffer_4; // std140vec2 UVToViewB;

    float GlobalConstantBuffer_5; // std140float fRadiusToScreen;
    float GlobalConstantBuffer_6; // std140float fR2;
    float GlobalConstantBuffer_7; // std140float fNegInvR2;
    float GlobalConstantBuffer_8; // std140float fNDotVBias;

    float GlobalConstantBuffer_9; // std140float fSmallScaleAOAmount;
    float GlobalConstantBuffer_10; // std140float fLargeScaleAOAmount;
    float GlobalConstantBuffer_11; // std140float fPowExponent;
    int GlobalConstantBuffer_12; // std140int iUnused;

    float GlobalConstantBuffer_13; // std140float fBlurViewDepth0;
    float GlobalConstantBuffer_14; // std140float fBlurViewDepth1;
    float GlobalConstantBuffer_15; // std140float fBlurSharpness0;
    float GlobalConstantBuffer_16; // std140float fBlurSharpness1;

    float GlobalConstantBuffer_17; // std140float fLinearizeDepthA;
    float GlobalConstantBuffer_18; // std140float fLinearizeDepthB;
    float GlobalConstantBuffer_19; // std140float fInverseDepthRangeA;
    float GlobalConstantBuffer_20; // std140float fInverseDepthRangeB;

    vec2 GlobalConstantBuffer_21; // std140vec2 f2InputViewportTopLeft;
    float GlobalConstantBuffer_22; // std140float fViewDepthThresholdNegInv;
    float GlobalConstantBuffer_23; // std140float fViewDepthThresholdSharpness;

    float GlobalConstantBuffer_24; // std140float fBackgroundAORadiusPixels;
    float GlobalConstantBuffer_25; // std140float fForegroundAORadiusPixels;
    int GlobalConstantBuffer_26; // std140int iDebugNormalComponent;
};

layout(std140) uniform uCSSAOPerPass {
    vec4 f4Jitter;
    vec2 f2Offset;
    float fSliceIndex;
    uint uSliceIndex;
} uPPCB;

uniform sampler2DArray uInputDepth;
uniform sampler2D uInputNormals;

out float fOut;

#define NUM_DIRECTIONS 8
#define NUM_STEPS 4

const vec2 uvRescaleDenom = vec2(textureSize(uInputNormals, 0)) * GlobalConstantBuffer_2;

vec3 UVToView(vec2 UV, float ViewDepth)
{
    UV = GlobalConstantBuffer_3 * UV + GlobalConstantBuffer_4;
    return vec3(UV * ViewDepth, ViewDepth);
}

vec3 FetchFullResViewNormal(vec2 UV)
{
    return texture(uInputNormals, UV / uvRescaleDenom).xyz * 2.0 - 1.0;
}

vec3 FetchQuarterResViewPos(vec2 UV)
{
    float ViewDepth = texture(uInputDepth, vec3(UV / uvRescaleDenom, 0), 0).x;
    return UVToView(UV, ViewDepth);
}

vec2 RotateDirection(vec2 V, vec2 RotationCosSin)
{
    return vec2(V.x*RotationCosSin.x - V.y*RotationCosSin.y,
                  V.x*RotationCosSin.y + V.y*RotationCosSin.x);
}

float DepthThresholdFactor(float ViewDepth)
{
    return clamp((ViewDepth * GlobalConstantBuffer_22 + 1.0) * GlobalConstantBuffer_23, 0.0, 1.0);
}

struct AORadiusParams
{
    float fRadiusPixels;
    float fNegInvR2;
};

void ScaleAORadius(inout AORadiusParams Params, float ScaleFactor)
{
    Params.fRadiusPixels *= ScaleFactor;
    Params.fNegInvR2 *= 1.0 / (ScaleFactor * ScaleFactor);
}

AORadiusParams GetAORadiusParams(float ViewDepth)
{
    AORadiusParams Params;
    Params.fRadiusPixels = GlobalConstantBuffer_5 / ViewDepth;
    Params.fNegInvR2 = GlobalConstantBuffer_7;

    if (GlobalConstantBuffer_24 != -1.f)
    {
        ScaleAORadius(Params, max(1.0, GlobalConstantBuffer_24 / Params.fRadiusPixels));
    }

    if (GlobalConstantBuffer_25 != -1.f)
    {
        ScaleAORadius(Params, min(1.0, GlobalConstantBuffer_25 / Params.fRadiusPixels));
    }

    return Params;
}

float Falloff(float DistanceSquare, AORadiusParams Params)
{
    return DistanceSquare * Params.fNegInvR2 + 1.0;
}

//----------------------------------------------------------------------------------
// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
//----------------------------------------------------------------------------------
float ComputeAO(vec3 P, vec3 N, vec3 S, AORadiusParams Params)
{
    vec3 V = S - P;
    float VdotV = dot(V, V);
    float NdotV = dot(N, V) * inversesqrt(VdotV);

    return clamp(NdotV - GlobalConstantBuffer_8, 0.0, 1.0) * clamp(Falloff(VdotV, Params), 0.0, 1.0);
}

void AccumulateAO(
    inout float AO,
    inout float RayPixels,
    float StepSizePixels,
    vec2 Direction,
    vec2 FullResUV,
    vec3 ViewPosition,
    vec3 ViewNormal,
    AORadiusParams Params
)
{
    vec2 SnappedUV = round(RayPixels * Direction) * GlobalConstantBuffer_1 + FullResUV;
    vec3 S = FetchQuarterResViewPos(SnappedUV);

    RayPixels += StepSizePixels;

    AO += ComputeAO(ViewPosition, ViewNormal, S, Params);
}

float ComputeCoarseAO(vec2 FullResUV, vec3 ViewPosition, vec3 ViewNormal, AORadiusParams Params)
{
    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    float StepSizePixels = (Params.fRadiusPixels / 4.0) / (NUM_STEPS + 1);

    vec4 Rand = uPPCB.f4Jitter;

    const float Alpha = 2.0 * 3.14159265f / NUM_DIRECTIONS;
    float SmallScaleAO = 0;
    float LargeScaleAO = 0;

    #pragma optionNV (unroll all)
    for (float DirectionIndex = 0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
    {
        float Angle = Alpha * DirectionIndex;

        // Compute normalized 2D direction
        vec2 Direction = RotateDirection(vec2(cos(Angle), sin(Angle)), Rand.xy);

        // Jitter starting sample within the first step
        float RayPixels = (Rand.z * StepSizePixels + 1.0);

        {
            AccumulateAO(SmallScaleAO, RayPixels, StepSizePixels, Direction, FullResUV, ViewPosition, ViewNormal, Params);
        }

        #pragma optionNV (unroll all)
        for (float StepIndex = 1; StepIndex < NUM_STEPS; ++StepIndex)
        {
            AccumulateAO(LargeScaleAO, RayPixels, StepSizePixels, Direction, FullResUV, ViewPosition, ViewNormal, Params);
        }
    }

    float AO = (SmallScaleAO * GlobalConstantBuffer_9) + (LargeScaleAO * GlobalConstantBuffer_10);

    AO /= (NUM_DIRECTIONS * NUM_STEPS);

    return AO;
}

void main()
{
    vec2 inPos = gl_FragCoord.xy * 4.0 + uPPCB.f2Offset;
    vec2 inUv = inPos * (GlobalConstantBuffer_1 / 2.0); // Orig 4

    vec3 ViewPosition = FetchQuarterResViewPos(inUv);
    vec3 ViewNormal = FetchFullResViewNormal(inUv);

    AORadiusParams Params = GetAORadiusParams(ViewPosition.z);

    // Early exit if the projected radius is smaller than 1 full-res pixel
    //[branch]
    if (Params.fRadiusPixels < 1.0)
    {
        fOut = 1.0;
        return;
    }

    float AO = ComputeCoarseAO(inUv, ViewPosition, ViewNormal, Params);

    //[branch]
    if (GlobalConstantBuffer_23 != -1.f)
    {
        AO *= DepthThresholdFactor(ViewPosition.z);
    }

    fOut = clamp(1.0 - AO * 2.0, 0.0, 1.0);
}
