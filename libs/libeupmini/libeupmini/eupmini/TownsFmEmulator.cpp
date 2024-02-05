/*
	reminder: CH3/6 "special mode" (see reg 27H) isn't implemented
	          in eupplayer.. should it be?

*/
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#include "TownsFmEmulator.hpp"

#include "sintbl.hpp"


/* TownsFmEmulator_Operator */

TownsFmEmulator_Operator::TownsFmEmulator_Operator()
{
    _state = _s_ready;
    _oldState = _s_ready;
    _currentLevel = ((int64_t)0x7f << 31);
    _phase = 0;
    _lastOutput = 0;
    _feedbackLevel = 0;
    _detune = 0;
    _multiple = 1;
    _specifiedTotalLevel = 127;
    _keyScale = 0;
    _specifiedAttackRate = 0;
    _specifiedDecayRate = 0;
    _specifiedSustainRate = 0;
    _specifiedReleaseRate = 15;
    this->velocity(0);
}

TownsFmEmulator_Operator::~TownsFmEmulator_Operator()
{
}

void TownsFmEmulator_Operator::velocity(int velo)
{
    _velocity = velo;
    _totalLevel = (((int64_t)_specifiedTotalLevel << 31) +
                   ((int64_t)(127-_velocity) << 29));
    _sustainLevel = ((int64_t)_specifiedSustainLevel << (31+2));
}

void TownsFmEmulator_Operator::feedbackLevel(int level)
{
    _feedbackLevel = level;
}

void TownsFmEmulator_Operator::setInstrumentParameter(u_char const *instrument)
{
    _detune = (instrument[8] >> 4) & 7;
    _multiple = instrument[8] & 15;
    _specifiedTotalLevel = instrument[12] & 127;
    _keyScale = (instrument[16] >> 6) & 3;
    _specifiedAttackRate = instrument[16] & 31;
    _specifiedDecayRate = instrument[20] & 31;
    _specifiedSustainRate = instrument[24] & 31;
    _specifiedSustainLevel = (instrument[28] >> 4) & 15;
    _specifiedReleaseRate = instrument[28] & 15;

    _state = _s_ready; // What about the real thing?
    this->velocity(_velocity);
}

void TownsFmEmulator_Operator::keyOn()
{
    _state = _s_attacking;
    _tickCount = 0;
    _phase = 0; // hello, it actually looks like this
    _currentLevel = ((int64_t)0x7f << 31); // This also looks like this
}

void TownsFmEmulator_Operator::keyOff()
{
    if (_state != _s_ready) {
        _state = _s_releasing;
    }
}

void TownsFmEmulator_Operator::frequency(int freq)
{
    int r;

    //DB(("freq=%d=%d[Hz]\n", freq, freq/262205));
    _frequency = freq;

    r = _specifiedAttackRate;
    if (r != 0) {
        r = r * 2 + (keyscaleTable[freq/262205] >> (3-_keyScale));
        if (r >= 64) {
            r = 63; // I think I should (red p.207)
        }
    }
#if 0
    _attackRate = 0x80;
    _attackRate *= powtbl[(r&3) << 7];
    _attackRate <<= 16 + (r >> 2);
    _attackRate >>= 1;
    _attackRate /= 9; // 0-96db takes 8970.24ms when r == 4
    //_attackRate /= 4; // 0-96db takes 8970.24ms when r == 4
    //DB(("AR=%d->%d, 0-96[db]=%d[ms]\n", _specifiedAttackRate, r, (((int64_t)0x80<<31) * 1000) / _attackRate));
#else
    {
        r = 63 - r;
        int64_t t;
        if (_specifiedTotalLevel >= 128) {
            t = 0;
        }
        else {
            t = powtbl[(r&3) << 7];
            t <<= (r >> 2);
            t *= 41; // 0-96db takes 8970.24ms when r == 4
            t >>= (15 + 5);
            t *= 127 - _specifiedTotalLevel;
            t /= 127;
        }
        _attackTime = t; // 1 second == (1 << 12)
        //DB(("AR=%d->%d, 0-96[db]=%d[ms]\n", _specifiedAttackRate, r, (int)((t*1000)>>12)));
    }
#endif

    r = _specifiedDecayRate;
    if (r != 0) {
        r = r * 2 + (keyscaleTable[freq/262205] >> (3-_keyScale));
        if (r >= 64) {
            r = 63;
        }
    }
    //DB(("DR=%d->%d\n", _specifiedDecayRate, r));
    _decayRate = 0x80;
    _decayRate *= powtbl[(r&3) << 7];
    _decayRate <<= 16 + (r >> 2);
    _decayRate >>= 1;
    _decayRate /= 124; // 0-96db is 123985.92ms when r == 4

    r = _specifiedSustainRate;
    if (r != 0) {
        r = r * 2 + (keyscaleTable[freq/262205] >> (3-_keyScale));
        if (r >= 64) {
            r = 63;
        }
    }
    //DB(("SR=%d->%d\n", _specifiedSustainRate, r));
    _sustainRate = 0x80;
    _sustainRate *= powtbl[(r&3) << 7];
    _sustainRate <<= 16 + (r >> 2);
    _sustainRate >>= 1;
    _sustainRate /= 124;

    r = _specifiedReleaseRate;
    if (r != 0) {
        r = r * 2 + 1; // I don't know if this timing is right
        r = r * 2 + (keyscaleTable[freq/262205] >> (3-_keyScale));
        // KS It seems that there is a correction by Although it is not described in red p.206.
        if (r >= 64) {
            r = 63;
        }
    }
    //DB(("RR=%d->%d\n", _specifiedReleaseRate, r));
    _releaseRate = 0x80;
    _releaseRate *= powtbl[(r&3) << 7];
    _releaseRate <<= 16 + (r >> 2);
    _releaseRate >>= 1;
    _releaseRate /= 124;
}

