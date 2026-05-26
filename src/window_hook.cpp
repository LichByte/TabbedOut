// ============================================================================
// window_hook.cpp - Window procedure hook implementation
//
// WINDOW PROCEDURE (WndProc) EXPLANATION:
// Every window in Windows has a "window procedure" -- a callback function
// that receives all messages for that window (mouse clicks, key presses,
// resize, focus changes, paint requests, etc.).
//
// We replace the game's window procedure with our own. Our procedure handles
// focus-related messages, then forwards everything else to the original.
// This is called "subclassing" a window.
//
// WHAT THIS FILE HANDLES:
//   - Focus gain/loss detection (alt-tab management)
//   - Audio muting via Windows Audio Session API (WASAPI)
//   - Borderless window style enforcement
//   - Cursor confinement
//   - Clean shutdown support
// ============================================================================

#include "window_hook.h"
#include "config.h"

#include <mmdeviceapi.h>   // WASAPI device enumeration
#include <audiopolicy.h>   // Audio session control
#include <cstdio>
#include <string>

// ============================================================================
// Logging (writes to AltTabFix_window.log next to the game exe)
// ============================================================================
static FILE* g_wlog = nullptr;

static void Log(const char* fmt, ...)
{
    if (!g_wlog)
    {
        char exePath[MAX_PATH];
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        std::string logPath(exePath);
        size_t dot = logPath.rfind('.');
        if (dot != std::string::npos)
            logPath = logPath.substr(0, dot) + "_window.log";
        else
            logPath += "_window.log";
        fopen_s(&g_wlog, logPath.c_str(), "w");
    }
    if (g_wlog)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(g_wlog, fmt, args);
        fprintf(g_wlog, "\n");
        fflush(g_wlog);
        va_end(args);
    }
}


// ============================================================================
// State
// ============================================================================
static HWND    g_hwnd          = nullptr;
static WNDPROC g_origWndProc   = nullptr;  // The game's original window procedure
static bool    g_hasFocus      = true;
static bool    g_cursorClipped = false;
static bool    g_shuttingDown  = false;     // Set when game is quitting


// ============================================================================
// AUDIO MUTING via Windows Audio Session API (WASAPI)
//
// WASAPI lets us mute just our game process without affecting other apps.
// On modern Windows, DirectSound (which FNV uses) runs through WASAPI
// internally, so this catches all game audio.
//
// Flow: enumerate audio devices -> get default output -> get our process's
// audio session -> mute/unmute it.
// ============================================================================
static ISimpleAudioVolume* g_audioVolume    = nullptr;
static bool                g_weInitCom      = false;
static bool                g_audioInitDone  = false;

static void InitAudioControl()
{
    if (g_audioInitDone) return;
    g_audioInitDone = true;

    // COM must be initialized to use WASAPI.
    // The game likely already did this for DirectX, but we handle all cases.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hr == S_OK || hr == S_FALSE)
        g_weInitCom = true;   // We incremented the ref count, must undo later
    else if (hr == RPC_E_CHANGED_MODE)
        g_weInitCom = false;  // Already initialized differently -- that's fine
    else
    {
        Log("InitAudioControl: CoInitializeEx failed (0x%08X)", hr);
        return;
    }

    // Step 1: Get the audio device enumerator
    IMMDeviceEnumerator* enumerator = nullptr;
    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr) || !enumerator)
    {
        Log("InitAudioControl: CoCreateInstance(MMDeviceEnumerator) failed (0x%08X)", hr);
        return;
    }

    // Step 2: Get the default audio output device (speakers/headphones)
    IMMDevice* device = nullptr;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
    enumerator->Release();
    if (FAILED(hr) || !device)
    {
        Log("InitAudioControl: GetDefaultAudioEndpoint failed (0x%08X)", hr);
        return;
    }

    // Step 3: Get the audio session manager for this device
    IAudioSessionManager* manager = nullptr;
    hr = device->Activate(
        __uuidof(IAudioSessionManager), CLSCTX_ALL, nullptr,
        reinterpret_cast<void**>(&manager));
    device->Release();
    if (FAILED(hr) || !manager)
    {
        Log("InitAudioControl: Activate(IAudioSessionManager) failed (0x%08X)", hr);
        return;
    }

    // Step 4: Get the volume control for our process's default audio session.
    //         Passing NULL as the session GUID gives us the default session,
    //         which is where DirectSound audio ends up on modern Windows.
    hr = manager->GetSimpleAudioVolume(nullptr, FALSE, &g_audioVolume);
    manager->Release();
    if (FAILED(hr) || !g_audioVolume)
    {
        Log("InitAudioControl: GetSimpleAudioVolume failed (0x%08X)", hr);
        g_audioVolume = nullptr;
        return;
    }

    Log("InitAudioControl: WASAPI audio control initialized successfully");
}

