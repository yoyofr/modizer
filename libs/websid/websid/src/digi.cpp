/*
* This file handles the detection of digi-samples.
*
* Ideally - i.e. if the SID emulation covered ALL the undocumented HW feature
* perfectly - then this logic here would be obsolete: The emulation then would
* know what extra voltage peaks may be temporarilly induced by the use of
* certain chip features and it would include those parameters for its output
* calculation.
*
* But currently this isn't modeled in the SID emulation and this here is a
* "poor man's" prothesis to deal with certain known scenarios. (So that they
* can be merged on top of the regular SID output.)
*
* Some of the techniques (e.g. pulse modulation based) would cause the
* imperfect SID emulation to generate wheezing noises (that may or may not be
* a problem on the real hardware). In case that the use of a respective technique
* is detected, the regular SID output is disabled to suppress that effect.
*
* Note: In the old predictive Tiny'R'Sid implementation this logic had been
* performed out of sync with the SID emulation but this is no longer the case
* with the current cycle-by-cycle emulation. (There still might be leftovers
* from that old implementation that have not been completely cleaned up yet.)
*
* Known issue: NMI players that reset D418 in their IRQ may lead to periodic
* clicks. This is probably due to the currently used ~22 cycles wide sampling 
* interval which might cause an IRQ
* setting to be used for a complete sample even if it is actually reset from
* the NMI much more quickly.
*
* Regarding D418 digis:
*
*  D418 digis are created by modulating the SID's output and while there is
*  no output this technique will NOT produce audible sounds either. This is
*  why respective songs that do not also create some "carrier base signal" may
*  not be audible on new 8580 SID models. Only when there is some base output
*  signal (i.e. not 0) the D418 alternation will modulate that signal to create
*  the audible digi playback. Old 6581 SID's are special in that they always
*  produce a positive base signal, i.e. their idle-signal (which is almost 0 on
*  new 8580 models) already produces an offset big enough to create audible d418
*  digis.
*
*  On the real HW the "carrier base signal"/"voice output" is always some kind
*  of positive voltage that is then rescaled by the D418 volume. But in the
*  current emulator implementation respective voice output uses signed values -
*  which causes negative values to result in an inversion of the intended digi
*  signal. As a workaround the current implementation shifts the 6581 output
*  into the positive range.
*
*  For regular d418-digis (except Mahoney's special case which is handled
*  specifically to improve output quality) there is no need to add any separate
*  digi-signal to the output. The modulated voice-output will be all
*  that is needed.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.94
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include "digi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern "C" {
#include "system.h"		// sysCycles()
#include "memory.h"		// only needed for PSID crap
#include "memory_opt.h"
}
#include "sid.h"
#include "filter.h"

#define MASK_DIGI_UNUSED 0x80
#define CENTER_SAMPLE 0x80

DigiDetector::DigiDetector(SID* sid) {
	_sid = sid;
	_base_addr = 0;	// not available at this point
	_digi_source = MASK_DIGI_UNUSED;
	_is_compatible = 1;
	_is_rsid = 1;
}

bool DigiDetector::isMahoney() {
	return _used_digi_type == DigiMahoneyD418;
}

int8_t DigiDetector::useOverrideDigiSignal(int32_t *digi_out, int32_t *dvoice_idx) {
	if ((_used_digi_type != DigiNone) && _digi_enabled) {

		(*digi_out) = getSample();	// used for "scope"

		// note: _used_digi_type can change within same song.. (see Storebror.sid)

		switch (_used_digi_type) {
			case DigiD418:
			case DigiMahoneyD418:
				// D418 needs no special handling since the effect is already
				//       achieved modulating the regular voice output
			
				break;
			case DigiFM:
				// FM sounds fine for most songs
				break;
			
			case DigiPWM:
			case DigiPWMTest:
				// PWM creates annoying carrier signal and used voices are therefore
				// muted - instead the final digi signal is directly fed back into the audio signal
				*dvoice_idx=  getSource() - 1;
				return 1;
			default:
				// do not show voice output based techniques in separate digi scope..
				break;
		}
	} else {
		(*digi_out) = 0;
	}
	(*dvoice_idx) = -1;
	return 0;
}

DigiType DigiDetector::getType() {
	return getRate() ? _used_digi_type : DigiNone;
}

const char* _typeDesc[7] = {
	"NONE",
	"D418",
	"M418",	// Mahoney's D418... something between 6-7 bits
	"FM",
	"PWM",
#ifdef DEBUG
	"PWMT",
#else
	"PWM",		// no point in using separate label for TEST-bit version
#endif
};

const char * DigiDetector::getTypeDesc() {
	return _typeDesc[getType()];
}

uint16_t DigiDetector::getRate() {
	return _digi_count > 13 ? _digi_count : 0;	// get rid of false positives
}

// detection of test-bit based samples

// note: Swallow's latest FM player (see Comaland_tune_3) uses 21 cycles..

const uint8_t TB0_TIMEOUT = 135; // testcase Super_Carling_the_Spider_credits: the interleaved use of 2 voices in this song results in
								// a 133 cycles delay of this step (normally in other songs the complete sequence takes less than 22 cyles)

const uint8_t TB_TIMEOUT = 22; // minimum that still works for "Vortex" is 10
const uint8_t TB_PULSE_TIMEOUT = 7; // reduced to avoid false positive in "Yie_ar_kung_fu.sid"


int32_t DigiDetector::getSample() {	// only for D418&PWM based samples (other techniques displayed directly in original voice)
	return _current_digi_sample;
}

int8_t DigiDetector::getSource() {
	return _current_digi_src;
}

void DigiDetector::recordSamplePWM(uint8_t sample, uint8_t voice_plus) {
	// PWM digis create annoying carrier signal that take the fun out of listening to 
	// respective songs. As a workaround the used voices are muted and the digi signal is 
	// instead added directly to the audio output

	// testcase: Wonderland_IX_part_9.sid
	_current_digi_sample = ((int32_t)sample * 0x101 - 0x8000);		// override regular signal 
	_current_digi_src = voice_plus;
	_digi_count++;
}

void DigiDetector::recordSampleD418(uint8_t sample) {
	
	_current_digi_sample = ((int32_t)sample * 0x101 - 0x8000) >> 1; // D418-digis need to be less loud.. (but not voice specific digis)
	_current_digi_src = 0;
	_digi_count++;

	// todo: it seems plausible to presume that updates done somewhere within
	// the interval of the current audio sample will lead to some kind
	// of aliasing, i.e. if the signal was "correctly" sampled every cycle
	// then the average across the sample should be different from the "last
	// written" value (which is currently used above).
	
	// status: an attempt to improve audio quality by simply interpolating the
	// value of the next rendered sample (by using the relative timing of the
	// update with regard to the sample's duration - see _sample_cycles in
	// core.cpp) failed spectacularly and added obvious distortions;
	// testcase: MyLife.sid
}

uint8_t DigiDetector::assertSameSource(uint8_t voice_plus) {
	// FIXME The switch to the cycle-by-cycle emulation has rendered this function
	// mostly obsolete.. the only scenario where it still is relevant are FM
	// digis where the IRQ still periodically sets D418

	// a song may perform different SID interactions that may or may not be
	// meant to produce digi-sample output. the same program may do both,
	// e.g. perform regular volume setting from MAIN or IRQ and also output
	// samples from NMI. Other programs actually output sample data from
	// their IRQ or MAIN. And some programs even do a mixed approach using
	// both NMI and IRQ to output samples (see some of THCM's stuff). Some
	// songs (e.g. Vicious_SID_2-15638Hz.sid) alternatingly use D418 and PWM
	// (on voice1 & voice2) from their MAIN to create sample output.

	// The goal here is to filter out/ignore false positives - which otherwise
	// may cause audible clicks.

	// assumption: if some "voice specific" approach is used, any D418 write
	// will NOT be interpreted as "sample output"..

	if (_digi_source != voice_plus) {
		// handle change in "digi source"
		
		if (_digi_source & MASK_DIGI_UNUSED) {
			// first "use" of an output sample
			_digi_source = voice_plus;		// correct it later if necessary
		} else {
			if (voice_plus == 0) {
				// d418 write while there is already other data.. just ignore
				return 0;

			} else if (_digi_source == 0) {
				// previously recorded D418 stuff is not really sample output
				// problem: is has already been rendered as a "sample"..
				// i.e. that cannot be undone

				_digi_count = 0;
				_digi_source = voice_plus;		// assumtion: only one digi voice..
			} else {
				// "non-D418" transition
				
				// accept voice switches - test-case: Vicious_SID_2-15638Hz.sid
				if (_used_digi_type == DigiFM) { // test-case: Busty_Cactus.sid
					return 0;	// trigger FM special case handling
				} else {
				}
			}
		}
	} else {
		// same source is OK
	}
	return 1;
}

// ------------------------------------------------------------------------------
// detection of test-bit/frequency modulation digi-sample technique (e.g.
// used in Vicious_SID_2-15638Hz.sid, Storebror.sid, Vaakataso.sid, Synthesis.sid, etc)

// the "beauty" of this approach is that the regular SID filters
// can still be applied to the digi-sample signal..

// it seems that as compared to PWM, this approach suffers much less from high pitched
// carrier signals (exception: Raveloop14_xm.sid) and directly replacing regular voice output 
// with the sample stream (i.e. disabling the regular voice) has many undesirable side 
// effects here (see songs like Soundcheck.sid that mix digis with other SID output in the same 
// voice) => use regular SID output 

// ------------------------------------------------------------------------------

uint8_t DigiDetector::isWithinFreqDetectTimeout0(uint8_t voice) {
	return (sysCycles() - _freq_detect_ts[voice]) < TB0_TIMEOUT;
}

uint8_t DigiDetector::isWithinFreqDetectTimeout(uint8_t voice) {
	return (sysCycles() - _freq_detect_ts[voice]) < TB_TIMEOUT;
}

uint8_t DigiDetector::recordFreqSample(uint8_t voice, uint8_t sample) {
	
	// current_digi_sample = ((int32_t)sample * 0x101 - 0x8000); // no need to directly record the digi-stream here
	_current_digi_src = voice + 1;
	_digi_count++;
	
	
	_freq_detect_state[voice] = FreqIdle;
	_freq_detect_ts[voice] = 0;

	_used_digi_type = DigiFM;

	// test-case: Storebror.sid (same voice is later used for regular output)
	// test-case: Synthesis.sid (makes other settings on voice during digi-playback)
	_fm_count++;
	
	return 1;
}

uint8_t DigiDetector::handleFreqModulationDigi(uint8_t voice, uint8_t reg, uint8_t value) {
	/*
	test-bit approach: the following settings are performed on the
	waveform/freq register in short order: 1) Triangle+GATE, 2) TEST+GATE
	3) GATE only 4) then the desired output sample is played by setting
	the "frequency hi-byte" (the whole sequence usually takes about 20-30
	cycles.. - exaxt limits still to be verified) .. possible variations: GATE
	is not set in step 2 and/or steps 3 and 4 are switched (see LMan - Vortex.sid)

	An unusual variation can be seen in Super_Carling_the_Spider_credits
	where 2 alternating NMIs are using 2 different voices for this..
	*/
	if (reg == 4) {	// waveform
		value &= 0x19;	// mask all excess bits..
		switch (value) {
			case 0x11:	// triangle/GATE (this should always be the 1st step!)
				// reset statemachine
				_freq_detect_state[voice] = FreqPrep;	// may be start of a digi playback
				_freq_detect_ts[voice] = sysCycles();
			break;
			case 0x8:	// TEST
			case 0x9:	// TEST/GATE
				if ((_freq_detect_state[voice] == FreqPrep) && isWithinFreqDetectTimeout0(voice)) {
					_freq_detect_state[voice] = FreqSet;	// we are getting closer
					
					// hack: reset base so that regular timeouts can be used below
					_freq_detect_ts[voice] = sysCycles() - 4;
				} else {
					_freq_detect_state[voice] = FreqIdle;	// just to reduce future comparisons
				}
				break;
			case 0x1:	// GATE
				if ((_freq_detect_state[voice] == FreqSet) && isWithinFreqDetectTimeout(voice)) {
					// variant 1: sample set after GATE
					_freq_detect_state[voice] = FreqVariant1;	// bring on that sample!
				} else if ((_freq_detect_state[voice] == FreqVariant2) && isWithinFreqDetectTimeout(voice)) {
					// variant 2: sample set before GATE
					return recordFreqSample(voice, _freq_detect_delayed_sample[voice]);
				} else {
					_freq_detect_state[voice] = FreqIdle;	// just to reduce future comparisons
				}
				break;
			case 0x0:	// testcase: Super_Carling_the_Spider_credits.sid
				if ((_freq_detect_state[voice] == FreqVariant2) && isWithinFreqDetectTimeout(voice)) {
					// variant 2: sample set before 0-WF
					return recordFreqSample(voice, _freq_detect_delayed_sample[voice]);
				}
				break;
		}
	} else if (reg == 1) {	// step: set sample
		if ((_freq_detect_state[voice] == FreqSet) && isWithinFreqDetectTimeout(voice)) {
			// variant 2: sample before GATE
			_freq_detect_delayed_sample[voice] = value;
			_freq_detect_state[voice] = FreqVariant2;	// now we only need confirmation
			_freq_detect_ts[voice] = sysCycles();
		} else if ((_freq_detect_state[voice] == FreqVariant1) && isWithinFreqDetectTimeout(voice)) {
			// variant 1: sample set after GATE (testcase: Synthesis.sid)
			return recordFreqSample(voice, value);
		}
	}
	return 0;	// other "detectors" may still take their turn
}

