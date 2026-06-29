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

#else

static inline void waiveUpdateGLDrawable(uintptr_t) {}

#endif

#endif // WAIVE_MAC_GL_H
