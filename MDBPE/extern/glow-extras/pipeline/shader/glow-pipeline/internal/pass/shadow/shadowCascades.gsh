#include <glow-pipeline/internal/common/globals.hh>

layout(triangles, invocations = GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT) in;

layout(triangle_strip, max_vertices = 3) out;

layout(std140) uniform uShadowLightVPUBO {
    mat4 lightVPs[GLOW_PIPELINE_SHADOW_CASCADE_AMOUNT];
};

void main()
{
    for (int i = 0; i < 3; ++i) // Triangles assumed
    {
        gl_Layer = gl_InvocationID;
        gl_Position = lightVPs[gl_InvocationID] * gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
