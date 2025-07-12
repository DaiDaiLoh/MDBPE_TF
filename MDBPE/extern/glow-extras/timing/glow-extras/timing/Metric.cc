#include "Metric.hh"

void glow::timing::Metric::update()
{
    mMin = 10000.f;
    mMax = 0.f;
    mAvg = 0.f;

    for (auto const& val : mMeasurements)
    {
        mAvg += val;
        if (val < mMin)
            mMin = val;
        if (val > mMax)
            mMax = val;
    }

    mAvg /= static_cast<float>(mMeasurements.size());

    mMeasurements.clear();
}

void glow::timing::Metric::onMeasure(float value, unsigned autoUpdateCount)
{
    mMeasurements.push_back(value);

    if (autoUpdateCount && mMeasurements.size() > autoUpdateCount)
        update();
}

void glow::timing::Metric::reset()
{
    mMeasurements.clear();
    mMin = mMax = mAvg = 0;
}
