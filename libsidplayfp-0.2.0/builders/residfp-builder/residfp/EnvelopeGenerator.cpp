/*
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

#define ENVELOPEGENERATOR_CPP

#include "EnvelopeGenerator.h"

#include "Dac.h"

namespace reSIDfp
{

const int EnvelopeGenerator::ENVELOPE_PERIOD[16] = {
	9, //   2ms*1.0MHz/256 =     7.81
	32, //   8ms*1.0MHz/256 =    31.25
	63, //  16ms*1.0MHz/256 =    62.50
	95, //  24ms*1.0MHz/256 =    93.75
	149, //  38ms*1.0MHz/256 =   148.44
	220, //  56ms*1.0MHz/256 =   218.75
	267, //  68ms*1.0MHz/256 =   265.63
	313, //  80ms*1.0MHz/256 =   312.50
	392, // 100ms*1.0MHz/256 =   390.63
	977, // 250ms*1.0MHz/256 =   976.56
	1954, // 500ms*1.0MHz/256 =  1953.13
	3126, // 800ms*1.0MHz/256 =  3125.00
	3907, //   1 s*1.0MHz/256 =  3906.25
	11720, //   3 s*1.0MHz/256 = 11718.75
	19532, //   5 s*1.0MHz/256 = 19531.25
	31251 //   8 s*1.0MHz/256 = 31250.00
};

void EnvelopeGenerator::set_exponential_counter() {
	// Check for change of exponential counter period.
	switch (envelope_counter) {
	case 0xff:
		exponential_counter_period = 1;
		break;
	case 0x5d:
		exponential_counter_period = 2;
		break;
	case 0x36:
		exponential_counter_period = 4;
		break;
	case 0x1a:
		exponential_counter_period = 8;
		break;
	case 0x0e:
		exponential_counter_period = 16;
		break;
	case 0x06:
		exponential_counter_period = 30;
		break;
	case 0x00:
		// FIXME: Check whether 0x00 really changes the period.
		// E.g. set R = 0xf, gate on to 0x06, gate off to 0x00, gate on to 0x04,
		// gate off, sample.
		exponential_counter_period = 1;

		// When the envelope counter is changed to zero, it is frozen at zero.
		// This has been verified by sampling ENV3.
		hold_zero = true;
		break;
	}
}

void EnvelopeGenerator::setChipModel(const ChipModel chipModel) {
	const int dacBitsLength = 8;
	float dacBits[dacBitsLength];
	Dac::kinkedDac(dacBits, dacBitsLength, chipModel == MOS6581 ? 2.30 : 2.00, chipModel == MOS8580);
	for (int i = 0; i < 256; i++) {
		float dacValue = 0.;
		for (int j = 0; j < dacBitsLength; j ++) {
			if ((i & (1 << j)) != 0) {
				dacValue += dacBits[j];
			}
		}
		dac[i] = (short) (dacValue + 0.5);
	}
}

void EnvelopeGenerator::reset() {
	envelope_counter = 0;
	envelope_pipeline = false;

	attack = 0;
	decay = 0;
	sustain = 0;
	release = 0;

	gate = false;

	rate_counter = 0;
	exponential_counter = 0;
	exponential_counter_period = 1;

	state = RELEASE;
	rate_period = ENVELOPE_PERIOD[release];
	hold_zero = true;
}

void EnvelopeGenerator::writeCONTROL_REG(const unsigned char control) {
	const bool gate_next = (control & 0x01) != 0;

	// The rate counter is never reset, thus there will be a delay before the
	// envelope counter starts counting up (attack) or down (release).

	// Gate bit on: Start attack, decay, sustain.
	if (!gate && gate_next) {
		state = ATTACK;
		rate_period = ENVELOPE_PERIOD[attack];

		// Switching to attack state unlocks the zero freeze and aborts any
		// pipelined envelope decrement.
		hold_zero = false;
		// FIXME: This is an assumption which should be checked using cycle exact
		// envelope sampling.
		envelope_pipeline = false;
	}
	// Gate bit off: Start release.
	else if (gate && !gate_next) {
		state = RELEASE;
		rate_period = ENVELOPE_PERIOD[release];
	}

	gate = gate_next;
}

void EnvelopeGenerator::writeATTACK_DECAY(const unsigned char attack_decay) {
	attack = (attack_decay >> 4) & 0x0f;
	decay = attack_decay & 0x0f;
	if (state == ATTACK) {
		rate_period = ENVELOPE_PERIOD[attack];
	} else if (state == DECAY_SUSTAIN) {
		rate_period = ENVELOPE_PERIOD[decay];
	}
}

void EnvelopeGenerator::writeSUSTAIN_RELEASE(const unsigned char sustain_release) {
	sustain = (sustain_release >> 4) & 0x0f;
	release = sustain_release & 0x0f;
	if (state == RELEASE) {
		rate_period = ENVELOPE_PERIOD[release];
	}
}

} // namespace reSIDfp
