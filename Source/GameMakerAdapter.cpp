#include "AegisGameMakerUniversal.h"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    constexpr std::uint32_t kMaxInstances = 8192;

    struct ProviderState
    {
        AegisGameMakerInstanceProvider instanceProvider = nullptr;
        void* instanceUserData = nullptr;
        AegisGameMakerViewProjectionProvider matrixProvider = nullptr;
        void* matrixUserData = nullptr;
        AegisGameMakerViewportProvider viewportProvider = nullptr;
        void* viewportUserData = nullptr;
    };

    std::mutex g_adapterMutex;
    ProviderState g_providers;
    std::vector<AegisGameMakerInstanceSnapshot> g_instances;
    AegisGameMakerMatrix4x4 g_viewProjection = {};
    AegisGameMakerViewport g_viewport = {};
    AegisGameMakerAdapterTiming g_timing = {};
    bool g_hasMatrix = false;
    bool g_hasViewport = false;

    template <std::size_t N>
    void CopyWide(wchar_t (&dest)[N], const wchar_t* value)
    {
        wcsncpy_s(dest, value ? value : L"", _TRUNCATE);
    }

    template <std::size_t N>
    void CopyAnsi(char (&dest)[N], const std::string& value)
    {
        strncpy_s(dest, value.c_str(), _TRUNCATE);
    }

    bool IsFinite(float value)
    {
        return std::isfinite(value);
    }

    bool IsValidViewport(const AegisGameMakerViewport& viewport)
    {
        return IsFinite(viewport.x) && IsFinite(viewport.y) &&
            IsFinite(viewport.width) && IsFinite(viewport.height) &&
            viewport.width > 1.0f && viewport.height > 1.0f;
    }

    bool IsValidMatrix(const AegisGameMakerMatrix4x4& matrix)
    {
        if ((matrix.flags & AegisGameMakerMatrix_ScreenSpace2D) != 0)
            return true;

        bool anyNonZero = false;
        for (float value : matrix.m)
        {
            if (!IsFinite(value))
                return false;
            anyNonZero = anyNonZero || std::fabs(value) > 0.000001f;
        }
        return anyNonZero;
    }

    LARGE_INTEGER NowCounter()
    {
        LARGE_INTEGER value = {};
        ::QueryPerformanceCounter(&value);
        return value;
    }

    double ElapsedMs(LARGE_INTEGER start, LARGE_INTEGER end)
    {
        LARGE_INTEGER frequency = {};
        ::QueryPerformanceFrequency(&frequency);
        if (frequency.QuadPart == 0)
            return 0.0;
        return (static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0) /
            static_cast<double>(frequency.QuadPart);
    }

    std::wstring Lower(std::wstring value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
            return static_cast<wchar_t>(::towlower(ch));
        });
        return value;
    }

    bool Contains(const std::wstring& value, const wchar_t* needle)
    {
        return needle && Lower(value).find(Lower(needle)) != std::wstring::npos;
    }

    bool ContainsAnsi(const char* value, const char* needle)
    {
        if (!value || !needle)
            return false;

        std::string haystack = value;
        std::string target = needle;
        std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        std::transform(target.begin(), target.end(), target.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return haystack.find(target) != std::string::npos;
    }

    std::filesystem::path ProcessDirectory()
    {
        wchar_t path[MAX_PATH] = {};
        const DWORD length = ::GetModuleFileNameW(nullptr, path, MAX_PATH);
        if (length == 0)
            return {};
        return std::filesystem::path(std::wstring(path, length)).parent_path();
    }

    bool ProcessSiblingExists(const wchar_t* name)
    {
        const std::filesystem::path dir = ProcessDirectory();
        if (dir.empty())
            return false;
        return std::filesystem::exists(dir / name);
    }

    bool ProjectScreenSpaceLocked(const AegisGameMakerVec3& world, AegisGameMakerProjectedPoint& outPoint)
    {
        if (!g_hasViewport || !IsValidViewport(g_viewport))
            return false;

        outPoint.x = g_viewport.x + world.x;
        outPoint.y = g_viewport.y + world.y;
        outPoint.depth = world.z;
        outPoint.clipped = (outPoint.x < g_viewport.x ||
            outPoint.x > g_viewport.x + g_viewport.width ||
            outPoint.y < g_viewport.y ||
            outPoint.y > g_viewport.y + g_viewport.height) ? 1 : 0;
        return IsFinite(outPoint.x) && IsFinite(outPoint.y);
    }

    bool ProjectWithConvention(
        const AegisGameMakerMatrix4x4& matrix,
        const AegisGameMakerViewport& viewport,
        const AegisGameMakerVec3& world,
        bool columnMajor,
        bool openGlDepth,
        bool yFlip,
        AegisGameMakerProjectedPoint& outPoint)
    {
        const float* m = matrix.m;
        float clipX = 0.0f;
        float clipY = 0.0f;
        float clipZ = 0.0f;
        float clipW = 0.0f;

        if (columnMajor)
        {
            clipX = world.x * m[0] + world.y * m[4] + world.z * m[8] + m[12];
            clipY = world.x * m[1] + world.y * m[5] + world.z * m[9] + m[13];
            clipZ = world.x * m[2] + world.y * m[6] + world.z * m[10] + m[14];
            clipW = world.x * m[3] + world.y * m[7] + world.z * m[11] + m[15];
        }
        else
        {
            clipX = world.x * m[0] + world.y * m[1] + world.z * m[2] + m[3];
            clipY = world.x * m[4] + world.y * m[5] + world.z * m[6] + m[7];
            clipZ = world.x * m[8] + world.y * m[9] + world.z * m[10] + m[11];
            clipW = world.x * m[12] + world.y * m[13] + world.z * m[14] + m[15];
        }

        if (!IsFinite(clipX) || !IsFinite(clipY) || !IsFinite(clipZ) || !IsFinite(clipW) || std::fabs(clipW) < 0.00001f)
            return false;

        const float ndcX = clipX / clipW;
        const float ndcY = clipY / clipW;
        const float ndcZ = clipZ / clipW;
        if (!IsFinite(ndcX) || !IsFinite(ndcY) || !IsFinite(ndcZ))
            return false;

        outPoint.x = viewport.x + ((ndcX * 0.5f) + 0.5f) * viewport.width;
        outPoint.y = viewport.y + (yFlip ? ((ndcY * 0.5f) + 0.5f) : (0.5f - (ndcY * 0.5f))) * viewport.height;
        outPoint.depth = ndcZ;

        const bool depthClipped = openGlDepth ? (ndcZ < -1.0f || ndcZ > 1.0f) : (ndcZ < 0.0f || ndcZ > 1.0f);
        const bool xyClipped = ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f;
        outPoint.clipped = (xyClipped || depthClipped || clipW < 0.0f) ? 1 : 0;
        return IsFinite(outPoint.x) && IsFinite(outPoint.y);
    }

    bool ProjectCurrentLocked(const AegisGameMakerVec3& world, AegisGameMakerProjectedPoint& outPoint)
    {
        if (!g_hasViewport || !IsValidViewport(g_viewport))
            return false;

        if (!g_hasMatrix || (g_viewProjection.flags & AegisGameMakerMatrix_ScreenSpace2D) != 0)
            return ProjectScreenSpaceLocked(world, outPoint);

        if (!IsValidMatrix(g_viewProjection))
            return false;

        const std::uint32_t flags = g_viewProjection.flags;
        const bool wantsColumn = (flags & AegisGameMakerMatrix_ColumnMajor) != 0;
        const bool wantsRow = (flags & AegisGameMakerMatrix_RowMajor) != 0;
        const bool openGlDepth = (flags & AegisGameMakerMatrix_OpenGLDepth) != 0;
        const bool yFlip = (flags & AegisGameMakerMatrix_YFlip) != 0;

        if (wantsColumn || wantsRow)
            return ProjectWithConvention(g_viewProjection, g_viewport, world, wantsColumn, openGlDepth, yFlip, outPoint);

        AegisGameMakerProjectedPoint row = {};
        if (ProjectWithConvention(g_viewProjection, g_viewport, world, false, openGlDepth, yFlip, row) && !row.clipped)
        {
            outPoint = row;
            return true;
        }

        AegisGameMakerProjectedPoint column = {};
        if (ProjectWithConvention(g_viewProjection, g_viewport, world, true, openGlDepth, yFlip, column))
        {
            outPoint = column;
            return true;
        }

        if (ProjectWithConvention(g_viewProjection, g_viewport, world, false, openGlDepth, yFlip, row))
        {
            outPoint = row;
            return true;
        }
        return false;
    }

    void RebuildProjectionStatsLocked()
    {
        g_timing.instanceCount = static_cast<std::uint32_t>(g_instances.size());
        g_timing.projectedCount = 0;
        g_timing.clippedCount = 0;

        for (const AegisGameMakerInstanceSnapshot& instance : g_instances)
        {
            AegisGameMakerProjectedPoint point = {};
            if (!ProjectCurrentLocked(instance.position, point))
                continue;
            if (point.clipped)
                ++g_timing.clippedCount;
            else
                ++g_timing.projectedCount;
        }
    }

    std::string JsonEscape(const char* value)
    {
        std::string escaped;
        if (!value)
            return escaped;

        for (const char* cursor = value; *cursor; ++cursor)
        {
            switch (*cursor)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(*cursor);
                break;
            }
        }
        return escaped;
    }

    std::string ExtractJsonString(const std::string& object, const char* key)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return {};
        pos = object.find(':', pos);
        pos = object.find('"', pos);
        if (pos == std::string::npos)
            return {};
        ++pos;

        std::string result;
        for (; pos < object.size(); ++pos)
        {
            const char ch = object[pos];
            if (ch == '\\' && pos + 1 < object.size())
            {
                result.push_back(object[pos + 1]);
                ++pos;
                continue;
            }
            if (ch == '"')
                break;
            result.push_back(ch);
        }
        return result;
    }

    bool ExtractJsonNumber(const std::string& object, const char* key, float& outValue)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return false;
        pos = object.find(':', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        const float value = std::strtof(object.c_str() + pos + 1, &end);
        if (end == object.c_str() + pos + 1 || !IsFinite(value))
            return false;
        outValue = value;
        return true;
    }

    bool ExtractJsonUInt64(const std::string& object, const char* key, std::uint64_t& outValue)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return false;
        pos = object.find(':', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        const unsigned long long value = std::strtoull(object.c_str() + pos + 1, &end, 10);
        if (end == object.c_str() + pos + 1)
            return false;
        outValue = static_cast<std::uint64_t>(value);
        return true;
    }

    bool ExtractJsonInt(const std::string& object, const char* key, std::int32_t& outValue)
    {
        float value = 0.0f;
        if (!ExtractJsonNumber(object, key, value))
            return false;
        outValue = static_cast<std::int32_t>(value);
        return true;
    }

    bool ExtractJsonVec3(const std::string& object, const char* key, AegisGameMakerVec3& outValue)
    {
        const std::string token = std::string("\"") + key + "\"";
        std::size_t pos = object.find(token);
        if (pos == std::string::npos)
            return false;
        pos = object.find('[', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        outValue.x = std::strtof(object.c_str() + pos + 1, &end);
        if (!end || *end == '\0')
            return false;
        outValue.y = std::strtof(end + 1, &end);
        if (!end || *end == '\0')
            return false;
        outValue.z = std::strtof(end + 1, &end);
        return IsFinite(outValue.x) && IsFinite(outValue.y) && IsFinite(outValue.z);
    }

    bool ExtractJsonMatrix(const std::string& json, AegisGameMakerMatrix4x4& outMatrix)
    {
        std::size_t pos = json.find("\"m\"");
        if (pos == std::string::npos)
            return false;
        pos = json.find('[', pos);
        if (pos == std::string::npos)
            return false;

        char* end = nullptr;
        const char* cursor = json.c_str() + pos + 1;
        for (float& value : outMatrix.m)
        {
            value = std::strtof(cursor, &end);
            if (end == cursor || !IsFinite(value))
                return false;
            cursor = end + 1;
        }

        float flags = 0.0f;
        if (ExtractJsonNumber(json, "matrixFlags", flags))
            outMatrix.flags = static_cast<std::uint32_t>(flags);
        return true;
    }

    std::wstring DetectRendererBackend()
    {
        if (!AegisUniversal_IsInitialized())
            AegisUniversal_Initialize();

        bool hasD3D9 = false;
        bool hasD3D11 = false;
        bool hasD3D12 = false;
        bool hasOpenGl = false;
        const std::uint32_t count = AegisUniversal_GetModuleCount();
        for (std::uint32_t index = 0; index < count; ++index)
        {
            AegisUniversalModuleInfo module = {};
            if (!AegisUniversal_GetModuleInfo(index, &module))
                continue;
            const std::wstring name = module.name;
            hasD3D9 = hasD3D9 || Contains(name, L"d3d9") || Contains(name, L"d3dx9");
            hasD3D11 = hasD3D11 || Contains(name, L"d3d11") || Contains(name, L"dxgi");
            hasD3D12 = hasD3D12 || Contains(name, L"d3d12");
            hasOpenGl = hasOpenGl || Contains(name, L"opengl32");
        }

        if (hasD3D12)
            return L"Direct3D12";
        if (hasD3D11)
            return L"Direct3D11";
        if (hasD3D9)
            return L"Direct3D9";
        if (hasOpenGl)
            return L"OpenGL";
        return L"Unknown";
    }
}

