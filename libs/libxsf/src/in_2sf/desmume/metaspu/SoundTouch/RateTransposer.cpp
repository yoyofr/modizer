////////////////////////////////////////////////////////////////////////////////
///
/// Sample rate transposer. Changes sample rate by using linear interpolation
/// together with anti-alias filtering (first order interpolation with anti-
/// alias filtering should be quite adequate for this application)
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2011-09-02 15:56:11 -0300 (sex, 02 set 2011) $
// File revision : $Revision: 4 $
//
// $Id: RateTransposer.cpp 131 2011-09-02 18:56:11Z oparviai $
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

#include <stdexcept>
#include "RateTransposer.h"

using namespace soundtouch;

/// A linear samplerate transposer class that uses integer arithmetics.
/// for the transposing.
class RateTransposerInteger : public RateTransposer
{
protected:
	int iSlopeCount;
	int iRate;
	SAMPLETYPE sPrevSampleL, sPrevSampleR;

	virtual void resetRegisters();

	virtual uint32_t transposeStereo(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t numSamples);
	virtual uint32_t transposeMono(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t numSamples);

public:
	RateTransposerInteger();
	virtual ~RateTransposerInteger();

	/// Sets new target rate. Normal rate = 1.0, smaller values represent slower
	/// rate, larger faster rates.
	virtual void setRate(float newRate);
};

/// A linear samplerate transposer class that uses floating point arithmetics
/// for the transposing.
class RateTransposerFloat : public RateTransposer
{
protected:
	float fSlopeCount;
	SAMPLETYPE sPrevSampleL, sPrevSampleR;

	virtual void resetRegisters();

	virtual uint32_t transposeStereo(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t numSamples);
	virtual uint32_t transposeMono(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t numSamples);

public:
	RateTransposerFloat();
	virtual ~RateTransposerFloat();
};

// Operator 'new' is overloaded so that it automatically creates a suitable instance
// depending on if we've a MMX/SSE/etc-capable CPU available or not.
void *RateTransposer::operator new(size_t)
{
	// Notice! don't use "new TDStretch" directly, use "newInstance" to create a new instance instead!
	//assert(false);
	//return NULL;
	throw std::runtime_error("Don't use 'new RateTransposer', use 'newInstance' member instead!");
}

RateTransposer *RateTransposer::newInstance()
{
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
	return ::new RateTransposerInteger;
#else
	return ::new RateTransposerFloat;
#endif
}

// Constructor
RateTransposer::RateTransposer() : FIFOProcessor(&outputBuffer)
{
	this->numChannels = 2;
	this->bUseAAFilter = true;
	this->fRate = 0;

	// Instantiates the anti-alias filter with default tap length
	// of 32
	this->pAAFilter.reset(new AAFilter(32));
}

RateTransposer::~RateTransposer()
{
}

/// Enables/disables the anti-alias filter. Zero to disable, nonzero to enable
void RateTransposer::enableAAFilter(bool newMode)
{
	this->bUseAAFilter = newMode;
}

/// Returns nonzero if anti-alias filter is enabled.
bool RateTransposer::isAAFilterEnabled() const
{
	return this->bUseAAFilter;
}

AAFilter *RateTransposer::getAAFilter()
{
	return this->pAAFilter.get();
}

// Sets new target iRate. Normal iRate = 1.0, smaller values represent slower
// iRate, larger faster iRates.
void RateTransposer::setRate(float newRate)
{
	double fCutoff;

	this->fRate = newRate;

	// design a new anti-alias filter
	if (newRate > 1.0f)
		fCutoff = 0.5f / newRate;
	else
		fCutoff = 0.5f * newRate;
    this->pAAFilter->setCutoffFreq(fCutoff);
}

// Adds 'nSamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void RateTransposer::putSamples(const SAMPLETYPE *samples, uint32_t nSamples)
{
	this->processSamples(samples, nSamples);
}

