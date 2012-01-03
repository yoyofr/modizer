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
 * ---------------------------------------------------------------------------
 * Filter distortion code written by Antti S. Lankila 2007 - 2008.
 *
 * @author Ken Händel
 * Ported to c++ by Leandro Nini
 *
 */

#ifndef FILTER_H
#define FILTER_H

namespace reSIDfp
{

/**
 * SID filter base class
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class Filter {

private:
	/**
	 * Filter enabled.
	 */
	bool enabled;

	/**
	 * Selects which inputs to route through filter.
	 */
	char filt;

protected:
	/**
	 * Filter cutoff frequency.
	 */
	int fc;

	/**
	 * Filter resonance.
	 */
	int res;

	/**
	 * Routing to filter or outside filter
	 */
	bool filt1, filt2, filt3, filtE;

	/**
	 * Switch voice 3 off.
	 */
	bool voice3off;

	/**
	 * Highpass, bandpass, and lowpass filter modes.
	 */
	bool hp, bp, lp;

	/**
	 * Current volume.
	 */
	int vol;

	/**
	 * Current clock frequency.
	 */
	float clockFrequency;

protected:
	/**
	 * Set filter cutoff frequency.
	 */
	virtual void updatedCenterFrequency()=0;

	/**
	 * Set filter resonance.
	 */
	virtual void updatedResonance()=0;

	/**
	 * Mixing configuration modified (offsets change)
	 */
	virtual void updatedMixing()=0;

public:
	Filter() :
		enabled(true),
		filt(0),
		fc(0),
		res(0),
		filt1(false),
		filt2(false),
		filt3(false),
		filtE(false),
		voice3off(0),
		hp(false),
		bp(false),
		lp(false),
		vol(0),
		clockFrequency(0.) {}

	virtual ~Filter() {}

	/**
	 * SID clocking - 1 cycle
	 *
	 * @param v1 voice 1 in
	 * @param v2 voice 2 in
	 * @param v3 voice 3 in
	 * @param vE external audio in
	 * @return filtered output
	 */
	virtual const int clock(const int v1, const int v2, const int v3)=0;

	/**
	 * Enable filter.
	 *
	 * @param enable
	 */
	void enable(const bool enable);

	void setClockFrequency(const float clock);

	/**
	 * SID reset.
	 */
	void reset();

	/**
	 * Register function.
	 *
	 * @param fc_lo
	 */
	void writeFC_LO(const unsigned char fc_lo);

	/**
	 * Register function.
	 *
	 * @param fc_hi
	 */
	void writeFC_HI(const unsigned char fc_hi);

	/**
	 * Register function.
	 *
	 * @param res_filt
	 */
	void writeRES_FILT(const unsigned char res_filt);

	/**
	 * Register function.
	 *
	 * @param mode_vol
	 */
	void writeMODE_VOL(const unsigned char mode_vol);

	virtual void input(const int input)=0;
};

} // namespace reSIDfp

#endif
