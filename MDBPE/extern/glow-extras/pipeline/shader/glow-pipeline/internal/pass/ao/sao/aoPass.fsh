#include <glow-pipeline/internal/pass/ao/sao/SAO.glsl>

out vec3 fAOOut;

void main()
{
    fAOOut = scalableAmbientObscurance();
}