int TownsFmEmulator_Operator::nextTick(int rate, int phaseShift)
{
    // sampling move forward one

    if (_oldState != _state) {
        //DB(("state %d -> %d\n", _oldState, _state));
        //DB(("(currentLevel = %08x %08x)\n", (int)(_currentLevel>>32), (int)(_currentLevel)));
        switch (_oldState) {
        default:
            break;
        };
        _oldState = _state;
    }

    switch (_state) {
    case _s_ready:
        return 0;
        break;
    case _s_attacking:
        ++_tickCount;
        if (_attackTime <= 0) {
            _currentLevel = 0;
            _state = _s_decaying;
        }
        else {
            int i = ((int64_t)_tickCount << (12+10)) / ((int64_t)rate * _attackTime);
            if (i >= 1024) {
                _currentLevel = 0;
                _state = _s_decaying;
            }
            else {
                _currentLevel = attackOut[i];
                _currentLevel <<= 31 - 8;
                //DB(("this=%08x, count=%d, time=%d, i=%d, out=%04x\n", this, _tickCount, _attackTime, i, attackOut[i]));
            }
        }
        break;
    case _s_decaying:
#if 1
        _currentLevel += _decayRate / rate;
        if (_currentLevel >= _sustainLevel) {
            _currentLevel = _sustainLevel;
            _state = _s_sustaining;
        }
#endif
        break;
    case _s_sustaining:
#if 1
        _currentLevel += _sustainRate / rate;
        if (_currentLevel >= ((int64_t)0x7f << 31)) {
            _currentLevel = ((int64_t)0x7f << 31);
            _state = _s_ready;
        }
#endif
        break;
    case _s_releasing:
#if 1
        _currentLevel += _releaseRate / rate;
        if (_currentLevel >= ((int64_t)0x7f << 31)) {
            _currentLevel = ((int64_t)0x7f << 31);
            _state = _s_ready;
        }
#endif
        break;
    default:
        // you shouldn't come here
        break;
    };

    int64_t level = _currentLevel + _totalLevel;
    int64_t output = 0;
    if (level < ((int64_t)0x7f << 31)) {
        int const feedback[] = {
            0,
            0x400 / 16,
            0x400 / 8,
            0x400 / 4,
            0x400 / 2,
            0x400,
            0x400 * 2,
            0x400 * 4,
        };

        _phase &= 0x3ffff;
        phaseShift >>= 2; // What is the correct amount of modulation? 3 is too small and 2 is too large.
        phaseShift += (((int64_t)(_lastOutput) * feedback[_feedbackLevel]) >> 16); // What is the correct amount of modulation? It seems to be somewhere between 16 and 17.
        output = sintbl[((_phase >> 7) + phaseShift) & 0x7ff];
        output >>= (level >> 34); // What is the correct amount of attenuation?
        output *= powtbl[511 - ((level>>25)&511)];
        output >>= 16;
        output >>= 1;

        if (_multiple > 0) {
            _phase += (_frequency * _multiple) / rate;
        }
        else {
            _phase += _frequency / (rate << 1);
        }
    }

    _lastOutput = output;
    return output;
}

/* TownsFmEmulator */

