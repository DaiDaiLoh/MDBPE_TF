/// Precalculated LUT for GGX lighting
uniform sampler2D uGlowMaterialEnvLutGGX;
/// Environment Map for specular GGX IBL
uniform samplerCube uGlowMaterialEnvMapGGX;

// F0 helper
vec3 getF0(vec3 albedo, float metallic)
{
    const vec3 dielectricF0 = vec3(0.04); // Constant for dielectric (non-metal) surfaces
    return mix(dielectricF0, albedo, metallic);
}

// Fresnel term
// Schlicks Approximation
vec3 F_Schlick(vec3 f0, float f90, float u)
{
    return f0 + (f90 - f0) * clamp(pow(1.0 - u, 5.0), 0, 1);
}

vec3 F_SchlickRoughness(float cosTheta, vec3 f0, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - cosTheta, 5.0);
}

// DO NOT MULTIPLY BY COS THETA
vec3 shadingSpecularGGX(vec3 N, vec3 V, vec3 L, float roughness, vec3 F0)
{
    // see http://www.filmicworlds.com/2014/04/21/optimizing-ggx-shaders-with-dotlh/
    vec3 H = normalize(V + L);

    float dotLH = max(dot(L, H), 0.0);
    float dotNH = max(dot(N, H), 0.0);
    float dotNL = max(dot(N, L), 0.0);
    float dotNV = max(dot(N, V), 0.0);

    float alpha = roughness * roughness;

    // D (GGX normal distribution)
    float alphaSqr = alpha * alpha;
    float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
    float D = alphaSqr / (denom * denom);
    // no pi because BRDF -> lighting

    // F (Fresnel term)
    vec3 F = F_SchlickRoughness(dotLH, F0, roughness);

    // G (remapped hotness, see Unreal Shading)
    float k = (alpha + 2 * roughness + 1) / 8.0;
    float G = dotNL / (mix(dotNL, 1, k) * mix(dotNV, 1, k));
    // '* dotNV' - canceled by normalization

    // orginal G:
    /*
    {
        float k = alpha / 2.0;
        float k2 = k * k;
        float invK2 = 1.0 - k2;
        float vis = 1 / (dotLH * dotLH * invK2 + k2);

        vec3 FV = mix(vec3(F_b), vec3(F_a), F0) * vis;
        vec3 specular = D * FV / 4.0f;
        return specular * dotNL;
    }
    */

    // '/ dotLN' - canceled by lambert
    // '/ dotNV' - canceled by G
    return D * F * G / 4.0;
}

// specular and diffuse contribution of a single light direction
// DOES NOT INCLUDE IBL
vec3 shadingGGX(vec3 N, vec3 V, vec3 L, vec3 albedo, float roughness, float metallic)
{
    vec3 diffuse = albedo * (1 - metallic); // metals have no diffuse
    vec3 F0 = getF0(albedo, metallic);

    float dotNL = max(dot(N, L), 0.0);

    return diffuse * dotNL + shadingSpecularGGX(N, V, L, max(0.01, roughness), F0);
}

// image based specular lighting for GGX
vec3 iblSpecularGGX(vec3 N, vec3 V, vec3 albedo, float roughness, float metallic)
{
    float dotNV = max(dot(N, V), 0.0);
    vec3 R = reflect(-V, N);

    vec3 F0 = getF0(albedo, metallic);

    // Derive mipmap levels of environment map
    // TODO: Uniform / Precalculate
    // In OpenGL 4.6, use textureQueryLevels()
    const float maxLevel = floor(log2(float(textureSize(uGlowMaterialEnvMapGGX, 0).x))); 

    vec2 brdf = texture2D(uGlowMaterialEnvLutGGX, vec2(roughness, dotNV)).xy;
    vec3 envcolor = textureLod(uGlowMaterialEnvMapGGX, R, roughness * maxLevel).rgb;

    return envcolor * (F0 * brdf.x + brdf.y);
}

// image based lighting (specular + diffuse) for GGX
vec3 iblightingGGX(vec3 N, vec3 V, vec3 albedo, float roughness, float metallic)
{
    float dotNV = max(dot(N, V), 0.0);
    vec3 F0 = getF0(albedo, metallic);

    vec3 F = F_SchlickRoughness(dotNV, F0, roughness);
    vec3 R = reflect(-V, N);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(uGlowMaterialEnvMapGGX, N).rgb;
    vec3 diffuse = irradiance * albedo;
    vec3 specular = iblSpecularGGX(N, V, albedo, roughness, metallic);

    return kD * diffuse + specular;
}
