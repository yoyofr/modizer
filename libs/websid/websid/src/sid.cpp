/*
* This emulates the C64's "MOS Technology SID" (see 6581, 8580 or 6582).
*
* It handles the respective SID voices and filters. Envelope generator with
* ADSR-delay bug support, waveform generation including combined waveforms.
* Supports all SID features like ring modulation, hard-sync, etc.
*
* Emulation is performed on a system clock cycle-by-cycle basis.
*
* Anything related to the handling of 'digi samples' is kept separately in digi.c
*
*
* Known limitations:
*
*  - the loudness of filtered voices seems to be too high (i.e. there should
*    probably be more sophisticated mixing logic)
*  - SID output is only calculated at the playback sample-rate and some of the
*    more high-frequency effects may be lost
*  - looking at resids physical HW modelling, there are quite a few effects
*    that are not handled here (non linear behavior, etc).
*
* Credits:
*
*  - TinySid (c) 1999-2012 T. Hinrichs, R. Sinsch (with additional fixes from
*             Markus Gritsch) originally provided the starting point - though
*             very little of that code remains here
*
*  - Hermit's jsSID.js provided a variant of "resid filter implementation", an
*             "anti aliasing" for "pulse" and "saw" waveforms, and a rather
*             neat approach to generate combined waveforms
*             (see http://hermit.sidrip.com/jsSID.html)
*  - Antti Lankila's 6581 distortion page provided valuable inspiration to somewhat
*             replace the 6581 impls that I had used previously.
*
* Links:
*
*  - http://www.waitingforfriday.com/index.php/Commodore_SID_6581_Datasheet
*  - http://www.sidmusic.org/sid/sidtech2.html
*
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

// todo: run performance tests to check how cycle based oversampling fares as compared to
// current once-per-sample approach

// todo: with the added external filter Hermit's antialiasing might no longer make sense
// and the respective handling may be the source of high-pitched noise during PWM/FM digis..

//TODO:  MODIZER changes start / YOYOFR
extern "C" {
#include "../../../../src/ModizerVoicesData.h"
}
//TODO:  MODIZER changes end / YOYOFR


#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "sid.h"
#include "envelope.h"
#include "filter6581.h"
#include "filter8580.h"
#include "digi.h"
#include "wavegenerator.h"
#include "memory_opt.h"

extern "C" {
#include "base.h"
#include "memory.h"
};


// Even when all the SID's voices are unfiltered, the C64's audio-output shows the kind of distortions
// caused by single-time-constant high-pass filters (see
// https://global.oup.com/us/companion.websites/fdscontent/uscompanion/us/static/companion.websites/9780199339136/Appendices/Appendix_E.pdf
// - figure E.14 ). It is caused by the circuitry that post-processes the SID's output inside the C64.
// The below is inspired by resid's analysis - though it is pretty flawed due to the fact that filtering
// is here only performed at the slow output sample rate.

//#define FREQUENCY 1000000 // // could use the real clock frequency here.. but  a fake precision here would be overkill
#ifdef RPI4
#define APPLY_EXTERNAL_FILTER_L(output) \
	/* my RPi4 based device does NOT use an external filter so the respective */\
	/* logic should be disabled when comparing respective output */\
	;
#define APPLY_EXTERNAL_FILTER_R(output) \
	;
#else
#define APPLY_EXTERNAL_FILTER_L(output) \
	/* note: an earlier cycle-by-cycle loop-impl was a performance killer and seems to have been the */\
	/* main reason why some multi-SID songs started to stutter on my old PC.. I therefore switched to */\
	/* the below hack - accepting the flaws as a tradeoff */\
\
	/* cutoff: w0=1/RC  // capacitor: 1 F=1000000 uF; 1 uF= 1000000 pF (i.e. 10e-12 F) */\
/*	const double cutoff_high_pass_ext = ((double)100) / _sample_rate;*/	/* hi-pass: R=1kOhm, C=10uF; i.e. w0=100	=> causes the "saw look" on pulse WFs*/\
/*	const double cutoff_low_pass_ext = ((double)100000) / FREQUENCY;	// lo-pass: R=10kOhm, C=1000pF; i.e. w0=100000  .. no point at low sample rate */\
\
	double out_l = _left_lp_out - _left_hp_out;\
	_left_hp_out += _cutoff_high_pass_ext * out_l;\
	_left_lp_out += (output - _left_lp_out);\
/*	_left_lp_out += cutoff_low_pass_ext * (output - _left_lp_out);*/\
	output = out_l;
