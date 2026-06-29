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

// Standalone, audio-free build of WAIVE-FRONT. It hosts the exact same control panel and viewer
// as the plugin (via the shared WaiveFrontController), but without any DPF plugin / audio wrapper.

#include "Application.hpp"
#include "WaiveFrontController.hpp"

USE_NAMESPACE_DISTRHO;

// -----------------------------------------------------------------------------------------------------------

/**
 * @brief The standalone control window. Mirrors WaiveFrontPluginUI, but as a plain DGL standalone
 *        ImGui window instead of a plugin UI.
 */
class WaiveFrontStandalone : public ImGuiStandaloneWindow
{
public:
    WaiveFrontStandalone(Application &app)
        : ImGuiStandaloneWindow(app)
    {
        setTitle("WAIVE-FRONT");
        setResizable(true);
        setSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);

        // The ImGui context is created and made current by the ImGuiStandaloneWindow base class, so
        // fonts must be added here (in the constructor), not during onImGuiDisplay().
        controller.initFonts();

        controller.setDefaults();
        controller.init();

        viewerWindow = new ViewerWindow(app, controller.parameters, controller.getLayersEnabled());

        // In standalone mode, closing either window quits the whole application.
        viewerWindow->setQuitOnClose(true);
    }

protected:
    /**
     * @brief Keep the OpenGL drawable in sync with the window size on resize.
     */
    void onReshape(uint width, uint height) override
    {
        ImGuiStandaloneWindow::onReshape(width, height);
        waiveUpdateGLDrawable(getNativeWindowHandle());
    }

    /**
     * @brief Closing the control window quits the application.
     *
     * @return true to allow the window to close
     */
    bool onClose() override
    {
        getApp().quit();
        return true;
    }

    /**
     * @brief Display the ImGui UI
     */
    void onImGuiDisplay() override
    {
        if (!initialized)
        {
            initialized = true;

            cinderTheme(ImGui::GetStyle());
        }

        controller.update(viewerWindow->getViewerWidget());
        controller.drawControls(getWidth(), getHeight());

        if (viewerWindow != nullptr)
        {
            viewerWindow->repaint();
        }
    }

private:
    WaiveFrontController controller;      /**< The shared control-panel logic */
    ViewerWindow *viewerWindow = nullptr; /**< The viewer window */
    bool initialized = false;             /**< Whether the UI has been initialized */

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaiveFrontStandalone)
};

// -----------------------------------------------------------------------------------------------------------

int main()
{
    USE_NAMESPACE_DGL;

    Application app(true);

    WaiveFrontStandalone window(app);
    window.show();

    app.exec();

    return 0;
}
