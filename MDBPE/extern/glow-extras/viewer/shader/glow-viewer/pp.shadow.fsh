uniform sampler2DRect uShadowMap;
uniform mat4 uSunView;
uniform mat4 uSunProj;
uniform vec3 uGroundShadowMin;
uniform vec3 uGroundShadowMax;
uniform bool uReverseZEnabled;

in vec2 vPosition;

out float fShadow;

void main()
{
    vec3 p = uGroundShadowMin + vec3(vPosition.x, 0, vPosition.y) * (uGroundShadowMax - uGroundShadowMin);

    fShadow = 0;

    // shadow
    vec4 sp = uSunProj * uSunView * vec4(p, 1);
    sp /= sp.w;
    if (sp.x > -1 && sp.x < 1 && sp.y > -1 && sp.y < 1)
    {
        float shadow_d = texture(uShadowMap, (sp.xy * .5 + .5) * textureSize(uShadowMap)).x;
        float ref_d = sp.z;

        if(uReverseZEnabled)
        {
            fShadow = int(shadow_d > ref_d);
        }
        else
        {
            shadow_d = shadow_d * 2 - 1;
            fShadow = int(shadow_d < ref_d);
        }
    }
}