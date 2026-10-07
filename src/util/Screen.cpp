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

#ifndef SCREEN_CPP
#define SCREEN_CPP

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <objc/message.h>
#include <objc/runtime.h>
#endif

#include <cstdint>

#include <algorithm>

namespace Util
{
	namespace Screen
	{
		/**
		 * @brief How much larger to open the windows, so they have about the same physical size on every screen
		 *
		 * macOS sizes windows in points, and a point is smaller on a laptop set to 'More space' than on a
		 * desktop monitor. This measures the screen under the mouse, where the user just opened the plugin.
		 * On Windows and Linux the system's own display scaling takes care of this, so it returns 1.
		 *
		 * @return double Factor between 1 and 2
		 */
		static double densityFactor()
		{
#ifdef __APPLE__
			// Points per inch of a typical desktop monitor at 1x
			const double referenceDensity = 110.0;

			CGDirectDisplayID display = CGMainDisplayID();

			CGEventRef event = CGEventCreate(nullptr);
			if (event != nullptr)
			{
				CGPoint mouse = CGEventGetLocation(event);
				CFRelease(event);

				CGDirectDisplayID underMouse;
				uint32_t count = 0;
				if (CGGetDisplaysWithPoint(mouse, 1, &underMouse, &count) == kCGErrorSuccess && count > 0)
					display = underMouse;
			}

			const CGSize millimetres = CGDisplayScreenSize(display);
			const CGRect points = CGDisplayBounds(display);

			if (millimetres.width <= 0 || points.size.width <= 0)
				return 1.0;

			const double density = points.size.width / (millimetres.width / 25.4);
			return std::min(2.0, std::max(1.0, density / referenceDensity));
#else
			return 1.0;
#endif
		}

		/**
		 * @brief The actual size and pixel density of a native view
		 */
		struct ViewMetrics
		{
			double width = 0.0;	  /**< Width in points */
			double height = 0.0;  /**< Height in points */
			double backing = 1.0; /**< Pixels per point of the screen the view is on now */
			double visibleWidth = 0.0;  /**< Width of the part the host shows, in points */
			double visibleHeight = 0.0; /**< Height of the part the host shows, in points (its bottom part) */
		};

		/**
		 * @brief Measure a native view as macOS sees it right now
		 *
		 * DPF fixes its scale factor when the plugin opens, and macOS changes the pixel density of the
		 * view when its window moves to another screen. This asks the view itself.
		 *
		 * @param handle The native view (Window::getNativeWindowHandle())
		 * @param metrics The result
		 * @return false where this cannot be measured (not macOS, or no window yet)
		 */
		static bool viewMetrics(uintptr_t handle, ViewMetrics &metrics)
		{
#ifdef __APPLE__
			id view = (id)handle;
			if (view == nullptr)
				return false;

			id window = ((id(*)(id, SEL))objc_msgSend)(view, sel_registerName("window"));
			if (window == nullptr)
				return false;

#if defined(__x86_64__)
			const CGRect bounds = ((CGRect(*)(id, SEL))objc_msgSend_stret)(view, sel_registerName("bounds"));
#else
			const CGRect bounds = ((CGRect(*)(id, SEL))objc_msgSend)(view, sel_registerName("bounds"));
#endif
			const double backing = ((double (*)(id, SEL))objc_msgSend)(window, sel_registerName("backingScaleFactor"));

			if (bounds.size.width <= 0 || bounds.size.height <= 0 || backing <= 0)
				return false;

			metrics.width = bounds.size.width;
			metrics.height = bounds.size.height;
			metrics.backing = backing;
			metrics.visibleWidth = bounds.size.width;
			metrics.visibleHeight = bounds.size.height;

			// The host's view can be smaller than the plugin's; only that part is visible
			id host = ((id(*)(id, SEL))objc_msgSend)(view, sel_registerName("superview"));
			if (host != nullptr)
			{
#if defined(__x86_64__)
				const CGRect hostBounds = ((CGRect(*)(id, SEL))objc_msgSend_stret)(host, sel_registerName("bounds"));
#else
				const CGRect hostBounds = ((CGRect(*)(id, SEL))objc_msgSend)(host, sel_registerName("bounds"));
#endif
				if (hostBounds.size.width > 0 && hostBounds.size.height > 0)
				{
					metrics.visibleWidth = std::min(metrics.width, (double)hostBounds.size.width);
					metrics.visibleHeight = std::min(metrics.height, (double)hostBounds.size.height);
				}
			}
			return true;
#else
			(void)handle;
			(void)metrics;
			return false;
#endif
		}

		/**
		 * @brief Give the host's view the size of the plugin's view
		 *
		 * DPF's Audio Unit wrapper sizes the host's view with its own scale factor, while pugl sizes
		 * the plugin's view for the screen it is on. On a Retina screen they disagree, and the UI
		 * ends up in a corner of a view twice its size. The plugin's view is the right one.
		 *
		 * @param handle The native view (Window::getNativeWindowHandle())
		 */
		static void fitHostView(uintptr_t handle)
		{
#ifdef __APPLE__
			id view = (id)handle;
			if (view == nullptr)
				return;

			id host = ((id(*)(id, SEL))objc_msgSend)(view, sel_registerName("superview"));
			if (host == nullptr)
				return;

#if defined(__x86_64__)
			const CGRect hostFrame = ((CGRect(*)(id, SEL))objc_msgSend_stret)(host, sel_registerName("frame"));
			const CGRect frame = ((CGRect(*)(id, SEL))objc_msgSend_stret)(view, sel_registerName("frame"));
#else
			const CGRect hostFrame = ((CGRect(*)(id, SEL))objc_msgSend)(host, sel_registerName("frame"));
			const CGRect frame = ((CGRect(*)(id, SEL))objc_msgSend)(view, sel_registerName("frame"));
#endif
			if (frame.size.width <= 0 || frame.size.height <= 0)
				return;

			if (!CGSizeEqualToSize(hostFrame.size, frame.size))
				((void (*)(id, SEL, CGSize))objc_msgSend)(host, sel_registerName("setFrameSize:"), frame.size);

			if (frame.origin.x != 0 || frame.origin.y != 0)
				((void (*)(id, SEL, CGPoint))objc_msgSend)(view, sel_registerName("setFrameOrigin:"), CGPointZero);
#else
			(void)handle;
#endif
		}
	}
}

#endif
