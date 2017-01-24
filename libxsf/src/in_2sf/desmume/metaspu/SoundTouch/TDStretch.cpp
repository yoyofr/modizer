////////////////////////////////////////////////////////////////////////////////
///
/// Sampled sound tempo changer/time stretch algorithm. Changes the sound tempo
/// while maintaining the original pitch by using a time domain WSOLA-like
/// method with several performance-increasing tweaks.
///
/// Note : MMX optimized functions reside in a separate, platform-specific
/// file, e.g. 'mmx_win.cpp' or 'mmx_gcc.cpp'
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-11-08 16:53:01 -0200 (qui, 08 nov 2012) $
// File revision : $Revision: 1.12 $
//
// $Id: TDStretch.cpp 160 2012-11-08 18:53:01Z oparviai $
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

#include <stdexcept>
#include <limits>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include "STTypes.h"
#include "cpu_detect.h"
#include "TDStretch.h"

using namespace soundtouch;

/*****************************************************************************
 *
 * Constant definitions
 *
 *****************************************************************************/

// Table for the hierarchical mixing position seeking algorithm
static const short _scanOffsets[][24] =
{
	{ 124, 186, 248, 310, 372, 434, 496, 558, 620, 682, 744, 806, 868, 930, 992, 1054, 1116, 1178, 1240, 1302, 1364, 1426, 1488, 0 },
	{-100, -75, -50, -25, 25, 50, 75, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ -20, -15, -10,  -5, 5, 10, 15, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ -4, -3, -2, -1, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 121, 114, 97, 114, 98, 105, 108, 32, 104, 99, 117, 111, 116, 100, 110, 117, 111, 115, 0, 0, 0, 0, 0, 0 }
};

/*****************************************************************************
 *
 * Implementation of the class 'TDStretch'
 *
 *****************************************************************************/


TDStretch::TDStretch() : FIFOProcessor(&outputBuffer)
{
	this->bQuickSeek = false;
	this->channels = 2;

	this->pMidBuffer = nullptr;
	this->pMidBufferUnaligned.reset();
	this->overlapLength = 0;

	this->bAutoSeqSetting = true;
	this->bAutoSeekSetting = true;

	//this->outDebt = 0;
	this->skipFract = 0;

	this->tempo = 1.0f;
	this->setParameters(48000, DEFAULT_SEQUENCE_MS, DEFAULT_SEEKWINDOW_MS, DEFAULT_OVERLAP_MS);
	this->setTempo(1.0f);

	this->clear();
}

TDStretch::~TDStretch()
{
}

// Sets routine control parameters. These control are certain time constants
// defining how the sound is stretched to the desired duration.
//
// 'sampleRate' = sample rate of the sound
// 'sequenceMS' = one processing sequence length in milliseconds (default = 82 ms)
// 'seekwindowMS' = seeking window length for scanning the best overlapping
//      position (default = 28 ms)
// 'overlapMS' = overlapping length (default = 12 ms)

void TDStretch::setParameters(int32_t aSampleRate, int32_t aSequenceMS, int32_t aSeekWindowMS, int32_t aOverlapMS)
{
	// accept only positive parameter values - if zero or negative, use old values instead
	if (aSampleRate > 0)
		this->sampleRate = aSampleRate;
	if (aOverlapMS > 0)
		this->overlapMs = aOverlapMS;

	if (aSequenceMS > 0)
	{
		this->sequenceMs = aSequenceMS;
		this->bAutoSeqSetting = false;
	}
	else if (!aSequenceMS)
		// if zero, use automatic setting
		this->bAutoSeqSetting = true;

	if (aSeekWindowMS > 0)
	{
		this->seekWindowMs = aSeekWindowMS;
		this->bAutoSeekSetting = false;
	}
	else if (!aSeekWindowMS)
		// if zero, use automatic setting
		this->bAutoSeekSetting = true;

	this->calcSeqParameters();

	this->calculateOverlapLength(this->overlapMs);

	// set tempo to recalculate 'sampleReq'
	this->setTempo(this->tempo);
}

