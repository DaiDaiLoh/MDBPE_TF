// Directional inscattering height fog, reference: UE4 implementation
// See:
    // https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/Private/HeightFogCommon.ush
    // https://github.com/EpicGames/UnrealEngine/blob/release/Engine/Shaders/Private/AtmosphericFogShader.usf
    // https://forums.unrealengine.com/development-discussion/rendering/108197-implementing-exponential-height-fog-in-a-custom-shader

vec3 getAtmosphericScattering(in float worldHeight, float fragDepth, in vec3 camPos, in vec3 V, in vec3 lightDirection, in float distanceToCamera, in vec3 lightColor, in float sunIntensity)
{
    // Distance in world units from the camera beyond which the sun starts bleeding into geometry
    const float inscatteringStartDistance = 100; 
    // Controls the sharpness of the sun
    const float inscatteringExponent = 100;
    // Controls the size of the sun
    const float inscatteringMultiplier = 1.02;

    float exponentialFog = 1.0 - exp(-distanceToCamera * gPipelineScene.fogDensity);

    // Correction multiplier to ensure inscattering is behind near objects
    // At the far plane, always show inscattering
    float fragDepthWorldUnits = fragDepth * uPipelineProjectionInfo.viewportNearFar.z;
    float distanceCorrection = (fragDepth == GLOW_PIPELINE_HORIZON_DEPTH ? 1.0 : clamp((fragDepthWorldUnits - inscatteringStartDistance) / uPipelineProjectionInfo.viewportNearFar.w, 0, 1));
    
    float inscatteringAmount = max(dot(-V, (normalize(lightDirection) * inscatteringMultiplier)), 0.0) * distanceCorrection;

    vec3 outputColor = mix(gPipelineScene.atmoScatterColor, lightColor, pow(inscatteringAmount, inscatteringExponent) * sunIntensity);
    float falloffCorrection = clamp(((((worldHeight - gPipelineScene.atmoScatterFalloffStart) / ( gPipelineScene.atmoScatterFalloffEnd - gPipelineScene.atmoScatterFalloffStart)) * (0 - 1)) + 1), 0, 1);

    return outputColor * exponentialFog * pow(falloffCorrection, gPipelineScene.atmoScatterFalloffExponent);
}
