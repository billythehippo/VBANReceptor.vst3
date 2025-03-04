/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//int nbinputs = 2;
//int nboutputs = 2;

std::mutex rbmutex;

//==============================================================================
VBANReceptorAudioProcessor::VBANReceptorAudioProcessor()

    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::discreteChannels(nbinputs), true)
                        .withOutput ("Output", juce::AudioChannelSet::discreteChannels(nboutputs), true)
                      ),

    parameters(*this, nullptr, "PARAMETERS",
           {
                std::make_unique<juce::AudioParameterBool>(juce::ParameterID("onoff", 1), "OnOff", false),
                std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gain", 1), "Gain", 0.0f, 1.0f, 1.0f),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("ip1", 1), "IP1", 0, 255, 127),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("ip2", 1), "IP2", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("ip3", 1), "IP3", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("ip4", 1), "IP4", 0, 255, 1),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("port4", 1), "Port4", 0, 9, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("port3", 1), "Port3", 0, 9, 6),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("port2", 1), "Port2", 0, 9, 9),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("port1", 1), "Port1", 0, 9, 8),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("port0", 1), "Port0", 0, 9, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname01", 1), "Streamname01", 0, 255, 'S'),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname02", 1), "Streamname02", 0, 255, 't'),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname03", 1), "Streamname03", 0, 255, 'r'),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname04", 1), "Streamname04", 0, 255, 'e'),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname05", 1), "Streamname05", 0, 255, 'a'),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname06", 1), "Streamname06", 0, 255, 'm'),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname07", 1), "Streamname07", 0, 255, '1'),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname08", 1), "Streamname08", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname09", 1), "Streamname09", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname10", 1), "Streamname10", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname11", 1), "Streamname11", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname12", 1), "Streamname12", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname13", 1), "Streamname13", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname14", 1), "Streamname14", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname15", 1), "Streamname15", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("streamname16", 1), "Streamname16", 0, 255, 0),
                std::make_unique<juce::AudioParameterInt>(juce::ParameterID("redundancy", 1), "Redundancy", 0, 4, 1),
                std::make_unique<juce::AudioParameterBool>(juce::ParameterID("plucking", 1), "Plucking", true)
           })
{
}

VBANReceptorAudioProcessor::~VBANReceptorAudioProcessor()
{
    parameters.getParameter("onoff")->setValueNotifyingHost(0.0);
}

//==============================================================================
const juce::String VBANReceptorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VBANReceptorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VBANReceptorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VBANReceptorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VBANReceptorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VBANReceptorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int VBANReceptorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VBANReceptorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VBANReceptorAudioProcessor::getProgramName (int index)
{
    return {};
}

void VBANReceptorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void VBANReceptorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    //inputChannelData = (const float**)malloc(nbinputs*sizeof(float*));
    //outputChannelData = (float**)malloc(nboutputs*sizeof(float*));
}

