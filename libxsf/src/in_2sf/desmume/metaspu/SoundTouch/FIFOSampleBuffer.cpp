////////////////////////////////////////////////////////////////////////////////
///
/// A buffer class for temporarily storaging sound samples, operates as a
/// first-in-first-out pipe.
///
/// Samples are added to the end of the sample buffer with the 'putSamples'
/// function, and are received from the beginning of the buffer by calling
/// the 'receiveSamples' function. The class automatically removes the
/// outputted samples from the buffer, as well as grows the buffer size
/// whenever necessary.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-11-08 16:53:01 -0200 (qui, 08 nov 2012) $
// File revision : $Revision: 4 $
//
// $Id: FIFOSampleBuffer.cpp 160 2012-11-08 18:53:01Z oparviai $
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
#include <cstring>
#include "FIFOSampleBuffer.h"

using namespace soundtouch;

// Constructor
FIFOSampleBuffer::FIFOSampleBuffer(int32_t numChannels)
{
	assert(numChannels > 0);
	this->sizeInBytes = 0; // reasonable initial value
	this->buffer = nullptr;
	this->bufferUnaligned.reset();
	this->samplesInBuffer = 0;
	this->bufferPos = 0;
	this->channels = numChannels;
	this->ensureCapacity(32); // allocate initial capacity
}

// Sets number of channels, 1 = mono, 2 = stereo
void FIFOSampleBuffer::setChannels(int32_t numChannels)
{
	assert(numChannels > 0);
	uint32_t usedBytes = this->channels * this->samplesInBuffer;
	this->channels = numChannels;
	this->samplesInBuffer = usedBytes / this->channels;
}

// if output location pointer 'bufferPos' isn't zero, 'rewinds' the buffer and
// zeroes this pointer by copying samples from the 'bufferPos' pointer
// location on to the beginning of the buffer.
void FIFOSampleBuffer::rewind()
{
	if (this->buffer && this->bufferPos)
	{
		memmove(this->buffer, this->ptrBegin(), sizeof(SAMPLETYPE) * this->channels * this->samplesInBuffer);
		this->bufferPos = 0;
	}
}

// Adds 'numSamples' pcs of samples from the 'samples' memory position to
// the sample buffer.
void FIFOSampleBuffer::putSamples(const SAMPLETYPE *samples, uint32_t nSamples)
{
	memcpy(this->ptrEnd(nSamples), samples, sizeof(SAMPLETYPE) * nSamples * this->channels);
	this->samplesInBuffer += nSamples;
}

// Increases the number of samples in the buffer without copying any actual
// samples.
//
// This function is used to update the number of samples in the sample buffer
// when accessing the buffer directly with 'ptrEnd' function. Please be
// careful though!
void FIFOSampleBuffer::putSamples(uint32_t nSamples)
{
	uint32_t req = this->samplesInBuffer + nSamples;
	this->ensureCapacity(req);
	this->samplesInBuffer += nSamples;
}

// Returns a pointer to the end of the used part of the sample buffer (i.e.
// where the new samples are to be inserted). This function may be used for
// inserting new samples into the sample buffer directly. Please be careful!
//
// Parameter 'slackCapacity' tells the function how much free capacity (in
// terms of samples) there _at least_ should be, in order to the caller to
// succesfully insert all the required samples to the buffer. When necessary,
// the function grows the buffer size to comply with this requirement.
//
// When using this function as means for inserting new samples, also remember
// to increase the sample count afterwards, by calling  the
// 'putSamples(numSamples)' function.
SAMPLETYPE *FIFOSampleBuffer::ptrEnd(uint32_t slackCapacity)
{
	this->ensureCapacity(this->samplesInBuffer + slackCapacity);
	return &this->buffer[this->samplesInBuffer * this->channels];
}

