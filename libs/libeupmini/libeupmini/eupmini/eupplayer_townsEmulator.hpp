// $Id: eupplayer_townsEmulator.h,v 1.13 2000/04/12 23:12:51 hayasaka Exp $

#ifndef TJH__EUP_TOWNSEMULATOR_H
#define TJH__EUP_TOWNSEMULATOR_H

#if defined ( __GNUC__ )
#include <sys/time.h>
#endif // __GNUC__
#include <sys/types.h>
#include <iostream>

#include "eupplayer_towns.hpp"
#include "MonophonicAudioSynthesizer.hpp"

#include "audioout.hpp"


extern struct pcm_struct eup_pcm;

class TownsPcmInstrument;
class TownsPcmSound;
class TownsPcmEnvelope;


class TownsPcmEmulator : public EUP_TownsEmulator_MonophonicAudioSynthesizer {
    int _control7;
	int _expression;
    int _envTick;
    int _currentLevel;
    int _gateTime;
    int _offVelocity;
    int _note;
    int _frequencyOffs;
    uint64_t _phase;
	int _volL;
	int _volR;

    TownsPcmInstrument const *_currentInstrument;
    TownsPcmSound const *_currentSound;
    TownsPcmEnvelope *_currentEnvelope;
public:
    TownsPcmEmulator();
    ~TownsPcmEmulator();
    void setControlParameter(int control, int value);
    void setInstrumentParameter(u_char const *fmInst, u_char const *pcmInst);
    void nextTick(int *outbuf, int buflen);
    void note(int n, int onVelo, int offVelo, int gateTime);
    void pitchBend(int value);
};

class EUP_TownsEmulator_Channel {
    enum { _maxDevices = 16 };
    EUP_TownsEmulator_MonophonicAudioSynthesizer *_dev[_maxDevices+1];
    int _lastNotedDeviceNum;
public:
    EUP_TownsEmulator_Channel();
    ~EUP_TownsEmulator_Channel();
    void add(EUP_TownsEmulator_MonophonicAudioSynthesizer *device);
    void note(int note, int onVelo, int offVelo, int gateTime);
    void setControlParameter(int control, int value);
    void setInstrumentParameter(u_char const *fmInst, u_char const *pcmInst);
    void pitchBend(int value);
    void nextTick(int *outbuf, int buflen);
    void rate(int r);
};

class EUP_TownsEmulator : public TownsAudioDevice {
    enum { _maxChannelNum = 16,
           _maxFmInstrumentNum = 128,
           _maxPcmInstrumentNum = 32,
           _maxPcmSoundNum = 128,
         };
    EUP_TownsEmulator_Channel *_channel[_maxChannelNum];
    bool _enabled[_maxChannelNum];
    u_char _fmInstrumentData[8 + 48*_maxFmInstrumentNum];
    u_char *_fmInstrument[_maxFmInstrumentNum];	// pointers into above _fmInstrumentData buffer
    TownsPcmInstrument *_pcmInstrument[_maxPcmInstrumentNum];
    TownsPcmSound *_pcmSound[_maxPcmSoundNum];
    int _rate;

	int *_mixBuf;
	int _mixBufLen; 	// in ticks
	int *getMixBuffer(int ticks);
public:
    EUP_TownsEmulator();
    ~EUP_TownsEmulator();
    void assignFmDeviceToChannel(int eupChannel, int fmChannel);
    void assignPcmDeviceToChannel(int channel);
    void setFmInstrumentParameter(int num, u_char const *instrument);
    void setPcmInstrumentParameters(u_char const *instrument, size_t size);
    void outputStream(FILE *ostr);
    FILE * outputStream_get();
    void nextTick(bool nosound=false);
    void enable(int ch, bool en=true);
    void note(int channel, int n,
              int onVelo, int offVelo, int gateTime);
    void pitchBend(int channel, int value);
    void controlChange(int channel, int control, int value);
    void programChange(int channel, int num);
    void rate(int r);
};

#endif
