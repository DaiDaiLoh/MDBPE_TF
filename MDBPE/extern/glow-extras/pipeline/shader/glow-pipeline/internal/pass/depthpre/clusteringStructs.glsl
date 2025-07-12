
// A packed Light, as it is uploaded to the GPU
struct PackedLightData {
    vec4 aSize;
    vec4 bRadius;
    vec4 colorMaskId;
};

// An unpacked, usable light
struct LightData {
    vec3 posA;
    vec3 posB;
    float radius;
    float size;
    vec3 color;
    int maskId;
};

// The offset and count of a cluster into the light index list
struct ClusterVisibility {
    uint offset;
    uint count;
};

// The AABB of a cluster in view space
struct AABB {
    vec4 min;
    vec4 max;
};