void VBANReceptorAudioProcessor::releaseResources()
{
    parameters.getParameter("onoff")->setValueNotifyingHost(0.0);
    //if (inputChannelData!=nullptr) free(inputChannelData);
    //if (outputChannelData!=nullptr) free(outputChannelData);
    for (int i=0; i<nbinputs; i++) inputChannelData[i] = nullptr;
    for (int i=0; i<nboutputs; i++) outputChannelData[i] = nullptr;

    if (rxThread!= nullptr)
    {
        //parameters.getParameter("onoff")->setValueNotifyingHost(0.0);
        fprintf(stderr, "Stopping rxThread\r\n");
        if (rxThread->isThreadRunning()) rxThread->stopThread(1000);
        fprintf(stderr, "Cleaning up rxThread\r\n");
        //delete (rxThread);
        rxThread = nullptr;
        fprintf(stderr, "rxThread cleaned up\r\n");
    }
    else fprintf(stderr, "rxThread is already NULL\r\n");//*/
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VBANReceptorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VBANReceptorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    long sampleRate = getSampleRate();

    int channel;
    int frame;

    if (hostSamplerate!= sampleRate)
    {
        hostSamplerate = sampleRate;
        // TODO: Stop plugin and notify
    }

    if (nframes!= numSamples)
    {
        nframes = numSamples;
        // TODO: say thread to recalculate ringbuffer
    }

    gain = parameters.getRawParameterValue("gain")->load();
    pluckingEnabled = parameters.getRawParameterValue("plucking");
    redundancy = parameters.getRawParameterValue("redundancy")->load();
    onoff = parameters.getRawParameterValue("onoff")->load();
    if ((onoffCurrent==false)&&(onoff==true)) // switching on
    {
        fprintf(stderr, "Button pressed to ON\r\n");
        onoffCurrent = true;
        //rxThread = new PlugThread("VBAN RX thread"); //
        //(char* ip, uint16_t* port, char* sn, long sr, int nfr, int nbc)
        rxThread.reset(new PlugThread("VBAN RX thread"));
        rxThread->start(ipAddr, &udpPort, streamname, &hostSamplerate, &nframes, (int)totalNumOutputChannels, redundancy);
    }
    if ((onoffCurrent==true)&&(onoff==false)) // switching off
    {
        fprintf(stderr, "Button pressed to OFF\r\n");
        onoffCurrent = false;
        if (rxThread!= nullptr)
        {
            if (rxThread->isThreadRunning()) rxThread->stopThread(1000);
            //delete (rxThread);
            rxThread = nullptr;
        }
    }

    for (channel = totalNumInputChannels; channel < totalNumOutputChannels; channel++)
        buffer.clear (channel, 0, buffer.getNumSamples());

    for (channel = 0; channel < totalNumInputChannels; ++channel)
    {
        inputChannelData[channel] = (float*)buffer.getReadPointer(channel);
        outputChannelData[channel] = buffer.getWritePointer(channel);
    }

    if (onoff==true)
    {
        if (ringbuffer!=nullptr)
        {
            lostFrames = 0;
            rbmutex.lock();
            bufReadSpace = ringbuffer_read_space(ringbuffer);
            //fprintf(stderr, "Readspace %d\r\n", bufReadSpace);
            if (pluckingEnabled)
            {
                if (bufReadSpace>(ringbuffer->size*3/4)) pluckingOn = true;
                else if (bufReadSpace<(ringbuffer->size*1/2)) pluckingOn = false;
            }
            else pluckingOn = false;
            rbmutex.unlock();
            for (frame = 0; frame < numSamples; frame++)
            {
                lostSamples = 0;
                rbmutex.lock();
                bufReadSpace = ringbuffer_read_space(ringbuffer);
                if (bufReadSpace>= totalNumOutputChannels*sizeof(float))
                {
                    for (channel = 0; channel < totalNumOutputChannels; channel++)
                    {
                        if (vban_read_frame_from_ringbuffer(&outputChannelData[channel][frame], ringbuffer, 1))
                        {
                            lostSamples++;
                            pluckingOn = false;
                        }
                    }
                    if (lostSamples > 0) lostFrames++;
                    if ((frame == (numSamples - 1))&&(pluckingEnabled==true))
                    {
                        bufReadSpace = ringbuffer_read_space(ringbuffer);
                        if (bufReadSpace>= totalNumOutputChannels*sizeof(float))
                            for (channel = 0; channel < totalNumOutputChannels; channel++)
                                vban_add_frame_from_ringbuffer(&outputChannelData[channel][frame], ringbuffer, 1);
                    }
                }
                rbmutex.unlock();
            }
            for (channel = 0; channel < totalNumOutputChannels; channel++)
                for (frame=0; frame < numSamples; frame++)
                    outputChannelData[channel][frame]*= gain;
            if (lostFrames)
            {
                if (lostPackets<9) fprintf(stderr, "%d samples lost\n", lostPackets);
                if (lostFrames==numSamples)
                {
                    if (lostPackets<10) lostPackets++;
                    if (lostPackets>=9)
                    {
                        for (channel = 0; channel < totalNumOutputChannels; channel++)
                            for (frame=0; frame < numSamples; frame++)
                                outputChannelData[channel][frame] = 0;
                    }
                }
                else lostPackets = 0;
            }//*/
        }
        else
        {
            rbmutex.lock();
            ringbuffer = rxThread->getRingBufferPointer();
            rbmutex.unlock();
            if (ringbuffer!= nullptr)
            {
                if (editor!= nullptr)
                {
                    editor->gettingParametersFromProcessor = true;
                    editor->textEditorIP.clear();
                    editor->textEditorIP.setText(ipAddr);
                    editor->textEditorSN.clear();
                    editor->textEditorSN.setText(streamname);
                    editor->gettingParametersFromProcessor = false;
                }
            }//*/
            for (frame=0; frame < numSamples; frame++)
                for (channel = 0; channel < totalNumOutputChannels; channel++)
                    outputChannelData[channel][frame] = 0;
        }
    }
    else
    {
        ringbuffer = nullptr;
        for (frame=0; frame < numSamples; frame++)
            for (channel = 0; channel < totalNumOutputChannels; channel++)
                outputChannelData[channel][frame] = 0;
    }
}

