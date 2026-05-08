/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ringbuffer.h"
#include "vban.h"
#include "zita-resampler/vresampler.h"
#include <cstdint>
#include <cstdlib>

#define UDP_CYCLE_US 100
#define CFMSIZE 5


std::mutex rbmutex;

uint16_t cfmed(uint16_t* medArray)
{
    uint8_t mind;
    uint8_t mid = (CFMSIZE>>1) + 1;
    uint16_t min;
    uint16_t array[CFMSIZE];
    memcpy(array, medArray, CFMSIZE*sizeof(uint16_t));
    for (uint8_t i = 0; i <= mid; i++)
    {
        mind = i;
        min = array[i];
        for (uint8_t j = i + 1; j < CFMSIZE; j++) if (array[j] < min)
        {
            mind = j;
            min = array[mind];
        }
        array[mind] = array[i];
        array[i] = min;
    }
    return array[mid];
}

//==============================================================================
VBANReceptorAudioProcessor::VBANReceptorAudioProcessor()

    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::canonicalChannelSet(NUMBER_OF_CHANNELS), true)
                      ),
    apvts(*this, nullptr, "Parameters", createParameterLayout()),
    juce::Thread("rxThread")
{
    memset(&rxPacket, 0, sizeof(VBanPacket));
    apvts.addParameterListener("onoff", this);
    apvts.addParameterListener ("correction", this);
    for (int i = 0; i < 16; i++) apvts.addParameterListener("streamname" + juce::String::formatted("%02d", i + 1), this);
    for (int i = 0; i < 5; i++)  apvts.addParameterListener("port" + juce::String(i), this);
    for (int i = 0; i < 4; i++)  apvts.addParameterListener("ip" + juce::String(i + 1), this);
}

VBANReceptorAudioProcessor::~VBANReceptorAudioProcessor()
{
    stopThread (1000);
    apvts.removeParameterListener("onoff", this);
    apvts.removeParameterListener ("correction", this);
    for (int i = 0; i < 16; i++) apvts.removeParameterListener("streamname" + juce::String::formatted("%02d", i + 1), this);
    for (int i = 0; i < 5; i++)  apvts.removeParameterListener("port" + juce::String(i), this);
    for (int i = 0; i < 4; i++)  apvts.removeParameterListener("ip" + juce::String(i + 1), this);
}

juce::AudioProcessorValueTreeState::ParameterLayout VBANReceptorAudioProcessor::createParameterLayout()
{
    uint8_t ipbytes[4] = {127, 0, 0, 1};
    uint8_t portdigits[5] = {0, 6, 9, 8, 0};
    memset(streamname, 0, 16);
    strncpy(streamname, "Stream1", 7);
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("onoff", 1), "OnOff", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("correction", 1), "Correction", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gain", 1), "Gain", 0.0f, 1.0f, 1.0f));
    for (int i = 0; i < 4; i++)
    {
        auto paramID = "ip" + juce::String(i + 1);
        auto paramNm = "IP" + juce::String(i + 1);
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(paramID, 1), paramNm, 0, 255, ipbytes[i]));
    }
    for (int i = 0; i < 5; i++)
    {
        auto paramID = "port" + juce::String(i);
        auto paramNm = "Port" + juce::String(i);
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(paramID, 1), paramNm, 0, 9, portdigits[i]));
    }
    for (int i = 0; i < 16; i++)
    {
        auto paramID = "streamname" + juce::String::formatted("%02d", i + 1, 2);
        auto paramNm = "Streamname" + juce::String::formatted("%02d", i + 1, 2);
        layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID(paramID, 1), paramNm, 0, 255, streamname[i]));
    }
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID("redundancy", 1), "Redundancy", 0, 4, 1));
    return layout;
}

void VBANReceptorAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "onoff")
    {
        bool isOn = apvts.getRawParameterValue("onoff")->load() > 0.5f;
        if (isOn)
        {
            rxSocket = std::make_unique<juce::DatagramSocket>(true);
            if (!rxSocket->bindToPort(udpPort))
            {
                rxSocket = nullptr;
                DBG ("Failed to bind socket");
            }
            else
            {
                startThread();
                fprintf(stderr, "Starting...\r\n");
            }
        }
        else
        {
            stopThread(500);
            rxSocket = nullptr;
        }
    }
    else if (parameterID.startsWith("port"))
    {
        juce::String fullPortString = "";
        for (int i = 0; i < 5; i++)
        {
            int digit = (int)*apvts.getRawParameterValue("port" + juce::String(i));
            fullPortString += juce::String(digit);
        }
        int finalPort = fullPortString.getIntValue();
        udpPort = juce::jlimit(0, 65535, finalPort); // Ограничиваем валидным диапазоном TCP/UDP портов
        fprintf(stderr, "New Port: %d\r\n", udpPort);
    }
    else if (parameterID.startsWith("streamname"))
    {
        int index = parameterID.substring(10).getTrailingIntValue() - 1;
        if (index >= 0 && index < 16)
        {
            streamname[index] = static_cast<char>(std::round(newValue));
        }
    }
    else if (parameterID.startsWith("ip"))
    {
         ipAddr = juce::IPAddress((uint8_t)*apvts.getRawParameterValue("ip1"),
                                  (uint8_t)*apvts.getRawParameterValue("ip2"),
                                  (uint8_t)*apvts.getRawParameterValue("ip3"),
                                  (uint8_t)*apvts.getRawParameterValue("ip4"));
    }
}

void VBANReceptorAudioProcessor::updateIP(juce::IPAddress newAddr, bool notifyUI)
{
    for (int i = 0; i < 4; i++)
    {
        auto paramID = "ip" + juce::String(i + 1);
        if (auto* param = apvts.getParameter(paramID))
        {
            float byteVal = (float)newAddr.address[i];
            float normalized = param->getNormalisableRange().convertTo0to1(byteVal);
            if (std::abs(param->getValue() - normalized) > 0.0001f) param->setValueNotifyingHost(normalized);
        }
    }
    ipAddr = newAddr;
    if (hasEditor()&&notifyUI) sendChangeMessage();
}

void VBANReceptorAudioProcessor::updateStreamName(const juce::String& newSN, bool notifyUI)
{
    for (int i = 0; i < 16; i++)
    {
        auto paramID = "streamname" + juce::String::formatted ("%02d", i + 1);
        if (auto* param = apvts.getParameter (paramID))
        {
            auto& range = param->getNormalisableRange();
            float charVal = (i < newSN.length()) ? (float)(uint8_t)newSN[i] : 0.0f;
            float normalized = range.convertTo0to1 (charVal);
            if (std::abs (param->getValue() - normalized) > 0.0001f) param->setValueNotifyingHost (normalized);
            streamname[i] = static_cast<char> (range.convertFrom0to1 (normalized));
        }
    }
    if (hasEditor()&&notifyUI) sendChangeMessage();
}

void VBANReceptorAudioProcessor::vbanToFloat(float* dest, char* src, int ns, uint8_t vbanFormat)
{
    int sample;
    uint8_t sampleSize = VBanBitResolutionSize[vbanFormat&VBAN_BIT_RESOLUTION_MASK];

    if (vbanFormat == VBAN_BITFMT_32_FLOAT) memcpy(dest, src, ns * sizeof(float));
    else for (int i = 0; i < ns; i++)
        {
            sample = 0;
            memmove(&sample, src + sampleSize * i, sampleSize);
            sample = sample << (sizeof(int) - sampleSize) * 8;
            dest[i] = (float)sample/2147483648.0f;
        }
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
    int numChannels = getTotalNumInputChannels();

    if ((hostNBChannels!= getTotalNumOutputChannels())||(hostNFrames!= (uint32_t)samplesPerBlock)||(hostSamplerate!= (uint32_t)sampleRate))
    {
        hostSamplerate = (int32_t)sampleRate;
        hostNBChannels = getTotalNumOutputChannels();
        hostNFrames = samplesPerBlock;
        if (hasEditor()) sendChangeMessage();
        fprintf(stderr, "Channels: %d, buffer %d frames, samplerate %d\r\n", hostNBChannels, hostNFrames, hostSamplerate);
        outputChannelData.resize(hostNBChannels);
        if (rxBuffer!= nullptr) free(rxBuffer);
        rxBuffer = (float*)malloc((hostNFrames + lagrangeNum) * sizeof(float) * hostNBChannels);
        if (ringbuffer!= NULL) ringbuffer_free(ringbuffer);
        ringbuffer = NULL;
        ringbuffer = ringbuffer_create(hostNFrames * hostNBChannels * sizeof(float) * 2 * (1 + redundancy));
        //fprintf(stderr, "RB %d bytes, RXbuf %d bytes\r\n %d channels, %d/%lu\r\n", ringbuffer->size, (hostNFrames + lagrangeNum) * sizeof(float) * hostNBChannels, hostNBChannels, hostNFrames, hostSamplerate);
    }
}