// ------------------------------------------------------------------------------
// detection of test-bit/pulse width modulation digi-sample technique
// (e.g. used in Wonderland_XII-Digi_part_1.sid, GhostOrGoblin.sid, etc)
// ------------------------------------------------------------------------------

uint8_t DigiDetector::isWithinPulseDetectTimeout(uint8_t voice) {
	return (sysCycles() - _pulse_detect_ts[voice]) < TB_PULSE_TIMEOUT;
}

uint8_t DigiDetector::recordPulseSample(uint8_t voice, uint8_t sample) {
 	if(assertSameSource(voice + 1))	recordSamplePWM(sample, voice + 1);

	_pulse_detect_state[voice] = PulseIdle;
	_pulse_detect_ts[voice] = 0;

	_used_digi_type = DigiPWMTest;
	return 1;
}

uint8_t DigiDetector::handlePulseModulationDigi(uint8_t voice, uint8_t reg, uint8_t value) {
	/*
	approach: the following settings are performed on the waveform/pulsewidth
	register in short order:
	1) desired output sample is played by setting the "pulse width" then
	2) PULSE+TEST+GATE, 3) PULSE+GATE (the whole sequence usually takes about
	20-30 cycles.. - exaxt limits still to be verified) variant: pulse-width
	is set between 2 and 3.

	e.g. used in "Wonderland XII - Digi (part4)"

	problem: Galway songs like Yie_Ar_Kung_Fu (see voice 2) are actually using
	the same sequence within regular song (reduce detection timeout to 7 cycles
	to avoid false positive).
	*/
	if (reg == 4) {	// waveform
		value &= 0x49;	// mask all excess bits..
		switch (value) {
			case 0x49:	// PULSE/TEST/GATE
				// test bit is set
				if ((_pulse_detect_state[voice] == PulsePrep) && isWithinPulseDetectTimeout(voice)) {
					_pulse_detect_state[voice] = PulseConfirm;	// we are getting closer
					_pulse_detect_ts[voice] = sysCycles();
				} else {
					// start of variant 2
					_pulse_detect_state[voice] = PulsePrep2;
					_pulse_detect_ts[voice] = sysCycles();
				}
				break;
			case 0x41:	// PULSE/GATE
				if (((_pulse_detect_state[voice] == PulseConfirm) || (_pulse_detect_state[voice] == PulseConfirm2))
						&& isWithinPulseDetectTimeout(voice)) {

			// 		I did not find any testcase for the "3" case
			//		uint8_t sample= (_pulse_detect_mode[voice] == 2) ?
			//						_pulse_detect_delayed_sample[voice] : 	// Wonderland_XII-Digi_part_1
			//						(_pulse_detect_delayed_sample[voice] << 4) & 0xff;

					uint8_t sample = _pulse_detect_delayed_sample[voice];

					// played waveform may actually be a problem here.. (maybe an envelope issue?)
					_sid->setMute(voice, 1);        // test-case: Wonderland_XII-Digi_part_1

					_pulse_detect_state[voice] = PulseIdle;	// just to reduce future comparisons
					return recordPulseSample(voice, sample);
				} else {
					_pulse_detect_state[voice] = PulseIdle;	// just to reduce future comparisons
				}
				break;
		}
	} else if (reg == 2/* || (reg == 3)*/) {	// PULSE width
		PulseDetectState followState;
		if ((_pulse_detect_state[voice] == PulsePrep2) && isWithinPulseDetectTimeout(voice)) {
			followState = PulseConfirm2;	// variant 2
		} else {
			followState = PulsePrep;		// variant 1
		}
		// reset statemachine
		_pulse_detect_state[voice] = followState;	// this may be the start of a digi playback
		_pulse_detect_ts[voice] = sysCycles();
		_pulse_detect_delayed_sample[voice] = value;
//		_pulse_detect_mode[voice] = reg;
	}
	return 0;	// other "detectors" may still take their turn
}

