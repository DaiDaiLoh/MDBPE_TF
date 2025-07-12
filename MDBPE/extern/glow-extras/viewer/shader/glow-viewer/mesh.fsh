uniform vec3 uSunPos;
uniform vec3 uCamPos;

uniform int uRenderMode;
uniform int uShadingMode;

uniform sampler2D uTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vColor;

out vec4 fColor;
out vec3 fNormal;

void main()
{
    vec3 V = normalize(uCamPos - vPosition);
    vec3 L = normalize(uSunPos - vPosition);
    vec3 N = normalize(vNormal);
    vec3 H = normalize(L + V);

    if (!gl_FrontFacing)
        N = -N;

    fColor = vec4(1,0,1,1);

    switch (uRenderMode) {
        case 0:
            fColor.rgb = vec3(1);
            break;
        case 1:
            fColor.rgb = vColor;
            break;
        case 2:
            fColor.rgb = N;
            break;
    }

    switch (uShadingMode) {
        case 0:
            // unlit
            break;
        case 1:
            fColor.rgb *= N.y * .5 + .5;
            break;
    }

    fNormal = N;
}
