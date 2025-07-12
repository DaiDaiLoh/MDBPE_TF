#pragma once

#ifdef GLOW_EXTRAS_HAS_IMGUI

#include <glow/common/shared.hh>

#include <glow-extras/timing/CpuTimer.hh>

#include "ImguiMetric.hh"

namespace glow
{
namespace debugging
{
GLOW_SHARED(class, ProfilingOverlay);
class ProfilingOverlay
{
private:
    timing::CpuTimer mFrameTimer;

    ImguiMetric mMetricFrametime;
    ImguiMetric mMetricGpu;
    ImguiMetric mMetricCpu;
    ImguiMetricPlot mPlotFrametime;
    ImguiMetricPlot mPlotGpuCpu;

public:
    ProfilingOverlay();

    /// Update timing and metrics
    void onFrame(float gpuTime, float cpuTime);

    /// Perform ImGui
    void onGui();

public:
    GLOW_SHARED_CREATOR(ProfilingOverlay);
};
}
}

#endif
