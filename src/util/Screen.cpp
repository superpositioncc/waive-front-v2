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
#endif

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
	}
}

#endif
