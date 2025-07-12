#include "StageCamera.hh"

#include <glow/objects/ShaderStorageBuffer.hh>
#include <glow/objects/TextureRectangle.hh>
#include <glow/std140.hh>

#include <glow-extras/camera/CameraUtils.hh>

#include <typed-geometry/tg.hh>

namespace
{
auto constexpr clusterPixelSizeX = GLOW_PIPELINE_CLUSTER_PIXEL_SIZE_X;
auto constexpr clusterPixelSizeY = GLOW_PIPELINE_CLUSTER_PIXEL_SIZE_Y;
auto constexpr clusterAmountZ = GLOW_PIPELINE_CLUSTER_AMOUNT_Z;
auto constexpr lightAssignComputeSizeX = GLOW_PIPELINE_LASSIGN_COMPUTE_X;
auto constexpr lightAssignComputeSizeY = GLOW_PIPELINE_LASSIGN_COMPUTE_Y;

template <typename T = unsigned, typename U = int>
constexpr T intDivCeil(T a, U b)
{
    return (a + static_cast<T>(b) - 1) / static_cast<T>(b);
}
}

void glow::pipeline::StageCamera::updateData(camera::CameraBase const& cam)
{
    // Un-dirty
    bool resolutionChanged = false;

    // Viewport Update
    {
        auto const newRes = cam.getViewportSize();
        if (mCameraData.resolution != newRes)
        {
            mCameraData.resolution = newRes;
            mCameraData.halfResolution = tg::isize2(intDivCeil(mCameraData.resolution.width, 2), intDivCeil(mCameraData.resolution.height, 2));
            mCameraData.quarterResolution = tg::isize2(intDivCeil(mCameraData.resolution.width, 4), intDivCeil(mCameraData.resolution.height, 4));
            resolutionChanged = true;
        }
    }

    // Un-dirty
    mCameraData.projectionChangedThisFrame = resolutionChanged;

    // Store previous frame camera matrices
    {
        mCameraData.prevView = mCameraData.view;
        mCameraData.prevCleanProj = mCameraData.cleanProj;
        mCameraData.prevCleanVp = mCameraData.cleanVp;
    }

    // Temporal Step (depends on resolution)
    stepTemporalData(resolutionChanged);

    // Camera info
    {
        mCameraData.camPos = cam.getPosition();

        float const newNearPlane = cam.getNearClippingPlane();
        float const newFarPlane = cam.getFarClippingPlane();
        auto const newFov = cam.getHorizontalFieldOfView();

        // The -Wfloat-equal warning here is intentionally ignored
        // (The comparison is not unsafe, it does exactly what you expect it to do)
        if (mCameraData.camNearPlane != newNearPlane || mCameraData.camFarPlane != newFarPlane || mCameraData.camFov.angle != newFov.angle)
        {
            mCameraData.camNearPlane = newNearPlane;
            mCameraData.camFarPlane = newFarPlane;
            mCameraData.camFov = newFov;
            mCameraData.projectionChangedThisFrame = true;
        }
    }


    // Camera matrices
    {
        mCameraData.view = cam.getViewMatrix();

        if (mCameraData.projectionChangedThisFrame)
        {
            // Only calculate (fetch) the projection matrix if it was dirtied
#if GLOW_PIPELINE_ENABLE_REVERSE_Z
            float const aspect = mCameraData.resolution.width / static_cast<float>(mCameraData.resolution.height);
            mCameraData.cleanProj = tg::perspective_reverse_z_opengl(cam.getHorizontalFieldOfView(), aspect, mCameraData.camNearPlane);
            mCameraData.simulatedCleanProj = cam.getProjectionMatrix();
#else
            mCameraData.cleanProj = cam.getProjectionMatrix();
#endif
            mCameraData.proj = mCameraData.cleanProj;
        }

        // Projection Matrix Jitter, depends on temporal data
        mCameraData.proj[2][0] = mTemporalData.subpixelJitter.x / static_cast<float>(mCameraData.resolution.width);
        mCameraData.proj[2][1] = mTemporalData.subpixelJitter.y / static_cast<float>(mCameraData.resolution.height);
    }

    // Precalculated matrices
    {
        mCameraData.viewInverse = tg::inverse(mCameraData.view);

        if (mCameraData.projectionChangedThisFrame)
        {
            mCameraData.cleanProjInverse = tg::inverse(mCameraData.cleanProj);
        }

        mCameraData.projInverse = tg::inverse(mCameraData.proj);
        mCameraData.vp = mCameraData.proj * mCameraData.view;
        mCameraData.vpInverse = tg::inverse(mCameraData.vp);
        mCameraData.cleanVp = mCameraData.cleanProj * mCameraData.view;
    }


    // Precomputed helper values
    {
        mCameraData.clipInfo
            = tg::vec3(mCameraData.camNearPlane * mCameraData.camFarPlane, mCameraData.camNearPlane - mCameraData.camFarPlane, mCameraData.camFarPlane);
        mCameraData.tanHalfFovX = 1.f / std::abs(mCameraData.proj[0][0]);
        mCameraData.tanHalfFovY = 1.f / std::abs(mCameraData.proj[1][1]);

        // Algorithm specific helper values
        const float scale = -2.0f * tan(cam.getHorizontalFieldOfView().angle * 0.5f);
        mCameraData.imagePlanePixelsPerMeter = std::abs(float(mCameraData.resolution.width) / scale);
    }

    // Cluster data (depends on projection data)
    updateClusterData(mCameraData.projectionChangedThisFrame);
}

