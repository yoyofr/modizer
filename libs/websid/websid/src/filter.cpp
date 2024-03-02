/*
* This file contains everything to do with the emulation of the SID chip's filter.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

// note on performance: see http://nicolas.limare.net/pro/notes/2014/12/12_arit_speed/

// example x86-64 (may differ depending on machine architecture):
// 278 Mips integer multiplication								=> 1
// 123 Mips integer division									=> 2.26 slower
// 138 Mips double multiplication (32-bit is actually slower)	=> 2.01 slower
// 67 Mips double division (32-bit is actually slower)			=> 4.15 slower

// example: non-digi song, means per frame volume is set 1x and then 882 samples (or more) are rendered
// 			- cost without prescaling: 882 * (1 + 2.26) =>	2875.32
//			- cost with prescaling: 4.15 + 882 *(4.15)	=>	3664.45
// 			=> for a digi song the costs for prescaling would be even higher

// => repeated integer oparations are usually cheaper when floating point divisions are the alternative

#include "filter.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>


const double SCOPE_SCALEDOWN =  0.5;
const double FILTERED_SCOPE_SCALEDOWN =  0.2;	// tuned using "424.sid"

#include "sid.h"

uint32_t Filter::_sample_rate;

Filter::Filter(SID* sid) {
	_sid = sid;
}

Filter::~Filter() {
}

void Filter::clearFilterState() {
	for (int i= 0; i<3; i++) {
		struct FilterState *state = &_voice[i];
		state->_lp_out = state->_bp_out = state->_hp_out = 0;
	}
}

void Filter::setSampleRate(uint32_t sample_rate) {
	_sample_rate = sample_rate;

	clearFilterState();
	resyncCache();	
}

/* Get the bit from an uint32_t at a specified position */
static bool getBit(uint32_t val, uint8_t idx) { return (bool) ((val >> idx) & 1); }

void Filter::clearSimOut(uint8_t voice_idx) {
	struct FilterState *state = &_sim_voice[voice_idx];
	state->_lp_out = state->_bp_out = state->_hp_out = 0;
}

void Filter::poke(uint8_t reg, uint8_t val) {
	switch (reg) {
        case 0x15: { _reg_cutoff_lo = val & 0x7;	resyncCache();  break; }
        case 0x16: { _reg_cutoff_hi = val;			resyncCache();  break; }
        case 0x17: {
				_reg_res_flt = val & 0xf7;	// ignore FiltEx
				resyncCache();

				for (uint8_t voice_idx= 0; voice_idx<3; voice_idx++) {
					_filter_ena[voice_idx] = getBit(val, voice_idx);

					// ditch cached stuff when filter is turned off
					if (!_filter_ena[voice_idx]) {
						clearSimOut(voice_idx);
					}
				}

			break;
			}
        case 0x18: {
				//  test-case Kapla_Caves and Immigrant_Song: song "randomly switches between
				// filter modes which will also occationally disable all filters:

				
				if (!(val & 0x70)) {
					clearFilterState();

					clearSimOut(0);
					clearSimOut(1);
					clearSimOut(2);
				}
#ifdef USE_FILTER
				_lowpass_ena =	getBit(val, 4);
				_bandpass_ena =	getBit(val, 5);
				_hipass_ena =	getBit(val, 6);

				_voice3_ena = !getBit(val, 7);		// chan3 off

				_is_filter_on = (val & 0x70) != 0;	// optimization
#endif
			break;
			}
	};
}

uint8_t Filter::isSilencedVoice3(uint8_t voice_idx) {
	return ((voice_idx == 2) && (!_voice3_ena) && (!_filter_ena[2]));
}

int32_t Filter::getVoiceOutput(int32_t voice_idx, int32_t* in) {
	FilterState *s= &_voice[voice_idx];
	int32_t out;
	
#ifdef USE_FILTER
	// regular routing
	if (_filter_ena[voice_idx] && _is_filter_on) {
		// route to filter
		out= doGetFilterOutput(*in, &s->_bp_out, &s->_lp_out, &s->_hp_out);
	} else {
		// route directly to output
		out= *in;
	}
#else
	// don't use filters
	if (*not_muted) {
		out = *in;
	}
#endif
	return out;
}


// note: in order to get a nicely centered graph, unfortunately the filter calcs
// have to be repeated (the alternative would be to compensate SID specific offsets - which would 
// be a pain in the ass due to filter specific signal flipping)

int32_t Filter::getVoiceScopeOutput(int32_t voice_idx, int32_t* in) {
	FilterState *s= &_sim_voice[voice_idx];
	
	int32_t out;
	
#ifdef USE_FILTER
	// regular routing
	if (_filter_ena[voice_idx] && _is_filter_on) {
		// route to filter
		out= doGetFilterOutput(*in, &s->_bp_out, &s->_lp_out, &s->_hp_out) * FILTERED_SCOPE_SCALEDOWN;
	} else {
		// route directly to output
		out= *in * SCOPE_SCALEDOWN;
	}
#else
	// don't use filters
	if (*not_muted) {
		out = *in * SCOPE_SCALEDOWN;
	}
#endif
	return out;
}

