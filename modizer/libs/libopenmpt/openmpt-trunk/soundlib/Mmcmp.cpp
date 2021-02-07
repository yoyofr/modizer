/*
 * mmcmp.cpp
 * ---------
 * Purpose: Handling of compressed modules (MMCMP, XPK, PowerPack PP20)
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "../common/FileReader.h"

#include <stdexcept>


OPENMPT_NAMESPACE_BEGIN


//#define MMCMP_LOG

struct MMCMPFILEHEADER
{
	char     id[8];	// "ziRCONia"
	uint16le hdrsize;
};

MPT_BINARY_STRUCT(MMCMPFILEHEADER, 10)

struct MMCMPHEADER
{
	uint16le version;
	uint16le nblocks;
	uint32le filesize;
	uint32le blktable;
	uint8le  glb_comp;
	uint8le  fmt_comp;
};

MPT_BINARY_STRUCT(MMCMPHEADER, 14)

struct MMCMPBLOCK
{
	uint32le unpk_size;
	uint32le pk_size;
	uint32le xor_chk;
	uint16le sub_blk;
	uint16le flags;
	uint16le tt_entries;
	uint16le num_bits;
};

MPT_BINARY_STRUCT(MMCMPBLOCK, 20)

struct MMCMPSUBBLOCK
{
	uint32le unpk_pos;
	uint32le unpk_size;
};

MPT_BINARY_STRUCT(MMCMPSUBBLOCK, 8)


#define MMCMP_COMP		0x0001
#define MMCMP_DELTA		0x0002
#define MMCMP_16BIT		0x0004
#define MMCMP_STEREO	0x0100
#define MMCMP_ABS16		0x0200
#define MMCMP_ENDIAN	0x0400

struct MMCMPBITBUFFER
{
	uint32 bitcount;
	uint32 bitbuffer;
	const uint8 *pSrc;
	uint32 bytesLeft;

	uint32 GetBits(uint32 nBits);
};


uint32 MMCMPBITBUFFER::GetBits(uint32 nBits)
//------------------------------------------
{
	uint32 d;
	if (!nBits) return 0;
	while (bitcount < 24)
	{
		if(bytesLeft)
		{
			bitbuffer |= *pSrc << bitcount;
			pSrc++;
			bytesLeft--;
		}
		bitcount += 8;
	}
	d = bitbuffer & ((1 << nBits) - 1);
	bitbuffer >>= nBits;
	bitcount -= nBits;
	return d;
}

static const uint32 MMCMP8BitCommands[8] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF8
};

static const uint32 MMCMP8BitFetch[8] =
{
	3, 3, 3, 3, 2, 1, 0, 0
};

static const uint32 MMCMP16BitCommands[16] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF0,
	0x1F0, 0x3F0, 0x7F0, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFFF0
};

static const uint32 MMCMP16BitFetch[16] =
{
	4, 4, 4, 4, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};


static bool MMCMP_IsDstBlockValid(const std::vector<char> &unpackedData, uint32 pos, uint32 len)
//----------------------------------------------------------------------------------------------
{
	if(pos >= unpackedData.size()) return false;
	if(len > unpackedData.size()) return false;
	if(len > unpackedData.size() - pos) return false;
	return true;
}


static bool MMCMP_IsDstBlockValid(const std::vector<char> &unpackedData, const MMCMPSUBBLOCK &subblk)
//---------------------------------------------------------------------------------------------------
{
	return MMCMP_IsDstBlockValid(unpackedData, subblk.unpk_pos, subblk.unpk_size);
}


bool UnpackMMCMP(std::vector<char> &unpackedData, FileReader &file)
//-----------------------------------------------------------------
{
	file.Rewind();
	unpackedData.clear();

	MMCMPFILEHEADER mfh;
	if(!file.ReadStruct(mfh)) return false;
	if(std::memcmp(mfh.id, "ziRCONia", 8) != 0) return false;
	if(mfh.hdrsize != sizeof(MMCMPHEADER)) return false;
	MMCMPHEADER mmh;
	if(!file.ReadStruct(mmh)) return false;
	if(mmh.nblocks == 0) return false;
	if(mmh.filesize == 0) return false;
	if(mmh.filesize > 0x80000000) return false;
	if(mmh.blktable > file.GetLength()) return false;
	if(mmh.blktable + 4 * mmh.nblocks > file.GetLength()) return false;

	unpackedData.resize(mmh.filesize);
	// 8-bit deltas
	uint8 ptable[256] = { 0 };

	for (uint32 nBlock=0; nBlock<mmh.nblocks; nBlock++)
	{
		if(!file.Seek(mmh.blktable + 4*nBlock)) return false;
		if(!file.CanRead(4)) return false;
		uint32 blkPos = file.ReadUint32LE();
		if(!file.Seek(blkPos)) return false;
		MMCMPBLOCK blk;
		if(!file.ReadStruct(blk)) return false;
		std::vector<MMCMPSUBBLOCK> subblks(blk.sub_blk);
		for(uint32 i=0; i<blk.sub_blk; ++i)
		{
			if(!file.ReadStruct(subblks[i])) return false;
		}
		const MMCMPSUBBLOCK *psubblk = blk.sub_blk > 0 ? subblks.data() : nullptr;

		if(blkPos + sizeof(MMCMPBLOCK) + blk.sub_blk * sizeof(MMCMPSUBBLOCK) >= file.GetLength()) return false;
		uint32 memPos = blkPos + sizeof(MMCMPBLOCK) + blk.sub_blk * sizeof(MMCMPSUBBLOCK);

#ifdef MMCMP_LOG
		Log("block %d: flags=%04X sub_blocks=%d", nBlock, (uint32)pblk->flags, (uint32)pblk->sub_blk);
		Log(" pksize=%d unpksize=%d", pblk->pk_size, pblk->unpk_size);
		Log(" tt_entries=%d num_bits=%d\n", pblk->tt_entries, pblk->num_bits);
#endif
		// Data is not packed
		if (!(blk.flags & MMCMP_COMP))
		{
			for (uint32 i=0; i<blk.sub_blk; i++)
			{
				if(!psubblk) return false;
				if(!MMCMP_IsDstBlockValid(unpackedData, *psubblk)) return false;
#ifdef MMCMP_LOG
				Log("  Unpacked sub-block %d: offset %d, size=%d\n", i, psubblk->unpk_pos, psubblk->unpk_size);
#endif
				if(!file.Seek(memPos)) return false;
				if(file.ReadRaw(&(unpackedData[psubblk->unpk_pos]), psubblk->unpk_size) != psubblk->unpk_size) return false;
				psubblk++;
			}
		} else
		// Data is 16-bit packed
		if (blk.flags & MMCMP_16BIT)
		{
			MMCMPBITBUFFER bb;
			uint32 subblk = 0;
			if(!psubblk) return false;
			if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
			char *pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
			uint32 dwSize = psubblk[subblk].unpk_size >> 1;
			uint32 dwPos = 0;
			uint32 numbits = blk.num_bits;
			uint32 oldval = 0;

#ifdef MMCMP_LOG
			Log("  16-bit block: pos=%d size=%d ", psubblk->unpk_pos, psubblk->unpk_size);
			if (pblk->flags & MMCMP_DELTA) Log("DELTA ");
			if (pblk->flags & MMCMP_ABS16) Log("ABS16 ");
			Log("\n");
#endif
			bb.bitcount = 0;
			bb.bitbuffer = 0;
			if(!file.Seek(memPos + blk.tt_entries)) return false;
			if(!file.CanRead(blk.pk_size - blk.tt_entries)) return false;
			bb.pSrc = file.GetRawData<uint8>();
			bb.bytesLeft = blk.pk_size - blk.tt_entries;
			while (subblk < blk.sub_blk)
			{
				uint32 newval = 0x10000;
				uint32 d = bb.GetBits(numbits+1);

				uint32 command = MMCMP16BitCommands[numbits & 0x0F];
				if (d >= command)
				{
					uint32 nFetch = MMCMP16BitFetch[numbits & 0x0F];
					uint32 newbits = bb.GetBits(nFetch) + ((d - command) << nFetch);
					if (newbits != numbits)
					{
						numbits = newbits & 0x0F;
					} else
					{
						if ((d = bb.GetBits(4)) == 0x0F)
						{
							if (bb.GetBits(1)) break;
							newval = 0xFFFF;
						} else
						{
							newval = 0xFFF0 + d;
						}
					}
				} else
				{
					newval = d;
				}
				if (newval < 0x10000)
				{
					newval = (newval & 1) ? (uint32)(-(int32)((newval+1) >> 1)) : (uint32)(newval >> 1);
					if (blk.flags & MMCMP_DELTA)
					{
						newval += oldval;
						oldval = newval;
					} else
					if (!(blk.flags & MMCMP_ABS16))
					{
						newval ^= 0x8000;
					}
					pDest[dwPos*2 + 0] = (uint8)(((uint16)newval) & 0xff);
					pDest[dwPos*2 + 1] = (uint8)(((uint16)newval) >> 8);
					dwPos++;
				}
				if (dwPos >= dwSize)
				{
					subblk++;
					dwPos = 0;
					if(!(subblk < blk.sub_blk)) break;
					if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
					dwSize = psubblk[subblk].unpk_size >> 1;
					pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
				}
			}
		} else
		// Data is 8-bit packed
		{
			MMCMPBITBUFFER bb;
			uint32 subblk = 0;
			if(!psubblk) return false;
			if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
			char *pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
			uint32 dwSize = psubblk[subblk].unpk_size;
			uint32 dwPos = 0;
			uint32 numbits = blk.num_bits;
			uint32 oldval = 0;
			if(blk.tt_entries > sizeof(ptable)
				|| !file.Seek(memPos)
				|| file.ReadRaw(ptable, blk.tt_entries) < blk.tt_entries)
				return false;

			bb.bitcount = 0;
			bb.bitbuffer = 0;
			if(!file.CanRead(blk.pk_size - blk.tt_entries)) return false;
			bb.pSrc = file.GetRawData<uint8>();
			bb.bytesLeft = blk.pk_size - blk.tt_entries;
			while (subblk < blk.sub_blk)
			{
				uint32 newval = 0x100;
				uint32 d = bb.GetBits(numbits+1);

				uint32 command = MMCMP8BitCommands[numbits & 0x07];
				if (d >= command)
				{
					uint32 nFetch = MMCMP8BitFetch[numbits & 0x07];
					uint32 newbits = bb.GetBits(nFetch) + ((d - command) << nFetch);
					if (newbits != numbits)
					{
						numbits = newbits & 0x07;
					} else
					{
						if ((d = bb.GetBits(3)) == 7)
						{
							if (bb.GetBits(1)) break;
							newval = 0xFF;
						} else
						{
							newval = 0xF8 + d;
						}
					}
				} else
				{
					newval = d;
				}
				if (newval < sizeof(ptable))
				{
					int n = ptable[newval];
					if (blk.flags & MMCMP_DELTA)
					{
						n += oldval;
						oldval = n;
					}
					pDest[dwPos++] = (uint8)n;
				}
				if (dwPos >= dwSize)
				{
					subblk++;
					dwPos = 0;
					if(!(subblk < blk.sub_blk)) break;
					if(!MMCMP_IsDstBlockValid(unpackedData, psubblk[subblk])) return false;
					dwSize = psubblk[subblk].unpk_size;
					pDest = &(unpackedData[psubblk[subblk].unpk_pos]);
				}
			}
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//
// XPK unpacker
//

struct XPKFILEHEADER
{
	char      XPKF[4];
	uint32be SrcLen;
	char      SQSH[4];
	uint32be DstLen;
	char      Name[16];
	uint32be Reserved;
};

MPT_BINARY_STRUCT(XPKFILEHEADER, 36)


struct XPK_error : public std::range_error
{
	XPK_error() : std::range_error("invalid XPK data") { }
};

struct XPK_BufferBounds
{
	const uint8 *pSrcBeg;
	std::size_t SrcSize;
	uint8 *pDstBeg;
	std::size_t DstSize;

	inline uint8 SrcRead(std::size_t index)
	{
		if(index >= SrcSize) throw XPK_error();
		return pSrcBeg[index];
	}
	inline void DstWrite(std::size_t index, uint8 value)
	{
		if(index >= DstSize) throw XPK_error();
		pDstBeg[index] = value;
	}
	inline uint8 DstRead(std::size_t index)
	{
		if(index >= DstSize) throw XPK_error();
		return pDstBeg[index];
	}
};

static int32 bfextu(std::size_t p, int32 bo, int32 bc, XPK_BufferBounds &bufs)
//----------------------------------------------------------------------------
{
	int32 r;

	p += bo / 8;
	r = bufs.SrcRead(p); p++;
	r <<= 8;
	r |= bufs.SrcRead(p); p++;
	r <<= 8;
	r |= bufs.SrcRead(p);
	r <<= bo % 8;
	r &= 0xffffff;
	r >>= 24 - bc;

	return r;
}

static int32 bfexts(std::size_t p, int32 bo, int32 bc, XPK_BufferBounds &bufs)
//----------------------------------------------------------------------------
{
	int32 r;

	p += bo / 8;
	r = bufs.SrcRead(p); p++;
	r <<= 8;
	r |= bufs.SrcRead(p); p++;
	r <<= 8;
	r |= bufs.SrcRead(p);
	r <<= (bo % 8) + 8;
	r >>= 32 - bc;

	return r;
}


static uint8 XPK_ReadTable(int32 index)
//-------------------------------------
{
	static const uint8 xpk_table[] = {
		2,3,4,5,6,7,8,0,3,2,4,5,6,7,8,0,4,3,5,2,6,7,8,0,5,4,6,2,3,7,8,0,6,5,7,2,3,4,8,0,7,6,8,2,3,4,5,0,8,7,6,2,3,4,5,0
	};
	if(index < 0) throw XPK_error();
	if(static_cast<std::size_t>(index) >= static_cast<std::size_t>(CountOf(xpk_table))) throw XPK_error();
	return xpk_table[index];
}

static bool XPK_DoUnpack(const uint8 *src_, uint32 srcLen, std::vector<char> &unpackedData, int32 len)
//----------------------------------------------------------------------------------------------------
{
	if(len <= 0) return false;
	int32 d0,d1,d2,d3,d4,d5,d6,a2,a5;
	int32 cp, cup1, type;
	std::size_t c;
	std::size_t src;
	std::size_t dst;

	std::size_t phist = 0;
	std::size_t dstmax = len;

	unpackedData.resize(len);

	XPK_BufferBounds bufs;
	bufs.pSrcBeg = src_;
	bufs.SrcSize = srcLen;
	bufs.pDstBeg = mpt::byte_cast<uint8 *>(unpackedData.data());
	bufs.DstSize = len;

	src = 0;
	dst = 0;
	c = src;
	while (len > 0)
	{
		type = bufs.SrcRead(c+0);
		cp = (bufs.SrcRead(c+4)<<8) | (bufs.SrcRead(c+5)); // packed
		cup1 = (bufs.SrcRead(c+6)<<8) | (bufs.SrcRead(c+7)); // unpacked
		//Log("  packed=%6d unpacked=%6d bytes left=%d dst=%08X(%d)\n", cp, cup1, len, dst, dst);
		c += 8;
		src = c+2;
		if (type == 0)
		{
			// RAW chunk
			if(cp < 0) throw XPK_error();
			for(int32 i = 0; i < cp; ++i)
			{
				bufs.DstWrite(dst + i, bufs.SrcRead(c + i));
			}
			dst+=cp;
			c+=cp;
			len -= cp;
			continue;
		}

		if (type != 1)
		{
		#ifdef MMCMP_LOG
			Log("Invalid XPK type! (%d bytes left)\n", len);
		#endif
			break;
		}
		len -= cup1;
		cp = (cp + 3) & 0xfffc;
		c += cp;

		d0 = d1 = d2 = a2 = 0;
		d3 = bufs.SrcRead(src); src++;
		bufs.DstWrite(dst, (uint8)d3);
		if (dst < dstmax) dst++;
		cup1--;

		while (cup1 > 0)
		{
			if (d1 >= 8) goto l6dc;
			if (bfextu(src,d0,1,bufs)) goto l75a;
			d0 += 1;
			d5 = 0;
			d6 = 8;
			goto l734;

		l6dc:
			if (bfextu(src,d0,1,bufs)) goto l726;
			d0 += 1;
			if (! bfextu(src,d0,1,bufs)) goto l75a;
			d0 += 1;
			if (bfextu(src,d0,1,bufs)) goto l6f6;
			d6 = 2;
			goto l708;

		l6f6:
			d0 += 1;
			if (!bfextu(src,d0,1,bufs)) goto l706;
			d6 = bfextu(src,d0,3,bufs);
			d0 += 3;
			goto l70a;

		l706:
			d6 = 3;
		l708:
			d0 += 1;
		l70a:
			d6 = XPK_ReadTable((8*a2) + d6 -17);
			if (d6 != 8) goto l730;
		l718:
			if (d2 >= 20)
			{
				d5 = 1;
				goto l732;
			}
			d5 = 0;
			goto l734;

		l726:
			d0 += 1;
			d6 = 8;
			if (d6 == a2) goto l718;
			d6 = a2;
		l730:
			d5 = 4;
		l732:
			d2 += 8;
		l734:
			while ((d5 >= 0) && (cup1 > 0))
			{
				d4 = bfexts(src,d0,d6,bufs);
				d0 += d6;
				d3 -= d4;
				bufs.DstWrite(dst, (uint8)d3);
				if (dst < dstmax) dst++;
				cup1--;
				d5--;
			}
			if (d1 != 31) d1++;
			a2 = d6;
		l74c:
			d6 = d2;
			d6 >>= 3;
			d2 -= d6;
		}
	}
	unpackedData.resize(bufs.DstSize - len);
	return !unpackedData.empty();

l75a:
	d0 += 1;
	if (bfextu(src,d0,1,bufs)) goto l766;
	d4 = 2;
	goto l79e;

l766:
	d0 += 1;
	if (bfextu(src,d0,1,bufs)) goto l772;
	d4 = 4;
	goto l79e;

l772:
	d0 += 1;
	if (bfextu(src,d0,1,bufs)) goto l77e;
	d4 = 6;
	goto l79e;

l77e:
	d0 += 1;
	if (bfextu(src,d0,1,bufs)) goto l792;
	d0 += 1;
	d6 = bfextu(src,d0,3,bufs);
	d0 += 3;
	d6 += 8;
	goto l7a8;

l792:
	d0 += 1;
	d6 = bfextu(src,d0,5,bufs);
	d0 += 5;
	d4 = 16;
	goto l7a6;

l79e:
	d0 += 1;
	d6 = bfextu(src,d0,1,bufs);
	d0 += 1;
l7a6:
	d6 += d4;
l7a8:
	if (bfextu(src,d0,1,bufs)) goto l7c4;
	d0 += 1;
	if (bfextu(src,d0,1,bufs)) goto l7bc;
	d5 = 8;
	a5 = 0;
	goto l7ca;

l7bc:
	d5 = 14;
	a5 = -0x1100;
	goto l7ca;

l7c4:
	d5 = 12;
	a5 = -0x100;
l7ca:
	d0 += 1;
	d4 = bfextu(src,d0,d5,bufs);
	d0 += d5;
	d6 -= 3;
	if (d6 >= 0)
	{
		if (d6 > 0) d1 -= 1;
		d1 -= 1;
		if (d1 < 0) d1 = 0;
	}
	d6 += 2;
	phist = dst + a5 - d4 - 1;

	while ((d6 >= 0) && (cup1 > 0))
	{
		d3 = bufs.DstRead(phist); phist++;
		bufs.DstWrite(dst, (uint8)d3);
		if (dst < dstmax) dst++;
		cup1--;
		d6--;
	}
	goto l74c;
}


bool UnpackXPK(std::vector<char> &unpackedData, FileReader &file)
//---------------------------------------------------------------
{
	file.Rewind();
	unpackedData.clear();

	XPKFILEHEADER header;
	if(!file.ReadStruct(header)) return false;
	if(std::memcmp(header.XPKF, "XPKF", 4) != 0) return false;
	if(std::memcmp(header.SQSH, "SQSH", 4) != 0) return false;
	if(header.SrcLen == 0) return false;
	if(header.DstLen == 0) return false;
	STATIC_ASSERT(sizeof(XPKFILEHEADER) >= 8);
	if(header.SrcLen < (sizeof(XPKFILEHEADER) - 8)) return false;
	if(!file.CanRead(header.SrcLen - (sizeof(XPKFILEHEADER) - 8))) return false;

#ifdef MMCMP_LOG
	Log("XPK detected (SrcLen=%d DstLen=%d) filesize=%d\n", header.SrcLen, header.DstLen, file.GetLength());
#endif
	bool result = false;
	try
	{
		result = XPK_DoUnpack(file.GetRawData<uint8>(), header.SrcLen - (sizeof(XPKFILEHEADER) - 8), unpackedData, header.DstLen);
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		return false;
	} catch(const XPK_error &)
	{
		return false;
	}

	return result;
}


//////////////////////////////////////////////////////////////////////////////
//
// PowerPack PP20 Unpacker
//


struct PPBITBUFFER
{
	uint32 bitcount;
	uint32 bitbuffer;
	const uint8 *pStart;
	const uint8 *pSrc;

	uint32 GetBits(uint32 n);
};


uint32 PPBITBUFFER::GetBits(uint32 n)
//-----------------------------------
{
	uint32 result = 0;

	for (uint32 i=0; i<n; i++)
	{
		if (!bitcount)
		{
			bitcount = 8;
			if (pSrc != pStart) pSrc--;
			bitbuffer = *pSrc;
		}
		result = (result<<1) | (bitbuffer&1);
		bitbuffer >>= 1;
		bitcount--;
	}
	return result;
}


static bool PP20_DoUnpack(const uint8 *pSrc, uint32 nSrcLen, uint8 *pDst, uint32 nDstLen)
//---------------------------------------------------------------------------------------
{
	PPBITBUFFER BitBuffer;
	uint32 nBytesLeft;

	BitBuffer.pStart = pSrc;
	BitBuffer.pSrc = pSrc + nSrcLen - 4;
	BitBuffer.bitbuffer = 0;
	BitBuffer.bitcount = 0;
	BitBuffer.GetBits(pSrc[nSrcLen-1]);
	nBytesLeft = nDstLen;
	while (nBytesLeft > 0)
	{
		if (!BitBuffer.GetBits(1))
		{
			uint32 n = 1;
			while (n < nBytesLeft)
			{
				uint32 code = BitBuffer.GetBits(2);
				n += code;
				if (code != 3) break;
			}
			LimitMax(n, nBytesLeft);
			for (uint32 i=0; i<n; i++)
			{
				pDst[--nBytesLeft] = (uint8)BitBuffer.GetBits(8);
			}
			if (!nBytesLeft) break;
		}
		{
			uint32 n = BitBuffer.GetBits(2)+1;
			if(n < 1 || n-1 >= nSrcLen) return false;
			uint32 nbits = pSrc[n-1];
			uint32 nofs;
			if (n==4)
			{
				nofs = BitBuffer.GetBits( (BitBuffer.GetBits(1)) ? nbits : 7 );
				while (n < nBytesLeft)
				{
					uint32 code = BitBuffer.GetBits(3);
					n += code;
					if (code != 7) break;
				}
			} else
			{
				nofs = BitBuffer.GetBits(nbits);
			}
			LimitMax(n, nBytesLeft);
			for (uint32 i=0; i<=n; i++)
			{
				pDst[nBytesLeft-1] = (nBytesLeft+nofs < nDstLen) ? pDst[nBytesLeft+nofs] : 0;
				if (!--nBytesLeft) break;
			}
		}
	}
	return true;
}


bool UnpackPP20(std::vector<char> &unpackedData, FileReader &file)
//----------------------------------------------------------------
{
	file.Rewind();
	unpackedData.clear();

	if(!file.ReadMagic("PP20")) return false;
	if(!file.CanRead(8)) return false;
	uint8 efficiency[4];
	file.ReadArray(efficiency);
	if(efficiency[0] < 9 || efficiency[0] > 15
		|| efficiency[1] < 9 || efficiency[1] > 15
		|| efficiency[2] < 9 || efficiency[2] > 15
		|| efficiency[3] < 9 || efficiency[3] > 15)
		return false;
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
	return PP20_DoUnpack(file.GetRawData<uint8>(), static_cast<uint32>(length - 4), mpt::byte_cast<uint8 *>(unpackedData.data()), dstLen);
}


OPENMPT_NAMESPACE_END