/// Get routine control parameters, see setParameters() function.
/// Any of the parameters to this function can be NULL, in such case corresponding parameter
/// value isn't returned.
void TDStretch::getParameters(int32_t *pSampleRate, int32_t *pSequenceMs, int32_t *pSeekWindowMs, int32_t *pOverlapMs)
{
	if (pSampleRate)
		*pSampleRate = this->sampleRate;

	if (pSequenceMs)
		*pSequenceMs = this->bAutoSeqSetting ? USE_AUTO_SEQUENCE_LEN : sequenceMs;

	if (pSeekWindowMs)
		*pSeekWindowMs = this->bAutoSeekSetting ? USE_AUTO_SEEKWINDOW_LEN : seekWindowMs;

	if (pOverlapMs)
		*pOverlapMs = this->overlapMs;
}

// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch::overlapMono(SAMPLETYPE *pOutput, const SAMPLETYPE *pInput) const
{
	SAMPLETYPE m1 = 0;
	SAMPLETYPE m2 = static_cast<SAMPLETYPE>(this->overlapLength);

	for (int32_t i = 0; i < this->overlapLength; ++i)
	{
		pOutput[i] = (pInput[i] * m1 + this->pMidBuffer[i] * m2) / this->overlapLength;
		++m1;
		--m2;
	}
}

void TDStretch::clearMidBuffer()
{
	memset(this->pMidBuffer, 0, 2 * sizeof(SAMPLETYPE) * this->overlapLength);
}

void TDStretch::clearInput()
{
	this->inputBuffer.clear();
	this->clearMidBuffer();
}

// Clears the sample buffers
void TDStretch::clear()
{
	this->outputBuffer.clear();
	this->clearInput();
}

// Enables/disables the quick position seeking algorithm. Zero to disable, nonzero
// to enable
void TDStretch::enableQuickSeek(bool enable)
{
	this->bQuickSeek = enable;
}

// Seeks for the optimal overlap-mixing position.
int32_t TDStretch::seekBestOverlapPosition(const SAMPLETYPE *refPos)
{
	if (this->bQuickSeek)
		return this->seekBestOverlapPositionQuick(refPos);
	else
		return this->seekBestOverlapPositionFull(refPos);
}

// Overlaps samples in 'midBuffer' with the samples in 'pInputBuffer' at position
// of 'ovlPos'.
void TDStretch::overlap(SAMPLETYPE *pOutput, const SAMPLETYPE *pInput, uint32_t ovlPos) const
{
	if (this->channels == 2)
		// stereo sound
		this->overlapStereo(pOutput, pInput + 2 * ovlPos);
	else
		// mono sound.
		this->overlapMono(pOutput, pInput + ovlPos);
}

// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
int32_t TDStretch::seekBestOverlapPositionFull(const SAMPLETYPE *refPos)
{
	double bestCorr = std::numeric_limits<float>::min();
	int32_t bestOffs = 0;

	// Scans for the best correlation value by testing each possible position
	// over the permitted range.
	for (int32_t i = 0; i < this->seekLength; ++i)
	{
		// Calculates correlation value for the mixing position corresponding
		// to 'i'
		double corr = this->calcCrossCorr(refPos + this->channels * i, this->pMidBuffer);
		// heuristic rule to slightly favour values close to mid of the range
		double tmp = (2.0 * i - this->seekLength) / this->seekLength;
		corr = (corr + 0.1) * (1.0 - 0.25 * tmp * tmp);

		// Checks for the highest correlation value
		if (corr > bestCorr)
		{
			bestCorr = corr;
			bestOffs = i;
		}
	}
	// clear cross correlation routine state if necessary (is so e.g. in MMX routines).
	this->clearCrossCorrState();

	return bestOffs;
}

// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
int32_t TDStretch::seekBestOverlapPositionQuick(const SAMPLETYPE *refPos)
{
	double bestCorr = std::numeric_limits<float>::min();
	int32_t bestOffs = _scanOffsets[0][0], corrOffset = 0;

	// Scans for the best correlation value using four-pass hierarchical search.
	//
	// The look-up table 'scans' has hierarchical position adjusting steps.
	// In first pass the routine searhes for the highest correlation with
	// relatively coarse steps, then rescans the neighbourhood of the highest
	// correlation with better resolution and so on.
	for (uint32_t scanCount = 0; scanCount < 4; ++scanCount)
	{
		int32_t j = 0;
		while (_scanOffsets[scanCount][j])
		{
			int32_t tempOffset = corrOffset + _scanOffsets[scanCount][j];
			if (tempOffset >= this->seekLength)
				break;

			// Calculates correlation value for the mixing position corresponding
			// to 'tempOffset'
			double corr = this->calcCrossCorr(refPos + this->channels * tempOffset, this->pMidBuffer);
			// heuristic rule to slightly favour values close to mid of the range
			double tmp = (2.0 * tempOffset - this->seekLength) / seekLength;
			corr = (corr + 0.1) * (1.0 - 0.25 * tmp * tmp);

			// Checks for the highest correlation value
			if (corr > bestCorr)
			{
				bestCorr = corr;
				bestOffs = tempOffset;
			}
			++j;
		}
		corrOffset = bestOffs;
	}
	// clear cross correlation routine state if necessary (is so e.g. in MMX routines).
	this->clearCrossCorrState();

	return bestOffs;
}

/// clear cross correlation routine state if necessary
void TDStretch::clearCrossCorrState()
{
	// default implementation is empty.
}

/// Calculates processing sequence length according to tempo setting
void TDStretch::calcSeqParameters()
{
	// Adjust tempo param according to tempo, so that variating processing sequence length is used
	// at varius tempo settings, between the given low...top limits
	static const double AUTOSEQ_TEMPO_LOW = 0.5; // auto setting low tempo range (-50%)
	static const double AUTOSEQ_TEMPO_TOP = 2.0; // auto setting top tempo range (+100%)

	// sequence-ms setting values at above low & top tempo
	static const double AUTOSEQ_AT_MIN = 125.0;
	static const double AUTOSEQ_AT_MAX = 50.0;
	static const double AUTOSEQ_K = (AUTOSEQ_AT_MAX - AUTOSEQ_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW);
	static const double AUTOSEQ_C = AUTOSEQ_AT_MIN - AUTOSEQ_K * AUTOSEQ_TEMPO_LOW;

	// seek-window-ms setting values at above low & top tempo
	static const double AUTOSEEK_AT_MIN = 25.0;
	static const double AUTOSEEK_AT_MAX = 15.0;
	static const double AUTOSEEK_K = (AUTOSEEK_AT_MAX - AUTOSEEK_AT_MIN) / (AUTOSEQ_TEMPO_TOP - AUTOSEQ_TEMPO_LOW);
	static const double AUTOSEEK_C = AUTOSEEK_AT_MIN - AUTOSEEK_K * AUTOSEQ_TEMPO_LOW;

	auto CHECK_LIMITS = [](double x, double mi, double ma) { return x < mi ? mi : (x > ma ? ma : x); };

	if (this->bAutoSeqSetting)
	{
		double seq = AUTOSEQ_C + AUTOSEQ_K * this->tempo;
		seq = CHECK_LIMITS(seq, AUTOSEQ_AT_MAX, AUTOSEQ_AT_MIN);
		this->sequenceMs = static_cast<int>(seq + 0.5);
	}

	if (this->bAutoSeekSetting)
	{
		double seek = AUTOSEEK_C + AUTOSEEK_K * this->tempo;
		seek = CHECK_LIMITS(seek, AUTOSEEK_AT_MAX, AUTOSEEK_AT_MIN);
		this->seekWindowMs = static_cast<int>(seek + 0.5);
    }

	// Update seek window lengths
	this->seekWindowLength = (this->sampleRate * this->sequenceMs) / 1000;
	if (this->seekWindowLength < 2 * this->overlapLength)
		this->seekWindowLength = 2 * this->overlapLength;
	this->seekLength = (this->sampleRate * this->seekWindowMs) / 1000;
}

// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower
// tempo, larger faster tempo.
void TDStretch::setTempo(float newTempo)
{
	this->tempo = newTempo;

	// Calculate new sequence duration
	this->calcSeqParameters();

	// Calculate ideal skip length (according to tempo value)
	this->nominalSkip = this->tempo * (this->seekWindowLength - this->overlapLength);
	int intskip = static_cast<int>(nominalSkip + 0.5f);

	// Calculate how many samples are needed in the 'inputBuffer' to
	// process another batch of samples
	this->sampleReq = std::max(intskip + this->overlapLength, this->seekWindowLength) + this->seekLength;
}

// Sets the number of channels, 1 = mono, 2 = stereo
void TDStretch::setChannels(int32_t numChannels)
{
	assert(numChannels > 0);
	if (this->channels == numChannels)
		return;
	assert(numChannels == 1 || numChannels == 2);

	this->channels = numChannels;
	this->inputBuffer.setChannels(channels);
	this->outputBuffer.setChannels(channels);
}

// Processes as many processing frames of the samples 'inputBuffer', store
// the result into 'outputBuffer'
void TDStretch::processSamples()
{
	/* Removed this small optimization - can introduce a click to sound when tempo setting
	   crosses the nominal value
	if (this->tempo == 1.0f)
	{
		// tempo not changed from the original, so bypass the processing
		this->processNominalTempo();
		return;
	}*/

	// Process samples as long as there are enough samples in 'inputBuffer'
	// to form a processing frame.
	while (static_cast<int32_t>(this->inputBuffer.numSamples()) >= this->sampleReq)
	{
		// If tempo differs from the normal ('SCALE'), scan for the best overlapping
		// position
		int32_t offset = this->seekBestOverlapPosition(this->inputBuffer.ptrBegin());

		// Mix the samples in the 'inputBuffer' at position of 'offset' with the
		// samples in 'midBuffer' using sliding overlapping
		// ... first partially overlap with the end of the previous sequence
		// (that's in 'midBuffer')
		this->overlap(this->outputBuffer.ptrEnd(this->overlapLength), this->inputBuffer.ptrBegin(), offset);
		this->outputBuffer.putSamples(this->overlapLength);

		// ... then copy sequence samples from 'inputBuffer' to output:

		// length of sequence
		int temp = this->seekWindowLength - 2 * this->overlapLength;

		// crosscheck that we don't have buffer overflow...
		if (static_cast<int32_t>(inputBuffer.numSamples()) < offset + temp + this->overlapLength * 2)
			continue; // just in case, shouldn't really happen

		this->outputBuffer.putSamples(this->inputBuffer.ptrBegin() + this->channels * (offset + this->overlapLength), temp);

		// Copies the end of the current sequence from 'inputBuffer' to
		// 'midBuffer' for being mixed with the beginning of the next
		// processing sequence and so on
		assert(offset + temp + this->overlapLength * 2 <= static_cast<int32_t>(this->inputBuffer.numSamples()));
		memcpy(this->pMidBuffer, this->inputBuffer.ptrBegin() + this->channels * (offset + this->seekWindowLength - this->overlapLength), this->channels * sizeof(SAMPLETYPE) * this->overlapLength);

		// Remove the processed samples from the input buffer. Update
		// the difference between integer & nominal skip step to 'skipFract'
		// in order to prevent the error from accumulating over time.
		this->skipFract += this->nominalSkip; // real skip size
		int ovlSkip = static_cast<int>(skipFract); // rounded to integer skip
		this->skipFract -= ovlSkip; // maintain the fraction part, i.e. real vs. integer skip
		this->inputBuffer.receiveSamples(ovlSkip);
	}
}

