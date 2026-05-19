# GameMaker Dumpbin Findings

Generated from:

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

## KR17

```text
KR17.exe
  Machine: Win32
  Files:
    data.win
    steam_api.dll
    D3DX9_43.dll
  Imports:
    d3d9.dll
    D3DX9_43.dll
    steam_api.dll
```

Notes:

```text
This target looks like an older D3D9 GameMaker runner. The profile uses process/file evidence plus D3D9 imports.
```

## Asteroids ++

```text
Asteroids.exe
  Machine: x64
  Files:
    data.win
    options.ini
    Steamworks_x64.dll
    steam_api64.dll
  Imports:
    d3d11.dll

Steamworks_x64.dll
  Exports:
    ?YYExtensionInitialise@@YAXPEBUYYRunnerInterface@@_K@Z
    steam_current_game_language
    steam_get_persona_name
    steam_initialised
```

Notes:

```text
The decorated YYExtensionInitialise export is strong evidence of a GameMaker extension built against the YY runner interface.
```

## Bacon May Die

```text
Bacon.exe
  Machine: Win32
  Files:
    data.win
    options.ini
    Steamworks.gml.dll
    steam_api.dll
  Imports:
    d3d11.dll

Steamworks.gml.dll
  Exports:
    RegisterCallbacks
    steam_gml_init_cpp
    steam_gml_update
    steam_get_persona_name
```

Notes:

```text
Steamworks.gml.dll provides clear GameMaker extension evidence through RegisterCallbacks and steam_gml_* exports.
```

## Block Tower TD 2 Prologue

```text
Block Tower 2.exe
  Machine: x64
  Files:
    data.win
    options.ini
    Steamworks_x64.dll
    display_mouse_lock_x64.dll
    steam_api64.dll
  Imports:
    d3d11.dll

display_mouse_lock_x64.dll
  Exports:
    display_mouse_bounds_raw
    display_mouse_lock
    display_mouse_unlock
```

Notes:

```text
The runner is x64 and uses D3D11. Mouse-lock exports are useful extension fingerprints.
```

## Iron Snout

```text
IronSnout.exe
  Machine: Win32
  Files:
    data.win
    options.ini
    GameAnalytics.dll
    Steamworks.gml.dll
    steam_api.dll
  Imports:
    d3d11.dll

GameAnalytics.dll
  Exports:
    initialize
    addDesignEvent
    gameAnalyticsStartSession
```

Notes:

```text
This target combines classic 32-bit runner layout with GameMaker extension DLLs.
```

## Prospector_The_First_Contract

```text
Prospector.exe
  Machine: x64
  Files:
    data.win
    options.ini
    GameAnalytics.dll
    GMLive
    libfiledialogs.dll
    libfilesystem.dll
    libxprocess.dll
    RAM_Usage.dll
    sdl2.dll
    steam_api64.dll
  Imports:
    d3d11.dll

libfilesystem.dll
  Exports:
    directory_get_current_working

libfiledialogs.dll
  Exports:
    get_open_filename
    get_directory
    show_message

RAM_Usage.dll
  Exports:
    ram_used
    ram_available
```

Notes:

```text
This target has a rich extension set. The profile treats these as GameMaker extension fingerprints, not as actor/object sources.
```

## RED HOT VENGEANCE

```text
RedHotVengeance.exe
  Machine: Win32
  Files:
    options.ini
    display_mouse_lock.dll
    fmod.dll
    FMODGMS.dll
    steam_api.dll
  Imports:
    d3d9.dll

FMODGMS.dll
  Exports:
    FMODGMS_Sys_Create
    FMODGMS_Sound_Load
    FMODGMS_Channel_Play
```

Notes:

```text
This target uses a D3D9 path and FMODGMS extension exports. Its data appears in a folder layout rather than a top-level data.win in the quick scan.
```

## TeraBlaster

```text
TeraBlaster.exe
  Machine: Win32
  Imports:
    Kernel32.dll
    User32.dll
```

Notes:

```text
This target has a minimal runner-style binary in the inspected layout. The profile keeps a process hint so it can still be identified as a known sample target.
```