// Transposes up the sample rate, causing the observed playback 'rate' of the
// sound to decrease
void RateTransposer::upsample(const SAMPLETYPE *src, uint32_t nSamples)
{
	// If the parameter 'uRate' value is smaller than 'SCALE', first transpose
	// the samples and then apply the anti-alias filter to remove aliasing.

	// First check that there's enough room in 'storeBuffer'
	// (+16 is to reserve some slack in the destination buffer)
	uint32_t sizeTemp = static_cast<uint32_t>(nSamples / this->fRate + 16.0f);

	// Transpose the samples, store the result into the end of "storeBuffer"
	uint32_t count = this->transpose(this->storeBuffer.ptrEnd(sizeTemp), src, nSamples);
	this->storeBuffer.putSamples(count);

	// Apply the anti-alias filter to samples in "store output", output the
	// result to "dest"
	uint32_t num = this->storeBuffer.numSamples();
	count = this->pAAFilter->evaluate(this->outputBuffer.ptrEnd(num), this->storeBuffer.ptrBegin(), num, this->numChannels);
	this->outputBuffer.putSamples(count);

	// Remove the processed samples from "storeBuffer"
	this->storeBuffer.receiveSamples(count);
}

// Transposes down the sample rate, causing the observed playback 'rate' of the
// sound to increase
void RateTransposer::downsample(const SAMPLETYPE *src, uint32_t nSamples)
{
	// If the parameter 'uRate' value is larger than 'SCALE', first apply the
	// anti-alias filter to remove high frequencies (prevent them from folding
	// over the lover frequencies), then transpose. */

	// Add the new samples to the end of the storeBuffer */
	this->storeBuffer.putSamples(src, nSamples);

	// Anti-alias filter the samples to prevent folding and output the filtered
	// data to tempBuffer. Note : because of the FIR filter length, the
	// filtering routine takes in 'filter_length' more samples than it outputs.
	assert(this->tempBuffer.isEmpty());
	uint32_t sizeTemp = this->storeBuffer.numSamples();

	uint32_t count = this->pAAFilter->evaluate(this->tempBuffer.ptrEnd(sizeTemp), this->storeBuffer.ptrBegin(), sizeTemp, this->numChannels);

	// Remove the filtered samples from 'storeBuffer'
	this->storeBuffer.receiveSamples(count);

	// Transpose the samples (+16 is to reserve some slack in the destination buffer)
	sizeTemp = static_cast<uint32_t>(nSamples / this->fRate + 16.0f);
	count = this->transpose(this->outputBuffer.ptrEnd(sizeTemp), this->tempBuffer.ptrBegin(), count);
	this->outputBuffer.putSamples(count);
}

// Transposes sample rate by applying anti-alias filter to prevent folding.
// Returns amount of samples returned in the "dest" buffer.
// The maximum amount of samples that can be returned at a time is set by
// the 'set_returnBuffer_size' function.
void RateTransposer::processSamples(const SAMPLETYPE *src, uint32_t nSamples)
{
	if (!nSamples)
		return;
	assert(this->pAAFilter);

	// If anti-alias filter is turned off, simply transpose without applying
	// the filter
	if (!bUseAAFilter)
	{
		uint32_t sizeReq = static_cast<uint32_t>(nSamples / this->fRate + 1.0f);
		uint32_t count = this->transpose(this->outputBuffer.ptrEnd(sizeReq), src, nSamples);
		this->outputBuffer.putSamples(count);
		return;
	}

	// Transpose with anti-alias filter
	if (this->fRate < 1.0f)
		this->upsample(src, nSamples);
	else
		this->downsample(src, nSamples);
}

// Transposes the sample rate of the given samples using linear interpolation.
// Returns the number of samples returned in the "dest" buffer
uint32_t RateTransposer::transpose(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t nSamples)
{
	if (this->numChannels == 2)
		return this->transposeStereo(dest, src, nSamples);
	else
		return this->transposeMono(dest, src, nSamples);
}

// Sets the number of channels, 1 = mono, 2 = stereo
void RateTransposer::setChannels(int32_t nChannels)
{
	assert(nChannels > 0);
	if (this->numChannels == nChannels)
		return;

	assert(nChannels == 1 || nChannels == 2);
	this->numChannels = nChannels;

	this->storeBuffer.setChannels(this->numChannels);
	this->tempBuffer.setChannels(this->numChannels);
	this->outputBuffer.setChannels(this->numChannels);

	// Inits the linear interpolation registers
	this->resetRegisters();
}

// Clears all the samples in the object
void RateTransposer::clear()
{
	this->outputBuffer.clear();
	this->storeBuffer.clear();
}

