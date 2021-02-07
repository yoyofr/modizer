#include <cmath>
#include <cstring>
#include "GBA.h"
#include "bios.h"
#include "GBAinline.h"
#include "Globals.h"
#include "XSFCommon.h"

static int32_t sineTable[] =
{
	0x0000, 0x0192, 0x0323, 0x04B5, 0x0645, 0x07D5, 0x0964, 0x0AF1,
	0x0C7C, 0x0E05, 0x0F8C, 0x1111, 0x1294, 0x1413, 0x158F, 0x1708,
	0x187D, 0x19EF, 0x1B5D, 0x1CC6, 0x1E2B, 0x1F8B, 0x20E7, 0x223D,
	0x238E, 0x24DA, 0x261F, 0x275F, 0x2899, 0x29CD, 0x2AFA, 0x2C21,
	0x2D41, 0x2E5A, 0x2F6B, 0x3076, 0x3179, 0x3274, 0x3367, 0x3453,
	0x3536, 0x3612, 0x36E5, 0x37AF, 0x3871, 0x392A, 0x39DA, 0x3A82,
	0x3B20, 0x3BB6, 0x3C42, 0x3CC5, 0x3D3E, 0x3DAE, 0x3E14, 0x3E71,
	0x3EC5, 0x3F0E, 0x3F4E, 0x3F84, 0x3FB1, 0x3FD3, 0x3FEC, 0x3FFB,
	0x4000, 0x3FFB, 0x3FEC, 0x3FD3, 0x3FB1, 0x3F84, 0x3F4E, 0x3F0E,
	0x3EC5, 0x3E71, 0x3E14, 0x3DAE, 0x3D3E, 0x3CC5, 0x3C42, 0x3BB6,
	0x3B20, 0x3A82, 0x39DA, 0x392A, 0x3871, 0x37AF, 0x36E5, 0x3612,
	0x3536, 0x3453, 0x3367, 0x3274, 0x3179, 0x3076, 0x2F6B, 0x2E5A,
	0x2D41, 0x2C21, 0x2AFA, 0x29CD, 0x2899, 0x275F, 0x261F, 0x24DA,
	0x238E, 0x223D, 0x20E7, 0x1F8B, 0x1E2B, 0x1CC6, 0x1B5D, 0x19EF,
	0x187D, 0x1708, 0x158F, 0x1413, 0x1294, 0x1111, 0x0F8C, 0x0E05,
	0x0C7C, 0x0AF1, 0x0964, 0x07D5, 0x0645, 0x04B5, 0x0323, 0x0192,
	0x0000, 0xFE6E, 0xFCDD, 0xFB4B, 0xF9BB, 0xF82B, 0xF69C, 0xF50F,
	0xF384, 0xF1FB, 0xF074, 0xEEEF, 0xED6C, 0xEBED, 0xEA71, 0xE8F8,
	0xE783, 0xE611, 0xE4A3, 0xE33A, 0xE1D5, 0xE075, 0xDF19, 0xDDC3,
	0xDC72, 0xDB26, 0xD9E1, 0xD8A1, 0xD767, 0xD633, 0xD506, 0xD3DF,
	0xD2BF, 0xD1A6, 0xD095, 0xCF8A, 0xCE87, 0xCD8C, 0xCC99, 0xCBAD,
	0xCACA, 0xC9EE, 0xC91B, 0xC851, 0xC78F, 0xC6D6, 0xC626, 0xC57E,
	0xC4E0, 0xC44A, 0xC3BE, 0xC33B, 0xC2C2, 0xC252, 0xC1EC, 0xC18F,
	0xC13B, 0xC0F2, 0xC0B2, 0xC07C, 0xC04F, 0xC02D, 0xC014, 0xC005,
	0xC000, 0xC005, 0xC014, 0xC02D, 0xC04F, 0xC07C, 0xC0B2, 0xC0F2,
	0xC13B, 0xC18F, 0xC1EC, 0xC252, 0xC2C2, 0xC33B, 0xC3BE, 0xC44A,
	0xC4E0, 0xC57E, 0xC626, 0xC6D6, 0xC78F, 0xC851, 0xC91B, 0xC9EE,
	0xCACA, 0xCBAD, 0xCC99, 0xCD8C, 0xCE87, 0xCF8A, 0xD095, 0xD1A6,
	0xD2BF, 0xD3DF, 0xD506, 0xD633, 0xD767, 0xD8A1, 0xD9E1, 0xDB26,
	0xDC72, 0xDDC3, 0xDF19, 0xE075, 0xE1D5, 0xE33A, 0xE4A3, 0xE611,
	0xE783, 0xE8F8, 0xEA71, 0xEBED, 0xED6C, 0xEEEF, 0xF074, 0xF1FB,
	0xF384, 0xF50F, 0xF69C, 0xF82B, 0xF9BB, 0xFB4B, 0xFCDD, 0xFE6E
};

