#include "CameraController.hh"

#include <glow/common/log.hh>
#include <glow/objects/Framebuffer.hh>

#include <typed-geometry/tg.hh>

namespace
{
tg::vec3 fwd_from_azi_alti(tg::angle azi, tg::angle alt)
{
    auto caz = cos(-azi);
    auto saz = sin(-azi);
    auto cal = cos(-alt);
    auto sal = sin(-alt);
    return tg::vec3(caz * cal, //
                    sal,       //
                    saz * cal);
}

tg::angle azi_from_fwd(tg::vec3 fwd) { return angle_between(tg::vec2(fwd.x, fwd.z), tg::vec2::unit_x) * tg::sign(-fwd.z); }
tg::angle alti_from_fwd(tg::vec3 fwd) { return -tg::asin(tg::dir(fwd).y); }
}


namespace glow::viewer
{
void CameraController::setupMesh(float size, tg::pos3 center)
{
    mMeshSize = size;
    mMeshCenter = center;

    // Originally, glow-viewer used a default FoV of 60° and a camera distance of 1 times mMeshSize.
    // We take these settings as a base value to maintain visual continuity for scenes that use the default camera FoV.
    const float legacyDistanceFactor = tg::tan(0.5f * 60_deg);

    // Camera distance depends on mesh size and the actual camera FoV.
    s.DefaultTargetDistance = mMeshSize * legacyDistanceFactor / tg::tan(0.5f * s.HorizontalFov.angle);

    s.TargetMinDistance = s.DefaultTargetDistance / 10000.f;
    s.MoveSpeedStart = s.DefaultTargetDistance;
    s.MoveSpeedMin = s.DefaultTargetDistance / 1000.f;
    s.MoveSpeedMax = s.DefaultTargetDistance * 1000.f;
    s.FocusRefDistance = s.DefaultTargetDistance / 3.f;

    // Near plane must not be zero
    auto constexpr floatEps = 1e-12f;
    static_assert(floatEps > 0.f, "Float epsilon too small");
    s.NearPlane = tg::max(s.DefaultTargetDistance / 100000.f, floatEps);
    s.FarPlane = 2.f * s.DefaultTargetDistance; // only used if no reverse z

    resetView();
}

void CameraController::resetView()
{
    mTargetPos = mMeshCenter;
    mTargetDistance = s.DefaultTargetDistance;
    mAltitude = 30_deg;
    mAzimuth = 45_deg;
    mFwd = fwd_from_azi_alti(mAzimuth, mAltitude);
    mUp = {0, 1, 0};
    mRight = normalize(cross(mFwd, mUp));
    mPos = mTargetPos - mFwd * mTargetDistance;
    mMode = Mode::Targeted;
    mMoveSpeed = s.MoveSpeedStart;
}

tg::pos3 CameraController::getPosition() const { return (mSmoothedTargetPos - mSmoothedFwd * mSmoothedTargetDistance); }

tg::mat4 CameraController::computeViewMatrix() const
{
    // tg::look_at crashes if position and target are too close, as floating precision ruins the normalize
    if (TG_LIKELY(tg::distance_sqr(mSmoothedPos, mSmoothedTargetPos) > 1e-36f))
        return tg::look_at_opengl(mSmoothedPos, mSmoothedTargetPos, mSmoothedUp);
    else
        return tg::look_at_opengl(mSmoothedPos, tg::dir3(0, 0, -1), tg::vec3(0, 1, 0));
}

tg::mat4 CameraController::computeProjMatrix() const
{
    if (mOrthographicModeEnabled)
    {
        auto const aspect = float(mWindowWidth) / mWindowHeight;
        auto bounds = mOrthographicBounds;
        if (aspect < 1.0f)
        {
            bounds.min.y /= aspect;
            bounds.max.y /= aspect;
        }
        else
        {
            bounds.min.x *= aspect;
            bounds.max.x *= aspect;
        }
        if (mReverseZEnabled)
            return tg::orthographic_reverse_z(bounds.min.x, bounds.max.x, bounds.min.y, bounds.max.y, bounds.min.z, bounds.max.z);
        else
            return tg::orthographic(bounds.min.x, bounds.max.x, bounds.min.y, bounds.max.y, bounds.min.z, bounds.max.z);
    }
    else if (mReverseZEnabled)
        return tg::perspective_reverse_z_opengl(s.HorizontalFov, mWindowWidth / float(mWindowHeight), s.NearPlane);
    else
    {
        auto np = tg::max(s.NearPlane, s.FarPlane / 4e3f);
        return tg::perspective_opengl(s.HorizontalFov, mWindowWidth / float(mWindowHeight), np, s.FarPlane);
    }
}

CameraController::CameraController() { resetView(); }

void CameraController::clipCamera()
{
    mSmoothedTargetPos = mTargetPos;
    mSmoothedTargetDistance = mTargetDistance;

    mSmoothedUp = mUp;
    mSmoothedRight = mRight;
    mSmoothedFwd = mFwd;
}

void CameraController::focusOnSelectedPoint(int x, int y, glow::SharedFramebuffer const& framebuffer)
{
    if (x < 0 || x >= mWindowWidth)
        return;
    if (y < 0 || y >= mWindowHeight)
        return;

    // read back depth

    auto proj = computeProjMatrix();
    auto view = computeViewMatrix();

    float d;
    {
        auto fb = framebuffer->bind();
        glReadPixels(x, y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &d);
    }

    if ((mReverseZEnabled && d == 0.0f) || (!mReverseZEnabled && tg::abs(d - 1.0f) < 0.001f))
        return; // background (reverse Z -> depth at 0 means far)

    tg::pos3 posNdc;
    posNdc.x = x / (mWindowWidth - 1.0f) * 2.0f - 1.0f;
    posNdc.y = y / (mWindowHeight - 1.0f) * 2.0f - 1.0f;

    if (mReverseZEnabled)
        posNdc.z = d;
    else
        posNdc.z = 2.0f * d - 1.0f;

    // screen to view
    auto posView = inverse(proj) * posNdc;

    // view to world
    auto posWorld = inverse(view) * posView;

    focusOnSelectedPoint(posWorld);
}

void CameraController::focusOnSelectedPoint(tg::pos3 pos)
{
    // zoom in
    auto dir = pos - mPos;
    auto dis = length(dir);
    if (dis > s.FocusRefDistance * 2) // too far? clip to focus dis
        dis = s.FocusRefDistance;
    else
        dis /= 2; // otherwise half distance

    mTargetPos = pos;

    // reuse old target distance
    mTargetDistance = tg::clamp(dis, s.TargetMinDistance, mTargetDistance);

    mMode = Mode::Targeted;
}

void CameraController::changeCameraSpeed(int delta)
{
    mMoveSpeed *= tg::pow(s.MoveSpeedFactor, delta);
    mMoveSpeed = tg::clamp(mMoveSpeed, s.MoveSpeedMin, s.MoveSpeedMax);

    glow::info() << "Set camera speed to " << mMoveSpeed / 1000 << " km/s";
}

void CameraController::resize(int w, int h)
{
    mWindowWidth = w;
    mWindowHeight = h;
}

void CameraController::zoom(float delta)
{
    float factor = tg::pow(s.ZoomFactor, -delta);

    mTargetDistance *= factor;
    mTargetDistance = tg::max(s.TargetMinDistance, mTargetDistance);

    mMode = Mode::Targeted;
}

void CameraController::fpsStyleLookaround(float relDx, float relDy)
{
    auto angleX = -relDx * s.HorizontalSensitivity;
    auto angleY = relDy * s.VerticalSensitivity;

    if (s.InvertHorizontal)
        angleX *= -1.0f;
    if (s.InvertVertical)
        angleY *= -1.0f;

    mAzimuth += angleX;
    mAltitude += angleY;

    mAltitude = clamp(mAltitude, -89_deg, 89_deg);

    auto pos = mPos;

    mFwd = fwd_from_azi_alti(mAzimuth, mAltitude);
    mRight = normalize(cross(mFwd, mRefUp));
    mUp = normalize(cross(mRight, mFwd));
    mTargetPos = pos + mFwd * mTargetDistance;

    mMode = Mode::FPS;
    mTargetDistance = s.FocusRefDistance;
}

void CameraController::targetLookaround(float relDx, float relDy)
{
    auto angleX = -relDx * s.HorizontalSensitivity * 2;
    auto angleY = relDy * s.VerticalSensitivity * 2;

    if (s.InvertHorizontal)
        angleX *= -1.0f;
    if (s.InvertVertical)
        angleY *= -1.0f;

    mAzimuth += angleX;
    mAltitude += angleY;

    mAltitude = clamp(mAltitude, -89_deg, 89_deg);

    mFwd = fwd_from_azi_alti(mAzimuth, mAltitude);
    mRight = normalize(cross(mFwd, mRefUp));
    mUp = normalize(cross(mRight, mFwd));

    mMode = Mode::Targeted;
}

void CameraController::moveCamera(float dRight, float dFwd, float dUp, float dUpAbsolute, float elapsedSeconds)
{
    if (dRight == 0 && dUp == 0 && dFwd == 0 && dUpAbsolute == 0)
        return;

    auto speed = mMoveSpeed * elapsedSeconds;

    mPos += mRight * speed * dRight;
    mPos += mUp * speed * dUp;
    mPos += tg::vec3{0.f, speed * dUpAbsolute, 0.f};
    mPos += mFwd * speed * dFwd;

    mMode = Mode::FPS;
    mTargetDistance = s.FocusRefDistance;
}

void CameraController::rotate(float unitsRight, float unitsUp)
{
    auto rx = unitsRight * s.NumpadRotateDegree;
    auto ry = unitsUp * s.NumpadRotateDegree;

    if (mMode == Mode::FPS)
    {
        rx = -rx;
        ry = -ry;
    }

    auto rot = tg::rotation_around(rx, normalize(mUp)) * tg::rotation_around(-ry, normalize(mRight));

    mFwd = rot * mFwd;
    mRight = normalize(cross(mFwd, mRefUp));
    mUp = normalize(cross(mRight, mFwd));
    mAzimuth = azi_from_fwd(mFwd);
    mAltitude = alti_from_fwd(mFwd);
}

void CameraController::setOrbit(tg::vec3 dir, tg::vec3 up)
{
    mTargetPos = mMeshCenter;
    mTargetDistance = s.DefaultTargetDistance;
    mFwd = -normalize(dir);
    mRight = normalize(cross(mFwd, up));
    mUp = cross(mRight, mFwd);
    mAzimuth = azi_from_fwd(mFwd);
    mAltitude = alti_from_fwd(mFwd);
    mMode = Mode::Targeted;
}

void CameraController::setOrbit(tg::angle azimuth, tg::angle altitude, float distance)
{
    mTargetPos = mMeshCenter;
    mTargetDistance = distance > 0 ? distance : s.DefaultTargetDistance;
    mAltitude = altitude;
    mAzimuth = azimuth;
    mFwd = fwd_from_azi_alti(mAzimuth, mAltitude);
    mUp = {0, 1, 0};
    mRight = normalize(cross(mFwd, mUp));
    mMode = Mode::Targeted;
}

void CameraController::setTransform(tg::pos3 position, tg::pos3 target)
{
    auto const dir = position - target;
    mTargetPos = target;
    mPos = position;
    mTargetDistance = tg::length(dir);
    mFwd = -normalize(dir);
    auto r = cross(mFwd, tg::vec3::unit_y);
    if (tg::is_zero_vector(r)) // looking up or down
        r = tg::vec3::unit_x;
    mRight = normalize(r);
    mUp = cross(mRight, mFwd);
    mAzimuth = azi_from_fwd(mFwd);
    mAltitude = alti_from_fwd(mFwd);
    mMode = Mode::Targeted;
}

void CameraController::enableOrthographicMode(tg::aabb3 bounds)
{
    mOrthographicModeEnabled = true;
    mOrthographicBounds = bounds;
}

void CameraController::update(float elapsedSeconds)
{
    // update camera view system
    mRight = normalize(cross(mFwd, mUp));
    mUp = normalize(cross(mRight, mFwd));

    TG_ASSERT(s.TranslationalSmoothingHalfTimeMillis >= 0);
    TG_ASSERT(s.RotationalSmoothingHalfTimeMillis >= 0);

    // smoothing
    auto alphaTranslational = 0.f;
    auto alphaRotational = 0.f;
    if (s.TranslationalSmoothingHalfTimeMillis > 0)
        alphaTranslational = tg::pow(0.5f, 1000 * elapsedSeconds / s.TranslationalSmoothingHalfTimeMillis);
    if (s.RotationalSmoothingHalfTimeMillis > 0)
        alphaRotational = tg::pow(0.5f, 1000 * elapsedSeconds / s.RotationalSmoothingHalfTimeMillis);

    mSmoothedTargetDistance = tg::mix(mTargetDistance, mSmoothedTargetDistance, alphaTranslational);
    mSmoothedTargetPos = mix(mTargetPos, mSmoothedTargetPos, alphaTranslational);
    mSmoothedPos = mix(mPos, mSmoothedPos, alphaTranslational);
    mSmoothedFwd = normalize(mix(mFwd, mSmoothedFwd, alphaRotational));
    mSmoothedRight = normalize(mix(mRight, mSmoothedRight, alphaRotational));
    mSmoothedUp = normalize(mix(mUp, mSmoothedUp, alphaRotational));

    // update derived properties
    switch (mMode)
    {
    case Mode::FPS:
        mTargetPos = mPos + mFwd * mTargetDistance;
        mSmoothedTargetPos = mSmoothedPos + mSmoothedFwd * mSmoothedTargetDistance;
        break;
    case Mode::Targeted:
        mPos = mTargetPos - mFwd * mTargetDistance;
        mSmoothedPos = mSmoothedTargetPos - mSmoothedFwd * mSmoothedTargetDistance;
        break;
    }

    // failsafe
    auto camOk = true;
    // TODO: check infs
    if (!camOk)
        clipCamera();
}
}
