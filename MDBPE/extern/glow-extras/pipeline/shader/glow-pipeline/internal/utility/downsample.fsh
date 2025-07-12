uniform sampler2DRect uInput;
uniform float uInverseRatio; // e.g. 2 for a quarter-res buffer

out vec3 fHDR;

void main()
{
    fHDR = texture(uInput, gl_FragCoord.xy * uInverseRatio).rgb;
}
