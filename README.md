# GameMaker Universal

Aegis universal runtime project for **GameMaker / YoYo Runner**. This follows the same diagnostic-SDK concept as the CryEngine, id Tech, and Godot universals: identify runtime capabilities, expose provider-based instance/camera/viewport submission, validate world-to-screen projection, and write reports plus replayable JSON snapshots from debug data.

It does not add game-specific memory scanners, instance offset scraping, anti-cheat bypasses, stealth behavior, or hidden injection behavior.

## Dumpbin Findings

The profile was refreshed from these local targets:

```text
C:\Program Files (x86)\Steam\steamapps\common\KR17
C:\Program Files (x86)\Steam\steamapps\common\Asteroids ++
C:\Program Files (x86)\Steam\steamapps\common\Bacon May Die
C:\Program Files (x86)\Steam\steamapps\common\Block Tower TD 2 Prologue
C:\Program Files (x86)\Steam\steamapps\common\Iron Snout
C:\Program Files (x86)\Steam\steamapps\common\Prospector_The_First_Contract
C:\Program Files (x86)\Steam\steamapps\common\RED HOT VENGEANCE
C:\Program Files (x86)\Steam\steamapps\common\TeraBlaster
```

Confirmed signals:

```text
Most targets:
  data.win and/or options.ini beside the runner executable

KR17\KR17.exe
  Machine: Win32
  Imports: D3DX9_43.dll, d3d9.dll, steam_api.dll

Asteroids ++\Asteroids.exe
  Machine: x64
  Imports: d3d11.dll, steam_api64.dll
  Extension: Steamworks_x64.dll exports YYExtensionInitialise and steam_* helpers

Bacon May Die\Bacon.exe
  Machine: Win32
  Imports: d3d11.dll, steam_api.dll
  Extension: Steamworks.gml.dll exports RegisterCallbacks, steam_gml_init_cpp, steam_gml_update

Block Tower TD 2 Prologue\Block Tower 2.exe
  Machine: x64
  Imports: d3d11.dll, steam_api64.dll
  Extensions: Steamworks_x64.dll, display_mouse_lock_x64.dll

Iron Snout\IronSnout.exe
  Machine: Win32
  Imports: d3d11.dll, steam_api.dll
  Extensions: GameAnalytics.dll, Steamworks.gml.dll

Prospector_The_First_Contract\Prospector.exe
  Machine: x64
  Imports: d3d11.dll, steam_api64.dll
  Extensions: GameAnalytics.dll, GMLive, libfilesystem.dll, libfiledialogs.dll, libxprocess.dll, RAM_Usage.dll

RED HOT VENGEANCE\RedHotVengeance.exe
  Machine: Win32
  Imports: d3d9.dll, steam_api.dll
  Extensions: display_mouse_lock.dll, FMODGMS.dll

TeraBlaster\TeraBlaster.exe
  Machine: Win32
  Minimal runner-style executable; profile uses process evidence for this target
```

## Runtime Flow

1. Load the DLL into a process you are authorized to inspect.
2. The bootstrap thread initializes the shared Aegis runtime.
3. Loaded modules are enumerated and matched against GameMaker process/module/export hints.
4. The GameMaker adapter can receive instance snapshots, viewport size, and view-projection matrices from a debug/test provider.
5. The adapter validates matrix/viewport health, detects common GameMaker files and extensions, projects submitted points to screen coordinates, records JSON snapshots, and reports backend/capability status.

## Adapter API

The GameMaker-specific API is declared in:

```text
Include\AegisGameMakerUniversal.h
```

Core provider functions:

```cpp
AegisGameMaker_RegisterInstanceProvider(...);
AegisGameMaker_RegisterViewProjectionProvider(...);
AegisGameMaker_RegisterViewportProvider(...);
AegisGameMaker_UpdateProviders();
```

Direct submission functions:

