/*
* Handles the wave output of a single voice of the C64's "MOS Technology SID" (see 6581, 8580 or 6582).
*
* This includes handling of the involved oscillators and the actual output generation
* based on the selected waveform(s) and operating mode. (The resulting wave-output is later - not
* here - scaled using the output of the envelope generator and may be optionally processed via the SIDs
* internal filter.)
*
* Limitation: Updates that are performed within the current sample (e.g. oscillator reset, WF, pulsewidth
* or frequency change) are not handled correctly. The wave output calculation does not currently
* consider the part which correctly should still be based on the previous setting.
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.93
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fenv.h>

#include "sid.h"
extern "C" {
#include "system.h"		// SYS_CYCLES()
}
#include "wavegenerator.h"

// waveform control register flags
	// other flags
#define GATE_BITMASK 	0x01
#define SYNC_BITMASK 	0x02
#define RING_BITMASK 	0x04
#define TEST_BITMASK 	0x08
	// waveform selection
#define WF_BITMASK 		0xf0
#define TRI_BITMASK 	0x10
#define SAW_BITMASK 	0x20
#define PULSE_BITMASK 	0x40
#define NOISE_BITMASK 	0x80


#ifdef USE_HERMIT_ANTIALIAS
const double SCALE_12_16 = ((double)0xffff) / 0xfff;
#endif

/**
* This utility class keeps the precalculated "combined waveform" lookup tables.
*
* There is no point keeping these in each SID instance (since they are the same anyway)
*/
class WaveformTables {
public:
	WaveformTables();
private:
	static void createCombinedWF(double* wfarray, double bitmul, double bitstrength, double threshold);
public:
	// Hermit's precalculated "combined waveforms"
	double TriSaw_8580[4096];
	double PulseSaw_8580[4096];
	double PulseTri_8580[4096];			// Hermit's use of PulseSaw_8580 does not convince in Last_Ninja
	double PulseTriSaw_8580[4096];
};

WaveformTables::WaveformTables() {
    createCombinedWF(TriSaw_8580, 		0.8, 2.4, 0.64);
    createCombinedWF(PulseSaw_8580, 	1.4, 1.9, 0.68);
	createCombinedWF(PulseTriSaw_8580, 	0.8, 2.5, 0.64);
	// far from "correct" but at least a bit better than Hermit's use of PulseSaw_8580 (see Last_Ninja)
    createCombinedWF(PulseTri_8580, 	0.8, 1.5, 0.38);	// improved settings are welcome!
}

void WaveformTables::createCombinedWF(double* wfarray, double bitmul, double bitstrength, double threshold) {
	// Hermit: "I found out how the combined waveform works (neighboring bits affect each other recursively)"
	for (uint16_t i = 0; i < 4096; i++) {
		wfarray[i] = 0; //neighbour-bit strength and DAC MOSFET threshold is approximately set by ears'n'trials
		for (uint8_t j = 0; j < 12; j++) {
			double bitlevel = 0;
			for (uint8_t k = 0; k < 12; k++) {
				bitlevel += (bitmul / pow(bitstrength, abs(k - j))) * (((i >> k) & 1) - 0.5);
			}
			wfarray[i] += (bitlevel >= threshold) ? pow(2.0, (double)j) : 0;
		}
		wfarray[i] *= 12;
	}
}

static WaveformTables _wave_table;		// only need one instance of this


// ----------------------- utils -----------------------------

#define PREV_IDX(voice_idx) \
	(voice_idx ? voice_idx - 1 : 2)

#define NEXT_IDX(voice_idx) \
	((voice_idx == 2) ? 0 : voice_idx + 1)

// Get the bit from an uint32_t at a specified position
#define GET_BIT(val, b) \
	((uint8_t) ((val >> b) & 1))

// Bob Yannes: "Ring Modulation was accomplished by substituting
// the accumulator MSB of an oscillator in the EXOR function of
// the triangle waveform generator with the accumulator MSB of the
// previous oscillator. That is why the triangle waveform must be
// selected to use Ring Modulation."

// what this means is to substitute the MSB of the current accumulator
// by EXORing it with the MSB of the respective previous oscillator:
#define GET_RINGMOD_COUNTER() \
	_ring_bit ? _counter ^ (_sid->getWaveGenerator(PREV_IDX(_voice_idx))->_counter & 0x800000) : _counter;


#define NULL_FLOAT_DURATION		3000000		// incorrect but irrelevant

// ---------------------------------------------------------------------------------------------
// ------ noise waveform handling (see "docs/noise-waveform.txt" for background info) ----------
// ---------------------------------------------------------------------------------------------

// As a performance optimization the impl only updates the _noise_LFSR while noise WF is actually
// used - which potentially might lead to an incorrect start position within the random-number
// stream and/or flawed combined-WF feedback _noise_LFSR. todo: do some performance measurements
// to quantify the actual benefit (if any).

// test cases: Quedex, Ghouls 'n' Ghosts, Trick'n'Treat (later parts), Clique_Baby, Smurf_Nightmare

// special case handling needed for noise-oversampling. (only _ref0_ts needs to be always
// updated and rest could be initialized on demand then noise is actualy used.. todo: lets
// first see if this is worth optimizing..)
#define SAMPLE_END() \
	_ref0_ts = _ref1_ts = SYS_CYCLES();\
	_noiseout_sum = 0;

