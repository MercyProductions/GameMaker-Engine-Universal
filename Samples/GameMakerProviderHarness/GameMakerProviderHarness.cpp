#include "../../Include/AegisGameMakerUniversal.h"

#include <Windows.h>

#include <cstdio>
#include <cstring>

namespace
{
    std::uint32_t AEGIS_GAMEMAKER_CALL SampleInstances(AegisGameMakerInstanceSnapshot* outInstances, std::uint32_t capacity, void*)
    {
        if (!outInstances || capacity < 5)
            return 0;

        AegisGameMakerInstanceSnapshot samples[5] = {};

        samples[0].id = 1001;
        strcpy_s(samples[0].objectName, "obj_player");
        strcpy_s(samples[0].instanceName, "player_0");
        strcpy_s(samples[0].roomName, "rm_debug_arena");
        samples[0].position = { 640.0f, 360.0f, 0.0f };
        samples[0].boundsMin = { 616.0f, 312.0f, 0.0f };
        samples[0].boundsMax = { 664.0f, 408.0f, 0.0f };
        samples[0].layerId = 1;
        samples[0].depth = 0;
        samples[0].visible = 1;
        samples[0].active = 1;
        samples[0].flags = AegisGameMakerInstance_Object | AegisGameMakerInstance_Player | AegisGameMakerInstance_Visible;

        samples[1].id = 1002;
        strcpy_s(samples[1].objectName, "obj_enemy");
        strcpy_s(samples[1].instanceName, "enemy_guard_0");
        strcpy_s(samples[1].roomName, "rm_debug_arena");
        samples[1].position = { 840.0f, 330.0f, 0.0f };
        samples[1].boundsMin = { 816.0f, 286.0f, 0.0f };
        samples[1].boundsMax = { 864.0f, 374.0f, 0.0f };
        samples[1].layerId = 1;
        samples[1].depth = 10;
        samples[1].visible = 1;
        samples[1].active = 1;
        samples[1].flags = AegisGameMakerInstance_Object | AegisGameMakerInstance_Enemy | AegisGameMakerInstance_Visible;

        samples[2].id = 1003;
        strcpy_s(samples[2].objectName, "obj_npc");
        strcpy_s(samples[2].instanceName, "npc_shopkeeper");
        strcpy_s(samples[2].roomName, "rm_debug_arena");
        samples[2].position = { 390.0f, 410.0f, 0.0f };
        samples[2].boundsMin = { 370.0f, 364.0f, 0.0f };
        samples[2].boundsMax = { 410.0f, 456.0f, 0.0f };
        samples[2].layerId = 1;
        samples[2].depth = 20;
        samples[2].visible = 1;
        samples[2].active = 1;
        samples[2].flags = AegisGameMakerInstance_Object | AegisGameMakerInstance_Visible;

        samples[3].id = 1004;
        strcpy_s(samples[3].objectName, "obj_pickup");
        strcpy_s(samples[3].instanceName, "pickup_coin_0");
        strcpy_s(samples[3].roomName, "rm_debug_arena");
        samples[3].position = { 510.0f, 290.0f, 0.0f };
        samples[3].boundsMin = { 498.0f, 278.0f, 0.0f };
        samples[3].boundsMax = { 522.0f, 302.0f, 0.0f };
        samples[3].layerId = 2;
        samples[3].depth = -5;
        samples[3].visible = 1;
        samples[3].active = 1;
        samples[3].flags = AegisGameMakerInstance_Object | AegisGameMakerInstance_Pickup | AegisGameMakerInstance_Visible;

        samples[4].id = 1005;
        strcpy_s(samples[4].objectName, "obj_enemy");
        strcpy_s(samples[4].instanceName, "enemy_offscreen");
        strcpy_s(samples[4].roomName, "rm_debug_arena");
        samples[4].position = { 1420.0f, 700.0f, 0.0f };
        samples[4].boundsMin = { 1396.0f, 656.0f, 0.0f };
        samples[4].boundsMax = { 1444.0f, 744.0f, 0.0f };
        samples[4].layerId = 1;
        samples[4].depth = 10;
        samples[4].visible = 1;
        samples[4].active = 1;
        samples[4].flags = AegisGameMakerInstance_Object | AegisGameMakerInstance_Enemy | AegisGameMakerInstance_Visible;

        std::memcpy(outInstances, samples, sizeof(samples));
        return 5;
    }

    std::int32_t AEGIS_GAMEMAKER_CALL SampleViewport(AegisGameMakerViewport* outViewport, void*)
    {
        if (!outViewport)
            return 0;
        *outViewport = { 0.0f, 0.0f, 1280.0f, 720.0f };
        return 1;
    }

    std::int32_t AEGIS_GAMEMAKER_CALL SampleMatrix(AegisGameMakerMatrix4x4* outMatrix, void*)
    {
        if (!outMatrix)
            return 0;

        *outMatrix = {};
        outMatrix->flags = AegisGameMakerMatrix_ScreenSpace2D;
        return 1;
    }

    template <typename T>
    bool LoadFunction(HMODULE module, const char* name, T& outFunction)
    {
        outFunction = reinterpret_cast<T>(::GetProcAddress(module, name));
        if (!outFunction)
            std::printf("missing export: %s\n", name);
        return outFunction != nullptr;
    }
}

