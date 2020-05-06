/*
 * mpUtils
 * ImGuiWindows.cpp
 *
 * @author: Hendrik Schwanekamp
 * @mail:   hendrik.schwanekamp@gmx.net
 *
 * Copyright (c) 2020 Hendrik Schwanekamp
 *
 */

// includes
//--------------------
#include "mpUtils/Graphics/Gui/ImGuiWindows.h"
#include "mpUtils/Graphics/Utils/misc.h"
#include "mpUtils/Graphics/Input.h"
//--------------------

// namespace
//--------------------
namespace mpu {
namespace gph {
//--------------------

void showBasicPerformanceWindow(bool* show, bool drawAsChild)
{
    bool visible;
    if(drawAsChild)
    {
        visible = ImGui::BeginChild("Performance");
    }
    else
    {
        ImGui::SetNextWindowSize(ImVec2(180,130),ImGuiCond_FirstUseEver);
        visible = ImGui::Begin("Performance", show);
    }

    if(visible)
    {
        // settings
        constexpr int plotSize = 100;
        constexpr int averageSize = 5;
        static bool holdPlot = false;

        // plotting data
        static std::vector<float> frametimes(plotSize);
        static int insertIndex = 1;
        if(insertIndex == plotSize)
            insertIndex = 0;

        // calculate average over last 5 frames
        static int averageIndex=0;
        static float averageData[5];
        static float average;
        if(averageIndex==averageSize)
        {
            averageIndex = 0;
            frametimes[insertIndex++] = 0.2f * (averageData[0] + averageData[1] + averageData[2]
                                                + averageData[3] + averageData[4]);
        }
        if(!holdPlot)
            averageData[averageIndex++] = mpu::gph::Input::deltaTime();

        ImGui::Text("Frametime: %f", mpu::gph::Input::deltaTime());
        ImGui::SameLine();
        ImGui::Text("FPS: %f", 1.0f / mpu::gph::Input::deltaTime());

        ImVec2 availSpace = ImGui::GetContentRegionAvail();
        availSpace.y -= ImGui::GetTextLineHeightWithSpacing();
        ImGui::PlotHistogram("plotFrametime", frametimes.data(), 100,insertIndex-1, nullptr, 0, FLT_MAX, availSpace);

        ImGui::Checkbox("hold plot",&holdPlot);
        ImGui::SameLine();

        static bool vsyncOverride;
        if(ImGui::Checkbox("Enable V-Sync (override)", &vsyncOverride))
            mpu::gph::enableVsync(vsyncOverride);
    }

    if(drawAsChild)
        ImGui::EndChild();
    else
        ImGui::End();
}

}}