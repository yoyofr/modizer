#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#include "TownsFmEmulator2.hpp"


extern "C" {
	#include "opn2.h"
}

// put these here to avoid stupidly unnecessary dependencies in header file
// (C++ is such garbage with regard to hiding private implementation details)
static int _buffer[2];
static stream_sample_t*  _outputs[2];



static int OPERATOR_OFFSET[4] = {0,8,4,12};	// register order != operator order; 2 & 3 swapped!

static u_char CHN_MAP[6] = {0,1,2, 4,5,6};

static bool IS_OUTPUT_OPERATOR[8][4] = {
	{ false, false, false,  true, }, // 0
	{ false, false, false,  true, }, // 1
	{ false, false, false,  true, }, // 2
	{ false, false, false,  true, }, // 3
	{ false,  true, false,  true, }, // 4
	{ false,  true,  true,  true, }, // 5
	{ false,  true,  true,  true, }, // 6
	{  true,  true,  true,  true, }, // 7
};

/* TownsFmEmulator2_Operator */

TownsFmEmulator2_Operator::TownsFmEmulator2_Operator()
{
	// TL is specifid per operator (also non-output operators),
	// i.e. it also defines the flavor of the note

    _specifiedTotalLevel = 127;	// i.e. the smallest level
}

void TownsFmEmulator2_Operator::init(TownsFmEmulator2 *emu, int chn, int opIdx)
{
	_emu = emu;
	_channel= chn;
	_registerBank = _channel / 3;
	_opIdx = opIdx;
	_operatorOffset = OPERATOR_OFFSET[opIdx] + (_channel % 3);
}

TownsFmEmulator2_Operator::~TownsFmEmulator2_Operator()
{
}

void TownsFmEmulator2_Operator::updateAttenuation()
{
//    _attenuation = attenuation;

	// "To make a note softer, only change the TL of the slots (the output operators).
	// Changing the other operators will affect the flavor of the note."

	int usedAttenuation = (IS_OUTPUT_OPERATOR[_emu->_algorithm][_opIdx]) ? _emu->getAttenuation() : 127;

    int64_t totalLevel = (((int64_t)_specifiedTotalLevel << 31) +
                   ((int64_t)(127-usedAttenuation) << 29));				// let's hope this original impl makes any sense

	// TL (total level) represents the envelope's highest amplitude,
	// with 0 being the largest and 127 the smallest

	writeOPN2Reg(_registerBank, 0x40 + _operatorOffset, totalLevel >> 31);
}


void TownsFmEmulator2_Operator::setInstrumentParameter( u_char const *instrument)
{
//	_algorithm = algorithm;

	// note: the "instrument" buffer contains interleaved data of 4 operators
	// (see how this function is called accordingly..)

	// these seem to map 1:1 to chip registers:

	// 30H+ 3bits detune + 4 bits multiple (for three channels × four operators)
	writeOPN2Reg(_registerBank, 0x30 + _operatorOffset, instrument[8]);


	// 40H+ TL (total level, aka volume)
    _specifiedTotalLevel = instrument[12] & 127;
    this->updateAttenuation();


	// 50H+ - rate scaling; attack rate
	writeOPN2Reg(_registerBank, 0x50 + _operatorOffset, instrument[16] & 0xdf);	// mask unused bit 5 just in case?


	// 60H+ DR and AM enable	(AM enable handling (bit 7) was absent on original emu)

		// todo: find testcase: AM enable would also rely on corresponding
		// 22H and B4H to be setup properly (no source for those).
		// testcase: "Help with farm work" uses "Modulate" command

	writeOPN2Reg(_registerBank, 0x60 + _operatorOffset, _emu->_modulate ? (0x80 | (instrument[20] & 31)) : (instrument[20] & 31));


	// 70H+ SR (5-bit)
	writeOPN2Reg(_registerBank, 0x70 + _operatorOffset, instrument[24] & 31 );


	// 80H+ RR (release rate) & SL (sustain level) => 4-bit each
	writeOPN2Reg(_registerBank, 0x80 + _operatorOffset, _emu->_sustain ? (instrument[28] & 0xf0) : instrument[28]);
}



/* TownsFmEmulator2 */

int TownsFmEmulator2::_chipInitialized = 0;

TownsFmEmulator2::TownsFmEmulator2(int channel)
{
	if (!_chipInitialized) {
		_outputs[0] = &_buffer[0];
		_outputs[1] = &_buffer[1];

		initOPN2();

		_chipInitialized = 1;
	}
	_currentInstrument = NULL;

	_channel = channel;
    _registerBank = channel / 3;

    _chn_volume = 100;	// power-on reset value (100) is defined in midi spec

	_expression = 127;	// specs talk of percentage but range is 0..127!? (testcase: "Passionate Ocean / Fox")
    _offVelocity = 0;
    _gateTime = 0;
    _note = 40;
    _pitchBend = 0x2000;
    _algorithm = 7;
	_pan = 0xc0; 			// enable output to L/R
	_sustain = 0;
	_modulate = 0;

	_opr = new TownsFmEmulator2_Operator[_numOfOperators];
	for (int i= 0; i<_numOfOperators ; i++) {
		_opr[i].init(this, channel, i);
	}
}

