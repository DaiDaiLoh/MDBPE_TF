#include "GenericCamera.hh"

#include <glow/common/log.hh>

#include <typed-geometry/feature/assert.hh>
#include <typed-geometry/tg.hh>

namespace
{
bool isApproxEqual(const tg::mat4& _v1, const tg::mat4& _v2, float _eps = .01f)
{
    tg::mat4 diff = _v1 - _v2;
    float d = 0;
    d += tg::abs(diff[0][0]);
    d += tg::abs(diff[0][1]);
    d += tg::abs(diff[0][2]);
    d += tg::abs(diff[0][3]);
    d += tg::abs(diff[1][0]);
    d += tg::abs(diff[1][1]);
    d += tg::abs(diff[1][2]);
    d += tg::abs(diff[1][3]);
    d += tg::abs(diff[2][0]);
    d += tg::abs(diff[2][1]);
    d += tg::abs(diff[2][2]);
    d += tg::abs(diff[2][3]);
    d += tg::abs(diff[3][0]);
    d += tg::abs(diff[3][1]);
    d += tg::abs(diff[3][2]);
    d += tg::abs(diff[3][3]);
    return d < _eps;
}

bool isApproxEqual(const tg::mat3& _v1, const tg::mat3& _v2, float _eps = .01f)
{
    tg::mat3 diff = _v1 - _v2;
    float d = 0;
    d += tg::abs(diff[0][0]);
    d += tg::abs(diff[0][1]);
    d += tg::abs(diff[0][2]);
    d += tg::abs(diff[1][0]);
    d += tg::abs(diff[1][1]);
    d += tg::abs(diff[1][2]);
    d += tg::abs(diff[2][0]);
    d += tg::abs(diff[2][1]);
    d += tg::abs(diff[2][2]);
    return d < _eps;
}

bool isOrthonormalMatrix(const tg::mat3& _matrix) { return isApproxEqual(tg::inverse(_matrix), tg::transpose(_matrix)); }
}

using namespace glow::camera;
using namespace std;

GenericCamera::GenericCamera() { setRotationMatrix(tg::mat3::identity); }

GenericCamera::GenericCamera(const std::string& _state) { setStateFromString(_state); }

void GenericCamera::FPSstyleLookAround(float _deltaX, float _deltaY)
{
    auto fwd = getForwardDirection();

    auto altitude = tg::atan2(fwd.y, length(tg::vec2(fwd.x, fwd.z)));
    auto azimuth = tg::atan2(fwd.z, fwd.x);

    azimuth += 1_rad * _deltaX;
    altitude = tg::clamp(altitude - 1_rad * _deltaY, -89_deg, 89_deg);

    auto caz = tg::cos(azimuth);
    auto saz = tg::sin(azimuth);
    auto cal = tg::cos(altitude);
    auto sal = tg::sin(altitude);

    fwd = tg::dir3( //
        cal * caz,  //
        sal,        //
        cal * saz   //
    );
    auto right = normalize(cross(fwd, tg::dir3(0, 1, 0)));
    auto up = cross(right, fwd);

    setRotationMatrix(transpose(tg::mat3::from_cols(right, up, -fwd)));
}

void GenericCamera::rotateAroundTarget_GlobalAxes(tg::angle _x, tg::angle _y, tg::angle _z)
{
    // move camera so, that the target is the center, then rotate around the
    // global coordinate system

    tg::mat3 t = tg::mat3::identity;

    rotateAroundTarget_helper(_x, _y, _z, t);
}

void GenericCamera::rotateAroundTarget_LocalAxes(tg::angle _x, tg::angle _y, tg::angle _z)
{
    tg::mat3 R = getRotationMatrix3();
    R = tg::transpose(R);

    rotateAroundTarget_helper(_x, _y, _z, R);
}

