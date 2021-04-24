/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

inline constexpr int32_t FIRST_VISIBLE_LINE = 1;

enum
{
	TILE_2BIT,
	TILE_4BIT,
	TILE_8BIT,
	TILE_2BIT_EVEN,
	TILE_2BIT_ODD,
	TILE_4BIT_EVEN,
	TILE_4BIT_ODD
};

inline constexpr size_t MAX_2BIT_TILES = 4096;
inline constexpr size_t MAX_4BIT_TILES = 2048;
inline constexpr size_t MAX_8BIT_TILES = 1024;

struct InternalPPU
{
	bool Interlace;
	uint32_t Red[256];
	uint32_t Green[256];
	uint32_t Blue[256];
};

struct SPPU
{
	struct
	{
		bool High;
		uint8_t Increment;
		uint16_t Address;
		uint16_t Mask1;
		uint16_t FullGraphicCount;
		uint16_t Shift;
	} VMA;

	uint32_t WRAM;

	bool CGFLIPRead;
	uint8_t CGADD;
	uint16_t CGDATA[256];

	uint16_t OAMAddr;
	uint16_t SavedOAMAddr;
	bool OAMPriorityRotation;
	uint8_t OAMFlip;
	uint16_t OAMWriteRegister;
	uint8_t OAMData[512 + 32];

	uint8_t FirstSprite;

	bool HTimerEnabled;
	bool VTimerEnabled;
	short HTimerPosition;
	short VTimerPosition;
	uint16_t IRQHBeamPos;
	uint16_t IRQVBeamPos;

	bool HBeamFlip;
	bool VBeamFlip;

	short MatrixA;
	short MatrixB;

	bool ForcedBlanking;

	uint16_t ScreenHeight;

	bool Need16x8Mulitply;
	uint8_t M7byte;

	uint8_t HDMA;
	uint8_t HDMAEnded;

	uint8_t OpenBus1;
	uint8_t OpenBus2;

	uint16_t VRAMReadBuffer;
};

extern uint16_t SignExtend[2];
extern SPPU PPU;
extern InternalPPU IPPU;

void S9xResetPPU();
void S9xSoftResetPPU();
void S9xSetPPU(uint8_t, uint16_t);
uint8_t S9xGetPPU(uint16_t);
void S9xSetCPU(uint8_t, uint16_t);
uint8_t S9xGetCPU(uint16_t);
void S9xUpdateIRQPositions(bool);

#include "memmap.h"

struct SnesModel
{
	uint8_t _5C77;
	uint8_t _5C78;
	uint8_t _5A22;
};

extern SnesModel *Model;
extern SnesModel M1SNES;

inline void S9xUpdateVRAMReadBuffer()
{
	if (PPU.VMA.FullGraphicCount)
	{
		uint32_t addr = PPU.VMA.Address;
		uint32_t rem = addr & PPU.VMA.Mask1;
		uint32_t address = (addr & ~PPU.VMA.Mask1) + (rem >> PPU.VMA.Shift) + ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3);
		PPU.VRAMReadBuffer = READ_WORD(&Memory.VRAM[(address << 1) & 0xffff]);
	}
	else
		PPU.VRAMReadBuffer = READ_WORD(&Memory.VRAM[(PPU.VMA.Address << 1) & 0xffff]);
}

inline void REGISTER_2104(uint8_t Byt)
{
	if (!(PPU.OAMFlip & 1))
	{
		PPU.OAMWriteRegister &= 0xff00;
		PPU.OAMWriteRegister |= Byt;
	}

	if (PPU.OAMAddr & 0x100)
	{
		int addr = ((PPU.OAMAddr & 0x10f) << 1) + (PPU.OAMFlip & 1);
		if (Byt != PPU.OAMData[addr])
			PPU.OAMData[addr] = Byt;
	}
	else if (PPU.OAMFlip & 1)
	{
		PPU.OAMWriteRegister &= 0x00ff;
		uint8_t lowbyte = static_cast<uint8_t>(PPU.OAMWriteRegister);
		uint8_t highbyte = Byt;
		PPU.OAMWriteRegister |= Byt << 8;

		int addr = PPU.OAMAddr << 1;
		if (lowbyte != PPU.OAMData[addr] || highbyte != PPU.OAMData[addr + 1])
		{
			PPU.OAMData[addr] = lowbyte;
			PPU.OAMData[addr + 1] = highbyte;
		}
	}

	PPU.OAMFlip ^= 1;
	if (!(PPU.OAMFlip & 1))
	{
		++PPU.OAMAddr;
		PPU.OAMAddr &= 0x1ff;
		if (PPU.OAMPriorityRotation && PPU.FirstSprite != (PPU.OAMAddr >> 1))
			PPU.FirstSprite = (PPU.OAMAddr & 0xfe) >> 1;
	}
}

