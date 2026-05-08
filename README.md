# VBANReceptor.vst3

VBAN Receptor VST3 plugin

This is a simple cross-platform VBAN Receptor JUCE based VST3 plugin for Linux, Mac OS and Windows, also AU one for Mac OS and LV2 for Linux.

It can save IP, port, Streamname, format and redundancy in presets of our DAWs (tested on Reaper and Ardour on Linux as VST3, on Reaper on Mac OS as AU and (yes, sound funny) LV2 (!!!) and also on Carla on Linux as LV2.

This is a stereo plugin version. If stream has more than 2 channels, plugin will take 2 firsts. If stream is mono you will hear it on plugin left channel.

Let's test it together!

BUILD:
To build it on Linux just clone this repo, go to Builds/LinuxMakeFile, open terminal there and type "make" It will be places to ~/vst3, just move it to directory what you want (if you want).
To build it on Mac OS use xcodebuild (see its docs or ask GPT).
To build it on/for other platforms just ask GPT how to build basic JUCE project.
OR:
Linux/Mac

mkdir build && cd build
cmake -DCHANNELS=<number> .. # number is of channels (must be from 1 to 64, default 2)
make -j<pnum> # pnum is number of processors you want to use for building

Then you can find plugins in user plugin directories.

Roadmap: to test it on Windows, iOS and even on Android.
