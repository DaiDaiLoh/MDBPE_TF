uniform sampler2DRect uTexDepth;
uniform sampler2DRect uTexNormal;

uniform float uNearPlane;
uniform float uFarPlane;
uniform vec3 uCamPos;
uniform mat4 uInvProj;
uniform mat4 uInvView;

uniform float uDepthThreshold;
uniform float uNormalThreshold;
uniform ivec2 uViewportOffset;

uniform bool uReverseZEnabled;

in vec2 vPosition;

out vec4 fColor;

float linearDepth(float ndc_depth)
{
    if(uReverseZEnabled)
    {
        return uNearPlane / ndc_depth;
    }
    else
    {
        return (2.0 * uNearPlane * uFarPlane) / (uFarPlane + uNearPlane - ndc_depth * (uFarPlane - uNearPlane));	
    }
}

float NdcDepthAt(vec2 t)
{
    if(uReverseZEnabled)
    {
        return texture(uTexDepth, t).x;
    }   
    else
    {
        return texture(uTexDepth, t).x * 2.0 - 1.0;
    }
}

float linearDepthAt(vec2 t)
{
    return linearDepth(NdcDepthAt(t));
}

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    float ndc_depth = NdcDepthAt(fragCoord);
    float d = linearDepth(ndc_depth);
    vec3 n = texture(uTexNormal, fragCoord).xyz;

    vec4 ss = vec4(vPosition * 2 - 1, ndc_depth, 1);
    vec4 vs = uInvProj * ss;
    vs /= vs.w;
    vec4 ws = uInvView * vs;
    vec3 worldPos = ws.xyz;

    vec3 camDir = normalize(worldPos - uCamPos);

    vec2[] offsets = vec2[](
        vec2(1, 0),
        vec2(0, 1),
        vec2(-1, 0),
        vec2(0, -1)
    );

    bool is_edge = false;

    float df = float(n != vec3(0)); // 0 if normal is 0

    float depthThreshold = uDepthThreshold / (abs(dot(camDir, n)) + 0.001);
    depthThreshold = mix(uDepthThreshold, depthThreshold, df);
    
    for (int od = 1; od <= 1; ++od)
    for (int i = 0; i < 4; ++i)
    {
        vec2 o = offsets[i] * od;
        float dd = linearDepthAt(fragCoord + o);
        vec3 nn = texture(uTexNormal, fragCoord + o).xyz;

        is_edge = is_edge || abs(d - dd) * df > depthThreshold;
        is_edge = is_edge || distance(n, nn) > uNormalThreshold;
    }

    if (is_edge)
        fColor = vec4(0,0,0,0.8);
    else
        discard;
}
