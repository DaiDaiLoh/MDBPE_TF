#include <glow-pipeline/internal/pass/ao/cssao/blurCommon.glsl>

in vec2 vTexCoord;

out vec4 fOut;

void main()
{
    float CenterDepth;
    float AO = ComputeBlur(vTexCoord, vec2(0, GlobalConstantBuffer_2.y), CenterDepth);

    fOut.x = pow(clamp(AO, 0.0, 1.0), GlobalConstantBuffer_11);
}
