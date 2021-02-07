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

#define WAVEFORMGENERATOR_CPP

#include "WaveformGenerator.h"

#include "Dac.h"

namespace reSIDfp
{

void WaveformGenerator::clock_shift_register() {
	// bit0 = (bit22 | test) ^ bit17
	const int bit0 = ((shift_register >> 22) ^ (shift_register >> 17)) & 0x1;
	shift_register = ((shift_register << 1) | bit0) & 0x7fffff;

	// New noise waveform output.
	set_noise_output();
}

void WaveformGenerator::write_shift_register() {
	// Write changes to the shift register output caused by combined waveforms
	// back into the shift register.
	// A bit once set to zero cannot be changed, hence the and'ing.
	// FIXME: Write test program to check the effect of 1 bits and whether
	// neighboring bits are affected.

	shift_register &=
	~((1<<20)|(1<<18)|(1<<14)|(1<<11)|(1<<9)|(1<<5)|(1<<2)|(1<<0)) |
	((waveform_output & 0x800) << 9) |  // Bit 11 -> bit 20
	((waveform_output & 0x400) << 8) |  // Bit 10 -> bit 18
	((waveform_output & 0x200) << 5) |  // Bit  9 -> bit 14
	((waveform_output & 0x100) << 3) |  // Bit  8 -> bit 11
	((waveform_output & 0x080) << 2) |  // Bit  7 -> bit  9
	((waveform_output & 0x040) >> 1) |  // Bit  6 -> bit  5
	((waveform_output & 0x020) >> 3) |  // Bit  5 -> bit  2
	((waveform_output & 0x010) >> 4);   // Bit  4 -> bit  0

	noise_output &= waveform_output;
	no_noise_or_noise_output = no_noise | noise_output;
}

void WaveformGenerator::reset_shift_register() {
	shift_register = 0x7fffff;
	shift_register_reset = 0;

	// New noise waveform output.
	set_noise_output();
}

void WaveformGenerator::set_noise_output() {
	noise_output =
	((shift_register & 0x100000) >> 9) |
	((shift_register & 0x040000) >> 8) |
	((shift_register & 0x004000) >> 5) |
	((shift_register & 0x000800) >> 3) |
	((shift_register & 0x000200) >> 2) |
	((shift_register & 0x000020) << 1) |
	((shift_register & 0x000004) << 3) |
	((shift_register & 0x000001) << 4);

	no_noise_or_noise_output = no_noise | noise_output;
}

void WaveformGenerator::setWaveformModels(array<short>* models) {
	model_wave = models;
}

void WaveformGenerator::setChipModel(const ChipModel chipModel) {
	float dacBits[12];
	Dac::kinkedDac(dacBits, 12, chipModel == MOS6581 ? 2.20 : 2.00, chipModel == MOS8580);

	for (int i = 0; i < 4096; i++) {
		float dacValue = 0.;
		for (int j = 0; j < 12; j ++) {
			if ((i & (1 << j)) != 0) {
				dacValue += dacBits[j];
			}
		}
		dac[i] = (short) (dacValue + 0.5);
	}
	const int offset = dac[chipModel == MOS6581 ? 0x380 : 0x800];
	for (int i = 0; i < 4096; i ++) {
		dac[i] -= offset;
	}
}

const void WaveformGenerator::synchronize(WaveformGenerator* syncDest, const WaveformGenerator* syncSource) {
	// A special case occurs when a sync source is synced itself on the same
	// cycle as when its MSB is set high. In this case the destination will
	// not be synced. This has been verified by sampling OSC3.
	if (msb_rising && syncDest->sync && !(sync && syncSource->msb_rising)) {
		syncDest->accumulator = 0;
	}
}

void WaveformGenerator::writeFREQ_LO(const unsigned char freq_lo) {
	freq = (freq & 0xff00) | (freq_lo & 0xff);
}

void WaveformGenerator::writeFREQ_HI(const unsigned char freq_hi) {
	freq = (freq_hi << 8 & 0xff00) | (freq & 0xff);
}

void WaveformGenerator::writePW_LO(const unsigned char pw_lo) {
	pw = (pw & 0xf00) | (pw_lo & 0x0ff);
}

void WaveformGenerator::writePW_HI(const unsigned char pw_hi) {
	pw = (pw_hi << 8 & 0xf00) | (pw & 0x0ff);
}

void WaveformGenerator::writeCONTROL_REG(const unsigned char control) {
	const int waveform_prev = waveform;
	const bool test_prev = test;
	waveform = (control >> 4) & 0x0f;
	test = (control & 0x08) != 0;
	sync = (control & 0x02) != 0;

	// Set up waveform table.
	wave = (*model_wave)[waveform & 0x7];

	// Substitution of accumulator MSB when sawtooth = 0, ring_mod = 1.
	ring_msb_mask = ((~control >> 5) & (control >> 2) & 0x1) << 23;

	// no_noise and no_pulse are used in set_waveform_output() as bitmasks to
	// only let the noise or pulse influence the output when the noise or pulse
	// waveforms are selected.
	no_noise = (waveform & 0x8) != 0 ? 0x000 : 0xfff;
	no_noise_or_noise_output = no_noise | noise_output;
	no_pulse = (waveform & 0x4) != 0 ? 0x000 : 0xfff;

	if (!test_prev && test) {
		// Reset accumulator.
		accumulator = 0;

		// Flush shift pipeline.
		shift_pipeline = 0;

		// Set reset time for shift register.
		shift_register_reset = 0x8000;
	}
	else if (test_prev && !test) {
		// When the test bit is falling, the second phase of the shift is
		// completed by enabling SRAM write.

		// bit0 = (bit22 | test) ^ bit17 = 1 ^ bit17 = ~bit17
		const int bit0 = (~shift_register >> 17) & 0x1;
		shift_register = ((shift_register << 1) | bit0) & 0x7fffff;

		// Set new noise waveform output.
		set_noise_output();
	}

	if (waveform == 0 && waveform_prev != 0) {
		// Change to floating DAC input.
		// Reset fading time for floating DAC input.
		floating_output_ttl = 0x4000;
	}
}

void WaveformGenerator::reset() {

	accumulator = 0;
	freq = 0;
	pw = 0;

	msb_rising = false;

	waveform = 0;
	test = false;
	sync = false;

	wave = model_wave ? (*model_wave)[0] : 0;

	ring_msb_mask = 0;
	no_noise = 0xfff;
	no_pulse = 0xfff;
	pulse_output = 0xfff;

	reset_shift_register();
	shift_pipeline = 0;

	waveform_output = 0;
	floating_output_ttl = 0;
}

} // namespace reSIDfp
