#include "Handle.hh"

#include <typed-geometry/tg.hh>

#include <algorithm>

tg::quat glow::camera::Handle::forwardToRotation(const tg::vec3& forward, const tg::vec3& up)
{
    auto const fwd = tg::normalize(forward);

    tg::vec3 rightVector = tg::normalize(tg::cross(fwd, up));
    tg::vec3 upVector = tg::cross(rightVector, fwd);

    tg::mat3 rotMatrix;
    rotMatrix[0][0] = rightVector.x;
    rotMatrix[0][1] = upVector.x;
    rotMatrix[0][2] = -fwd.x;
    rotMatrix[1][0] = rightVector.y;
    rotMatrix[1][1] = upVector.y;
    rotMatrix[1][2] = -fwd.y;
    rotMatrix[2][0] = rightVector.z;
    rotMatrix[2][1] = upVector.z;
    rotMatrix[2][2] = -fwd.z;

    return tg::quat::from_rotation_matrix(rotMatrix);
}

glow::camera::Handle::Handle(const glow::transform& transform)
{
    mTarget.transform = transform;
    snap();
}

void glow::camera::Handle::update(float dt)
{
    // Always slerp forward
    auto alphaRot = std::min(1.f, glow::transform::exponentialDecayAlpha(sensitivity.rotation, dt));
    mPhysical.forward = tg::normalize(tg::lerp(mPhysical.forward, mTarget.forward, alphaRot));

    if (mSmoothingMode == SmoothingMode::FPS)
    {
        // Snap target
        mPhysical.target = mTarget.target;

        // Lerp position
        auto alphaPos = std::min(1.f, glow::transform::exponentialDecayAlpha(sensitivity.position, dt));
        mPhysical.transform.lerpToPosition(mTarget.transform.position, alphaPos);
    }
    else // SmoothingMode::Orbit
    {
        // Lerp target
        auto alphaTarget = std::min(1.f, glow::transform::exponentialDecayAlpha(sensitivity.target, dt));
        mPhysical.target = tg::lerp(mPhysical.target, mTarget.target, alphaTarget);

        // Calculate forward
        // mPhysical.forward = glm::normalize(mPhysical.target - mPhysical.transform.position);

        // Set position
        mPhysical.transform.position = mPhysical.target - mPhysical.forward * mPhysical.targetDistance;
        mTarget.transform.position = mPhysical.transform.position;
    }

    // Set rotation from forward vector
    mPhysical.transform.rotation = forwardToRotation(mPhysical.forward, mCameraUp);

    // Always lerp target distance
    auto alphaDist = std::min(1.f, glow::transform::exponentialDecayAlpha(sensitivity.distance, dt));
    mPhysical.targetDistance = tg::lerp(mPhysical.targetDistance, mTarget.targetDistance, alphaDist);

    TG_ASSERT(isValid());
}

void glow::camera::Handle::move(const tg::vec3& distance)
{
    auto const transposedRotation = tg::transpose(tg::mat3(mPhysical.transform.rotation));
    auto const delta = transposedRotation * distance;
    mTarget.transform.position += delta;
    mTarget.target += delta;

    mSmoothingMode = SmoothingMode::FPS;
    TG_ASSERT(isValid());
}

void glow::camera::Handle::setPosition(const tg::pos3& pos) { mTarget.transform.position = pos; }

void glow::camera::Handle::setLookAt(const tg::pos3& pos, const tg::pos3& target, const tg::vec3& up)
{
    mCameraUp = up;
    mTarget.transform.position = pos;
    mTarget.target = target;

    tg::vec3 forwardVector = target - pos;
    mTarget.targetDistance = tg::length(forwardVector);
    if (mTarget.targetDistance < .0001f) // in case target == position
    {
        mTarget.targetDistance = .0001f;
        forwardVector = tg::vec3(mTarget.targetDistance, 0, 0);
    }

    mTarget.forward = forwardVector / mTarget.targetDistance;
    TG_ASSERT(isValid());
}