void BIOS_ArcTan()
{
	int32_t a =  -(static_cast<int32_t>(reg[0].I * reg[0].I) >> 14);
	int32_t b = ((0xA9 * a) >> 14) + 0x390;
	b = ((b * a) >> 14) + 0x91C;
	b = ((b * a) >> 14) + 0xFB6;
	b = ((b * a) >> 14) + 0x16AA;
	b = ((b * a) >> 14) + 0x2081;
	b = ((b * a) >> 14) + 0x3651;
	b = ((b * a) >> 14) + 0xA2F9;
	a = (static_cast<int32_t>(reg[0].I) * b) >> 16;
	reg[0].I = a;
}

void BIOS_ArcTan2()
{
	int32_t x = reg[0].I;
	int32_t y = reg[1].I;
	uint32_t res = 0;
	if (!y)
		res = (x >> 16) & 0x8000;
	else if (!x)
		res = ((y >> 16) & 0x8000) + 0x4000;
	else if (std::abs(x) > std::abs(y) || (fEqual(std::abs(x), std::abs(y)) && !(x < 0 && y < 0)))
	{
		reg[1].I = x;
		reg[0].I = y << 14;
		BIOS_Div();
		BIOS_ArcTan();
		if (x < 0)
			res = 0x8000 + reg[0].I;
		else
			res = (((y >> 16) & 0x8000) << 1) + reg[0].I;
	}
	else
	{
		reg[0].I = x << 14;
		BIOS_Div();
		BIOS_ArcTan();
		res = (0x4000 + ((y >> 16) & 0x8000)) - reg[0].I;
	}
	reg[0].I = res;
}

void BIOS_BitUnPack()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;
	uint32_t header = reg[2].I;

	int len = CPUReadHalfWord(header);
	// check address
	if (!(source & 0xe000000) || !((source + len) & 0xe000000))
		return;

	int bits = CPUReadByte(header + 2);
	int revbits = 8 - bits;
	//uint32_t value = 0;
	uint32_t base = CPUReadMemory(header + 4);
	bool addBase = !!(base & 0x80000000);
	base &= 0x7fffffff;
	int dataSize = CPUReadByte(header + 3);

	int data = 0;
	int bitwritecount = 0;
	while (1)
	{
		--len;
		if (len < 0)
			break;
		int mask = 0xff >> revbits;
		uint8_t b = CPUReadByte(source++);
		int bitcount = 0;
		while (1)
		{
			if (bitcount >= 8)
				break;
			uint32_t d = b & mask;
			uint32_t temp = d >> bitcount;
			if (d || addBase)
				temp += base;
			data |= temp << bitwritecount;
			bitwritecount += dataSize;
			if (bitwritecount >= 32)
			{
				CPUWriteMemory(dest, data);
				dest += 4;
				data = 0;
				bitwritecount = 0;
			}
			mask <<= bits;
			bitcount += bits;
		}
	}
}

void BIOS_GetBiosChecksum()
{
	reg[0].I = 0xBAAE187F;
}

