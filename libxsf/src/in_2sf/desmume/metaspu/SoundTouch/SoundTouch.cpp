//////////////////////////////////////////////////////////////////////////////
///
/// SoundTouch - main class for tempo/pitch/rate adjusting routines.
///
/// Notes:
/// - Initialize the SoundTouch object instance by setting up the sound stream
///   parameters with functions 'setSampleRate' and 'setChannels', then set
///   desired tempo/pitch/rate settings with the corresponding functions.
///
/// - The SoundTouch class behaves like a first-in-first-out pipeline: The
///   samples that are to be processed are fed into one of the pipe by calling
///   function 'putSamples', while the ready processed samples can be read
///   from the other end of the pipeline with function 'receiveSamples'.
///
/// - The SoundTouch processing classes require certain sized 'batches' of
///   samples in order to process the sound. For this reason the classes buffer
///   incoming samples until there are enough of samples available for
///   processing, then they carry out the processing step and consequently
///   make the processed samples available for outputting.
///
/// - For the above reason, the processing routines introduce a certain
///   'latency' between the input and output, so that the samples input to
///   SoundTouch may not be immediately available in the output, and neither
///   the amount of outputtable samples may not immediately be in direct
///   relationship with the amount of previously input samples.
///
/// - The tempo/pitch/rate control parameters can be altered during processing.
///   Please notice though that they aren't currently protected by semaphores,
///   so in multi-thread application external semaphore protection may be
///   required.
///
/// - This class utilizes classes 'TDStretch' for tempo change (without modifying
///   pitch) and 'RateTransposer' for changing the playback rate (that is, both
///   tempo and pitch in the same ratio) of the sound. The third available control
///   'pitch' (change pitch but maintain tempo) is produced by a combination of
///   combining the two other controls.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-06-13 16:29:53 -0300 (qua, 13 jun 2012) $
// File revision : $Revision: 4 $
//
// $Id: SoundTouch.cpp 143 2012-06-13 19:29:53Z oparviai $
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
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "SoundTouch.h"
#include "cpu_detect.h"

using namespace soundtouch;

SoundTouch::SoundTouch()
{
	// Initialize rate transposer and tempo changer instances

	this->pRateTransposer.reset(RateTransposer::newInstance());
	this->pTDStretch.reset(TDStretch::newInstance());

	this->setOutPipe(this->pTDStretch.get());

	this->rate = this->tempo = 0;

	this->virtualPitch = this->virtualRate = this->virtualTempo = 1.0;

	this->calcEffectiveRateAndTempo();

	this->channels = 0;
	this->bSrateSet = false;
}

// Sets the number of channels, 1 = mono, 2 = stereo
void SoundTouch::setChannels(uint32_t numChannels)
{
	if (numChannels != 1 && numChannels != 2)
		throw std::runtime_error("Illegal number of channels");
	this->channels = numChannels;
	this->pRateTransposer->setChannels(numChannels);
	this->pTDStretch->setChannels(numChannels);
}

// Sets new rate control value. Normal rate = 1.0, smaller values
// represent slower rate, larger faster rates.
void SoundTouch::setRate(float newRate)
{
	this->virtualRate = newRate;
	this->calcEffectiveRateAndTempo();
}

// Sets new tempo control value. Normal tempo = 1.0, smaller values
// represent slower tempo, larger faster tempo.
void SoundTouch::setTempo(float newTempo)
{
	this->virtualTempo = newTempo;
	this->calcEffectiveRateAndTempo();
}

// Calculates 'effective' rate and tempo values from the
// nominal control values.
void SoundTouch::calcEffectiveRateAndTempo()
{
	float oldTempo = this->tempo;
	float oldRate = this->rate;

	this->tempo = this->virtualTempo / this->virtualPitch;
	this->rate = this->virtualPitch * this->virtualRate;

	if (!fEqual(this->rate, oldRate))
		this->pRateTransposer->setRate(this->rate);
	if (!fEqual(this->tempo, oldTempo))
		this->pTDStretch->setTempo(this->tempo);

#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
	if (this->rate <= 1.0f)
	{
		if (this->output != this->pTDStretch.get())
		{
			FIFOSamplePipe *tempoOut;

			assert(this->output == this->pRateTransposer.get());
			// move samples in the current output buffer to the output of pTDStretch
			tempoOut = this->pTDStretch->getOutput();
			tempoOut->moveSamples(*this->output);
			// move samples in pitch transposer's store buffer to tempo changer's input
			this->pTDStretch->moveSamples(*this->pRateTransposer->getStore());

			this->output = pTDStretch.get();
		}
	}
	else
#endif
	{
		if (this->output != this->pRateTransposer.get())
		{
			assert(this->output == this->pTDStretch.get());
			// move samples in the current output buffer to the output of pRateTransposer
			FIFOSamplePipe *transOut = this->pRateTransposer->getOutput();
			transOut->moveSamples(*this->output);
			// move samples in tempo changer's input to pitch transposer's input
			this->pRateTransposer->moveSamples(*this->pTDStretch->getInput());

			this->output = this->pRateTransposer.get();
		}
	}
}

// Sets sample rate.
void SoundTouch::setSampleRate(uint32_t srate)
{
	this->bSrateSet = true;
	// set sample rate, leave other tempo changer parameters as they are.
	this->pTDStretch->setParameters(srate);
}

