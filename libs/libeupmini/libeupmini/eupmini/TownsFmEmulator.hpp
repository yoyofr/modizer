/*
	Old FN emulation originally located in eupplayer_townsEmulator
	
	Though unused by default, this old impl can still be activated 
	using a define USE_ORIG_OPL2_IMPL. (Might be useful for comparisons.)
*/
#ifndef TJH__TOWNSFMEMU_H
#define TJH__TOWNSFMEMU_H

#include "MonophonicAudioSynthesizer.hpp"


class TownsFmEmulator_Operator {
    enum State { _s_ready, _s_attacking, _s_decaying, _s_sustaining, _s_releasing };
    State _state;
    State _oldState;
    int64_t _currentLevel;
    int _frequency;
    int _phase;		
    int _lastOutput;
    int _feedbackLevel;
    int _detune;
    int _multiple;
    int64_t _totalLevel;
    int _keyScale;
    int _velocity;
    int _specifiedTotalLevel;		
    int _specifiedAttackRate;
    int _specifiedDecayRate;
    int _specifiedSustainLevel;
    int _specifiedSustainRate;
    int _specifiedReleaseRate;
    int _tickCount;
    int _attackTime;
    // int64_t _attackRate;
    int64_t _decayRate;
    int64_t _sustainLevel;
    int64_t _sustainRate;
    int64_t _releaseRate;
public:
    TownsFmEmulator_Operator();
    ~TownsFmEmulator_Operator();
    void feedbackLevel(int level);
    void setInstrumentParameter(u_char const *instrument);
    void velocity(int velo);
    void keyOn();
    void keyOff();
    void frequency(int freq);
    int nextTick(int rate, int phaseShift);
};

class TownsFmEmulator : public EUP_TownsEmulator_MonophonicAudioSynthesizer {
    enum { _numOfOperators = 4 };
    TownsFmEmulator_Operator *_opr;
    int _chn_volume;
	int _expression;
    int _gateTime;
    int _offVelocity;
    int _note;
    int _frequencyOffs;
    int _frequency;
    int _algorithm;
	int _enableL;
	int _enableR;
	
public:
    TownsFmEmulator();
    ~TownsFmEmulator();
    void setControlParameter(int control, int value);
    void setInstrumentParameter(u_char const *fmInst, u_char const *pcmInst);
    int velocity()
    {
        return EUP_TownsEmulator_MonophonicAudioSynthesizer::velocity();
    }
    void velocity(int velo);
    void nextTick(int *outbuf, int buflen);
    void note(int n, int onVelo, int offVelo, int gateTime);
    void pitchBend(int value);
    void recalculateFrequency();
};


#endif