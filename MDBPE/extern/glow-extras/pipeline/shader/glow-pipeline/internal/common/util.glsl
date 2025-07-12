#ifndef UTIL_GLSL
#define UTIL_GLSL

float distance2(vec3 a, vec3 b)
{
    vec3 d = a - b;
    return dot(d, d);
}

float rgbToLuminance(vec3 rgb)
{
    // Alternative luma weight: vec3(0.299, 0.587, 0.114) (FXAA 3.11 version)
    return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
}

// ---- DirectX ----

// -- Missing functions --

// DirectX: min
vec3 min3(vec3 a, vec3 b)
{
    return vec3(min(a.r, b.r), min(a.g, b.g), min(a.b, b.b));
}

// DirectX: max
vec3 max3(vec3 a, vec3 b)
{
    return vec3(max(a.r, b.r), max(a.g, b.g), max(a.b, b.b));
}

// DirectX: any
bool anyFloat(vec2 v)
{
    return (v.r != 0 || v.g != 0);
}

bool anyFloat(vec3 v)
{
    return (v.r != 0 || v.g != 0 || v.b != 0);
}


// -- Name mapping / Convenience --

float saturate(float v) { return clamp(v, 0, 1); }
vec2 saturate(vec2 v) { return clamp(v, 0, 1); }
vec3 saturate(vec3 v) { return clamp(v, 0, 1); }
vec4 saturate(vec4 v) { return clamp(v, 0, 1); }

vec4 ddy(vec4 v) { return dFdy(v); }
vec3 ddy(vec3 v) { return dFdy(v); }
vec2 ddy(vec2 v) { return dFdy(v); }

vec4 ddx(vec4 v) { return dFdx(v); }
vec3 ddx(vec3 v) { return dFdx(v); }
vec2 ddx(vec2 v) { return dFdx(v); }

#endif
