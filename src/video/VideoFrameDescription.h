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
 * @brief Stores a video frame description
 *
 */
struct VideoFrameDescription
{
	unsigned char *data; /**< Data of the frame */
	int width;			 /**< Width of the frame */
	int height;			 /**< Height of the frame */
	bool ready;			 /**< Whether the frame is ready */
};
