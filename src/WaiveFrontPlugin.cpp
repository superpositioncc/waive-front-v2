#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

class WaiveFrontPlugin : public Plugin
{
public:
    float parameters[Parameters::NumParameters];

    WaiveFrontPlugin()
        : Plugin(Parameters::NumParameters, 0, 0)
    {
        std::memset(parameters, 0, sizeof(float) * Parameters::NumParameters);
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
        case Value:
            parameter.name = "Value";
            parameter.symbol = "value";
            break;
        default:
            break;
        }
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
