#pragma once

#include <typed-geometry/tg-lean.hh>

#include <vector>

namespace glow
{
namespace pipeline
{
class SubpixelJitter
{
private:
    int mIterationLoopPoint; ///< Amount of iterations until the jitter sequence repeats
    int mBaseX;              ///< Halton sequence base for coordinate X
    int mBaseY;              ///< Halton sequence base for coordinate X

    int mCurrentIteration = 0; ///< Current iteration, % mIterationLoopPoint

    std::vector<tg::vec2> mCachedJitter;

    static float haltonSequence(int index, int base) noexcept
    {
        float f = 1.f;
        float r = 0.f;
        while (index > 0)
        {
            f = f / float(base);
            r = r + f * float(index % base);
            index = index / base;
        }
        return r;
    }

    void updateCache()
    {
        mCachedJitter.resize(static_cast<size_t>(mIterationLoopPoint));
        for (auto i = 0; i < mIterationLoopPoint; ++i)
        {
            mCachedJitter[static_cast<size_t>(i)] = tg::vec2(haltonSequence(i, mBaseX) - .5f, haltonSequence(i, mBaseY) - .5f);
        }
    }

public:
    SubpixelJitter(int loopPoint = 8, int baseX = 2, int baseY = 3) noexcept : mIterationLoopPoint(loopPoint), mBaseX(baseX), mBaseY(baseY)
    {
        updateCache();
    }

    /// Retrieves the jitter, then steps the iteration by one
    tg::vec2 getJitter()
    {
        ++mCurrentIteration;
        mCurrentIteration %= mIterationLoopPoint;
        return mCachedJitter[static_cast<size_t>(mCurrentIteration)];
    }

    void setLoopPoint(int loopPoint)
    {
        mIterationLoopPoint = loopPoint;
        updateCache();
    }

    void setHaltonBases(int x, int y)
    {
        mBaseX = x;
        mBaseY = y;
        updateCache();
    }
};


}
}
