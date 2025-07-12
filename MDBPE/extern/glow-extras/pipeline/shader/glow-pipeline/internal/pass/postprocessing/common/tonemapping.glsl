#include <glow-pipeline/internal/common/util.glsl>

// -- Basic Reinhard Tonemapping --

vec3 reinhardTonemap(vec3 rgb)
{
    return rgb / (rgb + vec3(1));
}

// -- Uncharted 2 Tonemapping --
// Reference: http://filmicworlds.com/blog/filmic-tonemapping-operators/

const float U2_A = 0.22;
	// from 0.00 to 1.00
	// Shoulder strength

const float U2_B = 0.30;
	// from 0.00 to 1.00
	// Linear strength

const float U2_C = 0.10;
	// from 0.00 to 1.00
	// Linear angle

const float U2_D = 0.20;
	// from 0.00 to 1.00
	// Toe strength

const float U2_E = 0.01;
	// from 0.00 to 1.00
	// Toe numerator

const float U2_F = 0.22;
	// from 0.00 to 1.00
        // Toe denominator

const float U2_W = 11.2;
	// from 0.00 to 20.00
	// Linear White Point Value

vec3 U2Tonemap(vec3 x)
{
   return ((x*(U2_A*x+U2_C*U2_B)+U2_D*U2_E)/(x*(U2_A*x+U2_B)+U2_D*U2_F))-U2_E/U2_F;
}

vec3 uncharted2Tonemap(vec3 inputColor, float exposure, float gamma)
{
   inputColor *= exposure;

   float ExposureBias = 2.0f;
   vec3 curr = U2Tonemap(ExposureBias * inputColor);

   vec3 whiteScale = 1.0 / U2Tonemap(vec3(U2_W));      
   return curr * whiteScale;
}

// -- Luminance based Uncharted 2 Tonemapping variation --
// Reference: https://github.com/Zackin5/Filmic-Tonemapping-ReShade/blob/master/Uncharted2.fx

vec3 uncharted2TonemapLuminance(vec3 inputColor, float exposure, float gamma)
{
	// Do inital de-gamma of the game image to ensure we're operating in the correct colour range.
	inputColor = pow(inputColor, vec3(gamma));
		
	inputColor *= exposure;

	const float ExposureBias = 2.0;
	vec3 curr;
	
    float lum = 0.2126 * inputColor.r + 0.7152 * inputColor.g + 0.0722 * inputColor.b;
    vec3 newLum = U2Tonemap(vec3(ExposureBias * lum));
    float lumScale = (newLum / lum).x;
    curr = inputColor*lumScale;

	vec3 whiteScale = 1.0 / U2Tonemap(vec3(U2_W));
	vec3 color = curr * whiteScale;
    
	// Do the post-tonemapping gamma correction
	color = pow(color, vec3(1 / gamma));
	
	return color;
}

// -- Optimized Hejl-Dawson Tonemap + Gamma correction --
// Reference: http://filmicworlds.com/blog/filmic-tonemapping-operators/

vec3 optimizedHejlDawsonTonemap(vec3 inputColor, float exposure, float gamma)
{
    inputColor *= exposure;
    vec3 x = max3(vec3(0), inputColor - 0.004);
    return (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
}

// -- Simple filmic ACES --
// Reference: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/

vec3 ACESFilmTonemap(vec3 inputColor, float exposure, float gamma)
{
    inputColor *= exposure;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((inputColor*(a*inputColor+b))/(inputColor*(c*inputColor+d)+e));
}

vec3 lumaWeightTonemap(vec3 rgb)
{
    return rgb / vec3(1.0 + rgbToLuminance(rgb));
}

vec3 lumaWeightTonemapInv(vec3 rgb)
{
    return rgb / vec3(1.0 - rgbToLuminance(rgb));
}

// -- Filmic ACES with conversion --
// Reference: https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat =
mat3(
    //0.59719, 0.35458, 0.04823,
    //0.07600, 0.90834, 0.01566,
    //0.02840, 0.13383, 0.83777
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04824, 0.01566, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat =
mat3(
    //1.60475, -0.53108, -0.07367,
    //-0.10208,  1.10813, -0.00605,
    //-0.00327, -0.07276,  1.07602

    1.60475, -0.10208, -0.00327,
    -0.53108, 1.10813, -0.07276,
    -0.07367, -0.00605, 1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 ACESFittedTonemap(vec3 inputColor, float exposure, float gamma)
{
    inputColor *= exposure;

    inputColor = ACESInputMat * inputColor;

    // Apply RRT and ODT
    inputColor = RRTAndODTFit(inputColor);

    inputColor = ACESOutputMat * inputColor;

    // Clamp to [0, 1]
    inputColor = saturate(inputColor);

    return inputColor;
}