void BIOS_BgAffineSet()
{
	uint32_t src = reg[0].I;
	uint32_t dest = reg[1].I;
	int num = reg[2].I;

	for (int i = 0; i < num; ++i)
	{
		int32_t cx = CPUReadMemory(src);
		src += 4;
		int32_t cy = CPUReadMemory(src);
		src += 4;
		int16_t dispx = CPUReadHalfWord(src);
		src += 2;
		int16_t dispy = CPUReadHalfWord(src);
		src += 2;
		int16_t rx = CPUReadHalfWord(src);
		src += 2;
		int16_t ry = CPUReadHalfWord(src);
		src += 2;
		uint16_t theta = CPUReadHalfWord(src) >> 8;
		src += 4; // keep structure alignment
		int32_t a = sineTable[(theta + 0x40) & 255];
		int32_t b = sineTable[theta];

		int16_t dx = (rx * a) >> 14;
		int16_t dmx = (rx * b) >> 14;
		int16_t dy = (ry * b) >> 14;
		int16_t dmy = (ry * a) >> 14;

		CPUWriteHalfWord(dest, dx);
		dest += 2;
		CPUWriteHalfWord(dest, -dmx);
		dest += 2;
		CPUWriteHalfWord(dest, dy);
		dest += 2;
		CPUWriteHalfWord(dest, dmy);
		dest += 2;

		int32_t startx = cx - dx * dispx + dmx * dispy;
		int32_t starty = cy - dy * dispx - dmy * dispy;

		CPUWriteMemory(dest, startx);
		dest += 4;
		CPUWriteMemory(dest, starty);
		dest += 4;
	}
}

void BIOS_CpuSet()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;
	uint32_t cnt = reg[2].I;

	if (!(source & 0xe000000) || !((source + (((cnt << 11) >> 9) & 0x1fffff)) & 0xe000000))
		return;

	int count = cnt & 0x1FFFFF;

	// 32-bit ?
	if ((cnt >> 26) & 1)
	{
		// needed for 32-bit mode!
		source &= 0xFFFFFFFC;
		dest &= 0xFFFFFFFC;
		// fill ?
		if ((cnt >> 24) & 1)
		{
			uint32_t value = source > 0x0EFFFFFF ? 0x1CAD1CAD : CPUReadMemory(source);
			while (count)
			{
				CPUWriteMemory(dest, value);
				dest += 4;
				--count;
			}
		}
		else
		{
			// copy
			while (count)
			{
				CPUWriteMemory(dest, source > 0x0EFFFFFF ? 0x1CAD1CAD : CPUReadMemory(source));
				source += 4;
				dest += 4;
				--count;
			}
		}
	}
	else
	{
		// 16-bit fill?
		if ((cnt >> 24) & 1)
		{
			uint16_t value = source > 0x0EFFFFFF ? 0x1CAD : CPUReadHalfWord(source);
			while (count)
			{
				CPUWriteHalfWord(dest, value);
				dest += 2;
				--count;
			}
		}
		else
		{
			// copy
			while (count)
			{
				CPUWriteHalfWord(dest, source > 0x0EFFFFFF ? 0x1CAD : CPUReadHalfWord(source));
				source += 2;
				dest += 2;
				--count;
			}
		}
	}
}

void BIOS_CpuFastSet()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;
	uint32_t cnt = reg[2].I;

	if (!(source & 0xe000000) || !((source + (((cnt << 11) >> 9) & 0x1fffff)) & 0xe000000))
		return;

	// needed for 32-bit mode!
	source &= 0xFFFFFFFC;
	dest &= 0xFFFFFFFC;

	int count = cnt & 0x1FFFFF;

	// fill?
	if ((cnt >> 24) & 1)
	{
		while (count > 0)
		{
			// BIOS always transfers 32 bytes at a time
			uint32_t value = source > 0x0EFFFFFF ? 0xBAFFFFFB : CPUReadMemory(source);
			for (int i = 0; i < 8; ++i)
			{
				CPUWriteMemory(dest, value);
				dest += 4;
			}
			count -= 8;
		}
	}
	else
	{
		// copy
		while (count > 0)
		{
			// BIOS always transfers 32 bytes at a time
			for (int i = 0; i < 8; ++i)
			{
				CPUWriteMemory(dest, source > 0x0EFFFFFF ? 0xBAFFFFFB :CPUReadMemory(source));
				source += 4;
				dest += 4;
			}
			count -= 8;
		}
	}
}

