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


uniform sampler2D uInputNearest;
uniform sampler2D uInputLinear;

#define KERNEL_RADIUS 4

//----------------------------------------------------------------------------------
vec2 PointSampleAODepth(vec2 UV)
{
    return texture(uInputNearest, UV).xy;
}
vec2 LinearSampleAODepth(vec2 UV)
{
    return texture(uInputLinear, UV).xy;
}

//----------------------------------------------------------------------------------
struct CenterPixelData
{
    vec2 UV;
    float Depth;
    float Sharpness;
    float Scale;
    float Bias;
};

//----------------------------------------------------------------------------------
float CrossBilateralWeight(float R, float SampleDepth, float DepthSlope, CenterPixelData Center)
{
    const float BlurSigma = (KERNEL_RADIUS+1.0) * 0.5;
    const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);

    SampleDepth -= DepthSlope * R;

    float DeltaZ = SampleDepth * Center.Scale + Center.Bias;

    return exp2(-R*R*BlurFalloff - DeltaZ*DeltaZ);
}

//-------------------------------------------------------------------------
void ProcessSample(vec2 AOZ,
                   float R,
                   float DepthSlope,
                   CenterPixelData Center,
                   inout float TotalAO,
                   inout float TotalW)
{
    float AO = AOZ.x;
    float Z = AOZ.y;

    float W = CrossBilateralWeight(R, Z, DepthSlope, Center);
    TotalAO += W * AO;
    TotalW += W;
}

//-------------------------------------------------------------------------
void ProcessRadius(float R0,
                   vec2 DeltaUV,
                   float DepthSlope,
                   CenterPixelData Center,
                   inout float TotalAO,
                   inout float TotalW)
{
    float R = R0;

    //[unroll]
    for (; R <= KERNEL_RADIUS/2; R += 1)
    {
        vec2 UV = R * DeltaUV + Center.UV;
        vec2 AOZ = PointSampleAODepth(UV);
        ProcessSample(AOZ, R, DepthSlope, Center, TotalAO, TotalW);
    }

    //[unroll]
    for (; R <= KERNEL_RADIUS; R += 2)
    {
        vec2 UV = (R + 0.5) * DeltaUV + Center.UV;
        vec2 AOZ = LinearSampleAODepth(UV);
        ProcessSample(AOZ, R, DepthSlope, Center, TotalAO, TotalW);
    }
}

//-------------------------------------------------------------------------
void ProcessRadius1(vec2 DeltaUV,
                    CenterPixelData Center,
                    inout float TotalAO,
                    inout float TotalW)
{
    vec2 AODepth = PointSampleAODepth(Center.UV + DeltaUV);
    float DepthSlope = AODepth.y - Center.Depth;

    ProcessSample(AODepth, 1, DepthSlope, Center, TotalAO, TotalW);
    ProcessRadius(2, DeltaUV, DepthSlope, Center, TotalAO, TotalW);
}

//-------------------------------------------------------------------------
float ComputeBlur(vec2 uv,
                  vec2 DeltaUV,
                  out float CenterDepth)
{
    // Normalize for oversized input textures (allocAtLeast)
    uv /= vec2(textureSize(uInputLinear, 0)) * GlobalConstantBuffer_2;

    vec2 AOZ = PointSampleAODepth(uv);
    CenterDepth = AOZ.y;

    CenterPixelData Center;
    Center.UV = uv;
    Center.Depth = CenterDepth;
    Center.Sharpness = GlobalConstantBuffer_16;

    Center.Scale = Center.Sharpness;
    Center.Bias = -Center.Depth * Center.Sharpness;

    float TotalAO = AOZ.x;
    float TotalW = 1.0;

    ProcessRadius1(DeltaUV, Center, TotalAO, TotalW);
    ProcessRadius1(-DeltaUV, Center, TotalAO, TotalW);

    return TotalAO / TotalW;
}