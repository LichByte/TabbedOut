# Tabbed Out

An NVSE plugin that fixes alt-tab crashes in **Fallout: New Vegas** by forcing borderless windowed mode.

No more device-lost crashes when you switch windows. The game stays running smoothly in the background.

## Features

- **Borderless Windowed Mode** — Forces the game into a borderless window matching your desktop resolution, preventing D3D9 device loss on alt-tab
- **Cursor Confinement** — Keeps the mouse cursor locked to the game window so it doesn't drift to other monitors
- **Anti-Minimize** — Prevents the game from minimizing itself when it loses focus
- **Configurable** — All features can be toggled via `AltTabFix.ini`

## Requirements

- [Fallout: New Vegas](https://store.steampowered.com/app/22380/)
- [xNVSE](https://www.nexusmods.com/newvegas/mods/67883) (v6.3.0+)

## Installation

1. Install xNVSE if you haven't already
2. Drop `AltTabFix.dll` and `AltTabFix.ini` into `Data/NVSE/Plugins/`
3. Launch the game through NVSE

Or install with a mod manager — the archive is structured for drag-and-drop.

## Configuration

Edit `Data/NVSE/Plugins/AltTabFix.ini`:

```ini
[Main]
bBorderlessWindowed=1   ; Core alt-tab fix (borderless windowed mode)
bPauseOnFocusLoss=1     ; Auto-pause when alt-tabbed
bMuteOnFocusLoss=1      ; Mute audio when alt-tabbed
bClipCursor=1           ; Confine cursor to game window

[Display]
iCustomWidth=0          ; Custom width (0 = desktop resolution)
iCustomHeight=0         ; Custom height (0 = desktop resolution)
iMonitor=0              ; Monitor index (0 = primary)
bTopMost=0              ; Keep window always on top
```

## Building from Source

Requires CMake 3.15+ and a C++17 compiler. **Must target x86** — FNV is 32-bit.

```bash
mkdir build && cd build
cmake .. -A Win32
cmake --build . --config Release
```

The DLL will be in `build/Release/AltTabFix.dll`.

## How It Works

The plugin hooks into the game at three levels:

1. **INI Interception** — Hooks `GetPrivateProfileIntA` to override `bFull Screen=0` and inject the desktop resolution, so the game thinks it's running windowed at native res
2. **Window Subclassing** — Replaces the game's window procedure to swallow focus-loss messages (`WM_ACTIVATEAPP`, `WM_ACTIVATE`) that would normally trigger a minimize
3. **ShowWindow Hook** — Blocks `SW_MINIMIZE` calls to prevent the window from disappearing

## Links

- [Nexus Mods Page](https://www.nexusmods.com/newvegas/mods/97897)

## License

MIT
