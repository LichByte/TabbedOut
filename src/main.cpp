// ============================================================================
// main.cpp - NVSE Plugin Entry Point
//
// This is where it all starts. NVSE (New Vegas Script Extender) loads our DLL
// and calls two functions:
//
//   1. NVSEPlugin_Query  - "Are you compatible with this version of the game?"
//   2. NVSEPlugin_Load   - "Okay, do your thing."
//
// Both must be exported as extern "C" so NVSE can find them by name.
//
// PLUGIN LOADING ORDER:
//   Game starts -> NVSE loads -> NVSE scans Data/NVSE/Plugins/*.dll ->
//   For each DLL: calls Query, then Load -> Game continues booting ->
//   Game calls Direct3DCreate9 (our hook fires) -> Game calls CreateDevice
//   (our hook fires, forces windowed) -> Game renders in borderless window
// ============================================================================

#include "../nvse/PluginAPI.h"
#include "config.h"
#include "d3d9_hook.h"
#include "window_hook.h"
#include "utils.h"
#include <cstdio>

// ============================================================================
// Plugin info
// ============================================================================
#define PLUGIN_NAME    "AltTabFix"
#define PLUGIN_VERSION 1

// Path to our DLL (filled in by DllMain)
static char g_pluginPath[MAX_PATH] = { 0 };

// Simple logging to a file next to the DLL
static FILE* g_log = nullptr;

static void Log(const char* fmt, ...)
{
    if (!g_log)
    {
        // Build log path from DLL path
        char logPath[MAX_PATH];
        strcpy_s(logPath, g_pluginPath);
        char* dot = strrchr(logPath, '.');
        if (dot) strcpy_s(dot, MAX_PATH - (dot - logPath), ".log");
        fopen_s(&g_log, logPath, "w");
    }

    if (g_log)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(g_log, fmt, args);
        fprintf(g_log, "\n");
        fflush(g_log);
        va_end(args);
    }
}


// ============================================================================
// NVSEPlugin_Query
//
// NVSE calls this first. We check if we're compatible and fill in our info.
// Return TRUE  = "I'm good, load me"
// Return FALSE = "Don't load me"
// ============================================================================
extern "C" __declspec(dllexport)
bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info)
{
    // Tell NVSE about ourselves
    info->infoVersion = PluginInfo::kInfoVersion;
    info->name        = (char*)PLUGIN_NAME;
    info->version     = PLUGIN_VERSION;

    Log("=== %s v%d ===", PLUGIN_NAME, PLUGIN_VERSION);
    Log("Query: NVSE version %d, runtime version 0x%08X",
        nvse->nvseVersion, nvse->runtimeVersion);

    // Don't load in the GECK editor -- we only work in-game
    if (nvse->isEditor)
    {
        Log("Query: Running in editor, skipping.");
        return false;
    }

    // Check NVSE version (we need at least v5)
    if (nvse->nvseVersion < 5)
    {
        Log("Query: NVSE version too old (need >= 5, got %d)", nvse->nvseVersion);
        return false;
    }

    Log("Query: Compatible! Returning true.");
    return true;
}


// ============================================================================
// NVSEPlugin_Load
//
// NVSE calls this after Query returns true. This is where we set up our hooks.
// At this point, the game hasn't initialized D3D9 yet, so our IAT hook will
// catch the call when it happens.
// ============================================================================
extern "C" __declspec(dllexport)
bool NVSEPlugin_Load(const NVSEInterface* nvse)
{
    Log("Load: Initializing...");

    // Load configuration from INI file
    Config::Get().Load(g_pluginPath);

    Config& cfg = Config::Get();
    Log("Load: Config loaded:");
    Log("  bBorderlessWindowed = %d", cfg.bBorderlessWindowed);
    Log("  bPauseOnFocusLoss   = %d", cfg.bPauseOnFocusLoss);
    Log("  bMuteOnFocusLoss    = %d", cfg.bMuteOnFocusLoss);
    Log("  bClipCursor         = %d", cfg.bClipCursor);
    Log("  iCustomWidth        = %d", cfg.iCustomWidth);
    Log("  iCustomHeight       = %d", cfg.iCustomHeight);

    // Install D3D9 hooks (IAT hook on Direct3DCreate9)
    if (cfg.bBorderlessWindowed)
    {
        if (D3D9Hook::Install())
        {
            Log("Load: D3D9 IAT hook installed successfully.");
        }
        else
        {
            Log("Load: ERROR - Failed to install D3D9 hook!");
            return false;
        }
    }

    // Note: Window hook will be installed later, after the game creates its
    // window. We do this in the CreateDevice hook since that's when we first
    // get the window handle. But we also set up a deferred install here
    // using a timer callback.
    //
    // Actually, the window hook gets installed right after CreateDevice
    // succeeds in d3d9_hook.cpp. But since we want to keep the modules
    // separate, we'll set up a one-shot timer to install it shortly after
    // the game window is created.

    Log("Load: Plugin loaded successfully!");
    return true;
}


// ============================================================================
// DllMain - Standard DLL entry point
//
// Called by Windows when the DLL is loaded/unloaded.
// We use it just to save our DLL's file path for finding the INI/log files.
// ============================================================================
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        // Save our DLL path for later (INI and log file locations)
        GetModuleFileNameA(hModule, g_pluginPath, MAX_PATH);

        // We don't need thread notifications, so disable them for performance
        DisableThreadLibraryCalls(hModule);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        // Clean up hooks to prevent crashes from dangling pointers.
        // This also unmutes audio if it was muted.
        WindowHook::Cleanup();
    }
    return TRUE;
}
