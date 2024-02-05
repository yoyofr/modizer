/*
	Below garbage is from "eupmini" implementation (was in eupplayer_townsEmulator.hpp)
		
	fixme: cleanup that pcm_struct dumbfuckery
*/
#ifndef TJH__EUP_AUDIOOUT_H
#define TJH__EUP_AUDIOOUT_H

/*
 * global data
 *
 */
const int streamAudioSampleOctectSize = 2;
/* rate 44100 Hz stream */
const int streamAudioRate = 44100;
const int streamAudioSamplesBlock = 512;
const int streamAudioChannelsNum = 2;
const int streamAudioSamplesBlockNum = 16;
const int streamAudioChannelsSamplesBlock = streamAudioSamplesBlock * streamAudioChannelsNum;
const int streamAudioBufferSamples = streamAudioChannelsSamplesBlock * streamAudioSamplesBlockNum;
const int streamAudioBufferOctectsSize = streamAudioBufferSamples * streamAudioSampleOctectSize;
const int streamBytesPerSecond = streamAudioRate * streamAudioChannelsNum * streamAudioSampleOctectSize;

struct pcm_struct {
    unsigned char on;
    int stop;

    int write_pos;
    int read_pos;

    int count;

    unsigned char buffer[streamAudioBufferOctectsSize];
};


#endif