#pragma once

#include "AegisUniversalRuntime.h"

#if defined(_MSC_VER)
#define AEGIS_GAMEMAKER_CALL __stdcall
#else
#define AEGIS_GAMEMAKER_CALL
#endif

enum AegisGameMakerMatrixFlags : std::uint32_t
{
    AegisGameMakerMatrix_Auto = 0,
    AegisGameMakerMatrix_RowMajor = 1u << 0,
    AegisGameMakerMatrix_ColumnMajor = 1u << 1,
    AegisGameMakerMatrix_D3DDepth = 1u << 2,
    AegisGameMakerMatrix_OpenGLDepth = 1u << 3,
    AegisGameMakerMatrix_YFlip = 1u << 4,
    AegisGameMakerMatrix_ScreenSpace2D = 1u << 5
};

enum AegisGameMakerInstanceFlags : std::uint32_t
{
    AegisGameMakerInstance_None = 0,
    AegisGameMakerInstance_Object = 1u << 0,
    AegisGameMakerInstance_Player = 1u << 1,
    AegisGameMakerInstance_Enemy = 1u << 2,
    AegisGameMakerInstance_Pickup = 1u << 3,
    AegisGameMakerInstance_Visible = 1u << 4
};

struct AegisGameMakerVec3
{
    float x;
    float y;
    float z;
};

struct AegisGameMakerViewport
{
    float x;
    float y;
    float width;
    float height;
};

struct AegisGameMakerMatrix4x4
{
    float m[16];
    std::uint32_t flags;
};

struct AegisGameMakerInstanceSnapshot
{
    std::uint64_t id;
    char objectName[96];
    char instanceName[96];
    char roomName[96];
    AegisGameMakerVec3 position;
    AegisGameMakerVec3 boundsMin;
    AegisGameMakerVec3 boundsMax;
    std::int32_t layerId;
    std::int32_t depth;
    std::int32_t visible;
    std::int32_t active;
    std::uint32_t flags;
};

struct AegisGameMakerProjectedPoint
{
    float x;
    float y;
    float depth;
    std::int32_t clipped;
};

struct AegisGameMakerAdapterTiming
{
    double instanceProviderMs;
    double matrixProviderMs;
    double viewportProviderMs;
    std::uint32_t instanceCount;
    std::uint32_t projectedCount;
    std::uint32_t clippedCount;
    std::uint64_t frameId;
};

struct AegisGameMakerCapabilityInfo
{
    std::int32_t gameMakerDetected;
    std::int32_t dataWinFound;
    std::int32_t optionsIniFound;
    std::int32_t yyExtensionInterfaceFound;
    std::int32_t steamworksExtensionFound;
    std::int32_t fmodGmsFound;
    std::int32_t gameAnalyticsFound;
    std::int32_t mouseLockExtensionFound;
    std::int32_t instanceProviderRegistered;
    std::int32_t viewProjectionProviderRegistered;
    std::int32_t viewportProviderRegistered;
    std::int32_t viewportValid;
    std::int32_t matrixValid;
    std::int32_t w2sProjectionWorking;
    std::int32_t snapshotReady;
    wchar_t rendererBackend[32];
    wchar_t architecture[16];
    wchar_t details[256];
};

using AegisGameMakerInstanceProvider = std::uint32_t(AEGIS_GAMEMAKER_CALL*)(
    AegisGameMakerInstanceSnapshot* outInstances,
    std::uint32_t capacity,
    void* userData);

using AegisGameMakerViewProjectionProvider = std::int32_t(AEGIS_GAMEMAKER_CALL*)(
    AegisGameMakerMatrix4x4* outMatrix,
    void* userData);

using AegisGameMakerViewportProvider = std::int32_t(AEGIS_GAMEMAKER_CALL*)(
    AegisGameMakerViewport* outViewport,
    void* userData);

AEGIS_UNIVERSAL_API void AegisGameMaker_RegisterInstanceProvider(AegisGameMakerInstanceProvider provider, void* userData);
AEGIS_UNIVERSAL_API void AegisGameMaker_RegisterViewProjectionProvider(AegisGameMakerViewProjectionProvider provider, void* userData);
AEGIS_UNIVERSAL_API void AegisGameMaker_RegisterViewportProvider(AegisGameMakerViewportProvider provider, void* userData);
AEGIS_UNIVERSAL_API int AegisGameMaker_UpdateProviders();
AEGIS_UNIVERSAL_API int AegisGameMaker_SubmitInstanceSnapshots(const AegisGameMakerInstanceSnapshot* instances, std::uint32_t count);
AEGIS_UNIVERSAL_API int AegisGameMaker_SubmitViewProjection(const AegisGameMakerMatrix4x4* matrix);
AEGIS_UNIVERSAL_API int AegisGameMaker_SubmitViewport(const AegisGameMakerViewport* viewport);
AEGIS_UNIVERSAL_API std::uint32_t AegisGameMaker_GetInstanceCount();
AEGIS_UNIVERSAL_API int AegisGameMaker_GetInstanceSnapshot(std::uint32_t index, AegisGameMakerInstanceSnapshot* outInstance);
AEGIS_UNIVERSAL_API int AegisGameMaker_GetAdapterTiming(AegisGameMakerAdapterTiming* outTiming);
AEGIS_UNIVERSAL_API int AegisGameMaker_GetCapabilityInfo(AegisGameMakerCapabilityInfo* outInfo);
AEGIS_UNIVERSAL_API int AegisGameMaker_ProjectWorldToScreen(const AegisGameMakerVec3* world, AegisGameMakerProjectedPoint* outPoint);
AEGIS_UNIVERSAL_API int AegisGameMaker_WriteSnapshotJson(const wchar_t* path);
AEGIS_UNIVERSAL_API int AegisGameMaker_LoadSnapshotJson(const wchar_t* path);
AEGIS_UNIVERSAL_API void AegisGameMaker_PrintCurrentInstances();
