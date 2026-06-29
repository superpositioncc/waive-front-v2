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

#ifndef WAIVE_FRONT_CONTROLLER_HPP
#define WAIVE_FRONT_CONTROLLER_HPP

#ifndef __APPLE__
#include <Windows.h>
#include <GL/glew.h>
#endif

#include "DistrhoUI.hpp"
#include "viewer/ViewerWindow.cpp"
#include "assets/themes/CinderTheme.cpp"
#include "DearImGui.hpp"
#include "video/VideoLoader.cpp"
#include "video/VideoFrameDescription.h"

#ifdef __APPLE__
#include <filesystem>
#else
#include <experimental/filesystem>
#endif

#include <iostream>
#include <chrono>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <functional>
#include "assets/fonts/SpaceMono_Regular.cpp"
#include <dirent.h>
#include "data/DataSources.hpp"
#include "util/Logger.cpp"
#include <vector>
#include "osc/OSCServer.cpp"

using namespace Util::Logger;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __APPLE__
namespace fs = std::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/**
 * @brief The WaiveFrontController holds all of the audio-agnostic state and logic shared between
 *        the VST/AU plugin UI and the standalone application: the parameter array, data sources,
 *        video loaders, OSC server, and the ImGui control panel rendering.
 *
 * The host (plugin UI or standalone window) owns the ViewerWindow and feeds the controller its
 * ViewerWidget each frame. Parameter edits made in the control panel are reported back through the
 * optional onParameterEdited callback so the plugin can forward them to the DAW; the standalone
 * leaves it unset because the parameters[] array is itself the single source of truth.
 */
class WaiveFrontController
{
public:
    float parameters[Parameters::NumParameters]; /**< The parameters of the plugin */
    DataSources dataSources;                     /**< The data sources */
    OSCServer *oscServer = nullptr;              /**< The OSC server */
    ImFont *regular = nullptr;                   /**< The regular font */

    /** Optional callback used by the plugin to mirror parameter edits to the DAW. */
    std::function<void(uint32_t, float)> onParameterEdited;

    /**
     * @brief Initialize the parameters with their default values.
     *
     * The standalone has no host to push defaults, so it calls this explicitly. The values mirror
     * the WaiveFrontPlugin constructor.
     */
    void setDefaults()
    {
        std::memset(parameters, 0, sizeof(float) * Parameters::NumParameters);

        parameters[FocusDistance] = 0.5f;
        parameters[BlurSize] = 0.05f;
        parameters[Space] = 0.1f;
        parameters[Zoom] = 0.0f;
        parameters[BackgroundHue] = 0.0f;
        parameters[BackgroundSaturation] = 0.0f;
        parameters[BackgroundValue] = 0.0f;
        parameters[EnableLayer1] = 1.0f;
        parameters[EnableLayer2] = 0.0f;
        parameters[EnableLayer3] = 0.0f;
        parameters[RandomizeCategory1] = 0.0f;
        parameters[RandomizeCategory2] = 0.0f;
        parameters[RandomizeCategory3] = 0.0f;
        parameters[RandomizeItem1] = 0.0f;
        parameters[RandomizeItem2] = 0.0f;
        parameters[RandomizeItem3] = 0.0f;
        parameters[OSCNote1] = 36;
        parameters[OSCNote2] = 42;
        parameters[OSCNote3] = 38;
        parameters[OSCRetrigger1] = true;
        parameters[OSCRetrigger2] = true;
        parameters[OSCRetrigger3] = true;
    }

