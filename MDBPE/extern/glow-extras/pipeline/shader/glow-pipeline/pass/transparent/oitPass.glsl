#glow pipeline internal_projectionInfoUbo

uniform sampler2DRect uDepth;
uniform vec3 uSunDirection;
uniform vec3 uSunColor;

float global_fragmentDepth = texelFetch(uDepth, ivec2(gl_FragCoord.xy)).x;

float getOitMaterialAlpha(vec3 N, vec3 V, vec3 specular) {
	float dotNV = max(dot(N, V), 0.0);
    float F_a = 1.0;
    float F_b = pow(1.0 - dotNV, 5);
    vec3 F = mix(vec3(F_b), vec3(F_a), specular);
    return max(F.x, max(F.y, F.z));
}

float getOitWeight(float fragZ, float alpha)
{
    float d = 1 - fragZ;
    return alpha * max(1e-2, 3e3 * d * d * d);
}

float transparentSpecularGGX(vec3 N, vec3 V, vec3 L, float roughness, vec3 F0)
{
    vec3 H = normalize(V + L);

    float LdotH = max(dot(L, H), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float alphaG = roughness * roughness;

    // D
    float alphaG2 = alphaG * alphaG;
    float denom = NdotH * NdotH * (alphaG2 - 1.0) + 1.0;
    float D = alphaG2 / (denom * denom);

    // G
    float k = (alphaG + 2 * roughness + 1) / 8.0;
    float G = NdotL / (mix(NdotL, 1, k) * mix(NdotV, 1, k));
    
    return D * G / 4.0;
}

struct OITMaterial
{
    vec3 albedo;
    float roughness;
    float alpha;
    vec3 specular;
    float reflectivity;
};

struct OITFragmentInfo
{
    vec3 N;
    vec3 V;
    vec3 R;
    vec3 posWorldSpace;
    vec4 posViewSpace;
    vec4 posScreenSpace;
    mat4 proj;
    mat4 view;
};

struct OITResult
{
    vec3 color;
    vec2 offset;
};

void getOitResult(
    in OITMaterial material, in OITFragmentInfo fragInfo, out OITResult result
    ) {
	// Thickness calculation
    vec2 projectedScreenXY = fragInfo.posScreenSpace.xy / fragInfo.posScreenSpace.w;
    vec4 opaquePos4 = uPipelineProjectionInfo.projectionInverse * vec4(projectedScreenXY, global_fragmentDepth * 2 - 1, 1.0);
    float opaqueDis = opaquePos4.z / opaquePos4.w;
    // Thickness: Distance between opaque depth beneath, and depth of current fragment
    float thickness = abs(fragInfo.posViewSpace.z - opaqueDis);
    //thickness = .5;

    // OIT Offset (Refraction)
    vec2 tsize = vec2(uPipelineProjectionInfo.viewportNearFar.xy);
    vec3 RF = refract(-fragInfo.V, fragInfo.N, 1 / 1.33);
    vec3 groundPosR = fragInfo.posWorldSpace + RF * clamp(thickness / 2, 0.0, 1.0);
    vec4 groundPosS = fragInfo.proj * fragInfo.view * vec4(groundPosR, 1.0);
    vec2 refrXY = groundPosS.xy / groundPosS.w;
    vec2 refrXYS = (refrXY * 0.5 + 0.5) * tsize;
    result.offset = refrXYS - gl_FragCoord.xy;

    // Depth modulated color
    const float depthFalloff = 5.0;
    vec3 depthAdjustedColor = material.albedo * (1 - pow(0.5, thickness / depthFalloff));

    // Reflection component
    vec3 refl = transparentSpecularGGX(fragInfo.N, fragInfo.V, uSunDirection, material.roughness, material.specular) * uSunColor;
    //refl += getPrefilterColor(R, roughness) * material.reflectivity;// * mix(1.0, shadowF, dot(R, L) * 0.5 + 0.5);

    // OIT Color
    result.color = refl + (depthAdjustedColor * material.alpha);
}

void outputOitGeometry(float fragZ, vec3 color, float alpha, vec2 offset)
{
    float weight = getOitWeight(fragZ, alpha);
    fAccuA = vec4(color * weight, alpha);
    fAccuB = alpha * weight;
    fDistortion = vec3(offset, 0.f);
}

void outputOitGeometry(float fragZ, vec3 color, float alpha)
{
    float weight = getOitWeight(fragZ, alpha);
    fAccuA = vec4(color * weight, alpha);
    fAccuB = alpha * weight;
    fDistortion = vec3(0);
}
