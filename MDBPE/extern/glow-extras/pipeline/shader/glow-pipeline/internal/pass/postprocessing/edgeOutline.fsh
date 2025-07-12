#include <glow-pipeline/internal/common/globals.hh>
#include <glow-pipeline/internal/common/util.glsl>

uniform sampler2DRect uDepth;
uniform sampler2D uNormals;

uniform float uDepthThreshold;
uniform float uNormalThreshold;
uniform vec3 uColor;
uniform vec2 uInverseResolution;

// LDR output
out vec4 fHDR;

float getDepthCondition(float c)
{
    float c1 = texture(uDepth, gl_FragCoord.xy + vec2(0, 1)).x;
    float c2 = texture(uDepth, gl_FragCoord.xy + vec2(1, 0)).x;
        
    float d = 0.0;
    d = max(d, abs(c - c1));
    d = max(d, abs(c - c2));

    return float(d > uDepthThreshold);
}

float getNormalCondition()
{
    // Sample normal
    vec2 du = vec2(uInverseResolution.x, 0);
    vec2 dv = vec2(0, uInverseResolution.y);
    vec2 uv = (gl_FragCoord.xy) / (vec2(textureSize(uNormals, 0)));
    vec3 n = texture(uNormals, uv).xyz;
    vec3 n1 = texture(uNormals, uv + du).xyz;
    vec3 n2 = texture(uNormals, uv + dv).xyz;

    float d = 0.0;
    d = max(d, distance(n, n1));
    d = max(d, distance(n, n2));

    return float(d > uNormalThreshold);
}

void main()
{
    float mainDepth = texture(uDepth, gl_FragCoord.xy).x;
    float depthCond = getDepthCondition(mainDepth);
    float normalCond = mainDepth == GLOW_GLSL_FLT_MAX ? 0.0 : getNormalCondition();
    fHDR = vec4(uColor, mix(0.0, 0.9, max(depthCond, normalCond)));
}