#define APPLY_EXTERNAL_FILTER_R(output) \
	/* note: an earlier cycle-by-cycle loop-impl was a performance killer and seems to have been the */\
	/* main reason why some multi-SID songs started to stutter on my old PC.. I therefore switched to */\
	/* the below hack - accepting the flaws as a tradeoff */\
\
	/* cutoff: w0=1/RC  // capacitor: 1 F=1000000 uF; 1 uF= 1000000 pF (i.e. 10e-12 F) */\
/*	const double cutoff_high_pass_ext = ((double)100) / _sample_rate;*/	/* hi-pass: R=1kOhm, C=10uF; i.e. w0=100	=> causes the "saw look" on pulse WFs*/\
/*	const double cutoff_low_pass_ext = ((double)100000) / FREQUENCY;	// lo-pass: R=10kOhm, C=1000pF; i.e. w0=100000  .. no point at low sample rate */\
\
	double out_r = _right_lp_out - _right_hp_out;\
	_right_hp_out += _cutoff_high_pass_ext * out_r;\
	_right_lp_out += (output - _right_lp_out);\
/*	_right_lp_out += cutoff_low_pass_ext * (output - _right_lp_out);*/\
	output = out_r;
#endif


// --------- HW configuration ----------------

static uint16_t	_sid_addr[MAX_SIDS];		// start addr of SID chips (0 means NOT available)
static bool 	_sid_is_6581[MAX_SIDS];		// models of installed SID chips


static bool 	_ext_multi_sid;				// use extended multi-sid mode

	// fixme: the original "ext multi-sid" stereo channel assignment has been replaced by the added
	// regular stereo-panning - remove the below remainders of the old impl
static uint8_t 	_sid_target_chan[MAX_SIDS];	// output channel for the SID chips
static uint8_t 	_sid_2nd_chan_idx;			// stereo sid-files: 1st chip using 2nd channel

SIDConfigurator::SIDConfigurator() {
}

void SIDConfigurator::init(uint16_t* addrs, bool* set_6581, uint8_t* target_chan, uint8_t* second_chan_idx,
			bool* ext_multi_sid_mode) {
	_addrs = addrs;
	_is_6581 = set_6581;
	_target_chan = target_chan;
	_second_chan_idx = second_chan_idx;
	_ext_multi_sid_mode = ext_multi_sid_mode;
}

uint16_t SIDConfigurator::getSidAddr(uint8_t center_byte) {
	if (((center_byte >= 0x42) && (center_byte <= 0xFE)) &&
		!((center_byte >= 0x80) && (center_byte <= 0xDF)) &&
		!(center_byte & 0x1)) {

		return ((uint16_t)0xD000) | (((uint16_t)center_byte) << 4);
	}
	return 0;
}

void SIDConfigurator::configure(uint8_t is_ext_file, uint8_t sid_file_version, uint16_t flags, uint8_t* addr_list) {
	(*_second_chan_idx) = 0;

	_addrs[0] = 0xd400;

	uint8_t rev = (flags >> 4) & 0x3; 	// bits selecting SID revision

	_is_6581[0] = (rev != 0x2);  // only use 8580 when bit is explicitly set as only option

	if (!is_ext_file) {	// allow max of 3 SIDs
		(*_ext_multi_sid_mode) = false;

		_target_chan[0] = 0;	// no stereo support

		// standard PSID maxes out at 3 sids
		_addrs[1] = getSidAddr((addr_list && (sid_file_version>2)) ? addr_list[0x0] : 0);
		rev = (flags >> 6) & 0x3;
		_is_6581[1] = !rev ? _is_6581[0] : (rev != 0x2);
		_target_chan[1] = 0;

		_addrs[2] = getSidAddr((addr_list && (sid_file_version>3)) ? addr_list[0x1] : 0);
		rev = (flags >> 8) & 0x3;
		_is_6581[2] = !rev ? _is_6581[0] : (rev != 0x2);
		_target_chan[2] = 0;

		_addrs[3] = 0;
		_is_6581[3] = 0;
		_target_chan[3] = 0;

	} else {	// allow max of 10 SIDs
		(*_ext_multi_sid_mode) = true;

		_target_chan[0] = (flags >> 6) & 0x1;

		uint8_t prev_chan = _target_chan[0];

		// is at even offset so there should be no alignment issue
		uint16_t *addr_list2 = (uint16_t*)addr_list;

		uint8_t i, ii;
		for (i= 0; i<(MAX_SIDS-1); i++) {
			uint16_t flags2 = addr_list2[i];	// bytes are flipped here
			if (!flags2) break;

			ii = i + 1;

			_addrs[ii] = getSidAddr(flags2 & 0xff);

			_target_chan[ii] = (flags2 >> 14) & 0x1;

			if (!(*_second_chan_idx)) {
				if (prev_chan != _target_chan[ii] ) {
					(*_second_chan_idx) = ii;	// 0 is the $d400 SID

					prev_chan = _target_chan[ii];
				}
			}

			uint8_t rev = (flags2 >> 12) & 0x3;	// bits selecting SID revision
			_is_6581[ii] = !rev ? _is_6581[0] : (rev != 0x2);
		}
		for (; i<(MAX_SIDS-1); i++) {	// mark as unused
			ii = i + 1;

			_addrs[ii] = 0;
			_is_6581[ii] = false;
			_target_chan[ii] = 0;
		}
	}
}

