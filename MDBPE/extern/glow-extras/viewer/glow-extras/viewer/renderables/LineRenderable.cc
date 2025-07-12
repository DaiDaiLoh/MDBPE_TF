#include "LineRenderable.hh"

#include <glow/common/hash.hh>

#include <glow-extras/material/IBL.hh>

#include "../RenderInfo.hh"
#include "../detail/MeshShaderBuilder.hh"

using namespace glow;
using namespace glow::viewer;

aabb LineRenderable::computeAabb()
{
    auto aabb = getMeshAABB().transformed(transform());
    if (mWorldSpaceSize)
    {
        const auto maxRadius = getAttribute("aLineWidth")->computeMaxFloat() / 2.0f;
        aabb.min -= tg::vec3(maxRadius);
        aabb.max += tg::vec3(maxRadius);
    }
    return aabb;
}

void LineRenderable::renderShadow(RenderInfo const& info)
{
    if (mVertexArray->getVertexCount() == 0)
        return; // empty

    auto shader = mShadowShader->use();
    shader["uIsReverseZEnabled"] = info.reverseZEnabled;
    shader.setUniform("uModel", transform());
    shader.setUniform("uInvModel", inverse(transform()));
    shader.setUniform("uNoCaps", mNoCaps);
    shader.setUniform("uWorldSpaceSize", mWorldSpaceSize);
    shader.setUniform("uCameraFacing", mCameraFacing);
    shader.setUniform("uScreenSize", tg::vec2(info.resolution));
    shader.setUniform("uView", info.view);
    shader.setUniform("uInvView", inverse(info.view));
    shader.setUniform("uInvProj", inverse(info.proj));
    shader.setUniform("uProj", info.proj);
    shader.setUniform("uCamPos", info.camPos);
    shader["uFragmentClipPlane"] = getFragmentClipPlane();
    shader["uPrimitiveClipPlane"] = getPrimitiveClipPlane();
    if (m3D)
    {
        auto const tanHalfFovY = 1.f / tg::abs(info.proj[1][1]);
        shader.setUniform("uTanFov2", tanHalfFovY);
    }

    for (auto const& a : getAttributes())
        a->prepareShader(shader);

    if (getMasking())
        getMasking()->prepareShader(shader);

    mVertexArray->bind().draw();
}

void LineRenderable::renderForward(RenderInfo const& info)
{
    if (mVertexArray->getVertexCount() == 0)
        return; // empty

    auto shader = mForwardShader->use();
    shader["uIsReverseZEnabled"] = info.reverseZEnabled;
    shader["uIsShadingEnabled"] = getShadingEnabled();
    shader["uModel"] = transform();
    shader["uInvModel"] = inverse(transform());
    shader["uNoCaps"] = mNoCaps;
    shader["uExtrapolate"] = mExtrapolate;
    shader["uWorldSpaceSize"] = mWorldSpaceSize;
    shader["uCameraFacing"] = mCameraFacing;
    shader["uScreenSize"] = tg::vec2(info.resolution);
    shader["uView"] = info.view;
    shader["uInvView"] = inverse(info.view);
    shader["uInvProj"] = inverse(info.proj);
    shader["uProj"] = info.proj;
    shader["uCamPos"] = info.camPos;
    shader["uFragmentClipPlane"] = getFragmentClipPlane();
    shader["uPrimitiveClipPlane"] = getPrimitiveClipPlane();
    shader["uSeed"] = tg::u32(info.accumulationCount);
    shader["uIsTransparent"] = getRenderMode() == GeometricRenderable::RenderMode::Transparent;

    if (m3D)
    {
        auto const tanHalfFovY = 1.f / tg::abs(info.proj[1][1]);
        shader["uTanFov2"] = tanHalfFovY;
    }

    if (getColorMapping())
        getColorMapping()->prepareShader(shader);
    if (getTexturing())
        getTexturing()->prepareShader(shader);
    if (getMasking())
        getMasking()->prepareShader(shader);
    for (auto const& a : getAttributes())
        a->prepareShader(shader);

    mVertexArray->bind().draw();
}

