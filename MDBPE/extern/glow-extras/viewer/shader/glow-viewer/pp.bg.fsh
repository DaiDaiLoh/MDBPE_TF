uniform bool uPrintMode;
uniform bool uUseEnvMap;
uniform vec3 uInnerColor;
uniform vec3 uOuterColor;

uniform samplerCube uTexEnvMap;
uniform mat4 uInvProj;
uniform mat4 uInvView;

in vec2 vPosition;

out vec4 fColor;
out vec3 fNormal;

void main()
{
    float d = smoothstep(0.0, 0.5, distance(vPosition, vec2(0.5)));
    fColor.a = 0;
    fColor.rgb = mix(uInnerColor, uOuterColor, d);
    fColor.rgb = pow(fColor.rgb, vec3(2.224));

    if (uPrintMode)
        fColor.rgb = vec3(1);

    if (uUseEnvMap)
    {
        vec4 near = uInvProj * vec4(vPosition * 2 - 1, 0.8, 1);
        vec4 far = uInvProj * vec4(vPosition * 2 - 1, 0.05, 1);

        near /= near.w;
        far /= far.w;

        near = uInvView * near;
        far = uInvView * far;

        fColor.rgb = texture(uTexEnvMap, (far - near).xyz).rgb;
    }

    fNormal = vec3(0,0,0);
}