TownsFmEmulator::TownsFmEmulator()
{
    _chn_volume = 127;
	_expression = 127;	// specs talk of percentage but range is 0..127!? fucking idiots! (testcase: "Passionate Ocean / Fox")
    _offVelocity = 0;
    _gateTime = 0;
    _note = 40;
    _frequency = 440;
    _frequencyOffs = 0x2000;
    _algorithm = 7;
    _opr = new TownsFmEmulator_Operator[_numOfOperators];

	_enableL = _enableR = 1;
    this->velocity(0);
}

TownsFmEmulator::~TownsFmEmulator()
{
    delete [] _opr;
}

void TownsFmEmulator::velocity(int velo)
{
    EUP_TownsEmulator_MonophonicAudioSynthesizer::velocity(velo);
#if 0
    int v = (velo * _chn_volume) >> 7; // This is not very accurate
#else
//    int v = velo + (_chn_volume - 127) * 4 * _expression / 127;	save the trouble until we find a testcase
    int v = velo + (_chn_volume - 127) * 4;
#endif
    bool iscarrier[8][4] = {
        { false, false, false,  true, }, /*0*/
        { false, false, false,  true, }, /*1*/
        { false, false, false,  true, }, /*2*/
        { false, false, false,  true, }, /*3*/
        { false,  true, false,  true, }, /*4*/
        { false,  true,  true,  true, }, /*5*/
        { false,  true,  true,  true, }, /*6*/
        {  true,  true,  true,  true, }, /*7*/
    };
    for (int opr = 0; opr < 4; opr++)
        if (iscarrier[_algorithm][opr]) {
            _opr[opr].velocity(v);
        }
        else {
            _opr[opr].velocity(127);
        }
}

void TownsFmEmulator::setControlParameter(int control, int value)
{
	// see https://nickfever.com/music/midi-cc-list for MIDI controls

    switch (control) {
	 case 0: 					// Bank Select (for devices with more than 128 programs)
		// base for "program change" commands
		if (value > 0)
			fprintf(stderr, "warning: unsupported Bank Select: %d\n", value);

		break;
	case 1: 					// Modulation controls a vibrato effect (pitch, loudness, brighness)

		// test-case: HEATED.eup sets this to 71 / 127
		// it is unclear what this should be controlling in the EUP context

		if (value >0)
			fprintf(stderr, "warning: use of unimplemented Modulation: %d\n", value);
		break;
    case 7:						// Control the volume of the channel
        _chn_volume = value;
        this->velocity(this->velocity());
        break;
	// case 8: 					// Balance
    case 10:					// Pan
        // coarse, channel specific pan setting... (range 0 - 7f; with $40 center)
		// there are songs that use this on FM channels but it is unclear what for - since chip only has binary L/R enable toggle


		if (value < 0x20) {
			_enableL = 1;
			_enableR = 0;
		}
		else  if (value < 0x60) {
			_enableL = 1;
			_enableR = 1;
		}
		else {
			_enableL = 0;
			_enableR = 1;
		}
        break;
	case 11: {				// MIDI: Expression  - testcase; Passionate Ocean
		// Expression is a percentage of volume (CC7).
		_expression= value;
        this->velocity(this->velocity());

		if(value != 127)
			fprintf(stderr, "warning: song uses unimplemented Expression control\n");
		break;
	}
	case 64: 					// Sustain Pedal (on/off) testcase: Bach: Aria on G String"
		if (value >0)
			fprintf(stderr, "warning: use of unimplemented Sustain Pedal: %d\n", value);

		break;
    default:
        fprintf(stderr, "TownsFmEmulator::setControlParameter: unknown control %d, val=%d\n", control, value);
        fflush(stderr);
        break;
    };
}

void TownsFmEmulator::setInstrumentParameter(u_char const *fmInst,
        u_char const *pcmInst)
{
    u_char const *instrument = fmInst;

    if (instrument == NULL) {
        fprintf(stderr, "%s@%p: can not set null instrument\n",
                "TownsFmEmulator::setInstrumentParameter", this);
        fflush(stderr);
        return;
    }

	// reminder: eventhough the instrument[33] seems to contain panning info
	// is is a bad idea to actually use it.. testcase: Let's hang out on the hay


    _algorithm = instrument[32] & 7;
    _opr[0].feedbackLevel((instrument[32] >> 3) & 7);
    _opr[1].feedbackLevel(0);
    _opr[2].feedbackLevel(0);
    _opr[3].feedbackLevel(0);
    _opr[0].setInstrumentParameter(instrument + 0);
    _opr[1].setInstrumentParameter(instrument + 2);
    _opr[2].setInstrumentParameter(instrument + 1);
    _opr[3].setInstrumentParameter(instrument + 3);
}

