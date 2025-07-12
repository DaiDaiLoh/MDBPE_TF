#pragma once

#include <vector>

namespace glow
{
namespace timing
{
/**
 * Track average, minimum and maximum of a measurement
 */
class Metric
{
private:
    std::vector<float> mMeasurements;

    float mAvg = 0;
    float mMin = 0;
    float mMax = 0;

public:
    /// Add a measurement in milliseconds, then automatically update every N measures (0 = never)
    void onMeasure(float value, unsigned autoUpdateCount = 60);

    /// Update average, min and max, then clear records
    void update();

    /// Reset the recorded measurements
    void reset();

    float const& getAverage() const { return mAvg; }
    float const& getMin() const { return mMin; }
    float const& getMax() const { return mMax; }
};
}
}
