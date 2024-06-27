/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

#define DISTRHO_UI_USER_RESIZABLE 0
#define DISTRHO_UI_USE_CUSTOM 1
#define DISTRHO_UI_CUSTOM_INCLUDE_PATH "DearImGui.hpp"
#define DISTRHO_UI_CUSTOM_WIDGET_TYPE DGL_NAMESPACE::ImGuiTopLevelWidget
#define DISTRHO_UI_DEFAULT_WIDTH 1280
#define DISTRHO_UI_DEFAULT_HEIGHT 1280
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
	// Threshold,
	// Width,
	NumParameters
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