TownsFmEmulator2::~TownsFmEmulator2()
{
	if (_chipInitialized) {
		device_stop_ym2612();
		_chipInitialized = 0;
	}

    delete [] _opr;
}

int TownsFmEmulator2::getAttenuation() {
	int v = EUP_TownsEmulator_MonophonicAudioSynthesizer::velocity();
//    int a = v + (_chn_volume - 127) * 4 * _expression / 127;	save the trouble until we find a testcase
//    int a = v + (_chn_volume - 127) * 4;
//	int a = (v * v * _chn_volume * _chn_volume) / (127 * 127 * 127);	// Tomoaki's experiment
	int a = (v * v * _chn_volume) / (127 * 127);	// with Tomoaki's experiment "F.Couperin Mysterious Barrier S-up" gets too loud

	return a;
}

void TownsFmEmulator2::velocity(int velo)
{
	// velocity seems to be a EUP/MIDI specific concept - that
	// has to be mapped to actual chip registers

    EUP_TownsEmulator_MonophonicAudioSynthesizer::velocity(velo);

	for (int opr = 0; opr < 4; opr++) {
		_opr[opr].updateAttenuation();
	}
}

void TownsFmEmulator2::setControlParameter(int control, int value)
{
    switch (control) {
	 case 0: 					// Bank Select (for devices with more than 128 programs)
		// base for "program change" commands
		if (value > 0)
			fprintf(stderr, "warning: unsupported Bank Select: %d\n", value);

		break;
	case 1: 					// Modulation controls a vibrato effect (pitch, loudness, brighness)

		// test-case: "Just Lucky" and "Help with Farm Work" use this
		_modulate = value;
		break;
    case 7:
        _chn_volume = value;

//		fprintf(stderr, "volume change; %d\n", value);
        this->velocity(EUP_TownsEmulator_MonophonicAudioSynthesizer::velocity());
        break;
    case 10:
        // panpot: this seems to be a MIDI type, coarse, channel specific pan setting...
		// (range 0 - 7f; with $40 center) there are songs that use this on FM channels
		// but it is unclear what for - since chip only has binary L/R enable toggle

		_pan= (value < 0x20) ? 0x80 : ((value < 0x60) ? 0xc0 : 0x40);
//		fprintf(stderr, "pan change; %d\n", value);

        break;
	case 11: {				// MIDI: Expression  - testcase; Passionate Ocean
		// Expression is a percentage of volume (CC7).
		_expression= value;
        this->velocity(EUP_TownsEmulator_MonophonicAudioSynthesizer::velocity());

		if(value != 127)
			fprintf(stderr, "warning: song uses unimplemented Expression control\n");
		break;
	}
	case 64: 					// Sustain Pedal (on/off) testcase: Hara.eup

		_sustain = value > 63;	// should a new note reset this? probably the pedal should be explicitly released..
		break;
    default:
        fprintf(stderr, "TownsFmEmulator2::setControlParameter: unknown control %d, val=%d\n", control, value);
        fflush(stderr);
        break;
    };
}

void TownsFmEmulator2::setSoundchipRegisters() {
	if (_currentInstrument == NULL) {
		fprintf(stderr, "warning: instrument config is missing!\n");
		return;
	}

	// reminder: eventhough the instrument[33] seems to contain panning info
	// it is a bad idea to actually use it.. testcase: "Let's hang out on the hay"

	int ams_pms = 0;		// disabled
	if (_modulate) {
		// testcase: "Help with farm work / H.Imoto" - used range is 3..127 but it is totally unclear
		// what the EUP specific behavior should be: modulate AMS only, FMS only?, something else entirely?
		// what default to use in 0x22? is the _modulate value using regular MIDI "more/less" semantics
		// or is it already a register-content to be used directly?

		// this is probably incorrect.. so please fix if you know the correct answer:

		ams_pms |= (_modulate >> 4) << 3;	// use proportionally for AMS (3 bit)
		ams_pms |= (_modulate >> 5); 		// use proportionally for FMS (2 bit)
	}

	// this is shared by all channels (needed to permanently enable potential modulation)..
	// so whenever one channel wants to use modulation it must be enabled:
	// use ams_pms to enable/disable for specific channels

	writeOPN2Reg(0, 0x22, 0x08);	// the question is what default frequency to use? 08..0f (can't say that I'd hear any difference..)


	writeOPN2Reg(_registerBank, 0xb4 + (_channel % 3), _pan | ams_pms);

    _algorithm = _currentInstrument[32] & 7;

	// set the algorithm and feedback in the respective channel's regs.
	writeOPN2Reg(_registerBank, 0xb0 + (_channel % 3), _currentInstrument[32]);


	// set the operator specific settings of the "instrument"
    _opr[0].setInstrumentParameter(_currentInstrument + 0);
    _opr[1].setInstrumentParameter(_currentInstrument + 2);
    _opr[2].setInstrumentParameter(_currentInstrument + 1);
    _opr[3].setInstrumentParameter(_currentInstrument + 3);
}

