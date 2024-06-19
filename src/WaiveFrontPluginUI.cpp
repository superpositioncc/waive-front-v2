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

#include "assets/fonts/SpaceMono_Regular.cpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class WaiveFrontPluginUI : public UI
{
public:
    float parameters[Parameters::NumParameters];
    // ResizeHandle fResizeHandle;

    WaiveFrontPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);

        openViewerWindow();

        ImGuiIO &io = ImGui::GetIO();
        regular = io.Fonts->AddFontFromMemoryCompressedTTF(SpaceMono_Regular_compressed_data, SpaceMono_Regular_compressed_size, 48.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    }

protected:
    void onFileSelected(const char *filename)
    {
        VideoFrameDescription vfd = videoLoader.loadVideo(filename);

        if (vfd.data != nullptr)
            viewerWindow->getViewerWidget()->setFrame(vfd.data, vfd.width, vfd.height);
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

        updateFileBrowser();

        const float width = getWidth();
        const float height = getHeight();
        const float margin = 20.0f * getScaleFactor();

        ImGui::SetNextWindowSizeConstraints(ImVec2(512, 0), ImVec2(width - 2 * margin, height - 2 * margin));

        ImGui::PushFont(regular);

        ImGui::Begin("WAIVE-FRONT V2", nullptr, ImGuiWindowFlags_NoResize);

        if (ImGui::SliderFloat("Threshold", &parameters[Threshold], 0.0f, 1.0f))
            setParameterValue(0, parameters[Threshold]);

        if (ImGui::SliderFloat("Width", &parameters[Width], 0.0f, 1.0f))
            setParameterValue(1, parameters[Width]);

        if (ImGui::Button("Select Video"))
        {
            openFileBrowser();
        }

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
