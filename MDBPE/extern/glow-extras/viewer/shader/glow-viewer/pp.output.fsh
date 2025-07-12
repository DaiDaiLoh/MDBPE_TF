uniform sampler2DRect uTexOutput;

uniform int uAccumCnt;
uniform ivec2 uViewportOffset;
uniform bool uDebugPixels;

out vec4 fColor;

/**
Basic FXAA implementation based on the code on geeks3d.com with the
modification that the texture2DLod stuff was removed since it's
unsupported by WebGL.
--
From:
https://github.com/mitsuhiko/webgl-meincraft
Copyright (c) 2011 by Armin Ronacher.
Some rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.
    * The names of the contributors may not be used to endorse or
    promote products derived from this software without specific
    prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FXAA_REDUCE_MIN
    #define FXAA_REDUCE_MIN   (1.0/ 128.0)
#endif
#ifndef FXAA_REDUCE_MUL
    #define FXAA_REDUCE_MUL   (1.0 / 8.0)
#endif
#ifndef FXAA_SPAN_MAX
    #define FXAA_SPAN_MAX     8.0
#endif

//optimized version for mobile, where dependent
//texture reads can be a bottleneck
vec4 fxaa(sampler2DRect tex, vec2 fragCoord) {
    vec4 color;
    vec3 rgbNW = texture(tex, fragCoord + vec2(-1.0, -1.0)).xyz;
    vec3 rgbNE = texture(tex, fragCoord + vec2(1.0, -1.0)).xyz;
    vec3 rgbSW = texture(tex, fragCoord + vec2(-1.0, 1.0)).xyz;
    vec3 rgbSE = texture(tex, fragCoord + vec2(1.0, 1.0)).xyz;
    vec4 texColor = texture(tex, fragCoord);
    vec3 rgbM  = texColor.xyz;
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    mediump vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) *
                        (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);

    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX),
            max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
            dir * rcpDirMin));

    vec3 rgbA = 0.5 * (
        texture(tex, fragCoord + dir * (1.0 / 3.0 - 0.5)).xyz +
        texture(tex, fragCoord + dir * (2.0 / 3.0 - 0.5)).xyz);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, fragCoord + dir * -0.5).xyz +
        texture(tex, fragCoord + dir * 0.5).xyz);

    float lumaB = dot(rgbB, luma);
    if ((lumaB < lumaMin) || (lumaB > lumaMax))
        color = vec4(rgbA, texColor.a);
    else
        color = vec4(rgbB, texColor.a);
    return color;
}

uint wang_hash(uint seed)
{
    seed = (seed ^ uint(61)) ^ (seed >> 16);
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float wang_float(uint hash)
{
    return hash / float(0x7FFFFFFF) / 2.0;
}

void main()
{
    float fxaaMin = 1;
    float fxaaMax = 8;
    vec2 fragCoord = gl_FragCoord.xy - uViewportOffset;

    vec4 color = texture(uTexOutput, fragCoord);
    float ca = color.a;

    if (uAccumCnt < fxaaMax)
    {
        vec4 fcolor = fxaa(uTexOutput, fragCoord);
        color = mix(fcolor, color, smoothstep(fxaaMin, fxaaMax, float(uAccumCnt)));
    }

    // dithering
    uint seed = uint(fragCoord.x) + uint(fragCoord.y) * 8196;
    float r = wang_float(wang_hash(seed * 3 + 0));
    float g = wang_float(wang_hash(seed * 3 + 1));
    float b = wang_float(wang_hash(seed * 3 + 2));
    vec3 random = vec3(r, g, b);
    color.rgb += (random - .5) * (1 / 256.);

    // transparency debug
    if (uDebugPixels)
    {
        bool t = ca > 0;
        bool t0 = texture(uTexOutput, fragCoord + vec2(1, 0)).a > 0;
        bool t1 = texture(uTexOutput, fragCoord + vec2(-1, 0)).a > 0;
        bool t2 = texture(uTexOutput, fragCoord + vec2(0, 1)).a > 0;
        bool t3 = texture(uTexOutput, fragCoord + vec2(0, -1)).a > 0;

        if (t)
        {
            float a = 1;
            if (t0 && t1 && t2 && t3)
                a = .3;
            color.rgb = mix(color.rgb, vec3(1,0,1), a);
        }
    }

    fColor = color;
}
