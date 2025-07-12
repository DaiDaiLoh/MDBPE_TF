uniform sampler2DRect uInput;
out vec3 fHDR;

void main()
{
    fHDR = texture(uInput, gl_FragCoord.xy).rgb;
}