// ------------------------------------------------------------------------------
// Mahoney's D418 "8-bit" digi sample technique

// note: the regular D418 handling would meanwhile (with the cycle-by-cycle
// emulation) also play this, except that the result would propabably still
// be lower quality due to the flaws in the filter implementation.
// ------------------------------------------------------------------------------

// from Mahoney's amplitude_table_6581.txt (round((A1+0.869965)/1.863635 *255))
static const uint8_t _mahoneySample6581[256] = {
	119, 130, 140, 151, 161, 171, 180, 190, 202, 210, 218, 226, 234, 242, 249, 255, 
	118, 116, 113, 110, 108, 106, 103, 101, 98, 96, 94, 93, 91, 89, 87, 86, 119, 130, 
	140, 151, 161, 171, 180, 189, 201, 210, 218, 226, 234, 241, 248, 255, 119, 117, 
	116, 114, 113, 112, 111, 109, 108, 107, 106, 105, 104, 103, 102, 101, 119, 128, 
	136, 144, 153, 160, 168, 175, 185, 191, 198, 204, 211, 216, 222, 227, 118, 116, 
	113, 110, 108, 105, 103, 101, 98, 96, 94, 92, 91, 89, 87, 86, 119, 128, 137, 145, 
	154, 161, 169, 177, 186, 193, 200, 206, 213, 218, 224, 230, 118, 117, 115, 114, 
	112, 111, 110, 108, 107, 105, 104, 103, 102, 101, 100, 99, 119, 119, 119, 119, 
	119, 119, 119, 119, 120, 120, 120, 120, 120, 120, 120, 120, 118, 107, 97, 88, 
	79, 70, 62, 54, 44, 37, 30, 24, 17, 11, 6, 0, 119, 120, 122, 123, 124, 126, 127, 
	128, 130, 131, 132, 133, 134, 135, 136, 137, 118, 110, 102, 94, 87, 80, 74, 68, 
	60, 54, 49, 43, 38, 33, 29, 24, 119, 118, 118, 117, 117, 116, 116, 116, 115, 115, 
	114, 114, 114, 114, 113, 113, 118, 109, 100, 91, 83, 75, 68, 61, 52, 46, 40, 34, 
	28, 23, 17, 12, 119, 119, 120, 121, 122, 122, 123, 123, 124, 125, 125, 126, 127, 
	127, 128, 128, 118, 110, 103, 96, 90, 83, 77, 72, 64, 59, 54, 50, 45, 40, 36, 32
	};

