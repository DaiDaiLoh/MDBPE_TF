uniform sampler2DRect uTexDepth;
uniform sampler2DRect uTexNormal;

uniform vec2 uScreenSize;
uniform float uRadius;
uniform uint uSeed;
uniform bool uReverseZEnabled;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uInvProj;

uniform int uSamples;
uniform ivec2 uViewportOffset;

out float fSSAO;

uint wang_hash(uint seed)
{
    seed = (seed ^ uint(61)) ^ (seed >> 16);
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

float wang_float(uint hash)
{
    return hash / float(0x7FFFFFFF) / 2.0;
}

vec3 viewPosAt(vec2 sp)
{
    float d = texture(uTexDepth, sp).x;
    if(!uReverseZEnabled)
        d = d * 2.0 - 1.0;
    vec4 vp = uInvProj * vec4(sp / uScreenSize * 2 - 1, d, 1);
    return vec3(vp) / vp.w;
}

void main()
{
    if (uRadius <= 0)
    {
        fSSAO = uSamples;
        return;
    }

    float visible_samples = 0;
    float pi = 3.14159265359;
    vec2 fragCoord = gl_FragCoord.xy;

    vec3 n = texture(uTexNormal, fragCoord).xyz;
    if (n == vec3(0))
    {
        fSSAO = uSamples;
        return;
    }

    n = normalize(mat3(uView) * n);
    vec3 p = viewPosAt(fragCoord);

    // Clamp back-facing normals to closest camera-orthogonal vector
    if (dot(n, p) > 0)
        n = normalize(n - n * dot(n, normalize(p)));

    vec3 t0 = normalize(cross(abs(n.x) > abs(n.y) ? vec3(0,1,0) : vec3(1,0,0), n));
    vec3 t1 = normalize(cross(n, t0));

    uint seed = uint(fragCoord.x) + (uint(fragCoord.y) << 16);
    seed ^= uSeed;

    for (int _ = 0; _ < uSamples; ++_)
    {
        seed = wang_hash(seed);
        float r = sqrt(wang_float(seed));
        seed = wang_hash(seed);
        float a = wang_float(seed) * pi * 2;
        seed = wang_hash(seed);
        float rr = wang_float(seed);

        float ca = cos(a);
        float sa = sin(a);

        float x = r * ca;
        float y = r * sa;
        float z = sqrt(1 - x * x - y * y);

        vec3 pp = p + (x * t0 + y * t1 + z * n) * (uRadius * rr);
        vec4 sp = uProj * vec4(pp, 1);
        sp.xyz /= sp.w;

        if (sp.w < 0)
            visible_samples++;
        else
        {
            float ref_d = texture(uTexDepth, (sp.xy * .5 + .5) * uScreenSize).x;
            if(uReverseZEnabled)
            {
                float test_d = sp.z;
                visible_samples += float(test_d > ref_d);
            }
            else
            {
                float  test_d = sp.z * 0.5 + 0.5;
                visible_samples += float(test_d < ref_d);
            }
        }
    }

    fSSAO = visible_samples;
}
