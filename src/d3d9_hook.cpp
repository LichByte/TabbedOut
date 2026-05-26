// ============================================================================
// d3d9_hook.cpp - Direct3D 9 hooking implementation (v3)
//
// APPROACH:
// Instead of vtable hooking (which failed because D3D9's debug/validation
// layer wraps the real object), we use two complementary strategies:
//
// STRATEGY 1: Patch FalloutPrefs.ini in memory
//   Before the game reads its display settings, we patch the "bFull Screen"
//   setting to 0 (windowed). The game then creates a windowed D3D9 device
//   natively -- no vtable hooking needed.
//
// STRATEGY 2: Window style hook
//   After the game creates its window, we strip borders and resize it to
//   fill the screen (borderless fullscreen). We detect the window via
//   a CBT hook that catches window creation.
//
// This is much more robust than vtable hooking because we work WITH the
// game's normal code path rather than trying to intercept internal D3D9 calls.
// ============================================================================

#include "d3d9_hook.h"
#include "window_hook.h"
#include "config.h"
#include "utils.h"

#include <d3d9.h>
#include <cstdio>
#include <string>
#pragma comment(lib, "d3d9.lib")

// ============================================================================
// Logging
// ============================================================================
static FILE* g_log = nullptr;
static char g_dllPath[MAX_PATH] = { 0 };

static void Log(const char* fmt, ...)
{
    if (!g_log)
    {
        // Log next to the DLL
        std::string logPath(g_dllPath);
        size_t dot = logPath.rfind('.');
        if (dot != std::string::npos)
            logPath = logPath.substr(0, dot) + "_d3d9.log";
        else
            logPath += "_d3d9.log";
        fopen_s(&g_log, logPath.c_str(), "w");
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
// Global state
// ============================================================================
static HWND  g_gameWindow     = nullptr;
static HHOOK g_cbtHook        = nullptr;
static bool  g_windowPatched  = false;
static bool  g_iniPatched     = false;

// Original GetProcAddress
typedef FARPROC(WINAPI* GetProcAddress_t)(HMODULE hModule, LPCSTR lpProcName);
static GetProcAddress_t g_origGetProcAddress = nullptr;

// Original GetPrivateProfileIntA (for intercepting INI reads)
typedef UINT(WINAPI* GetPrivateProfileIntA_t)(LPCSTR, LPCSTR, INT, LPCSTR);
static GetPrivateProfileIntA_t g_origGetPrivateProfileIntA = nullptr;

// Original ShowWindow (to block minimize)
typedef BOOL(WINAPI* ShowWindow_t)(HWND, int);
static ShowWindow_t g_origShowWindow = nullptr;


// ============================================================================
// GetDesktopDimensions
// ============================================================================
static void GetDesktopDimensions(int& width, int& height)
{
    DEVMODEA dm = {};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &dm))
    {
        width  = dm.dmPelsWidth;
        height = dm.dmPelsHeight;
    }
    else
    {
        width  = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }
}


// ============================================================================
// MakeWindowBorderless - Strip borders and fill the screen
// ============================================================================
static void MakeWindowBorderless(HWND hwnd)
{
    Config& cfg = Config::Get();

    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU | WS_BORDER);
    style |= WS_POPUP;
    SetWindowLongA(hwnd, GWL_STYLE, style);

    LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
    if (cfg.bTopMost) exStyle |= WS_EX_TOPMOST;
    SetWindowLongA(hwnd, GWL_EXSTYLE, exStyle);

    int screenW, screenH;
    if (cfg.iCustomWidth > 0 && cfg.iCustomHeight > 0)
    {
        screenW = cfg.iCustomWidth;
        screenH = cfg.iCustomHeight;
    }
    else
    {
        GetDesktopDimensions(screenW, screenH);
    }

    SetWindowPos(hwnd,
        cfg.bTopMost ? HWND_TOPMOST : HWND_TOP,
        0, 0, screenW, screenH,
        SWP_FRAMECHANGED | SWP_NOOWNERZORDER);

    ShowWindow(hwnd, SW_SHOW);
    Log("MakeWindowBorderless: applied %dx%d borderless", screenW, screenH);
}


