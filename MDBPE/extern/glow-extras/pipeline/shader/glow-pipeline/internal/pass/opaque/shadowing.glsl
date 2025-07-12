#include <glow-pipeline/internal/common/globals.hh>
#include <glow-pipeline/internal/common/cszReconstruct.glsl>

layout(std140) uniform uShadowLightVPUBO {
    mat4 lightVPs[GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT];
};

layout(std140) uniform uShadowCascadeLimitUBO {
    vec4 splitDepths;
    // float array
};

uniform sampler2DArrayShadow uShadowMaps;

const uvec2 global_shadowMapDimensions = uvec2(GLOW_PIPELINE_SHADOW_MAP_SIZE, GLOW_PIPELINE_SHADOW_MAP_SIZE);

// Either 3, 5 or 7
#define FilterSize_ 5

// Set to 1 to enable filtering across cascades
#define FilterAcrossCascades_ 1

vec2 computeReceiverPlaneDepthBias(vec3 texCoordDX, vec3 texCoordDY)
{
    vec2 biasUV;
    biasUV.x = texCoordDY.y * texCoordDX.z - texCoordDX.y * texCoordDY.z;
    biasUV.y = texCoordDX.x * texCoordDY.z - texCoordDY.x * texCoordDX.z;
    biasUV *= 1.0 / ((texCoordDX.x * texCoordDY.y) - (texCoordDX.y * texCoordDY.x));
    return biasUV;
}

float sampleShadowMap(in vec2 base_uv, in float u, in float v, in vec2 shadowMapSizeInv,
                      in uint cascadeIdx,  in float depth, in vec2 receiverPlaneDepthBias) {
    const vec2 uv = base_uv + vec2(u, v) * shadowMapSizeInv;
    const float z = depth + dot(vec2(u, v) * shadowMapSizeInv, receiverPlaneDepthBias);
    return texture(uShadowMaps, vec4(uv, cascadeIdx, z));
}

