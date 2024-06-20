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
        regular = io.Fonts->AddFontFromMemoryCompressedTTF(SpaceMono_Regular_compressed_data, SpaceMono_Regular_compressed_size, 48.0f, nullptr, io.Fonts->GetGlyphRangesDefault());

        // videoLoader.loadVideo("/Users/Bram/Documents/WAIVE/video.mp4");

        loadDataSources("/Users/Bram/Documents/WAIVE");
    }

protected:
    bool isVideoFile(const char *filename)
    {
        std::string name = std::string(filename);
        std::string extension = name.substr(name.find_last_of(".") + 1);

        return extension == "mp4" || extension == "mov";
    }

    void loadDataSources(const char *directory)
    {
        DIR *dir = opendir(directory);
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

        int randomIndex = std::rand() % dataSources.tags.size();
        selectTag(dataSources.tags[randomIndex]);
    }

    void selectTag(DataTag *tag)
    {
        selectedTag = tag;

        print("DATA", "Selected tag: " + tag->name);

        // Select random item from tag
        int randomIndex = std::rand() % tag->items.size();
        selectedItem = tag->items[randomIndex];

        print("DATA", "Selected item: " + selectedItem->title);

        // Random scene
        int randomScene = std::rand() % selectedItem->nScenes;
        std::string zeroPaddedScene = std::to_string(randomScene);
        zeroPaddedScene.insert(0, 3 - zeroPaddedScene.length(), '0');

        std::string scenePath = selectedItem->source->path + "/material/" + selectedItem->filename + "_scene" + zeroPaddedScene + ".mp4";

        if (isVideoFile(scenePath.c_str()))
        {
            videoLoader.loadVideo(scenePath.c_str());
        }
    }

    void onFileSelected(const char *filename)
    {
        videoLoader.loadVideo(filename);
    }

    void parameterChanged(uint32_t index, float value) override
    {
        parameters[index] = value;

        repaint();
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

        int64_t currentTime = getCurrentTime();

        if (videoLoader.getStatus() == 1 && videoLoader.shouldGetNextFrame(currentTime))
        {
            VideoFrameDescription vfd = videoLoader.getFrame();

            if (vfd.data != nullptr)
                viewerWindow->getViewerWidget()->setFrame(vfd.data, vfd.width, vfd.height);
        }

        updateFileBrowser();

        const float width = getWidth();
        const float height = getHeight();
        const float margin = 20.0f * getScaleFactor();

        ImGui::SetNextWindowSizeConstraints(ImVec2(800, 0), ImVec2(width - 2 * margin, height - 2 * margin));

        ImGui::PushFont(regular);

        ImGui::Begin("WAIVE-FRONT V2", nullptr, ImGuiWindowFlags_NoResize);

        if (ImGui::SliderFloat("Threshold", &parameters[Threshold], 0.0f, 1.0f))
            setParameterValue(0, parameters[Threshold]);

        if (ImGui::SliderFloat("Width", &parameters[Width], 0.0f, 1.0f))
            setParameterValue(1, parameters[Width]);

        // if (ImGui::Button("Select Video"))
        // {
        //     openFileBrowser();
        // }

        if (ImGui::BeginCombo("Tag", selectedTag != nullptr ? selectedTag->name.c_str() : "None"))
        {
            for (DataTag *tag : dataSources.tags)
            {
                if (ImGui::Selectable(tag->name.c_str()))
                {
                    selectTag(tag);
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button("Select Random Tag"))
        {
            int randomIndex = std::rand() % dataSources.tags.size();
            selectTag(dataSources.tags[randomIndex]);
        }

        ImGui::Text(selectedItem != nullptr ? selectedItem->title.c_str() : "None");

        ImGui::End();

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

    FileBrowserHandle fileBrowserHandle = nullptr;
    VideoLoader videoLoader;

    ImFont *regular;

    DataTag *selectedTag = nullptr;
    DataItem *selectedItem = nullptr;

    int64_t getCurrentTime()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void openViewerWindow()
    {
        Application &app = getApp();

        if (viewerWindow == nullptr)
        {
            viewerWindow = new ViewerWindow(app, parameters);
        }
    }

    void openFileBrowser()
    {
        FileBrowserOptions options;
        options.startDir = "/Users/Bram/Documents/WAIVE";

        if (fileBrowserHandle != nullptr)
        {
            fileBrowserClose(fileBrowserHandle);
        }

        fileBrowserHandle = fileBrowserCreate(false, getWindow().getNativeWindowHandle(), 1, options);
    }

    void updateFileBrowser()
    {
        if (fileBrowserHandle != nullptr)
        {
            if (fileBrowserIdle(fileBrowserHandle))
            {
                const char *filename = fileBrowserGetPath(fileBrowserHandle);

                if (filename != nullptr)
                {
                    onFileSelected(filename);
                }

                fileBrowserClose(fileBrowserHandle);
                fileBrowserHandle = nullptr;

                getWindow().focus();
            }
        }
    }
};

UI *createUI()
{
    return new WaiveFrontPluginUI();
}

END_NAMESPACE_DISTRHO

#endif
