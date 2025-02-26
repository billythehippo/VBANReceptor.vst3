#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class AudioParameterUInt8 : public juce::RangedAudioParameter
{
public:
    AudioParameterUInt8(const juce::String& parameterID,
                        const juce::String& name,
                        uint8_t defaultValue,
                        uint8_t minimum,
                        uint8_t maximum);

    uint8_t getCurrentValue() const override;
    void setValue(uint8_t newValue) override;
    void setValueNotifyingHost(uint8_t newValue) override;
    juce::var getValueObject() const override;
    double getDefaultValue() const override;
    bool isDiscrete() override;
    bool isBoolean() override;
    bool isMetaParameter() override;
    juce::String getLabel() const override;
    juce::String getText(double value, int length) const override;

protected:
    uint8_t defaultValue;
};
