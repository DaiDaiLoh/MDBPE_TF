#pragma once

#include <iosfwd>
#include <string>

#include <typed-geometry/feature/quat.hh>
#include <typed-geometry/tg.hh>

namespace glow
{
struct transform
{
public:
    const static transform Identity;

    /// Directions
    static tg::dir3 Forward() { return {0, 0, -1}; }
    static tg::dir3 Up() { return {0, 1, 0}; }
    static tg::dir3 Right() { return {1, 0, 0}; }

public:
    tg::pos3 position = tg::pos3::zero;
    tg::quat rotation = tg::quat::identity;
    float scale = 1.f;

public:
    transform() = default;

    transform(tg::pos3 const& pos, tg::quat const& rot = tg::quat::identity, float scale = 1.f) : position(pos), rotation(rot), scale(scale) {}

    /// Constructor from Model Matrix
    explicit transform(tg::mat4 m)
    {
        // NOTE: The following is a cut-down version of glm::decompose
        // (No Shear, no Perspective, slightly optimized / cleaned up)
        // TODO: Optimize this further

        // Normalize the matrix.
        if (m[3][3] == 0.f)
            return;

        for (auto i = 0; i < 4; ++i)
            for (auto j = 0; j < 4; ++j)
                m[i][j] /= m[3][3];

        // Clear the perspective partition
        m[0][3] = m[1][3] = m[2][3] = 0.f;
        m[3][3] = 1.f;

        // Extract translation
        position = tg::pos3(m[3]);
        m[3] = tg::vec4(0, 0, 0, m[3].w);

        tg::vec3 Row[3], Pdum3;

        // Extract Scale
        for (auto i = 0; i < 3; ++i)
            for (auto j = 0; j < 3; ++j)
                Row[i][j] = m[i][j];

        // Compute X scale factor and normalize first row.
        scale = length(Row[0]);

        Row[0] = normalize(Row[0]);
        Row[1] += Row[0] * -dot(Row[0], Row[1]);

        // Now, compute Y scale and normalize 2nd row.
        // scale.y = length(Row[1]);
        Row[1] = normalize(Row[1]);

        // Compute XZ and YZ shears, orthogonalize 3rd row.
        Row[2] += Row[0] * -dot(Row[0], Row[2]);
        Row[2] += Row[1] * -dot(Row[1], Row[2]);

        // Next, get Z scale and normalize 3rd row.
        // scale.z = length(Row[2]);
        Row[2] = normalize(Row[2]);

        // At this point, the matrix (in rows[]) is orthonormal.
        // Check for a coordinate system flip.  If the determinant
        // is -1, then negate the matrix and the scaling factors.
        Pdum3 = cross(Row[1], Row[2]);
        if (dot(Row[0], Pdum3) < 0.f)
        {
            scale *= -1.f;

            for (auto i = 0; i < 3; i++)
                Row[i] *= -1.f;
        }

        int i, j, k = 0;
        float root, trace = Row[0].x + Row[1].y + Row[2].z;
        if (trace > 0.f)
        {
            root = tg::sqrt(trace + 1.f);
            rotation.w = .5f * root;
            root = .5f / root;
            rotation.x = root * (Row[1].z - Row[2].y);
            rotation.y = root * (Row[2].x - Row[0].z);
            rotation.z = root * (Row[0].y - Row[1].x);
        }
        else
        {
            static constexpr int nextIndex[3] = {1, 2, 0};
            i = 0;
            if (Row[1].y > Row[0].x)
                i = 1;
            if (Row[2].z > Row[i][i])
                i = 2;
            j = nextIndex[i];
            k = nextIndex[j];

            root = tg::sqrt(Row[i][i] - Row[j][j] - Row[k][k] + 1.f);

            rotation[i] = .5f * root;
            root = .5f / root;
            rotation[j] = root * (Row[i][j] + Row[j][i]);
            rotation[k] = root * (Row[i][k] + Row[k][i]);
            rotation.w = root * (Row[j][k] - Row[k][j]);
        }
    }

    /// -- Transform Manipulation --

    /// Combine two Transforms
    transform& operator*=(transform const& rhs)
    {
        position += (rotation * tg::vec3(rhs.position)) * scale;
        rotation *= rhs.rotation;
        scale *= rhs.scale;
        return *this;
    }

    /// Moves the transform relative to the world
    void moveGlobal(tg::vec3 const& direction) { position += direction; }

    /// Moves the transform relative to its own orientation
    void moveLocal(tg::vec3 const& direction) { moveGlobal(rotation * direction); }
    void moveForward(float distance) { moveLocal(Forward() * distance); }
    void moveRight(float distance) { moveLocal(Right() * distance); }
    void moveUp(float distance) { moveLocal(Up() * distance); }
    void moveBack(float distance) { moveForward(-distance); }
    void moveLeft(float distance) { moveRight(-distance); }
    void moveDown(float distance) { moveUp(-distance); }

    void rotate(tg::quat const& rot) { rotation *= rot; }
    void applyScale(float scal) { scale *= scal; }

    void lerpToPosition(tg::pos3 const& pos, float amount) { position = tg::lerp(position, pos, amount); }
    void lerpToRotation(tg::quat const& rot, float amount) { rotation = tg::slerp(rotation, rot, amount); }
    void lerpToScale(float scal, float amount) { scale = tg::lerp(scale, scal, amount); }

    void lerpTo(transform const& target, float amount)
    {
        position = tg::lerp(position, target.position, amount);
        rotation = tg::slerp(rotation, target.rotation, amount);
        scale = tg::lerp(scale, target.scale, amount);
    }

    /// -- Getters and Setters --

    tg::mat4 getModelMatrix() const { return tg::translation(position) * to_mat4(rotation) * tg::scaling<3>(scale); }

    tg::mat4 getModelMatrixNoRotation() const { return tg::translation(position) * tg::scaling<3>(scale); }

    /// -- Miscellaneous --

    tg::vec3 getForwardVector() const
    {
        auto const rotMatrix = to_mat3(rotation);
        return tg::vec3(-rotMatrix[0][2], -rotMatrix[1][2], -rotMatrix[2][2]);
    }
    tg::vec3 getRightVector() const
    {
        auto const rotMatrix = to_mat3(rotation);
        return tg::vec3(rotMatrix[0][0], rotMatrix[1][0], rotMatrix[2][0]);
    }
    tg::vec3 getUpVector() const
    {
        auto const rotMatrix = to_mat3(rotation);
        return tg::vec3(rotMatrix[0][1], rotMatrix[1][1], rotMatrix[2][1]);
    }

public:
    /// -- Utility --

    /// Interpolation alpha helpers
    /// Usage: lerp(current, target, transform::xAlpha(dt, ..))

    /// Smoothed lerp alpha, framerate-correct
    static float smoothLerpAlpha(float smoothing, float dt) { return 1 - std::pow(smoothing, dt); }
    /// Exponential decay alpha, framerate-correct damp / lerp
    static float exponentialDecayAlpha(float lambda, float dt) { return 1 - std::exp(-lambda * dt); }
    /// alpha based on the halftime between current and target state
    static float halftimeLerpAlpha(float halftime, float dt) { return 1 - std::pow(.5f, dt / halftime); }
};

std::string to_string(transform const& t);

std::ostream& operator<<(std::ostream& stream, transform const& t);

}