void GenericCamera::rotateAroundTarget_helper(tg::angle _x, tg::angle _y, tg::angle _z, const tg::mat3& _rotationAxes)
{
    tg::vec4 T = tg::vec4(getTarget(), 1.0f);
    tg::vec4 P = tg::vec4(getPosition(), 1.0f);

    tg::vec4 tempPos = P - T;
    tg::mat4 newRotation = tg::rotation_around(_x, tg::dir3(_rotationAxes[0]));
    newRotation *= tg::rotation_around(_y, tg::dir3(_rotationAxes[1]));
    newRotation *= tg::rotation_around(_z, tg::dir3(_rotationAxes[2]));

    tempPos = newRotation * tempPos;

    P = tempPos + T; // new position
    tg::vec4 N = tg::vec4(getUpDirection(), 1.0f);
    N = newRotation * N;

    setLookAtMatrix(tg::pos3(P), tg::pos3(T), tg::vec3(N));
}


void GenericCamera::setHorizontalFieldOfView(tg::horizontal_fov _fovh)
{
    TG_ASSERT(_fovh.angle < 180_deg);
    TG_ASSERT(_fovh.angle > 0_deg);
    mHorizontalFieldOfView = _fovh;
}

void GenericCamera::setVerticalFieldOfView(tg::angle _fovv)
{
    TG_ASSERT(_fovv < 180_deg);
    TG_ASSERT(_fovv > 0_deg);

    // we only save the aspectRatio and the horizontal FoV
    // so if we change the vertical FoV, we change the aspectRatio

    // mAspectRatio = tan( glm::radians(0.5f * mHorizontalFieldOfView) ) / tan( glm::radians(0.5f * _fovv) );

    float x = tan(0.5f * _fovv) * mAspectRatio;
    mHorizontalFieldOfView = tg::horizontal_fov(2.0f * tg::atan(x));
}

tg::angle GenericCamera::getVerticalFieldOfView() const { return tg::atan(tan(0.5f * mHorizontalFieldOfView.angle) / mAspectRatio) * 2.0f; }

void GenericCamera::setNearClippingPlane(float _plane)
{
    TG_ASSERT(_plane > 0.0f);
    mNearClippingPlane = _plane;
}

void GenericCamera::setFarClippingPlane(float _plane)
{
    TG_ASSERT(_plane > 0.0f);
    mFarClippingPlane = _plane;
}

tg::mat4 GenericCamera::getViewMatrix() const
{
    if (mStereoMode == Mono)
    {
        return getMonoViewMatrix();
    }
    else
    {
        // all kinds of stereo
        bool eyeIsLeftEye = (getEye() == EyeLeft);
        return getStereoViewMatrix(eyeIsLeftEye, mStereoMode);
    }
}

tg::mat4 GenericCamera::getStereoViewMatrix(bool _leftEye, StereoMode _stereoMode) const
{
    // The view matrix is independent of the projection mode (isometric or perspective)
    // so only the stereo mode has to be checked.
    TG_ASSERT(_stereoMode != Mono && "mono is not a stereo mode!");

    float cameraPositionShiftValue = (mInterpupillaryDistance * 0.5f); // shift to the right
    if (_leftEye)
        cameraPositionShiftValue *= -1.0f; // if left eye shift to the left

    if ((_stereoMode == ParallelShift) || (_stereoMode == OffAxis))
    {
        //
        // parallel shift and off-axis have the same view matrices:
        // just shift the camera position to the left/right by half the eye-distance
        //

        // ACGL::Utils::debug() << "WARNING: getStereoViewMatrix() is not tested yet" << std::endl; // remove after
        // testing

        tg::mat3 inverseRotation = getInverseRotationMatrix3();
        tg::pos3 eyePosition = mPosition + (inverseRotation * tg::vec3(cameraPositionShiftValue, 0.0f, 0.0f));

        auto m = tg::mat4(mRotationMatrix);
        m[3][0] = -(m[0][0] * eyePosition.x + m[1][0] * eyePosition.y + m[2][0] * eyePosition.z);
        m[3][1] = -(m[0][1] * eyePosition.x + m[1][1] * eyePosition.y + m[2][1] * eyePosition.z);
        m[3][2] = -(m[0][2] * eyePosition.x + m[1][2] * eyePosition.y + m[2][2] * eyePosition.z);
        return m;
    }

    // else it has to be toe-in:
    TG_ASSERT(_stereoMode == ToeIn && "unsupported stereo mode!");
    //
    // Toe-in: shift the camera position to the left/right by half the eye-distance and
    //         rotate a bit inwards so that the two cameras focus the same point
    //         at the look-at distance (focal point)

    TG_ASSERT(0 && "getStereoViewMatrix() for TOE_IN is not implemented yet!");
    return tg::mat4();
}

