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

#include <cassert>
#include "../types.h"
#include "SndOut.h"

//----------------
static const int SndOutLatencyMS = 160;
static bool timeStretchDisabled = false;
//----------------

StereoOut32 StereoOut32::Empty;

StereoOut32::StereoOut32(const StereoOut16 &src) : Left(src.Left), Right(src.Right)
{
}

StereoOut32::StereoOut32(const StereoOutFloat &src) : Left(static_cast<int32_t>(src.Left * 2147483647.0f)), Right(static_cast<int32_t>(src.Right * 2147483647.0f))
{
}

StereoOut16 StereoOut32::DownSample() const
{
	return StereoOut16((Left >> SndOutVolumeShift) & 0xFFFF, (Right >> SndOutVolumeShift) & 0xFFFF);
}

std::unique_ptr<StereoOut32[]> SndBuffer::m_buffer;
int32_t SndBuffer::m_size;
int32_t SndBuffer::m_rpos;
int32_t SndBuffer::m_wpos;
int32_t SndBuffer::m_data;

bool SndBuffer::m_underrun_freeze;
std::unique_ptr<StereoOut32[]> SndBuffer::sndTempBuffer;
int SndBuffer::sndTempProgress = 0;

inline int GetAlignedBufferSize(int comp)
{
	return (comp + SndOutPacketSize - 1) & ~(SndOutPacketSize - 1);
}

// Returns true if there is data to be output, or false if no data
// is available to be copied.
bool SndBuffer::CheckUnderrunStatus(int &nSamples, int &quietSampleCount)
{
	quietSampleCount = 0;
	if (m_underrun_freeze)
	{
		int toFill = static_cast<int>(m_size * (timeStretchDisabled ? 0.50f : 0.1f));
		toFill = GetAlignedBufferSize(toFill);

		// toFill is now aligned to a SndOutPacket

		if (m_data < toFill)
		{
			quietSampleCount = nSamples;
			return false;
		}

		m_underrun_freeze = false;
		//TODO
		//if( MsgOverruns() )
			printf(" * SPU2 > Underrun compensation (%d packets buffered)\n", toFill / SndOutPacketSize);
		lastPct = 0.0; // normalize timestretcher
	}
	else if (m_data < nSamples)
	{
		nSamples = m_data;
		quietSampleCount = SndOutPacketSize - m_data;
		m_underrun_freeze = true;

		if (!timeStretchDisabled)
			timeStretchUnderrun();

		return !!nSamples;
	}

	return true;
}

void SndBuffer::_InitFail()
{
	// If a failure occurs, just initialize the NoSound driver.  This'll allow
	// the game to emulate properly (hopefully), albeit without sound.
	//OutputModule = FindOutputModuleById( NullOut.GetIdent() );
	//mods[OutputModule]->Init();
}

void SndBuffer::_WriteSamples(StereoOut32 *bData, int nSamples)
{
	int free = m_size - m_data;
	m_predictData = 0;

	assert(m_data <= m_size);

	// Problem:
	//  If the SPU2 gets out of sync with the SndOut device, the writepos of the
	//  circular buffer will overtake the readpos, leading to a prolonged period
	//  of hopscotching read/write accesses (ie, lots of staticy crap sound for
	//  several seconds).
	//
	// Compromise:
	//  When an overrun occurs, we adapt by discarding a portion of the buffer.
	//  The older portion of the buffer is discarded rather than incoming data,
	//  so that the overall audio synchronization is better.

	if (free < nSamples)
	{
		// Buffer overrun!
		// Dump samples from the read portion of the buffer instead of dropping
		// the newly written stuff.

		int32_t comp;

		if (!timeStretchDisabled)
			comp = timeStretchOverrun();
		else
		{
			// Toss half the buffer plus whatever's being written anew:
			comp = GetAlignedBufferSize((m_size + nSamples) / 2);
			if (comp > m_size - SndOutPacketSize)
				comp = m_size - SndOutPacketSize;
		}

		m_data -= comp;
		m_rpos = (m_rpos + comp) % m_size;
		//TODO
		//if( MsgOverruns() )
			printf(" * SPU2 > Overrun Compensation (%d packets tossed)\n", comp / SndOutPacketSize);
		lastPct = 0.0; // normalize the timestretcher
	}

	// copy in two phases, since there's a chance the packet
	// wraps around the buffer (it'd be nice to deal in packets only, but
	// the timestretcher and DSP options require flexibility).

	int endPos = m_wpos + nSamples;
	int secondCopyLen = endPos - m_size;
	StereoOut32 *wposbuffer = &m_buffer[m_wpos];

	m_data += nSamples;
	if (secondCopyLen > 0)
	{
		nSamples -= secondCopyLen;
		memcpy(m_buffer.get(), &bData[nSamples], secondCopyLen * sizeof(*bData));
		m_wpos = secondCopyLen;
	}
	else
		m_wpos += nSamples;

	memcpy(wposbuffer, bData, nSamples * sizeof(*bData));
}

void SndBuffer::Init()
{
	// initialize sound buffer
	// Buffer actually attempts to run ~50%, so allocate near double what
	// the requested latency is:

	m_rpos = m_wpos = m_data = 0;

	try
	{
		float latencyMS = SndOutLatencyMS * (timeStretchDisabled ? 1.5f : 2.0f);
		m_size = GetAlignedBufferSize(static_cast<int>(latencyMS * SampleRate / 1000.0f));
		m_buffer.reset(new StereoOut32[m_size]);
		m_underrun_freeze = false;

		sndTempBuffer.reset(new StereoOut32[SndOutPacketSize]);
	}
	catch(const std::bad_alloc &)
	{
		// out of memory exception (most likely)

		printf("Out of memory error occurred while initializing SPU2.");
		_InitFail();
		return;
	}

	// clear buffers!
	// Fixes loopy sounds on emu resets.
	memset(sndTempBuffer.get(), 0, sizeof(StereoOut32) * SndOutPacketSize);

	sndTempProgress = 0;

	soundtouchInit(); // initializes the timestretching

	// some crap
	//spdif_set51(mods[OutputModule]->Is51Out());

	// initialize module
	//if( mods[OutputModule]->Init() == -1 ) _InitFail();
}

void SndBuffer::Write(const StereoOut32 &Sample)
{
	//if(mods[OutputModule] == &NullOut) // null output doesn't need buffering or stretching! :p
	//	return;

	sndTempBuffer[sndTempProgress++] = Sample;

	// If we haven't accumulated a full packet yet, do nothing more:
	if (sndTempProgress < SndOutPacketSize)
		return;
	sndTempProgress = 0;

	if( !timeStretchDisabled )
		timeStretchWrite();
	else
		_WriteSamples(sndTempBuffer.get(), SndOutPacketSize);
}