void glow::viewer::LineRenderable::renderPicking(RenderInfo const& info, int32_t renderableID)
{
    if (mVertexArray->getVertexCount() == 0 || !hasPicker())
        return; // empty

    auto shader = mPickingShader->use();

    shader["uIsReverseZEnabled"] = info.reverseZEnabled;
    shader["uIsShadingEnabled"] = getShadingEnabled();
    shader["uModel"] = transform();
    shader["uInvModel"] = inverse(transform());
    shader["uNoCaps"] = mNoCaps;
    shader["uExtrapolate"] = mExtrapolate;
    shader["uWorldSpaceSize"] = mWorldSpaceSize;
    shader["uCameraFacing"] = mCameraFacing;
    shader["uScreenSize"] = tg::vec2(info.resolution);
    shader["uView"] = info.view;
    shader["uInvView"] = inverse(info.view);
    shader["uInvProj"] = inverse(info.proj);
    shader["uProj"] = info.proj;
    shader["uCamPos"] = info.camPos;
    shader["uFragmentClipPlane"] = getFragmentClipPlane();
    shader["uPrimitiveClipPlane"] = getPrimitiveClipPlane();
    shader["uRenderableID"] = renderableID;
    if (m3D)
    {
        auto const tanHalfFovY = 1.f / tg::abs(info.proj[1][1]);
        shader["uTanFov2"] = tanHalfFovY;
    }

    if (getMasking())
        getMasking()->prepareShader(shader);
    for (auto const& a : getAttributes())
        a->prepareShader(shader);

    mVertexArray->bind().draw();
}

size_t LineRenderable::computeHash() const
{
    auto h = computeGenericGeometryHash();
    h = glow::hash_xxh3(as_byte_view(mRoundCaps), h);
    h = glow::hash_xxh3(as_byte_view(mNoCaps), h);
    h = glow::hash_xxh3(as_byte_view(mExtrapolate), h);
    h = glow::hash_xxh3(as_byte_view(m3D), h);
    h = glow::hash_xxh3(as_byte_view(mCameraFacing), h);
    h = glow::hash_xxh3(as_byte_view(mWorldSpaceSize), h);
    return h;
}

SharedLineRenderable LineRenderable::create(const builder::LineBuilder& builder)
{
    auto r = std::make_shared<LineRenderable>();
    r->initFromBuilder(builder);
    return r;
}

void LineRenderable::initFromBuilder(const builder::LineBuilder& builder)
{
    initGeometry(builder.getMeshDef(), builder.getAttributes());

    if (builder.mRoundCaps)
        mRoundCaps = true;
    else if (builder.mSquareCaps)
        mRoundCaps = false;
    else if (builder.mNoCaps)
        mRoundCaps = false;
    else
        mRoundCaps = true; // default

    if (builder.mNoCaps)
        mNoCaps = true;
    else
        mNoCaps = false; // default

    if (builder.mExtrapolate)
        mExtrapolate = true;
    else if (builder.mNoExtrapolation)
        mExtrapolate = false;
    else
        mExtrapolate = false; // default

    if (builder.mForce3D)
        m3D = true;
    else if (builder.mCameraFacing)
        m3D = false;
    else if (builder.mOwnNormals)
        m3D = false;
    else
        m3D = true; // default

    if (builder.mForce3D)
        mCameraFacing = false;
    else if (builder.mCameraFacing)
        mCameraFacing = true;
    else if (builder.mOwnNormals)
        mCameraFacing = false;
    else
        mCameraFacing = false; // default, because m3D is true in this case

    if (builder.mWorldSpaceSize)
        mWorldSpaceSize = true;
    else if (builder.mScreenSpaceSize)
        mWorldSpaceSize = false;
    else
        mWorldSpaceSize = false; // default

    mForceTwoColored = builder.mForceTwoColored;
    mDashSizeWorld = builder.mDashSizeWorld;

    if (builder.mOwnNormals && !m3D && !builder.mWorldSpaceSize && !builder.mScreenSpaceSize)
        glow::warning() << "Normal aligned lines need some size information, since the default screen space size does not work in this case.";
}

