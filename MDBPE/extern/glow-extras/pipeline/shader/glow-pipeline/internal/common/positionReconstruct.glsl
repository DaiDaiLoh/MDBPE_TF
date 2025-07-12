#include <glow-pipeline/internal/common/globals.hh>

vec4 reconstructClipSpacePosition(vec4 screenPosition, float fragmentDepth)
{
#if GLOW_PIPELINE_ENABLE_REVERSE_Z == 1
    return vec4(screenPosition.xy / screenPosition.w, fragmentDepth, 1.0);
#else
    return vec4(screenPosition.xy / screenPosition.w, fragmentDepth * 2 - 1, 1.0);
#endif
}

vec3 reconstructViewPosition(vec4 screenPosition, float fragmentDepth)
{
    vec4 clipSpacePosition = reconstructClipSpacePosition(screenPosition, fragmentDepth);
    vec4 viewPosition = uPipelineProjectionInfo.projectionInverse * clipSpacePosition; 
    viewPosition /= viewPosition.w;
    return viewPosition.xyz;
}

vec3 reconstructWorldPosition(vec4 screenPosition, float fragmentDepth)
{
    vec4 clipSpacePosition = reconstructClipSpacePosition(screenPosition, fragmentDepth);
    vec4 viewPosition = uPipelineProjectionInfo.projectionInverse * clipSpacePosition; 
    viewPosition /= viewPosition.w;
    return vec3(uPipelineProjectionInfo.viewInverse * viewPosition);
}
