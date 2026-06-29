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

#include "DistrhoUI.hpp"
#include "Application.hpp"
#include "WaiveFrontController.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/**
 * @brief The WaiveFrontPluginUI class is the main DPF UI. It is a thin shell that owns the viewer
 *        window and delegates all control-panel logic to the shared WaiveFrontController.
 *
 */
class WaiveFrontPluginUI : public UI
{
public:
    /**
     * @brief Construct a new WAIVE-FRONT Plugin UI object
     *
     */
    WaiveFrontPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true)
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);
        setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);

        // The ImGui context is created and made current by the UI base class, so fonts must be
        // added here (in the constructor), not during onImGuiDisplay().
        controller.initFonts();

        controller.init();

        // Mirror parameter edits back to the DAW.
        controller.onParameterEdited = [this](uint32_t index, float value)
        {
            setParameterValue(index, value);
        };

        openViewerWindow();
    }

protected:
    /**
     * @brief Handle a parameter change
     *
     * @param index The index of the parameter
     * @param value The new value of the parameter
     */
    void parameterChanged(uint32_t index, float value) override
    {
        controller.parameters[index] = value;

        repaint();
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

        controller.update(viewerWindow->getViewerWidget());
        controller.drawControls(getWidth(), getHeight());

        if (viewerWindow != nullptr)
        {
            viewerWindow->repaint();
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaiveFrontPluginUI)

private:
    WaiveFrontController controller;      /**< The shared control-panel logic */
    ViewerWindow *viewerWindow = nullptr; /**< The viewer window */
    bool initialized = false;             /**< Whether the UI has been initialized */

    /**
     * @brief Open the viewer window
     *
     */
    void openViewerWindow()
    {
        Application &app = getApp();

        if (viewerWindow == nullptr)
        {
            viewerWindow = new ViewerWindow(app, controller.parameters, controller.getLayersEnabled());
        }
    }
};

UI *createUI()
{
    return new WaiveFrontPluginUI();
}

END_NAMESPACE_DISTRHO

#endif
