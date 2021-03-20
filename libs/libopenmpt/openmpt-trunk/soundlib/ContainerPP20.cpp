/*
 * ContainerPP20.cpp
 * -----------------
 * Purpose: Handling of PowerPack PP20 compressed modules
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"

#include "../common/FileReader.h"
#include "Container.h"
#include "Sndfile.h"

#include <stdexcept>


OPENMPT_NAMESPACE_BEGIN


struct PPBITBUFFER
{
	uint32 bitcount = 0;
	uint32 bitbuffer = 0;
	const uint8 *pStart = nullptr;
	const uint8 *pSrc = nullptr;

	uint32 GetBits(uint32 n);
};


uint32 PPBITBUFFER::GetBits(uint32 n)
{
	uint32 result = 0;

	for(uint32 i = 0; i < n; i++)
	{
		if(!bitcount)
		{
			bitcount = 8;
			if(pSrc != pStart)
				pSrc--;
			bitbuffer = *pSrc;
		}
		result = (result << 1) | (bitbuffer & 1);
		bitbuffer >>= 1;
		bitcount--;
	}
	return result;
}


static bool PP20_DoUnpack(const uint8 *pSrc, uint32 srcLen, uint8 *pDst, uint32 dstLen)
{
	const std::array<uint8, 4> modeTable{pSrc[0], pSrc[1], pSrc[2], pSrc[3]};
	PPBITBUFFER BitBuffer;
	BitBuffer.pStart = pSrc;
	BitBuffer.pSrc = pSrc + srcLen - 4;
	BitBuffer.GetBits(pSrc[srcLen - 1]);
	uint32 bytesLeft = dstLen;
	while(bytesLeft > 0)
	{
		if(!BitBuffer.GetBits(1))
		{
			uint32 count = 1, countAdd;
			do
			{
				countAdd = BitBuffer.GetBits(2);
				count += countAdd;
			} while(countAdd == 3);
			LimitMax(count, bytesLeft);
			for(uint32 i = 0; i < count; i++)
			{
				pDst[--bytesLeft] = (uint8)BitBuffer.GetBits(8);
			}
			if(!bytesLeft)
				break;
		}
		{
			uint32 modeIndex = BitBuffer.GetBits(2);
			MPT_CHECKER_ASSUME(modeIndex < 4);
			uint32 count = modeIndex + 2, offset;
			if(modeIndex == 3)
			{
				offset = BitBuffer.GetBits((BitBuffer.GetBits(1)) ? modeTable[modeIndex] : 7);
				uint32 countAdd = 7;
				do
				{
					countAdd = BitBuffer.GetBits(3);
					count += countAdd;
				} while(countAdd == 7);
			} else
			{
				offset = BitBuffer.GetBits(modeTable[modeIndex]);
			}
			LimitMax(count, bytesLeft);
			for(uint32 i = 0; i < count; i++)
			{
				pDst[bytesLeft - 1] = (bytesLeft + offset < dstLen) ? pDst[bytesLeft + offset] : 0;
				--bytesLeft;
			}
		}
	}
	return true;
}


struct PP20header
{
	char  magic[4];       // "PP20"
	uint8 efficiency[4];
};

MPT_BINARY_STRUCT(PP20header, 8)


static bool ValidateHeader(const PP20header &hdr)
{
	if(std::memcmp(hdr.magic, "PP20", 4) != 0)
	{
		return false;
	}
	if(hdr.efficiency[0] < 9 || hdr.efficiency[0] > 15
		|| hdr.efficiency[1] < 9 || hdr.efficiency[1] > 15
		|| hdr.efficiency[2] < 9 || hdr.efficiency[2] > 15
		|| hdr.efficiency[3] < 9 || hdr.efficiency[3] > 15)
	{
		return false;
	}
	return true;
}


CSoundFile::ProbeResult CSoundFile::ProbeFileHeaderPP20(MemoryFileReader file, const uint64 *pfilesize)
{
	PP20header hdr;
	if(!file.ReadStruct(hdr))
	{
		return ProbeWantMoreData;
	}
	if(!ValidateHeader(hdr))
	{
		return ProbeFailure;
	}
	MPT_UNREFERENCED_PARAMETER(pfilesize);
	return ProbeSuccess;
}


bool UnpackPP20(std::vector<ContainerItem> &containerItems, FileReader &file, ContainerLoadingFlags loadFlags)
{
	file.Rewind();
	containerItems.clear();

	PP20header hdr;
	if(!file.ReadStruct(hdr))
	{
		return false;
	}
	if(!ValidateHeader(hdr))
	{
		return false;
	}
	if(loadFlags == ContainerOnlyVerifyHeader)
	{
		return true;
	}

	if(!file.CanRead(4))
	{
		return false;
	}

	containerItems.emplace_back();
	containerItems.back().data_cache = std::make_unique<std::vector<char> >();
	std::vector<char> & unpackedData = *(containerItems.back().data_cache);

	FileReader::off_t length = file.GetLength();
	if(!Util::TypeCanHoldValue<uint32>(length)) return false;
	// Length word must be aligned
	if((length % 2u) != 0)
		return false;

	file.Seek(length - 4);
	uint32 dstLen = 0;
	dstLen |= file.ReadUint8() << 16;
	dstLen |= file.ReadUint8() << 8;
	dstLen |= file.ReadUint8() << 0;
	if(dstLen == 0) return false;
	try
	{
		unpackedData.resize(dstLen);
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		return false;
	}
	file.Seek(4);
	bool result = PP20_DoUnpack(file.GetRawData<uint8>(), static_cast<uint32>(length - 4), mpt::byte_cast<uint8 *>(unpackedData.data()), dstLen);

	if(result)
	{
		containerItems.back().file = FileReader(mpt::byte_cast<mpt::const_byte_span>(mpt::as_span(unpackedData)));
	}

	return result;
}


OPENMPT_NAMESPACE_END