#define OVERSAMPLE_NOISE_OUTPUT(out) \
	_noiseout_sum += (SYS_CYCLES() - _ref1_ts) * out; \
	_ref1_ts = SYS_CYCLES(); /* track interval that has already been handled */


#define INIT_NOISE_OVERSAMPLING(old_noise_bit, new_noise_bit) \
	if (new_noise_bit > old_noise_bit) { \
		/* Only the noise output rendering currently performs some type of  */ \
		/* oversampling. to correctly time respective WF switches, transitions  */ \
		/* into a noise-WF must be handled here.. known limitation: the */ \
		/* inverse transition from noise to something else will just lose the noise */ \
		/* part completely */ \
		\
		/* Remember previous waveform output for use in noise-interpolation */ \
		/* (the next clocking of the shift-register will restore a correct _noiseout */ \
		/* and just use this dummy for the "no-noise-yet" interval of the sample). */ \
		 \
		/* issue: this uses only 1-sample resolution and the anti-aliasing */ \
		/* used for some WFs might cause problems here.. */ \
		_noiseout = ((this)->*(this->getOutput))();	/* maybe a separate var should rather be used to not cause confusion.. */ \
	}


#define NOISE_RESET_DELAY		32000		// incorrect but irrelevant

#define NOISE_RESET 0x7fffff

// combined noise-waveform may feed back into _noise_LFSR shift-register potentially
// *clearing* (not setting) bits that are used for the "_noiseout"
static const uint32_t COMBINED_NOISE_MASK = ~((1<<20)|(1<<18)|(1<<14)|(1<<11)|(1<<9)|(1<<5)|(1<<2)|(1<<0));

void WaveGenerator::resetNoiseGenerator() {
	_noise_reset_ts =  SYS_CYCLES() + NOISE_RESET_DELAY;

	// no shifting while test-bit is set: an already scheduled shift might
	// get cancelled here
	_trigger_noise_shift = 0;
}

void WaveGenerator::refillNoiseShiftRegister() {
	_noise_LFSR = NOISE_RESET;
	activateNoiseOutput(); // init _noiseout
}

// based on resid's analysis: the process of shifting the noise shift-register
// extends over 3 clock cycles with slight differences regarding the feedback of
// combined WF output (i.e. when feedback does or does not occur).. the WebSid
// impl may be flawed since it does NOT update waveform output in every cycle..

#define CLOCK_NOISE_GENERATOR() \
	uint32_t trigger_in = _trigger_noise_shift; \
	if (_noise_bit) {	/* technically incorrect optimization: ignore noise while not used */ \
		if (_trigger_noise_shift && (--_trigger_noise_shift == 0)) { \
			shiftNoiseRegisterNoTestBit(); \
		} else if ((_counter & 0x080000) > (prev_counter & 0x080000)) { \
			_trigger_noise_shift = 2; /* delay actual shifting by 2 cycles */ \
		} \
		/* combined-WF output feeds back in every cycle.. */\
		/* testcase: Bojojoing.sid - feedback during 4 clocks of 0xf1 WF */\
		if ((_wf_bits > 0x80) /* && (trigger_in != 1)*/) { \
			/* during the "phase 1" cycle, feedback would supposedly NOT */ \
			/* directly affect the "active" shiftregister but some "temporary" latch */ \
			/* that would then be activated in "phase 2", i.e. that temporary */ \
			/* result might get cancelled via a set test-bit */ \
			/* todo: recalculating combined-WF in every cycle is overkill and could likely be optimized */ \
			feedbackNoise(combinedNoiseInput()); \
		} \
	}

// get waveform(s) that feed(s) into the combined noise-WF
uint16_t WaveGenerator::combinedNoiseInput() {
	// issue: based on the *current* WF-, test-bit and oscillator status, i.e.
	// unsuitable to retroactively calc previous output

	// known limitation: WebSid does not update respective signal/feedback in
	// every cycle and the below impls for combined-WFs may be flawed.

	uint16_t out;
	switch (_wf_bits  & 0x70) {		// everything except the noise
		// single WF
		case TRI_BITMASK:
			out = createTriangleOutput();
			break;
		case SAW_BITMASK:
			out = (uint16_t)(_counter >> 8);
			break;
		case PULSE_BITMASK:
			out = (uint16_t)((_test_bit == 0) && (_counter < _pulse_width12) ? 0 : 0xffff);
			break;

		// combined WFs
		case PULSE_BITMASK|TRI_BITMASK|SAW_BITMASK:
			out = pulseTriangleSawOutput();
			break;
		case PULSE_BITMASK|TRI_BITMASK:
			out = pulseTriangleOutput();
			break;
		case PULSE_BITMASK|SAW_BITMASK:
			out = pulseSawOutput();
			break;
		case TRI_BITMASK|SAW_BITMASK:
			out = triangleSawOutput();
			break;
		default:
			// dead code!
			out = 0;
			break;
	}
	return out;
}

