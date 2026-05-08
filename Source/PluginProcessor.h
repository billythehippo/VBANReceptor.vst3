/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cstdint>
#include "vban.h"
#include "vban_functions.h"
#include "zita-resampler/vresampler.h"

extern std::mutex rbmutex;
#define NUMBER_OF_CHANNELS 2


class VBANReceptorAudioProcessorEditor;
//==============================================================================

class VBANReceptorAudioProcessor  : public juce::AudioProcessor,
                                    public juce::AudioProcessorValueTreeState::Listener,
                                    public juce::Thread,
                                    public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    VBANReceptorAudioProcessor();
    ~VBANReceptorAudioProcessor() override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void updateIP(juce::IPAddress newAddr, bool notifyUI = true);
    void updateStreamName(const juce::String& newSN, bool notifyUI = true);
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    int getNBC();
    bool cleanup = false;
    VBANReceptorAudioProcessorEditor* editor;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void vbanToFloat(float* dest, char* src, int ns, uint8_t vbanFormat);
    void run() override;
    std::unique_ptr<juce::DatagramSocket> rxSocket;// = nullptr;
    std::unique_ptr<VResampler> resampler;
    VBanPacket rxPacket;
    uint32_t nuFrame = 0;

    ringbuffer_t* ringbuffer = NULL;
    uint32_t rbFill;
    double rratio = 1;
    double rratioSmoothed = 1;
    int16_t rbFillIntegral = 0;

    bool onoff = false;
    bool onoffCurrent = false;
    int hostNBChannels = 0;
    int rxNBChannels = 0;
    uint32_t hostSamplerate = 48000;
    uint32_t rxSamplerate = 0;
    int hostNFrames = 128;
    int rxNFrames;
    int lagrangeNum = 3;

    int resampler_inbuflen = 0;
    int resampler_outbuflen = 0;
    float* resampler_inbuf = nullptr;
    float* resampler_outbuf = nullptr;
    float* rxBuffer = nullptr;

    int redundancy;
    uint8_t vbanFormatSR = 4;
    juce::IPAddress ipAddr = juce::IPAddress(0,0,0,0);
    int udpPort = 6980;
    char streamname[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int lostPackets = 0;
    int lostFrames = 0;
    int lostSamples = 0;
    int bufReadSpace = 0;
    float gain;
    bool correctionEnabled = true;

    std::vector<float*> outputChannelData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VBANReceptorAudioProcessor)
};