static SIDConfigurator _hw_config;


static uint8_t _used_sids = 0;
static uint8_t _is_audible = 0;

static SID _sids[MAX_SIDS];	// allocate the maximum

// globally shared by all SIDs
static double		_cycles_per_sample;
static uint32_t		_sample_rate;				// target playback sample rate


/**
* This class represents one specific MOS SID chip.
*/
SID::SID() {
	_addr = 0;		// e.g. 0xd400

	_env_generators[0] = new Envelope(this, 0);
	_env_generators[1] = new Envelope(this, 1);
	_env_generators[2] = new Envelope(this, 2);

	_wave_generators[0] = new WaveGenerator(this, 0);
	_wave_generators[1] = new WaveGenerator(this, 1);
	_wave_generators[2] = new WaveGenerator(this, 2);

	_filter= NULL;
	setFilterModel(false);	// default to 8580

	_digi = new DigiDetector(this);
}

void SID::setFilterModel(bool set_6581) {
	if (!_filter || (set_6581 != _is_6581)) {
		_is_6581 = set_6581;

		if (_filter) { delete _filter; }

		_filter = _is_6581 ? (Filter*)new Filter6581(this) : (Filter*)new Filter8580(this);
	}
}

WaveGenerator* SID::getWaveGenerator(uint8_t voice_idx) {
	return _wave_generators[voice_idx];
}

void SID::resetEngine(uint32_t sample_rate, bool set_6581, uint32_t clock_rate) {
	// note: structs are NOT packed and contain additional padding..

	_sample_rate = sample_rate;
	_cutoff_high_pass_ext = ((double)100) / _sample_rate;

	_cycles_per_sample = ((double)clock_rate) / sample_rate;	// corresponds to Hermit's clk_ratio

	for (uint8_t i= 0; i<3; i++) {
		_wave_generators[i]->reset(_cycles_per_sample);
	}

	// reset envelope generator
	for (uint8_t i= 0; i<3; i++) {
		_env_generators[i]->reset();
	}

	// reset filter
	resetModel(set_6581);

	// reset external filter
	_left_lp_out= _left_hp_out= 0;
}

void SID::clockWaveGenerators() {
	// forward oscillators one CYCLE (required to properly time HARD SYNC)
	for (uint8_t voice_idx= 0; voice_idx<3; voice_idx++) {
		WaveGenerator* wave_gen = _wave_generators[voice_idx];
		wave_gen->clockPhase1();
	}

	// handle oscillator HARD SYNC (quality wise it isn't worth the trouble to
	// use this correct impl..)
	for (uint8_t voice_idx= 0; voice_idx<3; voice_idx++) {
		WaveGenerator* wave_gen = _wave_generators[voice_idx];
		wave_gen->clockPhase2();
	}
}

double SID::getCyclesPerSample() {
	return _cycles_per_sample;
}

// ------------------------- public API ----------------------------

struct SIDConfigurator* SID::getHWConfigurator() {
	// let loader directly manipulate respective state
	_hw_config.init(_sid_addr, _sid_is_6581, _sid_target_chan, &_sid_2nd_chan_idx, &_ext_multi_sid);
	return &_hw_config;
}

bool SID::isSID6581() {
	return _sid_is_6581[0];		// only for the 1st chip
}

uint8_t SID::setSID6581(bool set_6581) {
	// this minimal update should allow to toggle the
	// used filter without disrupting playback in progress

    //YOYOFR
    for (int i=0;i<MAX_SIDS;i++) _sid_is_6581[i]  = set_6581;
    //
	SID::setModels(_sid_is_6581);
	return 0;
}

// who knows what people might use to wire up the output signals of their
// multi-SID configurations... it probably depends on the song what might be
// "good" settings here.. alternatively I could just let the clipping do its
// work (without any rescaling). but for now I'll use Hermit's settings - this
// will make for a better user experience in DeepSID when switching players..