void WaveGenerator::activateNoiseOutput() {
	// get the 8-bit noise output-WF from the shift-register (as a 16-bit output)

	// according to resid's analysis this happens in the last cycle of the 3-cycle
	//"shift" sequence (i.e. the updated noise waveform output is "activated")

	_noiseout =  ((_noise_LFSR >> 5 /*(20 - 7 - 8)*/ ) & 0x8000 ) |
				 ((_noise_LFSR >> 4 /*(18 - 6 - 8)*/ ) & 0x4000 ) |
				 ((_noise_LFSR >> 1 /*(14 - 5 - 8)*/ ) & 0x2000 ) |
				 ((_noise_LFSR << 1 /*(11 - 4 - 8)*/ ) & 0x1000 ) |
				 ((_noise_LFSR << 2 /*(9  - 3 - 8)*/ ) & 0x0800 ) |
				 ((_noise_LFSR << 5 /*(5  - 2 - 8)*/ ) & 0x0400 ) |
				 ((_noise_LFSR << 7 /*(2  - 1 - 8)*/ ) & 0x0200 ) |
				 ((_noise_LFSR << 8 /*(0  - 0 - 8)*/ ) & 0x0100 );
}

void WaveGenerator::feedbackNoise(uint16_t out) {
	_noiseout &= out;

	uint32_t feedback = (GET_BIT(_noiseout, (7 + 8)) << 20) |
						(GET_BIT(_noiseout, (6 + 8)) << 18) |
						(GET_BIT(_noiseout, (5 + 8)) << 14) |
						(GET_BIT(_noiseout, (4 + 8)) << 11) |
						(GET_BIT(_noiseout, (3 + 8)) << 9) |
						(GET_BIT(_noiseout, (2 + 8)) << 5) |
						(GET_BIT(_noiseout, (1 + 8)) << 2) |
						(GET_BIT(_noiseout, (0 + 8)) << 0);

	_noise_LFSR &= COMBINED_NOISE_MASK | feedback;	// feed back into shift register
}

void WaveGenerator::shiftNoiseRegisterNoTestBit() {
	// "regular" shifting of noise register while test-bit is not set (triggered "within
	// SID clocking" by accumulator bit transition - with a 2 cycle delay):

	// handles the 2 final noise-update "phases" which would "correctly" be distributed across
	// 2 clock cycles (see resid docs): in the 1st of these cycles the register content is
	// shifted using some "temporary latch" (while reflecting feedback from combined waveforms
	// in that latch - but not in the active shift-register) and in the 2nd cycle (i.e. "now")
	// the respective latch is "activated", i.e. the updated content is actually "persisted"
	// in the shiftregister and the noise waveform output is extracted from the bits of the
	// shift register (i.e. can theoretically be cancelled via setting the test-bit).

	OVERSAMPLE_NOISE_OUTPUT(_noiseout);

	// shift of the register should correctly have happended 1 cycle earlier using some
	// separate latch, i.e. the latch would have accepted combined-WF feedback but the
	// shiftregister (and waveform output?) would have reflected this update only after the
	// completion of "phase2".

/*
	// since regular feedback was suppressed in the "phase 1" (see CLOCK_NOISE_GENERATOR)
	// we should better catch up on that now: todo: check if this does any good..
	if ((_ctrl&0xf0)>NOISE_BITMASK) {
		// _counter has already been updated for the current clock so the
		// below retroactively calulated output may actually be flawed
		feedbackNoise(combinedNoiseInput());
	}
*/
	uint32_t feed = (GET_BIT(_noise_LFSR, 22) ^ GET_BIT(_noise_LFSR, 17));
	_noise_LFSR = ((_noise_LFSR << 1) | feed);	// 23-bit register (execess bits are ignored)

	activateNoiseOutput();				// extract current _noiseout signal
}

void WaveGenerator::shiftNoiseRegisterTestBitDriven(const uint8_t new_ctrl) {
	// shifting triggered by high->low transition of the test-bit, i.e. triggered via
	// a bus-access from the CPU (i.e. during the the 1st half of the clock-cycle - while
	// the peripheral device (the SID in this case) sees that change in the during the
	// 2nd half of the clock.. (there may well be some delay that I am not handling yet).
	// The regular SID clocking for this cycle is still performed later.

	// The process still goes through the two phases discussed in resid's analysis. But the
	// shift's bit0 feed term "(bit22 | test)" gets evaluated while the test bit is
	// still set - which makes bit22 irrelevant in this context.

	// That updated shift-register would then have been "activated" in "phase two" where
	// we are now - i.e. potential combined-WF feedback during this "phase two" should still
	// be directly applied to the shiftregister (see "clockPhase1" - that phase is not related
	// to the phases that resid talks about..).

	_noise_reset_ts	= 0;	// reset stops when test-bit is cleared

/*
	// Regarding combined-noise feedback: The "phase one" shifting would normally have used
	// the old waveform output (with the test-bit still set) for feedback - however normally
	// there would be no shifting while test-bit is set

	// XXX in resid this feedback is only applied after checking for various "special rules"
	// but without any realworld testcase to back it up I may just as well leave it out completely..

	if ((_wf_bits>NOISE_BITMASK) && ((new_ctrl&WF_BITMASK) != NOISE_BITMASK)) {	// only a small subset of special cases is actually used by resid in this scenario
		uint16_t o = combinedNoiseInput();	// corresponds to non-noise-portion lookup in resid
		feedbackNoise(o);
	}
*/
	OVERSAMPLE_NOISE_OUTPUT(_noiseout);

	uint32_t feed = GET_BIT(~_noise_LFSR, 17);
	_noise_LFSR = ((_noise_LFSR << 1) | feed);	// 23-bit register (just ignore excess leading bits)

	activateNoiseOutput();				// extract current noise output signal
}