// ============================================================================
// CBT Hook - Catches window creation system-wide (within our process)
//
// A CBT (Computer-Based Training) hook is a Windows mechanism that lets us
// intercept window events before they complete. We use HCBT_CREATEWND to
// detect when the game creates its main rendering window.
//
// We identify the game window by its class name: "Fallout: New Vegas"
// uses the Gamebryo engine class name.
// ============================================================================
static LRESULT CALLBACK CBTHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HCBT_ACTIVATE && !g_windowPatched)
    {
        // wParam = HWND of the window being activated
        HWND hwnd = reinterpret_cast<HWND>(wParam);

        // Check if this is the game window by class name
        char className[256] = { 0 };
        GetClassNameA(hwnd, className, sizeof(className));

        // FNV's window class is typically "Fallout: New Vegas"
        // But let's also check window title
        char title[256] = { 0 };
        GetWindowTextA(hwnd, title, sizeof(title));

        Log("CBTHookProc: ACTIVATE hwnd=%p class='%s' title='%s'", hwnd, className, title);

        // Check for Gamebryo/FNV window class patterns
        if (strstr(className, "Fallout") || strstr(className, "Gamebryo") ||
            strstr(className, "NetImmerse") || strstr(title, "Fallout"))
        {
            Log("CBTHookProc: Found game window! Applying borderless...");
            g_gameWindow = hwnd;
            g_windowPatched = true;

            // Apply borderless styling
            MakeWindowBorderless(hwnd);

            // Install the window procedure hook for focus management
            WindowHook::Install(hwnd);

            // Remove the CBT hook -- we don't need it anymore
            if (g_cbtHook)
            {
                UnhookWindowsHookEx(g_cbtHook);
                g_cbtHook = nullptr;
            }
        }
    }

    return CallNextHookEx(g_cbtHook, nCode, wParam, lParam);
}


// ============================================================================
// Hook_GetPrivateProfileIntA - Intercept the game's INI file reads
//
// When the game reads "bFull Screen" from FalloutPrefs.ini, we return 0
// (windowed mode) instead of whatever is in the file. This makes the game
// create a windowed D3D9 device, which is the core of our alt-tab fix.
//
// We also intercept the resolution reads to force desktop resolution.
// ============================================================================
static UINT WINAPI Hook_GetPrivateProfileIntA(
    LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName)
{
    // Only intercept reads from Fallout INI files
    if (lpFileName && lpKeyName && Config::Get().bBorderlessWindowed)
    {
        // Case-insensitive check for FalloutPrefs.ini or Fallout.ini
        if (strstr(lpFileName, "Fallout") || strstr(lpFileName, "fallout"))
        {
            // Force windowed mode
            if (_stricmp(lpKeyName, "bFull Screen") == 0)
            {
                Log("Hook_GetPrivateProfileIntA: intercepted bFull Screen -> returning 0 (windowed)");
                return 0;  // Windowed mode
            }

            // Force desktop resolution width
            if (_stricmp(lpKeyName, "iSize W") == 0)
            {
                int w, h;
                Config& cfg = Config::Get();
                if (cfg.iCustomWidth > 0)
                    w = cfg.iCustomWidth;
                else
                    GetDesktopDimensions(w, h);
                Log("Hook_GetPrivateProfileIntA: intercepted iSize W -> returning %d", w);
                return w;
            }

            // Force desktop resolution height
            if (_stricmp(lpKeyName, "iSize H") == 0)
            {
                int w, h;
                Config& cfg = Config::Get();
                if (cfg.iCustomHeight > 0)
                    h = cfg.iCustomHeight;
                else
                    GetDesktopDimensions(w, h);
                Log("Hook_GetPrivateProfileIntA: intercepted iSize H -> returning %d", h);
                return h;
            }

            // Force borderless off in the game's own settings (we handle it)
            if (_stricmp(lpKeyName, "bBorderless") == 0)
            {
                return 0;
            }
        }
    }

    return g_origGetPrivateProfileIntA(lpAppName, lpKeyName, nDefault, lpFileName);
}


