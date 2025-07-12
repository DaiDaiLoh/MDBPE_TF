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

uniform sampler2DRect uInputDepth;

in vec2 vTexCoord;

out vec3 fOut;

vec3 UVToView(vec2 UV, float viewDepth)
{
    UV = GlobalConstantBuffer_3 * UV + GlobalConstantBuffer_4;
    return vec3(UV * viewDepth, viewDepth);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
    vec3 V1 = Pr - P;
    vec3 V2 = P - Pl;
    return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

vec3 ReconstructNormal(vec2 UV, vec3 P)
{
    vec3 Pr = UVToView(UV + vec2(GlobalConstantBuffer_2.x, 0), textureOffset(uInputDepth, gl_FragCoord.xy, ivec2(1, 0)).x);
    vec3 Pl = UVToView(UV + vec2(-GlobalConstantBuffer_2.x, 0), textureOffset(uInputDepth, gl_FragCoord.xy, ivec2(-1, 0)).x);
    vec3 Pt = UVToView(UV + vec2(0, GlobalConstantBuffer_2.y), textureOffset(uInputDepth, gl_FragCoord.xy, ivec2(0, 1)).x);
    vec3 Pb = UVToView(UV + vec2(0, -GlobalConstantBuffer_2.y), textureOffset(uInputDepth, gl_FragCoord.xy, ivec2(0, -1)).x);
    return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

void main()
{
    vec2 uv = vTexCoord.xy;

    vec3 viewPosition = UVToView(uv, textureOffset(uInputDepth, gl_FragCoord.xy, ivec2(0, 0)).x);
    vec3 viewNormal = ReconstructNormal(uv, viewPosition);

    fOut = viewNormal * 0.5 + vec3(.5);
}