void BIOS_Diff8bitUnFilterWram()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	int len = header >> 8;

	uint8_t data = CPUReadByte(source++);
	CPUWriteByte(dest++, data);
	--len;

	while (len > 0)
	{
		uint8_t diff = CPUReadByte(source++);
		data += diff;
		CPUWriteByte(dest++, data);
		--len;
	}
}

void BIOS_Diff8bitUnFilterVram()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	int len = header >> 8;

	uint8_t data = CPUReadByte(source++);
	uint16_t writeData = data;
	int shift = 8;
	int bytes = 1;

	while (len >= 2)
	{
		uint8_t diff = CPUReadByte(source++);
		data += diff;
		writeData |= data << shift;
		++bytes;
		shift += 8;
		if (bytes == 2)
		{
			CPUWriteHalfWord(dest, writeData);
			dest += 2;
			len -= 2;
			bytes = 0;
			writeData = 0;
			shift = 0;
		}
	}
}

void BIOS_Diff16bitUnFilter()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	int len = header >> 8;

	uint16_t data = CPUReadHalfWord(source);
	source += 2;
	CPUWriteHalfWord(dest, data);
	dest += 2;
	len -= 2;

	while (len >= 2)
	{
		uint16_t diff = CPUReadHalfWord(source);
		source += 2;
		data += diff;
		CPUWriteHalfWord(dest, data);
		dest += 2;
		len -= 2;
	}
}

void BIOS_Div()
{
	int number = reg[0].I;
	int denom = reg[1].I;

	if (denom)
	{
		reg[0].I = number / denom;
		reg[1].I = number % denom;
		int32_t temp = static_cast<int32_t>(reg[0].I);
		reg[3].I = static_cast<uint32_t>(temp < 0 ? -temp : temp);
	}
}

void BIOS_HuffUnComp()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	uint8_t treeSize = CPUReadByte(source++);

	uint32_t treeStart = source;

	source += ((treeSize + 1) << 1) - 1; // minus because we already skipped one byte

	int len = header >> 8;

	uint32_t mask = 0x80000000;
	uint32_t data = CPUReadMemory(source);
	source += 4;

	int pos = 0;
	uint8_t rootNode = CPUReadByte(treeStart);
	uint8_t currentNode = rootNode;
	bool writeData = false;
	int byteShift = 0;
	int byteCount = 0;
	uint32_t writeValue = 0;

	if ((header & 0x0F) == 8)
	{
		while (len > 0)
		{
			// take left
			if (!pos)
				++pos;
			else
				pos += ((currentNode & 0x3F) + 1) << 1;

			if (data & mask)
			{
				// right
				if (currentNode & 0x40)
					writeData = true;
				currentNode = CPUReadByte(treeStart + pos + 1);
			}
			else
			{
				// left
				if (currentNode & 0x80)
					writeData = true;
				currentNode = CPUReadByte(treeStart + pos);
			}

			if (writeData)
			{
				writeValue |= currentNode << byteShift;
				++byteCount;
				byteShift += 8;

				pos = 0;
				currentNode = rootNode;
				writeData = false;

				if (byteCount == 4)
				{
					byteCount = 0;
					byteShift = 0;
					CPUWriteMemory(dest, writeValue);
					writeValue = 0;
					dest += 4;
					len -= 4;
				}
			}
			mask >>= 1;
			if (!mask)
			{
				mask = 0x80000000;
				data = CPUReadMemory(source);
				source += 4;
			}
		}
	}
	else
	{
		int halfLen = 0;
		int value = 0;
		while (len > 0)
		{
			// take left
			if (!pos)
				++pos;
			else
				pos += ((currentNode & 0x3F) + 1) << 1;

			if (data & mask)
			{
				// right
				if (currentNode & 0x40)
					writeData = true;
				currentNode = CPUReadByte(treeStart + pos + 1);
			}
			else
			{
				// left
				if (currentNode & 0x80)
					writeData = true;
				currentNode = CPUReadByte(treeStart + pos);
			}

			if (writeData)
			{
				if (!halfLen)
					value |= currentNode;
				else
					value |= currentNode << 4;

				halfLen += 4;
				if (halfLen == 8)
				{
					writeValue |= value << byteShift;
					++byteCount;
					byteShift += 8;

					halfLen = 0;
					value = 0;

					if (byteCount == 4)
					{
						byteCount = 0;
						byteShift = 0;
						CPUWriteMemory(dest, writeValue);
						dest += 4;
						writeValue = 0;
						len -= 4;
					}
				}
				pos = 0;
				currentNode = rootNode;
				writeData = false;
			}
			mask >>= 1;
			if (!mask)
			{
				mask = 0x80000000;
				data = CPUReadMemory(source);
				source += 4;
			}
		}
	}
}