AEGIS_UNIVERSAL_API void AegisGameMaker_RegisterInstanceProvider(AegisGameMakerInstanceProvider provider, void* userData)
{
    std::lock_guard lock(g_adapterMutex);
    g_providers.instanceProvider = provider;
    g_providers.instanceUserData = userData;
}

AEGIS_UNIVERSAL_API void AegisGameMaker_RegisterViewProjectionProvider(AegisGameMakerViewProjectionProvider provider, void* userData)
{
    std::lock_guard lock(g_adapterMutex);
    g_providers.matrixProvider = provider;
    g_providers.matrixUserData = userData;
}

AEGIS_UNIVERSAL_API void AegisGameMaker_RegisterViewportProvider(AegisGameMakerViewportProvider provider, void* userData)
{
    std::lock_guard lock(g_adapterMutex);
    g_providers.viewportProvider = provider;
    g_providers.viewportUserData = userData;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_UpdateProviders()
{
    ProviderState providers = {};
    {
        std::lock_guard lock(g_adapterMutex);
        providers = g_providers;
    }

    std::vector<AegisGameMakerInstanceSnapshot> instances(kMaxInstances);
    std::uint32_t instanceCount = 0;
    double instanceMs = 0.0;
    double matrixMs = 0.0;
    double viewportMs = 0.0;
    bool hasMatrix = false;
    bool hasViewport = false;
    AegisGameMakerMatrix4x4 matrix = {};
    AegisGameMakerViewport viewport = {};

    if (providers.viewportProvider)
    {
        const LARGE_INTEGER start = NowCounter();
        hasViewport = providers.viewportProvider(&viewport, providers.viewportUserData) != 0 && IsValidViewport(viewport);
        viewportMs = ElapsedMs(start, NowCounter());
    }

    if (providers.matrixProvider)
    {
        const LARGE_INTEGER start = NowCounter();
        hasMatrix = providers.matrixProvider(&matrix, providers.matrixUserData) != 0 && IsValidMatrix(matrix);
        matrixMs = ElapsedMs(start, NowCounter());
    }

    if (providers.instanceProvider)
    {
        const LARGE_INTEGER start = NowCounter();
        instanceCount = providers.instanceProvider(instances.data(), kMaxInstances, providers.instanceUserData);
        instanceMs = ElapsedMs(start, NowCounter());
        instanceCount = std::min<std::uint32_t>(instanceCount, kMaxInstances);
        instances.resize(instanceCount);
    }
    else
    {
        instances.clear();
    }

    std::lock_guard lock(g_adapterMutex);
    if (providers.instanceProvider)
        g_instances = std::move(instances);
    if (providers.matrixProvider)
    {
        g_viewProjection = matrix;
        g_hasMatrix = hasMatrix;
    }
    if (providers.viewportProvider)
    {
        g_viewport = viewport;
        g_hasViewport = hasViewport;
    }
    g_timing.instanceProviderMs = instanceMs;
    g_timing.matrixProviderMs = matrixMs;
    g_timing.viewportProviderMs = viewportMs;
    ++g_timing.frameId;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_SubmitInstanceSnapshots(const AegisGameMakerInstanceSnapshot* instances, std::uint32_t count)
{
    if (!instances && count != 0)
        return 0;

    const std::uint32_t clampedCount = std::min<std::uint32_t>(count, kMaxInstances);
    std::lock_guard lock(g_adapterMutex);
    g_instances.assign(instances, instances + clampedCount);
    ++g_timing.frameId;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_SubmitViewProjection(const AegisGameMakerMatrix4x4* matrix)
{
    if (!matrix || !IsValidMatrix(*matrix))
        return 0;

    std::lock_guard lock(g_adapterMutex);
    g_viewProjection = *matrix;
    g_hasMatrix = true;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_SubmitViewport(const AegisGameMakerViewport* viewport)
{
    if (!viewport || !IsValidViewport(*viewport))
        return 0;

    std::lock_guard lock(g_adapterMutex);
    g_viewport = *viewport;
    g_hasViewport = true;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API std::uint32_t AegisGameMaker_GetInstanceCount()
{
    std::lock_guard lock(g_adapterMutex);
    return static_cast<std::uint32_t>(g_instances.size());
}

AEGIS_UNIVERSAL_API int AegisGameMaker_GetInstanceSnapshot(std::uint32_t index, AegisGameMakerInstanceSnapshot* outInstance)
{
    if (!outInstance)
        return 0;

    std::lock_guard lock(g_adapterMutex);
    if (index >= g_instances.size())
        return 0;
    *outInstance = g_instances[index];
    return 1;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_GetAdapterTiming(AegisGameMakerAdapterTiming* outTiming)
{
    if (!outTiming)
        return 0;
    std::lock_guard lock(g_adapterMutex);
    *outTiming = g_timing;
    return 1;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_GetCapabilityInfo(AegisGameMakerCapabilityInfo* outInfo)
{
    if (!outInfo)
        return 0;

    if (!AegisUniversal_IsInitialized())
        AegisUniversal_Initialize();

    AegisUniversalRuntimeInfo runtime = {};
    AegisUniversal_GetRuntimeInfo(&runtime);

    bool yyExtension = false;
    bool steamworks = false;
    bool fmod = false;
    bool analytics = false;
    bool mouseLock = false;
    const std::uint32_t exportCount = AegisUniversal_GetMatchedExportCount();
    for (std::uint32_t index = 0; index < exportCount; ++index)
    {
        AegisUniversalExportInfo exportInfo = {};
        if (!AegisUniversal_GetMatchedExportInfo(index, &exportInfo))
            continue;

        yyExtension = yyExtension || ContainsAnsi(exportInfo.exportName, "YYExtensionInitialise");
        steamworks = steamworks || ContainsAnsi(exportInfo.exportName, "steam_gml") || ContainsAnsi(exportInfo.exportName, "RegisterCallbacks");
        fmod = fmod || ContainsAnsi(exportInfo.exportName, "FMODGMS");
        analytics = analytics || ContainsAnsi(exportInfo.exportName, "addDesignEvent") || ContainsAnsi(exportInfo.exportName, "gameAnalytics");
        mouseLock = mouseLock || ContainsAnsi(exportInfo.exportName, "display_mouse_lock");
    }

    const std::uint32_t moduleCount = AegisUniversal_GetModuleCount();
    for (std::uint32_t index = 0; index < moduleCount; ++index)
    {
        AegisUniversalModuleInfo module = {};
        if (!AegisUniversal_GetModuleInfo(index, &module))
            continue;
        const std::wstring name = module.name;
        steamworks = steamworks || Contains(name, L"Steamworks");
        fmod = fmod || Contains(name, L"FMODGMS") || Contains(name, L"fmod");
        analytics = analytics || Contains(name, L"GameAnalytics");
        mouseLock = mouseLock || Contains(name, L"display_mouse_lock");
    }

    std::lock_guard lock(g_adapterMutex);
    *outInfo = {};
    outInfo->dataWinFound = ProcessSiblingExists(L"data.win") ? 1 : 0;
    outInfo->optionsIniFound = ProcessSiblingExists(L"options.ini") ? 1 : 0;
    outInfo->gameMakerDetected = ((runtime.flags & AegisUniversalRuntime_EngineDetected) || outInfo->dataWinFound) ? 1 : 0;
    outInfo->yyExtensionInterfaceFound = yyExtension ? 1 : 0;
    outInfo->steamworksExtensionFound = steamworks ? 1 : 0;
    outInfo->fmodGmsFound = fmod ? 1 : 0;
    outInfo->gameAnalyticsFound = analytics ? 1 : 0;
    outInfo->mouseLockExtensionFound = mouseLock ? 1 : 0;
    outInfo->instanceProviderRegistered = g_providers.instanceProvider ? 1 : 0;
    outInfo->viewProjectionProviderRegistered = g_providers.matrixProvider ? 1 : 0;
    outInfo->viewportProviderRegistered = g_providers.viewportProvider ? 1 : 0;
    outInfo->viewportValid = (g_hasViewport && IsValidViewport(g_viewport)) ? 1 : 0;
    outInfo->matrixValid = (g_hasMatrix && IsValidMatrix(g_viewProjection)) ? 1 : 0;
    outInfo->snapshotReady = !g_instances.empty() ? 1 : 0;

    AegisGameMakerProjectedPoint point = {};
    const AegisGameMakerVec3 origin = { 0.0f, 0.0f, 0.0f };
    outInfo->w2sProjectionWorking = ProjectCurrentLocked(origin, point) ? 1 : 0;
    CopyWide(outInfo->rendererBackend, DetectRendererBackend().c_str());
#if defined(_WIN64)
    CopyWide(outInfo->architecture, L"x64");
#else
    CopyWide(outInfo->architecture, L"Win32");
#endif

    std::wstringstream details;
    details << L"modules " << runtime.moduleCount
            << L", matched exports " << runtime.matchedExportCount
            << L", data.win " << (outInfo->dataWinFound ? L"yes" : L"no")
            << L", instances " << g_instances.size()
            << L", projected " << g_timing.projectedCount
            << L", clipped " << g_timing.clippedCount;
    CopyWide(outInfo->details, details.str().c_str());
    return 1;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_ProjectWorldToScreen(const AegisGameMakerVec3* world, AegisGameMakerProjectedPoint* outPoint)
{
    if (!world || !outPoint)
        return 0;

    std::lock_guard lock(g_adapterMutex);
    *outPoint = {};
    return ProjectCurrentLocked(*world, *outPoint) ? 1 : 0;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_WriteSnapshotJson(const wchar_t* path)
{
    if (!path || !path[0])
        return 0;

    std::lock_guard lock(g_adapterMutex);
    std::wofstream out(std::filesystem::path(path), std::ios::trunc);
    if (!out)
        return 0;

    out << L"{\n";
    out << L"  \"engine\": \"GameMaker\",\n";
    out << L"  \"frameId\": " << g_timing.frameId << L",\n";
    out << L"  \"viewport\": { \"x\": " << g_viewport.x << L", \"y\": " << g_viewport.y
        << L", \"width\": " << g_viewport.width << L", \"height\": " << g_viewport.height << L" },\n";
    out << L"  \"matrixFlags\": " << g_viewProjection.flags << L",\n";
    out << L"  \"m\": [";
    for (std::size_t index = 0; index < 16; ++index)
    {
        if (index)
            out << L", ";
        out << g_viewProjection.m[index];
    }
    out << L"],\n";
    out << L"  \"timing\": { \"instanceProviderMs\": " << g_timing.instanceProviderMs
        << L", \"matrixProviderMs\": " << g_timing.matrixProviderMs
        << L", \"viewportProviderMs\": " << g_timing.viewportProviderMs
        << L", \"projected\": " << g_timing.projectedCount
        << L", \"clipped\": " << g_timing.clippedCount << L" },\n";
    out << L"  \"instances\": [\n";
    for (std::size_t index = 0; index < g_instances.size(); ++index)
    {
        const AegisGameMakerInstanceSnapshot& instance = g_instances[index];
        out << L"    { \"id\": " << instance.id
            << L", \"objectName\": \"" << JsonEscape(instance.objectName).c_str()
            << L"\", \"instanceName\": \"" << JsonEscape(instance.instanceName).c_str()
            << L"\", \"roomName\": \"" << JsonEscape(instance.roomName).c_str()
            << L"\", \"position\": [" << instance.position.x << L", " << instance.position.y << L", " << instance.position.z
            << L"], \"boundsMin\": [" << instance.boundsMin.x << L", " << instance.boundsMin.y << L", " << instance.boundsMin.z
            << L"], \"boundsMax\": [" << instance.boundsMax.x << L", " << instance.boundsMax.y << L", " << instance.boundsMax.z
            << L"], \"layerId\": " << instance.layerId
            << L", \"depth\": " << instance.depth
            << L", \"visible\": " << instance.visible
            << L", \"active\": " << instance.active
            << L", \"flags\": " << instance.flags << L" }";
        if (index + 1 < g_instances.size())
            out << L",";
        out << L"\n";
    }
    out << L"  ]\n";
    out << L"}\n";
    return 1;
}

AEGIS_UNIVERSAL_API int AegisGameMaker_LoadSnapshotJson(const wchar_t* path)
{
    if (!path || !path[0])
        return 0;

    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in)
        return 0;

    const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    AegisGameMakerViewport viewport = {};
    ExtractJsonNumber(json, "x", viewport.x);
    ExtractJsonNumber(json, "y", viewport.y);
    ExtractJsonNumber(json, "width", viewport.width);
    ExtractJsonNumber(json, "height", viewport.height);

    AegisGameMakerMatrix4x4 matrix = {};
    const bool hasMatrix = ExtractJsonMatrix(json, matrix);

    std::vector<AegisGameMakerInstanceSnapshot> instances;
    std::size_t arrayPos = json.find("\"instances\"");
    if (arrayPos != std::string::npos)
    {
        std::size_t pos = json.find('{', arrayPos);
        while (pos != std::string::npos)
        {
            const std::size_t end = json.find('}', pos);
            if (end == std::string::npos)
                break;
            const std::string objectJson = json.substr(pos, end - pos + 1);

            AegisGameMakerInstanceSnapshot instance = {};
            ExtractJsonUInt64(objectJson, "id", instance.id);
            CopyAnsi(instance.objectName, ExtractJsonString(objectJson, "objectName"));
            CopyAnsi(instance.instanceName, ExtractJsonString(objectJson, "instanceName"));
            CopyAnsi(instance.roomName, ExtractJsonString(objectJson, "roomName"));
            ExtractJsonVec3(objectJson, "position", instance.position);
            ExtractJsonVec3(objectJson, "boundsMin", instance.boundsMin);
            ExtractJsonVec3(objectJson, "boundsMax", instance.boundsMax);
            ExtractJsonInt(objectJson, "layerId", instance.layerId);
            ExtractJsonInt(objectJson, "depth", instance.depth);
            ExtractJsonInt(objectJson, "visible", instance.visible);
            ExtractJsonInt(objectJson, "active", instance.active);
            std::int32_t flags = 0;
            ExtractJsonInt(objectJson, "flags", flags);
            instance.flags = static_cast<std::uint32_t>(flags);
            instances.push_back(instance);

            pos = json.find('{', end + 1);
        }
    }

    std::lock_guard lock(g_adapterMutex);
    if (IsValidViewport(viewport))
    {
        g_viewport = viewport;
        g_hasViewport = true;
    }
    if (hasMatrix && IsValidMatrix(matrix))
    {
        g_viewProjection = matrix;
        g_hasMatrix = true;
    }
    g_instances = std::move(instances);
    ++g_timing.frameId;
    RebuildProjectionStatsLocked();
    return 1;
}

AEGIS_UNIVERSAL_API void AegisGameMaker_PrintCurrentInstances()
{
    std::vector<AegisGameMakerInstanceSnapshot> snapshot;
    {
        std::lock_guard lock(g_adapterMutex);
        snapshot = g_instances;
    }

    HANDLE output = ::GetStdHandle(STD_OUTPUT_HANDLE);
    for (const AegisGameMakerInstanceSnapshot& instance : snapshot)
    {
        std::ostringstream line;
        line << "[GameMakerUniversal] Instance id=" << instance.id
             << " object=\"" << instance.objectName
             << "\" instance=\"" << instance.instanceName
             << "\" room=\"" << instance.roomName
             << "\" position=(" << instance.position.x << ", " << instance.position.y << ", " << instance.position.z << ")\n";
        const std::string text = line.str();
        ::OutputDebugStringA(text.c_str());
        if (output && output != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            if (!::WriteConsoleA(output, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr))
                ::WriteFile(output, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
        }
    }
}
