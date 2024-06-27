#include "DistrhoPlugin.hpp"
#include "dpf/distrho/extra/String.hpp"

START_NAMESPACE_DISTRHO

class WaiveFrontPlugin : public Plugin
{
public:
    float parameters[Parameters::NumParameters];

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
    }

protected:
    const char *getLabel() const override { return DISTRHO_PLUGIN_NAME; }
    const char *getDescription() const override { return DISTRHO_PLUGIN_NAME; }
    const char *getMaker() const override { return DISTRHO_PLUGIN_BRAND; }
    const char *getHomePage() const override { return DISTRHO_PLUGIN_URI; }
    const char *getLicense() const override { return "ISC"; }

    uint32_t getVersion() const override { return d_version(1, 0, 0); }

    void initAudioPort(bool input, uint32_t index, AudioPort &port) override
    {
        port.groupId = kPortGroupStereo;

        Plugin::initAudioPort(input, index, port);
    }

    void initParameter(uint32_t index, Parameter &parameter) override
    {
        parameter.hints = kParameterIsAutomatable;

        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.ranges.def = 0.0f;

        switch (index)
        {
        // case Threshold:
        //     parameter.name = "Threshold";
        //     parameter.symbol = "threshold";
        //     break;
        // case Width:
        //     parameter.name = "Width";
        //     parameter.symbol = "width";
        //     break;
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
        default:
            break;
        }

        parameter.symbol = String(parameter.name).replace(' ', '_').toLower();
    }

    float getParameterValue(uint32_t index) const override
    {
        return parameters[index];
    }

    void setParameterValue(uint32_t index, float value) override
    {
        parameters[index] = value;
    }

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
