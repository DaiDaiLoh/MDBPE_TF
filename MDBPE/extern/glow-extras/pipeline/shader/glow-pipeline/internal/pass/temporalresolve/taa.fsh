#include <glow-pipeline/internal/common/util.glsl>
#include <glow-pipeline/internal/pass/postprocessing/common/tonemapping.glsl>

#glow pipeline internal_projectionInfoUbo
#glow pipeline sceneInfo

out vec3 fHDR;
out vec3 fBloom;

uniform sampler2DRect uShadedOpaque;
uniform sampler2DRect uVelocity;
uniform sampler2DRect uTemporalHistory;

uniform bool uEnableHistoryRejection;
uniform vec2 uJitter;

// Unjitter the current color sample, can improve or degrade stability
#define UNJITTER_COLOR_SAMPLE 0

// 4-Tap Varying only samples 4 times (vs 9) but is less stable 
#define MINMAX_3X3_ROUNDED 1
#define MINMAX_4TAP_VARYING !MINMAX_3X3_ROUNDED

// Reduces flickering at the cost of (signifcantly) slower convergence
// (fast movements decrease sharpness)
#define HIGH_STABILITY_MODE 0

vec3 clipAabb(vec3 aabbMin, vec3 aabbMax, vec3 p, vec3 q)
{
    // note: only clips towards aabb center (but fast!)
    vec3 pClip = 0.5 * (aabbMax + aabbMin);
    vec3 eClip = 0.5 * (aabbMax - aabbMin) + 0.001;

    vec3 vClip = q - pClip;
    vec3 vUnit = vClip / eClip;
    vec3 aUnit = abs(vUnit);
    float maUnit = max(aUnit.x, max(aUnit.y, aUnit.z));

    if (maUnit > 1.0)
        return pClip + vClip / maUnit;
    else
        return q; // point inside aabb
}