tg::mat4 GenericCamera::getInverseViewMatrix() const
{
    if (mStereoMode == Mono)
    {
        return getMonoInverseViewMatrix();
    }

    tg::mat4 viewMatrix = getViewMatrix();
    return tg::inverse(viewMatrix);
}

tg::mat4 GenericCamera::getProjectionMatrix() const
{
    if (mStereoMode == Mono)
    {
        return getMonoProjectionMatrix();
    }
    else
    {
        // all kinds of stereo
        bool eyeIsLeftEye = (getEye() == EyeLeft);
        return getStereoProjectionMatrix(eyeIsLeftEye, mStereoMode);
    }
}

tg::mat4 GenericCamera::getMonoProjectionMatrix() const
{
    tg::mat4 projectionMatrix; // identity matrix

    if (getProjectionMode() == IsometricProjection)
    {
        // we don't set the left/right/top/bottom values explicitly, so we want that
        // all object at our focal distance appear the same in perspective and isometric view
        float right = tan(getHorizontalFieldOfView().angle * 0.5f) * mLookAtDistance;
        float left = -right;
        float top = tan(getVerticalFieldOfView() * 0.5f) * mLookAtDistance;
        float bottom = -top;

        // we do the same here as a glOrtho call would do.
        projectionMatrix[0][0] = 2.0f / (right - left);
        projectionMatrix[1][1] = 2.0f / (top - bottom);
        projectionMatrix[2][2] = -2.0f / (mFarClippingPlane - mNearClippingPlane);
        projectionMatrix[0][3] = -(right + left) / (right - left);
        projectionMatrix[1][3] = -(top + bottom) / (top - bottom);
        projectionMatrix[2][3] = -(mFarClippingPlane + mNearClippingPlane) / (mFarClippingPlane - mNearClippingPlane);
        projectionMatrix[3][3] = 1.0;
    }
    else if (mProjectionMode == PerspectiveProjectionOpenGL)
    {
        if (std::isinf(mFarClippingPlane))
        {
            float e = 1.0f / tan(getVerticalFieldOfView() * 0.5f);
            const float a = getAspectRatio();

            // infinite Perspective matrix reversed mapping to 1..-1
            projectionMatrix = tg::mat4::from_cols({e / a, 0.0f, 0.0f, 0.0f},  //
                                                   {0.0f, e, 0.0f, 0.0f},      //
                                                   {0.0f, 0.0f, -1.0f, -1.0f}, //
                                                   {0.0f, 0.0f, -2.0f * mNearClippingPlane, 0.0f});
        }
        else
        {
            projectionMatrix = tg::perspective_opengl(getHorizontalFieldOfView(), getAspectRatio(), mNearClippingPlane, mFarClippingPlane);
        }
    }
    else if (mProjectionMode == PerspectiveProjectionDXReverse)
    {
        if (std::isinf(mFarClippingPlane))
        {
            float e = 1.0f / tan(getVerticalFieldOfView() * 0.5f);
            const float a = getAspectRatio();

            // infinite Perspective matrix reversed mapping to 1..0
            projectionMatrix = tg::mat4::from_cols({e / a, 0.0f, 0.0f, 0.0f}, //
                                                   {0.0f, e, 0.0f, 0.0f},     //
                                                   {0.0f, 0.0f, 0.0f, -1.0f}, //
                                                   {0.0f, 0.0f, mNearClippingPlane, 0.0f});
        }
        else
        {
            TG_ASSERT(0 && "unsupported projection mode");
        }
    }

    else
        TG_ASSERT(0 && "unsupported projection mode");

    return projectionMatrix;
}

