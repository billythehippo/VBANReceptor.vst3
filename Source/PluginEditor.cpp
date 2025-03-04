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
  char IPAddr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  char udpPortTxt[5] = {0, 0, 0, 0, 0};
  int udpPort;
  char Streamname[VBAN_STREAM_NAME_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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

  char channelsnum[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  if (nboutputs==1) sprintf(channelsnum, "1 channel");
  else sprintf(channelsnum, "%d channels", nboutputs);
  fprintf(stderr, "%s\r\n", channelsnum);
  labelChannels.setText(channelsnum, juce::dontSendNotification);
  labelChannels.setJustificationType (36);
  addAndMakeVisible (labelChannels);
  labelChannels.setBounds (120, 184, 80, 24);

  addAndMakeVisible (textEditorIP); //refreshIPAddressTextFromParameters(ipAddr);
  audioProcessor.refreshIPAddressTextFromParameters(IPAddr);
  textEditorIP.setText (TRANS (IPAddr));
  textEditorIP.setBounds (120, 16, 100, 24);
  textEditorIP.onTextChange = [this]()
  {
    bool onoffState = audioProcessor.parameters.getRawParameterValue("onoff")->load();
    if ((onoffState==false)||(gettingParametersFromProcessor==true))
    {
      audioProcessor.refreshIPAddressParametersFromText((char*)textEditorIP.getText().toRawUTF8());
    }
    else
    {
      audioProcessor.refreshIPAddressTextFromParameters((char*)textEditorIP.getText().toRawUTF8());
      //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Warning", "Unable to change IP address while running!", "OK");
    }
  };

  addAndMakeVisible (textEditorPort);
  udpPort = audioProcessor.refreshPortTextFromParameters(udpPortTxt);
  textEditorPort.setText (TRANS (udpPortTxt));
  textEditorPort.setBounds (120, 48, 100, 24);
  textEditorPort.onTextChange = [this]()
  {
    int udpPort;
    bool onoffState = audioProcessor.parameters.getRawParameterValue("onoff")->load();
    if (onoffState==false)
    {
      udpPort = audioProcessor.refreshPortParametersFromText((char*)textEditorPort.getText().toRawUTF8());
    }
    else
    {
      udpPort = audioProcessor.refreshPortTextFromParameters((char*)textEditorPort.getText().toRawUTF8());
      //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Warning", "Unable to change UDP port while running!", "OK");
    }
  };

  addAndMakeVisible (textEditorSN);
  if (audioProcessor.refreshStreamNameTextFromParameters(Streamname, VBAN_STREAM_NAME_SIZE))
  {
    textEditorSN.clear();
    fprintf(stderr, "Can't refresh Streamname from parameters\r\n");
  }//*/
  else
  {
    textEditorSN.setText (TRANS (Streamname));
    fprintf(stderr, "Refreshing Streamname from parameters\r\n");
  }
  textEditorSN.setBounds (120, 80, 100, 24);
  textEditorSN.onTextChange = [this]()
  {
    bool onoffState = audioProcessor.parameters.getRawParameterValue("onoff")->load();
    if ((onoffState==false)||(gettingParametersFromProcessor==true))
    {
      audioProcessor.refreshStreamNameParametersFromText((char*)textEditorSN.getText().toRawUTF8(), strlen(textEditorSN.getText().toRawUTF8()));
    }
    else
    {
      audioProcessor.refreshStreamNameTextFromParameters((char*)textEditorSN.getText().toRawUTF8(), strlen(textEditorSN.getText().toRawUTF8()));
      //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Warning", "Unable to change Streamname while running!", "OK");
    }
  };

  addAndMakeVisible (comboBoxNQ);
  comboBoxNQ.addItem ("0", 1);
  comboBoxNQ.addItem ("1", 2);
  comboBoxNQ.addItem ("2", 3);
  comboBoxNQ.addItem ("3", 4);
  comboBoxNQ.addItem ("4", 5);
  comboBoxNQ.setBounds (120, 112, 100, 24);
  //comboBoxNQ.setSelectedItemIndex(0);
  comboBoxNQ.setSelectedItemIndex(audioProcessor.parameters.getRawParameterValue("redundancy")->load());
  comboBoxNQ.onChange = [this]()
  {
    float newValue;
    bool onoffState = audioProcessor.parameters.getRawParameterValue("onoff")->load();
    if (onoffState==false) // Current state is OFF
    {
      newValue = comboBoxNQ.getSelectedItemIndex();
      audioProcessor.parameters.getParameter("redundancy")->setValueNotifyingHost(newValue/4.0f);
    }
    else
    {
      newValue = audioProcessor.parameters.getRawParameterValue("redundancy")->load();
      comboBoxNQ.setSelectedId((int)newValue);
    }
  };

  textButtonGo.setButtonText(TRANS ("GO"));
  addAndMakeVisible (textButtonGo);
  textButtonGo.setBounds (135, 152, 70, 24);
  textButtonGo.onClick = [this]()
  {
    auto* onoffState = audioProcessor.parameters.getRawParameterValue("onoff");
    if (onoffState != nullptr)
    {
      if (*onoffState==false) // Current state is OFF
      {
        audioProcessor.parameters.getParameter("onoff")->setValueNotifyingHost(true);
        textButtonGo.setButtonText("STOP");

        //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "GO clicked", "GO!", "OK");
      }
      else // Current state is ON
      {
        audioProcessor.parameters.getParameter("onoff")->setValueNotifyingHost(false);
        textButtonGo.setButtonText("GO");

        //juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "GO clicked", "STOP!", "OK");
      }
    }
  };
  addAndMakeVisible (pluckingOnOff);
  pluckingOnOff.setBounds (24, 152, 70, 24);
  if (audioProcessor.parameters.getRawParameterValue("plucking")->load()) pluckingOnOff.setToggleState(true, juce::dontSendNotification);
  else pluckingOnOff.setToggleState(false, juce::dontSendNotification);
  pluckingOnOff.onClick = [this]()
  {
    if (pluckingOnOff.getToggleState())
    {
      audioProcessor.parameters.getParameter("plucking")->setValueNotifyingHost(1);
    }
    else
    {
      audioProcessor.parameters.getParameter("plucking")->setValueNotifyingHost(0);
    }
  };
  //audioProcessor.parameters.addParameterListener("plucking", this);

  gainSlider.setSliderStyle(juce::Slider::LinearVertical);
  gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  addAndMakeVisible(gainSlider);
  gainSlider.setBounds (240, 16, 70, 158);
  gainSlider.setSkewFactor(0.005);
  gainSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "gain", gainSlider);

  setSize (320, 210);//(240, 260);
}

VBANReceptorAudioProcessorEditor::~VBANReceptorAudioProcessorEditor()
{
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