static double _vol_map[] = { 1.0f, 0.6f, 0.4f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f, 0.3f };
static double _vol_scale;


uint16_t SID::getBaseAddr() {
	return _addr;
}

void SID::resetModel(bool set_6581) {
	_bus_write = 0;

	// On the real hardware all audio outputs would result in some (positive)
	// voltage level and different chip models would use different base
	// voltages to begin with. Effects like the D418 based output modulation used
	// for digi playback would always scale some positive input voltage.

	// On newer chip models the respective base voltage would actually be quite small -
	// causing D418-digis to be barely audible - unless a better carrier signal was
	// specifically created via the voice-outputs. But older chip models would use
	// a high base voltage which always provides a strong carrier signal for D418-digis.

	// The current use of signed values to represent audio signals causes a problem
	// here, since the D418-scaling of negative numbers inverts the originally intended
	// effect. (For the purpose of D418 output scaling, the respective input
	// should always be positive.)

	if (set_6581) {
		_wf_zero = -0x3800;
		_dac_offset = 0x8000 * 0xff;
	} else {
		// FIXME: below hack will cause negative "voltages" that will cause total garbage
		// (inverted digi output) when D418 samples are used.. correctly it should result
		// in "no digi" since there would just be NO voltage. cleanup respective
		// "output voltage level" handling-

		_wf_zero = -0x8000;
		_dac_offset = -0x1000 * 0xff;
	}

	setFilterModel(set_6581);
	_filter->setSampleRate(_sample_rate);
}

void SID::reset(uint16_t addr, uint32_t sample_rate, bool set_6581, uint32_t clock_rate,
				 uint8_t is_rsid, uint8_t is_compatible) {

	_addr = addr;

	resetEngine(sample_rate, set_6581, clock_rate);

	_digi->reset(clock_rate, is_rsid, is_compatible);

	// turn on full volume
	memWriteIO(getBaseAddr() + 0x18, 0xf);
	poke(0x18, 0xf);
}

uint8_t SID::isModel6581() {
	return _is_6581;
}

uint8_t SID::peekMem(uint16_t addr) {
	return MEM_READ_IO(addr);
}



uint8_t SID::readVoiceLevel(uint8_t voice_idx) {

	WaveGenerator* wave_gen = _wave_generators[voice_idx];
	bool is_muted = wave_gen->isMuted() || _filter->isSilencedVoice3(voice_idx);

	return is_muted ? 0 : _env_generators[voice_idx]->getOutput();
}

uint8_t SID::readMem(uint16_t addr) {
	uint16_t offset = addr - _addr;

	switch (offset) {
	case 0x1b:	// "oscillator" .. docs once again are wrong since this is WF specific!
		return _wave_generators[2]->getOsc();

	case 0x1c:	// envelope
		return _env_generators[2]->getOutput();
	}

	// reading of "write only" registers returns whatever has been last
	// written to the bus (register independent) and that value normally
	// would also fade away eventually.. only few songs use this and NONE
	// seem to depend on "fading" to be actually implemented;
	// testcase: Mysterious_Mountain.sid

	return _bus_write;
}


void SID::poke(uint8_t reg, uint8_t val) {
    uint8_t voice_idx = 0;
	if (reg < 7) {}
    else if (reg <= 13) { voice_idx = 1; reg -= 7; }
    else if (reg <= 20) { voice_idx = 2; reg -= 14; }

	// writes that impact the envelope generator
	if ((reg >= 0x4) && (reg <= 0x6)) {
		_env_generators[voice_idx]->poke(reg, val);
	}

	// writes that impact the filter
    switch (reg) {
		case 0x15:
		case 0x16:
		case 0x17:
			_filter->poke(reg, val);
			break;
		case 0x18:
			_filter->poke(reg, val);

			_volume = (val & 0xf);
			break;
	}

    switch (reg) {
        case 0x0: {
			WaveGenerator* wave_gen = _wave_generators[voice_idx];
			wave_gen->setFreqLow(val);
            break;
        }
        case 0x1: {
			WaveGenerator* wave_gen = _wave_generators[voice_idx];
			wave_gen->setFreqHigh(val);
            break;
        }
        case 0x2: {
			WaveGenerator* wave_gen = _wave_generators[voice_idx];
			wave_gen->setPulseWidthLow(val);
            break;
        }
        case 0x3: {
			WaveGenerator* wave_gen = _wave_generators[voice_idx];
			wave_gen->setPulseWidthHigh(val);
            break;
        }
        case 0x4: {
			WaveGenerator *wave_gen = _wave_generators[voice_idx];
			wave_gen->setWave(val);

			break;
		}
    }

    return;
}