tg::mat4 GenericCamera::getMonoViewMatrix() const
{
    tg::mat4 m(mRotationMatrix);
    m[3][0] = -(m[0][0] * mPosition.x + m[1][0] * mPosition.y + m[2][0] * mPosition.z);
    m[3][1] = -(m[0][1] * mPosition.x + m[1][1] * mPosition.y + m[2][1] * mPosition.z);
    m[3][2] = -(m[0][2] * mPosition.x + m[1][2] * mPosition.y + m[2][2] * mPosition.z);
    // TG_ASSERT(isApproxEqual(getRotationMatrix4() * getTranslationMatrix4(), m, mFarClippingPlane * 1e-5f));
    return m;
}

tg::mat4 GenericCamera::getMonoInverseViewMatrix() const
{
    tg::mat4 m(tg::transpose(mRotationMatrix));
    m[3][0] = mPosition.x;
    m[3][1] = mPosition.y;
    m[3][2] = mPosition.z;
    // TG_ASSERT(isApproxEqual(glm::inverse(getViewMatrix()), m));
    return m;
}

tg::mat4 GenericCamera::getStereoProjectionMatrix(bool _leftEye, StereoMode _stereoMode) const
{
    TG_ASSERT(_stereoMode != Mono && "mono is not a stereo mode!");

    if (getProjectionMode() == IsometricProjection)
    {
        // very unusual, prepare for headaches!
        return getMonoProjectionMatrix();
    }

    if ((_stereoMode == ParallelShift) || (_stereoMode == ToeIn))
    {
        // the view matrix changes but the projection matrix stays the same
        return getMonoProjectionMatrix();
    }

    // so off-axis it is!
    TG_ASSERT(_stereoMode == OffAxis && "unknown projection mode!");
    //
    // In this mode the camera positions (view matrix) is shifted to the left/right still looking
    // straight ahead. The projection is also looking ahead but the projection center is
    // off (hence off-axis).
    // There is one plane in front of the cameras where the view-frusta match.
    // This should be the distance to the physical screen from the users position.


    TG_ASSERT(0 && "getStereoViewMatrix() is not implemented for OFF_AXIS yet!");
    return tg::mat4();
}


/// Writes all internal state to one string
/// Elements are seperated by pipes ('|'), spaces can get ignored.
std::string GenericCamera::storeStateToString() const
{
    error() << "Not implemented";
    return "";
    /*std::string state;

    state = "ACGL_GenericCamera | "; // "magic number", for every version the same
    state += "1 | ";                 // version, always an integer

    state += toString(mPosition) + " | ";
    state += toString(mRotationMatrix) + " | ";
    if (mProjectionMode == ISOMETRIC_PROJECTION)
        state += "ISOMETRIC_PROJECTION | ";
    if (mProjectionMode == PERSPECTIVE_PROJECTION_OPENGL)
        state += "PERSPECTIVE_PROJECTION | ";
    if (mStereoMode == MONO)
        state += "MONO | ";
    if (mStereoMode == PARALLEL_SHIFT)
        state += "PARALLEL_SHIFT | ";
    if (mStereoMode == OFF_AXIS)
        state += "OFF_AXIS | ";
    if (mStereoMode == TOE_IN)
        state += "TOE_IN | ";
    if (mCurrentEye == EYE_LEFT)
        state += "EYE_LEFT | ";
    if (mCurrentEye == EYE_RIGHT)
        state += "EYE_RIGHT | ";
    state += toString(mHorizontalFieldOfView) + " | ";
    state += toString(mAspectRatio) + " | ";
    state += toString(mInterpupillaryDistance) + " | ";
    state += toString(mNearClippingPlane) + " | ";
    state += toString(mFarClippingPlane) + " | ";
    state += toString(mLookAtDistance) + " | ";
    state += toString(mViewportSize);

    return state;*/
}

