/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "vban_functions.h"

extern std::mutex rbmutex;
//extern int nbinputs;
//extern int nboutputs;
#define nbinputs 2
#define nboutputs 2


class VBANReceptorAudioProcessorEditor;
//==============================================================================

class PlugThread : public juce::Thread
{
public:

  PlugThread(const juce::String& name) : juce::Thread(name)
  {
  }

  ringbuffer_t* getRingBufferPointer()
  {
      return ringbuffer;
  }

  void clearRingBuffer()
  {
      if (ringbuffer!= nullptr)
      {
          ringbuffer_free(ringbuffer);
          if (ringbuffer!= nullptr) ringbuffer = nullptr;
      }
  }

  bool start(char* ip, int* port, char* sn, long* sr, int* nfr, int nbc, int red)
  {
    ipAddr = ip;
    streamname = sn;
    nbchannels = nbc;
    nframes = nfr;
    sampleRate = sr;
    redundancy = red;
    vbanFormatSR = vban_get_format_SR(*sampleRate);
    //rxsocket = new juce::DatagramSocket;
    rxsocket.reset(new juce::DatagramSocket);
    if (rxsocket->bindToPort(*port))
    {
      fprintf(stderr, "UDP port %d successfully open\r\n", *port);
      startThread();
      return true;
    }
    else
    {
      fprintf(stderr, "Failed to open UDP port %d\r\n", *port);
      rxsocket->shutdown();
      rxsocket = nullptr;
      //delete (rxsocket);
      return false;
    }
  }

protected:

