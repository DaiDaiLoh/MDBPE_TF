uniform sampler2DArray uInputAO;
uniform sampler2DRect uInputDepth;

in vec2 vTexCoord;

out vec4 fOut;

vec4 Input0;
vec4 Temp[2];
ivec4 Temp_int[2];
uvec4 Temp_uint[2];

void main()
{
    Input0.xy = gl_FragCoord.xy;
    Temp[0].xy = vec4(floor(Input0.xyxx)).xy;
    Temp[0].zw = vec4(abs(Temp[0].yyyx) * vec4(0.000000, 0.000000, 0.250000, 0.250000)).zw;
    Temp[0].xy = vec4(Temp[0].xyxx * vec4(0.250000, 0.250000, 0.000000, 0.000000)).xy;
    Temp_int[1].xy = ivec4(Temp[0].xyxx).xy;
    Temp[0].xy = vec4(fract(Temp[0].zwzz)).xy;
    Temp[0].x = vec4(dot((Temp[0].xyxx).xy, (vec4(16.000000, 4.000000, 0.000000, 0.000000)).xy)).x;
    Temp_int[1].z = int(Temp[0].x);
    Temp[1].w = vec4(0.000000).w;
    Temp[0].x = texelFetch(uInputAO, ivec3((Temp_int[1]).xyz), 0).x;
    fOut.x = vec4(Temp[0].x).x;
    Temp[0].x = (texture(uInputDepth, gl_FragCoord.xy)).x;
    fOut.y = vec4(Temp[0].x).y;
}
