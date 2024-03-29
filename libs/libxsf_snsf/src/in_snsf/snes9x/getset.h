/***********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2010  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),

  (c) Copyright 2002 - 2011  zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja

  (c) Copyright 2009 - 2011  BearOso,
                             OV2


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com),
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti

  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code used in 1.39-1.51
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  SPC7110 and RTC C++ emulator code used in 1.52+
  (c) Copyright 2009         byuu,
                             neviksti

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001 - 2006  byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound emulator code used in 1.5-1.51
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  Sound emulator code used in 1.52+
  (c) Copyright 2004 - 2007  Shay Green (gblargg@gmail.com)

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

  GTK+ GUI code
  (c) Copyright 2004 - 2011  BearOso

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja
  (c) Copyright 2009 - 2011  OV2

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2011  zones


  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 ***********************************************************************************/

#pragma once

#include "cpuexec.h"

inline void addCyclesInMemoryAccess(int32_t speed)
{
	if (!CPU.InDMAorHDMA)
	{
		CPU.PrevCycles = CPU.Cycles;
		CPU.Cycles += speed;
		S9xCheckInterrupts();
		while (CPU.Cycles >= CPU.NextEvent)
			S9xDoHEventProcessing();
	}
}

inline void addCyclesInMemoryAccess_x2(int32_t speed)
{
	if (!CPU.InDMAorHDMA)
	{
		CPU.PrevCycles = CPU.Cycles;
		CPU.Cycles += speed << 1;
		S9xCheckInterrupts();
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
	uint32_t mask = MEMMAP_MASK & (w == WRAP_PAGE ? 0xff : (w == WRAP_BANK ? 0xffff : 0xffffff));
	if ((Address & mask) == mask)
	{
		PC_t a;

		OpenBus = S9xGetByte(Address);

		switch (w)
		{
			case WRAP_PAGE:
				a.xPBPC = Address;
				++a.B.xPCl;
				return OpenBus | (S9xGetByte(a.xPBPC) << 8);

			case WRAP_BANK:
				a.xPBPC = Address;
				++a.W.xPC;
				return OpenBus | (S9xGetByte(a.xPBPC) << 8);

			default:
				return OpenBus | (S9xGetByte(Address + 1) << 8);
		}
	}

	int block = (Address & 0xffffff) >> MEMMAP_SHIFT;
	uint8_t *GetAddress = Memory.Map[block];
	int32_t speed = memory_speed(Address);
	uint16_t word;

	if (GetAddress >= reinterpret_cast<uint8_t *>(CMemory::MAP_LAST))
	{
        //printf("%08X %08X\n",GetAddress,reinterpret_cast<uint8_t *>(CMemory::MAP_LAST));
        //printf("%02X\n",GetAddress[Address & 0xffff]);
        
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
				OpenBus = S9xGetByte(Address);
				return OpenBus | (S9xGetByte(Address + 1) << 8);
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

	int block;
	uint8_t *GetAddress = Memory.Map[block = ((Address & 0xffffff) >> MEMMAP_SHIFT)];

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
