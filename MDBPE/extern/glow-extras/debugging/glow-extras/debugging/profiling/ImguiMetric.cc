/*
https://github.com/GameTechDev/MetricsGui
Slightly modified
Original license:

Copyright 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifdef GLOW_EXTRAS_HAS_IMGUI

#include "ImguiMetric.hh"

#include <algorithm>
#include <typed-geometry/feature/assert.hh>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace
{
static auto constexpr HBAR_PADDING_TOP = 2.f;
static auto constexpr HBAR_PADDING_BOTTOM = 2.f;
static auto constexpr DESC_HBAR_PADDING = 8.f;
static auto constexpr HBAR_VALUE_PADDING = 8.f;
static auto constexpr PLOT_LEGEND_PADDING = 8.f;
static auto constexpr LEGEND_TEXT_VERTICAL_SPACING = 2.f;

uint32_t gConstructedMetricIndex = 0;

int CreateQuantityLabel(char* memory, size_t memorySize, float quantity, char const* units, char const* prefix, bool useSiUnitPrefix)
{
    enum
    {
        NANO,
        MICRO,
        MILLI,
        NONE,
        KILO,
        MEGA,
        GIGA,
        TERA,
        NUM_SI_PREFIXES,
    };
    char const* siPrefixChar = "num kMGT";
    uint32_t siPrefix = NONE;
    double value = static_cast<double>(quantity);

    // Adjust SI magnitude if requested
    if (useSiUnitPrefix)
    {
        if (units[0] != '\0' && (strcmp(units + 1, "Hz") == 0 || strcmp(units + 1, "s") == 0))
        {
            switch (units[0])
            {
            case 'n':
                siPrefix = NANO;
                break;
            case 'u':
                siPrefix = MICRO;
                break;
            case 'm':
                siPrefix = MILLI;
                break;
            case 'k':
                siPrefix = KILO;
                break;
            case 'M':
                siPrefix = MEGA;
                break;
            case 'G':
                siPrefix = GIGA;
                break;
            case 'T':
                siPrefix = TERA;
                break;
            default:
                TG_ASSERT(false);
                break;
            }
            units = units + 1;
        }

        if (value == 0.0)
        { // If the value is zero, prevent 0 nUnits
            siPrefix = NONE;
        }
        else
        {
            auto sign = value < 0.0 ? -1.0 : 1.0;
            value *= sign;
            for (; value > 1000.0 && siPrefix < NUM_SI_PREFIXES - 1; ++siPrefix)
                value *= 0.001;
            for (; value < 1.0 && siPrefix > 0; --siPrefix)
                value *= 1000.0;
            value *= sign;
        }
    }

    // Convert value to 4 character long string
    //     XXX1234.123YYY => " XXX1234.123" (9+) => "XXX1234"
    //     234.123YYY     => " 234.123"     (8) => " 234"
    //     34.123YYY      => " 34.123"      (7) => "34.1"
    //     4.123YYY       => " 4.123"       (6) => "4.12"
    //     0.123YYY       => " 0.123"       (6) => ".123"
    char numberString[256];
    int n = snprintf(numberString, 256, " %.3lf", value);
    auto valueS = &numberString[1];

    if (n >= 8)
    {
        numberString[n - 4] = '\0';
        if (n == 8)
            valueS = &numberString[0];
    }
    else
    {
        if (numberString[1] == '0')
        {
            valueS = &numberString[1];

            // Special case: ".000" -> "   0"
            if (numberString[3] == '0' && numberString[4] == '0' && numberString[5] == '0')
            {
                numberString[1] = ' ';
                numberString[2] = ' ';
                numberString[3] = ' ';
            }
        }
        valueS[4] = '\0';
    }

    // Output final string
    char siPrefixS[] = {siPrefix == NONE ? '\0' : siPrefixChar[siPrefix], '\0'};
    return snprintf(memory, memorySize, "%s%s %s%s", prefix, valueS, siPrefixS, units);
}

void DrawQuantityLabel(float quantity, char const* units, char const* prefix, bool useSiUnitPrefix)
{
    char s[512] = {};
    CreateQuantityLabel(s, sizeof(s), quantity, units, prefix, useSiUnitPrefix);
    ImGui::TextUnformatted(s);
}

}


namespace glow
{
namespace debugging
{
ImguiMetric::ImguiMetric()
{
    auto c = ImColor::HSV(0.2f * gConstructedMetricIndex++, 0.8f, 0.8f);
    mColor[0] = c.Value.x;
    mColor[1] = c.Value.y;
    mColor[2] = c.Value.z;
    mColor[3] = c.Value.w;

    Initialize("", "", NONE);
}

ImguiMetric::ImguiMetric(char const* description, char const* units, uint32_t flags)
{
    auto c = ImColor::HSV(0.2f * gConstructedMetricIndex++, 0.8f, 0.8f);
    mColor[0] = c.Value.x;
    mColor[1] = c.Value.y;
    mColor[2] = c.Value.z;
    mColor[3] = c.Value.w;

    Initialize(description, units, flags);
}

void ImguiMetric::Initialize(char const* description, char const* units, uint32_t flags)
{
    mDescription = description == nullptr ? "" : description;
    mUnits = units == nullptr ? "" : units;
    mTotalInHistory = 0.;
    mHistoryCount = 0;
    memset(mHistory, 0, NUM_HISTORY_SAMPLES * sizeof(float));
    mKnownMinValue = 0.f;
    mKnownMaxValue = 0.f;
    mFlags = flags;
    mSelected = false;
}

void ImguiMetric::SetLastValue(float value, uint32_t prevIndex)
{
    TG_ASSERT(prevIndex < NUM_HISTORY_SAMPLES);
    auto p = &mHistory[NUM_HISTORY_SAMPLES - 1 - prevIndex];
    mTotalInHistory -= static_cast<double>(*p);
    *p = value;
    mTotalInHistory += static_cast<double>(value);
}

void ImguiMetric::AddNewValue(float value)
{
    mTotalInHistory -= static_cast<double>(mHistory[0]);
    memmove(mHistory, mHistory + 1, (NUM_HISTORY_SAMPLES - 1) * sizeof(mHistory[0]));
    mHistory[NUM_HISTORY_SAMPLES - 1] = value;
    mTotalInHistory += static_cast<double>(value);
    mHistoryCount = std::min(static_cast<uint32_t>(NUM_HISTORY_SAMPLES), mHistoryCount + 1);
}

float ImguiMetric::GetLastValue(uint32_t prevIndex) const
{
    TG_ASSERT(prevIndex < NUM_HISTORY_SAMPLES);
    return mHistory[NUM_HISTORY_SAMPLES - 1 - prevIndex];
}

float ImguiMetric::GetAverageValue() const
{
    return mHistoryCount == 0 ? 0.f : (static_cast<float>(mTotalInHistory) / mHistoryCount);
}

// Note: we defer computing the sizes because ImGui doesn't load the font until
// the first frame.

ImguiMetricPlot::WidthInfo::WidthInfo(ImguiMetricPlot* plot)
  : mLinkedPlots(1, plot), mDescWidth(0.f), mValueWidth(0.f), mLegendWidth(0.f), mInitialized(false)
{
}

void ImguiMetricPlot::WidthInfo::Initialize()
{
    if (mInitialized)
    {
        return;
    }

    auto prefixWidth = ImGui::CalcTextSize("XXX").x;
    auto sepWidth = ImGui::CalcTextSize(": ").x;
    auto valueWidth = ImGui::CalcTextSize("888. X").x;
    for (auto linkedPlot : mLinkedPlots)
    {
        for (auto metric : linkedPlot->mMetrics)
        {
            auto descWidth = ImGui::CalcTextSize(metric->mDescription.c_str()).x;
            auto unitsWidth = ImGui::CalcTextSize(metric->mUnits.c_str()).x;
            auto quantWidth = valueWidth + unitsWidth;

            mDescWidth = std::max(mDescWidth, descWidth);
            mValueWidth = std::max(mValueWidth, quantWidth);
            mLegendWidth = std::max(mLegendWidth, std::max(descWidth, prefixWidth) + sepWidth + quantWidth);
        }
    }

    mInitialized = true;
}

ImguiMetricPlot::ImguiMetricPlot()
  : mMetrics(),
    mMetricRange(),
    mWidthInfo(new ImguiMetricPlot::WidthInfo(this)),
    mMinValue(0.f),
    mMaxValue(0.f),
    mRangeInitialized(false),
    mBarRounding(0.f),
    mRangeDampening(0.95f),
    mInlinePlotRowCount(2),
    mPlotRowCount(5),
    mVBarMinWidth(6),
    mVBarGapWidth(1),
    mShowAverage(false),
    mShowInlineGraphs(false),
    mShowOnlyIfSelected(false),
    mShowLegendDesc(true),
    mShowLegendColor(true),
    mShowLegendUnits(true),
    mShowLegendAverage(false),
    mShowLegendMin(true),
    mShowLegendMax(true),
    mBarGraph(false),
    mStacked(false),
    mSharedAxis(false),
    mFilterHistory(true)
{
}

ImguiMetricPlot::ImguiMetricPlot(ImguiMetricPlot const& copy)
  : mMetrics(copy.mMetrics),
    mMetricRange(copy.mMetricRange),
    mWidthInfo(copy.mWidthInfo),
    mMinValue(copy.mMinValue),
    mMaxValue(copy.mMaxValue),
    mRangeInitialized(copy.mRangeInitialized),
    mBarRounding(copy.mBarRounding),
    mRangeDampening(copy.mRangeDampening),
    mInlinePlotRowCount(copy.mInlinePlotRowCount),
    mPlotRowCount(copy.mPlotRowCount),
    mVBarMinWidth(copy.mVBarMinWidth),
    mVBarGapWidth(copy.mVBarGapWidth),
    mShowAverage(copy.mShowAverage),
    mShowInlineGraphs(copy.mShowInlineGraphs),
    mShowOnlyIfSelected(copy.mShowOnlyIfSelected),
    mShowLegendDesc(copy.mShowLegendDesc),
    mShowLegendColor(copy.mShowLegendColor),
    mShowLegendUnits(copy.mShowLegendUnits),
    mShowLegendAverage(copy.mShowLegendAverage),
    mShowLegendMin(copy.mShowLegendMin),
    mShowLegendMax(copy.mShowLegendMax),
    mBarGraph(copy.mBarGraph),
    mStacked(copy.mStacked),
    mSharedAxis(copy.mSharedAxis),
    mFilterHistory(copy.mFilterHistory)
{
    mWidthInfo->mLinkedPlots.emplace_back(this);
}

ImguiMetricPlot::~ImguiMetricPlot()
{
    auto it = std::find(mWidthInfo->mLinkedPlots.begin(), mWidthInfo->mLinkedPlots.end(), this);
    TG_ASSERT(it != mWidthInfo->mLinkedPlots.end());
    mWidthInfo->mLinkedPlots.erase(it);
    if (mWidthInfo->mLinkedPlots.empty())
    {
        delete mWidthInfo;
    }
}

void ImguiMetricPlot::LinkLegends(ImguiMetricPlot* plot)
{
    // Already linked?
    auto otherWidthInfo = plot->mWidthInfo;
    if (mWidthInfo == otherWidthInfo)
    {
        return;
    }

    // Ensure either neither or both axes are initialized
    if (mWidthInfo->mInitialized || otherWidthInfo->mInitialized)
    {
        mWidthInfo->Initialize();
        otherWidthInfo->Initialize();
        mWidthInfo->mDescWidth = std::max(mWidthInfo->mDescWidth, otherWidthInfo->mDescWidth);
        mWidthInfo->mValueWidth = std::max(mWidthInfo->mValueWidth, otherWidthInfo->mValueWidth);
        mWidthInfo->mLegendWidth = std::max(mWidthInfo->mLegendWidth, otherWidthInfo->mLegendWidth);
    }

    // Move plot's linked plots to this
    do
    {
        plot = otherWidthInfo->mLinkedPlots.back();
        otherWidthInfo->mLinkedPlots.pop_back();

        plot->mWidthInfo = mWidthInfo;
        mWidthInfo->mLinkedPlots.emplace_back(plot);
    } while (!otherWidthInfo->mLinkedPlots.empty());

    delete otherWidthInfo;
}

void ImguiMetricPlot::UpdateAxes()
{
    float oldWeight;
    if (mRangeInitialized)
    {
        oldWeight = std::min(1.f, std::max(0.f, mRangeDampening));
    }
    else
    {
        oldWeight = 0.f;
        mRangeInitialized = true;
    }
    auto newWeight = 1.f - oldWeight;

    float minPlotValue = FLT_MAX;
    float maxPlotValue = FLT_MIN;
    for (size_t i = 0, N = mMetrics.size(); i < N; ++i)
    {
        auto metric = mMetrics[i];
        auto metricRange = &mMetricRange[i];

        auto knownMinValue = 0 != (metric->mFlags & ImguiMetric::KNOWN_MIN_VALUE);
        auto knownMaxValue = 0 != (metric->mFlags & ImguiMetric::KNOWN_MAX_VALUE);
        auto historyRange = std::make_pair(
            knownMinValue ? &metric->mKnownMinValue : std::min_element(metric->mHistory, metric->mHistory + ImguiMetric::NUM_HISTORY_SAMPLES),
            knownMaxValue ? &metric->mKnownMaxValue : std::max_element(metric->mHistory, metric->mHistory + ImguiMetric::NUM_HISTORY_SAMPLES));
        metricRange->first = metricRange->first * oldWeight + *historyRange.first * newWeight;
        metricRange->second = metricRange->second * oldWeight + *historyRange.second * newWeight;

        minPlotValue = std::min(minPlotValue, *historyRange.first);
        maxPlotValue = std::max(maxPlotValue, *historyRange.second);
    }

    if (mSharedAxis)
    {
        minPlotValue = mMetricRange[0].first;
        maxPlotValue = mMetricRange[0].second;
    }
    else if (mStacked)
    {
        maxPlotValue = FLT_MIN;
        for (size_t i = 0; i < ImguiMetric::NUM_HISTORY_SAMPLES; ++i)
        {
            float stackedValue = 0.f;
            for (auto metric : mMetrics)
            {
                stackedValue += metric->mHistory[i];
            }
            maxPlotValue = std::max(maxPlotValue, stackedValue);
        }
    }

    mMinValue = mMinValue * oldWeight + minPlotValue * newWeight;
    mMaxValue = mMaxValue * oldWeight + maxPlotValue * newWeight;
}

void ImguiMetricPlot::AddMetric(ImguiMetric* metric)
{
    mMetrics.emplace_back(metric);
    mMetricRange.emplace_back(FLT_MAX, FLT_MIN);
}

void ImguiMetricPlot::AddMetrics(ImguiMetric* metrics, size_t metricCount)
{
    mMetrics.reserve(mMetrics.size() + metricCount);
    mMetricRange.reserve(mMetrics.size());
    for (size_t i = 0; i < metricCount; ++i)
    {
        AddMetric(&metrics[i]);
    }
}

void ImguiMetricPlot::SortMetricsByName()
{
    std::sort(mMetrics.begin(), mMetrics.end(), [](ImguiMetric* a, ImguiMetric* b) { return a->mDescription.compare(b->mDescription) < 0; });
}

namespace
{
bool DrawPrefix(ImguiMetricPlot* plot)
{
    plot->mWidthInfo->Initialize();

    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
    {
        return false;
    }

    return true;
}

void DrawMetrics(ImguiMetricPlot* plot, std::vector<ImguiMetric*> const& metrics, uint32_t plotRowCount, float plotMinValue, float plotMaxValue)
{
    auto window = ImGui::GetCurrentWindow();
    auto const& style = GImGui->Style;

    auto textHeight = ImGui::GetTextLineHeight();

    auto plotWidth = std::max(0.f, ImGui::GetContentRegionAvail().x - window->WindowPadding.x - plot->mWidthInfo->mLegendWidth - PLOT_LEGEND_PADDING);
    auto plotHeight = std::max(0.f, (textHeight + LEGEND_TEXT_VERTICAL_SPACING) * plotRowCount);

    ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(plotWidth, plotHeight));
    ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);

    ImGui::ItemSize(frame_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(frame_bb, 0))
    {
        return;
    }

    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    plotWidth = inner_bb.GetWidth();
    plotHeight = inner_bb.GetHeight();

    size_t pointCount = ImguiMetric::NUM_HISTORY_SAMPLES;
    size_t maxBarCount = static_cast<size_t>(plotWidth / (plot->mVBarMinWidth + plot->mVBarGapWidth));

    if (plotMaxValue == plotMinValue)
    {
        pointCount = 0;
    }

    auto const verticalAxeLower = 0.f; // Originally: plotMinValue

    bool useFilterPath = plot->mFilterHistory || (maxBarCount > pointCount);
    if (!useFilterPath)
    {
        pointCount = maxBarCount;
    }
    else if (plot->mBarGraph)
    {
        pointCount = std::min(pointCount, maxBarCount);
    }
    else
    {
        pointCount = std::min(pointCount, static_cast<size_t>(plotWidth));
    }
    if (pointCount > 0)
    {
        std::vector<float> baseValue(pointCount, 0.f);
        auto hScale = plotWidth / static_cast<float>(plot->mBarGraph ? pointCount : (pointCount - 1));
        auto vScale = plotHeight / (plotMaxValue - verticalAxeLower);
        for (auto metric : metrics)
        {
            if (plot->mShowOnlyIfSelected && !metric->mSelected)
            {
                continue;
            }

            auto color = ImGui::ColorConvertFloat4ToU32(*reinterpret_cast<ImVec4*>(&metric->mColor));

            size_t historyBeginIdx = useFilterPath ? 0 : (ImguiMetric::NUM_HISTORY_SAMPLES - pointCount);
            ImVec2 p;
            float prevB = 0.f;
            for (size_t i = 0; i < pointCount; ++i)
            {
                size_t historyEndIdx = useFilterPath ? ((i + 1) * ImguiMetric::NUM_HISTORY_SAMPLES / pointCount) : (historyBeginIdx + 1);
                size_t N = historyEndIdx - historyBeginIdx;
                float v = 0.f;
                if (N > 0)
                {
                    do
                    {
                        v += metric->mHistory[historyBeginIdx];
                        ++historyBeginIdx;
                    } while (historyBeginIdx < historyEndIdx);
                    v = v / static_cast<float>(N);
                }
                float b = baseValue[i];
                v += b;

                ImVec2 pn(inner_bb.Min.x + hScale * i, inner_bb.Max.y - vScale * (v - verticalAxeLower));

                if (i > 0)
                {
                    if (plot->mBarGraph)
                    {
                        ImVec2 p1(pn.x - plot->mVBarGapWidth, inner_bb.Max.y - vScale * (prevB - verticalAxeLower));
                        p = ImClamp(p, inner_bb.Min, inner_bb.Max);
                        p1 = ImClamp(p1, inner_bb.Min, inner_bb.Max);
                        window->DrawList->AddRectFilled(p, p1, color, plot->mBarRounding);
                    }
                    else
                    {
                        pn = ImClamp(pn, inner_bb.Min, inner_bb.Max);
                        window->DrawList->AddLine(p, pn, color);
                    }
                }

                p = pn;
                prevB = b;
                if (plot->mStacked)
                {
                    baseValue[i] = v;
                }
            }

            if (plot->mBarGraph)
            {
                ImVec2 p1(inner_bb.Max.x - plot->mVBarGapWidth, inner_bb.Max.y - vScale * (prevB - verticalAxeLower));
                p = ImClamp(p, inner_bb.Min, inner_bb.Max);
                p1 = ImClamp(p1, inner_bb.Min, inner_bb.Max);
                window->DrawList->AddRectFilled(p, p1, color, plot->mBarRounding);
            }

            if (plot->mShowAverage)
            {
                auto avgValue = metric->GetAverageValue();
                auto y = inner_bb.Max.y - vScale * (avgValue - verticalAxeLower);
                y = ImClamp(y, inner_bb.Min.y, inner_bb.Max.y);
                window->DrawList->AddLine(ImVec2(inner_bb.Min.x, y), ImVec2(inner_bb.Max.x, y), color);
            }
        }
    }

    ImGui::SameLine();

    auto useSiUnitPrefix = false;
    auto units = "";
    if (plot->mShowLegendUnits)
    {
        useSiUnitPrefix = (metrics[0]->mFlags & ImguiMetric::USE_SI_UNIT_PREFIX) != 0;
        units = metrics[0]->mUnits.c_str();
    }


    // Draw legend, special case for single metric
    //
    // TODO: if nothing, enlarge plot
    ImGui::BeginGroup();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, LEGEND_TEXT_VERTICAL_SPACING));

    if (metrics.size() == 1)
    {
        // ---| Desc
        //    | Max: xxx
        //    | Avg: xxx
        //    | Min: xxx
        //    |
        // ---|
        if (plot->mShowLegendColor)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, *reinterpret_cast<ImVec4*>(&metrics[0]->mColor));
        }
        if (plot->mShowLegendDesc)
        {
            ImGui::TextUnformatted(metrics[0]->mDescription.c_str());
        }
        if (plot->mShowLegendMax)
        {
            DrawQuantityLabel(plotMaxValue, units, "Max: ", useSiUnitPrefix);
        }
        if (plot->mShowLegendAverage)
        {
            for (auto metric : metrics)
            {
                auto plotAvgValue = metric->GetAverageValue();
                DrawQuantityLabel(plotAvgValue, units, "Avg: ", useSiUnitPrefix);
            }
        }
        if (plot->mShowLegendMin)
        {
            DrawQuantityLabel(plotMinValue, units, "Min: ", useSiUnitPrefix);
        }
        if (plot->mShowLegendColor)
        {
            ImGui::PopStyleColor();
        }
    }
    else
    {
        // ---| Max: xxx
        //    | Desc
        //    | Avg: xxx
        //    |
        //    |
        // ---| Min: xxx
        if (plot->mShowLegendMax)
        {
            DrawQuantityLabel(plotMaxValue, units, "Max: ", useSiUnitPrefix);
        }
        if (plot->mShowLegendDesc || plot->mShowLegendAverage)
        {
            // Order series based on value and/or stack order
            std::vector<ImguiMetric*> ordered(metrics.begin(), metrics.end());
            if (plot->mStacked)
            {
                std::reverse(ordered.begin(), ordered.end());
            }
            else
            {
                std::sort(ordered.begin(), ordered.end(), [](ImguiMetric* a, ImguiMetric* b) { return b->GetAverageValue() < a->GetAverageValue(); });
            }
            for (auto metric : ordered)
            {
                if (plot->mShowLegendColor)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, *reinterpret_cast<ImVec4*>(&metric->mColor));
                }
                if (plot->mShowLegendDesc)
                {
                    if (plot->mShowLegendAverage)
                    {
                        char prefix[128];
                        snprintf(prefix, sizeof(prefix), "%s ", metric->mDescription.c_str());
                        auto plotAvgValue = metric->GetAverageValue();
                        DrawQuantityLabel(plotAvgValue, units, prefix, useSiUnitPrefix);
                    }
                    else
                    {
                        ImGui::TextUnformatted(metric->mDescription.c_str());
                    }
                }
                else
                {
                    auto plotAvgValue = metric->GetAverageValue();
                    DrawQuantityLabel(plotAvgValue, units, "Avg: ", useSiUnitPrefix);
                }
                if (plot->mShowLegendColor)
                {
                    ImGui::PopStyleColor();
                }
            }
        }
        if (plot->mShowLegendMin)
        {
            auto cy = window->DC.CursorPos.y;
            auto ty = frame_bb.Max.y - textHeight;
            if (cy < ty)
            {
                ImGui::ItemSize(ImVec2(0.f, ty - cy));
            }
            DrawQuantityLabel(plotMinValue, units, "Min: ", useSiUnitPrefix);
        }
    }


    ImGui::PopStyleVar(1);
    ImGui::EndGroup();
}

}

void ImguiMetricPlot::DrawList()
{
    if (!DrawPrefix(this))
    {
        return;
    }

    auto window = ImGui::GetCurrentWindow();
    auto height = ImGui::GetTextLineHeight();
    auto valueX = ImGui::GetContentRegionAvail().x - window->WindowPadding.x - mWidthInfo->mValueWidth;
    auto barStartX = mWidthInfo->mDescWidth + DESC_HBAR_PADDING;
    auto barEndX = valueX - HBAR_VALUE_PADDING;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 0));

    for (size_t i = 0, N = mMetrics.size(); i < N; ++i)
    {
        auto metric = mMetrics[i];
        auto const& metricRange = mMetricRange[i];

        // Draw description and value
        auto x = window->DC.CursorPos.x;
        auto y = window->DC.CursorPos.y;
        ImGui::Selectable(metric->mDescription.c_str(), &metric->mSelected, ImGuiSelectableFlags_SpanAvailWidth);
        if (valueX >= barStartX)
        {
            auto useSiUnitPrefix = 0 != (metric->mFlags & ImguiMetric::USE_SI_UNIT_PREFIX);
            auto lastValue = metric->GetLastValue();
            ImGui::SameLine(x + valueX - (window->Pos.x - window->Scroll.x));

            DrawQuantityLabel(lastValue, metric->mUnits.c_str(), "", useSiUnitPrefix);

            // Draw bar
            if (barEndX > barStartX)
            {
                auto normalizedValue = metricRange.second > metricRange.first
                                           ? ImSaturate((lastValue - metricRange.first) / (metricRange.second - metricRange.first))
                                           : (lastValue == 0.f ? 0.f : 1.f);
                window->DrawList->AddRectFilled(ImVec2(x + barStartX, y + HBAR_PADDING_TOP),
                                                ImVec2(x + barStartX + normalizedValue * (barEndX - barStartX), y + height - HBAR_PADDING_BOTTOM),
                                                ImGui::GetColorU32(*reinterpret_cast<ImVec4*>(&metric->mColor)), mBarRounding);
            }
        }

        if (mShowInlineGraphs && (!mShowOnlyIfSelected || metric->mSelected))
        {
            std::vector<ImguiMetric*> m(1, metric);
            DrawMetrics(this, m, mInlinePlotRowCount, metricRange.first, metricRange.second);
        }
    }

    ImGui::PopStyleVar();
}

void ImguiMetricPlot::DrawHistory()
{
    if (!DrawPrefix(this))
    {
        return;
    }

    DrawMetrics(this, mMetrics, mPlotRowCount, mMinValue, mMaxValue);
}
}
}

#endif
