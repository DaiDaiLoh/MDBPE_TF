// NOTE: This extension allows setting gl_Layer in the vertex shader
// It is supported all the way back to the HD 5000 Series (2009)
// See https://opengl.gpuinfo.org/listreports.php?extension=GL_AMD_vertex_shader_layer
#extension GL_AMD_vertex_shader_layer : require

#include <glow-pipeline/internal/common/globals.hh>

uniform uShadowVPUBO {
    mat4 lightVPs[SHADOW_CASCADE_AMOUNT];
    float cascadeLimits[SHADOW_CASCADE_AMOUNT + 1];
};

void outputShadowVertex(vec3 position, int cascade)
{
    gl_Layer = cascade;
    gl_Position = lightVPs[cascade] * position;
}
