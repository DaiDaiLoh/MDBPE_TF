layout(local_size_x = 1, local_size_y = 1) in;

#include <glow-pipeline/internal/common/globals.hh>
#include <glow-pipeline/internal/pass/depthpre/clusteringStructs.glsl>

layout (std430) restrict writeonly buffer sClusterAABBs {
    AABB clusterAABBs[];
};

uniform mat4 uProjInv;
uniform float uNearPlane;
uniform float uFarPlane;
uniform ivec2 uResolution;

vec4 clipToView(vec4 clip);
vec4 screenToView(vec4 screen);
vec3 lineIntersectionToZPlane(vec3 B, float zDistance);

void main() {
    uint tileIndex = gl_WorkGroupID.x +
                     gl_WorkGroupID.y * gl_NumWorkGroups.x +
                     gl_WorkGroupID.z * (gl_NumWorkGroups.x * gl_NumWorkGroups.y);

    // Min and max point in screen space
    vec4 maxPointSS = vec4(vec2(gl_WorkGroupID.x + 1, gl_WorkGroupID.y + 1) * GLOW_PIPELINE_CLUSTER_PIXEL_SIZE_X, -1.0, 1.0); // Top Right
    vec4 minPointSS = vec4(gl_WorkGroupID.xy * GLOW_PIPELINE_CLUSTER_PIXEL_SIZE_X, -1.0, 1.0); // Bottom left
    
    vec3 maxPointVS = screenToView(maxPointSS).xyz;
    vec3 minPointVS = screenToView(minPointSS).xyz;

    // Near and far values of the cluster in view space
    float tileNear  = -uNearPlane * pow(uFarPlane / uNearPlane, gl_WorkGroupID.z / float(gl_NumWorkGroups.z));
    float tileFar   = -uNearPlane * pow(uFarPlane / uNearPlane, (gl_WorkGroupID.z + 1) / float(gl_NumWorkGroups.z));

    // 4 intersection points to the cluster near/far plane
    vec3 minPointNear = lineIntersectionToZPlane(minPointVS, tileNear);
    vec3 minPointFar  = lineIntersectionToZPlane(minPointVS, tileFar);
    vec3 maxPointNear = lineIntersectionToZPlane(maxPointVS, tileNear);
    vec3 maxPointFar  = lineIntersectionToZPlane(maxPointVS, tileFar);

    vec3 minPointAABB = min(min(minPointNear, minPointFar),min(maxPointNear, maxPointFar));
    vec3 maxPointAABB = max(max(minPointNear, minPointFar),max(maxPointNear, maxPointFar));

    clusterAABBs[tileIndex].min = vec4(minPointAABB, 0);
    clusterAABBs[tileIndex].max = vec4(maxPointAABB, 0);
}

// Intersection point of a line from the camera to a point in screen space
// on a z oriented plane at the given distance
vec3 lineIntersectionToZPlane(vec3 B, float zDistance) {
    const vec3 normal = vec3(0, 0, 1);

    // Intersection length for the line and the plane
    float t = (zDistance - dot(normal, vec3(0))) / dot(normal, B);
    // Position of the point along the line
    return t * B;
}

vec4 clipToView(vec4 clip) {
    vec4 view = uProjInv * clip;
    return view / view.w;
}

vec4 screenToView(vec4 screen) {
    vec2 texCoord = screen.xy / uResolution;
    vec4 clip = vec4(vec2(texCoord.x, texCoord.y)* 2.0 - 1.0, screen.z, screen.w);
    return clipToView(clip);
}
