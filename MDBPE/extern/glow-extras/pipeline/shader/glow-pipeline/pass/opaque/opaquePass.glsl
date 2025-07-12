#ifndef _OPAQUE_PASS_GLSL
#define _OPAQUE_PASS_GLSL

#include <glow-pipeline/internal/pass/depthpre/clustering.glsl>
#include <glow-pipeline/internal/pass/depthpre/clusteringStructs.glsl>
#include <glow-pipeline/internal/pass/opaque/shadowing.glsl>

uniform sampler2DRect uSceneAo;

layout(std430) restrict readonly buffer sClusterLightIndexList {
    uint lightIndexList[];
} ssboLightIndexList;

layout(std430) restrict readonly buffer sClusterVisibilities {
    ClusterVisibility data[];
} ssboClusterVisibilities;

layout(std430) restrict readonly buffer sLightData {
    PackedLightData lights[];
} ssboLightData;

// -- Clustered Shading --

ClusterVisibility getClusterVisibilty()
{
    return ssboClusterVisibilities.data[getCurrentClusterIndex()];
}

uint getLightIndex(in uint offset)
{
    return ssboLightIndexList.lightIndexList[offset];
}

#define FOREACH_LIGHT(global_macro_lightIndex)                                          \
    ClusterVisibility _internal_clusterData = getClusterVisibilty();                    \
    uint global_macro_lightIndex = getLightIndex(_internal_clusterData.offset);         \
    for (uint _internal_i = 0u; _internal_i < _internal_clusterData.count; ++_internal_i, global_macro_lightIndex = getLightIndex(_internal_clusterData.offset + _internal_i))

// -- Lighting --

LightData unpackLightData(PackedLightData data) {
    LightData res;
    res.posA = data.aSize.xyz;
    res.posB = data.bRadius.xyz;
    res.size = data.aSize.w;
    res.radius = data.bRadius.w;
    res.color = data.colorMaskId.xyz;
    res.maskId = int(data.colorMaskId.w);
    return res;
}

LightData getLight(uint index)
{
    // For some reason, this line does not work on AMD:
    // return unpackLightData(ssboLightData.lights[index]);

    const PackedLightData data = ssboLightData.lights[index];
    LightData res;
    res.posA = data.aSize.xyz;
    res.posB = data.bRadius.xyz;
    res.size = data.aSize.w;
    res.radius = data.bRadius.w;
    res.color = data.colorMaskId.xyz;
    res.maskId = int(data.colorMaskId.w);
    return res;
}

// -- Lighting helpers --

float getSceneAo() {
    return clamp(texture(uSceneAo, gl_FragCoord.xy).r, 0, 1);
}

vec3 applyAo(in vec3 color, in float materialAo) {
    // Combined material + scene AO
    float combinedAo = (1 - ((1 - materialAo) + (1 - getSceneAo())));
    return color * combinedAo;
}

vec3 applyAo(in vec3 color) {
    return color * getSceneAo();
}

vec3 getTubeLightL(in vec3 R, in vec3 posA, in vec3 posB, in vec3 worldPos)
{
    const vec3 L0 = posA - worldPos;
    const vec3 L1 = posB - worldPos;
    const vec3 Ld = L1 - L0;

    const float tnum = dot(R, L0) * dot(R, Ld) - dot(L0, Ld);
    const float tden = pow(length(Ld), 2) - pow(dot(R, Ld), 2);
    const float t = tnum / tden;
    return L0 + clamp(t, 0.0, 1.0) * Ld;
}

vec3 getSphereLightL(in vec3 R, in vec3 L, in float sphereLightSize)
{
    const vec3 centerToRay = dot(L, R) * R - L; 
    return L + centerToRay * clamp(sphereLightSize / length(centerToRay), 0.0, 1.0);
}

void getTubeLightInfo(
    in LightData data, in vec3 worldPos, in vec3 V, in vec3 N, in float roughness,
    out vec3 correctedL, out vec3 radiance
    )
{
    vec3 lightDistanceVector = data.posA - worldPos;
    float areaNormalization = 1;

    if (data.size == 0)
    {
        // Degenerate case, Point Light 
        // (Tube Lights require a size > 0)
    }
    else
    {
        const vec3 R = reflect(-V, N);

        if (data.posA != data.posB)
        {
            // Tube Light
            lightDistanceVector = getTubeLightL(R, data.posA, data.posB, worldPos);
        }
        else
        {
            // Sphere Light
        }

        lightDistanceVector = getSphereLightL(R, lightDistanceVector, data.size);

        const float a = sqrt(max(roughness, 0.001));
        const float a2 = min(1.0, a + data.size / (2 * length(data.posA - worldPos)));
        areaNormalization = pow(a / a2, 2);
    }
    
    float lightDistance = sqrt(dot(lightDistanceVector, lightDistanceVector));

    // Basic Attenuation
    //float attenuation = smoothstep(data.radius, 0.0, lightDistance);

    // Inverse-Square Falloff
    float attenuation = pow(clamp(1 - pow(lightDistance / data.radius, 4), 0.0, 1.0), 2.0) / (pow(lightDistance, 2.0) + 1.0);

    radiance = data.color * attenuation * areaNormalization;
    correctedL = normalize(lightDistanceVector);
}

// -- Velocity and Output --

// Returns the screen space velocity vector for a given fragment,
// can be fed directly into outputOpaqueGeometry
// Expects unjittered homogenous device coordinates
// cleanHDC = cleanProjection * View * Model * aPosition;
// prevCleanHDC = previousCleanProjection * previousView * previousModel * aPosition;
vec2 getFragmentVelocity(in vec4 cleanHDC, in vec4 prevCleanHDC)
{
    // Both positions are unjittered
	// Range [0, 1] using the * 0.5 + 0.5 operation
	// corresponds to the UV coords of the TAA history buffer
	vec2 a = (cleanHDC.xy / cleanHDC.w) * 0.5 + 0.5;
	vec2 b = (prevCleanHDC.xy / prevCleanHDC.w) * 0.5 + 0.5;
	return a - b;
}

void outputOpaqueGeometry(vec3 hdrColor, vec2 velocity)
{
    fHdr = hdrColor;
    fVelocity = velocity;
}

void outputOpaqueGeometry(vec3 hdrColor)
{
    fHdr = hdrColor;
    fVelocity = vec2(0);
}

#endif