#ifdef RPI4
// extension callback used by the RaspberryPi4 version to play on an actual SID chip
extern void recordPokeSID(uint32_t ts, uint8_t reg, uint8_t value);
#include "system.h"
#endif

void SID::writeMem(uint16_t addr, uint8_t value) {
	_digi->detectSample(addr, value);
	_bus_write = value;

	// no reason anymore to NOT always write (unlike old/un-synced version)

	const uint16_t reg = addr & 0x1f;
#ifdef RPI4
	/*
	if (reg == 0x18) {
		value= ((value&0x0f)<=7) ? value : (value&0xf0) | 0x07;// hack: use half volume for test recordings..
	}
	// completely suppress writes to disabled voices so that individual voices can be analyzed more
	// easily using the real chip..
	if ((reg >= 7) && (reg <= 20)) {	// todo: add API to control this more
		// e.g. ignore voices 1+2
	}
    else */

	recordPokeSID(SYS_CYCLES(), reg, value);
#endif

	poke(reg, value);
	memWriteIO(addr, value);

	// some crappy songs like Aliens_Symphony.sid actually use d5xx instead
	// of d4xx for their SID settings.. i.e. they use mirrored addresses that
	// might actually be used by additional SID chips. always map to standard
	// address to ease debug output handling, e.g. visualization in DeepSid

	if (_used_sids == 1) {
		addr = 0xd400 | reg;
		memWriteIO(addr, value);
	}
}

void SID::clock() {
	clockWaveGenerators();		// for all 3 voices

	for (uint8_t voice_idx= 0; voice_idx<3; voice_idx++) {
		_env_generators[voice_idx]->clockEnvelope();
	}
}

const int32_t _clip_value = 32767;

// clipping (filter, multi-SID as well as PSID digi may bring output over the edge)
#define RENDER_CLIPPED(dest, final_sample) \
	if ( final_sample < -_clip_value ) { \
		final_sample = -_clip_value; \
	} else if ( final_sample > _clip_value ) { \
		final_sample = _clip_value; \
	} \
	*(dest)= (int16_t)final_sample


#define OUTPUT_SCALEDOWN ((double)1.0/90)

#define APPLY_MASTERVOLUME(sample) \
	sample *= _volume; /* fixme: volume here should always modulate some "positive voltage"! */\
	sample *= OUTPUT_SCALEDOWN /* fixme: opt? directly combine with _volume? */;