//==============================================================================

//==============================================================================
bool VBANReceptorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* VBANReceptorAudioProcessor::createEditor()
{
    auto ed = new VBANReceptorAudioProcessorEditor (*this);
    //editor = ed;
    return ed;
}

//==============================================================================
void VBANReceptorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree stateTree = parameters.copyState();
    juce::MemoryOutputStream stream(destData, true);
    stateTree.writeToStream(stream);
}

void VBANReceptorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (sizeInBytes > 0)
    {
        parameters.getParameter("onoff")->setValueNotifyingHost(0.0);
        juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
        juce::ValueTree stateTree = juce::ValueTree::readFromStream(stream);
        if (stateTree.isValid()) parameters.replaceState(stateTree);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VBANReceptorAudioProcessor();
}

//==============================================================================
int VBANReceptorAudioProcessor::refreshStreamNameTextFromParameters(char* text, int textlen)
{
    int* asciiCode;
    uint8_t snu8t[VBAN_STREAM_NAME_SIZE];
    char sname[VBAN_STREAM_NAME_SIZE];

    memset(snu8t, 0, VBAN_STREAM_NAME_SIZE);
    snu8t[0] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname01")->load());
    if (snu8t[0]==0) return 1;
    snu8t[1] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname02")->load());
    snu8t[2] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname03")->load());
    snu8t[3] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname04")->load());
    snu8t[4] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname05")->load());
    snu8t[5] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname06")->load());
    snu8t[6] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname07")->load());
    snu8t[7] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname08")->load());
    snu8t[8] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname09")->load());
    snu8t[9] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname10")->load());
    snu8t[10] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname11")->load());
    snu8t[11] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname12")->load());
    snu8t[12] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname13")->load());
    snu8t[13] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname14")->load());
    snu8t[14] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname15")->load());
    snu8t[15] = static_cast<uint8_t>(parameters.getRawParameterValue("streamname16")->load());

    memcpy(sname, snu8t, VBAN_STREAM_NAME_SIZE);
    memcpy(text, sname, textlen);

    return 0;
}

void VBANReceptorAudioProcessor::refreshStreamNameParametersFromText(const char* text, int textlen)
{
    static uint8_t snu8t[VBAN_STREAM_NAME_SIZE];

    memset(snu8t, 0, VBAN_STREAM_NAME_SIZE);
    memcpy(snu8t, text, textlen);

    parameters.getParameter("streamname01")->setValueNotifyingHost((float)snu8t[0]/255.0f);
    parameters.getParameter("streamname02")->setValueNotifyingHost((float)snu8t[1]/255.0f);
    parameters.getParameter("streamname03")->setValueNotifyingHost((float)snu8t[2]/255.0f);
    parameters.getParameter("streamname04")->setValueNotifyingHost((float)snu8t[3]/255.0f);
    parameters.getParameter("streamname05")->setValueNotifyingHost((float)snu8t[4]/255.0f);
    parameters.getParameter("streamname06")->setValueNotifyingHost((float)snu8t[5]/255.0f);
    parameters.getParameter("streamname07")->setValueNotifyingHost((float)snu8t[6]/255.0f);
    parameters.getParameter("streamname08")->setValueNotifyingHost((float)snu8t[7]/255.0f);
    parameters.getParameter("streamname09")->setValueNotifyingHost((float)snu8t[8]/255.0f);
    parameters.getParameter("streamname10")->setValueNotifyingHost((float)snu8t[9]/255.0f);
    parameters.getParameter("streamname11")->setValueNotifyingHost((float)snu8t[10]/255.0f);
    parameters.getParameter("streamname12")->setValueNotifyingHost((float)snu8t[11]/255.0f);
    parameters.getParameter("streamname13")->setValueNotifyingHost((float)snu8t[12]/255.0f);
    parameters.getParameter("streamname14")->setValueNotifyingHost((float)snu8t[13]/255.0f);
    parameters.getParameter("streamname15")->setValueNotifyingHost((float)snu8t[14]/255.0f);
    parameters.getParameter("streamname16")->setValueNotifyingHost((float)snu8t[15]/255.0f);
}