// Optimized Percentage Closer Filtering
// [The Witness] algorithm
// Ported and adjusted from https://mynameismjp.wordpress.com/2013/09/10/shadow-maps/ 
float sampleShadowCascade(vec3 shadowPos, vec3 shadowPosDX, vec3 shadowPosDY, uint cascadeIdx)
{
    float lightDepth = shadowPos.z;

    // -- Receiver plane depth bias --
    const vec2 shadowMapSizeInv = vec2(1.0) / global_shadowMapDimensions;
    const vec2 receiverPlaneDepthBias = computeReceiverPlaneDepthBias(shadowPosDX, shadowPosDY);

    // Static depth biasing to make up for incorrect fractional sampling on the shadow map grid
    // NOTE: This causes rather obvious peter panning
    const float fractionalSamplingError = 2 * dot(vec2(1.0, 1.0) * shadowMapSizeInv, abs(receiverPlaneDepthBias));
    lightDepth -= min(fractionalSamplingError, 0.01f);

    const vec2 uv = shadowPos.xy * global_shadowMapDimensions; // 1 unit - 1 texel

    vec2 base_uv;
    base_uv.x = floor(uv.x + 0.5);
    base_uv.y = floor(uv.y + 0.5);

    const float s = (uv.x + 0.5 - base_uv.x);
    const float t = (uv.y + 0.5 - base_uv.y);

    base_uv -= vec2(0.5, 0.5);
    base_uv *= shadowMapSizeInv; // [0, 1]

    float sum = 0;

    #if  FilterSize_ == 3

        float uw0 = (3 - 2 * s);
        float uw1 = (1 + 2 * s);

        float u0 = (2 - s) / uw0 - 1;
        float u1 = s / uw1 + 1;

        float vw0 = (3 - 2 * t);
        float vw1 = (1 + 2 * t);

        float v0 = (2 - t) / vw0 - 1;
        float v1 = t / vw1 + 1;

        sum += uw0 * vw0 * sampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * sampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw0 * vw1 * sampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * sampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0 / 16;

    #elif FilterSize_ == 5

        float uw0 = (4 - 3 * s);
        float uw1 = 7;
        float uw2 = (1 + 3 * s);

        float u0 = (3 - 2 * s) / uw0 - 2;
        float u1 = (3 + s) / uw1;
        float u2 = s / uw2 + 2;

        float vw0 = (4 - 3 * t);
        float vw1 = 7;
        float vw2 = (1 + 3 * t);

        float v0 = (3 - 2 * t) / vw0 - 2;
        float v1 = (3 + t) / vw1;
        float v2 = t / vw2 + 2;

        sum += uw0 * vw0 * sampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * sampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw0 * sampleShadowMap(base_uv, u2, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw1 * sampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * sampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw1 * sampleShadowMap(base_uv, u2, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw2 * sampleShadowMap(base_uv, u0, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw2 * sampleShadowMap(base_uv, u1, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw2 * sampleShadowMap(base_uv, u2, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0 / 144.0;

    #else // FilterSize_ == 7

        float uw0 = (5 * s - 6);
        float uw1 = (11 * s - 28);
        float uw2 = -(11 * s + 17);
        float uw3 = -(5 * s + 1);

        float u0 = (4 * s - 5) / uw0 - 3;
        float u1 = (4 * s - 16) / uw1 - 1;
        float u2 = -(7 * s + 5) / uw2 + 1;
        float u3 = -s / uw3 + 3;

        float vw0 = (5 * t - 6);
        float vw1 = (11 * t - 28);
        float vw2 = -(11 * t + 17);
        float vw3 = -(5 * t + 1);

        float v0 = (4 * t - 5) / vw0 - 3;
        float v1 = (4 * t - 16) / vw1 - 1;
        float v2 = -(7 * t + 5) / vw2 + 1;
        float v3 = -t / vw3 + 3;

        sum += uw0 * vw0 * sampleShadowMap(base_uv, u0, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw0 * sampleShadowMap(base_uv, u1, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw0 * sampleShadowMap(base_uv, u2, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw0 * sampleShadowMap(base_uv, u3, v0, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw1 * sampleShadowMap(base_uv, u0, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw1 * sampleShadowMap(base_uv, u1, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw1 * sampleShadowMap(base_uv, u2, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw1 * sampleShadowMap(base_uv, u3, v1, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw2 * sampleShadowMap(base_uv, u0, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw2 * sampleShadowMap(base_uv, u1, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw2 * sampleShadowMap(base_uv, u2, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw2 * sampleShadowMap(base_uv, u3, v2, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        sum += uw0 * vw3 * sampleShadowMap(base_uv, u0, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw1 * vw3 * sampleShadowMap(base_uv, u1, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw2 * vw3 * sampleShadowMap(base_uv, u2, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);
        sum += uw3 * vw3 * sampleShadowMap(base_uv, u3, v3, shadowMapSizeInv, cascadeIdx, lightDepth, receiverPlaneDepthBias);

        return sum * 1.0 / 2704.0;

    #endif
}

uint getCascade(float viewSpaceZ)
{
    uint cascadeIndex = 0;
    for (uint i = 0; i < GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT - 1; ++i) {
		if (viewSpaceZ < splitDepths[i])
			cascadeIndex = i + 1;
	}
    return cascadeIndex;
}

#if GLOW_PIPELINE_ENABLE_REVERSE_Z
uint getCascade() { return getCascade(getViewSpaceZ()); }
#endif

vec3 getShadowPosition(uint cascade, vec3 worldSpacePosition)
{
    vec4 lightSpacePosition = lightVPs[cascade] * vec4(worldSpacePosition, 1.0);
    vec3 fragPosProjected = lightSpacePosition.xyz / lightSpacePosition.w; // NDC
    return fragPosProjected * 0.5 + 0.5; // Within [0, 1]
}

#if GLOW_PIPELINE_ENABLE_REVERSE_Z
float getShadowVisibility(vec3 worldSpacePosition)
{
    float viewSpaceZ = getViewSpaceZ();
#else
float getShadowVisibility(vec3 worldSpacePosition, float viewSpaceZ)
{
#endif

    uint cascade = getCascade(viewSpaceZ);
    vec3 shadowPos = getShadowPosition(cascade, worldSpacePosition);
    vec3 shadowPosDX = dFdxFine(shadowPos);
    vec3 shadowPosDY = dFdyFine(shadowPos);

    float shadowVisibility = sampleShadowCascade(shadowPos, shadowPosDX, shadowPosDY, cascade);

    #if FilterAcrossCascades_
         // Filter across cascades: Sample the next cascade, and blend between the two results to smooth the transition
        if (cascade != GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT - 1)
        {
            const float blendThreshold = 0.9;
            float splitRatio = viewSpaceZ / splitDepths[cascade];

            if (splitRatio >= blendThreshold)
            {
                vec3 nextShadowPos = getShadowPosition(cascade + 1, worldSpacePosition);
                float nextVisibility = sampleShadowCascade(nextShadowPos, shadowPosDX, shadowPosDY, cascade + 1);

                float alpha = smoothstep(blendThreshold, 1.0, splitRatio);
                shadowVisibility = mix(shadowVisibility, nextVisibility, alpha);
            }
        }
    #endif

    return shadowVisibility;
}