// from Mahoney's amplitude_table_8580.txt
static const uint8_t _mahoneySample[256] = {
	164, 170, 176, 182, 188, 194, 199, 205, 212, 218, 224, 230, 236, 242, 248, 254,
	164, 159, 153, 148, 142, 137, 132, 127, 120, 115, 110, 105, 99, 94, 89, 84,
	164, 170, 176, 181, 187, 193, 199, 205, 212, 217, 223, 229, 235, 241, 246, 252,
	164, 159, 153, 148, 142, 137, 132, 127, 120, 115, 110, 105, 100, 94, 90, 85,
	164, 170, 176, 182, 188, 194, 200, 206, 213, 219, 225, 231, 237, 243, 249, 255,
	164, 159, 154, 149, 143, 138, 133, 128, 122, 117, 112, 107, 102, 97, 92, 87,
	164, 170, 176, 182, 188, 194, 199, 205, 212, 218, 224, 230, 236, 242, 248, 253,
	164, 159, 154, 149, 143, 138, 133, 128, 122, 117, 112, 107, 102, 97, 92, 87,
	164, 164, 164, 164, 164, 164, 164, 164, 163, 163, 163, 163, 163, 163, 163, 163,
	164, 153, 142, 130, 119, 108, 97, 86, 73, 62, 52, 41, 30, 20, 10, 0,
	164, 164, 164, 164, 164, 164, 163, 163, 163, 163, 163, 163, 163, 163, 162, 162,
	164, 153, 142, 131, 119, 108, 97, 87, 73, 63, 52, 42, 31, 21, 11, 1,
	164, 164, 164, 164, 164, 164, 164, 165, 165, 165, 165, 165, 165, 165, 165, 165,
	164, 153, 142, 131, 120, 109, 98, 88, 75, 64, 54, 44, 33, 23, 13, 3,
	164, 164, 164, 164, 164, 164, 164, 164,164, 164, 164, 164, 164, 164, 164, 164,
	164, 153, 142, 131, 120, 109, 99, 88, 75, 65, 55, 44, 34, 24, 14, 4
} ;

