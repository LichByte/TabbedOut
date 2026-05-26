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
// ============================================================================

#include "window_hook.h"
#include "config.h"

// ============================================================================
// State
// ============================================================================
static HWND    g_hwnd          = nullptr;
static WNDPROC g_origWndProc   = nullptr;  // The game's original window procedure
static bool    g_hasFocus      = true;
static bool    g_cursorClipped = false;


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
// ============================================================================
static void OnFocusGained(HWND hwnd)
{
    g_hasFocus = true;

    Config& cfg = Config::Get();

    // Re-clip cursor to window
    if (cfg.bClipCursor)
        ClipCursorToWindow(hwnd, true);

    // Show the cursor inside the game (the game manages its own cursor,
    // but we make sure Windows isn't hiding it)
}


// ============================================================================
// OnFocusLost - Called when the game window loses focus (alt-tab)
// ============================================================================
static void OnFocusLost(HWND hwnd)
{
    g_hasFocus = false;

    Config& cfg = Config::Get();

    // Release cursor so it can move to other windows/monitors
    if (cfg.bClipCursor)
        ClipCursorToWindow(hwnd, false);

    // Note: actual game pause and audio mute would require hooking into
    // the game's own systems. For now, releasing the cursor is the most
    // important thing for a smooth alt-tab experience.
}


// ============================================================================
// Hook_WndProc - Our replacement window procedure
//
// IMPORTANT MESSAGES WE HANDLE:
//   WM_ACTIVATEAPP  - Sent when the app gains/loses focus
//   WM_ACTIVATE     - Sent when the window is activated/deactivated
//   WM_SETCURSOR    - Sent when the cursor moves over the window
//   WM_KILLFOCUS    - Sent when the window loses keyboard focus
//   WM_SETFOCUS     - Sent when the window gains keyboard focus
// ============================================================================
static LRESULT CALLBACK Hook_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_ACTIVATEAPP:
        // wParam: TRUE if being activated, FALSE if deactivated
        //
        // KEY FIX: We do NOT forward focus-loss to the game's WndProc.
        // The game's original handler minimizes the window and pauses
        // when it sees WM_ACTIVATEAPP(FALSE). By swallowing it, the
        // game never knows it lost focus -- the window stays visible
        // behind other windows, exactly like borderless should behave.
        //
        // We still handle cursor clipping ourselves.
        if (wParam)
        {
            OnFocusGained(hwnd);
            // Forward activation so the game resumes input properly
            return CallWindowProcA(g_origWndProc, hwnd, msg, wParam, lParam);
        }
        else
        {
            OnFocusLost(hwnd);
            // SWALLOW -- don't tell the game it lost focus
            return 0;
        }

    case WM_ACTIVATE:
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

    case WM_SETCURSOR:
        // If the cursor is in the client area and we have focus, clip it
        if (LOWORD(lParam) == HTCLIENT && g_hasFocus && Config::Get().bClipCursor && !g_cursorClipped)
            ClipCursorToWindow(hwnd, true);
        break;

    case WM_SIZE:
        // Block minimize -- the game tries to minimize on focus loss
        if (wParam == SIZE_MINIMIZED)
            return 0;  // SWALLOW -- prevent minimize
        break;

    case WM_SYSCOMMAND:
    {
        WPARAM cmd = wParam & 0xFFF0;

        // Block the game from minimizing itself
        if (cmd == SC_MINIMIZE)
            return 0;

        // Prevent screensaver and monitor power-off while game is focused
        if (g_hasFocus && (cmd == SC_SCREENSAVE || cmd == SC_MONITORPOWER))
            return 0;
        break;
    }
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

    // Replace the window procedure and save the original
    g_origWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtrA(gameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Hook_WndProc))
    );

    if (!g_origWndProc)
        return false;

    // Initial cursor clip
    if (Config::Get().bClipCursor)
        ClipCursorToWindow(gameWindow, true);

    return true;
}


bool WindowHook::HasFocus()
{
    return g_hasFocus;
}
