#include "GpuTimer.hh"

#include <cassert>

#include <glow/objects/Timestamp.hh>

void glow::timing::GpuTimer::getResults()
{
    // Go over each timer currently in flight
    // For all that are available, add their deltas to the accumulated time

    auto freedIndex = timerCount;
    for (auto i = 0u; i < timerCount; ++i)
    {
        auto& timer = mTimestamps[i];

        if (timer.inFlight && timer.end->isAvailable())
        {
            timer.inFlight = false;
            auto const start = timer.begin->getNanoseconds();
            auto const end = timer.end->getNanoseconds();

            // Add the delta to the accumulator, in milliseconds
            mAccumulatedTime += float(double(end - start) * 1.e-9);
            ++mAccumulationCounter;

            freedIndex = i;
        }
    }

    if (freedIndex != timerCount)
    {
        // A new timer has been freed, jump to it
        mNextTimestamp = freedIndex;
    }
    else
    {
        // Increment
        ++mNextTimestamp;
        mNextTimestamp %= timerCount;
    }
}

glow::timing::GpuTimer::GpuTimer() {}

void glow::timing::GpuTimer::init()
{
    for (auto& timestampPair : mTimestamps)
    {
        timestampPair.begin = Timestamp::create();
        timestampPair.end = Timestamp::create();
    }
}

void glow::timing::GpuTimer::start()
{
    getResults(); // Accumulate previous timings if any are in flight
    mTimestamps[mNextTimestamp].begin->save();
}

void glow::timing::GpuTimer::stop()
{
    mTimestamps[mNextTimestamp].end->save();
    mTimestamps[mNextTimestamp].inFlight = true;
}

glow::timing::GpuTimer::ScopedGpuTimer glow::timing::GpuTimer::scope()
{
    return ScopedGpuTimer(this);
}

float glow::timing::GpuTimer::elapsedSeconds()
{
    auto const elapsedTime = getAverageSeconds();
    if (elapsedTime == 0.f)
        return mCachedElapsedTime;
    else
    {
        reset();
        mCachedElapsedTime = elapsedTime;
        return elapsedTime;
    }
}

void glow::timing::GpuTimer::reset()
{
    mAccumulatedTime = 0.f;
    mAccumulationCounter = 0;
}

glow::timing::SharedGpuTimer glow::timing::GpuTimer::create()
{
    auto const res = std::make_shared<GpuTimer>();
    res->init();
    return res;
}

float glow::timing::GpuTimer::getAverageSeconds()
{
    getResults();
    if (mAccumulationCounter == 0)
        return 0.f;
    else
        return mAccumulatedTime / mAccumulationCounter;
}

float glow::timing::GpuTimer::getAccumulatedSeconds()
{
    getResults();
    return mAccumulatedTime;
}
