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

#ifndef VOICE_H
#define VOICE_H

#include "siddefs-fp.h"
#include "WaveformGenerator.h"
#include "EnvelopeGenerator.h"

namespace reSIDfp
{

/**
 * Representation of SID voice block.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class Voice {

public:
	WaveformGenerator* wave;

	EnvelopeGenerator* envelope;

public:
	/**
	 * Amplitude modulated waveform output.
	 *
	 * The waveform DAC generates a voltage between 5 and 12 V corresponding
	 * to oscillator state 0 .. 4095.
	 *
	 * The envelope DAC generates a voltage between waveform gen output and
	 * the 5V level, corresponding to envelope state 0 .. 255.
	 *
	 * Ideal range [-2048*255, 2047*255].
	 *
	 * @param ringModulator Ring-modulator for waveform
	 * @return waveformgenerator output
	 */
	RESID_INLINE
	int output(const WaveformGenerator* ringModulator) {
		return wave->output(ringModulator) * envelope->output();
	}

	/**
	 * Constructor.
	 */
	Voice() :
		wave(new WaveformGenerator()),
		envelope(new EnvelopeGenerator()) {}

	~Voice() {
		delete wave;
		delete envelope;
	}

	// ----------------------------------------------------------------------------
	// Register functions.
	// ----------------------------------------------------------------------------

	/**
	 * Register functions.
	 *
	 * @param ring_modulator Ring modulator for waveform
	 * @param control Control register value.
	 */
	void writeCONTROL_REG(const unsigned char control) {
		wave->writeCONTROL_REG(control);
		envelope->writeCONTROL_REG(control);
	}

	/**
	 * SID reset.
	 */
	void reset() {
		wave->reset();
		envelope->reset();
	}
};

} // namespace reSIDfp

#endif