uint16_t WaveGenerator::noiseOutput() {
	// _noiseout has been updated previously - when the relevant change happend.
	// it included the handling of "feedback" from combined-WF (if any)

	// just evaluate the respective oversampling data here..

	_noiseout_sum += (SYS_CYCLES() - _ref1_ts) * (uint32_t)_noiseout; // fill remainder of the sample interval

    uint16_t result;
    //YOYOFR
    if (SYS_CYCLES() - _ref0_ts) result = _noiseout_sum / (SYS_CYCLES() - _ref0_ts);
    else result=0;
    //YOYOFR
	SAMPLE_END();
	return result;
}

// ---------------------------------------------------------------------------------------------
// ------ other single waveforms                                                      ----------
// ---------------------------------------------------------------------------------------------

uint16_t WaveGenerator::nullOutput0() { // see "docs/floating-waveform.txt" for background information

	if (_floating_null_ts >= SYS_CYCLES()) {
		return _floating_null_wf;
	} else {
		return 0;
	}
}

uint16_t WaveGenerator::nullOutput() {
	uint16_t o = nullOutput0();

	SAMPLE_END();
	return o;

}

uint16_t WaveGenerator::createTriangleOutput() {
	uint32_t tmp = GET_RINGMOD_COUNTER();
    uint32_t wfout = (tmp ^ (tmp & 0x800000 ? 0xffffff : 0)) >> 7;
	return wfout & 0xffff;
}

uint16_t WaveGenerator::triangleOutput() {
	uint16_t o = createTriangleOutput();
	SAMPLE_END();
	return o;
}

uint16_t WaveGenerator::createSawOutput() {	// test-case: Super_Huey.sid (2 saw voices), Alien.sid, Kawasaki_Synthesizer_Demo.sid
	// WebSid renders output at the END of a (e.g. ~22 cycles) sample-interval (more correctly the output
	// should actually use the average value between start/end - or actually oversample)

	// the raw output valid at this cycle will be systematically too high most of the time (since it only takes
	// the end-point of a usually rising curve).. since the error is a fixed offset this should not make any audible
	// difference... but due to the fact that the curve's discontinuity (end of tooth) may occur
	// anywhere *within* the sample-interval some kind of anti-aliasing should be added to avoid undesirable
	// aliasing effects at the end of each saw-tooth
	// return _counter >> 8;	// plain saw: top 16-bits (actual saw output valid at this exact clock cycle)


#ifdef USE_HERMIT_ANTIALIAS
	// Hermit's "anti-aliasing" seems to be quite a hack compared to the output of a "correctly averaged" result
	// based on cycle-by-cycle oversampling: Hermit makes the curve steeper than it should be and his anti-aliasing
	// at the discuntinuity is then also quite off (as compared to the oversampled curve).

	double wfout = _counter >> 8;	// top 16-bits (actual saw output valid at this exatc clock cycle)
	wfout += wfout * _freq_saw_step;	// Hermit uses a lookahead scheme! i.e. his output is off by 22 cycles/1 sample..
	if (wfout > 0xffff) {
		wfout = 0xffff - (wfout - 0x10000) / _freq_saw_step;	// with _freq_saw_step=0 it should never get in here
	}
	return wfout;
#else
	// this should also be slightly faster than Hermit's impl
	if (_counter < _freq_inc_sample) {
		// we are at the end of 1st step of a new sawtooth: but this sample still contains a part of the end
		// of the previous sawtooth (i.e. the reset of the saw signal occured within this sample).. interpolate
		// the respective parts to avoid aliasing effects

		double prop_down = _counter * _freq_inc_sample_inv; 	// proportion after reset

		// the range that does not require interpolation is [_saw_sample_step/2 to 0xffff-_saw_sample_step/2],
		// i.e. the available interpolation range is 0xffff-_saw_sample_step and the correct base would be
		// _saw_sample_step/2

		// the below calc contains a deliberate excess offset of _saw_sample_step/2 so that the antialiased
		// sample here uses the same error as the regular samples generated in the "else" branch

//		return _saw_sample_step + (0xffff-_saw_sample_step)*(1-prop_down);
		return  _saw_base - _saw_range * prop_down;     // same as above just using precalculated stuff

	} else {
		// regular step-up of rising curve.. (result could be adjusted
		// via freq specific offset to get to the "correct" average, i.e. half
		// the change affected during a sample-interval)
		return _counter >> 8;
	}
#endif
}

uint16_t WaveGenerator::sawOutput() {
	uint16_t o = createSawOutput();
	SAMPLE_END();
	return o;
}

#ifdef USE_HERMIT_ANTIALIAS
void WaveGenerator::calcPulseBase(uint32_t* tmp, uint32_t* pw) {

	// based on Hermit's impl
	(*pw) = _pulse_out;				// 16 bit
	(*tmp) = _freq_pulse_base;		// 15 MSB needed

	if ((0 < (*pw)) && ((*pw) < (*tmp))) { (*pw) = (*tmp); }
	(*tmp) ^= 0xffff;
	if ((*pw) > (*tmp)) { (*pw) = (*tmp); }
	(*tmp) = _counter >> 8;				// 16 MSB needed
}

