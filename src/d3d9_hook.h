// ============================================================================
// d3d9_hook.h - Direct3D 9 hooking for borderless windowed mode
//
// THE CORE FIX:
// Fallout: New Vegas runs in "exclusive fullscreen" mode by default. When you
// alt-tab, Windows has to yank the GPU away from the game (called "device loss"),
// and FNV handles this poorly -- causing crashes, black screens, or freezes.
//
// Our fix: intercept the game's call to IDirect3D9::CreateDevice() and change
// the parameters from "exclusive fullscreen" to "windowed mode". Then we make
// the window borderless and sized to fill the screen, so it LOOKS fullscreen
// but alt-tabbing is just a normal window switch. No device loss, no crash.
// ============================================================================
#pragma once
#include <windows.h>

namespace D3D9Hook
{
    // Call this from NVSEPlugin_Load to install all the hooks
    bool Install();

    // Get the game's window handle (set after hooks fire)
    HWND GetGameWindow();
}
