#include <glow-pipeline/internal/common/globals.hh>

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

uniform sampler2DRect uInputDepth;
#if GLOW_PIPELINE_ENABLE_REVERSE_Z == 1
uniform float uNearPlane;
#endif

out float fOut;

void main()
{
    float depth = texture(uInputDepth, ivec2(gl_FragCoord.xy)).x;
#if GLOW_PIPELINE_ENABLE_REVERSE_Z == 1
    // clamp to F32 max to avoid NaN at the (infinite) far plane
    fOut = clamp(uNearPlane / depth, 0.f, GLOW_GLSL_FLT_MAX); 
#else
    depth = GlobalConstantBuffer_19 * depth + GlobalConstantBuffer_20;
    depth = clamp(depth, 0.0, 1.0);
    depth = depth * GlobalConstantBuffer_17 + GlobalConstantBuffer_18;
    fOut = 1.0 / depth;
#endif
}
