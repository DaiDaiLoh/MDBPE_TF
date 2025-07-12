#include "FrameCounter.hh"

bool glow::timing::FrameCounter::onNextFrame()
{
    ++mFramesSinceReport;

    if (mFramesSinceReport >= mIntervalFrames)
    {
        auto const time = mTimer.elapsedSeconds();
        if (time > mIntervalTime)
        {
            mAverageFramerate = mFramesSinceReport / time;
            mFramesSinceReport = 0;
            mTimer.restart();
            return true;
        }
    }

    return false;
}

void glow::timing::FrameCounter::reset()
{
    mFramesSinceReport = 0;
    mTimer.restart();
}