// Returns nonzero if there aren't any samples available for outputting.
bool RateTransposer::isEmpty() const
{
	bool res = FIFOProcessor::isEmpty();
	if (!res)
		return false;
	return this->storeBuffer.isEmpty();
}

//////////////////////////////////////////////////////////////////////////////
//
// RateTransposerInteger - integer arithmetic implementation
//

/// fixed-point interpolation routine precision
static const int SCALE = 65536;

// Constructor
RateTransposerInteger::RateTransposerInteger() : RateTransposer()
{
	// Notice: use local function calling syntax for sake of clarity,
	// to indicate the fact that C++ constructor can't call virtual functions.
	RateTransposerInteger::resetRegisters();
	RateTransposerInteger::setRate(1.0f);
}

RateTransposerInteger::~RateTransposerInteger()
{
}

void RateTransposerInteger::resetRegisters()
{
	this->iSlopeCount = 0;
	this->sPrevSampleL = this->sPrevSampleR = 0;
}

// Transposes the sample rate of the given samples using linear interpolation.
// 'Mono' version of the routine. Returns the number of samples returned in
// the "dest" buffer
uint32_t RateTransposerInteger::transposeMono(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t nSamples)
{
	if (!nSamples)
		return 0; // no samples, no work

	unsigned used = 0, i = 0;

	// Process the last sample saved from the previous call first...
	LONG_SAMPLETYPE temp, vol1;
	while (this->iSlopeCount <= SCALE)
	{
		vol1 = SCALE - this->iSlopeCount;
		temp = vol1 * this->sPrevSampleL + this->iSlopeCount * src[0];
		dest[i] = static_cast<SAMPLETYPE>(temp / SCALE);
		++i;
		this->iSlopeCount += this->iRate;
	}
	// now always (iSlopeCount > SCALE)
	this->iSlopeCount -= SCALE;

	while (1)
	{
		while (this->iSlopeCount > SCALE)
		{
			this->iSlopeCount -= SCALE;
			++used;
			if (used >= nSamples - 1)
				goto end;
		}
		vol1 = SCALE - this->iSlopeCount;
		temp = src[used] * vol1 + this->iSlopeCount * src[used + 1];
		dest[i] = static_cast<SAMPLETYPE>(temp / SCALE);
		++i;
		this->iSlopeCount += this->iRate;
	}
end:
	// Store the last sample for the next round
	this->sPrevSampleL = src[nSamples - 1];

	return i;
}

// Transposes the sample rate of the given samples using linear interpolation.
// 'Stereo' version of the routine. Returns the number of samples returned in
// the "dest" buffer
uint32_t RateTransposerInteger::transposeStereo(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t nSamples)
{
	if (!nSamples)
		return 0; // no samples, no work

	unsigned used = 0, i = 0;

	// Process the last sample saved from the sPrevSampleLious call first...
	LONG_SAMPLETYPE temp, vol1;
	while (this->iSlopeCount <= SCALE)
	{
		vol1 = SCALE - this->iSlopeCount;
		temp = vol1 * this->sPrevSampleL + this->iSlopeCount * src[0];
		dest[2 * i] = static_cast<SAMPLETYPE>(temp / SCALE);
		temp = vol1 * this->sPrevSampleR + this->iSlopeCount * src[1];
		dest[2 * i + 1] = static_cast<SAMPLETYPE>(temp / SCALE);
		++i;
		this->iSlopeCount += this->iRate;
	}
	// now always (iSlopeCount > SCALE)
	this->iSlopeCount -= SCALE;

	while (1)
	{
		while (this->iSlopeCount > SCALE)
		{
			this->iSlopeCount -= SCALE;
			++used;
			if (used >= nSamples - 1)
				goto end;
		}
		unsigned srcPos = 2 * used;
		vol1 = SCALE - this->iSlopeCount;
		temp = src[srcPos] * vol1 + this->iSlopeCount * src[srcPos + 2];
		dest[2 * i] = static_cast<SAMPLETYPE>(temp / SCALE);
		temp = src[srcPos + 1] * vol1 + this->iSlopeCount * src[srcPos + 3];
		dest[2 * i + 1] = static_cast<SAMPLETYPE>(temp / SCALE);
		++i;
		this->iSlopeCount += this->iRate;
	}
end:
	// Store the last sample for the next round
	this->sPrevSampleL = src[2 * nSamples - 2];
	this->sPrevSampleR = src[2 * nSamples - 1];

	return i;
}