uint16_t WaveGenerator::createPulseOutput(uint32_t tmp, uint32_t pw) {	// elementary pulse
	if (_test_bit) return 0xffff;	// pulse start position

//	1) int32_t wfout = ((_counter>>12 >= _pulse_width))? 0 : 0xffff; // plain impl - inverted compared to resid
//	2) int32_t wfout = ((_counter>>12 < _pulse_width))? 0 : 0xffff; // plain impl

	// Hermit's "anti-aliasing" pulse

	// note: the smaller the step, the slower the phase shift, e.g. ramp-up/-down
	// rather than immediate switch (current setting does not cause much of an
	// effect - use "0.1*" to make it obvious ) larger steps cause "sizzling noise"

	// test-case: Touch_Me.sid - for some reason avoiding the "division by zero"
	// actually breaks this song..

//	double step = ((double)256.0) / (v->freq_inc_sample / (1<<16));	// Hermit's original shift-impl was prone to division-by-zero

	// simple pulse, most often used waveform, make it sound as clean as possible without oversampling
	double lim;
	int32_t wfout;
	if (tmp < pw) {	// rising edge (i.e. 0x0 side)	(in scope this is: TOP)
		lim = (0xffff - pw) * _freq_pulse_step;
		if (lim > 0xffff) { lim = 0xffff; }
		wfout = lim - (pw - tmp) * _freq_pulse_step;
		if (wfout < 0) { wfout = 0; }
	} else { //falling edge (i.e. 0xffff side)		(in scope this is: BOTTOM)
		lim = pw * _freq_pulse_step;
		if (lim > 0xffff) { lim = 0xffff; }
		wfout = (0xffff - tmp) * _freq_pulse_step - lim;
		if (wfout >= 0) { wfout = 0xffff; }
		wfout &= 0xffff;
	}
	return wfout;	// like "plain 2)"; NOTE: Hermit's original may actually be flipped due to filter.. which might cause problems with deliberate clicks.. see Geir Tjelta's feedback
//	return 0xffff - wfout; // like "plain 1)"
}
#else
void WaveGenerator::updatePulseCache() {
	_pulse_width12_neg = 0xfff000 - _pulse_width12;
	_pulse_width12_plus = _pulse_width12 + _freq_inc_sample; // used to check PW condition for previous sample
}

uint16_t WaveGenerator::createPulseOutput() {
	if (_test_bit) return 0xffff;	// pulse start position

	// note: at the usual sample-rates (<50kHz) the shortest
	// pulse repetition interval takes about 10 samples, i.e.
	// a sample depends on data from (usually) one or at most two such
	// intervals. The smallest spikes that can be selected via
	// the pulse-width are significantly shorter, i.e. they may last
	// just one clock-cycle and easily fit within a sample
	// (a respective spike may then be surrounded on both
	// sides by the respective inverted signal). The below impl
	// tries to generate output that mimicks the results that would
	// be achived when brute-force oversampling the output cycle-by-cycle.

	// there still are some relatively small errors as compared to the
	// resampled signal (maybe due to the variing integer cycles per sample
	// e.g. alternating 22/23 cycles, or some other rounding and/or precision
	// issue.

	// handle start of a new pulse (i.e. high to low toggle)

	if (_counter < _freq_inc_sample) {
		// pulse was reset less than _freq_inc_sample ago:
		// i.e. transition from high to low within this sample

		if(_counter < _pulse_width12) {
			// 1a) new pulse is still low (i.e. it was high for some time
			// during the previous pulse and until now still has been low
			// for the current pulse

			// _counter reflects the freq increments that have already been
			// applied so far:
			uint32_t np = _counter / _freq;		// number of clocks spent in the new pulse
			double op = _cycles_per_sample - np;// number of clocks spent in the old pulse

			// how many clocks can the PULSE be HIGH with the current FREQ?
			double highc = _pulse_width12_neg / _freq;
			if (highc > op) highc = op;

			return highc * _ffff_cycles_per_sample_inv;
		} else {
			// 1b) pulse already toggeled back to HIGH, i.e. there was but a
			// short LOW-spike during the start of this sample (since even the
			// shortest PULSE-duration is multipe samples long, the signal must have
			// been HIGH for all of the previous sample)
			// (testcase: _freq = 0xffff;_pulse_width = 0xff;)

			uint32_t lowc = _pulse_width12 / _freq;
			double highc = _cycles_per_sample - lowc;

			return highc * _ffff_cycles_per_sample_inv;
		}
	}

	// handle lo/hi transition within the current pulse (the 2 "new pulse" scenarios
	// are already covered above, i.e. the only scenario left here is the signal toggle
	// from low to hi based on the pulse-width.. (this also means that here the
	// signal was already low during the previous sample)
	else if ((_counter > _pulse_width12) && (_counter  < _pulse_width12_plus)) {

		double highc = (_counter - _pulse_width12) / _freq;
		return highc * _ffff_cycles_per_sample_inv;
	}

//	return (_counter >> 12) < _pulse_width ? 0 : 0xffff;	// plain pulse
	return _counter < _pulse_width12 ? 0 : 0xffff;	// plain pulse
}
#endif

uint16_t WaveGenerator::pulseOutput() {
#ifdef USE_HERMIT_ANTIALIAS
	uint32_t tmp, pw;	// 16 bits used
	calcPulseBase(&tmp, &pw);

	uint16_t o = createPulseOutput(tmp, pw);
#else
	uint16_t o = createPulseOutput();
#endif
	SAMPLE_END();
	return o;
}