void glow::camera::Handle::setTarget(const tg::pos3& target)
{
    mTarget.target = target;

    tg::vec3 forwardVector = target - mTarget.transform.position;
    mTarget.targetDistance = tg::length(forwardVector);

    if (mTarget.targetDistance < .0001f) // target ~= position
    {
        mTarget.targetDistance = .0001f;
        mTarget.forward = tg::vec3(1, 0, 0);
    }
    else
    {
        mTarget.forward = forwardVector / mTarget.targetDistance;
    }

    TG_ASSERT(isValid());
}

void glow::camera::Handle::setTargetDistance(float dist)
{
    mTarget.targetDistance = dist;
    mSmoothingMode = SmoothingMode::Orbit;
}

void glow::camera::Handle::addTargetDistance(float deltaDist)
{
    mTarget.targetDistance += deltaDist;
    mSmoothingMode = SmoothingMode::Orbit;
}

void glow::camera::Handle::lookAround(float dX, float dY)
{
    auto altitude = tg::atan2(mTarget.forward.y, length(tg::vec2(mTarget.forward.x, mTarget.forward.z)));
    auto azimuth = tg::atan2(mTarget.forward.z, mTarget.forward.x);

    azimuth += 1_rad * dX;
    altitude = tg::clamp(altitude - 1_rad * dY, -89_deg, 89_deg);

    auto caz = tg::cos(azimuth);
    auto saz = tg::sin(azimuth);
    auto cal = tg::cos(altitude);
    auto sal = tg::sin(altitude);

    mTarget.forward = tg::vec3(cal * caz, sal, cal * saz);
    mTarget.target = mTarget.transform.position + mTarget.forward * mTarget.targetDistance;

    mSmoothingMode = SmoothingMode::FPS;
    TG_ASSERT(isValid());
}

void glow::camera::Handle::orbit(float dX, float dY)
{
    auto azimuth = tg::atan2(mTarget.forward.z, mTarget.forward.x) + 1_rad * dX;
    auto altitude = tg::atan2(mTarget.forward.y, tg::sqrt(mTarget.forward.x * mTarget.forward.x + mTarget.forward.z * mTarget.forward.z)) - 1_rad * dY;
    altitude = tg::clamp(altitude, -89_deg, 89_deg);

    auto caz = tg::cos(azimuth);
    auto saz = tg::sin(azimuth);
    auto cal = tg::cos(altitude);
    auto sal = tg::sin(altitude);

    mTarget.forward = tg::vec3(cal * caz, sal, cal * saz);
    mTarget.transform.position = mTarget.target - mTarget.forward * mTarget.targetDistance;

    mSmoothingMode = SmoothingMode::Orbit;
    TG_ASSERT(isValid());
}

void glow::camera::Handle::translateFocus(const tg::pos3& target)
{
    auto const newPosition = target - mPhysical.forward * mPhysical.targetDistance;
    auto const posDelta = newPosition - mTarget.transform.position;
    mTarget.transform.position = newPosition;
    mTarget.target += posDelta;

    mSmoothingMode = SmoothingMode::FPS;
    TG_ASSERT(isValid());
}

bool glow::camera::Handle::isValid() const
{
    return all(is_finite(mTarget.target)) &&          //
           all(is_finite(mTarget.forward)) &&         //
           tg::is_finite(mTarget.targetDistance) &&   //
           all(is_finite(mPhysical.target)) &&        //
           all(is_finite(mPhysical.forward)) &&       //
           tg::is_finite(mPhysical.targetDistance) && //
           all(is_finite(mCameraUp));
}

tg::mat4 glow::camera::Handle::getViewMatrix() const
{
    auto const& position = mPhysical.transform.position;
    tg::mat4 viewMatrix = tg::mat4(mPhysical.transform.rotation);
    viewMatrix[3][0] = -(viewMatrix[0][0] * position.x + viewMatrix[1][0] * position.y + viewMatrix[2][0] * position.z);
    viewMatrix[3][1] = -(viewMatrix[0][1] * position.x + viewMatrix[1][1] * position.y + viewMatrix[2][1] * position.z);
    viewMatrix[3][2] = -(viewMatrix[0][2] * position.x + viewMatrix[1][2] * position.y + viewMatrix[2][2] * position.z);

    return viewMatrix;
}
