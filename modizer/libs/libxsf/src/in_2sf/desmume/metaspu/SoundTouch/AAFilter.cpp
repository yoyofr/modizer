////////////////////////////////////////////////////////////////////////////////
///
/// FIR low-pass (anti-alias) filter with filter coefficient design routine and
/// MMX optimization.
///
/// Anti-alias filter is used to prevent folding of high frequencies when
/// transposing the sample rate with interpolation.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.9 $
//
// $Id: AAFilter.cpp,v 1.9 2006/02/05 16:44:06 Olli Exp $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include "XSFCommon.h"

#include <vector>
#include <cassert>
#include "AAFilter.h"

using namespace soundtouch;

#ifndef M_PI
static const double M_PI = 3.14159265358979323846;
#endif

/*****************************************************************************
 *
 * Implementation of the class 'AAFilter'
 *
 *****************************************************************************/

AAFilter::AAFilter(const uint32_t len)
{
	this->pFIR.reset(FIRFilter::newInstance());
	this->cutoffFreq = 0.5;
	this->setLength(len);
}

// Sets new anti-alias filter cut-off edge frequency, scaled to
// sampling frequency (nyquist frequency = 0.5).
// The filter will cut frequencies higher than the given frequency.
void AAFilter::setCutoffFreq(double newCutoffFreq)
{
	this->cutoffFreq = newCutoffFreq;
	this->calculateCoeffs();
}

// Sets number of FIR filter taps
void AAFilter::setLength(uint32_t newLength)
{
	this->length = newLength;
	this->calculateCoeffs();
}

// Calculates coefficients for a low-pass FIR filter using Hamming window
void AAFilter::calculateCoeffs()
{
	assert(this->length > 0);
	assert(!(this->length % 4));
	assert(this->cutoffFreq >= 0);
	assert(this->cutoffFreq <= 0.5);

	auto work = std::vector<double>(this->length);
	auto coeffs = std::vector<SAMPLETYPE>(this->length);

	double fc2 = 2.0 * this->cutoffFreq;
	double wc = M_PI * fc2;
	double tempCoeff = 2 * M_PI / this->length;

	double sum = 0.0;
	for (uint32_t i = 0; i < this->length; ++i)
	{
		double cntTemp = i - (this->length / 2);

		double temp = cntTemp * wc, h;
		if (!fEqual(temp, 0.0))
			h = fc2 * std::sin(temp) / temp; // sinc function
		else
			h = 1.0;
		double w = 0.54 + 0.46 * std::cos(tempCoeff * cntTemp); // hamming window

		temp = w * h;
		work[i] = temp;

		// calc net sum of coefficients
		sum += temp;
	}

	// ensure the sum of coefficients is larger than zero
	assert(sum > 0);

	// ensure we've really designed a lowpass filter...
	assert(work[this->length / 2] > 0);
	assert(work[this->length / 2 + 1] > -1e-6);
	assert(work[this->length / 2 - 1] > -1e-6);

	// Calculate a scaling coefficient in such a way that the result can be
	// divided by 16384
	double scaleCoeff = 16384.0 / sum;

	for (uint32_t i = 0; i < this->length; ++i)
	{
		// scale & round to nearest integer
		double temp = work[i] * scaleCoeff;
		temp += temp >= 0 ? 0.5 : -0.5;
		// ensure no overfloods
		assert(temp >= -32768 && temp <= 32767);
		coeffs[i] = static_cast<SAMPLETYPE>(temp);
	}

	// Set coefficients. Use divide factor 14 => divide result by 2^14 = 16384
	this->pFIR->setCoefficients(&coeffs[0], length, 14);
}

// Applies the filter to the given sequence of samples.
// Note : The amount of outputted samples is by value of 'filter length'
// smaller than the amount of input samples.
uint32_t AAFilter::evaluate(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t numSamples, uint32_t numChannels) const
{
	return this->pFIR->evaluate(dest, src, numSamples, numChannels);
}

uint32_t AAFilter::getLength() const
{
	return this->pFIR->getLength();
}
