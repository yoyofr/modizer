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

#ifndef ENVELOPEGENERATOR_H
#define ENVELOPEGENERATOR_H

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 * A 15 bit counter is used to implement the envelope rates, in effect dividing
 * the clock to the envelope counter by the currently selected rate period.
 * <P>
 * In addition, another counter is used to implement the exponential envelope decay, in effect further dividing the clock to the envelope counter. The period of this counter is set to 1, 2, 4, 8, 16,
 * 30 at the envelope counter values 255, 93, 54, 26, 14, 6, respectively.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 */
class EnvelopeGenerator {

private:
	/**
	 * The envelope state machine's distinct states. In addition to this,
	 * envelope has a hold mode, which freezes envelope counter to zero.
	 */
	enum State {
		ATTACK, DECAY_SUSTAIN, RELEASE
	};

	/**
	 * Whether hold is enabled. Only switching to ATTACK can release envelope.
	 */
	bool hold_zero;

	bool envelope_pipeline;

	/**
	 * XOR shift register emulated via normal integer which implements delay
	 * until the next envelope operation occurs. The XOR shift register has
	 * 0x7fff different values to scan.
	 */
	int rate_counter;

	/**
	 * Comparison value (period) of the rate counter before next event.
	 */
	int rate_period;

	/**
	 * During release mode, the SID arpproximates envelope decay via piecewise
	 * linear decay rate.
	 */
	int exponential_counter;

	/**
	 * Comparison value (period) of the exponential decay counter before next
	 * decrement.
	 */
	int exponential_counter_period;

	/** The current digital value of envelope output. */
	unsigned char envelope_counter;

	/** Attack register */
	int attack;

	/** Decay register */
	int decay;

	/** Sustain register */
	int sustain;

	/** Release register */
	int release;

	/** Gate bit */
	bool gate;

	/** Current envelope state */
	State state;

	/**
	 * Lookup table to convert from attack, decay, or release value to rate
	 * counter period.
	 * <P>
	 * Rate counter periods are calculated from the Envelope Rates table in the Programmer's Reference Guide. The rate counter period is the number of cycles between each increment of the envelope
	 * counter. The rates have been verified by sampling ENV3.
	 * <P>
	 * The rate counter is a 16 bit register which is incremented each cycle. When the counter reaches a specific comparison value, the envelope counter is incremented (attack) or decremented
	 * (decay/release) and the counter is zeroed.
	 * <P>
	 * NB! Sampling ENV3 shows that the calculated values are not exact. It may seem like most calculated values have been rounded (.5 is rounded down) and 1 has beed added to the result. A possible
	 * explanation for this is that the SID designers have used the calculated values directly as rate counter comparison values, not considering a one cycle delay to zero the counter. This would
	 * yield an actual period of comparison value + 1.
	 * <P>
	 * The time of the first envelope count can not be exactly controlled, except possibly by resetting the chip. Because of this we cannot do cycle exact sampling and must devise another method to
	 * calculate the rate counter periods.
	 * <P>
	 * The exact rate counter periods can be determined e.g. by counting the number of cycles from envelope level 1 to envelope level 129, and dividing the number of cycles by 128. CIA1 timer A and B
	 * in linked mode can perform the cycle count. This is the method used to find the rates below.
	 * <P>
	 * To avoid the ADSR delay bug, sampling of ENV3 should be done using sustain = release = 0. This ensures that the attack state will not lower the current rate counter period.
	 * <P>
	 * The ENV3 sampling code below yields a maximum timing error of 14 cycles.
	 *
	 * <pre>
	 *      lda #$01
	 *  l1: cmp $d41c
	 *      bne l1
	 *      ...
	 *      lda #$ff
	 *  l2: cmp $d41c
	 *      bne l2
	 * </pre>
	 * 
	 * This yields a maximum error for the calculated rate period of 14/128 cycles. The described method is thus sufficient for exact calculation of the rate periods.
	 */
	static const int ENVELOPE_PERIOD[16];

	/**
	 * The 16 selectable sustain levels.
	 * <P>
	 * For decay and release, the clock to the envelope counter is sequentially divided by 1, 2, 4, 8, 16, 30, 1 to create a piece-wise linear approximation of an exponential. The exponential counter
	 * period is loaded at the envelope counter values 255, 93, 54, 26, 14, 6, 0. The period can be different for the same envelope counter value, depending on whether the envelope has been rising
	 * (attack -> release) or sinking (decay/release).
	 * <P>
	 * Since it is not possible to reset the rate counter (the test bit has no influence on the envelope generator whatsoever) a method must be devised to do cycle exact sampling of ENV3 to do the
	 * investigation. This is possible with knowledge of the rate period for A=0, found above.
	 * <P>
	 * The CPU can be synchronized with ENV3 by first synchronizing with the rate counter by setting A=0 and wait in a carefully timed loop for the envelope counter _not_ to change for 9 cycles. We
	 * can then wait for a specific value of ENV3 with another timed loop to fully synchronize with ENV3.
	 * <P>
	 * At the first period when an exponential counter period larger than one is used (decay or relase), one extra cycle is spent before the envelope is decremented. The envelope output is then
	 * delayed one cycle until the state is changed to attack. Now one cycle less will be spent before the envelope is incremented, and the situation is normalized.
	 * <P>
	 * The delay is probably caused by the comparison with the exponential counter, and does not seem to affect the rate counter. This has been verified by timing 256 consecutive complete envelopes
	 * with A = D = R = 1, S = 0, using CIA1 timer A and B in linked mode. If the rate counter is not affected the period of each complete envelope is
	 * <P>
	 * (255 + 162*1 + 39*2 + 28*4 + 12*8 + 8*16 + 6*30)*32 = 756*32 = 32352
	 * <P>
	 * which corresponds exactly to the timed value divided by the number of complete envelopes.
	 * <P>
	 * <P>
	 * From the sustain levels it follows that both the low and high 4 bits of the envelope counter are compared to the 4-bit sustain value. This has been verified by sampling ENV3.
	 */