void SID::synthSample(int16_t** synth_trace_bufs, uint32_t offset, int32_t *s_l, int32_t *s_r) {

	int32_t vout[3];	// outputs of the 3 voices

	// digi sample add-on
	int32_t dvoice_idx;
	int32_t digi_out = 0;
	int8_t digi_override= _digi->useOverrideDigiSignal(&digi_out, &dvoice_idx);

	// create output sample based on current SID state
	for (uint8_t voice_idx= 0; voice_idx<3; voice_idx++) {

		WaveGenerator* wave_gen = _wave_generators[voice_idx];

		bool is_muted = wave_gen->isMuted() || _filter->isSilencedVoice3(voice_idx);

		if (is_muted) {
			if (digi_override && (dvoice_idx == voice_idx)) {
				// hack: get rid of screeching carrier signal in PWM digi-songs by
				// replacing the SID output with the "intended" digi signal (flaw: since
				// regular "mute" feature is used, the respective voice can no longer be
				// manually turned off)
				vout[voice_idx] = digi_out;
			} else {
				vout[voice_idx]= 0;
			}

			// trace output (always make it 16-bit)
			if (synth_trace_bufs) {
				int16_t *voice_trace_buffer = synth_trace_bufs[voice_idx];
				*(voice_trace_buffer + offset) = vout[voice_idx];	// never filter
			}

		} else {
			uint8_t env_out = _env_generators[voice_idx]->getOutput();
			int32_t outv = ((wave_gen)->*(wave_gen->getOutput))(); // crappy C++ syntax for calling the "getOutput" method

			// note: the _wf_zero ofset *always* creates some wave-output that will be modulated via the
			// envelope (even when 0-waveform is set it will cause audible clicks and distortions in
			// the scope views)
            
			int32_t o = _vol_scale * ( env_out * (outv + _wf_zero) + _dac_offset);
			vout[voice_idx]= _filter->getVoiceOutput(voice_idx, &o);
            
            vgm_last_vol[m_voice_current_system*4+voice_idx]= env_out;//YOYOFR

			// trace output (always make it 16-bit)
			if (synth_trace_bufs) {
				// when filter is active then muted voices may still show some effect
				// in the trace... but there is no point to slow things down with additional
				// checks here
				int16_t *voice_trace_buffer = synth_trace_bufs[voice_idx];

				// the ">>8" should correctly be "/255" - but the faster but incorrect impl should be adequate here
				o = env_out * (outv - 0x8000) >> 8;	// make sure the scope is nicely centered
				*(voice_trace_buffer + offset) = (int16_t)_filter->getVoiceScopeOutput(voice_idx, &o);
			}
		}
	}


	if (synth_trace_bufs && (dvoice_idx == -1)) {
		// only the d418 based digis should be shown in the respective "digi track" scope
		int16_t *voice_trace_buffer = synth_trace_bufs[3];	// d418 digi scope buffer
		*(voice_trace_buffer + offset) = digi_out;			// save the trouble of filtering
	}

	int32_t final_sample_l;
	int32_t final_sample_r;

	if(_digi->isMahoney()) {
		// hack: directly output the digi to avoid distortions caused by the low sample rate..
		// testcase: Acid_Flashback.sid

		final_sample_l = final_sample_r = digi_out;
	} else {
		final_sample_l = vout[0]*_pan_left[0] + vout[1]*_pan_left[1] + vout[2]*_pan_left[2];
		APPLY_MASTERVOLUME(final_sample_l);
		final_sample_r = vout[0]*_pan_right[0] + vout[1]*_pan_right[1] + vout[2]*_pan_right[2];
		APPLY_MASTERVOLUME(final_sample_r);
	}

	APPLY_EXTERNAL_FILTER_L(final_sample_l);
	APPLY_EXTERNAL_FILTER_R(final_sample_r);

	final_sample_l = _digi->genPsidSample(final_sample_l);		// recorded PSID digis are merged in directly
	final_sample_r = _digi->genPsidSample(final_sample_r);		// recorded PSID digis are merged in directly

	*s_l = final_sample_l;
	*s_r = final_sample_r;
}

// same as above but without digi & no filter for trace buffers - once faster
// PCs are more widely in use, then this optimization may be ditched..

void SID::synthSampleStripped(int16_t** synth_trace_bufs, uint32_t offset, int32_t *s_l, int32_t *s_r) {
	int32_t vout[3];

	for (uint8_t voice_idx= 0; voice_idx<3; voice_idx++) {

		uint8_t env_out = _env_generators[voice_idx]->getOutput();
		WaveGenerator *wave_gen= _wave_generators[voice_idx];
		int32_t outv = ((wave_gen)->*(wave_gen->getOutput))(); // crappy C++ syntax for calling the "getOutput" method

		int32_t o = _vol_scale * ( env_out * (outv + _wf_zero) + _dac_offset);
		vout[voice_idx]= _filter->getVoiceOutput(voice_idx, &o);

		// trace output (always make it 16-bit)
		if (synth_trace_bufs) {
			// performance note: specially for multi-SID (e.g. 8SID) song's use
			// of the filter for trace buffers has a significant runtime cost
			// (e.g. 8*3= 24 additional filter calculations - per sample - instead
			// of just 1). switching to display of non-filtered data may make the
			// difference between "still playing" and "much too slow" -> like on
			// my old machine

			int16_t *voice_trace_buffer = synth_trace_bufs[voice_idx];
			*(voice_trace_buffer + offset) = env_out * (outv - 0x8000) >> 8;	// make sure the scope is nicely centered
		}
	}

	int32_t final_sample_l = vout[0]*_pan_left[0] + vout[1]*_pan_left[1] + vout[2]*_pan_left[2];
	APPLY_MASTERVOLUME(final_sample_l);

	int32_t final_sample_r = vout[0]*_pan_right[0] + vout[1]*_pan_right[1] + vout[2]*_pan_right[2];
	APPLY_MASTERVOLUME(final_sample_r);

	APPLY_EXTERNAL_FILTER_L(final_sample_l);
	APPLY_EXTERNAL_FILTER_R(final_sample_r);

	*s_l = final_sample_l;
	*s_r = final_sample_r;
}

// "friends only" accessors
uint8_t SID::getWave(uint8_t voice_idx) {
	return _wave_generators[voice_idx]->getWave();
}

uint16_t SID::getFreq(uint8_t voice_idx) {
	return _wave_generators[voice_idx]->getFreq();
}

uint16_t SID::getPulse(uint8_t voice_idx) {
	return _wave_generators[voice_idx]->getPulse();
}

