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

/*! \mainpage WAIVE-FRONT
 *
 * Welcome to the WAIVE-FRONT documentation. To start out, have a look at the WaiveFrontPluginUI class, which is the backbone of the plugin's user interface. From there, you can look at the ViewerWidget class, which handles the presentation views. If you're interested in the visual aspect of WAIVE-FRONT, check out the shaders in the /assets folder, mainly in main.frag. Video decoding is handled by the VideoLoader and OSC communication is handled by the OSCServer. Have fun, and of course, pull requests are welcome!
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "Superposition"
#define DISTRHO_PLUGIN_NAME "WAIVE-FRONT V2"
#define DISTRHO_PLUGIN_URI "https://superposition.cc"
#define DISTRHO_PLUGIN_CLAP_ID "cc.superposition.waive-front-v2"

#define DISTRHO_PLUGIN_BRAND_ID Sprp
#define DISTRHO_PLUGIN_UNIQUE_ID Wafr

#define DISTRHO_PLUGIN_HAS_UI 1
#define DISTRHO_PLUGIN_IS_RT_SAFE 1
#define DISTRHO_PLUGIN_NUM_INPUTS 2
#define DISTRHO_PLUGIN_NUM_OUTPUTS 2

#define DISTRHO_UI_USER_RESIZABLE 1
#define DISTRHO_UI_USE_CUSTOM 1
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui.hpp"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_DEFAULT_WIDTH 1280
#define DISTRHO_UI_DEFAULT_HEIGHT 820
#define DISTRHO_UI_USE_NANOVG 0
#define DISTRHO_UI_FILE_BROWSER 1

#define DGL_USE_OPENGL3 1

enum Parameters
{
	BlurSize,
	FocusDistance,
	Space,
	Zoom,
	BackgroundHue,
	BackgroundSaturation,
	BackgroundValue,
	EnableLayer1,
	EnableLayer2,
	EnableLayer3,
	RandomizeCategory1,
	RandomizeCategory2,
	RandomizeCategory3,
	RandomizeItem1,
	RandomizeItem2,
	RandomizeItem3,
	OSCNote1,
	OSCNote2,
	OSCNote3,
	OSCRetrigger1,
	OSCRetrigger2,
	OSCRetrigger3,
	NumParameters
}; /**< The parameters of the VST plugin */

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
