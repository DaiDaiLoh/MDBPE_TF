#ifndef GLOW_PIPELINE_CSZ_RECONSTRUCT_GLSL
#define GLOW_PIPELINE_CSZ_RECONSTRUCT_GLSL

#include <glow-pipeline/internal/common/globals.hh>

#if GLOW_PIPELINE_ENABLE_REVERSE_Z == 1
#glow pipeline internal_projectionInfoUbo
#else
uniform vec3 uClipInfo;
#endif

#if GLOW_PIPELINE_ENABLE_REVERSE_Z == 1
float getViewSpaceZ()
{
    return -(uPipelineProjectionInfo.viewportNearFar.z / gl_FragCoord.z);
}
#endif

float reconstructCSZ(float d) {
#if GLOW_PIPELINE_ENABLE_REVERSE_Z == 1
    return uPipelineProjectionInfo.viewportNearFar.z / d;
#else
    return uClipInfo.x / (uClipInfo.y * d + uClipInfo.z);
#endif
}

#endif