void TownsFmEmulator::nextTick(int *outbuf, int buflen)
{
    // steptime advance one step

    if (_gateTime > 0) {
        if (--_gateTime <= 0) {
            this->velocity(_offVelocity);
            for (int i = 0; i < _numOfOperators; i++) {
                _opr[i].keyOff();
            }
        }
    }
    if (this->velocity() == 0) {
        return;						// testcase for this?
    }

    for (int i = 0, j = 0; i < buflen; i++) {
        int d = 0;
        int d1, d2, d3, d4;
        switch (_algorithm) {
        case 0:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), d1);
            d3 = _opr[2].nextTick(this->rate(), d2);
            d4 = _opr[3].nextTick(this->rate(), d3);
            d = d4;
            break;
        case 1:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), 0);
            d3 = _opr[2].nextTick(this->rate(), d1+d2);
            d4 = _opr[3].nextTick(this->rate(), d3);
            d = d4;
            break;
        case 2:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), 0);
            d3 = _opr[2].nextTick(this->rate(), d2);
            d4 = _opr[3].nextTick(this->rate(), d1+d3);
            d = d4;
            break;
        case 3:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), d1);
            d3 = _opr[2].nextTick(this->rate(), 0);
            d4 = _opr[3].nextTick(this->rate(), d2+d3);
            d = d4;
            break;
        case 4:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), d1);
            d3 = _opr[2].nextTick(this->rate(), 0);
            d4 = _opr[3].nextTick(this->rate(), d3);
            d = d2 + d4;
            break;
        case 5:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), d1);
            d3 = _opr[2].nextTick(this->rate(), d1);
            d4 = _opr[3].nextTick(this->rate(), d1);
            d = d2 + d3 + d4;
            break;
        case 6:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), d1);
            d3 = _opr[2].nextTick(this->rate(), 0);
            d4 = _opr[3].nextTick(this->rate(), 0);
            d = d2 + d3 + d4;
            break;
        case 7:
            d1 = _opr[0].nextTick(this->rate(), 0);
            d2 = _opr[1].nextTick(this->rate(), 0);
            d3 = _opr[2].nextTick(this->rate(), 0);
            d4 = _opr[3].nextTick(this->rate(), 0);
            d = d1 + d2 + d3 + d4;
            break;
        default:
            break;
        };
		// testcase: "Let's hang out on the hay" - enable mutes voices..
        outbuf[j++] += _enableL ? d : 0;
        outbuf[j++] += _enableR ? d : 0;
    }
}

void TownsFmEmulator::note(int n, int onVelo, int offVelo, int gateTime)
{
    _note = n;
    this->velocity(onVelo);
    _offVelocity = offVelo;
    _gateTime = gateTime;
    this->recalculateFrequency();
    for (int i = 0; i < _numOfOperators; i++) {
        _opr[i].keyOn();
    }
}

void TownsFmEmulator::pitchBend(int value)
{
    _frequencyOffs = value;
    this->recalculateFrequency();
}

void TownsFmEmulator::recalculateFrequency()
{
    // It's also different from MIDI...
    // What are the specifications?
    // When I thought about it, this (?) seems to be the correct answer.
    int64_t basefreq = frequencyTable[_note];

	// hack (songs that seem to benefit: Kyou, MiracleMagic, Kyrie)
	double bend= ((double)_frequencyOffs - 0x2000) / 0x2000 * 2;	// in halftones  -2..2

//	int t[12]= {618,655,694,735,779,825,874,926,981,1040,1102,1167};
	double d= 1.0594937478936;	// factor for 1 halftone step in above table
	basefreq*= pow(d, bend);

	// flaw: above bending may interfer with this logic!
	// but since I am using MAME this isn't worth the trouble fixing
    int cfreq = frequencyTable[_note - (_note % 12)];	
	
    int oct = _note / 12;
    int fnum = (basefreq << 13) / cfreq; // Similar to fnum in OPL.


	// issue: judging by affected test songs, this original pitchBend implementatation is
	// garbage, leading to painful audio distortions
	/*
    fnum += _frequencyOffs - 0x2000;
    if (fnum < 0x2000) {
        fnum += 0x2000;
        oct--;
    }
    if (fnum >= 0x4000) {
        fnum -= 0x2000;
        oct++;
    }
	*/

    // _frequency is finally biased 256*1024 times
    _frequency = (frequencyTable[oct*12] * (int64_t)fnum) >> (13 - 10);

    for (int i = 0; i < _numOfOperators; i++) {
        _opr[i].frequency(_frequency);
    }
}