// Adds 'numSamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void SoundTouch::putSamples(const SAMPLETYPE *samples, uint32_t nSamples)
{
	if (!this->bSrateSet)
		throw std::runtime_error("SoundTouch : Sample rate not defined");
	else if (!this->channels)
		throw std::runtime_error("SoundTouch : Number of channels not defined");

	// Transpose the rate of the new samples if necessary
	/* Bypass the nominal setting - can introduce a click in sound when tempo/pitch control crosses the nominal value...
	if (this->rate == 1.0f)
	{
		// The rate value is same as the original, simply evaluate the tempo changer.
		assert(this->output == this->pTDStretch.get());
		if (!this->pRateTransposer->isEmpty())
		{
			// yet flush the last samples in the pitch transposer buffer
			// (may happen if 'rate' changes from a non-zero value to zero)
			this->pTDStretch->moveSamples(*this->pRateTransposer);
		}
		this->pTDStretch->putSamples(samples, nSamples);
	}*/
#ifndef SOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER
	else if (this->rate <= 1.0f)
	{
		// transpose the rate down, output the transposed sound to tempo changer buffer
		assert(this->output == this->pTDStretch.get());
		this->pRateTransposer->putSamples(samples, nSamples);
		this->pTDStretch->moveSamples(*this->pRateTransposer);
	}
	else
#endif
	{
		assert(this->rate > 1.0f);
		// evaluate the tempo changer, then transpose the rate up,
		assert(this->output == this->pRateTransposer.get());
		this->pTDStretch->putSamples(samples, nSamples);
		this->pRateTransposer->moveSamples(*this->pTDStretch);
	}
}

// Flushes the last samples from the processing pipeline to the output.
// Clears also the internal processing buffers.
//
// Note: This function is meant for extracting the last samples of a sound
// stream. This function may introduce additional blank samples in the end
// of the sound stream, and thus it's not recommended to call this function
// in the middle of a sound stream.
void SoundTouch::flush()
{
	// check how many samples still await processing, and scale
	// that by tempo & rate to get expected output sample count
	int32_t nUnprocessed = this->numUnprocessedSamples();
	nUnprocessed = static_cast<int32_t>(nUnprocessed / (tempo * rate) + 0.5);

	int32_t nOut = this->numSamples(); // ready samples currently in buffer ...
	nOut += nUnprocessed; // ... and how many we expect there to be in the end

	// "Push" the last active samples out from the processing pipeline by
	// feeding blank samples into the processing pipeline until new,
	// processed samples appear in the output (not however, more than
	// 8ksamples in any case)
	SAMPLETYPE buff[128] = { 0 };
	for (int i = 0; i < 128; ++i)
	{
		this->putSamples(buff, 64);
		if (static_cast<int32_t>(this->numSamples()) >= nOut)
		{
			// Enough new samples have appeared into the output!
			// As samples come from processing with bigger chunks, now truncate it
			// back to maximum "nOut" samples to improve duration accuracy
			this->adjustAmountOfSamples(nOut);

			// finish
			break;
		}
	}

	// Clear working buffers
	this->pRateTransposer->clear();
	this->pTDStretch->clearInput();
	// yet leave the 'tempoChanger' output intouched as that's where the
	// flushed samples are!
}

// Changes a setting controlling the processing system behaviour. See the
// 'SETTING_...' defines for available setting ID's.
bool SoundTouch::setSetting(int32_t settingId, int32_t value)
{
	int32_t sampleRate, sequenceMs, seekWindowMs, overlapMs;

	// read current tdstretch routine parameters
	pTDStretch->getParameters(&sampleRate, &sequenceMs, &seekWindowMs, &overlapMs);

	switch (settingId)
	{
		case SETTING_USE_AA_FILTER:
			// enables / disabless anti-alias filter
			this->pRateTransposer->enableAAFilter(!!value);
			return true;

		case SETTING_AA_FILTER_LENGTH:
			// sets anti-alias filter length
			this->pRateTransposer->getAAFilter()->setLength(value);
			return true;

		case SETTING_USE_QUICKSEEK:
			// enables / disables tempo routine quick seeking algorithm
			this->pTDStretch->enableQuickSeek(!!value);
			return true;

		case SETTING_SEQUENCE_MS:
			// change time-stretch sequence duration parameter
			this->pTDStretch->setParameters(sampleRate, value, seekWindowMs, overlapMs);
			return true;

		case SETTING_SEEKWINDOW_MS:
			// change time-stretch seek window length parameter
			this->pTDStretch->setParameters(sampleRate, sequenceMs, value, overlapMs);
			return true;

		case SETTING_OVERLAP_MS:
			// change time-stretch overlap length parameter
			this->pTDStretch->setParameters(sampleRate, sequenceMs, seekWindowMs, value);
			return true;

		default:
			return false;
	}
}

// Clears all the samples in the object's output and internal processing
// buffers.
void SoundTouch::clear()
{
	this->pRateTransposer->clear();
	this->pTDStretch->clear();
}

/// Returns number of samples currently unprocessed.
uint32_t SoundTouch::numUnprocessedSamples() const
{
	if (this->pTDStretch)
	{
		FIFOSamplePipe *psp = this->pTDStretch->getInput();
		if (psp)
			return psp->numSamples();
	}
	return 0;
}