// This code is correct, however due to Snes9x's inaccurate timings, some games might be broken by this chage. :(
inline bool CHECK_INBLANK()
{
	if (Settings.BlockInvalidVRAMAccess && !PPU.ForcedBlanking && CPU.V_Counter < PPU.ScreenHeight + FIRST_VISIBLE_LINE)
	{
		PPU.VMA.Address += !PPU.VMA.High ? PPU.VMA.Increment : 0;
		return true;
	}
	else
		return false;
}

inline void REGISTER_2118(uint8_t Byt)
{
	if (CHECK_INBLANK())
		return;

	uint32_t address;

	if (PPU.VMA.FullGraphicCount)
	{
		uint32_t rem = PPU.VMA.Address & PPU.VMA.Mask1;
		address = (((PPU.VMA.Address & ~PPU.VMA.Mask1) + (rem >> PPU.VMA.Shift) + ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) & 0xffff;
		Memory.VRAM[address] = Byt;
	}
	else
		Memory.VRAM[address = (PPU.VMA.Address << 1) & 0xffff] = Byt;

	if (!PPU.VMA.High)
		PPU.VMA.Address += PPU.VMA.Increment;
}

inline void REGISTER_2118_tile(uint8_t Byt)
{
	if (CHECK_INBLANK())
		return;

	uint32_t rem = PPU.VMA.Address & PPU.VMA.Mask1;
	uint32_t address = (((PPU.VMA.Address & ~PPU.VMA.Mask1) + (rem >> PPU.VMA.Shift) + ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) & 0xffff;

	Memory.VRAM[address] = Byt;

	if (!PPU.VMA.High)
		PPU.VMA.Address += PPU.VMA.Increment;
}

inline void REGISTER_2118_linear(uint8_t Byt)
{
	if (CHECK_INBLANK())
		return;

	uint32_t address;

	Memory.VRAM[address = (PPU.VMA.Address << 1) & 0xffff] = Byt;

	if (!PPU.VMA.High)
		PPU.VMA.Address += PPU.VMA.Increment;
}

inline void REGISTER_2119(uint8_t Byt)
{
	if (CHECK_INBLANK())
		return;

	uint32_t address;

	if (PPU.VMA.FullGraphicCount)
	{
		uint32_t rem = PPU.VMA.Address & PPU.VMA.Mask1;
		address = ((((PPU.VMA.Address & ~PPU.VMA.Mask1) + (rem >> PPU.VMA.Shift) + ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) + 1) & 0xffff;
		Memory.VRAM[address] = Byt;
	}
	else
		Memory.VRAM[address = ((PPU.VMA.Address << 1) + 1) & 0xffff] = Byt;

	if (PPU.VMA.High)
		PPU.VMA.Address += PPU.VMA.Increment;
}

inline void REGISTER_2119_tile(uint8_t Byt)
{
	if (CHECK_INBLANK())
		return;

	uint32_t rem = PPU.VMA.Address & PPU.VMA.Mask1;
	uint32_t address = ((((PPU.VMA.Address & ~PPU.VMA.Mask1) + (rem >> PPU.VMA.Shift) + ((rem & (PPU.VMA.FullGraphicCount - 1)) << 3)) << 1) + 1) & 0xffff;

	Memory.VRAM[address] = Byt;

	if (PPU.VMA.High)
		PPU.VMA.Address += PPU.VMA.Increment;
}

inline void REGISTER_2119_linear(uint8_t Byt)
{
	if (CHECK_INBLANK())
		return;

	uint32_t address;

	Memory.VRAM[address = ((PPU.VMA.Address << 1) + 1) & 0xffff] = Byt;

	if (PPU.VMA.High)
		PPU.VMA.Address += PPU.VMA.Increment;
}

inline void REGISTER_2180(uint8_t Byt)
{
	Memory.RAM[PPU.WRAM++] = Byt;
	PPU.WRAM &= 0x1ffff;
}

inline uint8_t REGISTER_4212()
{
	uint8_t byte = 0;

	if (CPU.V_Counter >= PPU.ScreenHeight + FIRST_VISIBLE_LINE && CPU.V_Counter < PPU.ScreenHeight + FIRST_VISIBLE_LINE + 3)
		byte = 1;
	if (CPU.Cycles < Timings.HBlankEnd || CPU.Cycles >= Timings.HBlankStart)
		byte |= 0x40;
	if (CPU.V_Counter >= PPU.ScreenHeight + FIRST_VISIBLE_LINE)
		byte |= 0x80;

	return byte;
}
