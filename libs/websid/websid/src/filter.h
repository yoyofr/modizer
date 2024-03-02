/*
* Poor man's emulation of the C64 SID's filter.
*
* Derived from Hermit's filter implementation: see http://hermit.sidrip.com/jsSID.html
* with "distortion" add-ons inspired by Antti Lankila's work here 
* https://bel.fi/alankila/c64-sw/index-cpp.html. The "external filter" is based 
* on the analysis by the resid team.
*
* Known limitation: In the current implementation the filter is fed with the output
* sample rate (which may be 22x slower than the clock speed actually used by the SID),
* i.e. higher frequency changes are totally ignored - which may lead to Moiré-effects
* and it may distort what the filter would correctly be doing (see fast changing
* waveforms like "noise" or certain combined-waveforms). However given the fact that
* certain users already complain that the existing implementation is "too slow" (on
* their old smartphone or iPad) the quality trade-off seems to be preferable at this
* point.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_FILTER_H
#define WEBSID_FILTER_H

// switch to completely disable use of the filter
#define USE_FILTER

extern "C" {
#include "base.h"
}

struct FilterState {
	double _lp_out;	// previous "low pass" output
	double _bp_out;	// previous "band pass" output
	double _hp_out;	// previous "hi pass" output
};

/**
* This class handles the filter of a SID chip.
*
* It is a construct exclusively used by the SID class and access is restricted accordingly.
*/
class Filter {
protected:
	Filter(class SID* sid);
	virtual ~Filter();
		
	void setSampleRate(uint32_t sample_rate);

	int32_t getVoiceOutput(int32_t voice_idx, int32_t* in);
	int32_t getVoiceScopeOutput(int32_t voice_idx, int32_t* in);
	
	/**
	* Handle those SID writes that impact the filter.
	*/
	void poke(uint8_t reg, uint8_t val);

	uint8_t isSilencedVoice3(uint8_t voice_idx);


	/**
	* Hooks that must be defined in subclasses.
	*/
	virtual void resyncCache() = 0;
	virtual double doGetFilterOutput(double sum_filter_in, double* band_pass, double* low_pass, double* hi_pass) = 0;

private:
	friend class SID;
	friend class DigiDetector;
	void clearSimOut(uint8_t voice_idx);

	class SID* _sid;

	void clearFilterState();
	
protected:
	static uint32_t _sample_rate;		// target playback sample rate

	// register input
	uint8_t _reg_cutoff_lo;		// filter cutoff low (3 bits)
	uint8_t _reg_cutoff_hi;		// filter cutoff high (8 bits)
	uint8_t _reg_res_flt;		// resonance (4bits) and filter bits

		// control flags from registers
	bool  _lowpass_ena;
	bool  _bandpass_ena;
	bool  _hipass_ena;
		
	// term used in biquad (1/Q)
	double _resonance;
		
private:
		// control flags from registers	
	bool  _is_filter_on;
	bool _filter_ena[3];	// filter activation per voice

	bool  _voice3_ena;		// special "voice 3 feature"

	// internal state of regular filter

		// derived from Hermit's filter implementation: see http://hermit.sidrip.com/jsSID.html
	struct FilterState _voice[3];

		// filter output for "scope view" visialization output of the voices
	struct FilterState _sim_voice[3];
};


#endif
