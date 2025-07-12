out vec2 vTexCoord;
out vec4 vScreenPosition;

void main()
{
    vTexCoord = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vScreenPosition = vec4(vTexCoord * vec2( 2.0, 2.0 ) + vec2( -1.0, -1.0) , 0.0, 1.0 );
    gl_Position = vScreenPosition;
}
