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

#ifndef WAIVE_FRONT_PLUGIN_UI_CPP
#define WAIVE_FRONT_PLUGIN_UI_CPP

#ifndef __APPLE__
#include <Windows.h>
#include <GL/glew.h>
#endif

#include "DistrhoUI.hpp"
#include "viewer/ViewerWindow.cpp"
#include "assets/themes/CinderTheme.cpp"
#include "Application.hpp"
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
#include "assets/fonts/SpaceMono_Regular.cpp"
#include <dirent.h>
#include "data/DataSources.hpp"
#include "util/Logger.cpp"
#include <vector>
#include "osc/OSCServer.cpp"

using namespace Util::Logger;

#ifdef __APPLE__
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/**
 * @brief The WaiveFrontPluginUI class is the main DPF UI and synchronizes the UI with the plugin
 *
 */
class WaiveFrontPluginUI : public UI
{
public:
    float parameters[Parameters::NumParameters]; /**< The parameters of the plugin */
    DataSources dataSources;                     /**< The data sources */
    OSCServer *oscServer;                        /**< The OSC server */

    /**
     * @brief Construct a new WAIVE-FRONT Plugin UI object
     *
     */
    WaiveFrontPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true)
    {
        std::srand(std::time(0));

        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);

        openViewerWindow();

        ImGuiIO &io = ImGui::GetIO();
        regular = io.Fonts->AddFontFromMemoryCompressedTTF(SpaceMono_Regular_compressed_data, SpaceMono_Regular_compressed_size, 32.0f, nullptr, io.Fonts->GetGlyphRangesDefault());

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