uint8_t SID::getAD(uint8_t voice_idx) {
	return _env_generators[voice_idx]->getAD();
}

uint8_t SID::getSR(uint8_t voice_idx) {
	return _env_generators[voice_idx]->getSR();
}

uint32_t SID::getSampleFreq() {
	return _sample_rate;
}

DigiType  SID::getDigiType() {
	return _digi->getType();
}

const char*  SID::getDigiTypeDesc() {
	return _digi->getTypeDesc();
}

uint16_t  SID::getDigiRate() {
	return _digi->getRate();
}

void SID::setMute(uint8_t voice_idx, uint8_t is_muted) {
	if (voice_idx > 3) voice_idx = 3; 	// no more than 4 voices per SID (volume as 4th "voice")

	if (voice_idx == 3) {
		_digi->setEnabled(!is_muted);

	} else {
		_wave_generators[voice_idx]->setMute(is_muted);
	}
}

void SID::resetStatistics() {
	_digi->resetCount();
}

/**
* Use a simple map to later find which IO access matches which SID (saves annoying if/elses later):
*/
const int MEM_MAP_SIZE = 0xbff;
static uint8_t _mem2sid[MEM_MAP_SIZE];	// maps memory area d400-dfff to available SIDs

void SID::setMute(uint8_t sid_idx, uint8_t voice_idx, uint8_t is_muted) {
	if (sid_idx > 9) sid_idx = 9; 	// no more than 10 sids supported

	_sids[sid_idx].setMute(voice_idx, is_muted);
}

DigiType SID::getGlobalDigiType() {
	return _sids[0].getDigiType();
}

const char* SID::getGlobalDigiTypeDesc() {
	return _sids[0].getDigiTypeDesc();
}

uint16_t SID::getGlobalDigiRate() {
	return _sids[0].getDigiRate();
}

void SID::clockAll() {
	for (uint8_t i= 0; i<_used_sids; i++) {
		SID &sid = _sids[i];
		sid.clock();
	}
}

void SID::synthSamplesSingleSID(int16_t* buffer, int16_t** synth_trace_bufs, uint32_t offset) {
	// most relevant: single-SID case

	int16_t *dest = buffer + (offset << 1);
    
    m_voice_current_system=0; //YOYOFR

	if (SID::isAudible()) {
		const uint8_t i= 0;
		SID &sid = _sids[i];
		int16_t **sub_buf = !synth_trace_bufs ? 0 : &synth_trace_bufs[i << 2];	// each sid uses 4 entries..

		int32_t s_l, s_r;
		sid.synthSample(sub_buf, offset, &s_l, &s_r);

		RENDER_CLIPPED(dest, s_l);
		RENDER_CLIPPED(dest+1, s_r);

	} else {
		dest[0]= dest[1]= 0;
	}
}

void SID::synthSamplesMultiSID(int16_t* buffer, int16_t** synth_trace_bufs, uint32_t offset) {
	// regular multi-SID

	int16_t *dest = buffer + (offset << 1);

	if (SID::isAudible()) {		// might be skipped in this scenario
		int32_t final_sample_l = 0;
		int32_t final_sample_r = 0;

		for (uint8_t i= 0; i<_used_sids; i++) {
			SID &sid = _sids[i];
			int16_t **sub_buf = !synth_trace_bufs ? 0 : &synth_trace_bufs[i << 2];	// each sid uses 4 entries..

			int32_t s_l, s_r;
            
            m_voice_current_system=i; //YOYOFR
            
			sid.synthSample(sub_buf, offset, &s_l, &s_r);

			final_sample_l += s_l;
			final_sample_r += s_r;
		}

		RENDER_CLIPPED(dest, final_sample_l);
		RENDER_CLIPPED(dest+1, final_sample_r);

	} else {
		dest[0]= dest[1]= 0;
	}
}

void SID::synthSamplesStrippedMultiSID(int16_t* buffer, int16_t** synth_trace_bufs, uint32_t offset) {
	// reduced multi-SID case

	int16_t *dest = buffer + (offset << 1);

	if (SID::isAudible()) {		// might be skipped in this scenario
		int32_t final_sample_l = 0;
		int32_t final_sample_r = 0;

		for (uint8_t i= 0; i<_used_sids; i++) {
			SID &sid = _sids[i];
			int16_t **sub_buf = !synth_trace_bufs ? 0 : &synth_trace_bufs[i << 2];	// each sid uses 4 entries..

			int32_t s_l, s_r;
			sid.synthSampleStripped(sub_buf, offset, &s_l, &s_r);

			final_sample_l += s_l;
			final_sample_r += s_r;
		}

		RENDER_CLIPPED(dest, final_sample_l);
		RENDER_CLIPPED(dest+1, final_sample_r);

	} else {
		dest[0]= dest[1]= 0;
	}
}