// Adds 'numsamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void TDStretch::putSamples(const SAMPLETYPE *samples, uint32_t nSamples)
{
	// Add the samples into the input buffer
	this->inputBuffer.putSamples(samples, nSamples);
	// Process the samples in input buffer
	this->processSamples();
}

/// Set new overlap length parameter & reallocate RefMidBuffer if necessary.
void TDStretch::acceptNewOverlapLength(int32_t newOverlapLength)
{
	assert(newOverlapLength >= 0);
	int32_t prevOvl = this->overlapLength;
	this->overlapLength = newOverlapLength;

	if (this->overlapLength > prevOvl)
	{
		this->pMidBufferUnaligned.reset(new SAMPLETYPE[this->overlapLength * 2 + 16 / sizeof(SAMPLETYPE)]);
		// ensure that 'pMidBuffer' is aligned to 16 byte boundary for efficiency
		this->pMidBuffer = reinterpret_cast<SAMPLETYPE *>(SOUNDTOUCH_ALIGN_POINTER_16(this->pMidBufferUnaligned.get()));

		this->clearMidBuffer();
	}
}

// Operator 'new' is overloaded so that it automatically creates a suitable instance
// depending on if we've a MMX/SSE/etc-capable CPU available or not.
void *TDStretch::operator new(size_t /*s*/)
{
	// Notice! don't use "new TDStretch" directly, use "newInstance" to create a new instance instead!
	//assert(false);
	//return NULL;
	throw std::runtime_error("Don't use 'new TDStretch', use 'newInstance' member instead!");
}

TDStretch *TDStretch::newInstance()
{
	uint32_t uExtensions = detectCPUextensions();

	// Check if MMX/SSE instruction set extensions supported by CPU

#ifdef SOUNDTOUCH_ALLOW_MMX
	// MMX routines available only with integer sample types
	if (uExtensions & SUPPORT_MMX)
		return ::new TDStretchMMX;
	else
#endif // SOUNDTOUCH_ALLOW_MMX
#ifdef SOUNDTOUCH_ALLOW_SSE
	if (uExtensions & SUPPORT_SSE)
		// SSE support
		return ::new TDStretchSSE;
	else
#endif //  SOUNDTOUCH_ALLOW_SSE
		// ISA optimizations not supported, use plain C version
		return ::new TDStretch;
}

//////////////////////////////////////////////////////////////////////////////
//
// Integer arithmetics specific algorithm implementations.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef SOUNDTOUCH_INTEGER_SAMPLES

// Overlaps samples in 'midBuffer' with the samples in 'pinput'. The 'Stereo'
// version of the routine.
void TDStretch::overlapStereo(short *poutput, const short *pinput) const
{
	for (int32_t i = 0; i < this->overlapLength; ++i)
	{
		short temp = this->overlapLength - i;
		int32_t cnt2 = 2 * i;
		poutput[cnt2] = (pinput[cnt2] * i + this->pMidBuffer[cnt2] * temp) / this->overlapLength;
		poutput[cnt2 + 1] = (pinput[cnt2 + 1] * i + this->pMidBuffer[cnt2 + 1] * temp) / this->overlapLength;
	}
}

// Calculates the x having the closest 2^x value for the given value
static int _getClosest2Power(double value)
{
	return static_cast<int>(std::log(value) / std::log(2.0) + 0.5);
}

/// Calculates overlap period length in samples.
/// Integer version rounds overlap length to closest power of 2
/// for a divide scaling operation.
void TDStretch::calculateOverlapLength(int32_t aoverlapMs)
{
	assert(aoverlapMs >= 0);

	// calculate overlap length so that it's power of 2 - thus it's easy to do
	// integer division by right-shifting. Term "-1" at end is to account for
	// the extra most significatnt bit left unused in result by signed multiplication
	this->overlapDividerBits = _getClosest2Power((this->sampleRate * aoverlapMs) / 1000.0) - 1;
	if (this->overlapDividerBits > 9)
		this->overlapDividerBits = 9;
	if (this->overlapDividerBits < 3)
		this->overlapDividerBits = 3;
	int32_t newOvl = static_cast<int32_t>(std::pow(2, this->overlapDividerBits + 1)); // +1 => account for -1 above

	this->acceptNewOverlapLength(newOvl);

	// calculate sloping divider so that crosscorrelation operation won't
	// overflow 32-bit register. Max. sum of the crosscorrelation sum without
	// divider would be 2^30*(N^3-N)/3, where N = overlap length
	this->slopingDivider = (newOvl * newOvl - 1) / 3;
}

