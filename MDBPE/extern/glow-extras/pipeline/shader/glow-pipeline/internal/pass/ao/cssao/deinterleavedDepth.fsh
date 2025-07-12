layout(std140) uniform uCSSAOGlobals {

    uvec4 GlobalConstantBuffer_0;

    vec2 GlobalConstantBuffer_1;
    vec2 GlobalConstantBuffer_2;

    vec2 GlobalConstantBuffer_3;
    vec2 GlobalConstantBuffer_4;

    float GlobalConstantBuffer_5;
    float GlobalConstantBuffer_6;
    float GlobalConstantBuffer_7;
    float GlobalConstantBuffer_8;

    float GlobalConstantBuffer_9;
    float GlobalConstantBuffer_10;
    float GlobalConstantBuffer_11;
    int GlobalConstantBuffer_12;

    float GlobalConstantBuffer_13;
    float GlobalConstantBuffer_14;
    float GlobalConstantBuffer_15;
    float GlobalConstantBuffer_16;

    float GlobalConstantBuffer_17;
    float GlobalConstantBuffer_18;
    float GlobalConstantBuffer_19;
    float GlobalConstantBuffer_20;

    vec2 GlobalConstantBuffer_21;
    float GlobalConstantBuffer_22;
    float GlobalConstantBuffer_23;

    float GlobalConstantBuffer_24;
    float GlobalConstantBuffer_25;
    int GlobalConstantBuffer_26;
};

layout(std140) uniform uCSSAOPerPass {
    vec4 f4Jitter;
    vec2 f2Offset;
    float fSliceIndex;
    uint uSliceIndex;
} PerPassConstantBuffer_0;

uniform sampler2DRect uInputLinDepth;

out float fOut0;
out float fOut1;
out float fOut2;
out float fOut3;
out float fOut4;
out float fOut5;
out float fOut6;
out float fOut7;

void main()
{
    vec2 uv = (floor(gl_FragCoord.xy) * 4.0 + PerPassConstantBuffer_0.f2Offset);

    fOut0 = texture(uInputLinDepth, uv).x;
    fOut1 = textureOffset(uInputLinDepth, uv, ivec2(1, 0)).x;
    fOut2 = textureOffset(uInputLinDepth, uv, ivec2(2, 0)).x;
    fOut3 = textureOffset(uInputLinDepth, uv, ivec2(3, 0)).x;
    fOut4 = textureOffset(uInputLinDepth, uv, ivec2(0, 1)).z;
    fOut5 = textureOffset(uInputLinDepth, uv, ivec2(1, 1)).x;
    fOut6 = textureOffset(uInputLinDepth, uv, ivec2(2, 1)).x;
    fOut7 = textureOffset(uInputLinDepth, uv, ivec2(3, 1)).x;
}