uint8_t DigiDetector::isMahoneyDigi() {
	// Mahoney's "8-bit" D418 sample-technique requires a specific SID setup

	// The idea of the approach is that the combined effect of the output of
	// all three voices (2 filtered, 1 unfiltered) creates a carrier base
	// signal what is modulated not only by the 4-bit volume but also by the
	// filter settings in D418 allowing for more than 4-bit digi resolution
	// (the name is misleading since the actual resolution is probably closer
	// to 6 or 7 bits..)

	// Either the voice-output OR the separately recorded input sample can be
	// used in the output signal - but not both! (using the later here)

	// We_Are_Demo_tune_2.sid seems to be using the same approach only the
	// SR uses 0xfb instead of 0xff

	// song using this from main-loop - testcase: Acid_Flashback.sid

	// testcase: 4_Non_Blondes-Whats_Up_Remix (uses 0xfe for d415/16)
	
	if ( (MEM_READ_IO(_base_addr + 0x17) == 0x3) && 	// voice 1&2 through filter
		 (MEM_READ_IO(_base_addr + 0x15) >= 0xfe) &&
		 (MEM_READ_IO(_base_addr + 0x16) >= 0xfe) &&		// "correct" filter cutoff
		 
		 (MEM_READ_IO(_base_addr + 0x06) >= 0xfb) &&
		 (MEM_READ_IO(_base_addr + 0x06) == MEM_READ_IO(_base_addr + 0x0d)) &&
		 (MEM_READ_IO(_base_addr + 0x06) == MEM_READ_IO(_base_addr + 0x14)) &&	// all same SR
		 
		 (MEM_READ_IO(_base_addr + 0x04) == 0x49) &&
		 (MEM_READ_IO(_base_addr + 0x0b) == 0x49) &&
		 (MEM_READ_IO(_base_addr + 0x12) == 0x49)  // correct waveform: pulse + test + gate 
		) {
			
		_used_digi_type = DigiMahoneyD418;
		return 1;
	}
	return 0;
}