static void SetProcessMute(bool mute)
{
    if (!g_audioVolume)
        InitAudioControl();

    if (g_audioVolume)
    {
        g_audioVolume->SetMute(mute ? TRUE : FALSE, nullptr);
        Log("SetProcessMute: %s", mute ? "MUTED" : "UNMUTED");
    }
}

static void CleanupAudio()
{
    if (g_audioVolume)
    {
        // Always unmute before cleanup so the game isn't stuck silent
        g_audioVolume->SetMute(FALSE, nullptr);
        g_audioVolume->Release();
        g_audioVolume = nullptr;
    }
    if (g_weInitCom)
    {
        CoUninitialize();
        g_weInitCom = false;
    }
}


// ============================================================================
// BORDERLESS ENFORCEMENT
//
// The game may re-apply its own window styles during startup (adding borders
// back after our initial MakeWindowBorderless call in the CBT hook).
// We use a timer to periodically check and re-strip borders during the
// first few seconds after window creation.
// ============================================================================
#define BORDERLESS_TIMER_ID   0xB0DE  // Unique timer ID
#define BORDERLESS_INTERVAL   100     // Check every 100ms
#define BORDERLESS_MAX_CHECKS 50      // For 5 seconds total

static int g_borderlessChecks = 0;

// ============================================================================
// GetMonitorRect - Get the exact rectangle of the monitor the window is on
//
// Handles multi-monitor setups correctly by using MonitorFromWindow
// instead of assuming the game is always on the primary monitor at (0,0).
// ============================================================================
static void GetMonitorRect(HWND hwnd, RECT& outRect)
{
    HMONITOR hMon = nullptr;
    if (hwnd)
        hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    else
        hMon = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);

    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    if (hMon && GetMonitorInfoA(hMon, &mi))
    {
        outRect = mi.rcMonitor;
    }
    else
    {
        outRect.left   = 0;
        outRect.top    = 0;
        outRect.right  = GetSystemMetrics(SM_CXSCREEN);
        outRect.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
}

