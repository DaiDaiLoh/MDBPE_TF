#include "RenderScene.hh"

#ifdef GLOW_EXTRAS_HAS_IMGUI
#include <imgui/imgui.h>

void glow::pipeline::RenderScene::imguiConfigWindow(bool leaveWindowOpen)
{
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

    ImGui::Begin("Pipeline Scene Config");

    if (ImGui::TreeNode("Background"))
    {
        ImGui::ColorEdit3("Background Color", tg::data_ptr(backgroundColor));

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Sun and Atmosphere"))
    {
        // ImGui::SliderFloat3("Sun Direction", glm::value_ptr(sunDirection), 0.f, 1.f);
        ImGui::ColorEdit3("Sun Color", tg::data_ptr(sun.color));
        ImGui::SliderFloat("Sun Intensity", &sun.intensity, 0.f, 5.f);
        ImGui::SliderFloat("Shadow Cascade Lambda", &shadow.cascadeSplitLambda, 0.1f, 1.f);

        if (ImGui::TreeNode("AtmoScatter Config"))
        {
            ImGui::SliderFloat("Intensity", &atmoScatter.intensity, 0.f, 5.f);

            ImGui::SliderFloat("Fog Density", &atmoScatter.density, 0.f, 0.01f);
            ImGui::ColorEdit3("Fog Color", tg::data_ptr(atmoScatter.fogColor));
            ImGui::SliderFloat("Height Falloff Start", &atmoScatter.heightFalloffStart, -1000.f, 1000.f);
            ImGui::SliderFloat("Height Falloff End", &atmoScatter.heightFalloffEnd, 0.f, 4000.f);
            ImGui::SliderFloat("Height Falloff Exponent", &atmoScatter.heightFalloffExponent, 0.1f, 10.f);

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Ambient Occlusion"))
    {
        ImGui::PushID("AO");

        ImGui::SliderFloat("Radius", &ao.radius, 0.1f, 5.f);
        ImGui::SliderFloat("Bias", &ao.bias, 0.f, 0.5f);
        ImGui::SliderFloat("Exponent", &ao.powerExponent, 0.f, 10.f);
        ImGui::SliderFloat("Meter Scale", &ao.metersToViewSpaceUnits, 0.f, 10.f);
        ImGui::SliderFloat("Sharpness", &ao.sharpness, 0.f, 15.f);
        ImGui::SliderFloat("Small Scale Intensity", &ao.smallScaleIntensity, 0.f, 10.f);
        ImGui::SliderFloat("Large Scale Intensity", &ao.largeScaleIntensity, 0.f, 10.f);

        ImGui::PopID();
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Bloom"))
    {
        ImGui::SliderFloat("Bloom Threshold", &bloomThreshold, 0.5f, 15.f);
        ImGui::SliderFloat("Bloom Intensity", &bloomIntensity, 0.f, 10.f);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Postprocessing"))
    {
        ImGui::SliderFloat("Exposure", &exposure, 0.f, 4.f);
        ImGui::SliderFloat("Gamma", &gamma, 2.f, 3.f);
        ImGui::SliderFloat("Contrast", &contrast, 0.5f, 2.f);
        ImGui::SliderFloat("Brightness", &brightness, 0.f, 2.f);
        ImGui::SliderFloat("Sharpening", &sharpenStrength, 0.f, 1.f);
        ImGui::Checkbox("Tonemapping", &tonemappingEnabled);

        if (ImGui::TreeNode("Outline"))
        {
            ImGui::PushID("Outline");
            ImGui::Checkbox("Enabled", &edgeOutline.enabled);
            ImGui::SliderFloat("Depth Threshold", &edgeOutline.depthThreshold, 0.f, 5.f);
            ImGui::SliderFloat("Normal Threshold", &edgeOutline.normalThreshold, 0.f, 1.f);
            ImGui::ColorEdit3("Color", tg::data_ptr(edgeOutline.color));

            ImGui::PopID();
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    if (!leaveWindowOpen)
        ImGui::End();
}
#endif
