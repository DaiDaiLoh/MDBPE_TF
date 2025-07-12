#include <glow-pipeline/internal/common/cszReconstruct.glsl>

out float fLinearDepth;

uniform sampler2DRect gDepth;

//uniform float uFocusDistance;
//uniform float uFocusRange;
//uniform float uBackgroundBlur;

void main() {
    float depth = texelFetch(gDepth, ivec2(gl_FragCoord.xy)).x;
    fLinearDepth = reconstructCSZ(depth);

//    float coc = (
//        (depth < 1) ?
//        (linDepth - uFocusDistance) / uFocusRange
//        : uBackgroundBlur
//    );

//    fLinearDepth = vec2(linDepth, coc);
}