  void run() override
  {
    static VBanPacket infopacket;
    int index;
    int pktlen;
    juce::String senderIPAddress;
    juce::String infoString;
    int senderPortNumber;
    int writecnt;

    fprintf(stderr, "RX thread started...\r\n");
    while (!threadShouldExit())
    {
        while (rxsocket->waitUntilReady(true, 0))
        {
          memset(&rxPacket, 0, VBAN_PROTOCOL_MAX_SIZE);
          pktlen = rxsocket->read(&rxPacket, VBAN_PROTOCOL_MAX_SIZE, false, senderIPAddress, senderPortNumber);
          if ((pktlen>VBAN_HEADER_SIZE)&&(rxPacket.header.vban==VBAN_HEADER_FOURC))
          {
              switch (rxPacket.header.format_SR&VBAN_PROTOCOL_MASK)
              {
              case VBAN_PROTOCOL_AUDIO:
                  // AUDIO
                  if ((streamname[0]==0)||(ipAddr[0]=='0')||(ipAddr[0]==0)) // No stream receiving
                  {
                      if ((ipAddr[0]=='0')||(ipAddr[0]==0))  // no IP setted
                      {
                          strncpy(ipAddr, senderIPAddress.toRawUTF8(), strlen(senderIPAddress.toRawUTF8()));
                          fprintf(stderr, "RX ip %s\r\n", ipAddr);
                          //refreshIPAddressParametersFromText(ipAddr);
                      }
                      if (streamname[0]==0)  //no SN setted
                      {
                          memcpy(streamname, rxPacket.header.streamname, 16);
                          fprintf(stderr, "SN ip %s\r\n", streamname);
                      }
                      //fprintf(stderr, "%s %s\r\n%s %s\r\n%d %d\r\n%d %d\r\n\r\n", rxPacket.header.streamname, streamname, ipAddr, senderIPAddress.toRawUTF8(), rxPacket.header.format_SR, vbanFormatSR, rxPacket.header.nuFrame, Frame);
                      /*// TODO: Rework for Resampler //*/
                  }
                  if ((memcmp(rxPacket.header.streamname, streamname, strlen(rxPacket.header.streamname))==0)&&
                      (memcmp(ipAddr, senderIPAddress.toRawUTF8(), strlen(senderIPAddress.toRawUTF8()))==0)&&
                      (rxPacket.header.nuFrame!= nuFrame)&&
                      ((rxPacket.header.format_SR&VBAN_SR_MASK)==vbanFormatSR))
                  {
                      //fprintf(stderr, "%d bytes received\r\n", pktlen);
                      nuFrame = rxPacket.header.nuFrame;
                      if (ringbuffer==nullptr) // still no stream receiving
                      {
                          if (framebuf!= nullptr) free(framebuf);
                          framebuf = (float*)malloc(nbchannels*sizeof(float));
                          fprintf(stderr, "Creating ringbuffer... %d\r\n", redundancy);
                          rblen = 2 * nbchannels * sizeof(float) * (1 + redundancy) * (rxPacket.header.format_nbs + 1 > *nframes ? rxPacket.header.format_nbs + 1 : *nframes);
                          ringbuffer = ringbuffer_create(rblen);
                          memset(ringbuffer->buf, 0, rblen);
                          switch (redundancy)
                          {
                            case 0:
                              break;
                            case 1:
                              ringbuffer_write_advance(ringbuffer, rblen/4);
                              ringbuffer_read_advance(ringbuffer, rblen/4);
                              break;
                            default:
                              ringbuffer_write_advance(ringbuffer, rblen/2);
                              ringbuffer_read_advance(ringbuffer, rblen/2);
                              break;
                          }
                      }
                      else
                      {
                          packetFrameSize = ((rxPacket.header.format_nbc + 1)*VBanBitResolutionSize[rxPacket.header.format_bit]);
                          int nbc = (nbchannels < (rxPacket.header.format_nbc + 1) ? nbchannels : rxPacket.header.format_nbc + 1);
                          //fprintf(stderr, "%u %u %u %u %u space\r\n", rblen, ringbuffer->size, ringbuffer_write_space(ringbuffer), packetFrameSize, nbchannels);
                          writecnt = 0;
                          for (int frame = 0; frame <= (rxPacket.header.format_nbs); frame++)
                          {
                              vban_sample_convert(framebuf, VBAN_BITFMT_32_FLOAT, &rxPacket.data[frame*packetFrameSize], rxPacket.header.format_bit, nbchannels);
                              rbmutex.lock();
                              if (ringbuffer_write_space(ringbuffer) >= (nbchannels*sizeof(float)))
                              {
                                  ringbuffer_write(ringbuffer, (char*)framebuf, nbc*sizeof(float));
                                  writecnt++;
                                  //fprintf(stderr, "%d bytes received\r\n", pktlen);
                              }
                              rbmutex.unlock();
                          }
                        //fprintf(stderr, "%d frames written\r\n", writecnt);
                      }
                  }
                    // TODO AUDIO
                  break;
                  case VBAN_PROTOCOL_SERIAL:
                    // MIDI
                    // TODO MIDI
                  break;
                  case VBAN_PROTOCOL_TXT:
                      // TEXT
                      fprintf(stderr, "Info request from %s:%d\n", senderIPAddress.toRawUTF8(), senderPortNumber);
                      if ((memcmp(rxPacket.header.streamname, "INFO", 4)==0)&&((memcmp(rxPacket.data, "/info", 5)==0)||(memcmp(rxPacket.data, "/INFO", 5)==0)))
                      {
                          memset(&infopacket, 0, VBAN_PROTOCOL_MAX_SIZE);
                          infopacket.header.vban = VBAN_HEADER_FOURC;
                          infopacket.header.format_SR = VBAN_PROTOCOL_TXT;
                          strcpy(infopacket.header.streamname, "INFO");
                          sprintf(infopacket.data, "VST3_Receptor streamname=%s nboutputs=%d", streamname, nbchannels);
                          rxsocket->write(senderIPAddress, senderPortNumber, &infopacket, VBAN_HEADER_SIZE + strlen(infopacket.data));
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
    rxsocket->shutdown();
    if (framebuf!= nullptr) free(framebuf);
    if (ringbuffer!= nullptr) clearRingBuffer();
    if (rxsocket!= nullptr)
    {
      rxsocket = nullptr;
      fprintf(stderr, "RX thread stopped...\r\n");
    }
  }

private:

  VBanPacket rxPacket;
  int packetFrameSize;
  ringbuffer_t* ringbuffer = nullptr;
  int rblen = 0;
  char* ipAddr;
  char* streamname;
  long* sampleRate;
  int nbchannels;
  uint8_t vbanFormatSR;
  int* nframes;
  int redundancy = 0;
  int nuFrame = 0;
  float* framebuf = nullptr;
  //juce::DatagramSocket* rxsocket;
  std::unique_ptr<juce::DatagramSocket>rxsocket;

  void threadWork()
  {
  }
};
/**
 */
class VBANReceptorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    VBANReceptorAudioProcessor();
    ~VBANReceptorAudioProcessor() override;

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

    int refreshStreamNameTextFromParameters(char* text, int textlen);
    void refreshStreamNameParametersFromText(const char* text, int textlen);
    int refreshIPAddressTextFromParameters(char* text);
    void refreshIPAddressParametersFromText(const char* text);
    int refreshPortTextFromParameters(char* text);
    int refreshPortParametersFromText(const char* text);

    juce::AudioProcessorValueTreeState parameters;
    bool cleanup = false;
    VBANReceptorAudioProcessorEditor* editor;

private:
    //==============================================================================

    std::unique_ptr<PlugThread>(rxThread);
    float* inputChannelData[nbinputs];
    float* outputChannelData[nboutputs];
    ringbuffer_t* ringbuffer = NULL;
    bool onoff = false;
    bool onoffCurrent = false;
    long hostSamplerate = 48000;
    int nframes;
    int redundancy;
    uint8_t vbanFormatSR = 4;
    char ipAddr[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int udpPort = 6980;
    char streamname[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int lostPackets = 0;
    int lostFrames = 0;
    int lostSamples = 0;
    int bufReadSpace = 0;
    float gain;
    bool pluckingEnabled = true;
    bool pluckingOn = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VBANReceptorAudioProcessor)
};
