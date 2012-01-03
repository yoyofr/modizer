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

#ifndef EXTERNALFILTER_H
#define EXTERNALFILTER_H

#include "siddefs-fp.h"

namespace reSIDfp
{

/**
 *
 * The audio output stage in a Commodore 64 consists of two STC networks, a
 * low-pass filter with 3-dB frequency 16kHz followed by a high-pass filter with
 * 3-dB frequency 16Hz (the latter provided an audio equipment input impedance
 * of 1kOhm).
 * <P>
 * The STC networks are connected with a BJT supposedly meant to act as a unity gain buffer, which is not really how it works. A more elaborate model would include the BJT, however DC circuit analysis
 * yields BJT base-emitter and emitter-base impedances sufficiently low to produce additional low-pass and high-pass 3dB-frequencies in the order of hundreds of kHz. This calls for a sampling
 * frequency of several MHz, which is far too high for practical use.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class ExternalFilter {

private:
	/**
	 * lowpass
	 */
	int Vlp;

	/**
	 * highpass
	 */
	int Vhp;

	int w0lp_1_s7;

	int w0hp_1_s20;

public:
	/**
	 * SID clocking - 1 cycle.
	 * 
	 * @param Vi
	 */
	const int clock(const int Vi);

	/**
	 * Constructor.
	 */
	ExternalFilter();

	/**
	 * Setup of the external filter sampling parameters.
	 * 
	 * @param clock
	 */
	void setClockFrequency(const float frequency);

	/**
	 * SID reset.
	 */
	void reset();
};

} // namespace reSIDfp

#if RESID_INLINING || defined(EXTERNALFILTER_CPP)

namespace reSIDfp
{

RESID_INLINE
const int ExternalFilter::clock(const int Vi) {
	const int dVlp = (w0lp_1_s7*((Vi << 11) - Vlp) >> 7);
	const int dVhp = (w0hp_1_s20*(Vlp - Vhp) >> 20);
	Vlp += dVlp;
	Vhp += dVhp;
	return (Vlp - Vhp) >> 11;
}

} // namespace reSIDfp

#endif

#endif
