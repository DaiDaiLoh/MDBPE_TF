#include <glow-pipeline/internal/common/globals.hh>

layout(local_size_x = GLOW_PIPELINE_LASSIGN_COMPUTE_X, local_size_y = GLOW_PIPELINE_LASSIGN_COMPUTE_Y, local_size_z = GLOW_PIPELINE_CLUSTER_AMOUNT_Z) in;

#include <glow-pipeline/internal/pass/depthpre/clusteringStructs.glsl>

// Cluster AABBs
layout(std430) restrict readonly buffer sClusterAABBs {
    AABB clusterAABBs[]; // Size: clusters.x * clusters.y * clusters.z
};

// All lights
layout(std430) restrict readonly buffer sLightData {
    PackedLightData lights[];
} ssboLightData;

// Global continuous lightIndex index list, indices into sLightData
layout(std430) restrict writeonly buffer sClusterLightIndexList {
    uint lightIndexList[];
} ssboLightIndexList;

// Cluster visibilities, indices into sClusterLightIndexList
layout(std430) restrict writeonly buffer sClusterVisibilities {
    ClusterVisibility data[];
} ssboClusterVisibilities;

// Single global index count, added atomically, to calculate offsets into the lightIndex index list
layout(std430) restrict buffer sGlobalIndexCount {
    uint globalIndexCount;
};

#define THREAD_COUNT (gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z)
#define LIGHTS_PER_BATCH (THREAD_COUNT)

shared vec4 sharedLights[LIGHTS_PER_BATCH];

uniform mat4 uView;

bool doesLightIntersectAabb(vec4 lightData, const AABB b){
    const vec3 center = vec3(uView * vec4(lightData.xyz, 1));
    const float radius = lightData.w;
    
    float sqDist = 0.0;

    // If this loop isn't unrolled manually, it causes a compiler / linker hang
    float v = center[0];
    if( v < b.min[0] ) sqDist += (b.min[0] - v) * (b.min[0] - v);
    if( v > b.max[0] ) sqDist += (v - b.max[0]) * (v - b.max[0]);
    v = center[1];
    if( v < b.min[1] ) sqDist += (b.min[1] - v) * (b.min[1] - v);
    if( v > b.max[1] ) sqDist += (v - b.max[1]) * (v - b.max[1]);
    v = center[2];
    if( v < b.min[2] ) sqDist += (b.min[2] - v) * (b.min[2] - v);
    if( v > b.max[2] ) sqDist += (v - b.max[2]) * (v - b.max[2]);

    return sqDist <= (radius * radius);
}

vec4 getBoundingSphere(PackedLightData light)
{
    // center (xyz) and bounding sphere radius (w)
    return vec4(
        (light.aSize.xyz + light.bRadius.xyz) * 0.5, 
        length(light.aSize.xyz - light.bRadius.xyz) * 0.5 + light.aSize.w + light.bRadius.w 
    );
}

void main() {
    // Reset the global index count
    globalIndexCount = 0;

    const uint lightCount = ssboLightData.lights.length();
    const uint batchAmount = (lightCount + LIGHTS_PER_BATCH - 1) / LIGHTS_PER_BATCH;

    uint clusterIndex = gl_GlobalInvocationID.x +
                     gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x +
                     gl_GlobalInvocationID.z * (gl_NumWorkGroups.x * gl_WorkGroupSize.x * gl_NumWorkGroups.y * gl_WorkGroupSize.y);
    const AABB clusterAabb = clusterAABBs[clusterIndex];

    uint visibleLightCount = 0;
    uint visibleLightIndices[GLOW_PIPELINE_MAX_LIGHTS_PER_CLUSTER];

    // Simple, brute-force variant
    // for (int i = 0; i < lightCount; ++i)
    // {
    //     if (doesLightIntersectAabb(getBoundingSphere(ssboLightData.lights[i]), clusterAabb))
    //     {
    //         visibleLightIndices[visibleLightCount] = i;
    //         ++visibleLightCount;
    //     } 
    // }

    // Load lights in batches into shared workgroup memory
    for (uint batch = 0; batch < batchAmount; ++batch) {
        uint lightIndex = batch * LIGHTS_PER_BATCH + gl_LocalInvocationIndex;
        // Prevent fetching a light out of bounds
        lightIndex = min(lightIndex, lightCount);

        // Load this thread's light into the shared light storage, compressed to the bounding sphere
        sharedLights[gl_LocalInvocationIndex] = getBoundingSphere(ssboLightData.lights[lightIndex]);

        barrier();

        // Test all lights of the batch against this thread's cluster AABB
        const uint maxBatchLightIndex = min(lightCount - batch * LIGHTS_PER_BATCH, LIGHTS_PER_BATCH);
        for (uint lightIndex = 0; lightIndex < maxBatchLightIndex; ++lightIndex) {
            if (doesLightIntersectAabb(sharedLights[lightIndex], clusterAabb)) {
                visibleLightIndices[visibleLightCount] = batch * LIGHTS_PER_BATCH + lightIndex;
                ++visibleLightCount;
            }
        }
    }

    // Wait until all threads are done
    barrier();

    uint offset = atomicAdd(globalIndexCount, visibleLightCount);

    for (uint i = 0; i < visibleLightCount; ++i) {
        ssboLightIndexList.lightIndexList[offset + i] = visibleLightIndices[i];
    }

    ssboClusterVisibilities.data[clusterIndex].offset = offset;
    ssboClusterVisibilities.data[clusterIndex].count = visibleLightCount;
}
