#include "QuadricRenderable.hh"

#include <glow/common/scoped_gl.hh>
#include <glow/objects/Program.hh>
#include <glow/objects/Shader.hh>
#include <glow/objects/ShaderStorageBuffer.hh>

#include <glow-extras/geometry/Cube.hh>

#include "../RenderInfo.hh"

glow::viewer::aabb glow::viewer::QuadricRenderable::computeAabb() { return mBoxAABB.transformed(transform()); }

size_t glow::viewer::QuadricRenderable::computeHash() const
{
    size_t h = mBoxQuadricHash;
    h = glow::hash_xxh3(as_byte_view(transform()), h);
    return h;
}

void glow::viewer::QuadricRenderable::renderShadow(const glow::viewer::RenderInfo& info)
{
    if (mQuadricCount == 0)
        return;

    renderQuadrics(mShadowShader, info);
}

void glow::viewer::QuadricRenderable::renderForward(const glow::viewer::RenderInfo& info)
{
    if (mQuadricCount == 0)
        return;

    renderQuadrics(mForwardShader, info);
}

glow::viewer::SharedQuadricRenderable glow::viewer::QuadricRenderable::create(array_view<const glow::viewer::boxed_quadric> quadrics)
{
    auto r = std::make_shared<QuadricRenderable>();
    r->initFromData(quadrics);
    return r;
}

void glow::viewer::QuadricRenderable::renderQuadrics(glow::SharedProgram const& program, const glow::viewer::RenderInfo& info)
{
    auto shader = program->use();

    GLOW_SCOPED(enable, GL_CULL_FACE);
    GLOW_SCOPED(cullFace, GL_FRONT);

    auto camPos = inverse(info.view) * tg::pos3::zero;

    shader["uModel"] = transform();
    shader["uViewProj"] = info.proj * info.view;
    shader["uLocalCamPos"] = inverse(transform()) * camPos;

    mVertexArray->bind().draw(mQuadricCount);
}