double TDStretch::calcCrossCorr(const short *mixingPos, const short *compare) const
{
	long corr = 0, norm = 0;
	// Same routine for stereo and mono. For stereo, unroll loop for better
	// efficiency and gives slightly better resolution against rounding.
	// For mono it same routine, just  unrolls loop by factor of 4
	for (int32_t i = 0; i < this->overlapLength; i += 4)
	{
		corr += (mixingPos[i] * compare[i] + mixingPos[i + 1] * compare[i + 1] + mixingPos[i + 2] * compare[i + 2] + mixingPos[i + 3] * compare[i + 3]) >> this->overlapDividerBits;
		norm += (mixingPos[i] * mixingPos[i] + mixingPos[i + 1] * mixingPos[i + 1] + mixingPos[i + 2] * mixingPos[i + 2] + mixingPos[i + 3] * mixingPos[i + 3]) >> this->overlapDividerBits;
	}

	// Normalize result by dividing by sqrt(norm) - this step is easiest
	// done using floating point operation
	if (!norm)
		norm = 1; // to avoid div by zero
	return corr / std::sqrt(static_cast<double>(norm));
}

#endif // SOUNDTOUCH_INTEGER_SAMPLES

//////////////////////////////////////////////////////////////////////////////
//
// Floating point arithmetics specific algorithm implementations.
//

#ifdef SOUNDTOUCH_FLOAT_SAMPLES

// Overlaps samples in 'midBuffer' with the samples in 'pInput'
void TDStretch::overlapStereo(float *pOutput, const float *pInput) const
{
	float fScale = 1.0f / this->overlapLength;

	float f1 = 0, f2 = 1.0f;

	for (int32_t i = 0; i < 2 * this->overlapLength; i += 2)
	{
		pOutput[i] = pInput[i] * f1 + this->pMidBuffer[i] * f2;
		pOutput[i + 1] = pInput[i + 1] * f1 + this->pMidBuffer[i + 1] * f2;

		f1 += fScale;
		f2 -= fScale;
	}
}

/// Calculates overlapInMsec period length in samples.
void TDStretch::calculateOverlapLength(int32_t overlapInMsec)
{
	assert(overlapInMsec >= 0);
	uint32_t newOvl = (this->sampleRate * overlapInMsec) / 1000;
	if (newOvl < 16)
		newOvl = 16;

	// must be divisible by 8
	newOvl -= newOvl % 8;

	this->acceptNewOverlapLength(newOvl);
}

double TDStretch::calcCrossCorr(const float *mixingPos, const float *compare) const
{
	double corr = 0, norm = 0;
	// Same routine for stereo and mono. For Stereo, unroll by factor of 2.
	// For mono it's same routine yet unrollsd by factor of 4.
	for (int32_t i = 0; i < this->channels * this->overlapLength; i += 4)
	{
		corr += mixingPos[i] * compare[i] + mixingPos[i + 1] * compare[i + 1];

		norm += mixingPos[i] * mixingPos[i] +  mixingPos[i + 1] * mixingPos[i + 1];

		// unroll the loop for better CPU efficiency:
		corr += mixingPos[i + 2] * compare[i + 2] + mixingPos[i + 3] * compare[i + 3];

		norm += mixingPos[i + 2] * mixingPos[i + 2] + mixingPos[i + 3] * mixingPos[i + 3];
	}

	if (norm < 1e-9)
		norm = 1.0; // to avoid div by zero
	return corr / std::sqrt(norm);
}

#endif // SOUNDTOUCH_FLOAT_SAMPLES
