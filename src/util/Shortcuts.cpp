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

#ifndef SHORTCUTS_CPP
#define SHORTCUTS_CPP

#include "Application.hpp"
#include "Widget.hpp"

START_NAMESPACE_DGL

/**
 * @brief Quit the standalone app on Command-Q (macOS)
 *
 * The standalone app has no menu bar, so macOS has no Quit item to send Command-Q to.
 * Inside a DAW this does nothing: the plugin must never quit its host.
 *
 * @return true if the event was Command-Q and the app is quitting
 */
static bool quitOnCommandQ(Application &app, const Widget::KeyboardEvent &event)
{
#ifdef DISTRHO_OS_MAC
	if (event.press && (event.mod & kModifierSuper) && (event.key == 'q' || event.key == 'Q') && app.isStandalone())
	{
		app.quit();
		return true;
	}
#endif
	return false;
}

END_NAMESPACE_DGL

#endif
