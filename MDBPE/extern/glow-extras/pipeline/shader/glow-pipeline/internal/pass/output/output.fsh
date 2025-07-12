//#include <glow-pipeline/internal/pass/output/common/fxaa.glsl>
//#include <glow-pipeline/internal/pass/output/common/sharpen.glsl>
#glow pipeline sceneInfo

uniform sampler2DRect uInput;
uniform sampler3D uColorLut;

uniform ivec2 uViewportOffset;

out vec3 fColor;

// Size of the color LUT in each dimensions
// This value has to coincide with OutputStage::colorLutDimensions
#define COLOR_LUT_DIMS 16
const float colorLutScale = (COLOR_LUT_DIMS - 1.0) / COLOR_LUT_DIMS;
const vec3 colorLutOffset = vec3(1.0 / (2.0 * COLOR_LUT_DIMS));

void main()
{
    //// Downscaling
    vec3 color = texture(uInput, gl_FragCoord.xy - uViewportOffset).rgb;

    // FXAA
    //vec3 color = fxaa(uInput, gl_FragCoord.xy).rgb;

    // Brightness and Contrast
    color = (color - 0.5) * gPipelineScene.contrast + 0.5 + (gPipelineScene.brightness - 1);

    // Color grading
    color = texture(uColorLut, colorLutScale * color + colorLutOffset).rgb;

    // Output
    fColor = color;
}