void glow::viewer::QuadricRenderable::initFromData(array_view<const glow::viewer::boxed_quadric> quadrics)
{
    mBoxQuadricHash = glow::hash_xxh3(quadrics.as_bytes(), 0x5321);
    mQuadricCount = int(quadrics.size());

    mBoxAABB = {};
    for (auto const& q : quadrics)
        for (auto const v : tg::vertices_of(q.box))
            mBoxAABB.add(v);

    mVertexArray = glow::geometry::make_cube();

    mQuadricBuffer = glow::ShaderStorageBuffer::create();
    mQuadricBuffer->bind().setData(quadrics.size() * sizeof(quadrics[0]), quadrics.data());

    auto const make_shader = [&](bool is_shadow) -> SharedProgram {
        glow::SharedShader vs, fs;

        std::string buffer_def = R"(

struct quadric_info
{
    float A00;
    float A01;
    float A02;
    float A11;
    float A12;
    float A22;
    float b0;
    float b1;
    float b2;
    float c;

    float posX;
    float posY;
    float posZ;
    float dir0X;
    float dir0Y;
    float dir0Z;
    float dir1X;
    float dir1Y;
    float dir1Z;
    float dir2X;
    float dir2Y;
    float dir2Z;

    float colR;
    float colG;
    float colB;
    float colA;

    uint options;

    float _padding;
};

layout(std430, binding = 0) restrict readonly buffer bQuadricBuffer
{
    quadric_info quadrics[];
};

mat3 matrixA(in quadric_info q)
{
    mat3 A;
    A[0][0] = q.A00;
    A[0][1] = q.A01;
    A[0][2] = q.A02;
    A[1][0] = q.A01;
    A[1][1] = q.A11;
    A[1][2] = q.A12;
    A[2][0] = q.A02;
    A[2][1] = q.A12;
    A[2][2] = q.A22;
    return A;
}

mat3 halfExtents(in quadric_info q)
{
    mat3 A;
    A[0][0] = q.dir0X;
    A[0][1] = q.dir0Y;
    A[0][2] = q.dir0Z;
    A[1][0] = q.dir1X;
    A[1][1] = q.dir1Y;
    A[1][2] = q.dir1Z;
    A[2][0] = q.dir2X;
    A[2][1] = q.dir2Y;
    A[2][2] = q.dir2Z;
    return A;
}

vec3 vecB(in quadric_info q)
{
    vec3 b;
    b.x = q.b0;
    b.y = q.b1;
    b.z = q.b2;
    return b;
}

vec3 vecPos(in quadric_info q)
{
    vec3 p;
    p.x = q.posX;
    p.y = q.posY;
    p.z = q.posZ;
    return p;
}
vec3 vecDir0(in quadric_info q)
{
    vec3 p;
    p.x = q.dir0X;
    p.y = q.dir0Y;
    p.z = q.dir0Z;
    return p;
}
vec3 vecDir1(in quadric_info q)
{
    vec3 p;
    p.x = q.dir1X;
    p.y = q.dir1Y;
    p.z = q.dir1Z;
    return p;
}
vec3 vecDir2(in quadric_info q)
{
    vec3 p;
    p.x = q.dir2X;
    p.y = q.dir2Y;
    p.z = q.dir2Z;
    return p;
}

float quadricValue(in quadric_info q, vec3 x)
{
    mat3 A = matrixA(q);
    vec3 b = vecB(q);
    float c = q.c;

    return dot(x, A * x) - 2 * dot(b, x) + c;
}

)";
        std::string uniform_def = R"(

uniform mat4 uModel;
uniform mat4 uViewProj;

uniform vec3 uLocalCamPos;

)";

        // VS
        {
            std::string src;
            src += buffer_def;
            src += uniform_def;

            src += R"(

in vec3 aPosition; // cube

out vec3 vLocalPos;
out vec3 vWorldPos;
flat out int vQuadricIdx;

void main()
{
    quadric_info qi = quadrics[gl_InstanceID];
    vQuadricIdx = gl_InstanceID;

    vec3 localPos = vecPos(qi) + halfExtents(qi) * aPosition;
    vec4 worldPos = uModel * vec4(localPos, 1);
    vLocalPos = localPos;
    vWorldPos = worldPos.xyz;
    gl_Position = uViewProj * worldPos;
}

)";

            vs = glow::Shader::createFromSource(GL_VERTEX_SHADER, src);
        }

        // FS
        {
            std::string src;
            src += buffer_def;
            src += uniform_def;

            (void)is_shadow; // TODO: optimize shader

            src += R"(

in vec3 vLocalPos;
in vec3 vWorldPos;
flat in int vQuadricIdx;

out vec4 fColor;
out vec3 fNormal;

bool is_valid(float t, vec3 ip, in quadric_info q)
{
    if (t < 0)
        return false;

    vec3 r = ip - vecPos(q);

    vec3 d0 = vecDir0(q);
    if (abs(dot(r, d0)) > dot(d0, d0) * 1.001)
        return false;

    vec3 d1 = vecDir1(q);
    if (abs(dot(r, d1)) > dot(d1, d1) * 1.001)
        return false;

    vec3 d2 = vecDir2(q);
    if (abs(dot(r, d2)) > dot(d2, d2) * 1.001)
        return false;

    return true;
}

vec3 cubeNormal(vec3 p, in quadric_info q)
{
    vec3 r = p - vecPos(q);

    vec3 d0 = vecDir0(q);
    vec3 d1 = vecDir1(q);
    vec3 d2 = vecDir2(q);

    float dot0 = dot(r, d0);
    float dot1 = dot(r, d1);
    float dot2 = dot(r, d2);

    float t0 = abs(dot(d0, d0) - abs(dot0));
    float t1 = abs(dot(d1, d1) - abs(dot1));
    float t2 = abs(dot(d2, d2) - abs(dot2));

    if (t0 < t1 && t0 < t2)
        return normalize(d0) * (dot0 > 0 ? 1 : -1);
    else if (t1 < t2)
        return normalize(d1) * (dot1 > 0 ? 1 : -1);
    else
        return normalize(d2) * (dot2 > 0 ? 1 : -1);
}

struct cube_intersection
{
    float t0;
    float t1;
    vec3 N0;
    vec3 N1;
    vec3 p0;
    vec3 p1;
};

cube_intersection intersect_cube(vec3 p, vec3 dir, in quadric_info q)
{
    cube_intersection r;
    r.t0 = -1e6;
    r.t1 = 1e6;
    r.N0 = vec3(0);
    r.N1 = vec3(0);
    r.p0 = vec3(0);
    r.p1 = vec3(0);

    if ((q.options & 0xFF) == 0)
    {
        r.t0 = -1;
        r.t1 = -1;
        return r;
    }

    vec3 d0 = vecDir0(q);
    vec3 d1 = vecDir1(q);
    vec3 d2 = vecDir2(q);

    vec3 ddd = d0 + d1 + d2;
    vec3 bMin = vecPos(q) - ddd - p;
    vec3 bMax = vecPos(q) + ddd - p;

    {
        float rDir = dot(dir, d0);
        if (abs(rDir) > 0.000001)
        {
            float tM0 = dot(bMin, d0) / rDir;
            float tM1 = dot(bMax, d0) / rDir;
            float tMin = min(tM0, tM1);
            float tMax = max(tM0, tM1);

            if (tMin > r.t0)
            {
                r.t0 = tMin;
                r.N0 = d0;
            }
            if (tMax < r.t1)
            {
                r.t1 = tMax;
                r.N1 = d0;
            }
        }
    }
    {
        float rDir = dot(dir, d1);
        if (abs(rDir) > 0.000001)
        {
            float tM0 = dot(bMin, d1) / rDir;
            float tM1 = dot(bMax, d1) / rDir;
            float tMin = min(tM0, tM1);
            float tMax = max(tM0, tM1);

            if (tMin > r.t0)
            {
                r.t0 = tMin;
                r.N0 = d1;
            }
            if (tMax < r.t1)
            {
                r.t1 = tMax;
                r.N1 = d1;
            }
        }
    }
    {
        float rDir = dot(dir, d2);
        if (abs(rDir) > 0.000001)
        {
            float tM0 = dot(bMin, d2) / rDir;
            float tM1 = dot(bMax, d2) / rDir;
            float tMin = min(tM0, tM1);
            float tMax = max(tM0, tM1);

            if (tMin > r.t0)
            {
                r.t0 = tMin;
                r.N0 = d2;
            }
            if (tMax < r.t1)
            {
                r.t1 = tMax;
                r.N1 = d2;
            }
        }
    }

    r.p0 = p + dir * r.t0;
    r.p1 = p + dir * r.t1;

    r.N0 = normalize(r.N0);
    if (dot(r.N0, dir) > 0)
        r.N0 = -r.N0;
    r.N1 = normalize(r.N1);
    if (dot(r.N1, dir) > 0)
        r.N1 = -r.N1;

    return r;
}

void writeResult(vec3 localPos, vec3 localN, vec3 viewDir, in quadric_info q)
{
    vec3 N = normalize(mat3(uModel) * localN);

    if (dot(N, viewDir) > 0)
        N = -N;

    fNormal = N;
    fColor = vec4(q.colR, q.colG, q.colB, 1);
    fColor.rgb *= (N.y * 0.4 + 0.6);

    vec4 screenPos = uViewProj * uModel * vec4(localPos, 1);
    gl_FragDepth = screenPos.z / screenPos.w; // assumes reverse Z
}

void main()
{
    quadric_info q = quadrics[vQuadricIdx];

    // DEBUG
    // {
    //     fNormal = cubeNormal(vLocalPos, q);
    //     fColor = vec4(q.colR, q.colG, q.colB, 1);
    //     vec4 screenPos = uViewProj * uModel * vec4(vLocalPos, 1);
    //     gl_FragDepth = screenPos.z / screenPos.w; // assumes reverse Z
    // }
    // return;

    mat3 A = matrixA(q);
    vec3 b = vecB(q);
    float c = q.c;

    vec3 d = normalize(vLocalPos - uLocalCamPos);
    vec3 p = uLocalCamPos;

    cube_intersection ci = intersect_cube(p, d, q);
    if (ci.t0 >= 0 && quadricValue(q, ci.p0) <= 0) // front intersection
    {
        writeResult(ci.p0, ci.N0, d, q);
        return;
    }

    if ((q.options & 0xFF00) == 0) // no quadric surface
    {
        if (ci.t1 >= 0 && quadricValue(q, ci.p1) <= 0) // back intersection
        {
            writeResult(ci.p1, ci.N1, d, q);
            return;
        }
        else
        {
            discard;
        }
    }

    vec3 Ad = A * d;
    float f = dot(d, Ad);
    float qp = 2 * (dot(p, Ad) - dot(b, d)) / f;
    float qq = (dot(p, A * p) - 2 * dot(b, p) + c) / f;

    float p_half = qp / 2;
    float D = p_half * p_half - qq;

    if (D < 0) // no quadric intersection
    {
        if (ci.t1 >= 0 && quadricValue(q, ci.p1) <= 0) // back intersection
        {
            writeResult(ci.p1, ci.N1, d, q);
            return;
        }
        else
        {
            discard;
        }
    }

    D = sqrt(D);

    float t0 = -D - p_half;
    float t1 = D - p_half;

    float t = t0;
    vec3 ip = p + d * t;

    if (!is_valid(t, ip, q))
    {
        t = t1;
        ip = p + d * t;

        if (!is_valid(t, ip, q))
        {
            if (ci.t1 >= 0 && quadricValue(q, ci.p1) <= 0) // back intersection
            {
                writeResult(ci.p1, ci.N1, d, q);
                return;
            }
            else
            {
                discard;
            }
        }
    }

    vec3 N = A * ip - b;

    writeResult(ip, N, d, q);
}

)";

            fs = glow::Shader::createFromSource(GL_FRAGMENT_SHADER, src);
        }

        auto prog = glow::Program::create({vs, fs});
        prog->setShaderStorageBuffer("bQuadricBuffer", mQuadricBuffer);
        return prog;
    };

    mForwardShader = make_shader(false);
    mShadowShader = make_shader(true);
}