// ---------------------------------------------------------------------------------------------
// ------ combined waveforms                                                          ----------
// ---------------------------------------------------------------------------------------------
uint16_t WaveGenerator::triangleSawOutput() {
	// TRIANGLE & SAW - like in Garden_Party.sid
	uint16_t o = combinedWF(_wave_table.TriSaw_8580, _counter >> 12, 1);			// 12 MSB needed
	SAMPLE_END();
	return o;
}

uint16_t WaveGenerator::pulseTriangleOutput() {
	uint16_t plsout;
#ifdef USE_HERMIT_ANTIALIAS
	uint32_t tmp, pw;	// 16 bits used
	calcPulseBase(&tmp, &pw);
	plsout =  ((tmp >= pw) || _test_bit) ? 0xffff : 0; //(this would be enough for simple but aliased-at-high-pitches pulse)
#else
//	plsout = createPulseOutput();	// plain should be good enough
	plsout =  ((_counter >> 12 >= _pulse_width) || _test_bit) ? 0xffff : 0;
#endif

	// PULSE & TRIANGLE - like in Kentilla, Convincing, Clique_Baby, etc
	// a good test is Last_Ninja:6 voice 1 at 35secs; here Hermit's
	// original PulseSaw settings seem to be lacking: the respective
	// sound has none of the crispness nor volume of the original

	uint32_t c = GET_RINGMOD_COUNTER();
	uint16_t o =  plsout ? combinedWF(_wave_table.PulseTri_8580, (c ^ (c & 0x800000 ? 0xffffff : 0)) >> 11, 0) : 0;	// either on or off

	SAMPLE_END();
	return o;
}

uint16_t WaveGenerator::pulseTriangleSawOutput() {
	// PULSE & TRIANGLE & SAW	- like in Lenore.sid
	uint16_t plsout;
#ifdef USE_HERMIT_ANTIALIAS
	uint32_t tmp, pw;	// 16 bits used
	calcPulseBase(&tmp, &pw);
	plsout =  ((tmp >= pw) || _test_bit) ? 0xffff : 0; //(this would be enough for simple but aliased-at-high-pitches pulse)
	uint16_t o =  plsout ? combinedWF(_wave_table.PulseTriSaw_8580, tmp >> 4, 1) : 0;	// tmp 12 MSB
#else
//	plsout = createPulseOutput();	// plain should be good enough
	plsout =  ((_counter >> 12 >= _pulse_width) || _test_bit) ? 0xffff : 0;
	uint16_t o =  plsout ? combinedWF(_wave_table.PulseTriSaw_8580, _counter >> 12, 1) : 0;	// 12 MSB needed
#endif
	SAMPLE_END();
	return o;
}

uint16_t WaveGenerator::pulseSawOutput() {
	// PULSE & SAW - like in Defiler.sid, Neverending_Story.sid
	uint16_t plsout;
#ifdef USE_HERMIT_ANTIALIAS
	uint32_t tmp, pw;	// 16 bits used
	calcPulseBase(&tmp, &pw);
	plsout =  ((tmp >= pw) || _test_bit) ? 0xffff : 0; //(this would be enough for simple but aliased-at-high-pitches pulse)
	uint16_t o =   plsout ? combinedWF(_wave_table.PulseSaw_8580, tmp >> 4, 1) : 0;	// tmp 12 MSB
#else
//	plsout = createPulseOutput();	// plain should be good enough
	plsout =  ((_counter >> 12 >= _pulse_width) || _test_bit) ? 0xffff : 0;
	uint16_t o =   plsout ? combinedWF(_wave_table.PulseSaw_8580, _counter >> 12, 1) : 0;	// 12 MSB needed
#endif
	SAMPLE_END();
	return o;
}

// Hermit's impl to calculate combined waveforms (check his jsSID-0.9.1-tech_comments
// in commented jsSID.js for background info): I did not thoroughly check how well
// this really works (it works well enough for Kentilla and Clique_Baby (apparently
// this one has to sound as shitty as it does)
uint16_t WaveGenerator::combinedWF(double* wfarray, uint16_t index, uint8_t differ6581) {
	//on 6581 most combined waveforms are essentially halved 8580-like waves

	if (differ6581 && _sid->_is_6581) index &= 0x7ff;	// todo: add getter for _sid var or replicate into WaveGenerator
	/* orig
	double combiwf = (wfarray[index] + _prev_wav_data) / 2;
	_prev_wav_data = wfarray[index];
	return (uint16_t)round(combiwf);
	*/

	// optimization?
	uint32_t combiwf = ((uint32_t)(wfarray[index] + _prev_wav_data)) >> 1;	// missing rounding might not make much of a difference
	_prev_wav_data = wfarray[index];
	return (uint16_t)combiwf;
}

