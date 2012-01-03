/**
 * This file is part of reSID, a MOS6581 SID emulator engine.
 * Copyright (C) 2004  Dag Lem <resid@nimrod.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * @author Ken Händel
 *
 */

#ifndef WAVEFORMGENERATOR_H
#define WAVEFORMGENERATOR_H

#include "siddefs-fp.h"
#include "array.h"

namespace reSIDfp
{

/**
 * A 24 bit accumulator is the basis for waveform generation. FREQ is added to the lower 16 bits of the accumulator each cycle. The accumulator is set to zero when TEST is set, and starts counting
 * when TEST is cleared. The noise waveform is taken from intermediate bits of a 23 bit shift register. This register is clocked by bit 19 of the accumulator.
 *
 * Java port of the reSID 1.0 waveformgenerator by Dag Lem.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 */
class WaveformGenerator {

private:
	array<short>* model_wave;
	short dac[4096];

	// Tell whether the accumulator MSB was set high on this cycle.
	bool msb_rising;

	// PWout = (PWn/40.95)%
	int pw;

	int shift_register;

	// Remaining time to fully reset shift register.
	int shift_register_reset;
	// Emulation of pipeline causing bit 19 to clock the shift register.
	int shift_pipeline;

	int ring_msb_mask;
	int no_noise;
	int noise_output;
	int no_noise_or_noise_output;
	int no_pulse;
	int pulse_output;

	/**
	 * The control register right-shifted 4 bits; used for output function table lookup.
	 */
	int waveform;

	int floating_output_ttl;

	short* wave;

	int waveform_output;

	/** Current and previous accumulator value. */
	int accumulator;

	// Fout  = (Fn*Fclk/16777216)Hz
	// reg16 freq;
	int freq;

	/**
	 * The control register bits. Gate is handled by EnvelopeGenerator.
	 */
	bool test;
	bool sync;

private:
	void clock_shift_register();

	void write_shift_register();

	void reset_shift_register();

	void set_noise_output();

public:
	void setWaveformModels(array<short>* models);

	/**
	 * Set nonlinearity parameter for imperfect analog DAC emulation.
	 * 1.0 means perfect 8580-like linearity, values between 0.95 - 0.97
	 * are probably realistic 6581 nonlinearity values.
	 *
	 * @param nonLinearity
	 */
	void setChipModel(const ChipModel chipModel);

	/**
	 * SID clocking - 1 cycle.
	 */
	void clock();

	/**
	 * Synchronize oscillators. This must be done after all the oscillators have been clock()'ed,
	 * so that they are in the same state.
	 *
	 * @param syncDest The oscillator I am syncing
	 * @param syncSource The oscillator syncing me.
	 */
	const void synchronize(WaveformGenerator* syncDest, const WaveformGenerator* syncSource);

	/**
	 * Constructor.
	 */
	WaveformGenerator() :
		model_wave(0),
		msb_rising(false),
		pw(0),
		shift_register(0x7fffff),
		shift_register_reset(0),
		shift_pipeline(0),
		ring_msb_mask(0),
		no_noise(0xfff),
		noise_output(0xfff),
		no_noise_or_noise_output(no_noise | noise_output),
		no_pulse(0xfff),
		pulse_output(0xfff),
		waveform(0),
		floating_output_ttl(0),
		wave(0),
		waveform_output(0),
		accumulator(0),
		freq(0),
		test(false),
		sync(false) {}

	/**
	 * Register functions.
	 *
	 * @param freq_lo low 8 bits of frequency
	 */
	void writeFREQ_LO(const unsigned char freq_lo);

	/**
	 * Register functions.
	 *
	 * @param freq_hi high 8 bits of frequency
	 */
	void writeFREQ_HI(const unsigned char freq_hi);

	/**
	 * Register functions.
	 *
	 * The original form was (acc >> 12) >= pw, where truth value is not affected by the contents of the low 12 bits. Therefore the lowest bits must be zero in the new formulation acc >= (pw << 12).
	 *
	 * @param pw_lo low 8 bits of pulse width
	 */
	void writePW_LO(const unsigned char pw_lo);