// Sets new target iRate. Normal iRate = 1.0, smaller values represent slower
// iRate, larger faster iRates.
void RateTransposerInteger::setRate(float newRate)
{
	this->iRate = static_cast<int>(newRate * SCALE + 0.5f);
	RateTransposer::setRate(newRate);
}

//////////////////////////////////////////////////////////////////////////////
//
// RateTransposerFloat - floating point arithmetic implementation
//
//////////////////////////////////////////////////////////////////////////////

// Constructor
RateTransposerFloat::RateTransposerFloat() : RateTransposer()
{
	// Notice: use local function calling syntax for sake of clarity,
	// to indicate the fact that C++ constructor can't call virtual functions.
	RateTransposerFloat::resetRegisters();
	RateTransposerFloat::setRate(1.0f);
}

RateTransposerFloat::~RateTransposerFloat()
{
}

void RateTransposerFloat::resetRegisters()
{
	this->fSlopeCount = 0;
	this->sPrevSampleL = this->sPrevSampleR = 0;
}

// Transposes the sample rate of the given samples using linear interpolation.
// 'Mono' version of the routine. Returns the number of samples returned in
// the "dest" buffer
uint32_t RateTransposerFloat::transposeMono(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t nSamples)
{
	unsigned used = 0, i = 0;

	// Process the last sample saved from the previous call first...
	while (this->fSlopeCount <= 1.0f)
	{
		dest[i] = static_cast<SAMPLETYPE>((1.0f - this->fSlopeCount) * this->sPrevSampleL + this->fSlopeCount * src[0]);
		++i;
		this->fSlopeCount += this->fRate;
	}
	this->fSlopeCount -= 1.0f;

	if (nSamples > 1)
	{
		while (1)
		{
			while (this->fSlopeCount > 1.0f)
			{
				this->fSlopeCount -= 1.0f;
				++used;
				if (used >= nSamples - 1)
					goto end;
			}
			dest[i] = static_cast<SAMPLETYPE>((1.0f - this->fSlopeCount) * src[used] + this->fSlopeCount * src[used + 1]);
			++i;
			this->fSlopeCount += this->fRate;
		}
	}
end:
	// Store the last sample for the next round
	this->sPrevSampleL = src[nSamples - 1];

	return i;
}

// Transposes the sample rate of the given samples using linear interpolation.
// 'Mono' version of the routine. Returns the number of samples returned in
// the "dest" buffer
uint32_t RateTransposerFloat::transposeStereo(SAMPLETYPE *dest, const SAMPLETYPE *src, uint32_t nSamples)
{
	if (!nSamples)
		return 0; // no samples, no work

	unsigned used = 0, i = 0;

	// Process the last sample saved from the sPrevSampleLious call first...
	while (this->fSlopeCount <= 1.0f)
	{
		dest[2 * i] = static_cast<SAMPLETYPE>((1.0f - this->fSlopeCount) * this->sPrevSampleL + this->fSlopeCount * src[0]);
		dest[2 * i + 1] = static_cast<SAMPLETYPE>((1.0f - this->fSlopeCount) * this->sPrevSampleR + this->fSlopeCount * src[1]);
		++i;
		this->fSlopeCount += this->fRate;
	}
	// now always (iSlopeCount > 1.0f)
	this->fSlopeCount -= 1.0f;

	if (nSamples > 1)
	{
		while (1)
		{
			while (this->fSlopeCount > 1.0f)
			{
				this->fSlopeCount -= 1.0f;
				++used;
				if (used >= nSamples - 1)
					goto end;
			}
			unsigned srcPos = 2 * used;

			dest[2 * i] = static_cast<SAMPLETYPE>((1.0f - this->fSlopeCount) * src[srcPos] + this->fSlopeCount * src[srcPos + 2]);
			dest[2 * i + 1] = static_cast<SAMPLETYPE>((1.0f - this->fSlopeCount) * src[srcPos + 1] + this->fSlopeCount * src[srcPos + 3]);
			++i;
			this->fSlopeCount += this->fRate;
		}
	}
end:
	// Store the last sample for the next round
	this->sPrevSampleL = src[2 * nSamples - 2];
	this->sPrevSampleR = src[2 * nSamples - 1];

	return i;
}
