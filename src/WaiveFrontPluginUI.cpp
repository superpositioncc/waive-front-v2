#ifndef WAIVE_FRONT_PLUGIN_UI_CPP
#define WAIVE_FRONT_PLUGIN_UI_CPP

#include "DistrhoUI.hpp"
#include "viewer/ViewerWindow.cpp"
#include "assets/themes/CinderTheme.cpp"
#include "Application.hpp"
#include "DearImGui.hpp"
#include "video/VideoLoader.cpp"
#include "video/VideoFrameDescription.h"
#include <iostream>
#include <chrono>
#include "assets/fonts/SpaceMono_Regular.cpp"
#include <dirent.h>
#include "data/DataSources.hpp"
#include "util/Logger.cpp"
#include <vector>

using namespace Util::Logger;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class WaiveFrontPluginUI : public UI
{
public:
    float parameters[Parameters::NumParameters];
    DataSources dataSources;

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

        for (int i = 0; i < 3; i++)
        {
            videoLoaders.push_back(new VideoLoader());
            selectedCategories.push_back(nullptr);
            selectedItems.push_back(nullptr);
            layersEnabled.push_back(i == 0);
        }

        loadDataSources(std::string(home) + "/Documents/WAIVE");

        for (int i = 0; i < 3; i++)
        {
            int randomIndex = std::rand() % dataSources.categories.size();

            selectCategory(i, dataSources.categories[randomIndex]);
        }
    }

protected:
    bool pRandomizeLayer[3] = {false, false, false};

    bool isVideoFile(const char *filename)
    {
        std::string name = std::string(filename);
        std::string extension = name.substr(name.find_last_of(".") + 1);

        return extension == "mp4" || extension == "mov";
    }

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
            if (entry->d_type == DT_DIR)
            {
                std::string name = std::string(entry->d_name);

                if (name != "." && name != "..")
                {
                    std::string path = std::string(directory) + "/" + name;

                    DataSource *dataSource = new DataSource(path);
                    dataSource->load(&dataSources);

                    if (dataSource->isValid())
                        dataSources.sources.push_back(dataSource);
                    else
                        delete dataSource;
                }
            }
        }

        closedir(dir);

        dataSources.collectItems();

        for (DataSource *dataSource : dataSources.sources)
        {
            print("DATA", "Loaded data source " + dataSource->name);
        }
    }

    void selectCategory(int i, DataCategory *category)
    {
        selectedCategories[i] = category;

        print("DATA", "Selected category: " + category->name);
        print("DATA", "Number of items: " + std::to_string(category->items.size()));

        // Select random item from category
        int randomIndex = std::rand() % category->items.size();
        selectedItems[i] = category->items[randomIndex];

        print("DATA", "Selected item: " + selectedItems[i]->title);

        // Random scene
        // int randomScene = std::rand() % selectedItems[i]->nScenes;
        // std::string zeroPaddedScene = std::to_string(randomScene);
        // zeroPaddedScene.insert(0, 3 - zeroPaddedScene.length(), '0');

        std::string scenePath = selectedItems[i]->source->path + "/items/" + selectedItems[i]->filename + ".mp4";

        if (isVideoFile(scenePath.c_str()))
        {
            videoLoaders[i]->loadVideo(scenePath.c_str());
        }
    }

    // void onFileSelected(const char *filename)
    // {
    //     videoLoader.loadVideo(filename);
    // }

    void parameterChanged(uint32_t index, float value) override
    {
        parameters[index] = value;

        repaint();
    }

    void randomizeLayer(int i)
    {
        int randomIndex = std::rand() % dataSources.categories.size();
        selectCategory(i, dataSources.categories[randomIndex]);
    }

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

        if (parameters[RandomizeLayer1] != pRandomizeLayer[0] && parameters[RandomizeLayer1])
        {
            pRandomizeLayer[0] = parameters[RandomizeLayer1];

            randomizeLayer(0);
        }
        else if (!parameters[RandomizeLayer1])
        {
            pRandomizeLayer[0] = false;
        }

        if (parameters[RandomizeLayer2] != pRandomizeLayer[1] && parameters[RandomizeLayer2])
        {
            pRandomizeLayer[1] = parameters[RandomizeLayer2];

            randomizeLayer(0);
        }
        else if (!parameters[RandomizeLayer2])
        {
            pRandomizeLayer[1] = false;
        }

        if (parameters[RandomizeLayer3] != pRandomizeLayer[2] && parameters[RandomizeLayer3])
        {
            pRandomizeLayer[2] = parameters[RandomizeLayer3];

            randomizeLayer(0);
        }
        else if (!parameters[RandomizeLayer3])
        {
            pRandomizeLayer[2] = false;
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

                if (vfd.data != nullptr)
                    viewerWindow->getViewerWidget()->setFrame(i, vfd.data, vfd.width, vfd.height, videoLoader->getColors());
            }
        }

        // updateFileBrowser();

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

        ImGui::End();

        // if (ImGui::Button("Select Video"))
        // {
        //     openFileBrowser();
        // }

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
                ImGui::Text("Category");
                if (ImGui::BeginCombo(("Category " + std::to_string(i + 1)).c_str(), selectedCategories[i] != nullptr ? selectedCategories[i]->name.c_str() : "None"))
                {
                    for (DataCategory *category : dataSources.categories)
                    {
                        if (ImGui::Selectable(category->name.c_str()))
                        {
                            selectCategory(i, category);
                        }
                    }

                    ImGui::EndCombo();
                }

                if (ImGui::Button(("Select Random Category " + std::to_string(i + 1)).c_str()))
                {
                    randomizeLayer(i);
                }

                ImGui::TextWrapped(selectedItems[i] != nullptr ? selectedItems[i]->title.c_str() : "None");

                std::vector<float> colors = videoLoaders[i]->getColors();

                // make a grid of color buttons next to each other
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
    ViewerWindow *viewerWindow = nullptr;
    bool initialized = false;

    // FileBrowserHandle fileBrowserHandle = nullptr;
    ImFont *regular;

    std::vector<VideoLoader *> videoLoaders;
    std::vector<DataCategory *> selectedCategories;
    std::vector<DataItem *> selectedItems;

    std::vector<bool> layersEnabled;

    int64_t getCurrentTime()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void openViewerWindow()
    {
        Application &app = getApp();

        if (viewerWindow == nullptr)
        {
            viewerWindow = new ViewerWindow(app, parameters, &layersEnabled);
        }
    }

    // void openFileBrowser()
    // {
    //     FileBrowserOptions options;
    //     options.startDir = "/Users/Bram/Documents/WAIVE";

    //     if (fileBrowserHandle != nullptr)
    //     {
    //         fileBrowserClose(fileBrowserHandle);
    //     }

    //     fileBrowserHandle = fileBrowserCreate(false, getWindow().getNativeWindowHandle(), 1, options);
    // }

    // void updateFileBrowser()
    // {
    //     if (fileBrowserHandle != nullptr)
    //     {
    //         if (fileBrowserIdle(fileBrowserHandle))
    //         {
    //             const char *filename = fileBrowserGetPath(fileBrowserHandle);

    //             if (filename != nullptr)
    //             {
    //                 onFileSelected(filename);
    //             }

    //             fileBrowserClose(fileBrowserHandle);
    //             fileBrowserHandle = nullptr;

    //             getWindow().focus();
    //         }
    //     }
    // }
};

UI *createUI()
{
    return new WaiveFrontPluginUI();
}

END_NAMESPACE_DISTRHO

#endif