protected:
    bool pRandomizeCategory[3] = {false, false, false}; /**< Whether to randomize the category on the next frame */
    bool pRandomizeItem[3] = {false, false, false};     /**< Whether to randomize the item on the next frame */
    bool allowOSC = true;                               /**< Whether to allow OSC control */

    /**
     * @brief Check if a file is a video file
     *
     * @param filename The filename to check
     * @return true
     * @return false
     */
    bool isVideoFile(const char *filename)
    {
        std::string name = std::string(filename);
        std::string extension = name.substr(name.find_last_of(".") + 1);

        return extension == "mp4" || extension == "mov";
    }

    /**
     * @brief Load the data sources from a directory
     *
     * @param directory Path to the directory
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
     *
     * @param i The index of the layer to change the category for
     * @param category The category to select
     */
    void selectCategory(int i, DataCategory *category)
    {
        selectedCategories[i] = category;

        print("DATA", "Selected category: " + category->name);
        randomizeItem(i);
    }

    /**
     * @brief Select an item
     *
     * @param i The index of the layer to change the item for
     * @param item The item to select
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
     * @brief Handle a parameter change
     *
     * @param index The index of the parameter
     * @param value The new value of the parameter
     */
    void parameterChanged(uint32_t index, float value) override
    {
        parameters[index] = value;

        repaint();
    }

    /**
     * @brief Randomize a category
     *
     * @param i The index of the layer to randomize the category for
     */
    void randomizeCategory(int i)
    {
        int randomIndex = std::rand() % dataSources.categories.size();
        selectCategory(i, dataSources.categories[randomIndex]);
    }

    /**
     * @brief Randomize an item
     *
     * @param i The index of the layer to randomize the item for
     */
    void randomizeItem(int i)
    {
        int randomIndex = std::rand() % selectedCategories[i]->items.size();
        selectItem(i, selectedCategories[i]->items[randomIndex]);
    }

    /**
     * @brief Display the ImGui UI
     *
     */
    void onImGuiDisplay() override
    {
        if (!initialized)
        {
            initialized = true;

            Window &window = getWindow();
            window.setOffsetY(window.getOffsetY() + 720 / 2 + 100);

            cinderTheme(ImGui::GetStyle());
        }

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
                    viewerWindow->getViewerWidget()->setFrame(i, vfd.data, vfd.width, vfd.height, videoLoader->getColors());
                }
            }
        }

        const float width = getWidth();
        const float height = getHeight();

        ImGui::SetNextWindowSizeConstraints(ImVec2(width / 4, 0), ImVec2(width / 4, height));
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGui::PushFont(regular);

        ImGui::Begin("WAIVE-FRONT V2", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Blur Size");
        ImGui::SetNextItemWidth(width / 4);
        if (ImGui::SliderFloat("Blur Size", &parameters[BlurSize], 0.0f, 1.0f))
            setParameterValue(BlurSize, parameters[BlurSize]);

        ImGui::Text("Focus Distance");
        ImGui::SetNextItemWidth(width / 4);
        if (ImGui::SliderFloat("Focus Distance", &parameters[FocusDistance], 0.0f, 1.0f))
            setParameterValue(FocusDistance, parameters[FocusDistance]);

        ImGui::Text("Space");
        ImGui::SetNextItemWidth(width / 4);
        if (ImGui::SliderFloat("Space", &parameters[Space], 0.0f, 0.2f))
            setParameterValue(Space, parameters[Space]);

        ImGui::Text("Zoom");
        ImGui::SetNextItemWidth(width / 4);
        if (ImGui::SliderFloat("Zoom", &parameters[Zoom], 0.0f, 1.0f))
            setParameterValue(Zoom, parameters[Zoom]);

        ImGui::Text("Background Color");
        ImGui::SetNextItemWidth(width / 4);
        float hsv[3] = {parameters[BackgroundHue], parameters[BackgroundSaturation], parameters[BackgroundValue]};
        if (ImGui::ColorPicker3("Background Color", hsv, ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_InputHSV))
        {
            setParameterValue(BackgroundHue, hsv[0]);
            setParameterValue(BackgroundSaturation, hsv[1]);
            setParameterValue(BackgroundValue, hsv[2]);

            parameters[BackgroundHue] = hsv[0];
            parameters[BackgroundSaturation] = hsv[1];
            parameters[BackgroundValue] = hsv[2];
        }

        ImGui::Toggle((std::string("OSC is ") + std::string(allowOSC ? "enabled" : "disabled")).c_str(), &allowOSC);
        ImGui::End();

        for (int i = 0; i < 3; i++)
        {
            ImGui::SetNextWindowSizeConstraints(ImVec2(width / 4, 0), ImVec2(width / 4, height));
            ImGui::SetNextWindowPos(ImVec2((i + 1) * width / 4, 0));
            ImGui::Begin(("Layer " + std::to_string(i + 1)).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            std::string buttonLabel = layersEnabled[i] ? "Disable Layer " + std::to_string(i + 1) : "Enable Layer " + std::to_string(i + 1);

            if (ImGui::Button(buttonLabel.c_str()))
            {
                layersEnabled[i] = !layersEnabled[i];

                if (i == 0)
                {
                    parameters[EnableLayer1] = layersEnabled[i];
                    setParameterValue(EnableLayer1, layersEnabled[i]);
                }
                else if (i == 1)
                {
                    parameters[EnableLayer2] = layersEnabled[i];
                    setParameterValue(EnableLayer2, layersEnabled[i]);
                }
                else if (i == 2)
                {
                    parameters[EnableLayer3] = layersEnabled[i];
                    setParameterValue(EnableLayer3, layersEnabled[i]);
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
                            setParameterValue(OSCNote1, layerNotes[i]);
                        }
                        else if (i == 1)
                        {
                            parameters[OSCNote2] = layerNotes[i];
                            setParameterValue(OSCNote2, layerNotes[i]);
                        }
                        else if (i == 2)
                        {
                            parameters[OSCNote3] = layerNotes[i];
                            setParameterValue(OSCNote3, layerNotes[i]);
                        }
                    }

                    bool retrigger = layerRetrigger[i];
                    if (ImGui::Toggle((std::string("OSC Retrigger ") + std::to_string(i + 1)).c_str(), &retrigger))
                    {
                        layerRetrigger[i] = retrigger;

                        if (i == 0)
                        {
                            parameters[OSCRetrigger1] = layerRetrigger[i];
                            setParameterValue(OSCRetrigger1, layerRetrigger[i]);
                        }
                        else if (i == 1)
                        {
                            parameters[OSCRetrigger2] = layerRetrigger[i];
                            setParameterValue(OSCRetrigger2, layerRetrigger[i]);
                        }
                        else if (i == 2)
                        {
                            parameters[OSCRetrigger3] = layerRetrigger[i];
                            setParameterValue(OSCRetrigger3, layerRetrigger[i]);
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

            ImGui::End();
        }

        ImGui::PopFont();

        if (viewerWindow != nullptr)
        {
            viewerWindow->repaint();
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaiveFrontPluginUI)

private:
    ViewerWindow *viewerWindow = nullptr; /**< The viewer window */
    bool initialized = false;             /**< Whether the UI has been initialized */

    ImFont *regular; /**< The regular font */

    std::vector<VideoLoader *> videoLoaders;        /**< The video loaders */
    std::vector<DataCategory *> selectedCategories; /**< The selected categories */
    std::vector<DataItem *> selectedItems;          /**< The selected items */

    std::vector<bool> layersEnabled;       /**< Whether each layer is enabled */
    std::vector<bool> layerRetrigger;      /**< Whether each layer is retriggered on each note */
    std::vector<int> layerNotes;           /**< Which note each layer should respond to */
    std::vector<std::string> lastMessages; /**< The last messages received */

    /**
     * @brief Get the current time
     *
     * @return int64_t The current time
     */
    int64_t getCurrentTime()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    /**
     * @brief Open the viewer window
     *
     */
    void openViewerWindow()
    {
        Application &app = getApp();

        if (viewerWindow == nullptr)
        {
            viewerWindow = new ViewerWindow(app, parameters, &layersEnabled);
        }
    }
};

UI *createUI()
{
    return new WaiveFrontPluginUI();
}

END_NAMESPACE_DISTRHO

#endif