void BIOS_LZ77UnCompVram()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	int byteCount = 0;
	int byteShift = 0;
	uint32_t writeValue = 0;

	int len = header >> 8;

	while (len > 0)
	{
		uint8_t d = CPUReadByte(source++);

		if (d)
		{
			for (int i = 0; i < 8; ++i)
			{
				if (d & 0x80)
				{
					uint16_t data = CPUReadByte(source++) << 8;
					data |= CPUReadByte(source++);
					int length = (data >> 12) + 3;
					int offset = data & 0x0FFF;
					uint32_t windowOffset = dest + byteCount - offset - 1;
					for (int i2 = 0; i2 < length; ++i2)
					{
						writeValue |= CPUReadByte(windowOffset++) << byteShift;
						byteShift += 8;
						++byteCount;

						if (byteCount == 2)
						{
							CPUWriteHalfWord(dest, writeValue);
							dest += 2;
							byteCount = 0;
							byteShift = 0;
							writeValue = 0;
						}
						--len;
						if (!len)
							return;
					}
				}
				else
				{
					writeValue |= CPUReadByte(source++) << byteShift;
					byteShift += 8;
					++byteCount;

					if (byteCount == 2)
					{
						CPUWriteHalfWord(dest, writeValue);
						dest += 2;
						byteCount = 0;
						byteShift = 0;
						writeValue = 0;
					}
					--len;
					if (!len)
						return;
				}
				d <<= 1;
			}
		}
		else
		{
			for (int i = 0; i < 8; ++i)
			{
				writeValue |= CPUReadByte(source++) << byteShift;
				byteShift += 8;
				++byteCount;
				if (byteCount == 2)
				{
					CPUWriteHalfWord(dest, writeValue);
					dest += 2;
					byteShift = 0;
					byteCount = 0;
					writeValue = 0;
				}
				--len;
				if (!len)
					return;
			}
		}
	}
}

void BIOS_LZ77UnCompWram()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	int len = header >> 8;

	while (len > 0)
	{
		uint8_t d = CPUReadByte(source++);

		if (d)
		{
			for (int i = 0; i < 8; ++i)
			{
				if (d & 0x80)
				{
					uint16_t data = CPUReadByte(source++) << 8;
					data |= CPUReadByte(source++);
					int length = (data >> 12) + 3;
					int offset = data & 0x0FFF;
					uint32_t windowOffset = dest - offset - 1;
					for (int i2 = 0; i2 < length; ++i2)
					{
						CPUWriteByte(dest++, CPUReadByte(windowOffset++));
						--len;
						if (!len)
							return;
					}
				}
				else
				{
					CPUWriteByte(dest++, CPUReadByte(source++));
					--len;
					if (!len)
						return;
				}
				d <<= 1;
			}
		}
		else
		{
			for (int i = 0; i < 8; ++i)
			{
				CPUWriteByte(dest++, CPUReadByte(source++));
				--len;
				if (!len)
					return;
			}
		}
	}
}