// ============================================================================
// Hook_ShowWindow - Block the game from minimizing itself
//
// The game calls ShowWindow(hwnd, SW_MINIMIZE) when it loses focus.
// We intercept this and block any minimize attempts so the window stays
// visible behind other apps -- proper borderless behavior.
// ============================================================================
static BOOL WINAPI Hook_ShowWindow(HWND hwnd, int nCmdShow)
{
    // Block minimize commands on the game window
    if (hwnd == g_gameWindow || (g_gameWindow == nullptr && nCmdShow == SW_MINIMIZE))
    {
        if (nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED ||
            nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == SW_FORCEMINIMIZE)
        {
            Log("Hook_ShowWindow: BLOCKED minimize (nCmdShow=%d)", nCmdShow);
            return TRUE;  // Pretend it worked
        }
    }

    return g_origShowWindow(hwnd, nCmdShow);
}


// ============================================================================
// Hook_GetProcAddress - Intercept dynamic function loading
//
// We no longer need to hook Direct3DCreate9 for vtable patching.
// Instead we hook GetPrivateProfileIntA to intercept INI reads.
// But we keep the GetProcAddress hook as the mechanism to install
// additional IAT hooks when needed modules load.
// ============================================================================
static FARPROC WINAPI Hook_GetProcAddress(HMODULE hModule, LPCSTR lpProcName)
{
    if (reinterpret_cast<uintptr_t>(lpProcName) > 0xFFFF)
    {
        // Log D3D9-related lookups for debugging
        if (strstr(lpProcName, "Direct3D"))
        {
            Log("Hook_GetProcAddress: game requested '%s'", lpProcName);
        }
    }

    return g_origGetProcAddress(hModule, lpProcName);
}


// ============================================================================
// HookIATEntry - Generic IAT hooking helper
//
// Searches the game's IAT for a specific DLL + function and replaces it.
// Returns the original function pointer, or nullptr on failure.
// ============================================================================
static void* HookIATEntry(const char* targetDll, const char* targetFunc, void* hookFunc)
{
    HMODULE gameModule = GetModuleHandleA(nullptr);
    if (!gameModule) return nullptr;

    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(gameModule);
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(
        reinterpret_cast<uintptr_t>(gameModule) + dosHeader->e_lfanew);

    auto importDir = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.Size == 0) return nullptr;

    auto importDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
        reinterpret_cast<uintptr_t>(gameModule) + importDir.VirtualAddress);

    // Get the real function address for comparison
    HMODULE targetModule = GetModuleHandleA(targetDll);
    if (!targetModule) targetModule = LoadLibraryA(targetDll);
    if (!targetModule) return nullptr;

    FARPROC realFunc = ::GetProcAddress(targetModule, targetFunc);

    for (; importDesc->Name != 0; importDesc++)
    {
        const char* dllName = reinterpret_cast<const char*>(
            reinterpret_cast<uintptr_t>(gameModule) + importDesc->Name);

        if (_stricmp(dllName, targetDll) != 0)
            continue;

        auto firstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
            reinterpret_cast<uintptr_t>(gameModule) + importDesc->FirstThunk);

        IMAGE_THUNK_DATA* origThunk = nullptr;
        if (importDesc->OriginalFirstThunk != 0)
        {
            origThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(
                reinterpret_cast<uintptr_t>(gameModule) + importDesc->OriginalFirstThunk);
        }

        for (; firstThunk->u1.Function != 0; firstThunk++)
        {
            bool isMatch = false;

            // Match by function pointer address
            if (reinterpret_cast<FARPROC>(firstThunk->u1.Function) == realFunc)
                isMatch = true;

            // Match by name
            if (!isMatch && origThunk)
            {
                if (!(origThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG))
                {
                    auto importByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                        reinterpret_cast<uintptr_t>(gameModule) + origThunk->u1.AddressOfData);
                    if (strcmp(importByName->Name, targetFunc) == 0)
                        isMatch = true;
                }
                origThunk++;
            }

            if (isMatch)
            {
                void* original = reinterpret_cast<void*>(firstThunk->u1.Function);
                DWORD oldProtect;
                VirtualProtect(&firstThunk->u1.Function, sizeof(uintptr_t),
                               PAGE_EXECUTE_READWRITE, &oldProtect);
                firstThunk->u1.Function = reinterpret_cast<uintptr_t>(hookFunc);
                VirtualProtect(&firstThunk->u1.Function, sizeof(uintptr_t),
                               oldProtect, &oldProtect);
                return original;
            }
        }
    }

    return nullptr;
}


