uniform sampler2DRect uTexSSAO;
uniform sampler2DRect uTexColor;
uniform sampler2DRect uTexColorOverlay;
uniform sampler2DRect uTexDepth;
uniform sampler2DRect uTexColorTransparent;
uniform sampler2DRect uTexDepthTransparent;
uniform sampler2DRect uTexAccum;

uniform int uAccumCnt;
uniform int uSSAOSamples;
uniform float uSSAOPower;
uniform bool uEnableSSAO;
uniform bool uEnableTonemap;
uniform bool uReverseZEnabled;
uniform bool uForceAlphaOne;

uniform float uTonemapExposure;

out vec4 fOutput;
out vec4 fAccum;

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
    inputColor = clamp(inputColor, 0.0, 1.0);

    return inputColor;
}

vec3 reinhardTonemap(vec3 rgb)
{
    return rgb / (rgb + vec3(1));
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    float d = texture(uTexDepth, fragCoord).x;

    // compose color
    vec4 color = texture(uTexColor, fragCoord);
    vec4 colorOver = texture(uTexColorOverlay, fragCoord);
    vec4 colorTrans = texture(uTexColorTransparent, fragCoord);
    float dTrans = texture(uTexDepthTransparent, fragCoord).x;

    if(uReverseZEnabled)
    {
        if (dTrans > d)
            color = colorTrans;
    }
    else
    {
        if (dTrans < d)
            color = colorTrans;
    }
    
    // ssao
    if (uEnableSSAO && d > 0)
    {
        float ssao = texture(uTexSSAO, fragCoord).x;
        ssao /= uSSAOSamples;
        ssao = pow(ssao, uSSAOPower);
        color.rgb *= ssao;
    }

    // overlay
    color = mix(color, colorOver, colorOver.a);
    
    // accum buffer
    if (uAccumCnt > 0)
    {
        vec4 accum = texelFetch(uTexAccum, ivec2(fragCoord));
        color = mix(accum, color, 1.0 / (uAccumCnt + 1));
    }
    fAccum = color;

    if(uReverseZEnabled)
    {
        if (d > 0 && uEnableTonemap) color = vec4(ACESFittedTonemap(color.xyz, uTonemapExposure, 2.224), 1.0);
        else color = clamp(color, 0.0, 1.0);
    }
    else
    {
        if (d < 1 && uEnableTonemap) color = vec4(ACESFittedTonemap(color.xyz, uTonemapExposure, 2.224), 1.0);
        else color = clamp(color, 0.0, 1.0);
    }

    // gamma correction
    color.rgb = pow(color.rgb, vec3(1 / 2.224));

	if (uForceAlphaOne)
	{
		color.a = 1.f;
	}

    fOutput = color;
}
