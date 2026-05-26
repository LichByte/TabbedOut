// ============================================================================
// NVSE Plugin API - Minimal header for plugin development
// Based on the public NVSE (New Vegas Script Extender) plugin interface
// ============================================================================
#pragma once
#include <windows.h>

// NVSE version info
#define NVSE_VERSION_INTEGER 6
#define NVSE_VERSION_INTEGER_MINOR 3
#define NVSE_VERSION_INTEGER_BETA 0

// The game's version we target (Steam 1.4.0.525)
#define RUNTIME_VERSION_1_4_0_525 0x010400020D

typedef UINT32 UInt32;
typedef UINT16 UInt16;
typedef UINT8  UInt8;

// ============================================================================
// PluginInfo - You fill this in during Query so NVSE knows about your plugin
// ============================================================================
struct PluginInfo
{
    enum { kInfoVersion = 1 };

    UInt32  infoVersion;    // Always set to kInfoVersion
    char*   name;           // Your plugin's name
    UInt32  version;        // Your plugin's version number
};

// ============================================================================
// NVSEInterface - NVSE passes this to your Query and Load functions
// ============================================================================
struct NVSEInterface
{
    UInt32  nvseVersion;        // NVSE version
    UInt32  runtimeVersion;     // Game executable version
    UInt32  editorVersion;      // GECK version (0 if not in editor)
    UInt32  isEditor;           // 1 if running in GECK, 0 if in-game
};

// ============================================================================
// These are the two functions your DLL must export:
//
//   NVSEPlugin_Query  - Called first. Return true if your plugin is compatible.
//   NVSEPlugin_Load   - Called second. Do your initialization here.
//
// They must be extern "C" and __declspec(dllexport).
// ============================================================================