// ------------------------------------------------------------------------------
// Swallow 'pulse width modulation': the players handled here use some PWM
// approach but without the more recent test-bit technique.. each player
// depends on specific frequency settings and differs in how sample values
// are transformed and then written as differently interpreted hi/lo pulse-
// width settings.. examples can be found from musicians like Swallow,
// Danko or Cyberbrain
// ------------------------------------------------------------------------------

uint8_t DigiDetector::setSwallowMode(uint8_t voice, uint8_t m) {
	_swallow_pwm[voice] = m;
	_sid->setMute(voice, 1);	// avoid wheezing base signals created by regular voice output

	return 1;
}

uint8_t DigiDetector::handleSwallowDigi(uint8_t voice, uint8_t reg,
										uint16_t addr, uint8_t value) {

	if (reg == 4) {
		if ((_sid->getWave(voice) & 0x8) && !(value & 0x8) && (value & 0x40)
				&& ((_sid->getAD(voice) == 0) && (_sid->getSR(voice) == 0xf0))) {

			// the tricky part here is that the tests here do not trigger for
			// songs which act similarily but which are not using "pulse width
			// modulation" to play digis (e.g. Combat_School.sid, etc)

			if ((_sid->getPulse(voice) == 0x0555) && (_sid->getFreq(voice) == 0xfe04)) {
				// e.g. Spasmolytic_part_2.sid
				return setSwallowMode(voice, 1);
			}  else if ((_sid->getPulse(voice) == 0x08fe) && (_sid->getFreq(voice) == 0xffff)) {
				// e.g. Sverige.sid, Holy_Maling.sid, Voodoo_People_part_*.sid
				return setSwallowMode(voice, 2);
			} else if ( (_sid->getPulse(voice)&0xff00) == 0x0800 ) {
				// e.g. Bla_Bla.sid, Bouncy_Balls_RCA_Intro.sid, Spasmolytic_part_6.sid
				// Bouncy_Balls_RCA_Intro.sid, Ragga_Run.sid, Wonderland_X_part_1.sid
				return setSwallowMode(voice, 3);
			}
		}
	} else if (_swallow_pwm[0] && (reg == 3)) {
		// depending on the specific player routine, the sample info is
		// available in different registers(d402/d403 and d409/a)..
		// for retrieval d403 seems to work for most player impls

		switch(_swallow_pwm[voice]) {
			case 1:
				recordSamplePWM((value & 0xf) * 0x11, voice + 1);
				break;
			case 2:
				recordSamplePWM(((value << 4) | (value >> 4)), voice + 1);	// trial & error
				break;
			case 3:
				recordSamplePWM((value & 0xf) * 0x11, voice + 1);
				break;
		}

		_used_digi_type = DigiPWM;

		return 1;
	}
	return 0;
}

// ------------------------ legacy PSID digi stuff ------------------------------
// (this is probably about the only code left from the original TinySID impl)

static int32_t _sample_active;
static int32_t _sample_position, _sample_start, _sample_end, _sample_repeat_start;
static int32_t _frac_pos = 0;  /* Fractal position of sample */
static int32_t _sample_period;
static int32_t _sample_repeats;
static int32_t _sample_order;
static int32_t _sample_nibble;