	/**
	 * Emulated nonlinearity of the envelope DAC.
	 *
	 * @See SID.kinked_dac
	 */
	short dac[256];

	void set_exponential_counter();

public:
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

	int output();

	/**
	 * Constructor.
	 */
	EnvelopeGenerator() :
		hold_zero(true),
		envelope_pipeline(false),
		rate_counter(0),
		rate_period(0),
		exponential_counter(0),
		exponential_counter_period(1),
		envelope_counter(0),
		attack(0),
		decay(0),
		sustain(0),
		release(0),
		gate(false),
		state(RELEASE) {}

	/**
	 * SID reset.
	 */
	void reset();

	// ----------------------------------------------------------------------------
	// Register functions.
	// ----------------------------------------------------------------------------

	/**
	 * @param control
	 *            control register
	 */
	void writeCONTROL_REG(const unsigned char control);

	/**
	 * @param attack_decay
	 *            attack/decay value
	 */
	void writeATTACK_DECAY(const unsigned char attack_decay);

	/**
	 * @param sustain_release
	 *            sustain/release value
	 */
	void writeSUSTAIN_RELEASE(const unsigned char sustain_release);

	/**
	 * Return the envelope current value.
	 *
	 * @return envelope counter
	 */
	unsigned char readENV() { return envelope_counter; }
};

} // namespace reSIDfp

#if RESID_INLINING || defined(ENVELOPEGENERATOR_CPP)

namespace reSIDfp
{

RESID_INLINE
void EnvelopeGenerator::clock() {
	if (envelope_pipeline) {
		-- envelope_counter;
		envelope_pipeline = false;
		// Check for change of exponential counter period.
		set_exponential_counter();
	}

	// Check for ADSR delay bug.
	// If the rate counter comparison value is set below the current value of the
	// rate counter, the counter will continue counting up until it wraps around
	// to zero at 2^15 = 0x8000, and then count rate_period - 1 before the
	// envelope can constly be stepped.
	// This has been verified by sampling ENV3.
	//
	if ((++ rate_counter & 0x8000) != 0) {
		rate_counter = (rate_counter + 1) & 0x7fff;
	}

	if (rate_counter != rate_period) {
		return;
	}

	rate_counter = 0;

	// The first envelope step in the attack state also resets the exponential
	// counter. This has been verified by sampling ENV3.
	//
	if (state == ATTACK || ++exponential_counter == exponential_counter_period) {
		// likely (~50%)
		exponential_counter = 0;

		// Check whether the envelope counter is frozen at zero.
		if (hold_zero) {
		return;
		}

		switch (state) {
		case ATTACK:
		// The envelope counter can flip from 0xff to 0x00 by changing state to
		// release, then to attack. The envelope counter is then frozen at
		// zero; to unlock this situation the state must be changed to release,
		// then to attack. This has been verified by sampling ENV3.
		//
		++ envelope_counter;
		if (envelope_counter == (unsigned char) 0xff) {
			state = DECAY_SUSTAIN;
			rate_period = ENVELOPE_PERIOD[decay];
		}
		break;
		case DECAY_SUSTAIN:
			if (envelope_counter == (unsigned char) (sustain << 4 | sustain)) {
				return;
			}
			if (exponential_counter_period != 1) {
				// unlikely (15%)
				// The decrement is delayed one cycle.
				envelope_pipeline = true;
				return;
			}
			-- envelope_counter;
			break;
		case RELEASE:
			// The envelope counter can flip from 0x00 to 0xff by changing state to
			// attack, then to release. The envelope counter will then continue
			// counting down in the release state.
			// This has been verified by sampling ENV3.
			// NB! The operation below requires two's complement integer.
			//
			if (exponential_counter_period != 1) {
				// likely (~50%)
				// The decrement is delayed one cycle.
				envelope_pipeline = true;
				return;
			}
			-- envelope_counter;
			break;
		}

		// Check for change of exponential counter period.
		set_exponential_counter();
	}
}

RESID_INLINE
int EnvelopeGenerator::output() {
	// DAC imperfections are emulated by using envelope_counter as an index
	// into a DAC lookup table. readENV() uses envelope_counter directly.
	return dac[envelope_counter & 0xff];
}

} // namespace reSIDfp

#endif

#endif
