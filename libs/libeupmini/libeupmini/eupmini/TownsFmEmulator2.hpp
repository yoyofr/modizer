/*
	Replacement for FN emulation originally located in eupplayer_townsEmulator

	In this updated version the original homegrown YM2612 emulation
	is replaced by a standard state-of-the-art emulator (MAME or Nuked-OPN2)
	and the code here just sets the respective audio chip registers.

	A consequence of this approach is that the FM audio streams are no longer
	rendered individually for each channel and then merged at the end in the EUP player
	- since the used OPN2 emulator has already done that.
*/
#ifndef TJH__TOWNSFMEMU2_H
#define TJH__TOWNSFMEMU2_H

#include "MonophonicAudioSynthesizer.hpp"


class TownsFmEmulator2_Operator {
    int _specifiedTotalLevel;

	int _channel;
	int _registerBank;
	int _opIdx;
	int _operatorOffset;

	class TownsFmEmulator2 *_emu;
public:
    TownsFmEmulator2_Operator();
    ~TownsFmEmulator2_Operator();
    void setInstrumentParameter( u_char const *instrument);
    void updateAttenuation();

	void init(TownsFmEmulator2 *emu, int chn, int opIdx);
};

class TownsFmEmulator2 : public EUP_TownsEmulator_MonophonicAudioSynthesizer {
	const u_char * _currentInstrument;

    enum { _numOfOperators = 4 };
    TownsFmEmulator2_Operator *_opr;
    int _chn_volume;
	int _expression;
    int _gateTime;
    int _offVelocity;
    int _note;
    int _pitchBend;	// MIDI-style 14-bit pitch bend (range: +-2 halftones)
    int _frequency;
    int _algorithm;
	int _pan;
	int _sustain;
	int _modulate;

	int _channel;
	int	_registerBank;

	static int _chipInitialized;
private:
	void setSoundchipRegisters();
	int getAttenuation();
public:
    TownsFmEmulator2(int chn);
    ~TownsFmEmulator2();
    void setControlParameter(int control, int value);
    void setInstrumentParameter(u_char const *fmInst, u_char const *pcmInst);
    void velocity(int velo);
    void nextTick(int *outbuf, int buflen);
    void note(int n, int onVelo, int offVelo, int gateTime);
    void pitchBend(int value);
    void recalculateFrequency();

	static int *getSample();

	friend TownsFmEmulator2_Operator;
};


#endif