#include "PostProcessing.hh"

#include <glow/common/scoped_gl.hh>

#include <glow/objects/ArrayBuffer.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Shader.hh>
#include <glow/objects/TextureRectangle.hh>
#include <glow/objects/VertexArray.hh>

using namespace glow;
using namespace shader;

void PostProcessing::gradientRadial(tg::color3 colorInner, tg::color3 colorOuter, tg::pos2 center, float radius)
{
    init();

    GLOW_SCOPED(disable, GL_CULL_FACE);
    GLOW_SCOPED(disable, GL_DEPTH_TEST);

    auto shader = mProgramGradient->use();
    shader.setUniform("uColorInner", colorInner);
    shader.setUniform("uColorOuter", colorOuter);
    shader.setUniform("uRadius", radius);
    shader.setUniform("uCenter", center);

    mQuad->bind().draw();
}

void PostProcessing::drawTexture(const SharedTextureRectangle& tex, tg::pos2 start, tg::pos2 end, tg::mat4 const& colorMatrix)
{
    init();

    GLOW_SCOPED(disable, GL_CULL_FACE);
    GLOW_SCOPED(disable, GL_DEPTH_TEST);

    auto shader = mProgramTextureRect->use();
    shader.setTexture("uTexture", tex);
    shader.setUniform("uStart", start);
    shader.setUniform("uEnd", end);
    shader.setUniform("uColorMat", colorMatrix);

    mQuad->bind().draw();
}

void PostProcessing::output(const SharedTextureRectangle& hdr, bool dithering, bool fxaa, bool gamma_correction)
{
    init();

    GLOW_SCOPED(disable, GL_CULL_FACE);
    GLOW_SCOPED(disable, GL_DEPTH_TEST);

    auto shader = mProgramOutput->use();
    shader.setTexture("uTexture", hdr);
    shader.setUniform("uDithering", dithering);
    shader.setUniform("uFXAA", fxaa);
    shader.setUniform("uGammaCorrection", gamma_correction);

    mQuad->bind().draw();
}

void PostProcessing::copyFrom(const SharedTextureRectangle& src)
{
    init();

    GLOW_SCOPED(disable, GL_CULL_FACE);
    GLOW_SCOPED(disable, GL_DEPTH_TEST);

    auto shader = mProgramCopy->use();
    shader.setTexture("uTexture", src);

    mQuad->bind().draw();
}

void PostProcessing::init()
{
    if (mInitialized)
        return;
    mInitialized = true;

    std::vector<tg::pos2> quadPos = {
        {0.0f, 0.0f}, //
        {1.0f, 0.0f}, //
        {1.0f, 1.0f}, //
        {0.0f, 1.0f}  //
    };
    auto abQuad = ArrayBuffer::create();
    abQuad->defineAttribute<tg::pos2>("aPosition");
    abQuad->bind().setData(quadPos);
    mQuad = VertexArray::create(abQuad, GL_TRIANGLE_FAN);

    auto vsQuad = Shader::createFromSource(GL_VERTEX_SHADER, R"(
                                           in vec2 aPosition;
                                           out vec2 vPosition;
                                           void main() {
                                                vPosition = aPosition;
                                                gl_Position = vec4(aPosition * 2 - 1, 0, 1);
                                           }
                                       )");

    auto fsGradient = Shader::createFromSource(GL_FRAGMENT_SHADER, R"(
                                               uniform vec3 uColorInner;
                                               uniform vec3 uColorOuter;
                                               uniform float uRadius;
                                               uniform vec2 uCenter;
                                               in vec2 vPosition;
                                               out vec3 fColor;
                                               void main() {
                                                    float d = smoothstep(0.0, uRadius, distance(vPosition, uCenter));
                                                    fColor = mix(uColorInner, uColorOuter, d);
                                               }
                                                                   )");
    auto fsTextureRect = Shader::createFromSource(GL_FRAGMENT_SHADER, R"(
                                               uniform sampler2DRect uTexture;
                                               uniform vec2 uStart;
                                               uniform vec2 uEnd;
                                               uniform mat4 uColorMat;
                                               in vec2 vPosition;
                                               out vec3 fColor;
                                               void main() {
                                                   if (gl_FragCoord.x < uStart.x)
                                                      discard;
                                                   if (gl_FragCoord.y < uStart.y)
                                                      discard;
                                                   if (gl_FragCoord.x > uEnd.x)
                                                      discard;
                                                   if (gl_FragCoord.y > uEnd.y)
                                                      discard;
                                                    vec3 c = texture(uTexture, (gl_FragCoord.xy - uStart) / (uEnd - uStart) * textureSize(uTexture)).rgb;
                                                    fColor = clamp(vec3(uColorMat * vec4(c, 1)), vec3(0), vec3(1));
                                               }
                                                                   )");
    auto fsCopy = Shader::createFromSource(GL_FRAGMENT_SHADER, R"(
                                               uniform sampler2DRect uTexture;
                                               out vec4 fColor;
                                               void main() {
                                                    fColor = texture(uTexture, gl_FragCoord.xy);
                                               }
                                                                   )");
    auto fsOutput = Shader::createFromSource(GL_FRAGMENT_SHADER, R"(
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
                                                                     seed *= 9;
                                                                     seed = seed ^ (seed >> 4);
                                                                     seed *= 0x27d4eb2d;
                                                                     seed = seed ^ (seed >> 15);
                                                                     return seed;
                                                                 }

                                                                 float wang_float(uint hash)
                                                                 {
                                                                     return hash / float(0x7FFFFFFF) / 2.0;
                                                                 }

                                                                 uniform sampler2DRect uTexture;

                                                                 uniform bool uFXAA;
                                                                 uniform bool uDithering;
                                                                 uniform bool uGammaCorrection;

                                                                 out vec3 fColor;

                                                                 void main() {
                                                                    vec3 color = uFXAA ? fxaa(uTexture, gl_FragCoord.xy).rgb : texture(uTexture, gl_FragCoord.xy).rgb;

                                                                    if (uGammaCorrection) {
                                                                        color = pow(color, vec3(1 / 2.224));
                                                                    }

                                                                    fColor = color;

                                                                    if (uDithering) {
                                                                        uint seed = uint(gl_FragCoord.x) + uint(gl_FragCoord.y) * 8096;
                                                                        float r = wang_float(wang_hash(seed * 3 + 0));
                                                                        float g = wang_float(wang_hash(seed * 3 + 1));
                                                                        float b = wang_float(wang_hash(seed * 3 + 2));
                                                                        vec3 random = vec3(r, g, b);
                                                                        fColor += (random - .5) * (1 / 256.);
                                                                    }
                                                                 }
                                                  )");

    mProgramGradient = Program::create({vsQuad, fsGradient});
    mProgramTextureRect = Program::create({vsQuad, fsTextureRect});
    mProgramOutput = Program::create({vsQuad, fsOutput});
    mProgramCopy = Program::create({vsQuad, fsCopy});
}