// ============================================================================
// D3D9Hook::Install - Main installation
//
// 1. Hook GetPrivateProfileIntA to intercept "bFull Screen" reads
// 2. Hook GetProcAddress for diagnostic logging
// 3. Install a CBT hook to catch game window creation
// ============================================================================
bool D3D9Hook::Install()
{
    // Save DLL path for logging
    GetModuleFileNameA(GetModuleHandleA(nullptr), g_dllPath, MAX_PATH);

    Log("D3D9Hook::Install v3 starting...");

    // Hook 1: GetPrivateProfileIntA (from kernel32.dll)
    // The game uses this to read bFull Screen, iSize W, iSize H from INI
    g_origGetPrivateProfileIntA = reinterpret_cast<GetPrivateProfileIntA_t>(
        HookIATEntry("kernel32.dll", "GetPrivateProfileIntA", &Hook_GetPrivateProfileIntA));

    if (g_origGetPrivateProfileIntA)
    {
        Log("D3D9Hook::Install: GetPrivateProfileIntA hooked successfully");
    }
    else
    {
        Log("D3D9Hook::Install: WARNING - GetPrivateProfileIntA hook failed, trying alternate approach");

        // Fallback: the function might be in a different DLL name casing
        // or imported through api-ms-win shims
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        if (kernel32)
        {
            g_origGetPrivateProfileIntA = reinterpret_cast<GetPrivateProfileIntA_t>(
                ::GetProcAddress(kernel32, "GetPrivateProfileIntA"));
        }

        // Try hooking via GetProcAddress interception instead
        g_origGetProcAddress = reinterpret_cast<GetProcAddress_t>(
            HookIATEntry("kernel32.dll", "GetProcAddress", &Hook_GetProcAddress));

        if (g_origGetProcAddress)
            Log("D3D9Hook::Install: GetProcAddress hooked as fallback");
        else
            Log("D3D9Hook::Install: WARNING - GetProcAddress hook also failed");
    }

    // Hook 2: GetProcAddress for diagnostics (if not already hooked as fallback)
    if (!g_origGetProcAddress)
    {
        g_origGetProcAddress = reinterpret_cast<GetProcAddress_t>(
            HookIATEntry("kernel32.dll", "GetProcAddress", &Hook_GetProcAddress));
        if (g_origGetProcAddress)
            Log("D3D9Hook::Install: GetProcAddress hooked for diagnostics");
    }

    // Hook 3: ShowWindow (from user32.dll) - block minimize
    g_origShowWindow = reinterpret_cast<ShowWindow_t>(
        HookIATEntry("user32.dll", "ShowWindow", &Hook_ShowWindow));
    if (g_origShowWindow)
        Log("D3D9Hook::Install: ShowWindow hooked (minimize blocked)");
    else
        Log("D3D9Hook::Install: WARNING - ShowWindow hook failed");

    // Hook 4: CBT hook for window creation detection
    g_cbtHook = SetWindowsHookExA(WH_CBT, CBTHookProc, nullptr, GetCurrentThreadId());
    if (g_cbtHook)
    {
        Log("D3D9Hook::Install: CBT hook installed for window detection");
    }
    else
    {
        Log("D3D9Hook::Install: WARNING - CBT hook failed (error %d)", GetLastError());
    }

    Log("D3D9Hook::Install: initialization complete");
    return g_origGetPrivateProfileIntA != nullptr || g_origGetProcAddress != nullptr;
}


HWND D3D9Hook::GetGameWindow()
{
    return g_gameWindow;
}