static void EnforceBorderless(HWND hwnd)
{
    if (!Config::Get().bBorderlessWindowed || g_shuttingDown)
        return;

    Config& cfg = Config::Get();

    // Figure out where the window SHOULD be (primary monitor)
    int targetX, targetY, targetW, targetH;
    if (cfg.iCustomWidth > 0 && cfg.iCustomHeight > 0)
    {
        targetX = 0;
        targetY = 0;
        targetW = cfg.iCustomWidth;
        targetH = cfg.iCustomHeight;
    }
    else
    {
        RECT monRect;
        GetMonitorRect(nullptr, monRect);
        targetX = monRect.left;
        targetY = monRect.top;
        targetW = monRect.right - monRect.left;
        targetH = monRect.bottom - monRect.top;
    }

    // Check 1: Are borders present?
    bool needsFix = false;
    LONG style = GetWindowLongA(hwnd, GWL_STYLE);
    if (style & (WS_CAPTION | WS_THICKFRAME | WS_BORDER))
    {
        Log("EnforceBorderless: Game re-added borders, stripping them");
        style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE |
                    WS_SYSMENU | WS_BORDER);
        style |= WS_POPUP;
        SetWindowLongA(hwnd, GWL_STYLE, style);

        LONG exStyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
        exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE |
                      WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
        SetWindowLongA(hwnd, GWL_EXSTYLE, exStyle);
        needsFix = true;
    }

    // Check 2: Is the window the wrong size or position?
    // The game may resize/reposition after our initial MakeWindowBorderless.
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int curX = wr.left;
    int curY = wr.top;
    int curW = wr.right - wr.left;
    int curH = wr.bottom - wr.top;

    if (curX != targetX || curY != targetY || curW != targetW || curH != targetH)
    {
        Log("EnforceBorderless: Window at (%d,%d) %dx%d, should be (%d,%d) %dx%d -- fixing",
            curX, curY, curW, curH, targetX, targetY, targetW, targetH);
        needsFix = true;
    }

    if (needsFix)
    {
        SetWindowPos(hwnd, HWND_TOP, targetX, targetY, targetW, targetH,
                     SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
    }
}


// ============================================================================
// ClipCursorToWindow - Confine the mouse cursor to the game window
//
// Without this, in borderless windowed mode the cursor can drift to other
// monitors. We clip it when the game has focus and unclip when it doesn't.
// ============================================================================
static void ClipCursorToWindow(HWND hwnd, bool clip)
{
    if (clip)
    {
        RECT rect;
        GetClientRect(hwnd, &rect);

        // GetClientRect returns coordinates relative to the window.
        // ClipCursor needs screen coordinates, so we convert.
        POINT topLeft = { rect.left, rect.top };
        POINT bottomRight = { rect.right, rect.bottom };
        ClientToScreen(hwnd, &topLeft);
        ClientToScreen(hwnd, &bottomRight);

        RECT screenRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
        ClipCursor(&screenRect);
        g_cursorClipped = true;
    }
    else
    {
        // Pass nullptr to free the cursor
        ClipCursor(nullptr);
        g_cursorClipped = false;
    }
}


// ============================================================================
// OnFocusGained - Called when the game window receives focus
//
// We aggressively bring the window to the foreground because some mods
// (NVTF, JIP LN) may interfere with normal window activation.
// ============================================================================
static void OnFocusGained(HWND hwnd)
{
    if (g_hasFocus) return;  // Already focused, avoid redundant work
    g_hasFocus = true;
    Log("OnFocusGained");

    Config& cfg = Config::Get();

    // Bring the window to the front -- all four calls cover different
    // edge cases in how Windows decides which window is "foreground"
    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    SetFocus(hwnd);

    // Re-clip cursor to window
    if (cfg.bClipCursor)
        ClipCursorToWindow(hwnd, true);

    // Unmute audio
    if (cfg.bMuteOnFocusLoss)
        SetProcessMute(false);
}


// ============================================================================
// OnFocusLost - Called when the game window loses focus (alt-tab)
// ============================================================================
static void OnFocusLost(HWND hwnd)
{
    if (!g_hasFocus) return;  // Already unfocused
    g_hasFocus = false;
    Log("OnFocusLost");

    Config& cfg = Config::Get();

    // Release cursor so it can move to other windows/monitors
    if (cfg.bClipCursor)
        ClipCursorToWindow(hwnd, false);

    // Mute the game audio
    if (cfg.bMuteOnFocusLoss)
        SetProcessMute(true);
}