vec3 performTemporalAA(vec2 uv, vec2 velocity)
{
#if MINMAX_3X3_ROUNDED

    // Sample 3x3 neighborhood of current frame
    vec2 du = vec2(1, 0);
    vec2 dv = vec2(0, 1);
    vec3 ctl = texture(uShadedOpaque, uv - dv - du).rgb;
    vec3 ctc = texture(uShadedOpaque, uv - dv).rgb;
    vec3 ctr = texture(uShadedOpaque, uv - dv + du).rgb;
    vec3 cml = texture(uShadedOpaque, uv - du).rgb;
    vec3 cmc = texture(uShadedOpaque, uv).rgb;
    vec3 cmr = texture(uShadedOpaque, uv + du).rgb;
    vec3 cbl = texture(uShadedOpaque, uv + dv - du).rgb;
    vec3 cbc = texture(uShadedOpaque, uv + dv).rgb;
    vec3 cbr = texture(uShadedOpaque, uv + dv + du).rgb;
        
    // Calculate component-wise minimum, maximum and average
    vec3 cmin = min3(ctl, min3(ctc, min3(ctr, min3(cml, min3(cmc, min3(cmr, min3(cbl, min3(cbc, cbr))))))));
    vec3 cmax = max3(ctl, max3(ctc, max3(ctr, max3(cml, max3(cmc, max3(cmr, max3(cbl, max3(cbc, cbr))))))));
    vec3 cavg = (ctl + ctc + ctr + cml + cmc + cmr + cbl + cbc + cbr) / 9.0;

    // Min/Max Rounding
    vec3 cmin5 = min3(ctc, min3(cml, min3(cmc, min3(cmr, cbc))));
    vec3 cmax5 = max3(ctc, max3(cml, max3(cmc, max3(cmr, cbc))));
    vec3 cavg5 = (ctc + cml + cmc + cmr + cbc) / 5.0;
    cmin = 0.5 * (cmin + cmin5);
    cmax = 0.5 * (cmax + cmax5);
    cavg = 0.5 * (cavg + cavg5);

#elif MINMAX_4TAP_VARYING

    const float _SubpixelThreshold = 0.5;
    const float _GatherBase = 0.5;
    const float _GatherSubpixelMotion = 0.1666;

    vec2 texel_vel = velocity / vec2(1);
    float texel_vel_mag = length(texel_vel) * 1 /* vs_dist */;
    float k_subpixel_motion = saturate(_SubpixelThreshold / (0.0001f + texel_vel_mag));
    float k_min_max_support = _GatherBase + _GatherSubpixelMotion * k_subpixel_motion;

    vec2 ss_offset01 = k_min_max_support * vec2(-1, 1);
    vec2 ss_offset11 = k_min_max_support * vec2(1, 1);
    vec3 c00 = texture(uShadedOpaque, uv - ss_offset11).rgb;
    vec3 c10 = texture(uShadedOpaque, uv - ss_offset01).rgb;
    vec3 c01 = texture(uShadedOpaque, uv + ss_offset01).rgb;
    vec3 c11 = texture(uShadedOpaque, uv + ss_offset11).rgb;

    vec3 cmin = min(c00, min(c10, min(c01, c11)));
    vec3 cmax = max(c00, max(c10, max(c01, c11)));

    vec3 cavg = (c00 + c10 + c01 + c11) / 4.0;

#endif

    // Sample current and history color
#if UNJITTER_COLOR_SAMPLE
    vec3 currentColor = texture(uShadedOpaque, uv - uJitter).rgb; 
#else
    #if MINMAX_3X3_ROUNDED
    vec3 currentColor = cmc; // Same as cmc, already sampled
    #else
    vec3 currentColor = texture(uShadedOpaque, uv).rgb;
    #endif
#endif
    vec3 historyColor = texture(uTemporalHistory, uv - velocity).rgb;

    // Clip to neighborhood color-space AABB
    historyColor = clipAabb(cmin, cmax, clamp(cavg, cmin, cmax), historyColor);

    // UE4 weighting (sucks)
    //float contrast = distance(cavg, currentColor);
    //float weight = 0.05 * contrast;
    //return mix(historyColor, currentColor, weight);

    // Feedback weight from unbiased luminance delta (t.lottes)
    float lum0 = rgbToLuminance(currentColor);
    float lum1 = rgbToLuminance(historyColor);
    float unbiasedDiff = abs(lum0 - lum1) / max(lum0, max(lum1, 0.2));
    float unbiasedWeight = 1.0 - unbiasedDiff;
    float unbiasedWeight2 = unbiasedWeight * unbiasedWeight;

#if HIGH_STABILITY_MODE
    float kFeedback = mix(0.97f, 0.999f, unbiasedWeight2);
#else
    float kFeedback = mix(0.88f, 0.97f, unbiasedWeight2);
#endif
    // History blending based on feedback weight
    return mix(currentColor, historyColor, kFeedback);
}

vec3 performTAANoRejection(vec2 uv, vec2 velocity)
{
    vec3 currentColor = texture(uShadedOpaque, uv).rgb;
    vec3 historyColor = texture(uTemporalHistory, uv - velocity).rgb;
    return mix(currentColor, historyColor, 0.98);
}

void main()
{
    // TAA
    vec2 velocity = texture(uVelocity, gl_FragCoord.xy).rg * uPipelineProjectionInfo.viewportNearFar.xy;
    vec3 hdrColor = uEnableHistoryRejection ? 
        performTemporalAA(gl_FragCoord.xy, velocity) : 
        performTAANoRejection(gl_FragCoord.xy, velocity);
    
    // HDR output and Bloom extraction
    fHDR = hdrColor;
    //fHDR.g *= length(velocity);

    float hdrColorLuminance = rgbToLuminance(hdrColor);
    vec3 normalizedColor = hdrColor / max(hdrColorLuminance, 0.1);

    fBloom = mix(
        vec3(0),
        normalizedColor * gPipelineScene.bloomIntensity,
        float(hdrColorLuminance > gPipelineScene.bloomThreshold)
    );

}
