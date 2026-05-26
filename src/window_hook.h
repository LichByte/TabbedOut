// ============================================================================
// window_hook.h - Window procedure hooking for focus management
//
// WHAT THIS DOES:
// When you alt-tab, Windows sends messages to the game window telling it
// that it lost focus (WM_ACTIVATEAPP). We intercept these messages to:
//   - Auto-pause the game
//   - Mute/unmute audio
//   - Clip/unclip the mouse cursor
//   - Prevent the game from doing anything stupid on focus loss
// ============================================================================
#pragma once
#include <windows.h>

namespace WindowHook
{
    // Install the window procedure hook. Call after D3D9Hook::Install()
    // because we need the game window handle.
    bool Install(HWND gameWindow);

    // Check if the game currently has focus
    bool HasFocus();
}