/// Sets all internal state from a string
void GenericCamera::setStateFromString(const std::string& _state)
{
    error() << "Not implemented";
    /*vector<string> token = split(_state, '|');
    for (size_t i = 0; i < token.size(); i++)
    {
        token[i] = stripOfWhiteSpaces(token[i]);
    }
    if ((token.size() < 14) || (token[0] != "ACGL_GenericCamera"))
    {
        ACGL::Utils::error() << "Generic camera state string is invalid: " << _state << std::endl;
        return;
    }
    if (to<int>(token[1]) != 1)
    {
        ACGL::Utils::error() << "Generic camera state version not supported: " << to<int>(token[1]) << std::endl;
        return;
    }

    int pos = 2;
    mPosition = toVec3(token[pos++]);
    mRotationMatrix = toMat3(token[pos++]);
    if (token[pos] == "ISOMETRIC_PROJECTION")
        mProjectionMode = ISOMETRIC_PROJECTION;
    if (token[pos] == "PERSPECTIVE_PROJECTION")
        mProjectionMode = PERSPECTIVE_PROJECTION_OPENGL;
    pos++;
    if (token[pos] == "MONO")
        mStereoMode = MONO;
    if (token[pos] == "PARALLEL_SHIFT")
        mStereoMode = PARALLEL_SHIFT;
    if (token[pos] == "OFF_AXIS")
        mStereoMode = OFF_AXIS;
    if (token[pos] == "TOE_IN")
        mStereoMode = TOE_IN;
    pos++;
    if (token[pos] == "EYE_LEFT")
        mCurrentEye = EYE_LEFT;
    if (token[pos] == "EYE_RIGHT")
        mCurrentEye = EYE_RIGHT;
    pos++;

    mHorizontalFieldOfView = to<float>(token[pos++]);
    mAspectRatio = to<float>(token[pos++]);
    mInterpupillaryDistance = to<float>(token[pos++]);
    mNearClippingPlane = to<float>(token[pos++]);
    mFarClippingPlane = to<float>(token[pos++]);
    mLookAtDistance = to<float>(token[pos++]);
    mViewportSize = toUvec2(token[pos++]);*/
}

float GenericCamera::getFocalLenghtInPixel() const { return ((float)mViewportSize.width) / (2.0f * tan(0.5f * mHorizontalFieldOfView.angle)); }

void GenericCamera::setFocalLengthInPixel(float _focalLengthInPixel)
{
    auto hFoVrad = 2.0f * tg::atan((0.5f * mViewportSize.width) * (1.0f / _focalLengthInPixel));
    setHorizontalFieldOfView(tg::horizontal_fov(hFoVrad));
}


void GenericCamera::setRotationMatrix(tg::mat3 _matrix)
{
    mRotationMatrix = _matrix;
    TG_ASSERT(isOrthonormalMatrix(mRotationMatrix));
}

void GenericCamera::setRotationMatrix(tg::mat4 _matrix)
{
    mRotationMatrix = tg::mat3(_matrix);
    TG_ASSERT(isOrthonormalMatrix(mRotationMatrix));
}

void GenericCamera::setLookAtMatrix(const tg::pos3& _position, const tg::pos3& _target, const tg::vec3& _up)
{
    if (distance_sqr(_position, _target) > mFarClippingPlane * mFarClippingPlane)
        glow::warning() << "Target is farther away than far clipping plane";

    setPosition(_position);
    setTarget(_target, _up);
}

