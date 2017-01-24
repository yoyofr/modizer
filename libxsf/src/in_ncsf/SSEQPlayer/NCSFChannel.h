/*
 * SSEQ Player - NCSFChannel structures
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-10-27
 *
 * Adapted from source code of FeOS Sound System
 * By fincs
 * https://github.com/fincs/FSS
 *
 * Some code/concepts from DeSmuME
 * http://desmume.org/
 */

#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include "SWAV.h"
#include "Track.h"

/*
 * This structure is meant to be similar to what is stored in the actual
 * Nintendo DS's sound registers.  Items that were not being used by this
 * player have been removed, and items which help the simulated registers
 * have been added.
 */
struct NDSSoundRegister
{
	// Control Register
	uint8_t volumeMul;
	uint8_t volumeDiv;
	uint8_t panning;
	uint8_t waveDuty;
	uint8_t repeatMode;
	uint8_t format;
	bool enable;

	// Data Source Register
	const SWAV *source;

	// Timer Register
	uint16_t timer;

	// PSG Handling, not a DS register
	uint16_t psgX;
	int16_t psgLast;
	uint32_t psgLastCount;

	// The following are taken from DeSmuME
	double samplePosition;
	double sampleIncrease;

	// Loopstart Register
	uint32_t loopStart;

	// Length Register
	uint32_t length;

	uint32_t totalLength;

	NDSSoundRegister();

	void ClearControlRegister();
	void SetControlRegister(uint32_t reg);
};

/*
 * From FeOS Sound System, this is temporary storage of what will go into
 * the Nintendo DS sound registers.  It is kept separate as the original code
 * from FeOS Sound System utilized this to hold data prior to passing it into
 * the DS's registers.
 */
struct TempSndReg
{
	uint32_t CR;
	const SWAV *SOURCE;
	uint16_t TIMER;
	uint32_t REPEAT_POINT, LENGTH;

	TempSndReg();
};

struct Player;

/*
 * This creates a ring buffer, which will store N samples of SWAV
 * data, duplicated. The way it is duplicated is done as follows:
 * the samples are stored in the center of the buffer, and on both
 * sides is half of the data, the first half being after the data
 * and the second half before before the data. This in essense
 * mirrors the data while allowing a pointer to always be retrieved
 * and no extra copies of the buffer are created. Part of the idea
 * for this came from kode54's original buffer implementation, but
 * this has been designed to make sure that there are no delays in
 * accessing the SWAVs samples and also doesn't use 0s before the
 * start of the SWAV or use 0s after the end of a non-looping SWAV.
 */
template<size_t N> struct RingBuffer
{
	int16_t buffer[N * 2];
	size_t bufferPos, getPos;

	RingBuffer() : bufferPos(N / 2), getPos(N / 2)
	{
		std::fill_n(&this->buffer[0], N * 2, 0);
	}
	void Clear()
	{
		std::fill_n(&this->buffer[0], N * 2, 0);
		this->bufferPos = this->getPos = N / 2;
	}
	void PushSample(int16_t sample)
	{
		this->buffer[this->bufferPos] = sample;
		if (this->bufferPos >= N)
			this->buffer[this->bufferPos - N] = sample;
		else
			this->buffer[this->bufferPos + N] = sample;
		++this->bufferPos;
		if (this->bufferPos >= N * 3 / 2)
			this->bufferPos -= N;
	}
	void PushSamples(const int16_t *samples, size_t size)
	{
		if (this->bufferPos + size > N * 3 / 2)
		{
			size_t free = N * 3 / 2 - this->bufferPos;
			std::copy_n(&samples[0], free, &this->buffer[this->bufferPos]);
			std::copy(&samples[free], &samples[size], &this->buffer[N / 2]);
		}
		else
			std::copy_n(&samples[0], size, &this->buffer[this->bufferPos]);
		size_t rightFree = this->bufferPos < N ? N - this->bufferPos : 0;
		if (rightFree < size)
		{
			if (!rightFree)
			{
				size_t leftStart = this->bufferPos - N;
				size_t leftSize = std::min(N / 2 - leftStart, size);
				std::copy_n(&samples[0], leftSize, &this->buffer[leftStart]);
				if (leftSize < size)
					std::copy(&samples[leftSize], &samples[size], &this->buffer[N * 3 / 2]);
			}
			else
			{
				std::copy_n(&samples[0], rightFree, &this->buffer[this->bufferPos + N]);
				std::copy(&samples[rightFree], &samples[size], &this->buffer[0]);
			}
		}
		else
			std::copy_n(&samples[0], size, &this->buffer[this->bufferPos + N]);
		this->bufferPos += size;
		if (this->bufferPos >= N * 3 / 2)
			this->bufferPos -= N;
	}
	const int16_t *GetBuffer() const
	{
		return &this->buffer[this->getPos];
	}
	void NextSample()
	{
		++this->getPos;
		if (this->getPos >= N * 3 / 2)
			this->getPos -= N;
	}
};

struct NCSFChannel
{
	int8_t chnId;

	TempSndReg tempReg;
	uint8_t state;
	int8_t trackId; // -1 = none
	uint8_t prio;
	bool manualSweep;

	std::bitset<CF_BITS> flags;
	int8_t pan; // -64 .. 63
	int16_t extAmpl;

	int16_t velocity;
	int8_t extPan;
	uint8_t key;

	int ampl; // 7 fractionary bits
	int extTune; // in 64ths of a semitone

	uint8_t orgKey;

	uint8_t modType, modSpeed, modDepth, modRange;
	uint16_t modDelay, modDelayCnt, modCounter;

	uint32_t sweepLen, sweepCnt;
	int16_t sweepPitch;

	uint8_t attackLvl, sustainLvl;
	uint16_t decayRate, releaseRate;

	/*
	 * These were originally global variables in FeOS Sound System, but
	 * since they were linked to a certain channel anyways, I moved them
	 * into this class.
	 */
	int noteLength;
	uint16_t vol;

	const Player *ply;
	NDSSoundRegister reg;

	/*
	 * Lookup tables for the Sinc interpolation, to
	 * avoid the need to call the sin/cos functions all the time.
	 * These are static as they will not change between channels or runs
	 * of the program.
	 */
	static bool initializedLUTs;
	static const unsigned SINC_RESOLUTION = 8192;
	static const unsigned SINC_WIDTH = 8;
	static const unsigned SINC_SAMPLES = SINC_RESOLUTION * SINC_WIDTH;
	static double sinc_lut[SINC_SAMPLES + 1];
	static double window_lut[SINC_SAMPLES + 1];

	RingBuffer<SINC_WIDTH * 2> ringBuffer;

	NCSFChannel();

	void UpdateVol(const Track &trk);
	void UpdatePan(const Track &trk);
	void UpdateTune(const Track &trk);
	void UpdateMod(const Track &trk);
	void UpdatePorta(const Track &trk);
	void Release();
	void Kill();
	void UpdateTrack();
	void Update();
	int32_t Interpolate();
	int32_t GenerateSample();
	void IncrementSample();
};
