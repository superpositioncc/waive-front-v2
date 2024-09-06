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

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

/**
 * @brief The WaiveFrontPlugin class is the main DPF plugin and synchronizes the plugin with the DAW
 *
 */
class WaiveFrontPlugin : public Plugin
{
public:
    float parameters[Parameters::NumParameters]; /**< The parameters of the plugin */

    /**
     * @brief Construct a new Waive Front Plugin object
     *
     */
    WaiveFrontPlugin()
        : Plugin(Parameters::NumParameters, 0, 0)
    {
        std::memset(parameters, 0, sizeof(float) * Parameters::NumParameters);

        // parameters[Threshold] = 0.5f;
        // parameters[Width] = 0.5f;
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

protected:
    const char *getLabel() const override { return DISTRHO_PLUGIN_NAME; }       /**< Get the label of the plugin */
    const char *getDescription() const override { return DISTRHO_PLUGIN_NAME; } /**< Get the description of the plugin */
    const char *getMaker() const override { return DISTRHO_PLUGIN_BRAND; }      /**< Get the maker of the plugin */
    const char *getHomePage() const override { return DISTRHO_PLUGIN_URI; }     /**< Get the homepage of the plugin */
    const char *getLicense() const override { return "GPLv3"; }                 /**< Get the license of the plugin */

    uint32_t getVersion() const override { return d_version(1, 0, 0); } /**< Get the version of the plugin */

    /**
     * @brief Initialize the audio port
     *
     * @param input Whether the port is an input or output
     * @param index The index of the port
     * @param port The port to initialize
     */
    void initAudioPort(bool input, uint32_t index, AudioPort &port) override
    {
        port.groupId = kPortGroupStereo;

        Plugin::initAudioPort(input, index, port);
    }

    /**
     * @brief Initialize the parameter
     *
     * @param index The index of the parameter
     * @param parameter The parameter to initialize
     */
    void initParameter(uint32_t index, Parameter &parameter) override
    {
        parameter.hints = kParameterIsAutomatable;

        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.ranges.def = 0.0f;

        switch (index)
        {
        case FocusDistance:
            parameter.name = "Focus Distance";
            break;
        case BlurSize:
            parameter.name = "Blur Size";
            break;
        case Space:
            parameter.name = "Space";
            parameter.ranges.max = 0.2f;
            break;
        case Zoom:
            parameter.name = "Zoom";
            break;
        case BackgroundHue:
            parameter.name = "Background Hue";
            break;
        case BackgroundSaturation:
            parameter.name = "Background Saturation";
            break;
        case BackgroundValue:
            parameter.name = "Background Value";
            break;
        case EnableLayer1:
            parameter.name = "Enable Layer 1";
            parameter.hints |= kParameterIsBoolean;
            break;
        case EnableLayer2:
            parameter.name = "Enable Layer 2";
            parameter.hints |= kParameterIsBoolean;
            break;
        case EnableLayer3:
            parameter.name = "Enable Layer 3";
            parameter.hints |= kParameterIsBoolean;
            break;
        case RandomizeCategory1:
            parameter.name = "Randomize Category 1";
            parameter.hints |= kParameterIsBoolean;
            break;
        case RandomizeCategory2:
            parameter.name = "Randomize Category 2";
            parameter.hints |= kParameterIsBoolean;
            break;
        case RandomizeCategory3:
            parameter.name = "Randomize Category 3";
            parameter.hints |= kParameterIsBoolean;
            break;
        case RandomizeItem1:
            parameter.name = "Randomize Item 1";
            parameter.hints |= kParameterIsBoolean;
            break;
        case RandomizeItem2:
            parameter.name = "Randomize Item 2";
            parameter.hints |= kParameterIsBoolean;
            break;
        case RandomizeItem3:
            parameter.name = "Randomize Item 3";
            parameter.hints |= kParameterIsBoolean;
            break;
        case OSCNote1:
            parameter.name = "OSC Note 1";
            parameter.hints |= kParameterIsInteger;
            parameter.ranges.min = 0;
            parameter.ranges.max = 127;
            break;
        case OSCNote2:
            parameter.name = "OSC Note 2";
            parameter.hints |= kParameterIsInteger;
            parameter.ranges.min = 0;
            parameter.ranges.max = 127;
            break;
        case OSCNote3:
            parameter.name = "OSC Note 3";
            parameter.hints |= kParameterIsInteger;
            parameter.ranges.min = 0;
            parameter.ranges.max = 127;
            break;
        case OSCRetrigger1:
            parameter.name = "OSC Retrigger 1";
            parameter.hints |= kParameterIsBoolean;
            break;
        case OSCRetrigger2:
            parameter.name = "OSC Retrigger 2";
            parameter.hints |= kParameterIsBoolean;
            break;
        case OSCRetrigger3:
            parameter.name = "OSC Retrigger 3";
            parameter.hints |= kParameterIsBoolean;
            break;
        default:
            break;
        }

        parameter.symbol = parameter.name + "";
        parameter.symbol.replace(' ', '_').toLower();
    }

    /**
     * @brief Get the value of a parameter
     *
     * @param index The index of the parameter
     * @return float The value of the parameter
     */
    float getParameterValue(uint32_t index) const override
    {
        return parameters[index];
    }

    /**
     * @brief Set the value of a parameter
     *
     * @param index The index of the parameter
     * @param value The value of the parameter
     */
    void setParameterValue(uint32_t index, float value) override
    {
        parameters[index] = value;
    }

    /**
     * @brief Run the plugin
     *
     * @param inputs The input audio
     * @param outputs The output audio
     * @param frames The number of frames
     */
    void run(const float **inputs, float **outputs, uint32_t frames) override
    {
        // Don't alter the audio, just pass it through
        if (outputs[0] != inputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float) * frames);

        if (outputs[1] != inputs[1])
            std::memcpy(outputs[1], inputs[1], sizeof(float) * frames);
    }

private:
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaiveFrontPlugin)
};

Plugin *createPlugin()
{
    return new WaiveFrontPlugin();
}

END_NAMESPACE_DISTRHO
