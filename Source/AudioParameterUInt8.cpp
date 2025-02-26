#include "AudioParameterUInt8.h"

AudioParameterUInt8::AudioParameterUInt8(const juce::String& parameterID,
                                         const juce::String& name,
                                         uint8_t defaultValue,
                                         uint8_t minimum,
                                         uint8_t maximum)
: juce::RangedAudioParameter(parameterID, name, minimum, maximum, defaultValue)
{
    setValue(defaultValue);
}

uint8_t AudioParameterUInt8::getCurrentValue() const
{
    return static_cast<uint8_t>(value.getValue());
}

void AudioParameterUInt8::setValue(uint8_t newValue)
{
    value.setValue(static_cast<double>(newValue));
}

void AudioParameterUInt8::setValueNotifyingHost(uint8_t newValue)
{
    beginChangeGesture();
    setValue(newValue);
    endChangeGesture();
    sendParameterChangeMessageToHost(getParameterID(), newValue);
}

juce::var AudioParameterUInt8::getValueObject() const
{
    return getCurrentValue();
}

double AudioParameterUInt8::getDefaultValue() const
{
    return static_cast<double>(defaultValue);
}

bool AudioParameterUInt8::isDiscrete() const
{
    return true;
}

bool AudioParameterUInt8::isBoolean() const
{
    return false;
}

bool AudioParameterUInt8::isMetaParameter() const
{
    return false;
}

juce::String AudioParameterUInt8::getLabel() const
{
    return {};
}

juce::String AudioParameterUInt8::getText(double value, int length) const
{
    return juce::String(static_cast<uint8_t>(value));
}
