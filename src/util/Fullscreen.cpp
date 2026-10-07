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

#ifndef FULLSCREEN_CPP
#define FULLSCREEN_CPP

#if defined(__APPLE__)
#include <objc/message.h>
#include <objc/runtime.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

#include <cstdint>

namespace Util
{
	/**
	 * @brief Fullscreen for a window of our own, such as the viewer
	 *
	 * macOS: the system's own fullscreen, on the screen the window is on.
	 * Windows: borderless over the whole monitor the window is on, above all other windows,
	 * including the host's plugin window.
	 */
	class Fullscreen
	{
	public:
		/**
		 * @brief Whether the window is fullscreen now
		 *
		 * @param handle The window's native handle (Window::getNativeWindowHandle())
		 */
		bool isActive(uintptr_t handle) const
		{
#if defined(__APPLE__)
			id window = nsWindow(handle);
			if (window == nullptr)
				return false;
			const unsigned long styleMask = ((unsigned long (*)(id, SEL))objc_msgSend)(window, sel_registerName("styleMask"));
			return (styleMask & (1UL << 14)) != 0; // NSWindowStyleMaskFullScreen
#else
			(void)handle;
			return active;
#endif
		}

		/**
		 * @brief Switch fullscreen on or off
		 *
		 * @param handle The window's native handle (Window::getNativeWindowHandle())
		 */
		void toggle(uintptr_t handle)
		{
#if defined(__APPLE__)
			id window = nsWindow(handle);
			if (window == nullptr)
				return;

			// Allow fullscreen for this window, then ask for it
			const unsigned long behavior = ((unsigned long (*)(id, SEL))objc_msgSend)(window, sel_registerName("collectionBehavior"));
			((void (*)(id, SEL, unsigned long))objc_msgSend)(window, sel_registerName("setCollectionBehavior:"), behavior | (1UL << 7)); // FullScreenPrimary
			((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("toggleFullScreen:"), nullptr);
#elif defined(_WIN32)
			HWND hwnd = (HWND)handle;
			if (hwnd == nullptr)
				return;

			if (!active)
			{
				MONITORINFO monitor = {sizeof(MONITORINFO)};
				if (!GetWindowRect(hwnd, &savedRect) || !GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &monitor))
					return;

				savedStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
				SetWindowLongPtr(hwnd, GWL_STYLE, (savedStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
				const RECT &r = monitor.rcMonitor;
				SetWindowPos(hwnd, HWND_TOPMOST, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
				active = true;
			}
			else
			{
				SetWindowLongPtr(hwnd, GWL_STYLE, savedStyle);
				SetWindowPos(hwnd, HWND_NOTOPMOST, savedRect.left, savedRect.top, savedRect.right - savedRect.left,
							 savedRect.bottom - savedRect.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
				active = false;
			}
#else
			(void)handle;
#endif
		}

	private:
#if defined(__APPLE__)
		static id nsWindow(uintptr_t handle)
		{
			id view = (id)handle;
			return view != nullptr ? ((id(*)(id, SEL))objc_msgSend)(view, sel_registerName("window")) : nullptr;
		}
#endif

		bool active = false; /**< Windows: fullscreen now */
#if defined(_WIN32)
		LONG_PTR savedStyle = 0; /**< Windows: the window style before fullscreen */
		RECT savedRect = {};	 /**< Windows: the window's place before fullscreen */
#endif
	};
}

#endif
