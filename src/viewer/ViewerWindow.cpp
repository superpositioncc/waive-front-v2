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

#ifndef VIEWER_WINDOW_CPP
#define VIEWER_WINDOW_CPP

#include "../util/Screen.cpp"
#include "DistrhoUI.hpp"
#include "ViewerWidget.cpp"
#include <vector>

START_NAMESPACE_DISTRHO

/**
 * @brief Viewer window is a window that displays the viewer widget, the presentation that the audience sees
 *
 */
class ViewerWindow : public Window
{
public:
	/**
	 * @brief Construct a new Viewer Window object
	 *
	 * @param app Application
	 * @param p Parameters
	 * @param layersEnabled Vector of booleans representing which layers have been enabled
	 * @param owner Native handle of a window that owns the viewer and keeps it in front, or 0
	 */
	ViewerWindow(Application &app, float (&p)[Parameters::NumParameters], std::vector<bool> *layersEnabled, uintptr_t owner = 0)
		// Created resizable: on macOS the drawing surface only follows the window
		// when the window is resizable from the start; setResizable() later is too late
		: Window(app, 0, 640, 360, 0.0, true),
		  viewerWidget(new ViewerWidget(*this, p, layersEnabled))
	{
		const double scale = getScaleFactor() * Util::Screen::densityFactor();

		setTitle("Viewer");
		setSize(640 * scale, 360 * scale);
		setResizable(true);

		if (owner != 0)
			setTransientParent(owner);

		// Near the top left of the main screen, so it is always visible
		setOffset(80 * scale, 80 * scale);
		show();
	}

	/**
	 * @brief Get the viewer widget
	 *
	 * @return ViewerWidget* Viewer widget
	 */
	ViewerWidget *getViewerWidget()
	{
		return viewerWidget;
	}

private:
	ViewerWidget *viewerWidget; /**< Viewer widget */

	DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewerWindow)
};

END_NAMESPACE_DISTRHO

#endif
