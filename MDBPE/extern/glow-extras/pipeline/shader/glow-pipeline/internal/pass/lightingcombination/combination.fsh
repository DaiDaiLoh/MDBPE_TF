#glow pipeline internal_projectionInfoUbo
#glow pipeline sceneInfo

#include <glow-pipeline/internal/common/util.glsl>
#include <glow-pipeline/internal/common/globals.hh>
#include <glow-pipeline/internal/common/positionReconstruct.glsl>
#include <glow-pipeline/internal/pass/lightingcombination/atmosphericScattering.glsl>

in vec4 vScreenPosition;

// IBuffer-sized
uniform sampler2DRect uShadedOpaque;
uniform sampler2DRect uDepth;

// T/GBuffer-sized
uniform sampler2DRect uAccumulationA;
uniform sampler2DRect uAccumulationB;
uniform sampler2DRect uDistortion;

uniform vec3 uCamPos;

out vec3 fHDR;

void main() 
{
    ivec2 coords = ivec2(gl_FragCoord.xy);

    // Read TBuffer
    vec4 tAccuA = texelFetch(uAccumulationA, coords);
    float tAccuB = texelFetch(uAccumulationB, coords).x;

    // Assemble weighted OIT
    float alpha = 1 - tAccuA.a;
    vec4 accu = vec4(tAccuA.xyz, tAccuB);
    vec3 transparentColor = accu.rgb / clamp(accu.a, 1e-4, 5e4);

    //  OIT Offset (Refraction)
    vec2 offset = texelFetch(uDistortion, coords).xy;
    vec3 opaqueColor = texture(uShadedOpaque, gl_FragCoord.xy + offset).rgb;

    vec3 hdrColor = mix(opaqueColor, transparentColor, alpha);

    // Color debug
    //hdrColor = vec3(texelFetch(uAccumulationA, coords).rgb / texelFetch(uAccumulationB, coords).x);
    
    // Distortion debug
    //hdrColor = vec3(fract(texelFetch(uDistortion, coords).xy / 30), 0);


    float fragDepth = texelFetch(uDepth, coords).x;
    vec3 worldPos = reconstructWorldPosition(vScreenPosition, fragDepth);

    vec3 cameraDistanceVector = uCamPos - worldPos;
    vec3 V = normalize(cameraDistanceVector);

    if (gPipelineScene.atmoScatterIntensity > 0.0)
    {
        // Normalize distance and height at the far plane
        // Otherwise fog would drastically change based on far plane settings
        const float simulatedFarPlane = 2000.0;

#if GLOW_PIPELINE_ENABLE_REVERSE_Z == 1
        float cameraDistance = fragDepth == 0.0 ? simulatedFarPlane : sqrt(dot(cameraDistanceVector, cameraDistanceVector));
        float worldHeight = fragDepth == 0.0 ? 0 : worldPos.y;
#else
        float cameraDistance = fragDepth == 1.0 ? simulatedFarPlane : sqrt(dot(cameraDistanceVector, cameraDistanceVector));
        float worldHeight = fragDepth == 1.0 ? worldPos.y * (simulatedFarPlane / uPipelineProjectionInfo.viewportNearFar.w) : worldPos.y;
#endif
        
        vec3 atmoScatter = gPipelineScene.atmoScatterIntensity * getAtmosphericScattering(worldHeight, fragDepth, uCamPos, V, gPipelineScene.sunDirection, cameraDistance, gPipelineScene.sunColor, gPipelineScene.sunIntensity);
        hdrColor += atmoScatter;
    }

    // HDR output and Bloom extraction
    // TODO: This clamp should not be required, but sometimes colors become negative in the preceding passes
    fHDR = max(hdrColor, vec3(0));
}
