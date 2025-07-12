#include <glow-pipeline/internal/pass/ao/cssao/blurCommon.glsl>

in vec2 vTexCoord;

out vec4 fOut;

void main()
{
    float CenterDepth;
    float AO = ComputeBlur(vTexCoord, vec2(GlobalConstantBuffer_2.x, 0), CenterDepth);

    fOut = vec4(AO, CenterDepth, 1, 1);
}
