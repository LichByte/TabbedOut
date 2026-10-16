// ============================================================================
// monitor.h - Pick the monitor and rectangle the game window should cover
//
// Shared by the INI hook (which tells the game what resolution to render at)
// and the window code (which sizes/positions the borderless window), so the
// two always agree.
// ============================================================================
#pragma once
#include <windows.h>
#include <vector>
#include "config.h"

namespace Monitor
{
    // ========================================================================
    // GetMonitorRectByIndex - Bounds of the monitor chosen by iMonitor
    //
    // Index 0 is always the primary monitor. 1, 2, ... are the remaining
    // monitors in the order Windows enumerates them. An index that doesn't
    // exist (e.g. a monitor was unplugged) falls back to the primary.
    // ========================================================================
    inline RECT GetMonitorRectByIndex(int index)
    {
        std::vector<RECT> rects;   // primary first, then the others

        EnumDisplayMonitors(nullptr, nullptr,
            [](HMONITOR hMon, HDC, LPRECT, LPARAM data) -> BOOL
            {
                auto& out = *reinterpret_cast<std::vector<RECT>*>(data);
                MONITORINFO mi = {};
                mi.cbSize = sizeof(mi);
                if (GetMonitorInfoA(hMon, &mi))
                {
                    if (mi.dwFlags & MONITORINFOF_PRIMARY)
                        out.insert(out.begin(), mi.rcMonitor);
                    else
                        out.push_back(mi.rcMonitor);
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&rects));

        if (index >= 0 && index < static_cast<int>(rects.size()))
            return rects[index];
        if (!rects.empty())
            return rects[0];

        // Fallback: assume primary monitor at (0,0)
        RECT r = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        return r;
    }

    // ========================================================================
    // GetTargetRect - Where the game window should be, in screen coordinates
    //
    // Starts at the top-left of the configured monitor. Each of
    // iCustomWidth / iCustomHeight overrides that dimension when > 0;
    // otherwise the monitor's own size is used.
    // ========================================================================
    inline RECT GetTargetRect()
    {
        Config& cfg = Config::Get();
        RECT mon = GetMonitorRectByIndex(cfg.iMonitor);

        int w = cfg.iCustomWidth  > 0 ? cfg.iCustomWidth  : mon.right - mon.left;
        int h = cfg.iCustomHeight > 0 ? cfg.iCustomHeight : mon.bottom - mon.top;

        RECT r = { mon.left, mon.top, mon.left + w, mon.top + h };
        return r;
    }
}