// Returns a pointer to the beginning of the currently non-outputted samples.
// This function is provided for accessing the output samples directly.
// Please be careful!
//
// When using this function to output samples, also remember to 'remove' the
// outputted samples from the buffer by calling the
// 'receiveSamples(numSamples)' function
SAMPLETYPE *FIFOSampleBuffer::ptrBegin()
{
	assert(this->buffer);
	return &this->buffer[this->bufferPos * this->channels];
}

// Ensures that the buffer has enought capacity, i.e. space for _at least_
// 'capacityRequirement' number of samples. The buffer is grown in steps of
// 4 kilobytes to eliminate the need for frequently growing up the buffer,
// as well as to round the buffer size up to the virtual memory page size.
void FIFOSampleBuffer::ensureCapacity(uint32_t capacityRequirement)
{
	if (capacityRequirement > this->getCapacity())
	{
		// enlarge the buffer in 4kbyte steps (round up to next 4k boundary)
		this->sizeInBytes = (capacityRequirement * this->channels * sizeof(SAMPLETYPE) + 4095) & static_cast<uint32_t>(-4096);
		assert(!(this->sizeInBytes % 2));
		auto tempUnaligned = std::unique_ptr<SAMPLETYPE[]>(new SAMPLETYPE[(this->sizeInBytes + 16) / sizeof(SAMPLETYPE)]);
		// Align the buffer to begin at 16byte cache line boundary for optimal performance
		SAMPLETYPE *temp = reinterpret_cast<SAMPLETYPE *>(SOUNDTOUCH_ALIGN_POINTER_16(tempUnaligned.get()));
		if (samplesInBuffer)
			memcpy(temp, this->ptrBegin(), samplesInBuffer * this->channels * sizeof(SAMPLETYPE));
		this->buffer = temp;
		this->bufferUnaligned = std::move(tempUnaligned);
		this->bufferPos = 0;
	}
	else
		// simply rewind the buffer (if necessary)
		this->rewind();
}

// Returns the current buffer capacity in terms of samples
uint32_t FIFOSampleBuffer::getCapacity() const
{
	return this->sizeInBytes / (this->channels * sizeof(SAMPLETYPE));
}

// Returns the number of samples currently in the buffer
uint32_t FIFOSampleBuffer::numSamples() const
{
	return this->samplesInBuffer;
}

// Output samples from beginning of the sample buffer. Copies demanded number
// of samples to output and removes them from the sample buffer. If there
// are less than 'numsample' samples in the buffer, returns all available.
//
// Returns number of samples copied.
uint32_t FIFOSampleBuffer::receiveSamples(SAMPLETYPE *output, uint32_t maxSamples)
{
	uint32_t num = maxSamples > this->samplesInBuffer ? this->samplesInBuffer : maxSamples;

	memcpy(output, this->ptrBegin(), this->channels * sizeof(SAMPLETYPE) * num);
	return this->receiveSamples(num);
}

// Removes samples from the beginning of the sample buffer without copying them
// anywhere. Used to reduce the number of samples in the buffer, when accessing
// the sample buffer with the 'ptrBegin' function.
uint32_t FIFOSampleBuffer::receiveSamples(uint32_t maxSamples)
{
	if (maxSamples >= this->samplesInBuffer)
	{
		uint32_t temp = this->samplesInBuffer;
		this->samplesInBuffer = 0;
		return temp;
	}

	this->samplesInBuffer -= maxSamples;
	this->bufferPos += maxSamples;

	return maxSamples;
}

// Returns nonzero if the sample buffer is empty
bool FIFOSampleBuffer::isEmpty() const
{
	return !this->samplesInBuffer;
}

// Clears the sample buffer
void FIFOSampleBuffer::clear()
{
	this->samplesInBuffer = 0;
	this->bufferPos = 0;
}

/// allow trimming (downwards) amount of samples in pipeline.
/// Returns adjusted amount of samples
uint32_t FIFOSampleBuffer::adjustAmountOfSamples(uint32_t numSmpls)
{
	if (numSmpls < this->samplesInBuffer)
		this->samplesInBuffer = numSmpls;
	return this->samplesInBuffer;
}
