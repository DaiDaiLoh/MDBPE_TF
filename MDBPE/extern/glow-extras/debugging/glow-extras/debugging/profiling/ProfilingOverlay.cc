#ifdef GLOW_EXTRAS_HAS_IMGUI

#include "ProfilingOverlay.hh"

#include <imgui/imgui.h>

glow::debugging::ProfilingOverlay::ProfilingOverlay()
  : mMetricFrametime("Frame time", "ms", ImguiMetric::USE_SI_UNIT_PREFIX),
    mMetricGpu("GPU time", "ms", ImguiMetric::USE_SI_UNIT_PREFIX),
    mMetricCpu("CPU time", "ms", ImguiMetric::USE_SI_UNIT_PREFIX)
{
    // Blue
    //    mMetricCpu.mColor[0] = 57 / 255.f;
    //    mMetricCpu.mColor[1] = 109 / 255.f;
    //    mMetricCpu.mColor[2] = 169 / 255.f;

    mPlotFrametime.AddMetric(&mMetricFrametime);
    mPlotFrametime.mShowLegendAverage = true;

    mPlotGpuCpu.AddMetric(&mMetricGpu);
    mPlotGpuCpu.AddMetric(&mMetricCpu);
    mPlotGpuCpu.mShowLegendAverage = true;
}

void glow::debugging::ProfilingOverlay::onFrame(float gpuTime, float cpuTime)
{
    auto const frametimeMs = mFrameTimer.elapsedMilliseconds();
    mFrameTimer.restart();

    mMetricFrametime.AddNewValue(frametimeMs);
    mMetricCpu.AddNewValue(cpuTime);
    mMetricGpu.AddNewValue(gpuTime);

    mPlotFrametime.UpdateAxes();
    mPlotGpuCpu.UpdateAxes();
}

void glow::debugging::ProfilingOverlay::onGui()
{
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Application profiling");

    mPlotFrametime.DrawHistory();
    mPlotGpuCpu.DrawHistory();

    ImGui::End();
}

#endif
