/*
 * Load_plm.cpp
 * ------------
 * Purpose: PLM (Disorder Tracker 2) module loader
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"


OPENMPT_NAMESPACE_BEGIN

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED PLMFileHeader
{
	char   magic[4];		// "PLM\x1A"
	uint8  headerSize;		// Number of bytes in header, including magic bytes
	uint8  version;			// version code of file format (0x10)
	char   songName[48];
	uint8  numChannels;
	uint8  flags;			// unused?
	uint8  maxVol;			// Maximum volume for vol slides, normally 0x40
	uint8  amplify;			// SoundBlaster amplify, 0x40 = no amplify
	uint8  tempo;
	uint8  speed;
	uint8  panPos[32];		// 0...15
	uint8  numSamples;
	uint8  numPatterns;
	uint16 numOrders;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(numOrders);
	}
};

STATIC_ASSERT(sizeof(PLMFileHeader) == 96);


struct PACKED PLMSampleHeader
{
	enum SampleFlags
	{
		smp16Bit = 1,
	};

	char   magic[4];		// "PLS\x1A"
	uint8  headerSize;		// Number of bytes in header, including magic bytes
	uint8  version;	
	char   name[32];
	char   filename[12];
	uint8  panning;			// 0...15, 255 = no pan
	uint8  volume;			// 0...64
	uint8  flags;			// See SampleFlags
	uint16 sampleRate;
	char   unused[4];
	uint32 loopStart;
	uint32 loopEnd;
	uint32 length;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(sampleRate);
		SwapBytesLE(loopStart);
		SwapBytesLE(loopEnd);
		SwapBytesLE(length);
	}
};

STATIC_ASSERT(sizeof(PLMSampleHeader) == 71);


struct PACKED PLMPatternHeader
{
	uint32 size;
	uint8  numRows;
	uint8  numChannels;
	uint8  color;
	char   name[25];

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(size);
	}
};

STATIC_ASSERT(sizeof(PLMPatternHeader) == 32);


struct PACKED PLMOrderItem
{
	uint16 x;		// Starting position of pattern
	uint8  y;		// Number of first channel
	uint8  pattern;

	// Convert all multi-byte numeric values to current platform's endianness or vice versa.
	void ConvertEndianness()
	{
		SwapBytesLE(x);
	}
};

STATIC_ASSERT(sizeof(PLMOrderItem) == 4);

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif


bool CSoundFile::ReadPLM(FileReader &file, ModLoadingFlags loadFlags)
//-------------------------------------------------------------------
{
	file.Rewind();

	PLMFileHeader fileHeader;
	if(!file.ReadConvertEndianness(fileHeader)
		|| memcmp(fileHeader.magic, "PLM\x1A", 4)
		|| fileHeader.version != 0x10
		|| fileHeader.numChannels == 0 || fileHeader.numChannels > 32
		|| !file.Seek(fileHeader.headerSize)
		|| !file.CanRead(4 * (fileHeader.numOrders + fileHeader.numPatterns + fileHeader.numSamples)))
	{
		return false;
	} else if(loadFlags == onlyVerifyHeader)
	{
		return true;
	}

	InitializeGlobals();
	InitializeChannels();
	SetModFlag(MSF_COMPATIBLE_PLAY, true);
	m_nType = MOD_TYPE_PLM;
	madeWithTracker = "Disorder Tracker 2";
	mpt::String::Read<mpt::String::maybeNullTerminated>(songName, fileHeader.songName);
	m_nChannels = fileHeader.numChannels + 1;	// Additional channel for writing pattern breaks
	m_nSamplePreAmp = fileHeader.amplify;
	m_nDefaultTempo = fileHeader.tempo;
	m_nDefaultSpeed = fileHeader.speed;
	for(CHANNELINDEX chn = 0; chn < fileHeader.numChannels; chn++)
	{
		ChnSettings[chn].nPan = fileHeader.panPos[chn] * 0x11;
	}
	m_nSamples = fileHeader.numSamples;

	std::vector<PLMOrderItem> order(fileHeader.numOrders);
	for(uint16 i = 0; i < fileHeader.numOrders; i++)
	{
		PLMOrderItem ord;
		file.ReadConvertEndianness(ord);
		order[i] = ord;
	}

	std::vector<uint32> patternPos, samplePos;
	file.ReadVectorLE(patternPos, fileHeader.numPatterns);
	file.ReadVectorLE(samplePos, fileHeader.numSamples);

	for(SAMPLEINDEX smp = 0; smp < fileHeader.numSamples; smp++)
	{
		ModSample &sample = Samples[smp + 1];
		sample.Initialize();

		if(samplePos[smp] == 0) continue;
		file.Seek(samplePos[smp]);
		PLMSampleHeader sampleHeader;
		file.ReadConvertEndianness(sampleHeader);

		mpt::String::Read<mpt::String::maybeNullTerminated>(m_szNames[smp + 1], sampleHeader.name);
		mpt::String::Read<mpt::String::maybeNullTerminated>(sample.filename, sampleHeader.filename);
		if(sampleHeader.panning <= 15)
		{
			sample.uFlags.set(CHN_PANNING);
			sample.nPan = sampleHeader.panning * 0x11;
		}
		sample.nGlobalVol = std::min(sampleHeader.volume, uint8(64));
		sample.nC5Speed = sampleHeader.sampleRate;
		sample.nLoopStart = sampleHeader.loopStart;
		sample.nLoopEnd = sampleHeader.loopEnd;
		sample.nLength = sampleHeader.length;
		if(sampleHeader.flags & PLMSampleHeader::smp16Bit)
		{
			sample.nLoopStart /= 2;
			sample.nLoopEnd /= 2;
			sample.nLength /= 2;
			// Apparently there is a bug in DT2 which adds an extra byte before the sample data.
			sampleHeader.headerSize++;
		}
		if(sample.nLoopEnd > sample.nLoopStart) sample.uFlags.set(CHN_LOOP);
		sample.SanitizeLoops();
		
		if(loadFlags & loadSampleData)
		{
			file.Seek(samplePos[smp] + sampleHeader.headerSize);
			SampleIO(
				(sampleHeader.flags & PLMSampleHeader::smp16Bit) ? SampleIO::_16bit : SampleIO::_8bit,
				SampleIO::mono,
				SampleIO::bigEndian,
				SampleIO::unsignedPCM)
				.ReadSample(sample, file);
		}
	}

	Order.clear();
	if(!(loadFlags & loadPatternData))
	{
		return true;
	}

	// PLM is basically one huge continuous pattern, so we split it up into smaller patterns.
	const ROWINDEX rowsPerPat = 64;
	uint32 maxPos = 0;

	static const ModCommand::COMMAND effTrans[] =
	{
		CMD_NONE,
		CMD_PORTAMENTOUP,
		CMD_PORTAMENTODOWN,
		CMD_TONEPORTAMENTO,
		CMD_VOLUMESLIDE,
		CMD_TREMOLO,
		CMD_VIBRATO,
		CMD_S3MCMDEX,		// Tremolo Waveform
		CMD_S3MCMDEX,		// Vibrato Waveform
		CMD_TEMPO,
		CMD_SPEED,
		CMD_POSITIONJUMP,	// Jump to order
		CMD_POSITIONJUMP,	// Break to end of this order
		CMD_OFFSET,
		CMD_S3MCMDEX,		// GUS Panning
		CMD_RETRIG,
		CMD_S3MCMDEX,		// Note Delay
		CMD_S3MCMDEX,		// Note Cut
		CMD_S3MCMDEX,		// Pattern Delay
		CMD_FINEVIBRATO,
		CMD_VIBRATOVOL,
		CMD_TONEPORTAVOL
	};

	for(uint16 i = 0; i < fileHeader.numOrders; i++)
	{
		const PLMOrderItem &ord = order[i];
		if(ord.pattern >= fileHeader.numPatterns || ord.y > fileHeader.numChannels) continue;

		file.Seek(patternPos[ord.pattern]);
		PLMPatternHeader patHeader;
		file.ReadConvertEndianness(patHeader);
		
		ORDERINDEX curOrd = ord.x / rowsPerPat;
		ROWINDEX curRow = ord.x % rowsPerPat;
		const CHANNELINDEX numChannels = std::min<CHANNELINDEX>(patHeader.numChannels, fileHeader.numChannels - ord.y);
		const uint32 patternEnd = ord.x + patHeader.numRows;
		maxPos = std::max(maxPos, patternEnd);

		for(ROWINDEX r = 0; r < patHeader.numRows; r++, curRow++)
		{
			if(curRow >= rowsPerPat)
			{
				curRow = 0;
				curOrd++;
			}
			if(curOrd >= Order.size())
			{
				PATTERNINDEX pat = Patterns.Insert(rowsPerPat);
				Order.resize(curOrd + 1);
				Order[curOrd] = pat;
			}
			PATTERNINDEX pat = Order[curOrd];
			if(!Patterns.IsValidPat(pat)) break;

			ModCommand *m = Patterns[pat].GetpModCommand(curRow, ord.y);
			for(CHANNELINDEX c = 0; c < numChannels; c++, m++)
			{
				uint8 data[5];
				file.ReadArray(data);
				if(data[0])
					m->note = (data[0] >> 4) * 12 + (data[0] & 0x0F) + 12 + NOTE_MIN;
				else
					m->note = NOTE_NONE;
				m->instr = data[1];
				m->volcmd = VOLCMD_VOLUME;
				if(data[2] != 0xFF)
					m->vol = data[2];
				else
					m->volcmd = VOLCMD_NONE;

				if(data[3] < CountOf(effTrans))
				{
					m->command = effTrans[data[3]];
					m->param = data[4];
					// Fix some commands
					switch(data[3])
					{
					case 0x07:	// Tremolo waveform
						m->param = 0x40 | (m->param & 0x03);
						break;
					case 0x08:	// Vibrato waveform
						m->param = 0x30 | (m->param & 0x03);
						break;
					case 0x0B:	// Jump to order
						if(m->param < fileHeader.numOrders)
						{
							uint16 target = order[m->param].x;
							m->param = static_cast<ModCommand::PARAM>(target / rowsPerPat);
							ModCommand *mBreak = Patterns[pat].GetpModCommand(curRow, m_nChannels - 1);
							mBreak->command = CMD_PATTERNBREAK;
							mBreak->param = static_cast<ModCommand::PARAM>(target % rowsPerPat);
						}
						break;
					case 0x0C:	// Jump to end of order
						{
							m->param = static_cast<ModCommand::PARAM>(patternEnd / rowsPerPat);
							ModCommand *mBreak = Patterns[pat].GetpModCommand(curRow, m_nChannels - 1);
							mBreak->command = CMD_PATTERNBREAK;
							mBreak->param = static_cast<ModCommand::PARAM>(patternEnd % rowsPerPat);
						}
						break;
					case 0x0E:	// GUS Panning
						m->param = 0x80 | (m->param & 0x0F);
						break;
					case 0x10:	// Delay Note
						m->param = 0xD0 | std::min<ModCommand::PARAM>(m->param, 0x0F);
						break;
					case 0x11:	// Cut Note
						m->param = 0xC0 | std::min<ModCommand::PARAM>(m->param, 0x0F);
						break;
					case 0x12:	// Pattern Delay
						m->param = 0x60 | std::min<ModCommand::PARAM>(m->param, 0x0F);
						break;
					case 0x04:	// Volume Slide
					case 0x14:	// Vibrato + Volume Slide
					case 0x15:	// Tone Portamento + Volume Slide
						// If both nibbles of a volume slide are set, act as fine volume slide up
						if((m->param & 0xF0) && (m->param & 0x0F) && (m->param & 0xF0) != 0xF0)
						{
							m->param |= 0x0F;
						}
						break;
					}
				}
			}
			if(patHeader.numChannels > numChannels)
			{
				file.Skip(5 * (patHeader.numChannels - numChannels));
			}
		}
	}
	// Module ends with the last row of the last order item
	ROWINDEX endPatSize = maxPos % rowsPerPat;
	if(endPatSize > 0)
	{
		PATTERNINDEX endPat = Order[maxPos / rowsPerPat];
		if(Patterns.IsValidPat(endPat))
		{
			Patterns[endPat].Resize(endPatSize);
		}
	}

	return true;
}

OPENMPT_NAMESPACE_END
