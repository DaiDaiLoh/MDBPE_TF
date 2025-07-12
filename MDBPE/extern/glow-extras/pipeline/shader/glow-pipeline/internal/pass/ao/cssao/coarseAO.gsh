layout(std140) uniform uCSSAOPerPass {
    vec4 f4Jitter;
    vec2 f2Offset;
    float fSliceIndex;
    uint uSliceIndex;
} uPPCB;

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

void main()
{
    gl_Layer = int(uPPCB.uSliceIndex);

    for (int VertexID = 0; VertexID < 3; VertexID++)
    {
        vec2 texCoords = vec2( (VertexID << 1) & 2, VertexID & 2 );
        gl_Position = vec4( texCoords * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0) , 0.0, 1.0 );
        EmitVertex();
    }
    EndPrimitive();
}