// ============================================================================
// Hook_WndProc - Our replacement window procedure
//
// IMPORTANT MESSAGES WE HANDLE:
//   WM_ACTIVATEAPP  - Sent when the app gains/loses focus
//   WM_ACTIVATE     - Sent when the window is activated/deactivated
//   WM_SETCURSOR    - Sent when the cursor moves over the window
//   WM_SIZE         - Sent when the window is resized (block minimize)
//   WM_SYSCOMMAND   - System commands (minimize, screensaver, etc.)
//   WM_TIMER        - Our borderless enforcement timer
//   WM_CLOSE        - Window is being closed (game quitting)
//   WM_DESTROY      - Window is being destroyed (game quitting)
//
// KEY DESIGN DECISION:
// When the game LOSES focus, we swallow the WM_ACTIVATEAPP and WM_ACTIVATE
// messages so the game doesn't know it lost focus. This prevents the game
// from minimizing itself or doing other unwanted things. But we STOP
// swallowing once the game is shutting down, to prevent quit freezes.
// ============================================================================
static LRESULT CALLBACK Hook_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    // ----------------------------------------------------------------
    // SHUTDOWN DETECTION
    // When the game is quitting, we must stop swallowing messages or
    // the shutdown sequence can deadlock (freeze on quit).
    // ----------------------------------------------------------------
    case WM_CLOSE:
        Log("WM_CLOSE received -- entering shutdown mode");
        g_shuttingDown = true;
        // Unmute audio and release cursor before shutting down
        if (g_audioVolume)
            g_audioVolume->SetMute(FALSE, nullptr);
        ClipCursor(nullptr);
        // Kill our timer
        KillTimer(hwnd, BORDERLESS_TIMER_ID);
        // Let the game handle shutdown normally
        break;

    case WM_DESTROY:
        Log("WM_DESTROY received");
        g_shuttingDown = true;
        // Restore original window procedure so no more hook interference
        if (g_origWndProc)
        {
            SetWindowLongPtrA(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
            Log("Original WndProc restored");
        }
        // Forward to original and let it handle destruction
        return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);

    // ----------------------------------------------------------------
    // FOCUS MANAGEMENT
    // ----------------------------------------------------------------
    case WM_ACTIVATEAPP:
        // wParam: TRUE if being activated, FALSE if deactivated
        if (g_shuttingDown)
            break;  // During shutdown, let ALL messages through

        if (wParam)
        {
            OnFocusGained(hwnd);
            // Forward activation so the game resumes input properly
            return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);
        }
        else
        {
            OnFocusLost(hwnd);
            // SWALLOW -- don't tell the game it lost focus.
            // The game's original handler would minimize the window and
            // pause. By swallowing, the window stays visible behind other
            // windows, which is correct borderless behavior.
            return 0;
        }

    case WM_ACTIVATE:
        if (g_shuttingDown)
            break;  // During shutdown, let messages through

        if (LOWORD(wParam) == WA_INACTIVE)
        {
            OnFocusLost(hwnd);
            // SWALLOW -- same reason as WM_ACTIVATEAPP
            return 0;
        }
        else
        {
            OnFocusGained(hwnd);
            return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);
        }

    // ----------------------------------------------------------------
    // CURSOR MANAGEMENT
    // ----------------------------------------------------------------
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && g_hasFocus &&
            Config::Get().bClipCursor && !g_cursorClipped)
        {
            ClipCursorToWindow(hwnd, true);
        }
        break;

    // ----------------------------------------------------------------
    // MINIMIZE PREVENTION
    // The game tries to minimize itself on focus loss. We block that
    // so the window stays visible behind other apps.
    // ----------------------------------------------------------------
    case WM_SIZE:
        if (!g_shuttingDown && wParam == SIZE_MINIMIZED)
            return 0;  // SWALLOW -- prevent minimize
        break;

    case WM_SYSCOMMAND:
    {
        WPARAM cmd = wParam & 0xFFF0;

        if (!g_shuttingDown)
        {
            // Block the game from minimizing itself
            if (cmd == SC_MINIMIZE)
                return 0;

            // Prevent screensaver and monitor power-off while game is focused
            if (g_hasFocus && (cmd == SC_SCREENSAVE || cmd == SC_MONITORPOWER))
                return 0;
        }
        break;
    }

    // ----------------------------------------------------------------
    // BORDERLESS ENFORCEMENT TIMER
    // Re-checks and re-strips borders every 100ms for the first 5
    // seconds after window creation, catching the game's late style
    // re-application during startup.
    // ----------------------------------------------------------------
    case WM_TIMER:
        if (wParam == BORDERLESS_TIMER_ID)
        {
            g_borderlessChecks++;
            EnforceBorderless(hwnd);

            if (g_borderlessChecks >= BORDERLESS_MAX_CHECKS)
            {
                KillTimer(hwnd, BORDERLESS_TIMER_ID);
                Log("Borderless enforcement timer stopped (startup phase over)");
            }
            return 0;
        }
        break;

    // ----------------------------------------------------------------
    // WINDOW STYLE ENFORCEMENT
    // If the game tries to change window styles (adding borders back),
    // we intercept and strip the border flags.
    // ----------------------------------------------------------------
    case WM_STYLECHANGING:
        if (!g_shuttingDown && Config::Get().bBorderlessWindowed &&
            wParam == GWL_STYLE)
        {
            STYLESTRUCT* ss = reinterpret_cast<STYLESTRUCT*>(lParam);
            LONG cleaned = ss->styleNew;
            cleaned &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE |
                          WS_MAXIMIZE | WS_SYSMENU | WS_BORDER);
            cleaned |= WS_POPUP;

            if (cleaned != ss->styleNew)
            {
                Log("WM_STYLECHANGING: blocked border re-application");
                ss->styleNew = cleaned;
            }
        }
        break;
    }

    // Forward everything else to the game's original window procedure
    return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);
}


