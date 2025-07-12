#pragma once

#include <typed-geometry/tg-lean.hh>

#include <glow/common/property.hh>
#include <glow/common/shared.hh>
#include <glow/fwd.hh>

#include <glow-extras/camera/CameraBase.hh>
#include <glow-extras/pipeline/utility/SubpixelJitter.hh>

#include "shader/glow-pipeline/internal/common/globals.hh"

namespace glow
{
namespace pipeline
{
struct CameraData
{
    // == Resolution ==
    tg::isize2 resolution;
    tg::isize2 halfResolution;
    tg::isize2 quarterResolution;

    // == Last frame values ==
    tg::mat4 prevView;      ///< View Matrix of the previous frame
    tg::mat4 prevCleanProj; ///< Unjittered Projection Matrix of the previous frame
    tg::mat4 prevCleanVp;   ///< Unjittered VP Matrix of the previous frame

    // == Camera information ==
    tg::pos3 camPos;
    float camNearPlane = 0;
    // If in reverse Z, the provided camera far plane is used as the simulated far plane.
    // It concerns directional shadow cascades and light clustering range.
    float camFarPlane = 0;
    tg::horizontal_fov camFov = tg::horizontal_fov(0_deg);
    bool projectionChangedThisFrame = false;

    // == View and Projection matrices ==
    tg::mat4 view;      ///< View Matrix
    tg::mat4 proj;      ///< Projection Matrix
    tg::mat4 cleanProj; ///< Unjittered Projection Matrix
    tg::mat4 vp;        ///< Projection * View Matrix
    tg::mat4 cleanVp;   ///< Unjittered Projection * View Matrix
#if GLOW_PIPELINE_ENABLE_REVERSE_Z
    tg::mat4 simulatedCleanProj; ///< Simulated projection matrix for shadow mapping
#endif

    // == Precalculated inverse matrices ==
    tg::mat4 viewInverse;
    tg::mat4 projInverse;
    tg::mat4 cleanProjInverse;
    tg::mat4 vpInverse;

    // == Special precomputed values ==
    tg::vec3 clipInfo;
    float tanHalfFovX = 0;
    float tanHalfFovY = 0;
    float imagePlanePixelsPerMeter = 0;
};

struct ClusterData
{
    tg::isize3 clusterGridSize;         ///< Amount of clusters in each dimension
    unsigned clusterAmount = 0;         ///< Amount of clusters in total
    tg::isize2 lightAssignmentDispatch; ///< Amount of invocations of the light assignment compute shader
    tg::vec2 scaleBias;                 ///< Precomputed values to calculate cluster indices
};

struct TemporalData
{
    bool evenFrame = true;
    tg::vec2 subpixelJitter;

    bool rejectHistory = true; ///< If true, enable history rejection (disabling is useful for screenshots)

    SharedTextureRectangle historyBufferEven;
    SharedTextureRectangle historyBufferOdd;
};

GLOW_SHARED(class, StageCamera);
class StageCamera
{
private:
    /// Camera Data
    /// Depends on values retrieved from mBaseCamera
    CameraData mCameraData;

    /// Temporal Data
    /// Contains temporal variables like frame parity or current TAA subpixel jitter
    SubpixelJitter mJitterGenerator;
    TemporalData mTemporalData;

    /// Cluster Data
    ClusterData mClusterData;

private:
    /// Updates all data, must only be called once per frame
    void updateData(camera::CameraBase const& cam);

    /// Updates temporal variables
    void stepTemporalData(bool resolutionChanged);

    /// Updates cluster data
    void updateClusterData(bool resolutionChanged);

public:
    StageCamera();

    void onNewFrame(camera::CameraBase const& cam);

    GLOW_GETTER(CameraData);
    GLOW_GETTER(TemporalData);
    GLOW_GETTER(ClusterData);

public:
    bool const& isRejectingHistory() const { return mTemporalData.rejectHistory; }
    void setRejectHistory(bool reject) { mTemporalData.rejectHistory = reject; }

public:
    GLOW_SHARED_CREATOR(StageCamera);
};

}
}