static int32_t _internal_period, _internal_order, _internal_start, _internal_end,
_internal_add, _internal_repeat_times, _internal_repeat_start;

static void handlePsidDigi(uint16_t addr, uint8_t value) {
	// "new" SID-register
	if ((addr > 0xd418) && (addr < 0xd500)) {
		// start-ji
		if (addr == 0xd41f) _internal_start = (_internal_start & 0x00ff) | (value << 8);
		
		// start-lo
		if (addr == 0xd41e) _internal_start = (_internal_start & 0xff00) | (value);
		
		// repeat-hi
		if (addr == 0xd47f) _internal_repeat_start = (_internal_repeat_start & 0x00ff) | (value << 8);
		
		// repeat-lo
		if (addr == 0xd47e) _internal_repeat_start = (_internal_repeat_start & 0xff00) | (value);

		// end-hi
		if (addr == 0xd43e) {
			_internal_end = (_internal_end & 0x00ff) | (value << 8);
		}
		
		// end-lo
		if (addr == 0xd43d) {
			_internal_end = (_internal_end & 0xff00) | (value);
		}
		
		// loop-size
		if (addr == 0xd43f) _internal_repeat_times = value;
		
		// period-hi
		if (addr == 0xd45e) _internal_period = (_internal_period & 0x00ff) | (value << 8);
		
		// period-lo
		if (addr == 0xd45d) {
			_internal_period = (_internal_period & 0xff00) | (value);
		}
		// sample prder
		if (addr == 0xd47d) _internal_order = value;
		
		// sample add
		if (addr == 0xd45f) _internal_add = value;
		
		// start-sampling
		if (addr == 0xd41d) {
			_sample_repeats = _internal_repeat_times;
			_sample_position = _internal_start;
			_sample_start = _internal_start;
			_sample_end = _internal_end;
			_sample_repeat_start = _internal_repeat_start;
			_sample_period = _internal_period;
			_sample_order = _internal_order;
			switch (value)
			{
				case 0xfd: _sample_active = 0; break;
				case 0xfe:
				case 0xff: _sample_active = 1; break;

				default: return;
			}
		}
	}
}

int32_t DigiDetector::genPsidSample(int32_t sample_in)
{
    static int32_t sample = 0;

    if (!_sample_active) return sample_in;

    if ((_sample_position < _sample_end) && (_sample_position >= _sample_start)) {

        sample_in += sample;

        _frac_pos += _clock_rate / _sample_period;

        if (_frac_pos > (int32_t)SID::getSampleFreq()) {
            _frac_pos %= SID::getSampleFreq();

			// fetch next sample-nibble
            if (_sample_order == 0) {
                _sample_nibble++;                        // next sample-nibble
                if (_sample_nibble == 2) {
                    _sample_nibble = 0;
                    _sample_position++;
                }
            } else {
                _sample_nibble--;
                if (_sample_nibble < 0) {
                    _sample_nibble = 1;
                    _sample_position++;
                }
            }
            if (_sample_repeats) {
                if  (_sample_position > _sample_end) {
                    _sample_repeats--;
                    _sample_position = _sample_repeat_start;
                } else {
					_sample_active = 0;
				}
            }

            sample = memReadRAM(_sample_position & 0xffff);
            if (_sample_nibble == 1) {  // fetch hi-nibble?
                sample = (sample & 0xf0) >> 4;
            } else {
				sample = sample & 0x0f;
			}
			// transform unsigned 4 bit range into signed 16 bit (?32,768 to 32,767) range
			sample = (sample << 11) - 0x3fc0;
        }
    }
    return sample_in;
}

uint8_t _slow_down = 1;
void DigiDetector::resetCount() {
	_slow_down = !_slow_down;
	if (_slow_down) {
		// FIXME reminder: the below "resetting" hack may lead to clicking noises in
		// respective songs and it should best be eliminated..
		// test-cases: Banditti_2SID.sid

		return;
	}

	if (_used_digi_type == DigiFM) {
		if (!_fm_count) {
			// test-case: Storebror.sid => switches back to other digi technique

			// note: the temporary absence of detected FM digis is NOT a valid
			// detection criteria for a regular use of the voice (the digi might
			// just be temporarily suspended).. but it seems to be good enough for now

			// testcase: Soundcheck.sid => sporadic blip due to "un-mute"

			_digi_source = MASK_DIGI_UNUSED;			// restart detection
			_used_digi_type = DigiNone;
		} else {
			_fm_count = 0;
		}
	} else if (_used_digi_type == DigiD418) {
		// test-case: Arkanoid.sid - doesn't use NMI at all
		if (_digi_count == 0) {
			_used_digi_type = DigiNone;
		}
	}
	_digi_count = 0;
}

