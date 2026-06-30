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

        // Automatic mode is on by default, so seed its engine here (the toggle only re-seeds on a
        // rising edge, which won't happen when it starts enabled).
        beginAutomatic();
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

        ImGui::SeparatorText("Blur Size");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("##blur", &parameters[BlurSize], 0.0f, 1.0f))
            editParameter(BlurSize, parameters[BlurSize]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("blur", AP_Blur);

        ImGui::SeparatorText("Focus Distance");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("##focus", &parameters[FocusDistance], 0.0f, 1.0f))
            editParameter(FocusDistance, parameters[FocusDistance]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("focus", AP_Focus);

        ImGui::SeparatorText("Space");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("##space", &parameters[Space], 0.0f, 0.2f))
            editParameter(Space, parameters[Space]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("space", AP_Space);

        ImGui::SeparatorText("Zoom");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::BeginDisabled(automatic);
        if (ImGui::SliderFloat("##zoom", &parameters[Zoom], 0.0f, 1.0f))
            editParameter(Zoom, parameters[Zoom]);
        ImGui::EndDisabled();
        if (automatic)
            drawAutoSliders("zoom", AP_Zoom);

        ImGui::SeparatorText("Background Color");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::BeginDisabled(automatic);
        float hsv[3] = {parameters[BackgroundHue], parameters[BackgroundSaturation], parameters[BackgroundValue]};
        if (ImGui::ColorPicker3("##background", hsv, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV))
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
            drawAutoSliders("background", AP_Background);

        ImGui::SeparatorText("OSC");
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
                    ImGui::SeparatorText("OSC Note");
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

                    if (ImGui::SliderInt(("##oscnote" + std::to_string(i)).c_str(), &layerNotes[i], 0, 127))
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

                ImGui::SeparatorText("Category");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo(("##category" + std::to_string(i)).c_str(), selectedCategories[i] != nullptr ? selectedCategories[i]->presentationName.c_str() : "None"))
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

                ImGui::SeparatorText("Item");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::BeginCombo(("##item" + std::to_string(i)).c_str(), selectedItems[i] != nullptr ? selectedItems[i]->title.c_str() : "None"))
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
    bool automatic = true;                              /**< Whether automatic "demo" mode is active */

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

    float autoSpeed[AP_Count] = {0.75f, 0.75f, 0.75f, 0.75f, 0.75f}; /**< Per-parameter: how fast values change */
    float autoChaos[AP_Count] = {0.35f, 0.35f, 0.35f, 0.35f, 0.35f}; /**< Per-parameter: 1 = full range, 0 = settled */

    /** The drivers a parameter can be modulated by; each parameter switches between them over time. */
    enum AutoDriver
    {
        DRIVER_LFO,   /**< Slow sine oscillation */
        DRIVER_PULSE, /**< Rhythmic pulse with exponential falloff */
        DRIVER_NOISE, /**< Smoothed random drift */
        DRIVER_COUNT
    };

    /** Per-parameter modulation state: which driver is active and each driver's own phase. */
    struct DriverState
    {
        double clock = 0.0;      /**< Speed-scaled time this parameter runs on */
        int driver = DRIVER_LFO; /**< Currently active driver */
        double nextSwitch = 0.0; /**< When to switch to a different driver */
        double lfoPeriod = 12.0; /**< LFO period (seconds) */
        double lastBeat = 0.0;   /**< Pulse: time of the last beat */
        double nextBeat = 0.0;   /**< Pulse: time of the next beat */
        float noiseFrom = 0.5f;  /**< Noise: start value of the current segment */
        float noiseTo = 0.5f;    /**< Noise: target value of the current segment */
        double noiseStart = 0.0; /**< Noise: when the current segment started */
        double noiseDur = 1.0;   /**< Noise: duration of the current segment */
    };

    /**
     * @brief Timing and phase state for the automatic "demo" mode engine.
     */
    struct AutoState
    {
        double lastReal = 0.0;                    /**< Real (wall-clock) time at the previous frame */
        double structClock = 0.0;                 /**< Steady clock driving structural changes */
        DriverState driver[AP_Count];             /**< Per-parameter driver/modulation state */
        double nextItemChange[3] = {0, 0, 0};     /**< When to randomize each layer's item */
        double nextCategoryChange[3] = {0, 0, 0}; /**< When to randomize each layer's category */
        double nextLayerToggle = 0.0;             /**< When to toggle a random layer */
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

    /** @brief Map a normalized [0, 1] value into [lo, hi], scaling toward lo as chaos drops to 0. */
    float mapTowardZero(float v01, float lo, float hi, float chaos)
    {
        return lo + (hi - lo) * v01 * chaos;
    }

    /** @brief Map a normalized [0, 1] value into [lo, hi], pulling toward the centre as chaos drops. */
    float mapTowardCentre(float v01, float lo, float hi, float chaos)
    {
        return lo + (hi - lo) * chaosCenter(v01, chaos);
    }

    /** @brief Evaluate a parameter's active driver in [0, 1], advancing that driver's phase. */
    float driverValue(DriverState &s)
    {
        const double t = s.clock;

        switch (s.driver)
        {
        case DRIVER_PULSE:
        {
            if (t >= s.nextBeat)
            {
                s.lastBeat = t;
                s.nextBeat = t + randf(0.35f, 0.9f);
            }
            return clampf((float)std::exp(-6.0 * (t - s.lastBeat)), 0.0f, 1.0f);
        }
        case DRIVER_NOISE:
        {
            if (t >= s.noiseStart + s.noiseDur)
            {
                s.noiseFrom = s.noiseTo;
                s.noiseTo = randf();
                s.noiseStart = t;
                s.noiseDur = randf(0.5f, 2.0f);
            }
            float x = clampf((float)((t - s.noiseStart) / s.noiseDur), 0.0f, 1.0f);
            return s.noiseFrom + (s.noiseTo - s.noiseFrom) * (x * x * (3.0f - 2.0f * x)); // smoothstep
        }
        case DRIVER_LFO:
        default:
            return lfo01(t, s.lfoPeriod);
        }
    }

    /** @brief Every now and then, switch a parameter to a different driver and reseed its phase. */
    void maybeSwitchDriver(DriverState &s)
    {
        if (s.clock < s.nextSwitch)
            return;

        int next = std::rand() % DRIVER_COUNT;
        if (next == s.driver)
            next = (next + 1) % DRIVER_COUNT;
        s.driver = next;

        // Reseed every driver's phase at the current clock so the new one starts cleanly.
        s.lfoPeriod = randf(8.0f, 26.0f);
        s.lastBeat = s.clock;
        s.nextBeat = s.clock;
        s.noiseFrom = s.noiseTo;
        s.noiseTo = randf();
        s.noiseStart = s.clock;
        s.noiseDur = randf(0.5f, 2.0f);
        s.nextSwitch = s.clock + randf(6.0f, 18.0f);
    }

    /**
     * @brief Seed the automatic mode engine (called on the toggle's rising edge).
     */
    void beginAutomatic()
    {
        autoState.lastReal = nowSeconds();
        autoState.structClock = 0.0;

        // Start the parameters on a spread of drivers/periods for immediate variety; they each
        // switch driver on their own schedule from here.
        const int startDriver[AP_Count] = {DRIVER_PULSE, DRIVER_LFO, DRIVER_NOISE, DRIVER_LFO, DRIVER_LFO};
        const double startPeriod[AP_Count] = {12.0, 14.0, 11.0, 18.0, 23.0};

        for (int g = 0; g < AP_Count; g++)
        {
            DriverState &s = autoState.driver[g];
            s.clock = 0.0;
            s.driver = startDriver[g];
            s.lfoPeriod = startPeriod[g];
            s.lastBeat = 0.0;
            s.nextBeat = 0.0;
            s.noiseFrom = randf();
            s.noiseTo = randf();
            s.noiseStart = 0.0;
            s.noiseDur = randf(0.5f, 2.0f);
            s.nextSwitch = randf(6.0f, 18.0f);
        }

        for (int i = 0; i < 3; i++)
        {
            autoState.nextItemChange[i] = randf(0.0f, 4.0f);
            autoState.nextCategoryChange[i] = randf(2.0f, 8.0f);
        }

        autoState.nextLayerToggle = randf(4.0f, 10.0f);
    }

    /**
     * @brief Per-frame automatic mode engine. Drives the visual parameters and structural choices,
     *        writing directly into parameters[] so the result is purely visual.
     */
    void updateAutomatic()
    {
        // Advance each parameter's own speed-scaled clock (so per-parameter "Speed" stretches or
        // compresses time independently) and occasionally switch its driver, plus a steady clock
        // for the structural changes.
        const double real = nowSeconds();
        const double dt = real - autoState.lastReal;
        autoState.lastReal = real;

        for (int g = 0; g < AP_Count; g++)
        {
            autoState.driver[g].clock += dt * autoSpeed[g];
            maybeSwitchDriver(autoState.driver[g]);
        }
        autoState.structClock += dt;

        const double ts = autoState.structClock;

        // Each parameter is modulated by its currently-active driver (lfo / pulse / noise), mapped
        // into the parameter's range. Chaos pulls focus/space toward the centre, and blur, zoom and
        // background down toward zero (no blur, no zoom, black/unsaturated).
        parameters[BlurSize] = mapTowardZero(driverValue(autoState.driver[AP_Blur]), 0.0f, 0.65f, autoChaos[AP_Blur]);
        parameters[FocusDistance] = mapTowardCentre(driverValue(autoState.driver[AP_Focus]), 0.1f, 0.9f, autoChaos[AP_Focus]);
        parameters[Space] = mapTowardCentre(driverValue(autoState.driver[AP_Space]), 0.02f, 0.18f, autoChaos[AP_Space]);
        parameters[Zoom] = mapTowardZero(driverValue(autoState.driver[AP_Zoom]), 0.0f, 0.7f, autoChaos[AP_Zoom]);

        const float bg = driverValue(autoState.driver[AP_Background]);
        parameters[BackgroundHue] = (float)std::fmod(autoState.driver[AP_Background].clock * 0.03, 1.0);
        parameters[BackgroundSaturation] = mapTowardZero(bg, 0.0f, 1.0f, autoChaos[AP_Background]);
        parameters[BackgroundValue] = mapTowardZero(bg, 0.0f, 1.0f, autoChaos[AP_Background]);

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
    void drawAutoSliders(const char *id, int group)
    {
        // Indented and dimmed so they read as sub-controls of the parameter above them. Labels go
        // above the sliders and the slider's own label is hidden with "##", so nothing overflows.
        ImGui::Indent();

        ImGui::TextDisabled("Speed");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat((std::string("##speed_") + id).c_str(), &autoSpeed[group], 0.1f, 4.0f);

        ImGui::TextDisabled("Chaos");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat((std::string("##chaos_") + id).c_str(), &autoChaos[group], 0.0f, 1.0f);

        ImGui::Unindent();
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
