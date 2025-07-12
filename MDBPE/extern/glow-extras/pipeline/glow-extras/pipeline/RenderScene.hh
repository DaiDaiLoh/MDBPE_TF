#pragma once

#include <vector>

#include <typed-geometry/tg.hh>

#include <glow/fwd.hh>

#include "Settings.hh"
#include "fwd.hh"

namespace glow
{
namespace pipeline
{
enum class ShadowMode
{
    UpdateAlways, // Default: Redraw shadows every frame
    UpdateOnce,   // Draw shadows once, then switch to DontUpdate
    DontUpdate    // Do nothing
};

GLOW_SHARED(class, RenderScene);
class RenderScene
{
public:
    RenderScene() = default;

    // == Background ==
    tg::color3 backgroundColor = tg::color3::black;

    // == Atmosphere and sun ==
    struct
    {
        tg::vec3 direction = normalize(tg::vec3(0.33f, 0.94f, -0.09f));
        tg::color3 color = {1.f, 0.883f, 0.8235f};
        float intensity = 1;
    } sun;

    struct
    {
        float intensity = .25f;                ///< Multiplier for the final atmo scatter
        float density = .001f;                 ///< Fog density
        tg::color3 fogColor = {.5f, .6f, .7f}; ///< Fog color
        float heightFalloffStart = 0.f;        ///< Start of height falloff in world units (0 = World horizon)
        float heightFalloffEnd = 1500.f;       ///< End of the height falloff in world units
        float heightFalloffExponent = 2.73f;   ///< Height falloff function
    } atmoScatter;

    // == AO ==
    struct
    {
        float radius = 1.5f;
        float bias = 0.2f;
        float powerExponent = 2.f;
        float metersToViewSpaceUnits = 1.f;
        float sharpness = 4;
        float smallScaleIntensity = 1.f;
        float largeScaleIntensity = 1.f;
    } ao;

    // == Bloom ==
    float bloomThreshold = 1.5f; ///< The color luminance beyond which colors enter the bloom buffer
    float bloomIntensity = 2.5f; ///< Multiplier for the luminance-normalized color of bloom

    // == Postprocessing ==
    float exposure = 1.f;
    float gamma = 2.24f;
    float contrast = 1.2f;
    float brightness = 1.f;
    float sharpenStrength = .2f;
    bool tonemappingEnabled = true;

    // == Color grading ==
    // A 16x16x16 color lookup table
    // If nullptr, the identity color LUT is used
    SharedTexture3D colorLut = nullptr;

    // == Shadows ==
    struct
    {
        SharedTexture2DArray cascades = nullptr;
        ShadowMode mode = ShadowMode::UpdateAlways;
        float cascadeSplitLambda = 0.95f;
    } shadow;

    // == Edge outline effect ==
    struct
    {
        bool enabled = false;
        float depthThreshold = 3.333f;
        float normalThreshold = 0.65f;
        tg::color3 color = {0, 0, 10 / 255.f};
    } edgeOutline;

#ifdef GLOW_EXTRAS_HAS_IMGUI
public:
    /// Draw an ImGui configuration window to tweak scene parameters
    ///
    /// Optionally leave the window open (No call to ImGui::End)
    /// to allow for custom extensions.
    void imguiConfigWindow(bool leaveWindowOpen = false);
#endif

    GLOW_SHARED_CREATOR(RenderScene);
};
}
}