tg::mat4 GenericCamera::getTranslationMatrix4() const
{
    tg::mat4 trans;
    trans[3][0] = -mPosition.x;
    trans[3][1] = -mPosition.y;
    trans[3][2] = -mPosition.z;
    return trans;
}

tg::dir3 GenericCamera::getUpDirection() const
{
    tg::dir3 up(mRotationMatrix[0][1], mRotationMatrix[1][1], mRotationMatrix[2][1]);
    return up;
}
tg::dir3 GenericCamera::getRightDirection() const
{
    tg::dir3 right(mRotationMatrix[0][0], mRotationMatrix[1][0], mRotationMatrix[2][0]);
    return right;
}
tg::dir3 GenericCamera::getForwardDirection() const
{
    tg::dir3 forward(-mRotationMatrix[0][2], -mRotationMatrix[1][2], -mRotationMatrix[2][2]);
    return forward;
}

void GenericCamera::getViewRay(tg::pos2 mousePosition, tg::pos3* pos, tg::dir3* dir) const
{
    tg::vec3 ps[2];
    auto i = 0;
    for (auto d : {0.5f, -0.5f})
    {
        tg::vec4 v{mousePosition.x * 2 - 1, 1 - mousePosition.y * 2, d * 2 - 1, 1.0};

        v = tg::inverse(getProjectionMatrix()) * v;
        v /= v.w;
        v = tg::inverse(getViewMatrix()) * v;
        ps[i++] = tg::vec3(v);
    }

    if (pos)
        *pos = getPosition();
    if (dir)
        *dir = normalize(ps[0] - ps[1]);
}

void GenericCamera::setTarget(const tg::pos3& _target, const tg::vec3& _up)
{
    auto forwardVector = _target - mPosition;
    mLookAtDistance = length(forwardVector);
    if (mLookAtDistance < .0001f) // in case target == mPosition
    {
        mLookAtDistance = .0001f;
        forwardVector = tg::vec3(mLookAtDistance, 0, 0);
    }

    forwardVector = forwardVector / mLookAtDistance; // normalize
    tg::vec3 rightVector = tg::normalize(tg::cross(forwardVector, _up));
    tg::vec3 upVector = tg::cross(rightVector, forwardVector);

    tg::mat3 rotMatrix;
    rotMatrix[0][0] = rightVector.x;
    rotMatrix[0][1] = upVector.x;
    rotMatrix[0][2] = -forwardVector.x;
    rotMatrix[1][0] = rightVector.y;
    rotMatrix[1][1] = upVector.y;
    rotMatrix[1][2] = -forwardVector.y;
    rotMatrix[2][0] = rightVector.z;
    rotMatrix[2][1] = upVector.z;
    rotMatrix[2][2] = -forwardVector.z;

    setRotationMatrix(rotMatrix);
}

void GenericCamera::setLookAtDistance(float _distance)
{
    TG_ASSERT(_distance > 0.0f);
    mLookAtDistance = _distance;
}

tg::mat4 GenericCamera::getModelMatrix() const
{
    tg::mat4 m(mRotationMatrix);
    m[3][0] = -(m[0][0] * mPosition.x + m[1][0] * mPosition.y + m[2][0] * mPosition.z);
    m[3][1] = -(m[0][1] * mPosition.x + m[1][1] * mPosition.y + m[2][1] * mPosition.z);
    m[3][2] = -(m[0][2] * mPosition.x + m[1][2] * mPosition.y + m[2][2] * mPosition.z);
    TG_ASSERT(isApproxEqual(getRotationMatrix4() * getTranslationMatrix4(), m));
    return m;
}


tg::mat4 GenericCamera::getInverseModelMatrix() const
{
    tg::mat4 modelMatrix = getModelMatrix();
    return tg::inverse(modelMatrix);
}


void GenericCamera::move(const tg::vec3& _vector)
{
    tg::mat3 inverseRotation = getInverseRotationMatrix3();
    mPosition += (inverseRotation * _vector);
}