	/**
	 * Register functions.
	 * 
	 * @param pw_hi high 8 bits of pulse width
	 */
	void writePW_HI(const unsigned char pw_hi);

	/**
	 * Register functions.
	 *
	 * @param ring_modulator ring-modulator modulating me.
	 * @param control control register value
	 */
	void writeCONTROL_REG(const unsigned char control);

	/**
	 * SID reset.
	 */
	void reset();

	/**
	 * 12-bit waveform output.
	 *
	 * @param ringModulator The oscillator ring-modulating me.
	 * @return output from waveformgenerator
	 */
	short output(const WaveformGenerator* ringModulator);

	/**
	 * Read OSC3 value (6581, not latched/delayed version)
	 *
	 * @param ring_modulator The ring modulating partner of this waveform
	 * @return OSC3 value
	 */
	unsigned char readOSC() { return (unsigned char) (waveform_output >> 4); }

	/**
	* Read accumulator value.
	*/
	const int readAccumulator() const { return accumulator; }

	/**
	* Read freq value.
	*/
	const int readFreq() const { return freq; }

	/**
	* Read test value.
	*/
	const bool readTest() const { return test; }

	/**
	* Read sync value.
	*/
	const bool readSync() const { return sync; }
};

} // namespace reSIDfp

#if RESID_INLINING || defined(WAVEFORMGENERATOR_CPP)

namespace reSIDfp
{

RESID_INLINE
void WaveformGenerator::clock() {
	if (test) {
		if (shift_register_reset != 0 && -- shift_register_reset == 0) {
			reset_shift_register();
		}

		// The test bit sets pulse high.
		pulse_output = 0xfff;
	} else {
		// Calculate new accumulator value;
		const int accumulator_next = (accumulator + freq) & 0xffffff;
		const int accumulator_bits_set = ~accumulator & accumulator_next;
		accumulator = accumulator_next;

		// Check whether the MSB is set high. This is used for synchronization.
		msb_rising = (accumulator_bits_set & 0x800000) != 0;

		// Shift noise register once for each time accumulator bit 19 is set high.
		// The shift is delayed 2 cycles.
		if ((accumulator_bits_set & 0x080000) != 0) {
			// Pipeline: Detect rising bit, shift phase 1, shift phase 2.
			shift_pipeline = 2;
		}
		else if (shift_pipeline != 0 && -- shift_pipeline == 0) {
			clock_shift_register();
		}
	}
}

RESID_INLINE
short WaveformGenerator::output(const WaveformGenerator* ringModulator) {
	// Set output value.
	if (waveform != 0) {
		// The bit masks no_pulse and no_noise are used to achieve branch-free
		// calculation of the output value.
		const int ix = (accumulator ^ (ringModulator->accumulator & ring_msb_mask)) >> 12;
		waveform_output = wave[ix] & (no_pulse | pulse_output) & no_noise_or_noise_output;
		if (waveform > 0x8) {
			// Combined waveforms write to the shift register.
			write_shift_register();
		}
	} else {
		// Age floating DAC input.
		if (floating_output_ttl != 0 && -- floating_output_ttl == 0) {
			waveform_output = 0;
		}
	}

	// The pulse level is defined as (accumulator >> 12) >= pw ? 0xfff : 0x000.
	// The expression -((accumulator >> 12) >= pw) & 0xfff yields the same
	// results without any branching (and thus without any pipeline stalls).
	// NB! This expression relies on that the result of a boolean expression
	// is either 0 or 1, and furthermore requires two's complement integer.
	// A few more cycles may be saved by storing the pulse width left shifted
	// 12 bits, and dropping the and with 0xfff (this is valid since pulse is
	// used as a bit mask on 12 bit values), yielding the expression
	// -(accumulator >= pw24). However this only results in negligible savings.

	// The result of the pulse width compare is delayed one cycle.
	// Push next pulse level into pulse level pipeline.
	pulse_output = ((accumulator >> 12) >= pw) ? 0xfff : 0x000;

	// DAC imperfections are emulated by using waveform_output as an index
	// into a DAC lookup table. readOSC() uses waveform_output directly.
	return dac[waveform_output];
}

} // namespace reSIDfp

#endif

#endif