// rather dispatch once here than repeat the checks for each
// sample.. note: once again the effects of the browser's "random performance behavior"
// seem to be bigger than the eventual optimization effects achieved here, i.e.
// the exact same version may run 30% faster/slower between successive runs (due to whatever
// other shit the browser might be doing in the background - or maybe due to other Win10
// processes interfering with the measurements..)
#define SET_OUTPUT_FUNC(wf_bits) \
	switch (wf_bits) {\
		case 0:\
			getOutput = &WaveGenerator::nullOutput;\
			break;\
		case TRI_BITMASK:\
			getOutput = &WaveGenerator::triangleOutput;\
			break;\
		case SAW_BITMASK:\
			getOutput = &WaveGenerator::sawOutput;\
			break;\
		case PULSE_BITMASK:\
			getOutput = &WaveGenerator::pulseOutput;\
			break;\
		case NOISE_BITMASK:\
			getOutput = &WaveGenerator::noiseOutput;\
			break;\
					\
		/* commonly used combined waveforms */\
		case TRI_BITMASK|SAW_BITMASK:\
			getOutput = &WaveGenerator::triangleSawOutput;\
			break;\
		case PULSE_BITMASK|TRI_BITMASK:\
			getOutput = &WaveGenerator::pulseTriangleOutput;\
			break;\
		case PULSE_BITMASK|TRI_BITMASK|SAW_BITMASK:\
			getOutput = &WaveGenerator::pulseTriangleSawOutput;\
			break;\
		case PULSE_BITMASK|SAW_BITMASK:\
			getOutput = &WaveGenerator::pulseSawOutput;\
			break;\
					\
		default:\
			getOutput = &WaveGenerator::noiseOutput;\
			break;\
	}

uint8_t	WaveGenerator::getOsc() {
	// What is sometimes incorrectly referred to as the "value of the oscillator" is indeed
	// the top 8 bits of the  waveform output. For practical purposes combined WFs seem to
	// be irrelevant here and the below incorrect approximation seems to be good enough
	// (see anti-aliasing related issues below though).

	uint16_t outv = 0xffff;

	if (_wf_bits == 0)   {
		outv = nullOutput0();	// testcase: Synthesis.sid
	} else {
		if (_wf_bits & TRI_BITMASK) {
			outv &= createTriangleOutput();
		}
		// CAUTION: anti-aliasing MUST NOT be used here  (testcase: we_are_demo_tune_2)
		if (_wf_bits & SAW_BITMASK) {
			outv &= _counter >> 8;							// plain saw
		}
		if (_wf_bits & PULSE_BITMASK) {
			outv &= ((_test_bit == 0) && (_counter < _pulse_width12)) ? 0 : 0xffff;	// plain pulse
		}
		if (_wf_bits & NOISE_BITMASK)   {
			outv &= _noiseout;
		}
	}

	return outv >> 8;
}


WaveGenerator::WaveGenerator(SID* sid, uint8_t voice_idx) {
	_sid = sid;
	_voice_idx = voice_idx;

//	reset();
}

void WaveGenerator::reset(double cycles_per_sample) {
	_cycles_per_sample = cycles_per_sample;

    _counter = _freq = _msb_rising = _ctrl =_wf_bits = _test_bit = _sync_bit = _ring_bit = _noise_bit = 0;
	_pulse_width = _pulse_width12 = 0;

#ifdef USE_HERMIT_ANTIALIAS
	_pulse_out = _freq_pulse_base = _freq_pulse_step = _freq_saw_step = 0;
#else
	_freq_inc_sample_inv = _ffff_freq_inc_sample_inv = _ffff_cycles_per_sample_inv = 0;
	_pulse_width12_neg = _pulse_width12_plus = 0;
#endif

	_trigger_noise_shift = 0;
	_noise_LFSR = NOISE_RESET;
	activateNoiseOutput(); // init _noiseout


	_freq_inc_sample = _prev_wav_data = 0;

	setMute(0);

	_floating_null_wf = 0;

	getOutput = &WaveGenerator::nullOutput;
}

void WaveGenerator::setMute(uint8_t is_muted) {
	_is_muted = is_muted;
}

uint8_t	WaveGenerator::isMuted() {
	return _is_muted;
}

void WaveGenerator::setWave(const uint8_t new_ctrl) {
	const uint8_t	old_ctrl = _ctrl;
	const uint8_t	old_noise_bit = _noise_bit;
	const uint8_t	new_noise_bit = new_ctrl & NOISE_BITMASK;

	const uint8_t new_wf_bits = new_ctrl & WF_BITMASK;

	if (_wf_bits && (new_wf_bits == 0)) {
		// when WF selector is set to 0, the output enters into a "floating mode"
		// (see "docs/floating-waveform.txt" for details)
		_floating_null_wf = ((this)->*(this->getOutput))();// crappy C++ syntax for calling the "getOutput" method;
		_floating_null_ts = SYS_CYCLES() + NULL_FLOAT_DURATION;
	} else {
		INIT_NOISE_OVERSAMPLING(old_noise_bit, new_noise_bit);
	}

	const uint8_t	old_test_bit = _test_bit;
	const uint8_t	new_test_bit = (new_ctrl & TEST_BITMASK);

	if (old_test_bit != new_test_bit) {
		if (new_test_bit) {
			_counter = 0;	// rather do this here once - instead of repeatedly during clocking..
			resetNoiseGenerator();
		} else {
			shiftNoiseRegisterTestBitDriven(new_ctrl);
		}
	}

	_ctrl = new_ctrl;
	// performance optimization: read much more often then written
	_wf_bits = new_wf_bits;
	_test_bit = new_test_bit;
	_sync_bit = new_ctrl & SYNC_BITMASK;
	_ring_bit = new_ctrl & RING_BITMASK;
	_noise_bit = new_noise_bit;

	SET_OUTPUT_FUNC(_wf_bits);
}

