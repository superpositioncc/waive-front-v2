#ifndef WAIVE_FRONT_PLUGIN_UI_CPP
#define WAIVE_FRONT_PLUGIN_UI_CPP

#include "DistrhoUI.hpp"
#include "ViewerWindow.cpp"
#include "Application.hpp"
#include <iostream>

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
    }

protected:
    void parameterChanged(uint32_t index, float value) override
    {
        parameters[index] = value;

        repaint();
    }

    void onImGuiDisplay() override
    {
        const float width = getWidth();
        const float height = getHeight();
        const float margin = 20.0f * getScaleFactor();

        ImGui::SetNextWindowPos(ImVec2(margin, margin));
        ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

        ImGui::Begin("WAIVE-FRONT V2", nullptr, ImGuiWindowFlags_NoResize);

        if (ImGui::SliderFloat("Value", &parameters[Value], 0.0f, 1.0f))
            setParameterValue(0, parameters[Value]);

        ImGui::End();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaiveFrontPluginUI)

private:
    ViewerWindow *viewerWindow = nullptr;

    void openViewerWindow()
    {
        std::cout << "Opening viewer window" << std::endl;
        Application &app = getApp();

        if (viewerWindow == nullptr)
        {
            viewerWindow = new ViewerWindow(app, parameters);
        }
    }
};

UI *createUI()
{
    return new WaiveFrontPluginUI();
}

END_NAMESPACE_DISTRHO

#endif