int wmain(int argc, wchar_t** argv)
{
#if defined(_WIN64)
    const wchar_t* defaultDll = L"..\\..\\build\\x64\\Release\\AegisGameMakerUniversal.dll";
#else
    const wchar_t* defaultDll = L"..\\..\\build\\Win32\\Release\\AegisGameMakerUniversal.dll";
#endif

    const wchar_t* dllPath = argc > 1 ? argv[1] : defaultDll;
    HMODULE dll = ::LoadLibraryW(dllPath);
    if (!dll)
    {
        ::wprintf(L"LoadLibrary failed: %ls (%lu)\n", dllPath, ::GetLastError());
        return 1;
    }

    using RegisterInstancesFn = void(__cdecl*)(AegisGameMakerInstanceProvider, void*);
    using RegisterMatrixFn = void(__cdecl*)(AegisGameMakerViewProjectionProvider, void*);
    using RegisterViewportFn = void(__cdecl*)(AegisGameMakerViewportProvider, void*);
    using UpdateFn = int(__cdecl*)();
    using CapabilityFn = int(__cdecl*)(AegisGameMakerCapabilityInfo*);
    using CountFn = std::uint32_t(__cdecl*)();
    using GetInstanceFn = int(__cdecl*)(std::uint32_t, AegisGameMakerInstanceSnapshot*);
    using TimingFn = int(__cdecl*)(AegisGameMakerAdapterTiming*);
    using ProjectFn = int(__cdecl*)(const AegisGameMakerVec3*, AegisGameMakerProjectedPoint*);
    using WriteJsonFn = int(__cdecl*)(const wchar_t*);
    using LoadJsonFn = int(__cdecl*)(const wchar_t*);
    using PrintFn = void(__cdecl*)();

    RegisterInstancesFn registerInstances = nullptr;
    RegisterMatrixFn registerMatrix = nullptr;
    RegisterViewportFn registerViewport = nullptr;
    UpdateFn update = nullptr;
    CapabilityFn getCapability = nullptr;
    CountFn getCount = nullptr;
    GetInstanceFn getInstance = nullptr;
    TimingFn getTiming = nullptr;
    ProjectFn project = nullptr;
    WriteJsonFn writeJson = nullptr;
    LoadJsonFn loadJson = nullptr;
    PrintFn printInstances = nullptr;

    if (!LoadFunction(dll, "AegisGameMaker_RegisterInstanceProvider", registerInstances) ||
        !LoadFunction(dll, "AegisGameMaker_RegisterViewProjectionProvider", registerMatrix) ||
        !LoadFunction(dll, "AegisGameMaker_RegisterViewportProvider", registerViewport) ||
        !LoadFunction(dll, "AegisGameMaker_UpdateProviders", update) ||
        !LoadFunction(dll, "AegisGameMaker_GetCapabilityInfo", getCapability) ||
        !LoadFunction(dll, "AegisGameMaker_GetInstanceCount", getCount) ||
        !LoadFunction(dll, "AegisGameMaker_GetInstanceSnapshot", getInstance) ||
        !LoadFunction(dll, "AegisGameMaker_GetAdapterTiming", getTiming) ||
        !LoadFunction(dll, "AegisGameMaker_ProjectWorldToScreen", project) ||
        !LoadFunction(dll, "AegisGameMaker_WriteSnapshotJson", writeJson) ||
        !LoadFunction(dll, "AegisGameMaker_LoadSnapshotJson", loadJson) ||
        !LoadFunction(dll, "AegisGameMaker_PrintCurrentInstances", printInstances))
    {
        return 2;
    }

    registerInstances(SampleInstances, nullptr);
    registerMatrix(SampleMatrix, nullptr);
    registerViewport(SampleViewport, nullptr);
    update();

    AegisGameMakerCapabilityInfo capability = {};
    getCapability(&capability);
    ::wprintf(L"backend=%ls arch=%ls details=%ls\n", capability.rendererBackend, capability.architecture, capability.details);
    std::printf("instances=%u\n", getCount());

    AegisGameMakerAdapterTiming timing = {};
    getTiming(&timing);
    std::printf("projected=%u clipped=%u frame=%llu\n",
        timing.projectedCount,
        timing.clippedCount,
        static_cast<unsigned long long>(timing.frameId));

    for (std::uint32_t index = 0; index < getCount(); ++index)
    {
        AegisGameMakerInstanceSnapshot instance = {};
        if (!getInstance(index, &instance))
            continue;

        AegisGameMakerProjectedPoint screen = {};
        const int projected = project(&instance.position, &screen);
        std::printf("[%u] %s/%s room=%s screen=(%.1f, %.1f) clipped=%d ok=%d\n",
            index,
            instance.objectName,
            instance.instanceName,
            instance.roomName,
            screen.x,
            screen.y,
            screen.clipped,
            projected);
    }

    printInstances();
    const wchar_t* snapshotPath = L"GameMakerProviderHarness.snapshot.json";
    writeJson(snapshotPath);

    if (loadJson(snapshotPath))
    {
        AegisGameMakerAdapterTiming replayTiming = {};
        getTiming(&replayTiming);
        std::printf("replay instances=%u projected=%u clipped=%u frame=%llu\n",
            getCount(),
            replayTiming.projectedCount,
            replayTiming.clippedCount,
            static_cast<unsigned long long>(replayTiming.frameId));
    }
    return 0;
}