void VBANReceptorAudioProcessor::releaseResources()
{
    apvts.getParameter("onoff")->setValueNotifyingHost(0.0);
    // if (rxBuffer!= nullptr)
    // {
    //     free(rxBuffer);
    //     rxBuffer = nullptr;
    // }
    // ringbuffer_free(ringbuffer);
    // ringbuffer = nullptr;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VBANReceptorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    int outputs = layouts.getMainOutputChannelSet().size();
    //if ((outputs < 1)||(outputs > 64)) return false;
    if (outputs!= NUMBER_OF_CHANNELS) return false;
    return true;
}
#endif

void VBANReceptorAudioProcessor::run()
{
    juce::String senderIP;
    juce::IPAddress senderIPAddr;
    uint16_t frame = 0;
    int senderPort;
    int pktlen = 0;
    int nbChannels;
    float* frameBuf = nullptr;
    uint16_t frameSizeF = 0;
    double currentTime;
    double newTime = 0;
    double oldTime = 0;
    double timeDelta;
    double timeDeltaSmoothed = 0;
    uint16_t rxCycleFrames = 0;
    uint16_t rxCycleFramesMarr[CFMSIZE];
    uint16_t rxCyclePacCnt = 0;
    uint8_t cfind = 0;
    bool netXRun = false;

    setPriority(juce::Thread::Priority::highest);
    while (!threadShouldExit()&&(rxSocket!= nullptr))
    {
        if (rxSocket->waitUntilReady (true, 50) > 0)
        {
            currentTime = juce::Time::getHighResolutionTicks();
            memset(&rxPacket, 0, VBAN_PROTOCOL_MAX_SIZE);
            pktlen = rxSocket->read(&rxPacket, sizeof(VBanPacket), false, senderIP, senderPort);
            senderIPAddr = juce::IPAddress(senderIP);
            if ((pktlen>VBAN_HEADER_SIZE)&&(rxPacket.header.vban==VBAN_HEADER_FOURC))
            {
                switch (rxPacket.header.format_SR&VBAN_PROTOCOL_MASK)
                {
                case VBAN_PROTOCOL_AUDIO:
                    // AUDIO
                    if ((rxSamplerate == 0)||(rxNBChannels == 0)||(ringbuffer == NULL)) // No stream receiving
                    {
                        if (streamname[0]==0)
                        {
                            if ((ipAddr == juce::IPAddress::any())||(ipAddr == senderIPAddr))
                            {
                                updateStreamName(juce::String(rxPacket.header.streamname, 16).trim());
                                if (ipAddr == juce::IPAddress::any()) updateIP(senderIPAddr);
                            }
                        }
                        else if (memcmp(streamname, rxPacket.header.streamname, VBAN_STREAM_NAME_SIZE) == 0)
                        {
                            if (ipAddr == juce::IPAddress::any()) updateIP(senderIPAddr);
                        }

                        if ((memcmp(rxPacket.header.streamname, streamname, strlen(rxPacket.header.streamname))==0)&&(ipAddr == senderIPAddr))
                        {
                            rxSamplerate = VBanSRList[rxPacket.header.format_SR&VBAN_SR_MASK];
                            rxNBChannels = rxPacket.header.format_nbc + 1;
                            nbChannels = (hostNBChannels < rxNBChannels ? hostNBChannels : rxNBChannels);

                            if (rxSamplerate!= 0)
                            {
                                if (resampler_inbuf!=nullptr) free(resampler_inbuf);
                                if (resampler_outbuf!=nullptr) free(resampler_outbuf);
                                resampler_inbuflen = rxPacket.header.format_nbs + 1;
                                resampler_inbuf = (float*)malloc(resampler_inbuflen * nbChannels * sizeof(float));
                                resampler_outbuflen = resampler_inbuflen * hostSamplerate / rxSamplerate + 1;
                                resampler_outbuf = (float*)malloc(resampler_outbuflen * nbChannels * sizeof(float));
                                if (resampler!= nullptr) resampler = nullptr;
                                resampler = std::make_unique<VResampler>();
                                if (resampler->setup((double)hostSamplerate / (double)rxSamplerate, nbChannels, 64))
                                {
                                    fprintf (stderr, "Resampler can't handle the ratio\r\n");
                                    exit(1);
                                }
                                fprintf(stderr, "SRs = %d/%d, channels: rx %d, host %d\r\n", hostSamplerate, rxSamplerate, rxNBChannels, hostNBChannels);
                                resampler->inp_count = resampler->inpsize() - 1;
                                resampler->inp_data = 0;
                                resampler->out_count = 999999;
                                resampler->out_data = 0;
                                resampler->process();
                            }
                        }
                    }
                    else
                    {
                        nbChannels = (hostNBChannels < rxNBChannels ? hostNBChannels : rxNBChannels);
                        if (frameSizeF!= hostNBChannels)
                        {
                            frameSizeF = hostNBChannels;
                            if (frameBuf) free(frameBuf);
                            frameBuf = (float*)malloc(hostNBChannels * sizeof(float));
                            memset(frameBuf, 0, hostNBChannels * sizeof(float));
                        }

                        if ((memcmp(rxPacket.header.streamname, streamname, strlen(rxPacket.header.streamname))==0)&&(ipAddr == senderIPAddr)&&(rxPacket.header.nuFrame!= nuFrame)&&(ringbuffer!= nullptr))
                        {
                            for (frame = 0; frame < resampler_inbuflen; frame++)
                                vbanToFloat(&resampler_inbuf[frame * nbChannels], rxPacket.data + (frame * rxNBChannels * VBanBitResolutionSize[rxPacket.header.format_bit]), nbChannels, rxPacket.header.format_bit);
                            resampler->inp_count = resampler_inbuflen;
                            resampler->inp_data = resampler_inbuf;
                            resampler->out_count = resampler_outbuflen;
                            resampler->out_data = resampler_outbuf;
                            //fprintf(stderr, "Resampling...\r\n");
                            resampler->process();
                            //fprintf(stderr, "Resampling done! (%d)\r\n", resampler->out_count);
                            for (frame = 0; frame < (resampler_outbuflen - resampler->out_count); frame++)
                            {
                                memcpy(frameBuf, &resampler_outbuf[frame * nbChannels], nbChannels * sizeof(float));
                                if (ringbuffer_write_space(ringbuffer)>=frameSizeF * sizeof(float))
                                {
                                    ringbuffer_write(ringbuffer, (const char*)frameBuf, frameSizeF * sizeof(float));
                                }
                            }

                            newTime = juce::Time::highResolutionTicksToSeconds(currentTime) * 1000000.0;
                            timeDelta = newTime - oldTime;
                            if ((apvts.getRawParameterValue("correction")->load() > 0.5f)&&(oldTime!= 0))
                            {
                                if (timeDeltaSmoothed == 0) timeDeltaSmoothed = timeDelta;
                                else timeDeltaSmoothed = 0.9 *timeDeltaSmoothed + 0.1 * timeDelta;
                                if (timeDelta > UDP_CYCLE_US) // Transaction starts
                                {
                                    rxCycleFramesMarr[cfind] = rxCycleFrames;
                                    cfind = (cfind == CFMSIZE - 1 ? 0 : cfind + 1);
                                    rxNFrames = cfmed(rxCycleFramesMarr);

                                    if ((rxNFrames > hostNFrames)&&(ringbuffer!= nullptr))
                                        rbFill = ringbuffer_read_space(ringbuffer)/(hostNBChannels * sizeof(float)) + hostNFrames - frame;

                                    rxCyclePacCnt = 1;
                                    rxCycleFrames = rxPacket.header.format_nbs + 1;
                                    netXRun = false;
                                }
                                else
                                {
                                    if (rxPacket.header.nuFrame != nuFrame) netXRun = true;
                                    rxCyclePacCnt++;
                                    rxCycleFrames+= rxPacket.header.format_nbs + 1;
                                }
                            }
                            oldTime = newTime;

                            nuFrame = rxPacket.header.nuFrame;
                        }
                    }
                    break;
                    case VBAN_PROTOCOL_SERIAL:
                        // MIDI
                        // TODO MIDI
                    break;
                    case VBAN_PROTOCOL_TXT:
                        // TEXT
                        nbChannels = (hostNBChannels < rxNBChannels ? hostNBChannels : rxNBChannels);
                        fprintf(stderr, "Info request from %s:%d\n", senderIP.toRawUTF8(), senderPort);
                        if ((memcmp(rxPacket.header.streamname, "INFO", 4)==0)&&((memcmp(rxPacket.data, "/info", 5)==0)||(memcmp(rxPacket.data, "/INFO", 5)==0)))
                        {
                            VBanPacket infopacket;
                            memset(&infopacket, 0, VBAN_PROTOCOL_MAX_SIZE);
                            infopacket.header.vban = VBAN_HEADER_FOURC;
                            infopacket.header.format_SR = VBAN_PROTOCOL_TXT;
                            strcpy(infopacket.header.streamname, "INFO");
                            sprintf(infopacket.data, "VST3_Receptor streamnamerx=%s nboutputs=%d", streamname, nbChannels);
                            rxSocket->write(senderIP, senderPort, &infopacket, VBAN_HEADER_SIZE + strlen(infopacket.data));
                        }
                    break;
                    case VBAN_PROTOCOL_USER:
                        // RAW or other
                    break;
                    default:
                        // non matching packets etc.
                    break;
                }
            }
        }
    }
}

void VBANReceptorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    int ust;
    double rbFillPI;
    double rbFillPPM = 0;
    float w = 0.01;
    uint16_t lost = 0;
    uint16_t llost = 0;
    uint16_t lcnt = 0;

    int channel;
    int frame = 0;
    int lframe = 0;
    int rxBufPos = 0;
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    long sampleRate = getSampleRate();
    if ((hostSamplerate!= sampleRate)||(hostNFrames!= numSamples)||(hostNBChannels!= totalNumOutputChannels))
    {
        hostSamplerate = sampleRate;
        hostNFrames = numSamples;
        hostNBChannels = totalNumOutputChannels;
        if (hasEditor()) sendChangeMessage();
    }

    bool onoff = true;//apvts.getRawParameterValue("onoff")->load() > 0.5f;
    float gain = apvts.getRawParameterValue("gain")->load();
    //bool correctionEnabled = apvts.getRawParameterValue("correction")->load() > 0.5f;
    // redundancy = apvts.getRawParameterValue("redundancy")->load();

    for (channel = totalNumInputChannels; channel < totalNumOutputChannels; channel++)
        buffer.clear (channel, 0, buffer.getNumSamples());

    for (channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        outputChannelData[channel] = buffer.getWritePointer(channel);
    }
    if (onoff==true)
    {
        if (ringbuffer!=nullptr)
        {
            memcpy(rxBuffer, rxBuffer + hostNFrames * hostNBChannels, lagrangeNum * hostNBChannels * sizeof(float)); // TAIL TO HEAD
            for (frame = 0; frame < numSamples; frame++)
            {
                rxBufPos = hostNBChannels * (lframe + lagrangeNum);
                //rbmutex.lock();
                if (ringbuffer_read_space(ringbuffer) < totalNumOutputChannels*sizeof(float))
                {
                    lost++;
                    llost++;
                    if (llost < 3)
                    {
                        lcnt++;
                        for (int channel = 0; channel < hostNBChannels; channel++)
                            ((float*)(rxBuffer + rxBufPos))[channel] =  2.4 * ((float*)(rxBuffer + rxBufPos - hostNBChannels))[channel] -
                                                                        2.4 * ((float*)(rxBuffer + rxBufPos - 2 * hostNBChannels))[channel] +
                                                                        0.8 * ((float*)(rxBuffer + rxBufPos - 3 * hostNBChannels))[channel]; // LAGRANGE
                        for (int channel = 0; channel < hostNBChannels; channel++)
                            outputChannelData[channel][lframe] = ((float*)(rxBuffer + hostNBChannels * lframe))[channel];
                        lframe++;
                    }
                }
                else
                {
                    ringbuffer_read(ringbuffer, (char*)(rxBuffer + rxBufPos), hostNBChannels * sizeof(float));
                    if ((llost > 0)&&(llost < 3))
                    {
                        for (int channel = 0; channel < hostNBChannels; channel++)
                            ((float*)(rxBuffer + rxBufPos))[channel] =  0.3 * ((float*)(rxBuffer + rxBufPos))[channel] +
                                                                        0.9 * ((float*)(rxBuffer + rxBufPos - hostNBChannels))[channel] -
                                                                        0.9 * ((float*)(rxBuffer + rxBufPos - 2 * hostNBChannels))[channel] +
                                                                        0.3 * ((float*)(rxBuffer + rxBufPos - 3 * hostNBChannels))[channel]; // LAGRANGE
                    }
                    llost = 0;
                    for (int channel = 0; channel < hostNBChannels; channel++)
                        outputChannelData[channel][lframe] = ((float*)(rxBuffer + hostNBChannels * lframe))[channel];
                    lframe++;
                }
                //rbmutex.unlock();
            }

            if (lframe < (hostNFrames >> 1))
            {
                memmove(rxBuffer + hostNBChannels * (hostNFrames - 1 - lframe) + lagrangeNum,
                        rxBuffer + hostNBChannels * lframe + lagrangeNum,
                        hostNBChannels * lframe * sizeof(float));
                for (frame = hostNFrames - 1 - lframe; frame < hostNFrames; frame++)
                    for (int channel = 0; channel < hostNBChannels; channel++)
                        outputChannelData[channel][frame] = ((float*)(rxBuffer + hostNBChannels * frame))[channel];
            }

            if (lost)
            {
                if (lostPackets < 9) fprintf(stderr, "%d frames lost!\r\n", lost);
                if (lost == hostNFrames)
                {
                    if (lostPackets < 10) lostPackets++;
                    if (lostPackets >= 9)
                        for (int channel = 0; channel < hostNBChannels; channel++)
                            for (int frame = 0; frame < hostNFrames; frame++)
                                outputChannelData[channel][frame] = 0;
                }
            }

            for (channel = 0; channel < totalNumOutputChannels; channel++)
                for (frame = 0; frame < numSamples; frame++)
                    outputChannelData[channel][frame]*= gain;

            if (apvts.getRawParameterValue("correction")->load() > 0.5f)
            {
                if (rxNFrames <= hostNFrames) rbFill = ringbuffer_read_space(ringbuffer)/(hostNBChannels * sizeof(float));
                ust = hostNFrames + rxNFrames + lagrangeNum;
                if (std::abs(rratioSmoothed - 1) > 0.0025) rratioSmoothed = 1.0;
                w = (((rbFill - ust) > (2 * ust)) ? 0.1 : 0.01);
                rbFillIntegral+= ust - rbFill;
                if (rbFillIntegral > 1000) rbFillIntegral = 1000;
                if (rbFillIntegral <-1000) rbFillIntegral =-1000;
                rbFillPPM = 0.0001 * (5 * (ust - rbFill) + 0.005 * rbFillIntegral);
                rbFillPI = 1 + rbFillPPM;
                if (rbFillPPM < 0) rbFillPI+= rbFillPPM;
                rratio = 0.99 * rratioSmoothed + 0.01 * rbFillPI;
                if (rratio > 1.003) rratio = 1.003;
                else if (rratio < 0.998) rratio = 0.998;
                if (resampler!= nullptr) resampler->set_rratio(rratio);
            }
        }
    }
}

//==============================================================================
int VBANReceptorAudioProcessor::getNBC()
{
    return hostNBChannels;
}


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
    juce::ValueTree stateTree = apvts.copyState();
    juce::MemoryOutputStream stream(destData, true);
    stateTree.writeToStream(stream);
}

void VBANReceptorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (sizeInBytes > 0)
    {
        apvts.getParameter("onoff")->setValueNotifyingHost(0.0);
        juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
        juce::ValueTree stateTree = juce::ValueTree::readFromStream(stream);
        if (stateTree.isValid()) apvts.replaceState(stateTree);
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VBANReceptorAudioProcessor();
}

//==============================================================================