void BIOS_ObjAffineSet()
{
	uint32_t src = reg[0].I;
	uint32_t dest = reg[1].I;
	int num = reg[2].I;
	int offset = reg[3].I;

	for (int i = 0; i < num; ++i)
	{
		int16_t rx = CPUReadHalfWord(src);
		src += 2;
		int16_t ry = CPUReadHalfWord(src);
		src += 2;
		uint16_t theta = CPUReadHalfWord(src) >> 8;
		src += 4; // keep structure alignment

		int32_t a = sineTable[(theta + 0x40) & 255];
		int32_t b = sineTable[theta];

		int16_t dx = (static_cast<int32_t>(rx) * a) >> 14;
		int16_t dmx = (static_cast<int32_t>(rx) * b) >> 14;
		int16_t dy = (static_cast<int32_t>(ry) * b) >> 14;
		int16_t dmy = (static_cast<int32_t>(ry) * a) >> 14;

		CPUWriteHalfWord(dest, dx);
		dest += offset;
		CPUWriteHalfWord(dest, -dmx);
		dest += offset;
		CPUWriteHalfWord(dest, dy);
		dest += offset;
		CPUWriteHalfWord(dest, dmy);
		dest += offset;
	}
}

void BIOS_RegisterRamReset(uint32_t flags)
{
	// no need to trace here. this is only called directly from GBA.cpp
	// to emulate bios initialization

	CPUUpdateRegister(0x0, 0x80);

	if (flags)
	{
		if (flags & 0x01)
			// clear work RAM
			memset(&workRAM[0], 0, 0x40000);
		if (flags & 0x02)
			// clear internal RAM
			memset(&internalRAM[0], 0, 0x7e00); // don't clear 0x7e00-0x7fff
		if (flags & 0x04)
			// clear palette RAM
			memset(&paletteRAM[0], 0, 0x400);
		if (flags & 0x08)
			// clear VRAM
			memset(&vram[0], 0, 0x18000);
		if (flags & 0x10)
			// clean OAM
			memset(&oam[0], 0, 0x400);

		if (flags & 0x80)
		{
			int i;
			for (i = 0; i < 0x10; ++i)
				CPUUpdateRegister(0x200 + i * 2, 0);

			for (i = 0; i < 0xF; ++i)
				CPUUpdateRegister(0x4 + i * 2, 0);

			for (i = 0; i < 0x20; ++i)
				CPUUpdateRegister(0x20 + i * 2, 0);

			for (i = 0; i < 0x18; ++i)
				CPUUpdateRegister(0xb0 + i * 2, 0);

			CPUUpdateRegister(0x130, 0);
			CPUUpdateRegister(0x20, 0x100);
			CPUUpdateRegister(0x30, 0x100);
			CPUUpdateRegister(0x26, 0x100);
			CPUUpdateRegister(0x36, 0x100);
		}

		if (flags & 0x20)
		{
			int i;
			for (i = 0; i < 8; ++i)
				CPUUpdateRegister(0x110 + i * 2, 0);
			CPUUpdateRegister(0x134, 0x8000);
			for (i = 0; i < 7; ++i)
				CPUUpdateRegister(0x140 + i * 2, 0);
		}

		if (flags & 0x40)
		{
			CPUWriteByte(0x4000084, 0);
			CPUWriteByte(0x4000084, 0x80);
			CPUWriteMemory(0x4000080, 0x880e0000);
			CPUUpdateRegister(0x88, CPUReadHalfWord(0x4000088) & 0x3ff);
			CPUWriteByte(0x4000070, 0x70);
			int i;
			for (i = 0; i < 8; ++i)
				CPUUpdateRegister(0x90 + i * 2, 0);
			CPUWriteByte(0x4000070, 0);
			for (i = 0; i < 8; ++i)
				CPUUpdateRegister(0x90 + i * 2, 0);
			CPUWriteByte(0x4000084, 0);
		}
	}
}

