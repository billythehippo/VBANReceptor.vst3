/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "vban_functions.h"

//==============================================================================
/**
*/
class VBANReceptorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    VBANReceptorAudioProcessorEditor (VBANReceptorAudioProcessor&);
    ~VBANReceptorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VBANReceptorAudioProcessor& audioProcessor;

    juce::Label labelIP;
    juce::Label labelPort;
    juce::Label labelSN;
    juce::Label labelRed;
    juce::Label labelFmt;
    juce::TextEditor textEditorIP;
    juce::TextEditor textEditorPort;
    juce::TextEditor textEditorSN;
    juce::ComboBox comboBoxNQ;
    juce::ComboBox comboBoxFmt;
    juce::ComboBox comboBoxReceptors;
    juce::TextButton textButtonScan;
    juce::TextButton textButtonGo;

    juce::Slider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainSliderAttachment;

    bool scanEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VBANReceptorAudioProcessorEditor)
};
