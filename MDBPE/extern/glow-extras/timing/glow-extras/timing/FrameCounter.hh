#pragma once

#include <vector>

#include <glow/common/property.hh>

#include "CpuTimer.hh"

namespace glow
{
namespace timing
{
/**
 * Measure averaged FPS and frametime
 * over a configurable duration
 *
 * Usage:
 *
 * // After each rendered frame
 * counter.onNextFrame();
 *
 * // Any time
 * log() << "Current FPS: " << counter.getAverageFramerate();
 */
class FrameCounter
{
private:
    CpuTimer mTimer;

    int mIntervalFrames = 5;   ///< Minimal length of an interval in frames
    float mIntervalTime = .5f; ///< Minimal length of an interval in seconds

    int mFramesSinceReport = 0;    ///< Frames of the current interval
    float mAverageFramerate = 0.f; ///< Averaged framerate of the last interval

public:
    /// Call at the end of each frame, returns true if stats have been updated as a consequence
    bool onNextFrame();

    /// Reset after a rendering pause to avoid falsifying the framerate
    void reset();

    /// Retrieve the average framerate
    GLOW_GETTER(AverageFramerate);

    /// Retrieve the average frametime
    float getAverageFrametime() const { return 1000.f / mAverageFramerate; }

public:
    // Configure interval length
    GLOW_PROPERTY(IntervalTime);
    GLOW_PROPERTY(IntervalFrames);
};
}
}
