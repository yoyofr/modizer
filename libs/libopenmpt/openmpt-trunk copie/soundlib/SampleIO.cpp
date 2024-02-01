/*
 * SampleIO.cpp
 * ------------
 * Purpose: Central code for reading and writing samples. Create your SampleIO object and have a go at the ReadSample and WriteSample functions!
 * Notes  : Not all combinations of possible sample format combinations are implemented, especially for WriteSample.
 *          Using the existing generic functions, it should be quite easy to extend the code, though.
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Loaders.h"
#include "SampleIO.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "ModSampleCopy.h"
#include "ITCompression.h"
#include "../common/mptIO.h"
#ifndef MODPLUG_NO_FILESAVE
#include "../common/mptFileIO.h"
#endif
#include "BitReader.h"


OPENMPT_NAMESPACE_BEGIN

// Sample decompression routines in other source files
void AMSUnpack(const int8 * const source, size_t sourceSize, void * const dest, const size_t destSize, char packCharacter);
uintptr_t DMFUnpack(FileReader &file, uint8 *psample, uint32 maxlen);


// Read a sample from memory
size_t SampleIO::ReadSample(ModSample &sample, FileReader &file) const
{
	if(!file.IsValid())
	{
		return 0;
	}

	LimitMax(sample.nLength, MAX_SAMPLE_LENGTH);

	FileReader::off_t bytesRead = 0;	// Amount of memory that has been read from file

	FileReader::off_t filePosition = file.GetPosition();
	const std::byte * sourceBuf = nullptr;
	FileReader::PinnedRawDataView restrictedSampleDataView;
	FileReader::off_t fileSize = 0;
	if(UsesFileReaderForDecoding())
	{
		sourceBuf = nullptr;
		fileSize = file.BytesLeft();
	} else if(!IsVariableLengthEncoded())
	{
		restrictedSampleDataView = file.GetPinnedRawDataView(CalculateEncodedSize(sample.nLength));
		sourceBuf = restrictedSampleDataView.data();
		fileSize = restrictedSampleDataView.size();
	} else
	{
		MPT_ASSERT_NOTREACHED();
	}

	if(!IsVariableLengthEncoded() && sample.nLength > 0x40000)
	{
		// Limit sample length to available bytes in file to avoid excessive memory allocation.
		// However, for ProTracker MODs we need to support samples exceeding the end of file
		// (see the comment about MOD.shorttune2 in Load_mod.cpp), so as a semi-arbitrary threshold,
		// we do not apply this limit to samples shorter than 256K.
		size_t maxLength = fileSize - std::min(GetEncodedHeaderSize(), fileSize);
		uint8 bps = GetEncodedBitsPerSample();
		if(bps % 8u != 0)
		{
			MPT_ASSERT(GetEncoding() == ADPCM && bps == 4);
			if(Util::MaxValueOfType(maxLength) / 2u >= maxLength)
				maxLength *= 2;
			else
				maxLength = Util::MaxValueOfType(maxLength);
		} else
		{
			size_t encodedBytesPerSample = GetNumChannels() * GetEncodedBitsPerSample() / 8u;
			// Check if we can round up without overflowing
			if(Util::MaxValueOfType(maxLength) - maxLength >= (encodedBytesPerSample - 1u))
				maxLength += encodedBytesPerSample - 1u;
			else
				maxLength = Util::MaxValueOfType(maxLength);
			maxLength /= encodedBytesPerSample;
		}
		LimitMax(sample.nLength, mpt::saturate_cast<SmpLength>(maxLength));
	} else if(GetEncoding() == IT214 || GetEncoding() == IT215 || GetEncoding() == MDL || GetEncoding() == DMF)
	{
		// In the best case, IT compression represents each sample point as a single bit.
		// In practice, there is of course the two-byte header per compressed block and the initial bit width change.
		// As a result, if we have a file length of n, we know that the sample can be at most n*8 sample points long.
		// For DMF, there are at least two bits per sample, and for MDL at least 5 (so both are worse than IT).
		size_t maxLength = fileSize;
		uint8 maxSamplesPerByte = 8 / GetNumChannels();
		if(Util::MaxValueOfType(maxLength) / maxSamplesPerByte >= maxLength)
			maxLength *= maxSamplesPerByte;
		else
			maxLength = Util::MaxValueOfType(maxLength);
		LimitMax(sample.nLength, mpt::saturate_cast<SmpLength>(maxLength));
	} else if(GetEncoding() == AMS)
	{
		if(fileSize <= 9)
			return 0;

		file.Skip(4);  // Target sample size (we already know this)
		SmpLength maxLength = std::min(file.ReadUint32LE(), mpt::saturate_cast<uint32>(fileSize));
		file.SkipBack(8);

		// In the best case, every byte triplet can decode to 255 bytes, which is a ratio of exactly 1:85
		if(Util::MaxValueOfType(maxLength) / 85 >= maxLength)
			maxLength *= 85;
		else
			maxLength = Util::MaxValueOfType(maxLength);
		LimitMax(sample.nLength, maxLength / (m_bitdepth / 8u));
	}

	if(sample.nLength < 1)
	{
		return 0;
	}

	sample.uFlags.set(CHN_16BIT, GetBitDepth() >= 16);
	sample.uFlags.set(CHN_STEREO, GetChannelFormat() != mono);
	size_t sampleSize = sample.AllocateSample();	// Target sample size in bytes

	if(sampleSize == 0)
	{
		sample.nLength = 0;
		return 0;
	}

	MPT_ASSERT(sampleSize >= sample.GetSampleSizeInBytes());

	//////////////////////////////////////////////////////
	// Compressed samples

	if(*this == SampleIO(_8bit, mono, littleEndian, ADPCM))
	{
		// 4-Bit ADPCM data
		int8 compressionTable[16];	// ADPCM Compression LUT
		if(file.ReadArray(compressionTable))
		{
			size_t readLength = (sample.nLength + 1) / 2;
			LimitMax(readLength, file.BytesLeft());

			const uint8 *inBuf = mpt::byte_cast<const uint8*>(sourceBuf) + sizeof(compressionTable);
			int8 *outBuf = sample.sample8();
			int8 delta = 0;

			for(size_t i = readLength; i != 0; i--)
			{
				delta += compressionTable[*inBuf & 0x0F];
				*(outBuf++) = delta;
				delta += compressionTable[(*inBuf >> 4) & 0x0F];
				*(outBuf++) = delta;
				inBuf++;
			}
			bytesRead = sizeof(compressionTable) + readLength;
		}
	} else if(GetEncoding() == IT214 || GetEncoding() == IT215)
	{
		// IT 2.14 / 2.15 compressed samples
		ITDecompression(file, sample, GetEncoding() == IT215);
		bytesRead = file.GetPosition() - filePosition;
	} else if(GetEncoding() == AMS && GetChannelFormat() == mono)
	{
		// AMS compressed samples
		file.Skip(4);  // Target sample size (we already know this)
		uint32 sourceSize = file.ReadUint32LE();
		int8 packCharacter = file.ReadUint8();
		bytesRead += 9;

		FileReader::PinnedRawDataView packedDataView = file.ReadPinnedRawDataView(sourceSize);
		LimitMax(sourceSize, mpt::saturate_cast<uint32>(packedDataView.size()));
		bytesRead += sourceSize;

		AMSUnpack(reinterpret_cast<const int8 *>(packedDataView.data()), packedDataView.size(), sample.samplev(), sample.GetSampleSizeInBytes(), packCharacter);
		if(sample.uFlags[CHN_16BIT] && !mpt::endian_is_little())
		{
			auto p = sample.sample16();
			for(SmpLength length = sample.nLength; length != 0; length--, p++)
			{
				*p = mpt::bit_cast<int16le>(*p);
			}
		}
	} else if(GetEncoding() == PTM8Dto16 && GetChannelFormat() == mono && GetBitDepth() == 16)
	{
		// PTM 8-Bit delta to 16-Bit sample
		bytesRead = CopyMonoSample<SC::DecodeInt16Delta8>(sample, sourceBuf, fileSize);
	} else if(GetEncoding() == MDL && GetChannelFormat() == mono && GetBitDepth() <= 16)
	{
		// Huffman MDL compressed samples
		if(file.CanRead(8) && (fileSize = file.ReadUint32LE()) >= 4)
		{
			BitReader chunk = file.ReadChunk(fileSize);
			bytesRead = chunk.GetLength() + 4;

			uint8 dlt = 0, lowbyte = 0;
			const bool is16bit = GetBitDepth() == 16;
			try
			{
				for(SmpLength j = 0; j < sample.nLength; j++)
				{
					uint8 hibyte;
					if(is16bit)
					{
						lowbyte = static_cast<uint8>(chunk.ReadBits(8));
					}
					bool sign = chunk.ReadBits(1) != 0;
					if(chunk.ReadBits(1))
					{
						hibyte = static_cast<uint8>(chunk.ReadBits(3));
					} else
					{
						hibyte = 8;
						while(!chunk.ReadBits(1))
						{
							hibyte += 0x10;
						}
						hibyte += static_cast<uint8>(chunk.ReadBits(4));
					}
					if(sign)
					{
						hibyte = ~hibyte;
					}
					dlt += hibyte;
					if(!is16bit)
					{
						sample.sample8()[j] = dlt;
					} else
					{
						sample.sample16()[j] = lowbyte | (dlt << 8);
					}
				}
			} catch(const BitReader::eof &)
			{
				// Data is not sufficient to decode the whole sample
				//AddToLog(LogWarning, "Truncated MDL sample block");
			}
		}
	} else if(GetEncoding() == DMF && GetChannelFormat() == mono && GetBitDepth() <= 16)
	{
		// DMF Huffman compression
		if(fileSize > 4)
		{
			bytesRead = DMFUnpack(file, mpt::byte_cast<uint8 *>(sample.sampleb()), sample.GetSampleSizeInBytes());
		}
#ifdef MODPLUG_TRACKER
	} else if((GetEncoding() == uLaw || GetEncoding() == aLaw) && GetBitDepth() == 16 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved))
	{
		// 8-to-16 bit G.711 u-law / a-law
		static constexpr int16 uLawTable[256] =
		{
			-32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
			-23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
			-15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
			-11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316,
			 -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
			 -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
			 -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
			 -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
			 -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
			 -1372, -1308, -1244, -1180, -1116, -1052,  -988,  -924,
			  -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
			  -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
			  -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260,
			  -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132,
			  -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64,
			   -56,   -48,   -40,   -32,   -24,   -16,    -8,    -1,
			 32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
			 23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
			 15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
			 11900, 11388, 10876, 10364,  9852,  9340,  8828,  8316,
			  7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140,
			  5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
			  3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,
			  2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
			  1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436,
			  1372,  1308,  1244,  1180,  1116,  1052,   988,   924,
			   876,   844,   812,   780,   748,   716,   684,   652,
			   620,   588,   556,   524,   492,   460,   428,   396,
			   372,   356,   340,   324,   308,   292,   276,   260,
			   244,   228,   212,   196,   180,   164,   148,   132,
			   120,   112,   104,    96,    88,    80,    72,    64,
			    56,    48,    40,    32,    24,    16,     8,     0,
		};

		static constexpr int16 aLawTable[256] =
		{
			 -5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736,
			 -7552, -7296, -8064, -7808, -6528, -6272, -7040, -6784,
			 -2752, -2624, -3008, -2880, -2240, -2112, -2496, -2368,
			 -3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,
			-22016,-20992,-24064,-23040,-17920,-16896,-19968,-18944,
			-30208,-29184,-32256,-31232,-26112,-25088,-28160,-27136,
			-11008,-10496,-12032,-11520, -8960, -8448, -9984, -9472,
			-15104,-14592,-16128,-15616,-13056,-12544,-14080,-13568,
			  -344,  -328,  -376,  -360,  -280,  -264,  -312,  -296,
			  -472,  -456,  -504,  -488,  -408,  -392,  -440,  -424,
			   -88,   -72,   -120,  -104,  -24,   -8,    -56,   -40,
			  -216,  -200,  -248,  -232,  -152,  -136,  -184,  -168,
			 -1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184,
			 -1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696,
			  -688,  -656,  -752,  -720,  -560,  -528,  -624,  -592,
			  -944,  -912,  -1008, -976,  -816,  -784,  -880,  -848,
			  5504,  5248,  6016,  5760,  4480,  4224,  4992,  4736,
			  7552,  7296,  8064,  7808,  6528,  6272,  7040,  6784,
			  2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368,
			  3776,  3648,  4032,  3904,  3264,  3136,  3520,  3392,
			 22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
			 30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
			 11008, 10496, 12032, 11520,  8960,  8448,  9984,  9472,
			 15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
			   344,   328,   376,   360,   280,   264,   312,   296,
			   472,   456,   504,   488,   408,   392,   440,   424,
			    88,    72,   120,   104,    24,     8,    56,    40,
			   216,   200,   248,   232,   152,   136,   184,   168,
			  1376,  1312,  1504,  1440,  1120,  1056,  1248,  1184,
			  1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696,
			   688,   656,   752,   720,   560,   528,   624,   592,
			   944,   912,  1008,   976,   816,   784,   880,   848,
		};

		const auto &lut = GetEncoding() == uLaw ? uLawTable : aLawTable;

		SmpLength readLength = sample.nLength * GetNumChannels();
		LimitMax(readLength, mpt::saturate_cast<SmpLength>(file.BytesLeft()));
		bytesRead = readLength;

		const uint8 *inBuf = mpt::byte_cast<const uint8*>(sourceBuf);
		int16 *outBuf = sample.sample16();

		while(readLength--)
		{
			*(outBuf++) = lut[*(inBuf++)];
		}
#endif // MODPLUG_TRACKER
	}


	/////////////////////////
	// Uncompressed samples

	//////////////////////////////////////////////////////
	// 8-Bit / Mono / PCM
	else if(GetBitDepth() == 8 && GetChannelFormat() == mono)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 8-Bit / Mono / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt8>(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Mono / Unsigned / PCM
			bytesRead = CopyMonoSample<SC::DecodeUint8>(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Mono / Delta / PCM
		case MT2:
			bytesRead = CopyMonoSample<SC::DecodeInt8Delta>(sample, sourceBuf, fileSize);
			break;
		case PCM7to8:		// 7 Bit stored as 8-Bit with highest bit unused / Mono / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt7>(sample, sourceBuf, fileSize);
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 8-Bit / Stereo Split / PCM
	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoSplit)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 8-Bit / Stereo Split / Signed / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt8>(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeUint8>(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Stereo Split / Delta / PCM
		case MT2:			// same as deltaPCM, but right channel is stored as a difference from the left channel
			bytesRead = CopyStereoSplitSample<SC::DecodeInt8Delta>(sample, sourceBuf, fileSize);
			if(GetEncoding() == MT2)
			{
				for(int8 *p = sample.sample8(), *pEnd = p + sample.nLength * 2; p < pEnd; p += 2)
				{
					p[1] = static_cast<int8>(static_cast<uint8>(p[0]) + static_cast<uint8>(p[1]));
				}
			}
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 8-Bit / Stereo Interleaved / PCM
	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoInterleaved)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 8-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt8>(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 8-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeUint8>(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 8-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt8Delta>(sample, sourceBuf, fileSize);
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Mono / Little Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == mono && GetEndianness() == littleEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
		case MT2:
			bytesRead = CopyMonoSample<SC::DecodeInt16Delta<littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Mono / Big Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == mono && GetEndianness() == bigEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Mono / Signed / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Mono / Unsigned / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Mono / Delta / PCM
			bytesRead = CopyMonoSample<SC::DecodeInt16Delta<bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Split / Little Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoSplit && GetEndianness() == littleEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Split / Signed / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Split / Delta / PCM
		case MT2:		// same as deltaPCM, but right channel is stored as a difference from the left channel
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16Delta<littleEndian16> >(sample, sourceBuf, fileSize);
			if(GetEncoding() == MT2)
			{
				for(int16 *p = sample.sample16(), *pEnd = p + sample.nLength * 2; p < pEnd; p += 2)
				{
					p[1] = static_cast<int16>(static_cast<uint16>(p[0]) + static_cast<uint16>(p[1]));
				}
			}
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Split / Big Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoSplit && GetEndianness() == bigEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Split / Signed / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Split / Unsigned / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Split / Delta / PCM
			bytesRead = CopyStereoSplitSample<SC::DecodeInt16Delta<bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Interleaved / Little Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoInterleaved && GetEndianness() == littleEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0x8000u, littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16Delta<littleEndian16> >(sample, sourceBuf, fileSize);
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 16-Bit / Stereo Interleaved / Big Endian / PCM
	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoInterleaved && GetEndianness() == bigEndian)
	{
		switch(GetEncoding())
		{
		case signedPCM:		// 16-Bit / Stereo Interleaved / Signed / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case unsignedPCM:	// 16-Bit / Stereo Interleaved / Unsigned / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16<0x8000u, bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		case deltaPCM:		// 16-Bit / Stereo Interleaved / Delta / PCM
			bytesRead = CopyStereoInterleavedSample<SC::DecodeInt16Delta<bigEndian16> >(sample, sourceBuf, fileSize);
			break;
		default:
			MPT_ASSERT_NOTREACHED();
			break;
		}
	}

	//////////////////////////////////////////////////////
	// 24-Bit / Signed / Mono / PCM
	else if(GetBitDepth() == 24 && GetChannelFormat() == mono && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, bigEndian24> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 24-Bit / Signed / Stereo Interleaved / PCM
	else if(GetBitDepth() == 24 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, bigEndian24> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Signed / Mono / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Signed / Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 64-Bit / Signed / Mono / PCM
	else if(GetBitDepth() == 64 && GetChannelFormat() == mono && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int64>, SC::DecodeInt64<0, littleEndian64> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, int64>, SC::DecodeInt64<0, bigEndian64> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 64-Bit / Signed / Stereo Interleaved / PCM
	else if(GetBitDepth() == 64 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int64>, SC::DecodeInt64<0, littleEndian64> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, int64>, SC::DecodeInt64<0, bigEndian64> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Mono / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == floatPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == floatPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<littleEndian32> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 64-Bit / Float / Mono / PCM
	else if(GetBitDepth() == 64 && GetChannelFormat() == mono && GetEncoding() == floatPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, float64>, SC::DecodeFloat64<littleEndian64> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyMonoSample<SC::ConversionChain<SC::Convert<int16, float64>, SC::DecodeFloat64<bigEndian64> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 64-Bit / Float / Stereo Interleaved / PCM
	else if(GetBitDepth() == 64 && GetChannelFormat() == stereoInterleaved && GetEncoding() == floatPCM)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, float64>, SC::DecodeFloat64<littleEndian64> > >(sample, sourceBuf, fileSize);
		} else
		{
			bytesRead = CopyStereoInterleavedSample<SC::ConversionChain<SC::Convert<int16, float64>, SC::DecodeFloat64<bigEndian64> > >(sample, sourceBuf, fileSize);
		}
	}

	//////////////////////////////////////////////////////
	// 24-Bit / Signed / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 24 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == signedPCMnormalize)
	{
		// Normalize to 16-Bit
		uint32 srcPeak = uint32(1)<<31;
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, sourceBuf, fileSize, &srcPeak);
		} else
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, bigEndian24> > >(sample, sourceBuf, fileSize, &srcPeak);
		}
		if(bytesRead && srcPeak != uint32(1)<<31)
		{
			// Adjust sample volume so we do not affect relative volume of the sample. Normalizing is only done to increase precision.
			sample.nGlobalVol = static_cast<uint16>(Clamp(Util::muldivr_unsigned(sample.nGlobalVol, srcPeak, uint32(1)<<31), uint32(1), uint32(64)));
			sample.uFlags.set(SMP_MODIFIED);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Signed / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == signedPCMnormalize)
	{
		// Normalize to 16-Bit
		uint32 srcPeak = uint32(1)<<31;
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, littleEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		} else
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt32<0, bigEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		}
		if(bytesRead && srcPeak != uint32(1)<<31)
		{
			// Adjust sample volume so we do not affect relative volume of the sample. Normalizing is only done to increase precision.
			sample.nGlobalVol = static_cast<uint16>(Clamp(Util::muldivr_unsigned(sample.nGlobalVol, srcPeak, uint32(1)<<31), uint32(1), uint32(64)));
			sample.uFlags.set(SMP_MODIFIED);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 32 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == floatPCMnormalize)
	{
		// Normalize to 16-Bit
		float32 srcPeak = 1.0f;
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float32>, SC::DecodeFloat32<littleEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		} else
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, sourceBuf, fileSize, &srcPeak);
		}
		if(bytesRead && srcPeak != 1.0f)
		{
			// Adjust sample volume so we do not affect relative volume of the sample. Normalizing is only done to increase precision.
			sample.nGlobalVol = mpt::saturate_round<uint16>(Clamp(sample.nGlobalVol * srcPeak, 1.0f, 64.0f));
			sample.uFlags.set(SMP_MODIFIED);
		}
	}

	//////////////////////////////////////////////////////
	// 64-Bit / Float / Mono, Stereo Interleaved / PCM
	else if(GetBitDepth() == 64 && (GetChannelFormat() == mono || GetChannelFormat() == stereoInterleaved) && GetEncoding() == floatPCMnormalize)
	{
		// Normalize to 16-Bit
		float64 srcPeak = 1.0;
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float64>, SC::DecodeFloat64<littleEndian64> > >(sample, sourceBuf, fileSize, &srcPeak);
		} else
		{
			bytesRead = CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float64>, SC::DecodeFloat64<bigEndian64> > >(sample, sourceBuf, fileSize, &srcPeak);
		}
		if(bytesRead && srcPeak != 1.0)
		{
			// Adjust sample volume so we do not affect relative volume of the sample. Normalizing is only done to increase precision.
			sample.nGlobalVol = mpt::saturate_round<uint16>(Clamp(sample.nGlobalVol * srcPeak, 1.0, 64.0));
			sample.uFlags.set(SMP_MODIFIED);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Mono / PCM / full scale 2^15
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == floatPCM15)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		} else
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM / full scale 2^15
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == floatPCM15)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		} else
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<15)))
				);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM / full scale 2^23
	else if(GetBitDepth() == 32 && GetChannelFormat() == mono && GetEncoding() == floatPCM23)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		} else
		{
			bytesRead = CopyMonoSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		}
	}

	//////////////////////////////////////////////////////
	// 32-Bit / Float / Stereo Interleaved / PCM / full scale 2^23
	else if(GetBitDepth() == 32 && GetChannelFormat() == stereoInterleaved && GetEncoding() == floatPCM23)
	{
		if(GetEndianness() == littleEndian)
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<littleEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<littleEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		} else
		{
			bytesRead = CopyStereoInterleavedSample
				(sample, sourceBuf, fileSize,
				SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeScaledFloat32<bigEndian32> >
				(SC::Convert<int16, float32>(), SC::DecodeScaledFloat32<bigEndian32>(1.0f / static_cast<float>(1<<23)))
				);
		}
	}

	////////////////
	// Unsupported
	else
	{
		MPT_ASSERT_NOTREACHED();
	}

	MPT_ASSERT(filePosition + bytesRead <= file.GetLength());
	file.Seek(filePosition + bytesRead);
	return bytesRead;
}


#ifndef MODPLUG_NO_FILESAVE


// Write a sample to file
size_t SampleIO::WriteSample(std::ostream &f, const ModSample &sample, SmpLength maxSamples) const
{
	if(sample.uFlags[CHN_ADLIB])
	{
		mpt::IO::Write(f, sample.adlib);
		return sizeof(sample.adlib);
	}
	if(!sample.HasSampleData())
	{
		return 0;
	}

	std::array<std::byte, mpt::IO::BUFFERSIZE_TINY> writeBuffer;
	mpt::IO::WriteBuffer<std::ostream> fb{f, mpt::as_span(writeBuffer)};

	SmpLength numSamples = sample.nLength;

	if(maxSamples && numSamples > maxSamples)
	{
		numSamples = maxSamples;
	}

	std::size_t len = CalculateEncodedSize(numSamples);

	if(GetBitDepth() == 16 && GetChannelFormat() == mono && GetEndianness() == littleEndian &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 16-bit little-endian mono samples
		MPT_ASSERT(len == numSamples * 2);
		const int16 *const pSample16 = sample.sample16();
		const int16 *p = pSample16;
		int s_old = 0;
		const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x8000 : 0;
		for(SmpLength j = 0; j < numSamples; j++)
		{
			int s_new = *p;
			p++;
			if(sample.uFlags[CHN_STEREO])
			{
				// Downmix stereo
				s_new = (s_new + (*p) + 1) / 2;
				p++;
			}
			if(GetEncoding() == deltaPCM)
			{
				mpt::IO::Write(fb, mpt::as_le(static_cast<int16>(s_new - s_old)));
				s_old = s_new;
			} else
			{
				mpt::IO::Write(fb, mpt::as_le(static_cast<int16>(s_new + s_ofs)));
			}
		}
	}

	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoSplit &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 8-bit Stereo samples (not interleaved)
		MPT_ASSERT(len == numSamples * 2);
		const int8 *const pSample8 = sample.sample8();
		const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x80 : 0;
		for (uint32 iCh=0; iCh<2; iCh++)
		{
			const int8 *p = pSample8 + iCh;
			int s_old = 0;
			for (SmpLength j = 0; j < numSamples; j++)
			{
				int s_new = *p;
				p += 2;
				if (GetEncoding() == deltaPCM)
				{
					mpt::IO::Write(fb, static_cast<int8>(s_new - s_old));
					s_old = s_new;
				} else
				{
					mpt::IO::Write(fb, static_cast<int8>(s_new + s_ofs));
				}
			}
		}
	}

	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoSplit && GetEndianness() == littleEndian &&
		(GetEncoding() == signedPCM || GetEncoding() == unsignedPCM || GetEncoding() == deltaPCM))
	{
		// 16-bit little-endian Stereo samples (not interleaved)
		MPT_ASSERT(len == numSamples * 4);
		const int16 *const pSample16 = sample.sample16();
		const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x8000 : 0;
		for (uint32 iCh=0; iCh<2; iCh++)
		{
			const int16 *p = pSample16 + iCh;
			int s_old = 0;
			for (SmpLength j = 0; j < numSamples; j++)
			{
				int s_new = *p;
				p += 2;
				if (GetEncoding() == deltaPCM)
				{
					mpt::IO::Write(fb, mpt::as_le(static_cast<int16>(s_new - s_old)));
					s_old = s_new;
				} else
				{
					mpt::IO::Write(fb, mpt::as_le(static_cast<int16>(s_new + s_ofs)));
				}
			}
		}
	}

	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM)
	{
		// Stereo signed interleaved
		MPT_ASSERT(len == numSamples * 2);
		const int8 *const pSample8 = sample.sample8();
		mpt::IO::WriteRaw(f, reinterpret_cast<const std::byte*>(pSample8), len);
	}

	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM && GetEndianness() == littleEndian)
	{
		// Stereo signed interleaved
		MPT_ASSERT(len == numSamples * 4);
		const int16 *const pSample16 = sample.sample16();
		const int16 *p = pSample16;
		for(SmpLength j = 0; j < numSamples; j++)
		{
			mpt::IO::Write(fb, mpt::as_le(p[0]));
			mpt::IO::Write(fb, mpt::as_le(p[1]));
			p += 2;
		}
	}

	else if(GetBitDepth() == 16 && GetChannelFormat() == stereoInterleaved && GetEncoding() == signedPCM && GetEndianness() == bigEndian)
	{
		// Stereo signed interleaved
		MPT_ASSERT(len == numSamples * 4);
		const int16 *const pSample16 = sample.sample16();
		const int16 *p = pSample16;
		for(SmpLength j = 0; j < numSamples; j++)
		{
			mpt::IO::Write(fb, mpt::as_be(p[0]));
			mpt::IO::Write(fb, mpt::as_be(p[1]));
			p += 2;
		}
	}

	else if(GetBitDepth() == 8 && GetChannelFormat() == stereoInterleaved && GetEncoding() == unsignedPCM)
	{
		// Stereo unsigned interleaved
		MPT_ASSERT(len == numSamples * 2);
		const int8 *const pSample8 = sample.sample8();
		for(SmpLength j = 0; j < numSamples * 2; j++)
		{
			mpt::IO::Write(fb, static_cast<int8>(static_cast<uint8>(pSample8[j]) + 0x80));
		}
	}

	else if(GetEncoding() == IT214 || GetEncoding() == IT215)
	{
		// IT2.14-encoded samples
		ITCompression its(sample, GetEncoding() == IT215, &f, numSamples);
		len = its.GetCompressedSize();
	}

	// Default: assume 8-bit PCM data
	else
	{
		MPT_ASSERT(GetBitDepth() == 8);
		MPT_ASSERT(len == numSamples);
		if(sample.uFlags[CHN_16BIT])
		{
			const int16 *p = sample.sample16();
			int s_old = 0;
			const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x80 : 0;
			for(SmpLength j = 0; j < numSamples; j++)
			{
				int s_new = mpt::rshift_signed(*p, 8);
				p++;
				if(sample.uFlags[CHN_STEREO])
				{
					s_new = (s_new + mpt::rshift_signed(*p, 8) + 1) / 2;
					p++;
				}
				if(GetEncoding() == deltaPCM)
				{
					mpt::IO::Write(fb, static_cast<int8>(s_new - s_old));
					s_old = s_new;
				} else
				{
					mpt::IO::Write(fb, static_cast<int8>(s_new + s_ofs));
				}
			}
		} else
		{
			const int8 *const pSample8 = sample.sample8();
			const int8 *p = pSample8;
			int s_old = 0;
			const int s_ofs = (GetEncoding() == unsignedPCM) ? 0x80 : 0;
			for(SmpLength j = 0; j < numSamples; j++)
			{
				int s_new = *p;
				p++;
				if(sample.uFlags[CHN_STEREO])
				{
					s_new = (s_new + (static_cast<int>(*p)) + 1) / 2;
					p++;
				}
				if(GetEncoding() == deltaPCM)
				{
					mpt::IO::Write(fb, static_cast<int8>(s_new - s_old));
					s_old = s_new;
				} else
				{
					mpt::IO::Write(fb, static_cast<int8>(s_new + s_ofs));
				}
			}
		}
	}

	return len;
}


#endif // MODPLUG_NO_FILESAVE


OPENMPT_NAMESPACE_END
