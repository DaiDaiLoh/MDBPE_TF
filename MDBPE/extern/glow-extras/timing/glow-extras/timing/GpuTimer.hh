#pragma once

#include <array>

#include <glow/common/nodiscard.hh>
#include <glow/fwd.hh>

namespace glow
{
namespace timing
{
/**
 * Accurately time GPU operations
 *
 * Basic Usage:
 *
 * timer->start();
 * // timed code
 * timer->stop();
 *
 * float time = timer->elapsedSeconds();
 *
 * This timer uses multiple pairs of GL_TIMESTAMP queries, and accumulates their resulting deltas.
 * Each timestamp pair can be "in flight", and is only read once available to avoid stalling.
 * Thus, ::elapsedSeconds() is ideally only called every 2 or more frames.
 *
 * ::elapsedSeconds() simplifies the interface.
 * The returned value can be zero if no timestamps are ready in time,
 * and it can be an average of multiple measurements that were in flight previously.
 * If you want more fine-grained data, use the methods under "Advanced usage"
 */
GLOW_SHARED(class, GpuTimer);
class GpuTimer
{
public:
    struct ScopedGpuTimer
    {
    private:
        GpuTimer* const mTimer; ///< Backreference to timer

        ScopedGpuTimer(GpuTimer* timer) : mTimer(timer) { mTimer->start(); }

        friend class GpuTimer;

    public:
        ~ScopedGpuTimer() { mTimer->stop(); }
    };

private:
    static constexpr auto timerCount = 4u;

    struct Timer
    {
        glow::SharedTimestamp begin = nullptr;
        glow::SharedTimestamp end = nullptr;
        bool inFlight = false;
    };

    std::array<Timer, timerCount> mTimestamps;
    size_t mNextTimestamp = 0;

    float mAccumulatedTime = 0.f; // in sec
    int mAccumulationCounter = 0;

    float mCachedElapsedTime = 0.f;

private:
    void getResults();

public:
    GpuTimer();
    void init();

public:
    // -- Basic usage --

    /// Starts the timer
    void start();

    /// Stops the timer
    void stop();

    /// Returns elapsed time in seconds
    float elapsedSeconds();

    /// RAII start/stop helper
    /// {
    ///     auto scopedTimer = timer->scope();
    ///     // timed code..
    /// }
    GLOW_NODISCARD ScopedGpuTimer scope();

public:
    // -- Advanced usage --

    /// Returns the average elapsed time of recorded measurements
    float getAverageSeconds();

    /// Returns the combined time of accumulated measurements
    float getAccumulatedSeconds();

    /// Returns the amount of accumulated measurements
    int getAccumulationCounter() const { return mAccumulationCounter; }

    /// Resets the record of measurements
    void reset();

public:
    GLOW_NODISCARD static SharedGpuTimer create();
};
}
}
