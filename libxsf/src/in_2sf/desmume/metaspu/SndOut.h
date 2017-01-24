/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <algorithm>
#include "../types.h"

struct StereoOut16;
struct StereoOutFloat;

struct StereoOut32
{
	static StereoOut32 Empty;

	int32_t Left, Right;

	StereoOut32() : Left(0), Right(0)
	{
	}

	StereoOut32(int32_t left, int32_t right) : Left(left), Right(right)
	{
	}

	StereoOut32(const StereoOut16 &src);
	explicit StereoOut32(const StereoOutFloat &src);

	StereoOut16 DownSample() const;

	StereoOut32 operator+(const StereoOut32 &right) const
	{
		return StereoOut32(this->Left + right.Left, this->Right + right.Right);
	}

	StereoOut32 operator/(int src) const
	{
		return StereoOut32(this->Left / src, this->Right / src);
	}
};

// Number of stereo samples per SndOut block.
// All drivers must work in units of this size when communicating with
// SndOut.
const int SndOutPacketSize = 512;

// edit - zeromus 23-oct-2009
// this is hardcoded differently for metaspu
const int SndOutVolumeShift = 0;

// Samplerate of the SPU2. For accurate playback we need to match this
// exactly.  Trying to scale samplerates and maintain SPU2's Ts timing accuracy
// is too problematic. :)
// this is hardcoded differently for metaspu
// edit - zeromus 23-oct-2009
//const int SampleRate = 48000;
//const int SampleRate = 44100;
// edit - nitsuja: make it use the global sample rate define
#include "../SPU.h"
const int SampleRate = DESMUME_SAMPLE_RATE;

struct StereoOut16
{
	int16_t Left, Right;

	StereoOut16() : Left(0), Right(0)
	{
	}

	StereoOut16(const StereoOut32 &src) : Left(src.Left & 0xFFFF), Right(src.Right & 0xFFFF)
	{
	}

	StereoOut16(int16_t left, int16_t right) : Left(left), Right(right)
	{
	}

	void ResampleFrom(const StereoOut32 &src)
	{
		// Use StereoOut32's built in conversion
		*this = src.DownSample();
	}
};

struct StereoOutFloat
{
	float Left, Right;

	StereoOutFloat() : Left(0.0f), Right(0.0f)
	{
	}

	explicit StereoOutFloat(const StereoOut32 &src) : Left(src.Left / 2147483647.0f), Right(src.Right / 2147483647.0f)
	{
	}

	explicit StereoOutFloat(int32_t left, int32_t right) : Left(left / 2147483647.0f), Right(right / 2147483647.0f)
	{
	}

	StereoOutFloat(float left, float right) : Left(left), Right(right)
	{
	}
};

// Developer Note: This is a static class only (all static members).
class SndBuffer
{
private:
	static bool m_underrun_freeze;
	static int32_t m_predictData;
	static float lastPct;

	static std::unique_ptr<StereoOut32[]> sndTempBuffer;

	static int sndTempProgress;

	static std::unique_ptr<StereoOut32[]> m_buffer;
	static int32_t m_size;
	static int32_t m_rpos;
	static int32_t m_wpos;
	static int32_t m_data;

	static float lastEmergencyAdj;
	static float cTempo;
	static float eTempo;
	static int freezeTempo;

	static void _InitFail();
	static void _WriteSamples(StereoOut32 *bData, int nSamples);
	static bool CheckUnderrunStatus(int &nSamples, int &quietSampleCount);

	static void soundtouchInit();
	static void timeStretchWrite();
	static void timeStretchUnderrun();
	static int32_t timeStretchOverrun();

	static void PredictDataWrite(int samples);
	static float GetStatusPct();
	static void UpdateTempoChange();

public:
	static void Init();
	static void Write(const StereoOut32 &Sample);

	// Note: When using with 32 bit output buffers, the user of this function is responsible
	// for shifting the values to where they need to be manually.  The fixed point depth of
	// the sample output is determined by the SndOutVolumeShift, which is the number of bits
	// to shift right to get a 16 bit result.
	template<typename T> static void ReadSamples(T *bData)
	{
		int nSamples = SndOutPacketSize;

		// Problem:
		//  If the SPU2 gets even the least bit out of sync with the SndOut device,
		//  the readpos of the circular buffer will overtake the writepos,
		//  leading to a prolonged period of hopscotching read/write accesses (ie,
		//  lots of staticy crap sound for several seconds).
		//
		// Fix:
		//  If the read position overtakes the write position, abort the
		//  transfer immediately and force the SndOut driver to wait until
		//  the read buffer has filled up again before proceeding.
		//  This will cause one brief hiccup that can never exceed the user's
		//  set buffer length in duration.

		int quietSamples;
		if (CheckUnderrunStatus(nSamples, quietSamples))
		{
			assert(nSamples <= SndOutPacketSize);

			// [Air] [TODO]: This loop is probably a candidate for SSE2 optimization.

			int endPos = m_rpos + nSamples;
			int secondCopyLen = endPos - m_size;
			const StereoOut32 *rposbuffer = &m_buffer[m_rpos];

			m_data -= nSamples;

			if (secondCopyLen > 0)
			{
				nSamples -= secondCopyLen;
				for (int i = 0; i < secondCopyLen; ++i)
					bData[nSamples + i].ResampleFrom(m_buffer[i]);
				m_rpos = secondCopyLen;
			}
			else
				m_rpos += nSamples;

			for (int i = 0; i < nSamples; ++i)
				bData[i].ResampleFrom(rposbuffer[i]);
		}

		// If quietSamples != 0 it means we have an underrun...
		// Let's just dull out some silence, because that's usually the least
		// painful way of dealing with underruns:
		memset(bData, 0, quietSamples * sizeof(T));
	}
};