void SID::resetGlobalStatistics() {
	for (uint8_t i= 0; i<_used_sids; i++) {
		SID &sid = _sids[i];
		sid.resetStatistics();
	}
}

void SID::setModels(const bool* set_6581) {
	for (uint8_t i= 0; i<_used_sids; i++) {
		SID &sid = _sids[i];
		sid.resetModel(set_6581[i]);
	}
}

uint8_t SID::getNumberUsedChips() {
	return _used_sids;
}

uint16_t SID::getSIDBaseAddr(uint8_t idx) {
	return idx < _used_sids ? _sid_addr[idx] : 0;
}

uint8_t SID::isAudible() {
	return _is_audible;
}

void SID::resetAll(uint32_t sample_rate, uint32_t clock_rate, uint8_t is_rsid,
					uint8_t is_compatible) {

	// determine the number of used SIDs
	_used_sids = 0;
	for (uint8_t i= 0; i<MAX_SIDS; i++) {
		if (_sid_addr[i]) {
			_used_sids++;
		}
	}

	memset(_mem2sid, 0, MEM_MAP_SIZE); // default is SID #0

	_is_audible = 0;

//	if (_ext_multi_sid) {
//		_vol_scale = _vol_map[_sid_2nd_chan_idx ? _used_sids >> 1 : _used_sids - 1] / 0xff;
//	} else {
		_vol_scale = _vol_map[_used_sids - 1] / 0xff;	// 0xff serves to normalize the 8-bit envelope
//	}

	// setup the configured SID chips & make map where to find them

	for (uint8_t i= 0; i<_used_sids; i++) {
		SID &sid = _sids[i];
		sid.reset(_sid_addr[i], sample_rate, _sid_is_6581[i], clock_rate, is_rsid, is_compatible);	// stereo only used for my extended sid-file format

		if (i) {	// 1st entry is always the regular default SID
			memset((void*)(_mem2sid + _sid_addr[i] - 0xd400), i, 0x1f);
		}
	}
}
void SID::initPanning(float *panPerSID) {
	for (uint8_t i= 0; i<_used_sids; i++) {
		SID &sid = _sids[i];
		for (uint8_t j= 0; j<3; j++) {
			sid.setPanning(j, panPerSID[3*i + j]);
		}
	}
}
extern "C" void	sidSetPanning(uint8_t sid_idx, uint8_t voice_idx, float panning) {
	if (sid_idx < _used_sids) {
		SID &sid = _sids[sid_idx];
		sid.setPanning(voice_idx, panning);
	}
}

void SID::setPanning(uint8_t voice_idx, float pan) {
	// reminder: ideally the _vol_scale scaling would be moved here (to avoid the additional multiplication)
	// but the used float values seem to be at the limit of what can be handled by that type, i.e. clamping
	// kicks in - leading to distorted audio output.

	// the issue here could be avoided by compiling with: -s BINARYEN_TRAP_MODE='allow' (this also would
	// have a performance benefit). however that leads to "divide by zero" (testcase: MSK-Marlboro.sid)
	// elsewhere in the code (not analyzed yet) so for now I stick to -s BINARYEN_TRAP_MODE='clamp'
	// and keep the extra scaling multiplication

	_pan_right[voice_idx] = pan;
	_pan_left[voice_idx] = (1.0 - pan);
}

uint8_t SID::isExtMultiSidMode() {
	return _ext_multi_sid;
}

// gets what has actually been last written (even for write-only regs)
uint8_t SID::peek(uint16_t addr) {
	uint8_t sid_idx = _mem2sid[addr - 0xd400];
	return _sids[sid_idx].peekMem(addr);
}

// -------------- API used by C code --------------


extern "C" uint8_t sidReadVoiceLevel(uint8_t sid_idx, uint8_t voice_idx) {
	return _sids[sid_idx].readVoiceLevel(voice_idx);
}

extern "C" uint8_t sidReadMem(uint16_t addr) {
	uint8_t sid_idx = _mem2sid[addr - 0xd400];
	return _sids[sid_idx].readMem(addr);
}

extern "C" void sidWriteMem(uint16_t addr, uint8_t value) {
	_is_audible |= value;	// detect use by the song

	uint8_t sid_idx = _mem2sid[addr - 0xd400];
	_sids[sid_idx].writeMem(addr, value);
}
