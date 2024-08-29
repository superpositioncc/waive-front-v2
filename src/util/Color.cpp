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

#pragma once

/**
 * @brief Various utility functions
 */
namespace Util
{
	/**
	 * @brief Various functions relating to color
	 *
	 */
	namespace Color
	{
		/**
		 * @brief Convert RGB to HSV
		 *
		 * @param r Red
		 * @param g Green
		 * @param b Blue
		 * @return float* HSV
		 */
		float *HSVtoRGB(float h, float s, float v)
		{
			float r, g, b;

			int i = int(h * 6);
			float f = h * 6 - i;
			float p = v * (1 - s);
			float q = v * (1 - f * s);
			float t = v * (1 - (1 - f) * s);

			switch (i % 6)
			{
			case 0:
				r = v;
				g = t;
				b = p;
				break;
			case 1:
				r = q;
				g = v;
				b = p;
				break;
			case 2:
				r = p;
				g = v;
				b = t;
				break;
			case 3:
				r = p;
				g = q;
				b = v;
				break;
			case 4:
				r = t;
				g = p;
				b = v;
				break;
			case 5:
				r = v;
				g = p;
				b = q;
				break;
			}

			float *rgb = new float[3];
			rgb[0] = r;
			rgb[1] = g;
			rgb[2] = b;

			return rgb;
		}

	}
}