    /**
     * @brief Load data sources, seed the layers and start the OSC server.
     *
     * Mirrors the original WaiveFrontPluginUI constructor body (minus window/UI setup).
     */
    void init()
    {
        std::srand(std::time(0));

        // MacOS and Linux
        char *home = getenv("HOME");
        // Windows
        if (home == nullptr)
        {
            home = getenv("USERPROFILE");
        }

        int notes[3] = {36, 42, 38};

        for (int i = 0; i < 3; i++)
        {
            videoLoaders.push_back(new VideoLoader());
            selectedCategories.push_back(nullptr);
            selectedItems.push_back(nullptr);
            layersEnabled.push_back(i == 0);
            layerNotes.push_back(notes[i]);
            layerRetrigger.push_back(true);
            lastMessages.push_back("");
        }

        loadDataSources(std::string(home) + "/Documents/WAIVE");

        for (int i = 0; i < 3; i++)
        {
            int randomIndex = std::rand() % dataSources.categories.size();

            selectCategory(i, dataSources.categories[randomIndex]);
        }

        oscServer = new OSCServer(8000, &dataSources);
    }

    /**
     * @brief Load the regular font into the current ImGui context (call once, on first display).
     */
    void initFonts()
    {
        ImGuiIO &io = ImGui::GetIO();
        regular = io.Fonts->AddFontFromMemoryCompressedTTF(SpaceMono_Regular_compressed_data, SpaceMono_Regular_compressed_size, 32.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    }

    /** Reference to the per-layer enabled flags, handed to the ViewerWidget. */
    std::vector<bool> *getLayersEnabled() { return &layersEnabled; }

    /**
     * @brief Per-frame, non-ImGui logic: sync parameters into state, handle randomize triggers and
     *        OSC messages, and push decoded video frames into the viewer widget.
     *
     * @param viewerWidget The widget that receives decoded frames.
     */
    void update(ViewerWidget *viewerWidget)
    {
        if (automatic)
            updateAutomatic();
        else
            updateManual();

        int64_t currentTime = getCurrentTime();

        for (int i = 0; i < videoLoaders.size(); i++)
        {
            if (!layersEnabled[i])
            {
                continue;
            }

            VideoLoader *videoLoader = videoLoaders[i];

            if (videoLoader->getStatus() == 1 && videoLoader->shouldGetNextFrame(currentTime))
            {
                VideoFrameDescription vfd = videoLoader->getFrame();

                if (vfd.data != nullptr && vfd.ready)
                {
                    viewerWidget->setFrame(i, vfd.data, vfd.width, vfd.height, videoLoader->getColors());
                }
            }
        }
    }

    /**
     * @brief Manual control logic: sync parameters into state, handle randomize triggers and OSC.
     */
    void updateManual()
    {
        if (parameters[EnableLayer1] != layersEnabled[0])
            layersEnabled[0] = parameters[EnableLayer1];

        if (parameters[EnableLayer2] != layersEnabled[1])
            layersEnabled[1] = parameters[EnableLayer2];

        if (parameters[EnableLayer3] != layersEnabled[2])
            layersEnabled[2] = parameters[EnableLayer3];

        if (parameters[OSCNote1] != layerNotes[0])
            layerNotes[0] = parameters[OSCNote1];

        if (parameters[OSCNote2] != layerNotes[1])
            layerNotes[1] = parameters[OSCNote2];

        if (parameters[OSCNote3] != layerNotes[2])
            layerNotes[2] = parameters[OSCNote3];

        if (parameters[OSCRetrigger1] != layerRetrigger[0])
            layerRetrigger[0] = parameters[OSCRetrigger1];

        if (parameters[OSCRetrigger2] != layerRetrigger[1])
            layerRetrigger[1] = parameters[OSCRetrigger2];

        if (parameters[OSCRetrigger3] != layerRetrigger[2])
            layerRetrigger[2] = parameters[OSCRetrigger3];

        if (parameters[RandomizeCategory1] != pRandomizeCategory[0] && parameters[RandomizeCategory1])
        {
            pRandomizeCategory[0] = parameters[RandomizeCategory1];

            randomizeCategory(0);
        }
        else if (!parameters[RandomizeCategory1])
        {
            pRandomizeCategory[0] = false;
        }

        if (parameters[RandomizeCategory2] != pRandomizeCategory[1] && parameters[RandomizeCategory2])
        {
            pRandomizeCategory[1] = parameters[RandomizeCategory2];

            randomizeCategory(0);
        }
        else if (!parameters[RandomizeCategory2])
        {
            pRandomizeCategory[1] = false;
        }

        if (parameters[RandomizeCategory3] != pRandomizeCategory[2] && parameters[RandomizeCategory3])
        {
            pRandomizeCategory[2] = parameters[RandomizeCategory3];

            randomizeCategory(0);
        }
        else if (!parameters[RandomizeCategory3])
        {
            pRandomizeCategory[2] = false;
        }

        if (parameters[RandomizeItem1] != pRandomizeItem[0] && parameters[RandomizeItem1])
        {
            pRandomizeItem[0] = parameters[RandomizeItem1];

            randomizeItem(0);
        }
        else if (!parameters[RandomizeItem1])
        {
            pRandomizeItem[0] = false;
        }

        if (parameters[RandomizeItem2] != pRandomizeItem[1] && parameters[RandomizeItem2])
        {
            pRandomizeItem[1] = parameters[RandomizeItem2];

            randomizeItem(0);
        }
        else if (!parameters[RandomizeItem2])
        {
            pRandomizeItem[1] = false;
        }

        if (parameters[RandomizeItem3] != pRandomizeItem[2] && parameters[RandomizeItem3])
        {
            pRandomizeItem[2] = parameters[RandomizeItem3];

            randomizeItem(0);
        }
        else if (!parameters[RandomizeItem3])
        {
            pRandomizeItem[2] = false;
        }

        if (allowOSC && oscServer->available())
        {
            OSCMessage message = oscServer->getMessage();

            int note = message.note;
            int layer = -1;

            for (int i = 0; i < 3; i++)
            {
                if (layerNotes[i] == note)
                {
                    layer = i;
                    break;
                }
            }

            if (layer != -1)
            {
                if (lastMessages[layer] != message.rawCategories)
                {
                    lastMessages[layer] = message.rawCategories;

                    print("OSC", "Received message for layer " + std::to_string(layer + 1));
                    selectCategory(layer, message.categories[0]);
                }
                else if (layerRetrigger[layer])
                {
                    videoLoaders[layer]->rewind();
                }
            }
        }
    }

    /**
     * @brief Render the ImGui control panel.
     *
     * @param width The host window width.
     * @param height The host window height.
     */
    void drawControls(float width, float height)
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(width / 4, 0), ImVec2(width / 4, height));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGui::PushFont(regular);