int VBANReceptorAudioProcessor::refreshIPAddressTextFromParameters(char* text)
{
    char ipaddress[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t ipBytes[4];
    memset(ipaddress, 0, 16);

    ipBytes[0] = static_cast<uint8_t>(parameters.getRawParameterValue("ip1")->load());
    ipBytes[1] = static_cast<uint8_t>(parameters.getRawParameterValue("ip2")->load());
    ipBytes[2] = static_cast<uint8_t>(parameters.getRawParameterValue("ip3")->load());
    ipBytes[3] = static_cast<uint8_t>(parameters.getRawParameterValue("ip4")->load());
    sprintf(ipaddress, "%hhu.%hhu.%hhu.%hhu", ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]);
    //fprintf(stderr, "Ipaddress: %s\r\n", ipaddress);
    memcpy(text, ipaddress, 16);
    return 0;
}

void VBANReceptorAudioProcessor::refreshIPAddressParametersFromText(const char* text)
{
    static uint8_t ipBytes[4];

    sscanf(text, "%hhu.%hhu.%hhu.%hhu", &ipBytes[0], &ipBytes[1], &ipBytes[2], &ipBytes[3]);
    parameters.getParameter("ip1")->setValueNotifyingHost((float)ipBytes[0]/255.0f);
    parameters.getParameter("ip2")->setValueNotifyingHost((float)ipBytes[1]/255.0f);
    parameters.getParameter("ip3")->setValueNotifyingHost((float)ipBytes[2]/255.0f);
    parameters.getParameter("ip4")->setValueNotifyingHost((float)ipBytes[3]/255.0f);
}

int VBANReceptorAudioProcessor::refreshPortTextFromParameters(char* text)
{
    char portText[5] = {0, 0, 0, 0, 0};
    float portDigits[5];
    int len = 0;
    int port = 0;
    int mul = 1;

    portDigits[0] = parameters.getRawParameterValue("port0")->load();
    portDigits[1] = parameters.getRawParameterValue("port1")->load();
    portDigits[2] = parameters.getRawParameterValue("port2")->load();
    portDigits[3] = parameters.getRawParameterValue("port3")->load();
    portDigits[4] = parameters.getRawParameterValue("port4")->load();
    for (int i=0; i<5; i++)
    {
        port+= portDigits[i]*mul;
        mul*= 10;
    }
    if (text!=nullptr)
    {
        sprintf(portText, "%d", port);
        //fprintf(stderr, "Port: %d %s\r\n", port, portText);
        memcpy(text, portText, strlen(portText));
    }
    else fprintf(stderr, "Error: Cannot save port as text\r\n");
    return port;
}

int VBANReceptorAudioProcessor::refreshPortParametersFromText(const char* text)
{
    static uint8_t portDigits[5] = {0, 0, 0, 0, 0};
    static int len = 0;
    static int port = 0;
    static int portDiv = 0;

    len = strlen(text);
    sscanf(text, "%d", &port);
    portDiv = port;
    for (int i=0; i<len; i++)
    {
        portDigits[i] = portDiv%10;
        portDiv = portDiv/10;
        if (portDiv==0) break;
    }
    parameters.getParameter("port0")->setValueNotifyingHost((float)portDigits[0]/9.0f);
    parameters.getParameter("port1")->setValueNotifyingHost((float)portDigits[1]/9.0f);
    parameters.getParameter("port2")->setValueNotifyingHost((float)portDigits[2]/9.0f);
    parameters.getParameter("port3")->setValueNotifyingHost((float)portDigits[3]/9.0f);
    parameters.getParameter("port4")->setValueNotifyingHost((float)portDigits[4]/9.0f);

    return port;
}
