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
	 */
	ViewerWindow(Application &app, float (&p)[Parameters::NumParameters], std::vector<bool> *layersEnabled)
		: Window(app),
		  viewerWidget(new ViewerWidget(*this, p, layersEnabled))
	{
		setTitle("Viewer");
		setSize(1280, 720);
		setResizable(true);
		show();

		setOffsetY(getOffsetY() - 720 / 2);
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
