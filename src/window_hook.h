// ============================================================================
// window_hook.h - Window procedure hooking for focus management
//
// WHAT THIS DOES:
// When you alt-tab, Windows sends messages to the game window telling it
// that it lost focus (WM_ACTIVATEAPP). We intercept these messages to:
//   - Prevent the game from minimizing (stays visible behind other windows)
//   - Mute/unmute audio via WASAPI
//   - Clip/unclip the mouse cursor
//   - Enforce borderless window styles
//   - Handle clean shutdown (prevent freeze on quit)
// ============================================================================
#pragma once
#include <windows.h>

namespace WindowHook
{
    // Install the window procedure hook. Call after D3D9Hook::Install()
    // because we need the game window handle.
    bool Install(HWND gameWindow);

    // Clean up hooks and restore original state.
    // Call during DLL unload or game shutdown.
    void Cleanup();

    // Check if the game currently has focus
    bool HasFocus();
}