        ImGui::Begin("WAIVE-FRONT V2", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::Toggle((std::string("Automatic mode is ") + std::string(automatic ? "on" : "off")).c_str(), &automatic) && automatic)
            beginAutomatic();

        ImGui::Text("Blur Size");
        ImGui::SetNextItemWidth(width / 4);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("Blur Size", &parameters[BlurSize], 0.0f, 1.0f))
            editParameter(BlurSize, parameters[BlurSize]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("blur", AP_Blur, width);

        ImGui::Text("Focus Distance");
        ImGui::SetNextItemWidth(width / 4);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("Focus Distance", &parameters[FocusDistance], 0.0f, 1.0f))
            editParameter(FocusDistance, parameters[FocusDistance]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("focus", AP_Focus, width);

        ImGui::Text("Space");
        ImGui::SetNextItemWidth(width / 4);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("Space", &parameters[Space], 0.0f, 0.2f))
            editParameter(Space, parameters[Space]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("space", AP_Space, width);

        ImGui::Text("Zoom");
        ImGui::SetNextItemWidth(width / 4);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("Zoom", &parameters[Zoom], 0.0f, 1.0f))
            editParameter(Zoom, parameters[Zoom]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("zoom", AP_Zoom, width);

        ImGui::Text("Background Color");
        ImGui::SetNextItemWidth(width / 4);
        ImGui::BeginDisabled(automatic);
        float hsv[3] = {parameters[BackgroundHue], parameters[BackgroundSaturation], parameters[BackgroundValue]};
        if (ImGui::ColorPicker3("Background Color", hsv, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV))
        {
            editParameter(BackgroundHue, hsv[0]);
            editParameter(BackgroundSaturation, hsv[1]);
            editParameter(BackgroundValue, hsv[2]);

            parameters[BackgroundHue] = hsv[0];
            parameters[BackgroundSaturation] = hsv[1];
            parameters[BackgroundValue] = hsv[2];
        }
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("background", AP_Background, width);

        ImGui::BeginDisabled(automatic);
        ImGui::Toggle((std::string("OSC is ") + std::string(allowOSC ? "enabled" : "disabled")).c_str(), &allowOSC);
        ImGui::EndDisabled();

        ImGui::End();

        for (int i = 0; i < 3; i++)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(width / 4, 0), ImVec2(width / 4, height));
            ImGui::SetNextWindowPos(ImVec2((i + 1) * width / 4, 0));
            ImGui::Begin(("Layer " + std::to_string(i + 1)).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            ImGui::BeginDisabled(automatic);

            std::string buttonLabel = layersEnabled[i] ? "Disable Layer " + std::to_string(i + 1) : "Enable Layer " + std::to_string(i + 1);

            if (ImGui::Button(buttonLabel.c_str()))
            {
                layersEnabled[i] = !layersEnabled[i];

                if (i == 0)
                {
                    parameters[EnableLayer1] = layersEnabled[i];
                    editParameter(EnableLayer1, layersEnabled[i]);
                }
                else if (i == 1)
                {
                    parameters[EnableLayer2] = layersEnabled[i];
                    editParameter(EnableLayer2, layersEnabled[i]);
                }
                else if (i == 2)
                {
                    parameters[EnableLayer3] = layersEnabled[i];
                    editParameter(EnableLayer3, layersEnabled[i]);
                }
            }

            if (layersEnabled[i])
            {
                if (allowOSC)
                {
                    ImGui::Text("OSC Note");
                    ImGui::SetNextItemWidth(width / 4);

                    if (ImGui::SliderInt("OSC Note", &layerNotes[i], 0, 127))
                    {
                        if (i == 0)
                        {
                            parameters[OSCNote1] = layerNotes[i];
                            editParameter(OSCNote1, layerNotes[i]);
                        }
                        else if (i == 1)
                        {
                            parameters[OSCNote2] = layerNotes[i];
                            editParameter(OSCNote2, layerNotes[i]);
                        }
                        else if (i == 2)
                        {
                            parameters[OSCNote3] = layerNotes[i];
                            editParameter(OSCNote3, layerNotes[i]);
                        }
                    }

                    bool retrigger = layerRetrigger[i];
                    if (ImGui::Toggle((std::string("OSC Retrigger ") + std::to_string(i + 1)).c_str(), &retrigger))
                    {
                        layerRetrigger[i] = retrigger;

                        if (i == 0)
                        {
                            parameters[OSCRetrigger1] = layerRetrigger[i];
                            editParameter(OSCRetrigger1, layerRetrigger[i]);
                        }
                        else if (i == 1)
                        {
                            parameters[OSCRetrigger2] = layerRetrigger[i];
                            editParameter(OSCRetrigger2, layerRetrigger[i]);
                        }
                        else if (i == 2)
                        {
                            parameters[OSCRetrigger3] = layerRetrigger[i];
                            editParameter(OSCRetrigger3, layerRetrigger[i]);
                        }
                    }
                }

                ImGui::Text("Category");
                if (ImGui::BeginCombo(("Category " + std::to_string(i + 1)).c_str(), selectedCategories[i] != nullptr ? selectedCategories[i]->presentationName.c_str() : "None"))
                {
                    for (DataCategory *category : dataSources.categories)
                    {
                        if (ImGui::Selectable(category->presentationName.c_str()))
                        {
                            selectCategory(i, category);
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::Button(("Select Random Category " + std::to_string(i + 1)).c_str()))
                {
                    randomizeCategory(i);
                }

                ImGui::Text("Item");
                if (ImGui::BeginCombo(("Item " + std::to_string(i + 1)).c_str(), selectedItems[i] != nullptr ? selectedItems[i]->title.c_str() : "None"))
                {
                    for (DataItem *item : selectedCategories[i]->items)
                    {
                        if (ImGui::Selectable(item->title.c_str()))
                        {
                            selectItem(i, item);
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::Button(("Select Random Item " + std::to_string(i + 1)).c_str()))
                {
                    randomizeItem(i);
                }

                ImGui::TextWrapped(selectedItems[i] != nullptr ? selectedItems[i]->title.c_str() : "None");

                std::vector<float> colors = videoLoaders[i]->getColors();

                if (colors.size() > 0)
                {
                    ImGui::Columns(colors.size() / 3, nullptr, false);

                    for (int j = 0; j < colors.size(); j += 3)
                    {
                        float r = colors[j];
                        float g = colors[j + 1];
                        float b = colors[j + 2];

                        ImGui::ColorButton(("Color " + std::to_string(j / 3)).c_str(), ImVec4(r, g, b, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(width / 4 / 5, width / 4 / 5));
                        ImGui::NextColumn();
                    }

                    ImGui::Columns(1);
                }
            }

            ImGui::EndDisabled();
            ImGui::End();
        }

        ImGui::PopFont();
    }

private:
    bool pRandomizeCategory[3] = {false, false, false}; /**< Whether to randomize the category on the next frame */
    bool pRandomizeItem[3] = {false, false, false};     /**< Whether to randomize the item on the next frame */
    bool allowOSC = true;                               /**< Whether to allow OSC control */
    bool automatic = false;                             /**< Whether automatic "demo" mode is active */

    /** The visual parameters that automatic mode drives, each with its own speed/chaos. */
    enum AutoParam
    {
        AP_Blur,
        AP_Focus,
        AP_Space,
        AP_Zoom,
        AP_Background,
        AP_Count
    };

    float autoSpeed[AP_Count] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}; /**< Per-parameter: how fast values change */
    float autoChaos[AP_Count] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}; /**< Per-parameter: 1 = full range, 0 = settled */

    /**
     * @brief Timing and phase state for the automatic "demo" mode engine.
     */
    struct AutoState
    {
        double lastReal = 0.0;           /**< Real (wall-clock) time at the previous frame, seconds */
        double clock[AP_Count] = {0, 0, 0, 0, 0}; /**< Per-parameter speed-scaled time */
        double structClock = 0.0;        /**< Steady (unscaled) clock driving structural changes */
        double lastBeat = 0.0;           /**< Time of the last rhythmic blur pulse (blur clock) */
        double nextBeat = 0.0;           /**< Time of the next rhythmic blur pulse (blur clock) */
        double beatInterval = 0.5;       /**< Current pulse interval */
        double nextItemChange[3] = {0, 0, 0};     /**< When to randomize each layer's item */
        double nextCategoryChange[3] = {0, 0, 0}; /**< When to randomize each layer's category */
        double nextLayerToggle = 0.0;    /**< When to toggle a random layer */
        float noiseFrom = 0.5f;          /**< Start value of the current noise segment */
        float noiseTo = 0.5f;            /**< Target value of the current noise segment */
        double noiseStart = 0.0;         /**< When the current noise segment started (focus clock) */
        double noiseDur = 1.0;           /**< Duration of the current noise segment */
    } autoState; /**< The automatic mode engine state */

    std::vector<VideoLoader *> videoLoaders;        /**< The video loaders */
    std::vector<DataCategory *> selectedCategories; /**< The selected categories */
    std::vector<DataItem *> selectedItems;          /**< The selected items */

    std::vector<bool> layersEnabled;       /**< Whether each layer is enabled */
    std::vector<bool> layerRetrigger;      /**< Whether each layer is retriggered on each note */
    std::vector<int> layerNotes;           /**< Which note each layer should respond to */
    std::vector<std::string> lastMessages; /**< The last messages received */

    /**
     * @brief Report a parameter edit to the host (if a callback was provided).
     */
    void editParameter(uint32_t index, float value)
    {
        if (onParameterEdited)
            onParameterEdited(index, value);
    }

    // ----- Automatic "demo" mode --------------------------------------------------------------

    /** @brief A random float in [0, 1]. */
    float randf()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }

    /** @brief A random float in [lo, hi]. */
    float randf(float lo, float hi)
    {
        return lo + (hi - lo) * randf();
    }

    /** @brief The current time in seconds (monotonic). */
    double nowSeconds()
    {
        return getCurrentTime() / 1e6;
    }

    /** @brief Clamp a value between a lower and upper bound. */
    float clampf(float v, float lo, float hi)
    {
        return std::max(lo, std::min(v, hi));
    }

    /** @brief A sine LFO at the given period (seconds), in [0, 1]. */
    float lfo01(double t, double period, float phase = 0.0f)
    {
        return 0.5f + 0.5f * std::sin(2.0 * M_PI * (t / period) + phase);
    }

    /** @brief Pull a normalized [0, 1] value toward its centre (0.5) according to the chaos amount. */
    float chaosCenter(float v01, float chaos)
    {
        return 0.5f + (v01 - 0.5f) * chaos;
    }

    /** @brief A sine LFO mapped to [lo, hi], pulled toward the range centre by the chaos amount. */
    float lfo(double t, double period, float lo, float hi, float chaos, float phase = 0.0f)
    {
        return lo + (hi - lo) * chaosCenter(lfo01(t, period, phase), chaos);
    }

    /** @brief Smoothed value noise in [0, 1] that drifts between random targets over time. */
    float valueNoise(double t)
    {
        if (t >= autoState.noiseStart + autoState.noiseDur)
        {
            autoState.noiseFrom = autoState.noiseTo;
            autoState.noiseTo = randf();
            autoState.noiseStart = t;
            autoState.noiseDur = randf(0.5f, 2.0f);
        }

        float x = clampf((float)((t - autoState.noiseStart) / autoState.noiseDur), 0.0f, 1.0f);
        float smooth = x * x * (3.0f - 2.0f * x); // smoothstep
        return autoState.noiseFrom + (autoState.noiseTo - autoState.noiseFrom) * smooth;
    }

    /**
     * @brief Seed the automatic mode engine (called on the toggle's rising edge).
     */
    void beginAutomatic()
    {
        autoState.lastReal = nowSeconds();
        autoState.structClock = 0.0;

        for (int g = 0; g < AP_Count; g++)
            autoState.clock[g] = 0.0;

        autoState.lastBeat = 0.0;
        autoState.beatInterval = randf(0.35f, 0.9f);
        autoState.nextBeat = 0.0;

        for (int i = 0; i < 3; i++)
        {
            autoState.nextItemChange[i] = randf(0.0f, 4.0f);
            autoState.nextCategoryChange[i] = randf(2.0f, 8.0f);
        }

        autoState.nextLayerToggle = randf(4.0f, 10.0f);

        autoState.noiseFrom = randf();
        autoState.noiseTo = randf();
        autoState.noiseStart = 0.0;
        autoState.noiseDur = randf(0.5f, 2.0f);
    }

    /**
     * @brief Per-frame automatic mode engine. Drives the visual parameters and structural choices,
     *        writing directly into parameters[] so the result is purely visual.
     */
    void updateAutomatic()
    {
        // Advance each parameter's own speed-scaled clock (so per-parameter "Speed" stretches or
        // compresses time independently and can be changed smoothly while running), plus a steady
        // clock for the structural changes.
        const double real = nowSeconds();
        const double dt = real - autoState.lastReal;
        autoState.lastReal = real;

        for (int g = 0; g < AP_Count; g++)
            autoState.clock[g] += dt * autoSpeed[g];
        autoState.structClock += dt;

        const double tBlur = autoState.clock[AP_Blur];
        const double tFocus = autoState.clock[AP_Focus];
        const double tSpace = autoState.clock[AP_Space];
        const double tZoom = autoState.clock[AP_Zoom];
        const double tBg = autoState.clock[AP_Background];
        const double ts = autoState.structClock;

        // --- Slow LFOs (lfo() pulls values toward the range centre by that parameter's chaos) ---
        parameters[Space] = lfo(tSpace, 11.0, 0.02f, 0.18f, autoChaos[AP_Space]);
        parameters[FocusDistance] = clampf(lfo(tFocus, 14.0, 0.1f, 0.9f, autoChaos[AP_Focus]) + 0.15f * autoChaos[AP_Focus] * (valueNoise(tFocus) - 0.5f), 0.0f, 1.0f);
        parameters[Zoom] = lfo(tZoom, 18.0, 0.0f, 0.7f, autoChaos[AP_Zoom]);

        parameters[BackgroundHue] = (float)std::fmod(tBg * 0.03, 1.0);
        // Saturation and value use the full 0-1 range; chaos scales them down toward
        // unsaturated / black rather than pulling toward the range centre.
        parameters[BackgroundSaturation] = lfo01(tBg, 23.0) * autoChaos[AP_Background];
        parameters[BackgroundValue] = lfo01(tBg, 29.0) * autoChaos[AP_Background];

        // --- Rhythmic blur pulse with falloff envelope (runs on the blur clock) ---------------
        if (tBlur >= autoState.nextBeat)
        {
            autoState.lastBeat = tBlur;
            autoState.beatInterval = randf(0.35f, 0.9f);
            autoState.nextBeat = tBlur + autoState.beatInterval;
        }

        float env = std::exp(-6.0 * (tBlur - autoState.lastBeat));
        // Chaos scales the blur pulse down toward 0 rather than pulling it toward the centre.
        parameters[BlurSize] = clampf(0.05f + 0.6f * env, 0.0f, 1.0f) * autoChaos[AP_Blur];

        // --- Structural variety: items, categories, layers (steady clock) --------------------
        for (int i = 0; i < 3; i++)
        {
            if (ts >= autoState.nextItemChange[i])
            {
                if (layersEnabled[i])
                    randomizeItem(i);

                autoState.nextItemChange[i] = ts + randf(2.0f, 7.0f);
            }

            if (ts >= autoState.nextCategoryChange[i])
            {
                if (layersEnabled[i])
                    randomizeCategory(i);

                autoState.nextCategoryChange[i] = ts + randf(6.0f, 16.0f);
            }
        }

        if (ts >= autoState.nextLayerToggle)
        {
            int i = std::rand() % 3;
            bool newState = !layersEnabled[i];

            // Guarantee at least one layer stays enabled.
            if (!newState)
            {
                int enabledCount = 0;
                for (int j = 0; j < 3; j++)
                    if (layersEnabled[j])
                        enabledCount++;

                if (enabledCount <= 1)
                {
                    // Instead of turning off the last layer, enable a different one.
                    i = (i + 1 + (std::rand() % 2)) % 3;
                    newState = true;
                }
            }

            setLayerEnabled(i, newState);
            autoState.nextLayerToggle = ts + randf(4.0f, 10.0f);
        }
    }

    /** @brief Draw the per-parameter "Speed" and "Chaos" sliders for automatic mode. */
    void drawAutoSliders(const char *id, int group, float width)
    {
        ImGui::SetNextItemWidth(width / 4);
        ImGui::SliderFloat((std::string("Speed##") + id).c_str(), &autoSpeed[group], 0.1f, 4.0f);
        ImGui::SetNextItemWidth(width / 4);
        ImGui::SliderFloat((std::string("Chaos##") + id).c_str(), &autoChaos[group], 0.0f, 1.0f);
    }

    /** @brief Set a layer's enabled state, updating both the state vector and the parameter. */
    void setLayerEnabled(int i, bool enabled)
    {
        layersEnabled[i] = enabled;
        parameters[EnableLayer1 + i] = enabled ? 1.0f : 0.0f;
    }

    /**
     * @brief Check if a file is a video file
     */
    bool isVideoFile(const char *filename)
    {
        std::string name = std::string(filename);
        std::string extension = name.substr(name.find_last_of(".") + 1);

        return extension == "mp4" || extension == "mov";
    }

    /**
     * @brief Load the data sources from a directory
     */
    void loadDataSources(std::string directory)
    {
        DIR *dir = opendir(directory.c_str());
        struct dirent *entry;

        if (dir == NULL)
        {
            return;
        }

        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = std::string(entry->d_name);

            if (name != "." && name != "..")
            {
                std::string path = std::string(directory) + "/" + name;
                fs::path p(path);

                if (!fs::is_directory(p))
                    continue;

                DataSource *dataSource = new DataSource(path);
                dataSource->load(&dataSources);

                if (dataSource->isValid())
                    dataSources.sources.push_back(dataSource);
                else
                    delete dataSource;
            }
        }

        closedir(dir);

        dataSources.collectItems();

        for (DataSource *dataSource : dataSources.sources)
        {
            print("DATA", "Loaded data source " + dataSource->name);
        }

        std::string categoriesPath = directory + "/categories.json";

        json categories;

        try
        {
            std::ifstream file(categoriesPath);
            file >> categories;

            int order = 0;
            for (json cat : categories)
            {
                std::string name = cat["category"].get<std::string>();
                std::string presentationName = cat["title"].get<std::string>();
                std::vector<std::string> triggers = cat["tags"].get<std::vector<std::string>>();

                DataCategory *category = nullptr;

                for (DataCategory *c : dataSources.categories)
                {
                    if (c->name == name)
                    {
                        category = c;
                        break;
                    }
                }

                if (category == nullptr)
                    continue;

                category->presentationName = presentationName;
                category->triggers = triggers;
                category->order = order;

                order++;
            }

            // Find any categories that have no presentation name
            for (DataCategory *category : dataSources.categories)
            {
                if (category->presentationName.empty())
                {
                    warn("DATA", "Category " + category->name + " has no presentation name");
                }
            }

            print("DATA", "Loaded categories from: " + categoriesPath);

            dataSources.sortCategories();
        }
        catch (const std::exception &e)
        {
            warn("DATA", "Failed to load categories from: " + categoriesPath);
            warn("DATA", e.what());
            return;
        }
    }

    /**
     * @brief Select a category
     */
    void selectCategory(int i, DataCategory *category)
    {
        selectedCategories[i] = category;

        print("DATA", "Selected category: " + category->name);
        randomizeItem(i);
    }

    /**
     * @brief Select an item
     */
    void selectItem(int i, DataItem *item)
    {
        selectedItems[i] = item;

        print("DATA", "Selected item: " + selectedItems[i]->title);

        std::string scenePath = selectedItems[i]->source->path + "/items/" + selectedItems[i]->filename + ".mp4";

        if (isVideoFile(scenePath.c_str()))
        {
            videoLoaders[i]->loadVideo(scenePath.c_str());
        }
    }

    /**
     * @brief Randomize a category
     */
    void randomizeCategory(int i)
    {
        int randomIndex = std::rand() % dataSources.categories.size();
        selectCategory(i, dataSources.categories[randomIndex]);
    }

    /**
     * @brief Randomize an item
     */
    void randomizeItem(int i)
    {
        int randomIndex = std::rand() % selectedCategories[i]->items.size();
        selectItem(i, selectedCategories[i]->items[randomIndex]);
    }

    /**
     * @brief Get the current time
     */
    int64_t getCurrentTime()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

END_NAMESPACE_DISTRHO

#endif // WAIVE_FRONT_CONTROLLER_HPP
