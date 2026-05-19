#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "AegisGameMakerUniversal.h"
#include "AegisUniversalOverlay.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
    struct Rect2D
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        bool valid = false;
    };

    bool g_drawEnabled = true;
    bool g_drawBoxes = true;
    bool g_drawCornerBoxes = true;
    bool g_drawFilledBoxes = false;
    bool g_drawLines = true;
    bool g_drawLabels = true;
    bool g_hideInactive = true;
    bool g_hideInvisible = true;
    float g_boxThickness = 1.5f;
    float g_lineThickness = 1.25f;
    ImVec4 g_boxColor = ImVec4(1.0f, 0.55f, 0.12f, 1.0f);
    ImVec4 g_lineColor = ImVec4(0.25f, 0.82f, 1.0f, 0.9f);
    ImVec4 g_fillColor = ImVec4(1.0f, 0.55f, 0.12f, 0.12f);

    AegisGameMakerVec3 Add(const AegisGameMakerVec3& a, const AegisGameMakerVec3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    bool BoundsLookAbsolute(const AegisGameMakerInstanceSnapshot& instance)
    {
        return instance.boundsMin.x <= instance.position.x && instance.position.x <= instance.boundsMax.x &&
            instance.boundsMin.y <= instance.position.y && instance.position.y <= instance.boundsMax.y;
    }

    bool ProjectBounds(const AegisGameMakerInstanceSnapshot& instance, Rect2D& rect, std::uint32_t& projected, std::uint32_t& clipped)
    {
        const bool absoluteBounds = BoundsLookAbsolute(instance);
        const AegisGameMakerVec3 minWorld = absoluteBounds ? instance.boundsMin : Add(instance.position, instance.boundsMin);
        const AegisGameMakerVec3 maxWorld = absoluteBounds ? instance.boundsMax : Add(instance.position, instance.boundsMax);

        const std::array<AegisGameMakerVec3, 8> corners = {
            AegisGameMakerVec3{ minWorld.x, minWorld.y, minWorld.z },
            AegisGameMakerVec3{ maxWorld.x, minWorld.y, minWorld.z },
            AegisGameMakerVec3{ minWorld.x, maxWorld.y, minWorld.z },
            AegisGameMakerVec3{ maxWorld.x, maxWorld.y, minWorld.z },
            AegisGameMakerVec3{ minWorld.x, minWorld.y, maxWorld.z },
            AegisGameMakerVec3{ maxWorld.x, minWorld.y, maxWorld.z },
            AegisGameMakerVec3{ minWorld.x, maxWorld.y, maxWorld.z },
            AegisGameMakerVec3{ maxWorld.x, maxWorld.y, maxWorld.z }
        };

        rect = {};
        for (const AegisGameMakerVec3& corner : corners)
        {
            AegisGameMakerProjectedPoint point = {};
            if (!AegisGameMaker_ProjectWorldToScreen(&corner, &point) || point.clipped ||
                !std::isfinite(point.x) || !std::isfinite(point.y))
            {
                ++clipped;
                continue;
            }

            ++projected;
            if (!rect.valid)
            {
                rect.minX = rect.maxX = point.x;
                rect.minY = rect.maxY = point.y;
                rect.valid = true;
            }
            else
            {
                rect.minX = std::min(rect.minX, point.x);
                rect.minY = std::min(rect.minY, point.y);
                rect.maxX = std::max(rect.maxX, point.x);
                rect.maxY = std::max(rect.maxY, point.y);
            }
        }

        return rect.valid && rect.maxX > rect.minX && rect.maxY > rect.minY;
    }

    void DrawCornerBox(ImDrawList* drawList, const Rect2D& rect, ImU32 color)
    {
        const float width = rect.maxX - rect.minX;
        const float height = rect.maxY - rect.minY;
        const float x = rect.minX;
        const float y = rect.minY;
        const float w = width * 0.25f;
        const float h = height * 0.22f;
        drawList->AddLine(ImVec2(x, y), ImVec2(x + w, y), color, g_boxThickness);
        drawList->AddLine(ImVec2(x, y), ImVec2(x, y + h), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y), ImVec2(x + width - w, y), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y), ImVec2(x + width, y + h), color, g_boxThickness);
        drawList->AddLine(ImVec2(x, y + height), ImVec2(x + w, y + height), color, g_boxThickness);
        drawList->AddLine(ImVec2(x, y + height), ImVec2(x, y + height - h), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y + height), ImVec2(x + width - w, y + height), color, g_boxThickness);
        drawList->AddLine(ImVec2(x + width, y + height), ImVec2(x + width, y + height - h), color, g_boxThickness);
    }

    void DrawInstanceOverlay()
    {
        if (!g_drawEnabled)
            return;

        const std::uint32_t count = std::min<std::uint32_t>(AegisGameMaker_GetInstanceCount(), 512);
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(g_boxColor);
        const ImU32 lineColor = ImGui::ColorConvertFloat4ToU32(g_lineColor);
        const ImU32 fillColor = ImGui::ColorConvertFloat4ToU32(g_fillColor);

        std::uint32_t projected = 0;
        std::uint32_t clipped = 0;
        for (std::uint32_t i = 0; i < count; ++i)
        {
            AegisGameMakerInstanceSnapshot instance = {};
            if (!AegisGameMaker_GetInstanceSnapshot(i, &instance))
                continue;
            if (g_hideInactive && !instance.active)
                continue;
            if (g_hideInvisible && !instance.visible)
                continue;

            Rect2D rect = {};
            if (!ProjectBounds(instance, rect, projected, clipped))
                continue;

            if (g_drawFilledBoxes)
                drawList->AddRectFilled(ImVec2(rect.minX, rect.minY), ImVec2(rect.maxX, rect.maxY), fillColor);
            if (g_drawBoxes)
                drawList->AddRect(ImVec2(rect.minX, rect.minY), ImVec2(rect.maxX, rect.maxY), boxColor, 0.0f, 0, g_boxThickness);
            if (g_drawCornerBoxes)
                DrawCornerBox(drawList, rect, boxColor);
            if (g_drawLines)
                drawList->AddLine(ImVec2(display.x * 0.5f, display.y), ImVec2((rect.minX + rect.maxX) * 0.5f, rect.maxY), lineColor, g_lineThickness);
            if (g_drawLabels)
                drawList->AddText(ImVec2(rect.minX, std::max(0.0f, rect.minY - 16.0f)), boxColor, instance.instanceName[0] ? instance.instanceName : instance.objectName);
        }
    }

    void DrawInstanceTable()
    {
        const std::uint32_t count = std::min<std::uint32_t>(AegisGameMaker_GetInstanceCount(), 128);
        if (ImGui::BeginTable("gamemaker-instances", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupColumn("Object");
            ImGui::TableSetupColumn("Instance");
            ImGui::TableSetupColumn("Room");
            ImGui::TableSetupColumn("Position");
            ImGui::TableSetupColumn("Visible");
            ImGui::TableSetupColumn("Active");
            ImGui::TableSetupColumn("Flags");
            ImGui::TableHeadersRow();
            for (std::uint32_t i = 0; i < count; ++i)
            {
                AegisGameMakerInstanceSnapshot instance = {};
                if (!AegisGameMaker_GetInstanceSnapshot(i, &instance))
                    continue;
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%llu", static_cast<unsigned long long>(instance.id));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(instance.objectName);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(instance.instanceName);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(instance.roomName);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f, %.2f, %.2f", instance.position.x, instance.position.y, instance.position.z);
                ImGui::TableNextColumn();
                ImGui::Text("%s", instance.visible ? "yes" : "no");
                ImGui::TableNextColumn();
                ImGui::Text("%s", instance.active ? "yes" : "no");
                ImGui::TableNextColumn();
                ImGui::Text("0x%08x", instance.flags);
            }
            ImGui::EndTable();
        }
    }
}