void TownsFmEmulator2::setInstrumentParameter(u_char const *fmInst,
        u_char const *pcmInst)
{
	// this my be reset during playback via "program change"

    u_char const *instrument = fmInst;

    if (instrument == NULL) {
		_currentInstrument = NULL;

        fprintf(stderr, "%s@%p: can not set null instrument\n",
                "TownsFmEmulator2::setInstrumentParameter", this);
        fflush(stderr);
        return;
    }

	// since there are multiple emulator instances for the same chip channel,
	// registers should only be updates when an instance actually plays a note
	_currentInstrument = instrument;
}


void TownsFmEmulator2::nextTick(int *outbuf, int buflen)
{
	// this is a per-channel API but since emulator delivers the overall
	// result, no audio data is handled here (unlike old or PCM impl)

    // steptime advance one step

    if (_gateTime > 0) {
        if (--_gateTime <= 0) {
            this->velocity(_offVelocity);

			// each of the "player channels" has its own instances
			// of the 6 FM emulators, i.e. there are 16 FM emu instances
			// competing for each physical chip channel!
			// (each of which is ticked..). if multiple of these actively
			// manipulate the same chip channel, garbage will be the result.

			// it is unclear it the player impl effectively prevents this

			writeOPN2Reg(0, 0x28, 0x00 | CHN_MAP[_channel]);	// key-off
        }
    }

    if (EUP_TownsEmulator_MonophonicAudioSynthesizer::velocity() == 0) {
		// original impl effectively muted the output here (any testcase for this?)

		// in principle the channel could be muted by disabling L/R output
//		writeOPN2Reg(_registerBank, 0xb4 + (_channel % 3), 0);
        return;
    }
}

void TownsFmEmulator2::note(int n, int onVelo, int offVelo, int gateTime)
{
    if (_currentInstrument == NULL) return;

	// note: these commands are copy/paste from MIDI - check MIDI specs for details!
	// velocity: how hard a key is struck; influencing loudness; range 0..127

    this->velocity(onVelo);
    _offVelocity = offVelo;

    _gateTime = gateTime;		// duration the note is played before "release"

    _note = n;
    this->recalculateFrequency();


	// if channel ever gets muted in nextTick() then we might add this:
//	writeOPN2Reg(_registerBank, 0xb4 + (_channel % 3), _pan | ams_pms );

	setSoundchipRegisters();

	// key-on: turn on all Operators at once
	writeOPN2Reg(0, 0x28, 0xf0 | CHN_MAP[_channel]);
}

void TownsFmEmulator2::pitchBend(int value)
{
	// note: a MIDI instrument stays pitch-bend until it is specifically reset
	// (it is unclear if only new notes should be affected - but not notes that already play)
    _pitchBend = value;
    this->recalculateFrequency();
}

void TownsFmEmulator2::recalculateFrequency()
{
    int const fnum[] = {	// base used by original eupplayer impl for table generation
      0x026a, 0x028f, 0x02b6, 0x02df,
      0x030b, 0x0339, 0x036a, 0x039e,
      0x03d5, 0x0410, 0x044e, 0x048f,
    };

	// pretty close to the respective table from MSV spec:
	// 0x026b, 0x028e, 0x02b5, 0x02de
	// 0x030a, 0x0338, 0x0369, 0x039d
	// 0x03d4, 0x040e, 0x044c, 0x048d

	int note = _note - 8;	// this seems to be the correct offset (compared to old impl output)
    int block = note / 12;

	// note: there seems to be enough range left at block range borders
	// to allow bendiung without having to change blocks..

	double bend = ((double)_pitchBend - 0x2000) / 0x2000;	// -1..1
	bend = pow(1.059327523542, 2*bend);		// factor (halftone step) tuned to above table

	int freq= fnum[note % 12] * bend;

	// set frequency and "octave" registers
	int offset = _channel % 3;
	writeOPN2Reg(_registerBank, 0xa4 + offset, ((freq>>8) & 0x7) | (block<<3));
	writeOPN2Reg(_registerBank, 0xa0 + offset,  freq & 0xff);
}


int *TownsFmEmulator2::getSample() {
	ym2612_stream_update(_outputs, 1);
	return _buffer;
}