void glow::viewer::boxed_quadric::set_cylinder(const tg::cylinder3& c)
{
    auto const center = centroid_of(c.axis);
    auto const he0 = c.axis.pos1 - center;
    auto const he1 = c.radius * tg::any_normal(he0);
    auto const he2 = c.radius * normalize(cross(he0, he1));

    box.center = center;
    box.half_extents[0] = he0;
    box.half_extents[1] = he1;
    box.half_extents[2] = he2;

    // TODO: this can be optimized by explicitly computing the final transformation
    tg::mat4 Q;
    Q[0][0] = 0;
    Q[1][1] = 1;
    Q[2][2] = 1;
    Q[3][3] = -c.radius * c.radius;

    // auto const inv_he0 = he0 / length_sqr(he0);
    // auto const inv_he1 = he1 / tg::pow2(c.radius);
    // auto const inv_he2 = he2 / tg::pow2(c.radius);

    tg::mat4 M;
    M[0] = tg::vec4(normalize(he0), 0);
    M[1] = tg::vec4(he1 / c.radius, 0);
    M[2] = tg::vec4(he2 / c.radius, 0);
    M[3] = tg::vec4(center, 1);

    auto const invM = inverse(M);

    quadric = tg::quadric3::from_coefficients(transpose(invM) * Q * invM);
}
