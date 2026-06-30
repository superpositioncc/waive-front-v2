/*
WAIVE-FRONT
Copyright (C) 2024  Bram Bogaerts, Superposition

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef WAIVE_MAC_GL_H
#define WAIVE_MAC_GL_H

#include <cstdint>

#ifdef __APPLE__

/**
 * @brief Force the NSOpenGLView drawable inside a pugl wrapper view to resize to the view bounds.
 *
 * On macOS the GL backing only resizes when [NSOpenGLContext update] runs, which AppKit normally
 * does during its display cycle. The standalone drives rendering manually (one window repaints
 * another), bypassing that, so the framebuffer stays frozen at its initial size on resize. Calling
 * this on every window reshape keeps the drawable in sync with the window.
 *
 * @param wrapperViewHandle The native window handle (pugl wrapper NSView), from getNativeWindowHandle().
 */
extern "C" void waiveUpdateGLDrawable(uintptr_t wrapperViewHandle);

/**
 * @brief Hide or show the mouse cursor (application-wide). Calls are balanced internally, so it is
 *        safe to call repeatedly with the same value.
 *
 * @param hidden Whether the cursor should be hidden.
 */
extern "C" void waiveSetCursorHidden(bool hidden);

/**
 * @brief Return the path to the bundled WAIVE data folder inside the .app bundle
 *        (Contents/Resources/WAIVE), or an empty string if not running as a bundle.
 */
extern "C" const char* waiveGetBundledDataPath();

#elif defined(_WIN32)

#include <windows.h>
#include <string>
#include <filesystem>

static inline void waiveUpdateGLDrawable(uintptr_t) {}

static bool s_waiveCursorHidden = false;

static inline void waiveSetCursorHidden(bool hidden)
{
    if (hidden == s_waiveCursorHidden) return;
    s_waiveCursorHidden = hidden;
    ShowCursor(hidden ? FALSE : TRUE);
}

static bool s_waiveIsFullscreen = false;
static RECT s_waiveSavedRect = {};
static LONG s_waiveSavedStyle = 0;

static inline void waiveToggleFullscreen(uintptr_t nativeHandle)
{
    HWND hwnd = reinterpret_cast<HWND>(nativeHandle);
    if (!s_waiveIsFullscreen) {
        s_waiveSavedStyle = GetWindowLong(hwnd, GWL_STYLE);
        GetWindowRect(hwnd, &s_waiveSavedRect);
        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfo(monitor, &mi);
        SetWindowLong(hwnd, GWL_STYLE, s_waiveSavedStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right  - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        s_waiveIsFullscreen = true;
    } else {
        SetWindowLong(hwnd, GWL_STYLE, s_waiveSavedStyle);
        SetWindowPos(hwnd, nullptr,
            s_waiveSavedRect.left, s_waiveSavedRect.top,
            s_waiveSavedRect.right  - s_waiveSavedRect.left,
            s_waiveSavedRect.bottom - s_waiveSavedRect.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        s_waiveIsFullscreen = false;
    }
}

static inline const char* waiveGetBundledDataPath()
{
    static std::string s;
    wchar_t exePath[32768] = {};
    GetModuleFileNameW(NULL, exePath, 32768);
    s = std::filesystem::path(exePath).parent_path().string() + "/WAIVE";
    return s.c_str();
}

#else

static inline void waiveUpdateGLDrawable(uintptr_t) {}
static inline void waiveSetCursorHidden(bool) {}
static inline const char* waiveGetBundledDataPath() { return ""; }

#endif

#endif // WAIVE_MAC_GL_H
