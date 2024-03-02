/*
* Handles the wave output of a single voice of the C64's "MOS Technology SID" (see 6581, 8580 or 6582).
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.95
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_VOICE_H
#define WEBSID_VOICE_H

#include "base.h"


// Hermit's impls are quite a bit off as compared to respective oversampled
// signals. My new impl now should be much closer to the real thing.
//#define USE_HERMIT_ANTIALIAS


class WaveGenerator {
protected:
	friend class SID;								// the only user of Voice

	WaveGenerator(class SID* sid, uint8_t voice_idx);

	void reset(double cycles_per_sample);

	// 2-phase clocking as base for "hard-sync"
	void		clockPhase1();
	void		clockPhase2();

	void		setMute(uint8_t is_muted);
	uint8_t		isMuted();

	// access to related registers
	void		setWave(const uint8_t wave);
	uint8_t		getWave();

	void		setPulseWidthLow(const uint8_t val);
	void		setPulseWidthHigh(const uint8_t val);
	uint16_t	getPulse();

	void		setFreqLow(const uint8_t val);
	void		setFreqHigh(const uint8_t val);
	uint16_t	getFreq();

	// waveform generation
	uint16_t	(WaveGenerator::*getOutput)();	// try to save additional wrapper by using pointer directly..
	uint8_t		getOsc();

private:
	// utils for waveform generation
	void		updateFreqCache();
	uint16_t	combinedWF(double* wfarray, uint16_t index, uint8_t differ6581);
	uint16_t	createTriangleOutput();
	uint16_t	createSawOutput();

#ifdef USE_HERMIT_ANTIALIAS
	void		calcPulseBase(uint32_t* tmp, uint32_t* pw);
	uint16_t	createPulseOutput(uint32_t tmp, uint32_t pw);
#else
	void		updatePulseCache();
	uint16_t	createPulseOutput();
#endif
	void		activateNoiseOutput();
	uint16_t	combinedNoiseInput();

	void 		shiftNoiseRegisterNoTestBit();
	void 		shiftNoiseRegisterTestBitDriven(const uint8_t new_ctrl);

	void 		feedbackNoise(uint16_t out);
		// "shift register refill" feature
	void		resetNoiseGenerator();
	void		refillNoiseShiftRegister();


	// functions for specific waveform combinations (used via getOutput)
	uint16_t nullOutput();
	uint16_t nullOutput0();
	uint16_t triangleOutput();
	uint16_t sawOutput();
	uint16_t pulseOutput();
	uint16_t noiseOutput();

	uint16_t triangleSawOutput();
	uint16_t pulseTriangleOutput();
	uint16_t pulseSawOutput();

	uint16_t pulseTriangleSawOutput();

private:
	class SID*	_sid;
	uint8_t		_voice_idx;
	double		_cycles_per_sample;


	uint8_t		_is_muted;			// player's separate "mute" feature

	// base oscillator state
    uint32_t	_counter;			// aka "accumulator" (24-bit)
	uint16_t	_freq;				// counter increment per cycle
	uint8_t		_msb_rising;		// hard sync handling

	// waveform generation
    uint8_t		_ctrl;				// waveform control register
		// performance opt: redundant flags from _ctrl (at their original position)
	uint8_t		_wf_bits;
	uint8_t		_test_bit;
	uint8_t		_sync_bit;
	uint8_t		_ring_bit;
	uint8_t		_noise_bit;

	double		_freq_inc_sample;

		// pulse waveform
	uint16_t	_pulse_width;		// 12-bit "pulse width" from respective SID registers
	uint32_t	_pulse_width12;
#ifdef USE_HERMIT_ANTIALIAS
	uint32_t	_pulse_out;			// 16-bit "pulse width" _pulse_width<<4 shifted for convenience)
	uint32_t	_freq_pulse_base;
	double		_freq_pulse_step;

		// saw waveform
	double		_freq_saw_step;
#else
	double		_freq_inc_sample_inv;
	double		_ffff_freq_inc_sample_inv;
	double		_ffff_cycles_per_sample_inv;
	uint32_t	_pulse_width12_neg;
	uint32_t	_pulse_width12_plus;

		// saw waveform
	uint16_t	_saw_range;
	uint32_t	_saw_base;
#endif

		// noise waveform
    uint32_t	_noise_LFSR;
	uint32_t	_trigger_noise_shift;
    uint32_t	_noise_reset_ts;
			// sub-sample noise handling/resampling
    uint16_t	_noiseout;		// last wave output
	uint32_t	_ref0_ts;		// start of the current sample interval
	uint32_t	_ref1_ts;		// already handled part of the current sample interval
    uint32_t	_noiseout_sum;	// summed up noise used for interpolation



	// add-ons snatched from Hermit's implementation
	double		_prev_wav_data;		// combined waveform handling

	// floating wavegen
	uint16_t 	_floating_null_wf;
	uint32_t 	_floating_null_ts;
};

#endif