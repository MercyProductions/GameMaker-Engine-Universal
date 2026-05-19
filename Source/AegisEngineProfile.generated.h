#pragma once

#include "AegisUniversalRuntime.h"

inline constexpr AegisUniversalSignature kAegisUniversalSignatures[] = {
    { L"Asteroids.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Dumpbin target: x64 GameMaker runner with data.win and D3D11 import" },
    { L"Bacon.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Dumpbin target: Win32 GameMaker runner with data.win, Steamworks.gml.dll, and D3D11 import" },
    { L"Block Tower 2.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Dumpbin target: x64 GameMaker runner with data.win, Steamworks_x64.dll, and D3D11 import" },
    { L"IronSnout.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Dumpbin target: Win32 GameMaker runner with data.win, Steamworks.gml.dll, and D3D11 import" },
    { L"KR17.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Dumpbin target: Win32 GameMaker runner with data.win and D3D9 import" },
    { L"Prospector.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Dumpbin target: x64 GameMaker runner with data.win, Steamworks_x64.dll, GameAnalytics, and D3D11 import" },
    { L"RedHotVengeance.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core | AegisUniversalSignature_Renderer, "Dumpbin target: Win32 GameMaker runner with FMODGMS and D3D9 import" },
    { L"TeraBlaster.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Dumpbin target: Win32 GameMaker runner" },
    { L"runner.exe", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { L"gamemaker", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { L"yoyorunner", nullptr, nullptr, AegisUniversalSignature_Process | AegisUniversalSignature_Core, "Process hint" },
    { nullptr, L"data.win", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "GameMaker data package file" },
    { nullptr, L"options.ini", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker options/config file" },
    { nullptr, L"Steamworks.gml.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "Win32 GameMaker Steamworks extension" },
    { nullptr, L"Steamworks_x64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "x64 GameMaker Steamworks extension" },
    { nullptr, L"display_mouse_lock.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker display mouse lock extension" },
    { nullptr, L"display_mouse_lock_x64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "x64 GameMaker display mouse lock extension" },
    { nullptr, L"FMODGMS.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker FMOD extension" },
    { nullptr, L"GameAnalytics.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker GameAnalytics extension" },
    { nullptr, L"GMLive", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "GMLive tooling folder/module evidence" },
    { nullptr, L"RAM_Usage.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker RAM usage extension" },
    { nullptr, L"libfilesystem.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker filesystem extension" },
    { nullptr, L"libfiledialogs.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker file dialogs extension" },
    { nullptr, L"libxprocess.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "GameMaker process extension" },
    { nullptr, L"runner.exe", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, L"YYC", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "Module hint" },
    { nullptr, L"YoYo", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "Module hint" },
    { nullptr, L"GameMaker", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, L"steam_api.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, L"steam_api64.dll", nullptr, AegisUniversalSignature_Module | AegisUniversalSignature_Core, "Module hint" },
    { nullptr, nullptr, "?YYExtensionInitialise@@YAXPEBUYYRunnerInterface@@_K@Z", AegisUniversalSignature_Export | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "x64 GameMaker YYRunnerInterface extension initializer" },
    { nullptr, nullptr, "RegisterCallbacks", AegisUniversalSignature_Export | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "Win32 Steamworks.gml callback export" },
    { nullptr, nullptr, "steam_gml_init_cpp", AegisUniversalSignature_Export | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "Win32 Steamworks.gml initializer export" },
    { nullptr, nullptr, "steam_gml_update", AegisUniversalSignature_Export | AegisUniversalSignature_Core | AegisUniversalSignature_Scripting, "Steamworks.gml update export" },
    { nullptr, nullptr, "display_mouse_lock", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "GameMaker mouse lock extension export" },
    { nullptr, nullptr, "display_mouse_unlock", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "GameMaker mouse lock extension export" },
    { nullptr, nullptr, "FMODGMS_Sys_Create", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "FMODGMS system export" },
    { nullptr, nullptr, "initialize", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "GameAnalytics extension initialize export" },
    { nullptr, nullptr, "directory_get_current_working", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "libfilesystem extension export" },
    { nullptr, nullptr, "get_open_filename", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "libfiledialogs extension export" },
    { nullptr, nullptr, "ram_used", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "RAM usage extension export" },
    { nullptr, nullptr, "YYExtensionInitialise", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "Export hint" },
    { nullptr, nullptr, "YYExtensionFinalize", AegisUniversalSignature_Export | AegisUniversalSignature_Core, "Export hint" },
};

inline constexpr AegisUniversalProfile kAegisUniversalProfile = {
    L"GameMaker",
    L"GameMaker",
    L"GameMaker_Universal_Report.txt",
    L"GameMaker_Universal_Trace.txt",
    kAegisUniversalSignatures,
    sizeof(kAegisUniversalSignatures) / sizeof(kAegisUniversalSignatures[0])
};