void LineRenderable::init()
{
    // add missing attributes
    if (getMasking())
        addAttribute(getMasking()->mDataAttribute);
    if (getTexturing())
        addAttribute(getTexturing()->mCoordsAttribute);
    else if (getColorMapping())
        addAttribute(getColorMapping()->mDataAttribute);
    else if (!hasAttribute("aColor"))
        addAttribute(detail::make_mesh_attribute("aColor", tg::color4(tg::color3(mCameraFacing ? 0.1f : 0.25f), 1.0f)));

    const auto aColor = getAttribute("aColor");
    const auto twoColored = aColor && (mForceTwoColored || aColor->hasTwoColoredLines());

    if (!hasAttribute("aLineWidth"))
        addAttribute(detail::make_mesh_attribute("aLineWidth", 5.0f));
    if (!hasAttribute("aNormal"))
    {
        addAttribute(detail::make_mesh_attribute("aNormal", tg::color3(0, 1, 0)));

        if (twoColored && m3D)
            glow::warning() << "Two colored 3D lines need normal information. Please add them using normals(...) and then force3D() render mode.";
    }
    if (hasPicker() && !hasAttribute("aPickID"))
    {
        int32_t vID = 0;
        auto& p = this->getPicker();
        pm::edge_attribute<int32_t> IDs;

        auto pm = std::dynamic_pointer_cast<detail::PolyMeshDefinition>(getMeshDefinition());
        TG_ASSERT(pm);

        IDs = pm::edge_attribute<int32_t>(pm->mesh);
        std::vector<pm::edge_index> edge_indices;
        edge_indices.reserve(IDs.size());
        for (auto e : pm->mesh.edges())
        {
            IDs[e] = vID;
            vID++;
            edge_indices.push_back(e.idx); // offers ability to map back the read IDs to face_indices
        }

        addAttribute(detail::make_mesh_attribute("aPickID", IDs));
        p.init(edge_indices);
    }

    // build meshes
    {
        std::vector<SharedArrayBuffer> abs;

        for (auto const& attr : getAttributes())
        {
            auto ab = attr->createLineRenderableArrayBuffer(*getMeshDefinition());
            if (!ab)
                continue;

            abs.push_back(ab);
        }

        mVertexArray = VertexArray::create(abs, nullptr, GL_LINES);
    }

    // build shader
    {
        auto const use_gAlpha = !m3D && (mRoundCaps || twoColored);

        // Shared functionality
        const auto createCommonShaderParts = [&](detail::MeshShaderBuilder& sb)
        {
            sb.addUniform("mat4", "uModel");
            sb.addUniform("mat4", "uInvModel");
            sb.addUniform("bool", "uWorldSpaceSize");
            sb.addUniform("bool", "uCameraFacing");
            sb.addUniform("bool", "uNoCaps");
            sb.addUniform("vec2", "uScreenSize");
            if (m3D)
                sb.addUniform("float", "uTanFov2");

            sb.addUniform("mat4", "uView");
            sb.addUniform("mat4", "uInvView");
            sb.addUniform("mat4", "uProj");
            sb.addUniform("mat4", "uInvProj");
            sb.addUniform("vec3", "uCamPos");
            sb.addUniform("bool", "uIsReverseZEnabled");
            sb.addUniform("vec4", "uFragmentClipPlane");
            sb.addUniform("vec4", "uPrimitiveClipPlane");

            sb.addPassthrough("vec3", "Position");
            sb.addPassthrough("vec3", "Normal");
            sb.addPassthrough("float", "LineWidth");
            sb.addPassthrough("vec3", "fragPosWS");

            if (mDashSizeWorld)
                sb.addPassthrough("float", "DashSize");

            sb.addVertexShaderCode("vOut.fragPosWS = vec3(0);");

            for (auto const& attr : getAttributes())
                attr->buildShader(sb);

            // masked mesh
            if (getMasking())
                getMasking()->buildShader(sb);

            // Geometry shader in- and output
            sb.addGeometryShaderDecl("layout(lines) in;");
            if (m3D)
                sb.addGeometryShaderDecl("layout(triangle_strip, max_vertices = 6) out;"); // 6 vertices for 4 triangles
            else
                sb.addGeometryShaderDecl("layout(triangle_strip, max_vertices = 8) out;"); // 8 vertices for 6 triangles

            if (m3D)
            {
                sb.addGeometryShaderDecl("out vec3 gLineOrigin;");
                sb.addGeometryShaderDecl("out vec3 gLineEnd;");
                sb.addGeometryShaderDecl("out vec3 gLineDir;");
                sb.addFragmentShaderDecl("in vec3 gLineOrigin;");
                sb.addFragmentShaderDecl("in vec3 gLineEnd;");
                sb.addFragmentShaderDecl("in vec3 gLineDir;");
            }

            if (use_gAlpha)
            {
                sb.addGeometryShaderDecl("out vec2 gAlpha;");
                sb.addFragmentShaderDecl("in vec2 gAlpha;");
            }

            // Geometry shader code

            if (m3D)
            { // 3D lines
                sb.addGeometryShaderCode(R"(
    PASSTHROUGH(0); // Load passthrough data for first vertex

    vec3 pos0 = vec3(uModel * vec4(vIn[0].Position, 1.0));
    vec3 pos1 = vec3(uModel * vec4(vIn[1].Position, 1.0));

    if (dot(uPrimitiveClipPlane.xyz, pos0) > uPrimitiveClipPlane.w || dot(uPrimitiveClipPlane.xyz, pos1) > uPrimitiveClipPlane.w)
        return;

    float s;
    if(uWorldSpaceSize) { // 3D world space lines
        s = vIn[0].LineWidth * 0.5;
    } else { // 3D screen space lines
        float l = 2 * distance(pos0, uCamPos) * uTanFov2;
        s = l * vIn[0].LineWidth / uScreenSize.y;
        vOut.LineWidth = 2 * s;
    }

    vec4 pos0VS = uView * vec4(pos0, 1);
    vec4 pos0CS = uProj * pos0VS;
    vec3 pos0NDC = pos0CS.xyz / pos0CS.w;

    // clip position to near plane in NDC and get ray origin by transforming into WS (required for orthographic rendering)
    vec4 p0NearNDC = vec4(pos0NDC.xy, -1, 1);
    vec4 p0NearCS = p0NearNDC * pos0CS.w;
    vec4 p0NearVS = uInvProj * p0NearCS;
    p0NearVS /= p0NearVS.w;
    vec4 p0NearWS = uInvView * p0NearVS;

    vec3 viewDir = normalize(pos0 - p0NearWS.xyz);
    vec3 diff = pos1 - pos0;
    vec3 right = normalize(diff);
    vec3 up = normalize(cross(right, viewDir));
    vec3 back = normalize(cross(right, up));
    vec3 r = s * right;
    vec3 u = s * up;
    vec3 b = s * back;
    vec3 n = normalize(mat3(uModel) * vIn[0].Normal); // Needed for two colored 3D mode, but eventually always recomputed in the fragment shader
    float ea = s / length(diff); // Extrapolation alpha
    gLineOrigin = pos0; // Used in fragment shader
    gLineEnd = pos1;
    gLineDir = right;

    // Determine which line end is visible
    bool firstVisible = dot(viewDir, diff) > 0;

    // Quad at one line end
    float cr = uNoCaps ? 0 : -1;
    if(firstVisible) {
        createVertex(pos0 - b, ea, n, r, u, cr, 1);
        createVertex(pos0 - b, ea, n, r, u, cr, -1);
    }
    createVertex(pos0 + b, ea, n, r, u, cr, 1);
    createVertex(pos0 + b, ea, n, r, u, cr, -1);

    // Quad at other line end, reusing last two vertices to also create quad along line

    PASSTHROUGH(1); // Load passthrough data for second vertex

    if(uWorldSpaceSize) {
        s = vIn[1].LineWidth * 0.5;
    } else {
        float l = 2 * distance(pos1, uCamPos) * uTanFov2;
        s = l * vIn[1].LineWidth / uScreenSize.y;
        vOut.LineWidth = 2 * s;
    }

    vec4 pos1VS = uView * vec4(pos1, 1);
    vec4 pos1CS = uProj * pos1VS;
    vec3 pos1NDC = pos1CS.xyz / pos1CS.w;

    // clip position to near plane in NDC and get ray origin by transforming into WS (required for orthographic rendering)
    vec4 p1NearNDC = vec4(pos1NDC.xy, -1, 1);
    vec4 p1NearCS = p1NearNDC * pos1CS.w;
    vec4 p1NearVS = uInvProj * p1NearCS;
    p1NearVS /= p1NearVS.w;
    vec4 p1NearWS = uInvView * p1NearVS;

    viewDir = normalize(pos1 - p1NearWS.xyz);
    up = normalize(cross(right, viewDir));
    back = normalize(cross(right, up));
    r = s * right;
    u = s * up;
    b = s * back;
    n = normalize(mat3(uModel) * vIn[1].Normal);
    ea = s / length(diff);

    cr = uNoCaps ? 1 : 2;
    createVertex(pos1 + b, ea, n, r, u, cr, 1);
    createVertex(pos1 + b, ea, n, r, u, cr, -1);
    if(!firstVisible) {
        createVertex(pos1 - b, ea, n, r, u, cr, 1);
        createVertex(pos1 - b, ea, n, r, u, cr, -1);
    }
)");
            }
            else
            {
                if (mWorldSpaceSize)
                { // Flat world space lines
                    sb.addGeometryShaderCode(R"(
    PASSTHROUGH(0); // Load passthrough data for first vertex

    vec3 pos0 = vec3(uModel * vec4(vIn[0].Position, 1.0));
    vec3 pos1 = vec3(uModel * vec4(vIn[1].Position, 1.0));

    if (dot(uPrimitiveClipPlane.xyz, pos0) > uPrimitiveClipPlane.w || dot(uPrimitiveClipPlane.xyz, pos1) > uPrimitiveClipPlane.w)
        return;

    vec3 normal0 = uCameraFacing ? normalize(uCamPos - pos0) : normalize(mat3(uModel) * vIn[0].Normal); // TODO: Correct?
    vec3 normal1 = uCameraFacing ? normalize(uCamPos - pos1) : normalize(mat3(uModel) * vIn[1].Normal);

    vec3 diff = pos1 - pos0;
    vec3 right = normalize(diff);
    vec3 up = normalize(cross(normal0, right));
    vec3 r = vIn[0].LineWidth * 0.5 * right;
    vec3 u = vIn[0].LineWidth * 0.5 * up;
    vec3 n = uCameraFacing ? vec3(0,0,0) : normal0; // If cameraFacing: Normal 0 signals unlit rendering
    float ea = vIn[0].LineWidth * 0.5 / length(diff); // Extrapolation alpha

    if(!uNoCaps) {
        createVertex(pos0, ea, n, r, u, -1, 1);
        createVertex(pos0, ea, n, r, u, -1, -1);
    }
    PASSTHROUGH(0);
    createVertex(pos0, ea, n, r, u, 0, 1);
    createVertex(pos0, ea, n, r, u, 0, -1);

    up = normalize(cross(normal1, right));
    r = vIn[1].LineWidth * 0.5 * right;
    u = vIn[1].LineWidth * 0.5 * up;

    PASSTHROUGH(1); // Load passthrough data for second vertex
    n = uCameraFacing ? vec3(0,0,0) : normal1;
    ea = vIn[1].LineWidth * 0.5 / length(diff);

    createVertex(pos1, ea, n, r, u, 1, 1);
    createVertex(pos1, ea, n, r, u, 1, -1);
    if(!uNoCaps) {
        createVertex(pos1, ea, n, r, u, 2, 1);
        createVertex(pos1, ea, n, r, u, 2, -1);
    }
)");
                }
                else
                { // Flat screen space lines
                    sb.addGeometryShaderCode(R"(
    PASSTHROUGH(0); // Load passthrough data for first vertex

    // it only makes sense if uCameraFacing is true for screenSpaceSize lines
    vec3 n = vec3(0,0,0);

    vec3 pos0 = vec3(uModel * vec4(vIn[0].Position, 1.0));
    vec3 pos1 = vec3(uModel * vec4(vIn[1].Position, 1.0));

    if (dot(uPrimitiveClipPlane.xyz, pos0) > uPrimitiveClipPlane.w || dot(uPrimitiveClipPlane.xyz, pos1) > uPrimitiveClipPlane.w)
        return;

    vec4 spos0 = uProj * uView * vec4(pos0, 1.0);
    vec4 spos1 = uProj * uView * vec4(pos1, 1.0);

    if(spos0.w < 0 || spos1.w < 0) return; // Fix for lines nearer than the near plane

    spos0 /= spos0.w;
    spos1 /= spos1.w;

    vec2 diff = spos1.xy - spos0.xy;
    vec2 right = normalize(diff);
    vec2 up = vec2(-right.y, right.x);
    vec4 r = vec4(vIn[0].LineWidth * 0.5 * right / uScreenSize, 0, 0);
    vec4 u = vec4(vIn[0].LineWidth * 0.5 * up / uScreenSize, 0, 0);
    float ea = length(vIn[0].LineWidth * 0.5 / uScreenSize) / length(diff);

    if(!uNoCaps) {
        createVertexInverse(spos0, ea, n, r, u, -1, 1);
        createVertexInverse(spos0, ea, n, r, u, -1, -1);
    }
    PASSTHROUGH(0);
    createVertexInverse(spos0, ea, n, r, u, 0, 1);
    createVertexInverse(spos0, ea, n, r, u, 0, -1);

    PASSTHROUGH(1); // Load passthrough data for second vertex
    r = vec4(vIn[1].LineWidth * 0.5 * right / uScreenSize, 0, 0);
    u = vec4(vIn[1].LineWidth * 0.5 * up / uScreenSize, 0, 0);
    ea = length(vIn[1].LineWidth * 0.5 / uScreenSize) / length(diff);

    createVertexInverse(spos1, ea, n, r, u, 1, 1);
    createVertexInverse(spos1, ea, n, r, u, 1, -1);
    if(!uNoCaps) {
        createVertexInverse(spos1, ea, n, r, u, 2, 1);
        createVertexInverse(spos1, ea, n, r, u, 2, -1);
    }
)");
                }
            }


            // Fragment shader code

            if (m3D)
            {
                sb.addFragmentShaderDecl(R"(
float distance2(vec3 a, vec3 b)
{
    vec3 d = a - b;
    return dot(d, d);
}
                )");

                // Camera ray: rayOrigin + tRay * rayDir;
                // Line: gLineOrigin + tLine * gLIneDir;
                // A: the angle between the Line and the Camera ray
                // r: the radius of the 2D slice of the capsule, i.e. its line width and radius of the two cap circles
                // s: the amount to go back on the ray dir to get to the intersection point
                // TODO in this shader code
                sb.addFragmentShaderCode(R"(
    // clip FragCoord to near plane in NDC and transform back into WS (required for orhtographic rendering)
    vec3 rayOriginNDC = vec3((gl_FragCoord.x / uScreenSize.x) * 2 - 1, (gl_FragCoord.y / uScreenSize.y) * 2 - 1, uIsReverseZEnabled ? 1 : -1);

    vec4 rayOriginCS = vec4(rayOriginNDC,1) / gl_FragCoord.w;
    vec4 rayOriginVS = (uInvProj * rayOriginCS);
    rayOriginVS /= rayOriginVS.w;
    vec3 rayOrigin = (uInvView * rayOriginVS).xyz;
    vec3 rayDir = normalize(vIn.fragPosWS - rayOrigin);

    float cosA = dot(gLineDir, rayDir);
    float sinA2 = 1 - cosA * cosA;

    // TODO: Handle view parallel to line. Can for example be detected by sinA2 == 0

    // Compute closest points of the two lines
    vec3 origDiff = rayOrigin - gLineOrigin;
    float fRay = dot(rayDir, origDiff);
    float fLine = dot(gLineDir, origDiff);
    float tRay = (cosA * fLine - fRay) / sinA2;
    float tLine = (fLine - cosA * fRay) / sinA2;

    vec3 closestOnRay = rayOrigin + tRay * rayDir;
    vec3 closestOnLine = gLineOrigin + tLine * gLineDir;
    float lineRayDist2 = distance2(closestOnRay, closestOnLine);
    float lineRadius2 = vLineWidth * vLineWidth / 4; // vLineWidth is diameter, thus halved for radius

    if(lineRayDist2 > lineRadius2) discard;

    // Radius in 2D slice
    float r = sqrt(lineRadius2 - lineRayDist2);

    // Infinite cylinder intersection
    float s = r / sqrt(sinA2);
    vec3 cylIntersection = closestOnRay - s * rayDir;
    float tRayCyl = tRay - s;

    // Project onto line segment
    float lineLength = length(gLineEnd - gLineOrigin);
    float lambda = dot(cylIntersection - gLineOrigin, gLineDir); // TODO: QUESTION: Is there a better way?
    lambda = clamp(lambda, 0, lineLength);
)");

                if (mDashSizeWorld)
                    sb.addFragmentShaderCode(R"(
    // dash
    float dash_size = vDashSize;
    if (dash_size > 0)
    {
        float dashes = float(int(ceil(lineLength / dash_size)) / 2 * 2);
        if (int(round(lambda / lineLength * dashes)) % 2 == 1)
            discard;
    }
)");

                sb.addFragmentShaderCode(R"(
    vec3 closestOnSegment = gLineOrigin + lambda * gLineDir;

    // Ray-Sphere intersection same as in PointRenderable
    vec3 sphereCenter = closestOnSegment;
    float tRaySphere = dot(rayDir, sphereCenter - rayOrigin);
    vec3 closestP = rayOrigin + tRaySphere * rayDir;
    float sphereDis2 = distance2(closestP, sphereCenter);

    if(sphereDis2 > lineRadius2) discard;

    tRaySphere -= sqrt(lineRadius2 - sphereDis2); // Go back on ray to intersection

    vec3 newPos = rayOrigin + max(tRayCyl, tRaySphere) * rayDir;

    if (dot(uFragmentClipPlane.xyz, newPos) > uFragmentClipPlane.w)
        discard;

    vec4 newPosCS = uProj * uView * vec4(newPos, 1);
    float depthNDC = newPosCS.z / newPosCS.w;

    if(uIsReverseZEnabled)
        gl_FragDepth = (depthNDC - gl_DepthRange.near) / gl_DepthRange.diff;
    else
        gl_FragDepth = depthNDC * 0.5 + 0.5;
                              )");
            }
            else // Not 3D
            {
                if (mRoundCaps)
                {
                    sb.addFragmentShaderCode(R"(
    if (dot(uFragmentClipPlane.xyz, vIn.fragPosWS) > uFragmentClipPlane.w)
        discard;
    if(gAlpha.x < 0 && distance(gAlpha, vec2(0, 0)) > 1) discard;
    if(gAlpha.x > 1 && distance(gAlpha, vec2(1, 0)) > 1) discard;)");
                }
            }
        };
        // End of common


        // Forward
        {
            detail::MeshShaderBuilder sbForward;

            if (twoColored)
                sbForward.addGeometryShaderCode(R"(
    gRightColor = vIn[0].Color;
    gLeftColor = vIn[1].Color;
)");

            createCommonShaderParts(sbForward);

            sbForward.addUniform("bool", "uExtrapolate");
            sbForward.addUniform("bool", "uIsShadingEnabled");
            sbForward.addUniform("uint", "uSeed");
            sbForward.addUniform("bool", "uIsTransparent");

            sbForward.addFragmentLocation("vec4", "fColor");
            sbForward.addFragmentLocation("vec3", "fNormal");

            // colored mesh
            if (aColor)
                sbForward.addPassthrough(aColor->typeInShader(), "Color", detail::MeshShaderBuilder::TypeHandling::ExtendToVec4Color);

            if (twoColored)
            {
                const auto type = aColor->typeInShader();
                sbForward.addGeometryShaderDecl("flat out " + type + " gLeftColor;");
                sbForward.addGeometryShaderDecl("flat out " + type + " gRightColor;");
                sbForward.addFragmentShaderDecl("flat in " + type + " gLeftColor;");
                sbForward.addFragmentShaderDecl("flat in " + type + " gRightColor;");
            }

            // data mapped mesh
            if (getColorMapping())
                getColorMapping()->buildShader(sbForward);

            // texture mesh
            if (getTexturing())
                getTexturing()->buildShader(sbForward);

            // Helper functions to emit vertices from the given relative line coordinates

            std::string createVertexForward = "void createVertex(vec3 basePos, float ea, vec3 n, vec3 r, vec3 u, float relX, float relY) \n{\n";
            if (use_gAlpha)
                createVertexForward += "    gAlpha = vec2(relX, relY); // Relative line position for fragment shader\n";
            createVertexForward += R"(
    if (uExtrapolate)
    {
        if (relX < 0)
            passthroughMix01(relX * ea);
        if (relX > 1)
            passthroughMix01((relX - 1) * ea + 1);
    }
    if (relX > 0)
        relX -= 1; // This sets the factor for the r vector to the right value in case of the second half

    vec3 outPos = basePos + relX * r + relY * u;
    gl_Position = uProj * uView * vec4(outPos, 1.0);

    vOut.fragPosWS = outPos;
    vOut.Normal = n; // Set normal again as it might have been overwritten by passthrough
    EmitVertex();
}
            )";
            sbForward.addGeometryShaderDecl(createVertexForward);

            std::string createVertexInverseForward
                = "void createVertexInverse(vec4 basePos, float ea, vec3 n, vec4 r, vec4 u, float relX, float relY) \n{\n";
            if (use_gAlpha)
                createVertexInverseForward += "    gAlpha = vec2(relX, relY);\n";
            createVertexInverseForward += R"(
    if (uExtrapolate)
    {
        if (relX < 0)
            passthroughMix01(relX * ea);
        if (relX > 1)
            passthroughMix01((relX - 1) * ea + 1);
    }
    if (relX > 0)
        relX -= 1;

    vec4 outPos = basePos + relX * r + relY * u;
    gl_Position = outPos;

    outPos = uInvProj * outPos;
    outPos /= outPos.w;
    outPos = uInvView * outPos;

    vOut.fragPosWS = vec3(outPos);
    vOut.Normal = n;
    EmitVertex();
}
            )";
            sbForward.addGeometryShaderDecl(createVertexInverseForward);

            // Fragment shader code
            if (twoColored && m3D)
                sbForward.addFragmentShaderCode(R"(
    vec3 leftDir = normalize(cross(vNormal, gLineDir));
    vNormal = normalize(newPos - closestOnSegment);
    vColor = padColor(dot(leftDir, vNormal) > 0 ? gLeftColor : gRightColor);)");

            else if (m3D)
                sbForward.addFragmentShaderCode("    vNormal = normalize(newPos - closestOnSegment);");

            else if (twoColored) // implies use of gAlpha
                sbForward.addFragmentShaderCode("    vColor = padColor(gAlpha.y > 0 ? gLeftColor : gRightColor);");

            // Rest is same for all versions
            sbForward.addFragmentShaderCode(R"(
    fNormal = vNormal == vec3(0) ? vNormal : normalize(vNormal);
    fColor.rgb = vColor.rgb * (uIsShadingEnabled ? fNormal.y * .4 + .6 : 1.0);
    fColor.a = 1;)");

            if (getRenderMode() == GeometricRenderable::RenderMode::Transparent)
                sbForward.addFragmentShaderCode(R"(
                                         if (uIsTransparent)
                                         {
                                            float a = vColor.a;

                                            if (a < make_hashed_threshold(gl_FragCoord.z, uSeed))
                                                discard;
                                         })");

            mForwardShader = sbForward.createProgram();
        }

        // Shadow
        {
            detail::MeshShaderBuilder sbShadow;
            createCommonShaderParts(sbShadow);

            // Helper functions to emit vertices from the given relative line coordinates
            std::string createVertexShadow = "void createVertex(vec3 basePos, float ea, vec3 n, vec3 r, vec3 u, float relX, float relY) \n{\n";
            if (use_gAlpha)
                createVertexShadow += "    gAlpha = vec2(relX, relY); // Relative line position for fragment shader\n";
            createVertexShadow += R"(
    if (relX > 0)
        relX -= 1; // This sets the factor for the r vector to the right value in case of the second half

    vec3 outPos = basePos + relX * r + relY * u;
    gl_Position = uProj * uView * vec4(outPos, 1.0);

    vOut.fragPosWS = outPos;
    EmitVertex();
}
            )";

            sbShadow.addGeometryShaderDecl(createVertexShadow);

            std::string createVertexInverseShadow
                = "void createVertexInverse(vec4 basePos, float ea, vec3 n, vec4 r, vec4 u, float relX, float relY)\n{\n";
            if (use_gAlpha)
                createVertexInverseShadow += "    gAlpha = vec2(relX, relY);\n";
            createVertexInverseShadow += R"(
    if (relX > 0)
        relX -= 1;

    vec4 outPos = basePos + relX * r + relY * u;
    gl_Position = outPos;

    outPos = uInvProj * outPos;
    outPos /= outPos.w;
    outPos = uInvView * outPos;

    vOut.fragPosWS = vec3(outPos);
    EmitVertex();
}
            )";
            sbShadow.addGeometryShaderDecl(createVertexInverseShadow);

            mShadowShader = sbShadow.createProgram();
        }

        // picking shader
        if (hasPicker())
        {
            detail::MeshShaderBuilder sbPicking;

            createCommonShaderParts(sbPicking);

            sbPicking.addUniform("bool", "uExtrapolate");
            sbPicking.addUniform("bool", "uIsShadingEnabled");

            sbPicking.addFragmentLocation("ivec2", "fPickIDs");

            sbPicking.addUniform("int", "uRenderableID");
            sbPicking.addPassthrough("int", "FragmentID");
            sbPicking.addPassthrough("int", "RenderableID");

            // Helper functions to emit vertices from the given relative line coordinates
            std::string createVertexForward = "void createVertex(vec3 basePos, float ea, vec3 n, vec3 r, vec3 u, float relX, float relY) \n{\n";
            if (use_gAlpha)
                createVertexForward += "    gAlpha = vec2(relX, relY); // Relative line position for fragment shader\n";
            createVertexForward += R"(
    if (uExtrapolate)
    {
        if (relX < 0)
            passthroughMix01(relX * ea);
        if (relX > 1)
            passthroughMix01((relX - 1) * ea + 1);
    }
    if (relX > 0)
        relX -= 1; // This sets the factor for the r vector to the right value in case of the second half

    vec3 outPos = basePos + relX * r + relY * u;
    gl_Position = uProj * uView * vec4(outPos, 1.0);

    vOut.fragPosWS = outPos;
    vOut.Normal = n; // Set normal again as it might have been overwritten by passthrough
    EmitVertex();
}
            )";
            sbPicking.addGeometryShaderDecl(createVertexForward);

            std::string createVertexInverseForward
                = "void createVertexInverse(vec4 basePos, float ea, vec3 n, vec4 r, vec4 u, float relX, float relY) \n{\n";
            if (use_gAlpha)
                createVertexInverseForward += "    gAlpha = vec2(relX, relY);\n";
            createVertexInverseForward += R"(
    if (uExtrapolate)
    {
        if (relX < 0)
            passthroughMix01(relX * ea);
        if (relX > 1)
            passthroughMix01((relX - 1) * ea + 1);
    }
    if (relX > 0)
        relX -= 1;

    vec4 outPos = basePos + relX * r + relY * u;
    gl_Position = outPos;

    outPos = uInvProj * outPos;
    outPos /= outPos.w;
    outPos = uInvView * outPos;

    vOut.fragPosWS = vec3(outPos);
    vOut.Normal = n;
    EmitVertex();
}
            )";
            sbPicking.addGeometryShaderDecl(createVertexInverseForward);

            sbPicking.addVertexShaderCode(R"(
									vOut.FragmentID = aPickID;
									vOut.RenderableID = uRenderableID;
                                   )");

            sbPicking.addFragmentShaderCode(R"(							
                                  fPickIDs = ivec2((vIn.RenderableID), (vIn.FragmentID));
                                 )");

            mPickingShader = sbPicking.createProgram();
        }
    }
}