```cpp
AegisGameMaker_SubmitInstanceSnapshots(...);
AegisGameMaker_SubmitViewProjection(...);
AegisGameMaker_SubmitViewport(...);
```

Diagnostics:

```cpp
AegisGameMaker_GetCapabilityInfo(...);
AegisGameMaker_ProjectWorldToScreen(...);
AegisGameMaker_WriteSnapshotJson(...);
AegisGameMaker_LoadSnapshotJson(...);
AegisGameMaker_PrintCurrentInstances();
```

## SDK Dump / Reload / Resolver

The shared runtime can now dump a structured SDK, reload it into DLL memory, and resolve symbols through either live exports or the reloaded SDK cache:

```cpp
AegisUniversal_DumpSdkJson(...);
AegisUniversal_WriteSdkHeader(...);
AegisUniversal_WriteSdkMapCsv(...);
AegisUniversal_LoadSdkJson(...);
AegisUniversal_GetLoadedSdkExportCount();
AegisUniversal_GetLoadedSdkExportInfo(...);
AegisUniversal_ResolveExport(...);
AegisUniversal_ResolveRva(...);
AegisUniversal_ValidateLoadedSdk(...);
AegisUniversal_WriteSdkValidationJson(...);
```

On startup the DLL writes `_SDK.json`, `_SDK.h`, `_SDKMap.csv`, and `_SDKValidation.json` beside the existing temp reports, then reloads the JSON into resolver memory. Runtime detection now filters the universal DLL itself out of module/export matching, so `AegisGameMakerUniversal.dll` cannot be counted as GameMaker evidence.

The F4 Runtime tab shows loaded SDK export count, live-resolved count, SDK-only count, stale RVA count, and any self-reference count.

## Instance Snapshot

Each submitted instance snapshot contains:

```text
id
objectName
instanceName
roomName
position
boundsMin / boundsMax
layerId
depth
visible
active
flags
```

This lets a GameMaker debug adapter submit players, enemies, NPCs, pickups, hazards, projectiles, UI markers, or other project objects without hardcoding title-specific offsets into the universal DLL.

## Renderer Status

`AegisGameMaker_GetCapabilityInfo` reports the detected backend as one of:

```text
Direct3D12
Direct3D11
Direct3D9
OpenGL
Unknown
```

The backend is inferred from loaded modules such as `d3d12.dll`, `d3d11.dll`, `dxgi.dll`, `d3d9.dll`, `d3dx9_43.dll`, and `opengl32.dll`.

## Internal ImGui Overlay

The DLL now owns an in-process ImGui diagnostics menu. On load it starts an internal render bridge and attempts backend setup in this order:

```text
Direct3D11 Present
Direct3D9 EndScene
OpenGL SwapBuffers
```

Press `F4` to show or hide the menu. The menu includes runtime status, renderer status, adapter capability checks, instance/provider timing, and overlay toggles for boxes, corner boxes, filled boxes, lines, and labels. D3D12 is still detected and reported clearly, but this build does not render ImGui through that backend yet.

## Build

Many GameMaker runners are 32-bit, while some newer exports are x64, so the project supports both.

This repository vendors the small shared Aegis runtime under:

```text
Common\AegisUniversalRuntime
```

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" ".\AegisGameMakerUniversal.sln" /m /p:Configuration=Release /p:Platform=x64 /v:minimal
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" ".\AegisGameMakerUniversal.sln" /m /p:Configuration=Release /p:Platform=Win32 /v:minimal
```

Outputs:

```text
build\x64\Release\AegisGameMakerUniversal.dll
build\Win32\Release\AegisGameMakerUniversal.dll
```

## Sample Harness

A local provider harness is included at:

```text
Samples\GameMakerProviderHarness\GameMakerProviderHarness.cpp
```

It loads the DLL, registers known test instances, submits a viewport plus screen-space projection mode, prints instance names/rooms, and writes `GameMakerProviderHarness.snapshot.json` so projection and replay can be validated without relying on a specific game.
