#glow pipeline sceneInfo

#include <glow-pipeline/internal/pass/postprocessing/common/tonemapping.glsl>
#include <glow-pipeline/internal/pass/postprocessing/common/srgb.glsl>
#include <glow-pipeline/internal/pass/postprocessing/common/dithering.glsl>
#include <glow-pipeline/internal/pass/output/common/sharpen.glsl>

in vec2 vPosition;

// HDR input
uniform sampler2DRect uInput;
uniform sampler2DRect uBloom;

// LDR output
out vec3 fLdr;

vec3 sampleWithChromaticAbberation(sampler2DRect tex, float intensity)
{
    vec2 fragmentToCenter = vPosition - vec2(0.5);
    float distance = dot(fragmentToCenter, fragmentToCenter) * intensity;
    vec2 dir = normalize(vPosition - vec2(0.5)) * distance;

    return vec3(
        texture(tex, gl_FragCoord.xy - dir * 1.0).r,
        texture(tex, gl_FragCoord.xy - dir * 0.8).g,
        texture(tex, gl_FragCoord.xy - dir * 0.7).b
    );
}

void main()
{
    vec3 bloomColor = max(texture(uBloom, gl_FragCoord.xy).rgb, 0.0); // This buffer is sometimes negative

    // Sample HDR color and sharpen
    vec3 hdrColor = sharpen(uInput, gl_FragCoord.xy, 1, gPipelineScene.sharpenStrength).rgb;

    // Add Bloom
    vec3 color = hdrColor + bloomColor;

    // Tonemapping
    if (gPipelineScene.tonemapEnabled > 0)
    {
        //color = vec3(1.0) - exp(-color * gPipelineScene.exposure);
        //color = optimizedHejlDawsonTonemap(color, gPipelineScene.exposure, gPipelineScene.gamma);
        color = ACESFittedTonemap(color, gPipelineScene.exposure, gPipelineScene.gamma);
        //color = lumaWeightTonemap(color);
    }

    // Gamma correction
    color = linearToSRGB(color, gPipelineScene.gamma);

    // Dithering and output
    fLdr = dither(color, gl_FragCoord.xy, 100); // Divisor is originally 255, but this looks much better (on an 8bit screen)
}
