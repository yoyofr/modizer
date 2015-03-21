/*
 * XMTools.cpp
 * -----------
 * Purpose: Definition of XM file structures and helper functions
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "XMTools.h"
#include "Sndfile.h"
#include <algorithm>


OPENMPT_NAMESPACE_BEGIN


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XMFileHeader::ConvertEndianness()
//------------------------------------
{
	SwapBytesLE(version);
	SwapBytesLE(size);
	SwapBytesLE(orders);
	SwapBytesLE(restartPos);
	SwapBytesLE(channels);
	SwapBytesLE(patterns);
	SwapBytesLE(instruments);
	SwapBytesLE(flags);
	SwapBytesLE(speed);
	SwapBytesLE(tempo);
}


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XMInstrument::ConvertEndianness()
//------------------------------------
{
	for(size_t i = 0; i < CountOf(volEnv); i++)
	{
		SwapBytesLE(volEnv[i]);
		SwapBytesLE(panEnv[i]);
	}
	SwapBytesLE(volFade);
	SwapBytesLE(midiProgram);
	SwapBytesLE(pitchWheelRange);
}



// Convert OpenMPT's internal envelope representation to XM envelope data.
void XMInstrument::ConvertEnvelopeToXM(const InstrumentEnvelope &mptEnv, uint8 &numPoints, uint8 &flags, uint8 &sustain, uint8 &loopStart, uint8 &loopEnd, EnvType env)
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	numPoints = static_cast<uint8>(std::min(12u, mptEnv.nNodes));

	// Envelope Data
	for(size_t i = 0; i < numPoints; i++)
	{
		switch(env)
		{
		case EnvTypeVol:
			volEnv[i * 2] = std::min(mptEnv.Ticks[i], uint16_max);
			volEnv[i * 2 + 1] = std::min(mptEnv.Values[i], uint8(64));
			break;
		case EnvTypePan:
			panEnv[i * 2] = std::min(mptEnv.Ticks[i], uint16_max);
			panEnv[i * 2 + 1] = std::min(mptEnv.Values[i], uint8(63));
			break;
		}
	}

	// Envelope Flags
	if(mptEnv.dwFlags[ENV_ENABLED]) flags |= XMInstrument::envEnabled;
	if(mptEnv.dwFlags[ENV_SUSTAIN]) flags |= XMInstrument::envSustain;
	if(mptEnv.dwFlags[ENV_LOOP]) flags |= XMInstrument::envLoop;

	// Envelope Loops
	sustain = std::min(uint8(12), mptEnv.nSustainStart);
	loopStart = std::min(uint8(12), mptEnv.nLoopStart);
	loopEnd = std::min(uint8(12), mptEnv.nLoopEnd);

}


// Convert OpenMPT's internal sample representation to an XMInstrument.
uint16 XMInstrument::ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport)
//-------------------------------------------------------------------------------------
{
	MemsetZero(*this);

	// FFF is maximum in the FT2 GUI, but it can also accept other values. MilkyTracker just allows 0...4095 and 32767 ("cut")
	volFade = static_cast<uint16>(std::min(mptIns.nFadeOut, uint32(32767)));

	// Convert envelopes
	ConvertEnvelopeToXM(mptIns.VolEnv, volPoints, volFlags, volSustain, volLoopStart, volLoopEnd, EnvTypeVol);
	ConvertEnvelopeToXM(mptIns.PanEnv, panPoints, panFlags, panSustain, panLoopStart, panLoopEnd, EnvTypePan);

	// Create sample assignment table
	std::vector<SAMPLEINDEX> sampleList = GetSampleList(mptIns, compatibilityExport);
	for(size_t i = 0; i < CountOf(sampleMap); i++)
	{
		if(mptIns.Keyboard[i + 12] > 0)
		{
			std::vector<SAMPLEINDEX>::iterator sample = std::find(sampleList.begin(), sampleList.end(), mptIns.Keyboard[i + 12]);
			if(sample != sampleList.end())
			{
				// Yep, we want to export this sample.
				sampleMap[i] = static_cast<uint8>(sample - sampleList.begin());
			}
		}
	}

	if(mptIns.nMidiChannel != MidiNoChannel)
	{
		midiEnabled = 1;
		midiChannel = (mptIns.nMidiChannel != MidiMappedChannel ? (mptIns.nMidiChannel - MidiFirstChannel) : 0);
	}
	midiProgram = (mptIns.nMidiProgram != 0 ? mptIns.nMidiProgram - 1 : 0);
	pitchWheelRange = std::min(mptIns.midiPWD, int8(36));

	return static_cast<uint16>(sampleList.size());
}


// Get a list of samples that should be written to the file.
std::vector<SAMPLEINDEX> XMInstrument::GetSampleList(const ModInstrument &mptIns, bool compatibilityExport) const
//---------------------------------------------------------------------------------------------------------------
{
	std::vector<SAMPLEINDEX> sampleList;		// List of samples associated with this instrument
	std::vector<bool> addedToList;			// Which samples did we already add to the sample list?

	uint8 numSamples = 0;
	for(size_t i = 0; i < CountOf(sampleMap); i++)
	{
		const SAMPLEINDEX smp = mptIns.Keyboard[i + 12];
		if(smp > 0)
		{
			if(smp > addedToList.size())
			{
				addedToList.resize(smp, false);
			}

			if(!addedToList[smp - 1] && numSamples < (compatibilityExport ? 16 : 32))
			{
				// We haven't considered this sample yet.
				addedToList[smp - 1] = true;
				numSamples++;
				sampleList.push_back(smp);
			}
		}
	}
	return sampleList;
}


// Convert XM envelope data to an OpenMPT's internal envelope representation.
void XMInstrument::ConvertEnvelopeToMPT(InstrumentEnvelope &mptEnv, uint8 numPoints, uint8 flags, uint8 sustain, uint8 loopStart, uint8 loopEnd, EnvType env) const
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	mptEnv.nNodes = MIN(numPoints, 12);

	// Envelope Data
	for(size_t i = 0; i < 12; i++)
	{
		switch(env)
		{
		case EnvTypeVol:
			mptEnv.Ticks[i] = volEnv[i * 2];
			mptEnv.Values[i] = static_cast<uint8>(volEnv[i * 2 + 1]);
			break;
		case EnvTypePan:
			mptEnv.Ticks[i] = panEnv[i * 2];
			mptEnv.Values[i] = static_cast<uint8>(panEnv[i * 2 + 1]);
			break;
		}

		if(i > 0 && mptEnv.Ticks[i] < mptEnv.Ticks[i - 1])
		{
			// libmikmod code says: "Some broken XM editing program will only save the low byte of the position
			// value. Try to compensate by adding the missing high byte" - I guess that's what this code is for.
			// Note: It appears that MPT 1.07's XI instrument saver omitted the high byte of envelope nodes.
			// This might be the source for some broken envelopes in IT and XM files.

			mptEnv.Ticks[i] &= 0xFF;
			mptEnv.Ticks[i] += mptEnv.Ticks[i - 1] & 0xFF00;
			if(mptEnv.Ticks[i] < mptEnv.Ticks[i - 1]) mptEnv.Ticks[i] += 0x100;
		}
	}
	// Sanitize envelope
	mptEnv.Ticks[0] = 0;

	// Envelope Flags
	mptEnv.dwFlags.reset();
	if((flags & XMInstrument::envEnabled) != 0 && mptEnv.nNodes != 0) mptEnv.dwFlags.set(ENV_ENABLED);

	// Envelope Loops
	if(sustain < 12)
	{
		if((flags & XMInstrument::envSustain) != 0) mptEnv.dwFlags.set(ENV_SUSTAIN);
		mptEnv.nSustainStart = mptEnv.nSustainEnd = sustain;
	}

	if(loopEnd < 12 && loopEnd >= loopStart)
	{
		if((flags & XMInstrument::envLoop) != 0) mptEnv.dwFlags.set(ENV_LOOP);
		mptEnv.nLoopStart = loopStart;
		mptEnv.nLoopEnd = loopEnd;
	}
}


// Convert an XMInstrument to OpenMPT's internal instrument representation.
void XMInstrument::ConvertToMPT(ModInstrument &mptIns) const
//----------------------------------------------------------
{
	mptIns.nFadeOut = volFade;

	// Convert envelopes
	ConvertEnvelopeToMPT(mptIns.VolEnv, volPoints, volFlags, volSustain, volLoopStart, volLoopEnd, EnvTypeVol);
	ConvertEnvelopeToMPT(mptIns.PanEnv, panPoints, panFlags, panSustain, panLoopStart, panLoopEnd, EnvTypePan);

	// Create sample assignment table
	for(size_t i = 0; i < CountOf(sampleMap); i++)
	{
		mptIns.Keyboard[i + 12] = sampleMap[i];
	}

	if(midiEnabled)
	{
		mptIns.nMidiChannel = midiChannel + MidiFirstChannel;
		Limit(mptIns.nMidiChannel, uint8(MidiFirstChannel), uint8(MidiLastChannel));
		mptIns.nMidiProgram = static_cast<uint8>(std::min(midiProgram, uint16(127)) + 1);
	}
	mptIns.midiPWD = static_cast<int8>(pitchWheelRange);
}


// Apply auto-vibrato settings from sample to file.
void XMInstrument::ApplyAutoVibratoToXM(const ModSample &mptSmp, MODTYPE fromType)
//--------------------------------------------------------------------------------
{
	vibType = mptSmp.nVibType;
	vibSweep = mptSmp.nVibSweep;
	vibDepth = mptSmp.nVibDepth;
	vibRate = mptSmp.nVibRate;

	if((vibDepth | vibRate) != 0 && !(fromType & MOD_TYPE_XM))
	{
		// Sweep is upside down in XM
		vibSweep = 255 - vibSweep;
	}
}


// Apply auto-vibrato settings from file to a sample.
void XMInstrument::ApplyAutoVibratoToMPT(ModSample &mptSmp) const
//---------------------------------------------------------------
{
	mptSmp.nVibType = vibType;
	mptSmp.nVibSweep = vibSweep;
	mptSmp.nVibDepth = vibDepth;
	mptSmp.nVibRate = vibRate;
}


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XMInstrumentHeader::ConvertEndianness()
//------------------------------------------
{
	SwapBytesLE(size);
	SwapBytesLE(sampleHeaderSize);
	SwapBytesLE(numSamples);
	instrument.ConvertEndianness();
}


// Write stuff to the header that's always necessary (also for empty instruments)
void XMInstrumentHeader::Finalise()
//---------------------------------
{
	size = sizeof(XMInstrumentHeader);
	if(numSamples > 0)
	{
		sampleHeaderSize = sizeof(XMSample);
	} else
	{
		// TODO: FT2 completely ignores MIDI settings (and also the less important stuff) if not at least one (empty) sample is assigned to this instrument!
		size -= sizeof(XMInstrument);
		sampleHeaderSize = 0;
	}
}


// Convert OpenMPT's internal sample representation to an XMInstrumentHeader.
void XMInstrumentHeader::ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport)
//-----------------------------------------------------------------------------------------
{
	numSamples = instrument.ConvertToXM(mptIns, compatibilityExport);
	mpt::String::Write<mpt::String::spacePadded>(name, mptIns.name);

	type = mptIns.nMidiProgram;	// If FT2 writes crap here, we can do so, too! (we probably shouldn't, though. This is just for backwards compatibility with old MPT versions.)
}


// Convert an XMInstrumentHeader to OpenMPT's internal instrument representation.
void XMInstrumentHeader::ConvertToMPT(ModInstrument &mptIns) const
//----------------------------------------------------------------
{
	instrument.ConvertToMPT(mptIns);

	// Create sample assignment table
	for(size_t i = 0; i < CountOf(instrument.sampleMap); i++)
	{
		if(instrument.sampleMap[i] < numSamples)
		{
			mptIns.Keyboard[i + 12] = instrument.sampleMap[i];
		} else
		{
			mptIns.Keyboard[i + 12] = 0;
		}
	}

	mpt::String::Read<mpt::String::spacePadded>(mptIns.name, name);

	// Old MPT backwards compatibility
	if(!instrument.midiEnabled)
	{
		mptIns.nMidiProgram = type;
	}
}


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XIInstrumentHeader::ConvertEndianness()
//------------------------------------------
{
	SwapBytesLE(version);
	SwapBytesLE(numSamples);
	instrument.ConvertEndianness();
}


// Convert OpenMPT's internal sample representation to an XIInstrumentHeader.
void XIInstrumentHeader::ConvertToXM(const ModInstrument &mptIns, bool compatibilityExport)
//-----------------------------------------------------------------------------------------
{
	numSamples = instrument.ConvertToXM(mptIns, compatibilityExport);

	memcpy(signature, "Extended Instrument: ", 21);
	mpt::String::Write<mpt::String::spacePadded>(name, mptIns.name);
	eof = 0x1A;

	memcpy(trackerName, "Created by OpenMPT  ", 20);

	version = 0x102;
}


// Convert an XIInstrumentHeader to OpenMPT's internal instrument representation.
void XIInstrumentHeader::ConvertToMPT(ModInstrument &mptIns) const
//----------------------------------------------------------------
{
	instrument.ConvertToMPT(mptIns);

	// Fix sample assignment table
	for(size_t i = 12; i < CountOf(instrument.sampleMap) + 12; i++)
	{
		if(mptIns.Keyboard[i] >= numSamples)
		{
			mptIns.Keyboard[i] = 0;
		}
	}

	mpt::String::Read<mpt::String::spacePadded>(mptIns.name, name);
}


// Convert all multi-byte numeric values to current platform's endianness or vice versa.
void XMSample::ConvertEndianness()
//--------------------------------
{
	SwapBytesLE(length);
	SwapBytesLE(loopStart);
	SwapBytesLE(loopLength);
}


// Convert OpenMPT's internal sample representation to an XMSample.
void XMSample::ConvertToXM(const ModSample &mptSmp, MODTYPE fromType, bool compatibilityExport)
//---------------------------------------------------------------------------------------------
{
	MemsetZero(*this);

	// Volume / Panning
	vol = static_cast<uint8>(std::min(mptSmp.nVolume / 4u, 64u));
	pan = static_cast<uint8>(std::min(mptSmp.nPan, uint16(255)));

	// Sample Frequency
	if((fromType & (MOD_TYPE_MOD | MOD_TYPE_XM)))
	{
		finetune = mptSmp.nFineTune;
		relnote = mptSmp.RelativeTone;
	} else
	{
		int f2t = ModSample::FrequencyToTranspose(mptSmp.nC5Speed);
		relnote = (int8)(f2t >> 7);
		finetune = (int8)(f2t & 0x7F);
	}

	flags = 0;
	if(mptSmp.uFlags[CHN_LOOP])
	{
		flags |= mptSmp.uFlags[CHN_PINGPONGLOOP] ? XMSample::sampleBidiLoop : XMSample::sampleLoop;
	}

	// Sample Length and Loops
	length = mpt::saturate_cast<uint32>(mptSmp.nLength);
	loopStart = mpt::saturate_cast<uint32>(mptSmp.nLoopStart);
	loopLength = mpt::saturate_cast<uint32>(mptSmp.nLoopEnd - mptSmp.nLoopStart);

	if(mptSmp.uFlags[CHN_16BIT])
	{
		flags |= XMSample::sample16Bit;
		length *= 2;
		loopStart *= 2;
		loopLength *= 2;
	}

	if(mptSmp.uFlags[CHN_STEREO] && !compatibilityExport)
	{
		flags |= XMSample::sampleStereo;
		length *= 2;
		loopStart *= 2;
		loopLength *= 2;
	}
}


// Convert an XMSample to OpenMPT's internal sample representation.
void XMSample::ConvertToMPT(ModSample &mptSmp) const
//--------------------------------------------------
{
	mptSmp.Initialize(MOD_TYPE_XM);

	// Volume
	mptSmp.nVolume = vol * 4;
	LimitMax(mptSmp.nVolume, uint16(256));

	// Panning
	mptSmp.nPan = pan;
	mptSmp.uFlags = CHN_PANNING;

	// Sample Frequency
	mptSmp.nFineTune = finetune;
	mptSmp.RelativeTone = relnote;

	// Sample Length and Loops
	mptSmp.nLength = length;
	mptSmp.nLoopStart = loopStart;
	mptSmp.nLoopEnd = mptSmp.nLoopStart + loopLength;

	if((flags & XMSample::sample16Bit))
	{
		mptSmp.nLength /= 2;
		mptSmp.nLoopStart /= 2;
		mptSmp.nLoopEnd /= 2;
	}

	if((flags & XMSample::sampleStereo))
	{
		mptSmp.nLength /= 2;
		mptSmp.nLoopStart /= 2;
		mptSmp.nLoopEnd /= 2;
	}

	if((flags & (XMSample::sampleLoop | XMSample::sampleBidiLoop)) && mptSmp.nLoopStart < mptSmp.nLength && mptSmp.nLoopEnd > mptSmp.nLoopStart)
	{
		mptSmp.uFlags.set(CHN_LOOP);
		if((flags & XMSample::sampleBidiLoop))
		{
			mptSmp.uFlags.set(CHN_PINGPONGLOOP);
		}
	}

	mptSmp.SanitizeLoops();

	strcpy(mptSmp.filename, "");
}


// Retrieve the internal sample format flags for this instrument.
SampleIO XMSample::GetSampleFormat() const
//----------------------------------------
{
	if(reserved == sampleADPCM && !(flags & (XMSample::sample16Bit | XMSample::sampleStereo)))
	{
		// MODPlugin :(
		return SampleIO(SampleIO::_8bit, SampleIO::mono, SampleIO::littleEndian, SampleIO::ADPCM);
	}

	return SampleIO(
		(flags & XMSample::sample16Bit) ? SampleIO::_16bit : SampleIO::_8bit,
		(flags & XMSample::sampleStereo) ? SampleIO::stereoSplit : SampleIO::mono,
		SampleIO::littleEndian,
		SampleIO::deltaPCM);
}


OPENMPT_NAMESPACE_END