void BIOS_RegisterRamReset()
{
	BIOS_RegisterRamReset(reg[0].I);
}

void BIOS_RLUnCompVram()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source & 0xFFFFFFFC);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	int len = header >> 8;
	int byteCount = 0;
	int byteShift = 0;
	uint32_t writeValue = 0;

	while (len > 0)
	{
		uint8_t d = CPUReadByte(source++);
		int l = d & 0x7F;
		if (d & 0x80)
		{
			uint8_t data = CPUReadByte(source++);
			l += 3;
			for (int i = 0;i < l; ++i)
			{
				writeValue |= data << byteShift;
				byteShift += 8;
				++byteCount;

				if (byteCount == 2)
				{
					CPUWriteHalfWord(dest, writeValue);
					dest += 2;
					byteCount = 0;
					byteShift = 0;
					writeValue = 0;
				}
				--len;
				if (!len)
					return;
			}
		}
		else
		{
			++l;
			for (int i = 0; i < l; ++i)
			{
				writeValue |= CPUReadByte(source++) << byteShift;
				byteShift += 8;
				++byteCount;
				if (byteCount == 2)
				{
					CPUWriteHalfWord(dest, writeValue);
					dest += 2;
					byteCount = 0;
					byteShift = 0;
					writeValue = 0;
				}
				--len;
				if (!len)
					return;
			}
		}
	}
}

void BIOS_RLUnCompWram()
{
	uint32_t source = reg[0].I;
	uint32_t dest = reg[1].I;

	uint32_t header = CPUReadMemory(source & 0xFFFFFFFC);
	source += 4;

	if (!(source & 0xe000000) || !((source + ((header >> 8) & 0x1fffff)) & 0xe000000))
		return;

	int len = header >> 8;

	while (len > 0)
	{
		uint8_t d = CPUReadByte(source++);
		int l = d & 0x7F;
		if (d & 0x80)
		{
			uint8_t data = CPUReadByte(source++);
			l += 3;
			for (int i = 0; i < l; ++i)
			{
				CPUWriteByte(dest++, data);
				--len;
				if (!len)
					return;
			}
		}
		else
		{
			++l;
			for (int i = 0; i < l; ++i)
			{
				CPUWriteByte(dest++, CPUReadByte(source++));
				--len;
				if (!len)
					return;
			}
		}
	}
}

void BIOS_SoftReset()
{
	armState = true;
	armMode = 0x1F;
	armIrqEnable = false;
	C_FLAG = V_FLAG = N_FLAG = Z_FLAG = false;
	reg[13].I = 0x03007F00;
	reg[14].I = 0x00000000;
	reg[16].I = 0x00000000;
	reg[R13_IRQ].I = 0x03007FA0;
	reg[R14_IRQ].I = 0x00000000;
	reg[SPSR_IRQ].I = 0x00000000;
	reg[R13_SVC].I = 0x03007FE0;
	reg[R14_SVC].I = 0x00000000;
	reg[SPSR_SVC].I = 0x00000000;
	uint8_t b = internalRAM[0x7ffa];

	memset(&internalRAM[0x7e00], 0, 0x200);

	if (b)
	{
		armNextPC = 0x02000000;
		reg[15].I = 0x02000004;
	}
	else
	{
		armNextPC = 0x08000000;
		reg[15].I = 0x08000004;
	}
}

void BIOS_Sqrt()
{
	reg[0].I = static_cast<uint32_t>(std::sqrt(static_cast<double>(reg[0].I)));
}

void BIOS_MidiKey2Freq()
{
	int freq = CPUReadMemory(reg[0].I + 4);
	double tmp = (180.0 - reg[1].I) - (reg[2].I / 256.0);
	tmp = std::pow(2.0, tmp / 12.0);
	reg[0].I = static_cast<int>(freq / tmp);
}

void BIOS_SndDriverJmpTableCopy()
{
	for (int i = 0; i < 0x24; ++i)
	{
		CPUWriteMemory(reg[0].I, 0x9c);
		reg[0].I += 4;
	}
}