// ============================================================================
// WindowHook::Install - Replace the game's window procedure with ours
//
// SetWindowLongPtr with GWLP_WNDPROC replaces the window procedure.
// It returns the previous procedure, which we save so we can forward
// messages we don't handle.
// ============================================================================
bool WindowHook::Install(HWND gameWindow)
{
    if (!gameWindow || !IsWindow(gameWindow))
        return false;

    g_hwnd = gameWindow;
    g_shuttingDown = false;

    Log("WindowHook::Install on HWND %p", gameWindow);

    // Replace the window procedure and save the original
    g_origWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hook_WndProc))
    );

    if (!g_origWndProc)
    {
        Log("WindowHook::Install: SetWindowLongPtrA FAILED (error %d)", GetLastError());
        return false;
    }

    Log("WindowHook::Install: WndProc hooked successfully");

    // Initial cursor clip
    if (Config::Get().bClipCursor)
        ClipCursorToWindow(gameWindow, true);

    // Start the borderless enforcement timer.
    // This catches cases where the game re-applies borders during startup
    // AFTER our initial MakeWindowBorderless call in the CBT hook.
    if (Config::Get().bBorderlessWindowed)
    {
        g_borderlessChecks = 0;
        SetTimer(gameWindow, BORDERLESS_TIMER_ID, BORDERLESS_INTERVAL, nullptr);
        Log("WindowHook::Install: Borderless enforcement timer started");
    }

    return true;
}


// ============================================================================
// WindowHook::Cleanup - Restore everything to original state
//
// Called during DLL unload to prevent crashes from dangling hook pointers.
// ============================================================================
void WindowHook::Cleanup()
{
    Log("WindowHook::Cleanup");

    // Kill the borderless timer
    if (g_hwnd)
        KillTimer(g_hwnd, BORDERLESS_TIMER_ID);

    // Restore the original window procedure
    if (g_hwnd && g_origWndProc && IsWindow(g_hwnd))
    {
        SetWindowLongPtrA(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
        g_origWndProc = nullptr;
        Log("WindowHook::Cleanup: Original WndProc restored");
    }

    // Release cursor
    ClipCursor(nullptr);

    // Clean up audio (unmutes if still muted)
    CleanupAudio();

    if (g_wlog)
    {
        fclose(g_wlog);
        g_wlog = nullptr;
    }
}


bool WindowHook::HasFocus()
{
    return g_hasFocus;
}