void glow::pipeline::StageCamera::stepTemporalData(bool resolutionChanged)
{
    mTemporalData.evenFrame = !mTemporalData.evenFrame;
    mTemporalData.subpixelJitter = mJitterGenerator.getJitter();

    // History buffer resize
    if (resolutionChanged)
    {
        mTemporalData.historyBufferEven->bind().resize(mCameraData.resolution);
        mTemporalData.historyBufferOdd->bind().resize(mCameraData.resolution);

        mTemporalData.historyBufferEven->clear(tg::color3::black);
        mTemporalData.historyBufferOdd->clear(tg::color3::black);
    }
}

void glow::pipeline::StageCamera::updateClusterData(bool projectionChanged)
{
    if (projectionChanged)
    {
        mClusterData.clusterGridSize
            = {intDivCeil(mCameraData.resolution.width, clusterPixelSizeX), intDivCeil(mCameraData.resolution.height, clusterPixelSizeY), clusterAmountZ};
        mClusterData.clusterAmount = mClusterData.clusterGridSize.width * mClusterData.clusterGridSize.height * mClusterData.clusterGridSize.depth;
        mClusterData.lightAssignmentDispatch = {intDivCeil(mClusterData.clusterGridSize.width, lightAssignComputeSizeX),
                                                intDivCeil(mClusterData.clusterGridSize.height, lightAssignComputeSizeY)};

        mClusterData.scaleBias.x = static_cast<float>(clusterAmountZ) / std::log2f(mCameraData.camFarPlane / mCameraData.camNearPlane);
        mClusterData.scaleBias.y
            = -(static_cast<float>(clusterAmountZ) * std::log2f(mCameraData.camNearPlane) / std::log2f(mCameraData.camFarPlane / mCameraData.camNearPlane));
    }
}

glow::pipeline::StageCamera::StageCamera()
{
    mTemporalData.historyBufferEven = TextureRectangle::create(1, 1, GL_RGB16F);
    mTemporalData.historyBufferOdd = TextureRectangle::create(1, 1, GL_RGB16F);
}

void glow::pipeline::StageCamera::onNewFrame(camera::CameraBase const& cam) { updateData(cam); }

#undef INT_DIV_CEIL
