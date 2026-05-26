// ============================================================================
// config.h - INI configuration reader
//
// Lets users tweak the mod's behavior through a .ini file instead of
// recompiling. The INI file lives next to the DLL in the NVSE plugins folder.
// ============================================================================
#pragma once
#include <windows.h>
#include <string>

struct Config
{
    // --- Features ---
    bool bBorderlessWindowed;   // Force borderless windowed mode (main fix)
    bool bPauseOnFocusLoss;     // Auto-pause when alt-tabbing out
    bool bMuteOnFocusLoss;      // Mute audio when alt-tabbing out
    bool bClipCursor;           // Confine mouse cursor to window when focused

    // --- Display ---
    int  iCustomWidth;          // 0 = use desktop resolution
    int  iCustomHeight;         // 0 = use desktop resolution
    int  iMonitor;              // Which monitor to use (0 = primary)
    bool bTopMost;              // Keep window on top (some setups need this)

    // Singleton access
    static Config& Get()
    {
        static Config instance;
        return instance;
    }

    // Load settings from INI file
    void Load(const char* pluginPath)
    {
        // Build the INI path from the DLL path
        // e.g., "Data/NVSE/Plugins/AltTabFix.dll" -> "Data/NVSE/Plugins/AltTabFix.ini"
        std::string iniPath(pluginPath);
        size_t dot = iniPath.rfind('.');
        if (dot != std::string::npos)
            iniPath = iniPath.substr(0, dot) + ".ini";
        else
            iniPath += ".ini";

        const char* ini = iniPath.c_str();

        // GetPrivateProfileInt reads integer values from an INI file.
        // Format:  GetPrivateProfileInt("Section", "Key", DefaultValue, "path.ini")

        // [Main]
        bBorderlessWindowed = GetPrivateProfileIntA("Main", "bBorderlessWindowed", 1, ini) != 0;
        bPauseOnFocusLoss   = GetPrivateProfileIntA("Main", "bPauseOnFocusLoss",   1, ini) != 0;
        bMuteOnFocusLoss    = GetPrivateProfileIntA("Main", "bMuteOnFocusLoss",     1, ini) != 0;
        bClipCursor         = GetPrivateProfileIntA("Main", "bClipCursor",          1, ini) != 0;

        // [Display]
        iCustomWidth  = GetPrivateProfileIntA("Display", "iCustomWidth",  0, ini);
        iCustomHeight = GetPrivateProfileIntA("Display", "iCustomHeight", 0, ini);
        iMonitor      = GetPrivateProfileIntA("Display", "iMonitor",      0, ini);
        bTopMost      = GetPrivateProfileIntA("Display", "bTopMost",      0, ini) != 0;
    }

private:
    Config()
        : bBorderlessWindowed(true)
        , bPauseOnFocusLoss(true)
        , bMuteOnFocusLoss(true)
        , bClipCursor(true)
        , iCustomWidth(0)
        , iCustomHeight(0)
        , iMonitor(0)
        , bTopMost(false)
    {}
};
