/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include "cpuexec.h"

inline void addCyclesInMemoryAccess(int32_t speed)
{
	if (!CPU.InDMAorHDMA)
	{
		CPU.Cycles += speed;
		while (CPU.Cycles >= CPU.NextEvent)
			S9xDoHEventProcessing();
	}
}

inline void addCyclesInMemoryAccess_x2(int32_t speed)
{
	if (!CPU.InDMAorHDMA)
	{
		CPU.Cycles += speed << 1;
		while (CPU.Cycles >= CPU.NextEvent)
			S9xDoHEventProcessing();
	}
}

extern uint8_t OpenBus;

inline int32_t memory_speed(uint32_t address)
{
	if (address & 0x408000)
	{
		if (address & 0x800000)
			return CPU.FastROMSpeed;

		return SLOW_ONE_CYCLE;
	}

	if ((address + 0x6000) & 0x4000)
		return SLOW_ONE_CYCLE;

	if ((address - 0x4000) & 0x7e00)
		return ONE_CYCLE;

	return TWO_CYCLES;
}

inline uint8_t S9xGetByte(uint32_t Address)
{
	int block = (Address & 0xffffff) >> MEMMAP_SHIFT;
	uint8_t *GetAddress = Memory.Map[block];
	int32_t speed = memory_speed(Address);
	uint8_t byte;

	if (GetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
	{
		byte = GetAddress[Address & 0xffff];
		addCyclesInMemoryAccess(speed);
		return byte;
	}

	switch (reinterpret_cast<intptr_t>(GetAddress))
	{
		case CMemory::MAP_CPU:
			byte = S9xGetCPU(Address & 0xffff);
			addCyclesInMemoryAccess(speed);
			return byte;

		case CMemory::MAP_PPU:
			if (CPU.InDMAorHDMA && (Address & 0xff00) == 0x2100)
				return OpenBus;

			byte = S9xGetPPU(Address & 0xffff);
			addCyclesInMemoryAccess(speed);
			return byte;

		case CMemory::MAP_LOROM_SRAM:
		case CMemory::MAP_SA1RAM:
			// Address & 0x7fff   : offset into bank
			// Address & 0xff0000 : bank
			// bank >> 1 | offset : SRAM address, unbound
			// unbound & SRAMMask : SRAM offset
			byte = Memory.SRAM[(((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask];
			addCyclesInMemoryAccess(speed);
			return byte;

		case CMemory::MAP_HIROM_SRAM:
		case CMemory::MAP_RONLY_SRAM:
			byte = Memory.SRAM[((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask];
			addCyclesInMemoryAccess(speed);
			return byte;

		default:
			byte = OpenBus;
			addCyclesInMemoryAccess(speed);
			return byte;
	}
}

inline uint16_t S9xGetWord(uint32_t Address, s9xwrap_t w = WRAP_NONE)
{
	uint16_t word;

	uint32_t mask = MEMMAP_MASK & (w == WRAP_PAGE ? 0xff : (w == WRAP_BANK ? 0xffff : 0xffffff));
	if ((Address & mask) == mask)
	{
		PC_t a;

		word = OpenBus = S9xGetByte(Address);

		switch (w)
		{
			case WRAP_PAGE:
				a.xPBPC = Address;
				++a.B.xPCl;
				return word | (S9xGetByte(a.xPBPC) << 8);

			case WRAP_BANK:
				a.xPBPC = Address;
				++a.W.xPC;
				return word | (S9xGetByte(a.xPBPC) << 8);

			default:
				return word | (S9xGetByte(Address + 1) << 8);
		}
	}

	int block = (Address & 0xffffff) >> MEMMAP_SHIFT;
	uint8_t *GetAddress = Memory.Map[block];
	int32_t speed = memory_speed(Address);

	if (GetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
	{
		word = READ_WORD(&GetAddress[Address & 0xffff]);
		addCyclesInMemoryAccess_x2(speed);
		return word;
	}

	switch (reinterpret_cast<intptr_t>(GetAddress))
	{
		case CMemory::MAP_CPU:
			word  = S9xGetCPU(Address & 0xffff);
			addCyclesInMemoryAccess(speed);
			word |= S9xGetCPU((Address + 1) & 0xffff) << 8;
			addCyclesInMemoryAccess(speed);
			return word;

		case CMemory::MAP_PPU:
			if (CPU.InDMAorHDMA)
			{
				word = OpenBus = S9xGetByte(Address);
				return word | (S9xGetByte(Address + 1) << 8);
			}

			word  = S9xGetPPU(Address & 0xffff);
			addCyclesInMemoryAccess(speed);
			word |= S9xGetPPU((Address + 1) & 0xffff) << 8;
			addCyclesInMemoryAccess(speed);
			return word;

		case CMemory::MAP_LOROM_SRAM:
		case CMemory::MAP_SA1RAM:
			if (Memory.SRAMMask >= MEMMAP_MASK)
				word = READ_WORD(&Memory.SRAM[(((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask]);
			else
				word = Memory.SRAM[(((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask] |
					(Memory.SRAM[((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Memory.SRAMMask] << 8);
			addCyclesInMemoryAccess_x2(speed);
			return word;

		case CMemory::MAP_HIROM_SRAM:
		case CMemory::MAP_RONLY_SRAM:
			if (Memory.SRAMMask >= MEMMAP_MASK)
				word = READ_WORD(&Memory.SRAM[((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask]);
			else
				word = Memory.SRAM[((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask] |
					(Memory.SRAM[(((Address + 1) & 0x7fff) - 0x6000 + (((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask] << 8);
			addCyclesInMemoryAccess_x2(speed);
			return word;

		default:
			word = OpenBus | (OpenBus << 8);
			addCyclesInMemoryAccess_x2(speed);
			return word;
	}
}

inline void S9xSetByte(uint8_t Byt, uint32_t Address)
{
	int block = (Address & 0xffffff) >> MEMMAP_SHIFT;
	uint8_t *SetAddress = Memory.WriteMap[block];
	int32_t speed = memory_speed(Address);

	if (SetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
	{
		SetAddress[Address & 0xffff] = Byt;
		addCyclesInMemoryAccess(speed);
		return;
	}

	switch (reinterpret_cast<intptr_t>(SetAddress))
	{
		case CMemory::MAP_CPU:
			S9xSetCPU(Byt, Address & 0xffff);
			addCyclesInMemoryAccess(speed);
			break;

		case CMemory::MAP_PPU:
			if (CPU.InDMAorHDMA && (Address & 0xff00) == 0x2100)
				return;

			S9xSetPPU(Byt, Address & 0xffff);
			addCyclesInMemoryAccess(speed);
			break;

		case CMemory::MAP_LOROM_SRAM:
			if (Memory.SRAMMask)
				Memory.SRAM[(((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask] = Byt;

			addCyclesInMemoryAccess(speed);
			break;

		case CMemory::MAP_HIROM_SRAM:
			if (Memory.SRAMMask)
				Memory.SRAM[((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask] = Byt;

			addCyclesInMemoryAccess(speed);
			break;

		case CMemory::MAP_SA1RAM:
			Memory.SRAM[Address & 0xffff] = Byt;
			addCyclesInMemoryAccess(speed);
			break;

		default:
			addCyclesInMemoryAccess(speed);
	}
}

inline void S9xSetWord(uint16_t Word, uint32_t Address, s9xwrap_t w = WRAP_NONE, s9xwriteorder_t o = WRITE_01)
{
	uint32_t mask = MEMMAP_MASK & (w == WRAP_PAGE ? 0xff : (w == WRAP_BANK ? 0xffff : 0xffffff));
	if ((Address & mask) == mask)
	{
		PC_t a;

		if (!o)
			S9xSetByte(static_cast<uint8_t>(Word), Address);

		switch (w)
		{
			case WRAP_PAGE:
				a.xPBPC = Address;
				++a.B.xPCl;
				S9xSetByte(Word >> 8, a.xPBPC);
				break;

			case WRAP_BANK:
				a.xPBPC = Address;
				++a.W.xPC;
				S9xSetByte(Word >> 8, a.xPBPC);
				break;

			default:
				S9xSetByte(Word >> 8, Address + 1);
		}

		if (o)
			S9xSetByte(static_cast<uint8_t>(Word), Address);

		return;
	}

	int block = (Address & 0xffffff) >> MEMMAP_SHIFT;
	uint8_t *SetAddress = Memory.WriteMap[block];
	int32_t speed = memory_speed(Address);

	if (SetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
	{
		WRITE_WORD(&SetAddress[Address & 0xffff], Word);
		addCyclesInMemoryAccess_x2(speed);
		return;
	}

	switch (reinterpret_cast<intptr_t>(SetAddress))
	{
		case CMemory::MAP_CPU:
			if (o)
			{
				S9xSetCPU(Word >> 8, (Address + 1) & 0xffff);
				addCyclesInMemoryAccess(speed);
				S9xSetCPU(static_cast<uint8_t>(Word), Address & 0xffff);
				addCyclesInMemoryAccess(speed);
			}
			else
			{
				S9xSetCPU(static_cast<uint8_t>(Word), Address & 0xffff);
				addCyclesInMemoryAccess(speed);
				S9xSetCPU(Word >> 8, (Address + 1) & 0xffff);
				addCyclesInMemoryAccess(speed);
			}
			break;

		case CMemory::MAP_PPU:
			if (CPU.InDMAorHDMA)
			{
				if ((Address & 0xff00) != 0x2100)
					S9xSetPPU(static_cast<uint8_t>(Word), Address & 0xffff);
				if (((Address + 1) & 0xff00) != 0x2100)
					S9xSetPPU(Word >> 8, (Address + 1) & 0xffff);
				break;
			}

			if (o)
			{
				S9xSetPPU(Word >> 8, (Address + 1) & 0xffff);
				addCyclesInMemoryAccess(speed);
				S9xSetPPU(static_cast<uint8_t>(Word), Address & 0xffff);
				addCyclesInMemoryAccess(speed);
			}
			else
			{
				S9xSetPPU(static_cast<uint8_t>(Word), Address & 0xffff);
				addCyclesInMemoryAccess(speed);
				S9xSetPPU(Word >> 8, (Address + 1) & 0xffff);
				addCyclesInMemoryAccess(speed);
			}
			break;

		case CMemory::MAP_LOROM_SRAM:
			if (Memory.SRAMMask)
			{
				if (Memory.SRAMMask >= MEMMAP_MASK)
					WRITE_WORD(&Memory.SRAM[(((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask], Word);
				else
				{
					Memory.SRAM[(((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask] = static_cast<uint8_t>(Word);
					Memory.SRAM[((((Address + 1) & 0xff0000) >> 1) | ((Address + 1) & 0x7fff)) & Memory.SRAMMask] = Word >> 8;
				}
			}

			addCyclesInMemoryAccess_x2(speed);
			break;

		case CMemory::MAP_HIROM_SRAM:
			if (Memory.SRAMMask)
			{
				if (Memory.SRAMMask >= MEMMAP_MASK)
					WRITE_WORD(&Memory.SRAM[((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask], Word);
				else
				{
					Memory.SRAM[((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask] = static_cast<uint8_t>(Word);
					Memory.SRAM[(((Address + 1) & 0x7fff) - 0x6000 + (((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask] = Word >> 8;
				}
			}

			addCyclesInMemoryAccess_x2(speed);
			break;

		case CMemory::MAP_SA1RAM:
			WRITE_WORD(&Memory.SRAM[Address & 0xffff], Word);
			addCyclesInMemoryAccess_x2(speed);
			break;

		default:
			addCyclesInMemoryAccess_x2(speed);
	}
}

inline void S9xSetPCBase(uint32_t Address)
{
	Registers.PC.xPBPC = Address & 0xffffff;
	ICPU.ShiftedPB = Address & 0xff0000;

	uint8_t *GetAddress = Memory.Map[static_cast<int>((Address & 0xffffff) >> MEMMAP_SHIFT)];

	CPU.MemSpeed = memory_speed(Address);
	CPU.MemSpeedx2 = CPU.MemSpeed << 1;

	if (GetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
	{
		CPU.PCBase = GetAddress;
		return;
	}

	switch (reinterpret_cast<intptr_t>(GetAddress))
	{
		case CMemory::MAP_LOROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				CPU.PCBase = nullptr;
			else
				CPU.PCBase = &Memory.SRAM[((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask) - (Address & 0xffff)];
			break;

		case CMemory::MAP_HIROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				CPU.PCBase = nullptr;
			else
				CPU.PCBase = &Memory.SRAM[(((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask) - (Address & 0xffff)];
			break;

		case CMemory::MAP_SA1RAM:
			CPU.PCBase = &Memory.SRAM[0];
			break;

		default:
			CPU.PCBase = nullptr;
	}
}

inline uint8_t *S9xGetBasePointer(uint32_t Address)
{
	uint8_t *GetAddress = Memory.Map[(Address & 0xffffff) >> MEMMAP_SHIFT];

	if (GetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
		return GetAddress;

	switch (reinterpret_cast<intptr_t>(GetAddress))
	{
		case CMemory::MAP_LOROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				return nullptr;
			return &Memory.SRAM[((((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask) - (Address & 0xffff)];

		case CMemory::MAP_HIROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				return nullptr;
			return &Memory.SRAM[(((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask) - (Address & 0xffff)];

		case CMemory::MAP_SA1RAM:
			return &Memory.SRAM[0];

		default:
			return nullptr;
	}
}

inline uint8_t *S9xGetMemPointer(uint32_t Address)
{
	uint8_t *GetAddress = Memory.Map[(Address & 0xffffff) >> MEMMAP_SHIFT];

	if (GetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
		return &GetAddress[Address & 0xffff];

	switch (reinterpret_cast<intptr_t>(GetAddress))
	{
		case CMemory::MAP_LOROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				return nullptr;
			return &Memory.SRAM[(((Address & 0xff0000) >> 1) | (Address & 0x7fff)) & Memory.SRAMMask];

		case CMemory::MAP_HIROM_SRAM:
			if ((Memory.SRAMMask & MEMMAP_MASK) != MEMMAP_MASK)
				return nullptr;
			return &Memory.SRAM[((Address & 0x7fff) - 0x6000 + ((Address & 0xf0000) >> 3)) & Memory.SRAMMask];

		case CMemory::MAP_SA1RAM:
			return &Memory.SRAM[Address & 0xffff];

		default:
			return nullptr;
	}
}