extern "C" const char* AegisUniversalOverlay_GetEngineOverlayName()
{
    return "GameMaker";
}

extern "C" void AegisUniversalOverlay_PollEngineProviders()
{
    AegisGameMaker_UpdateProviders();
}

extern "C" void AegisUniversalOverlay_DrawEngineOverlay()
{
    DrawInstanceOverlay();
}

extern "C" void AegisUniversalOverlay_DrawEngineMenu()
{
    if (ImGui::BeginTabItem("Adapter"))
    {
        AegisGameMakerCapabilityInfo capability = {};
        AegisGameMaker_GetCapabilityInfo(&capability);
        AegisGameMakerAdapterTiming timing = {};
        AegisGameMaker_GetAdapterTiming(&timing);

        ImGui::Text("Renderer backend: %ls | architecture: %ls", capability.rendererBackend, capability.architecture);
        ImGui::TextWrapped("Details: %ls", capability.details);
        ImGui::Separator();
        ImGui::Text("GameMaker detected: %s | data.win: %s | options.ini: %s",
            capability.gameMakerDetected ? "pass" : "warn",
            capability.dataWinFound ? "pass" : "warn",
            capability.optionsIniFound ? "pass" : "warn");
        ImGui::Text("YY extension API: %s | Steamworks: %s | FMOD: %s",
            capability.yyExtensionInterfaceFound ? "pass" : "warn",
            capability.steamworksExtensionFound ? "pass" : "warn",
            capability.fmodGmsFound ? "pass" : "warn");
        ImGui::Text("Instance provider: %s | Matrix provider: %s | Viewport provider: %s",
            capability.instanceProviderRegistered ? "pass" : "warn",
            capability.viewProjectionProviderRegistered ? "pass" : "warn",
            capability.viewportProviderRegistered ? "pass" : "warn");
        ImGui::Text("Viewport valid: %s | Matrix valid: %s | W2S: %s",
            capability.viewportValid ? "pass" : "warn",
            capability.matrixValid ? "pass" : "warn",
            capability.w2sProjectionWorking ? "pass" : "warn");
        ImGui::Text("Frame %llu | instances %u | projected %u | clipped %u",
            static_cast<unsigned long long>(timing.frameId),
            timing.instanceCount,
            timing.projectedCount,
            timing.clippedCount);
        ImGui::Text("Provider timing: instances %.3f ms | matrix %.3f ms | viewport %.3f ms",
            timing.instanceProviderMs,
            timing.matrixProviderMs,
            timing.viewportProviderMs);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Overlay"))
    {
        ImGui::Checkbox("Draw provider overlay", &g_drawEnabled);
        ImGui::Checkbox("Hide inactive instances", &g_hideInactive);
        ImGui::Checkbox("Hide invisible instances", &g_hideInvisible);
        ImGui::Checkbox("Boxes", &g_drawBoxes);
        ImGui::Checkbox("Corner boxes", &g_drawCornerBoxes);
        ImGui::Checkbox("Filled boxes", &g_drawFilledBoxes);
        ImGui::Checkbox("Lines", &g_drawLines);
        ImGui::Checkbox("Labels", &g_drawLabels);
        ImGui::SliderFloat("Box thickness", &g_boxThickness, 0.5f, 6.0f, "%.1f");
        ImGui::SliderFloat("Line thickness", &g_lineThickness, 0.5f, 6.0f, "%.1f");
        ImGui::ColorEdit4("Box color", &g_boxColor.x);
        ImGui::ColorEdit4("Line color", &g_lineColor.x);
        ImGui::ColorEdit4("Fill color", &g_fillColor.x);
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Instances"))
    {
        ImGui::Text("Instance count: %u", AegisGameMaker_GetInstanceCount());
        if (ImGui::Button("Print current instances to console"))
            AegisGameMaker_PrintCurrentInstances();
        DrawInstanceTable();
        ImGui::EndTabItem();
    }
}
