/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VBANReceptorAudioProcessorEditor::VBANReceptorAudioProcessorEditor (VBANReceptorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    audioProcessor.editor = this;

    labelIP.setText("IP address", juce::dontSendNotification);
    addAndMakeVisible (labelIP);
    labelIP.setBounds (16, 16, 70, 24);

    labelPort.setText("Port", juce::dontSendNotification);
    addAndMakeVisible (labelPort);
    labelPort.setBounds (16, 48, 70, 24);

    labelSN.setText("Streamname", juce::dontSendNotification);
    addAndMakeVisible (labelSN);
    labelSN.setBounds (16, 80, 70, 24);

    labelRed.setText("Redundancy", juce::dontSendNotification);
    addAndMakeVisible (labelRed);
    labelRed.setBounds (16, 112, 70, 24);

    updateCNlabel(NUMBER_OF_CHANNELS);
    labelChannels.setJustificationType (36);
    labelChannels.setBounds (120, 184, 80, 24);
    addAndMakeVisible (labelChannels);

    textEditorIP.setBounds (120, 16, 100, 24);
    textEditorIP.setInputRestrictions (15, "0123456789.");
    updateIP();
    textEditorIP.onTextChange = [this]()
    {
        juce::IPAddress targetIP (textEditorIP.getText());
        if (targetIP != juce::IPAddress::any() || textEditorIP.getText() == "0.0.0.0")
            audioProcessor.updateIP(targetIP);
    };
    addAndMakeVisible (textEditorIP);

    textEditorPort.setBounds (120, 48, 100, 24);
    textEditorPort.setInputRestrictions (5, "0123456789");
    juce::String currentPort = "";
    for (int i = 0; i < 5; i++)
    {
        int digit = (int)*audioProcessor.apvts.getRawParameterValue("port" + juce::String(i));
        currentPort += juce::String(digit);
    }
    textEditorPort.setText(juce::String(currentPort.getIntValue()), juce::dontSendNotification);
    textEditorPort.onTextChange = [this]()
    {
        juce::String text = textEditorPort.getText();
        juce::String paddedText = text.paddedLeft('0', 5);
        for (int i = 0; i < 5; i++)
        {
            auto paramID = "port" + juce::String(i);
            if (auto* param = audioProcessor.apvts.getParameter(paramID))
            {
                float digitValue = (float)(paddedText[i] - '0');
                float normalized = param->getNormalisableRange().convertTo0to1(digitValue);
                if (std::abs(param->getValue() - normalized) > 0.001f)
                    param->setValueNotifyingHost(normalized);
            }
        }
    };
    addAndMakeVisible (textEditorPort);

    textEditorSN.setBounds (120, 80, 100, 24);
    updateSN();
    textEditorSN.onTextChange = [this]()
    {
        auto text = textEditorSN.getText();
        for (int i = 0; i < 16; i++)
        {
            auto paramID = "streamname" + juce::String::formatted("%02d", i + 1);
            if (auto* param = audioProcessor.apvts.getParameter(paramID))
            {
                float newValue = (i < text.length()) ? (float)(uint8_t)text[i] : 0.0f;
                if (std::abs(param->getValue() - param->getNormalisableRange().convertTo0to1(newValue)) > 0.001f)
                {
                    param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(newValue));
                }
            }
        }
    };
    addAndMakeVisible (textEditorSN);

    sliderNQ.setSliderStyle(juce::Slider::LinearHorizontal);
    sliderNQ.setTextBoxStyle(juce::Slider::TextBoxRight, false, 30, 20);
    sliderNQ.setBounds(120, 112, 100, 24);
    addAndMakeVisible(sliderNQ);
    nqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "redundancy", sliderNQ);

    textButtonGo.setButtonText("GO");
    textButtonGo.setBounds (135, 152, 70, 24);
    textButtonGo.setClickingTogglesState(true);
    goButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "onoff", textButtonGo);
    textButtonGo.onStateChange = [this]()
    {
        bool isOn = textButtonGo.getToggleState();
        if (isOn)
        {
            textEditorIP.setEnabled(false);
            textEditorPort.setEnabled(false);
            textEditorSN.setEnabled(false);
            textButtonGo.setButtonText("STOP");
        }
        else
        {
            textEditorIP.setEnabled(true);
            textEditorPort.setEnabled(true);
            textEditorSN.setEnabled(true);
            textButtonGo.setButtonText("GO");
        }
    };
    textButtonGo.onStateChange();
    addAndMakeVisible (textButtonGo);

    corButton.setBounds (20, 152, 80, 24);
    corButton.setToggleState(false, juce::sendNotification);
    corButtonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "correction", corButton);
    addAndMakeVisible (corButton);

    gainSlider.setSliderStyle(juce::Slider::LinearVertical);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    gainSlider.setBounds (240, 16, 70, 158);
    gainSlider.setSkewFactor(0.005);
    gainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "gain", gainSlider);
    addAndMakeVisible(gainSlider);

    audioProcessor.addChangeListener(this);

    setSize (320, 210);
}

VBANReceptorAudioProcessorEditor::~VBANReceptorAudioProcessorEditor()
{
    audioProcessor.removeChangeListener(this);
    audioProcessor.editor = nullptr;
}

void VBANReceptorAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &audioProcessor)
    {
        updateIP();
        updateSN();
        labelChannels.setText(juce::String(audioProcessor.getNBC()) + " channels", juce::dontSendNotification);
    }
}
//==============================================================================
void VBANReceptorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
}

void VBANReceptorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}

void VBANReceptorAudioProcessorEditor::updateIP()
{
    juce::String currentIP;
    for (int i = 0; i < 4; i++)
    {
        auto paramID = "ip" + juce::String(i + 1);
        int oct = (int)*audioProcessor.apvts.getRawParameterValue(paramID);
        currentIP += juce::String(oct);
        if (i < 3) currentIP += ".";
    }
    textEditorIP.setText(currentIP, juce::dontSendNotification);
}

void VBANReceptorAudioProcessorEditor::updateSN()
{
    juce::String currentName = "";
    for (int i = 0; i < 16; i++)
    {
        auto paramID = "streamname" + juce::String::formatted("%02d", i + 1);
        float val01 = audioProcessor.apvts.getParameter(paramID)->getValue();
        int ascii = (int)std::round(val01 * 255.0f);
        if (ascii > 0 && ascii <= 255) currentName += (char)ascii;
    }
    textEditorSN.setText(currentName, juce::dontSendNotification);
}

void VBANReceptorAudioProcessorEditor::updateCNlabel(int num)
{
    if (num == 1) labelChannels.setText("1 channel", juce::dontSendNotification);
    else labelChannels.setText(juce::String(num) + " channels", juce::dontSendNotification);
}