void DigiDetector::setEnabled(uint8_t value) {
	_digi_enabled = value;
}

void DigiDetector::reset(uint32_t clock_rate, uint8_t is_rsid, uint8_t is_compatible) {

	_base_addr = _sid->getBaseAddr();

	_clock_rate = clock_rate;
	_is_rsid = is_rsid;
	_is_compatible = is_compatible;

	_digi_enabled = 1;

	memset( (uint8_t*)&_swallow_pwm, 0, sizeof(_swallow_pwm) );

	_digi_count = 0;
	_digi_source = MASK_DIGI_UNUSED;

	// PSID digi stuff
	_sample_active = _sample_position = _sample_start = _sample_end = _sample_repeat_start = _frac_pos =
		_sample_period = _sample_repeats = _sample_order = _sample_nibble = 0;
	_internal_period = _internal_order = _internal_start = _internal_end =
		_internal_add = _internal_repeat_times = _internal_repeat_start = 0;

	//	digi sample detection
	for (uint8_t i= 0; i<3; i++) {
		_freq_detect_state[i] = FreqIdle;
		_freq_detect_ts[i] = 0;
		_freq_detect_delayed_sample[i] = 0;

		_pulse_detect_state[i] = PulseIdle;
//		_pulse_detect_mode[i] = 0;
		_pulse_detect_ts[i] = 0;
		_pulse_detect_delayed_sample[i] = 0;

		_swallow_pwm[i] = 0;
	}

	_current_digi_sample = CENTER_SAMPLE * 0x101 - 0x8000;

	_current_digi_src = 0;

	_used_digi_type = DigiNone;
}

uint8_t DigiDetector::getD418Sample( uint8_t value) {
	/*
	The D418 "volume register" technique was probably the oldest of the
	attempts to play 4-bit digi samples on the C64.

	The output is based on 2 effects:

	1) Since it is the "volume register" that is used, the setting impacts
	whatever signal is being output by the three SID voices, i.e. the
	respective voice output is distorted accordingly. (Songs like "Arkanoid"
	use all three voices for melody and parts of the played digi-samples
	are audible based on the those distorted signals alone.

	2) On older 6581 SIDs there is an "idle" voltage offset that acts as a
	"carrier signal" even when there is no voice output. (on newer 8580 models
	this offset is close to 0 and this is the reason why early songs that
	depended on the offset alone, where barely audible on newer SID models)
	*/
	if (_used_digi_type == DigiMahoneyD418) {
		// better turn those voices back on
		_sid->setMute(0, 0);
		_sid->setMute(1, 0);
		_sid->setMute(2, 0);
	}
	_used_digi_type = DigiD418;

	return (value & 0xf) * 0x11;
}

// CAUTION: use of READ-MODIFY-WRITE OPs could mess up the detection since
// same register is written 2x within same cycle (correctly the writes would
// be sequential but that hasn't been implemented - testcase Soundcheck.sid
// which used SAX to write WF register)
uint8_t DigiDetector::detectSample(uint16_t addr, uint8_t value) {

	if (SID::isExtMultiSidMode()) return 0;	// optimization for multi-SID

	if (!_is_compatible) {
		handlePsidDigi(addr, value);
	} else {
		// mask out alternative addresses of d400 SID (see 5-Channel_Digi-Tune)..
		// use in PSID would crash playback of recorded samples
		if (SID::getNumberUsedChips() == 1) addr &= ~(0x3e0);

		// for songs like MicroProse_Soccer_V1.sid tracks >5 (PSID digis must
		// still be handled.. like Demi-Demo_4_PSID.sid)
		if (!_is_rsid) return 0;

		uint8_t reg = addr & 0x1f;
		uint8_t voice = 0;
		if ((reg >= 7) && (reg <=13)) { voice = 1; reg -= 7; }
		if ((reg >= 14) && (reg <=20)) { voice = 2; reg -= 14; }

		if (handleFreqModulationDigi(voice, reg, value)) return 1;
		if (handlePulseModulationDigi(voice, reg, value)) return 1;
		if (handleSwallowDigi(voice, reg, addr, value)) return 1;

		// normal handling
		if ((addr == (_base_addr + 0x18)) && assertSameSource(0)) {
			recordSampleD418(isMahoneyDigi() ? 
									(_sid->isSID6581() ? _mahoneySample6581[value] : _mahoneySample[value] )
									: getD418Sample(value));	// this may lead to false positives..
		}
	}
	return 0;
}