void WaveGenerator::setPulseWidthLow(const uint8_t val) {
	_pulse_width = (_pulse_width & 0x0f00) | val;
	_pulse_width12 = ((uint32_t)_pulse_width) << 12;	// for direct comparisons with 24-bit osc accumulator
	
#ifdef USE_HERMIT_ANTIALIAS
	// 16 MSB pulse needed (input is 12-bit)
	_pulse_out = (uint32_t)(_pulse_width * SCALE_12_16);
#else
	updatePulseCache();
#endif
}

void WaveGenerator::setPulseWidthHigh(const uint8_t val) {
	_pulse_width = (_pulse_width & 0xff) | ((val & 0xf) << 8);
	_pulse_width12 = ((uint32_t)_pulse_width) << 12;	// for direct comparisons with 24-bit osc accumulator

#ifdef USE_HERMIT_ANTIALIAS
	// 16 MSB pulse needed (input is 12-bit)
	_pulse_out = (uint32_t)(_pulse_width * SCALE_12_16);
#else
	updatePulseCache();
#endif
}


void WaveGenerator::updateFreqCache() {

	_freq_inc_sample = _cycles_per_sample * _freq;	// per 1-sample interval (e.g. ~22 cycles)
#ifndef USE_HERMIT_ANTIALIAS
	_freq_inc_sample_inv = 1.0 / _freq_inc_sample; 	// use faster multiplications later
	_ffff_freq_inc_sample_inv = ((double)0xffff) / _freq_inc_sample;
	_ffff_cycles_per_sample_inv = ((double)0xffff) / _cycles_per_sample;

	// optimization for saw
	uint16_t saw_sample_step = ((uint32_t)_freq_inc_sample) >> 8;	// less than 16-bits
	_saw_range = 0xffff - saw_sample_step;
	_saw_base = saw_sample_step + _saw_range;		// includes  +saw_sample_step/2 error (same as in regular signal)
#endif
#ifdef USE_HERMIT_ANTIALIAS
	// optimization for Hermit's waveform generation:
	_freq_saw_step = _freq_inc_sample / 0x1200000;			// for SAW  (e.g. 51k/0x1200000	-> 0.0027)

	_freq_pulse_base = ((uint32_t)_freq_inc_sample) >> 9;	// for PULSE: 15 MSB needed

	uint32_t d= (((uint32_t)_freq_inc_sample) >> 16);
	_freq_pulse_step = d ? 256 / d : 0.0;	// testcase: Dirty_64, Ice_Guys, Wonderland_XIII_tune_1 (digis at 2:49)

#else
	updatePulseCache();
#endif
}

void WaveGenerator::setFreqLow(const uint8_t val) {
	_freq = (_freq & 0xff00) | val;
	updateFreqCache();	// perf opt
}

void WaveGenerator::setFreqHigh(const uint8_t val) {
	_freq = (_freq & 0xff) | (val << 8);
	updateFreqCache();	// perf opt
}

uint8_t WaveGenerator::getWave() {
	return _ctrl;
}
uint16_t WaveGenerator::getFreq() {
	return _freq;
}
uint16_t WaveGenerator::getPulse() {
	return _pulse_width;
}

void WaveGenerator::clockPhase1() {
	// 2-phase clocking required since all oscillators must have been clocked before
	// any oscillator syncing can be performed in clockPhase2

	if (_test_bit) {
		// The TEST bit resets (see setWave()) and locks oscillator at zero
		// (i.e. no update here) until the TEST bit is cleared. The noise waveform
		// output is also reset and the pulse waveform output is held at a DC level
		// (handled in waveform impls)

		if (_noise_reset_ts == SYS_CYCLES()) {
			refillNoiseShiftRegister();
		}
	} else {
		uint32_t prev_counter = _counter;

		_counter = (_counter + _freq) & 0xffffff;

		// base for hard sync
		_msb_rising = (_counter & 0x800000) > (prev_counter & 0x800000);

		CLOCK_NOISE_GENERATOR();
	}
}

void WaveGenerator::clockPhase2() {
	// sync the oscillators: "hard sync" is accomplished by clearing the accumulator
	// of an oscillator based on the accumulator MSB of the previous oscillator.

	// tests for hard sync:  Ben Daglish's Wilderness music from The Last Ninja
	// (https://www.youtube.com/watch?v=AbBENI8sHFE) .. seems to be used on
	// voice 2 early on.. for some reason the instrument that starts on voice 1
	// at about 45 secs (some combined waveform?) does not sound as crisp as it
	// should.. (neither in Hermit's player)

	// intro noise in Martin Galway's Roland's Rat Race
	// (https://www.youtube.com/watch?v=Zc91S1lrU1I) music.

	// the below logic is from the "previous oscillator" perspective

	if (!_freq) return;

	if (_msb_rising) {	// perf opt: use nested "if" to avoid unnecessary calcs up front
		WaveGenerator *dest_voice = _sid->getWaveGenerator(NEXT_IDX(_voice_idx));
		uint8_t dest_sync = dest_voice->_sync_bit;	// sync requested?

		if (dest_sync) {
			// exception: when sync source is itself synced in the same cycle then
			// destination is NOT synced (based on analysis performed by reSID team)

			if (!(_sync_bit && _sid->getWaveGenerator(PREV_IDX(_voice_idx))->_msb_rising)) {
				dest_voice->_counter = 0;
			}
		}
	}
}
