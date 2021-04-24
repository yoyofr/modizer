/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "snes9x.h"
#include "memmap.h"
#include "cpuops.h"
#include "dma.h"
#include "apu/apu.h"

static inline void S9xReschedule()
{
	switch (CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
			CPU.WhichEvent = HC_HDMA_START_EVENT;
			CPU.NextEvent = Timings.HDMAStart;
			break;

		case HC_HDMA_START_EVENT:
			CPU.WhichEvent = HC_HCOUNTER_MAX_EVENT;
			CPU.NextEvent = Timings.H_Max;
			break;

		case HC_HCOUNTER_MAX_EVENT:
			CPU.WhichEvent = HC_HDMA_INIT_EVENT;
			CPU.NextEvent = Timings.HDMAInit;
			break;

		case HC_HDMA_INIT_EVENT:
			CPU.WhichEvent = HC_RENDER_EVENT;
			CPU.NextEvent = Timings.RenderPos;
			break;

		case HC_RENDER_EVENT:
			CPU.WhichEvent = HC_WRAM_REFRESH_EVENT;
			CPU.NextEvent = Timings.WRAMRefreshPos;
			break;

		case HC_WRAM_REFRESH_EVENT:
			CPU.WhichEvent = HC_HBLANK_START_EVENT;
			CPU.NextEvent = Timings.HBlankStart;
	}
}

static void CHECK_FOR_IRQ_CHANGE()
{
	if (Timings.IRQFlagChanging)
	{
		if (Timings.IRQFlagChanging & IRQ_TRIGGER_NMI)
		{
			CPU.NMIPending = true;
			Timings.NMITriggerPos = CPU.Cycles + 6;
		}
		if (Timings.IRQFlagChanging & IRQ_CLEAR_FLAG)
			ClearIRQ();
		else if (Timings.IRQFlagChanging & IRQ_SET_FLAG)
			SetIRQ();
		Timings.IRQFlagChanging = IRQ_NONE;
	}
}

void S9xMainLoop()
{
	if (CPU.Flags & SCAN_KEYS_FLAG)
		CPU.Flags &= ~SCAN_KEYS_FLAG;

	for (;;)
	{
		if (CPU.NMIPending)
		{
			if (Timings.NMITriggerPos <= CPU.Cycles)
			{
				CPU.NMIPending = false;
				Timings.NMITriggerPos = 0xffff;
				if (CPU.WaitingForInterrupt)
				{
					CPU.WaitingForInterrupt = false;
					++Registers.PC.W.xPC;
					CPU.Cycles += TWO_CYCLES + ONE_DOT_CYCLE / 2;
					while (CPU.Cycles >= CPU.NextEvent)
						S9xDoHEventProcessing();
				}

				CHECK_FOR_IRQ_CHANGE();
				S9xOpcode_NMI();
			}
		}

		if (CPU.Cycles >= Timings.NextIRQTimer)
		{
			S9xUpdateIRQPositions(false);
			CPU.IRQLine = true;
		}

		if (CPU.IRQLine)
		{
			if (CPU.WaitingForInterrupt)
			{
				CPU.WaitingForInterrupt = false;
				++Registers.PC.W.xPC;
				CPU.Cycles += TWO_CYCLES + ONE_DOT_CYCLE / 2;
				while (CPU.Cycles >= CPU.NextEvent)
					S9xDoHEventProcessing();
			}

			if (!CheckFlag(IRQ))
			{
				/* The flag pushed onto the stack is the new value */
				CHECK_FOR_IRQ_CHANGE();
				S9xOpcode_IRQ();
			}
		}

		CHECK_FOR_IRQ_CHANGE();

		if (CPU.Flags & SCAN_KEYS_FLAG)
			break;

		uint8_t Op;
		SOpcodes *Opcodes;

		if (CPU.PCBase)
		{
			Op = CPU.PCBase[Registers.PC.W.xPC];
			CPU.Cycles += CPU.MemSpeed;
			Opcodes = ICPU.S9xOpcodes;
		}
		else
		{
			Op = S9xGetByte(Registers.PC.xPBPC);
			OpenBus = Op;
			Opcodes = S9xOpcodesSlow;
		}

		if ((Registers.PC.W.xPC & MEMMAP_MASK) + ICPU.S9xOpLengths[Op] >= MEMMAP_BLOCK_SIZE)
		{
			uint8_t *oldPCBase = CPU.PCBase;

			CPU.PCBase = S9xGetBasePointer(ICPU.ShiftedPB + static_cast<uint16_t>(Registers.PC.W.xPC + 4));
			if (oldPCBase != CPU.PCBase || (Registers.PC.W.xPC & ~MEMMAP_MASK) == (0xffff & ~MEMMAP_MASK))
				Opcodes = S9xOpcodesSlow;
		}

		++Registers.PC.W.xPC;
		(*Opcodes[Op].S9xOpcode)();
	}

	S9xPackStatus();
}

void S9xDoHEventProcessing()
{
	switch (CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
			S9xReschedule();
			break;

		case HC_HDMA_START_EVENT:
			S9xReschedule();

			if (PPU.HDMA && CPU.V_Counter <= PPU.ScreenHeight)
				PPU.HDMA = S9xDoHDMA(PPU.HDMA);

			break;

		case HC_HCOUNTER_MAX_EVENT:
			S9xAPUEndScanline();
			CPU.Cycles -= Timings.H_Max;
			if (Timings.NMITriggerPos != 0xffff)
				Timings.NMITriggerPos -= Timings.H_Max;
			if (Timings.NextIRQTimer != 0x0fffffff)
				Timings.NextIRQTimer -= Timings.H_Max;
			S9xAPUSetReferenceTime(CPU.Cycles);

			++CPU.V_Counter;
			if (CPU.V_Counter >= Timings.V_Max) // V ranges from 0 to Timings.V_Max - 1
			{
				CPU.V_Counter = 0;
				Timings.InterlaceField = !Timings.InterlaceField;

				// From byuu:
				// [NTSC]
				// interlace mode has 525 scanlines: 263 on the even frame, and 262 on the odd.
				// non-interlace mode has 524 scanlines: 262 scanlines on both even and odd frames.
				// [PAL] <PAL info is unverified on hardware>
				// interlace mode has 625 scanlines: 313 on the even frame, and 312 on the odd.
				// non-interlace mode has 624 scanlines: 312 scanlines on both even and odd frames.
				if (IPPU.Interlace && !Timings.InterlaceField)
					Timings.V_Max = Timings.V_Max_Master + 1; // 263 (NTSC), 313?(PAL)
				else
					Timings.V_Max = Timings.V_Max_Master; // 262 (NTSC), 312?(PAL)

				Memory.FillRAM[0x213F] ^= 0x80;

				// FIXME: reading $4210 will wait 2 cycles, then perform reading, then wait 4 more cycles.
				Memory.FillRAM[0x4210] = Model->_5A22;
			}

			// From byuu:
			// In non-interlace mode, there are 341 dots per scanline, and 262 scanlines per frame.
			// On odd frames, scanline 240 is one dot short.
			// In interlace mode, there are always 341 dots per scanline. Even frames have 263 scanlines,
			// and odd frames have 262 scanlines.
			// Interlace mode scanline 240 on odd frames is not missing a dot.
			if (CPU.V_Counter == 240 && !IPPU.Interlace && Timings.InterlaceField) // V=240
				Timings.H_Max = Timings.H_Max_Master - ONE_DOT_CYCLE; // HC=1360
			else
				Timings.H_Max = Timings.H_Max_Master; // HC=1364

			if (Model->_5A22 == 2)
			{
				if (CPU.V_Counter != 240 || IPPU.Interlace || !Timings.InterlaceField)	// V=240
				{
					if (Timings.WRAMRefreshPos == SNES_WRAM_REFRESH_HC_v2 - ONE_DOT_CYCLE) // HC=534
						Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2; // HC=538
					else
						Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2 - ONE_DOT_CYCLE; // HC=534
				}
			}
			else
				Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v1;

			if (CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE) // VBlank starts from V=225(240).
			{
				CPU.Flags |= SCAN_KEYS_FLAG;

				PPU.HDMA = 0;
				// Bits 7 and 6 of $4212 are computed when read in S9xGetPPU.
				PPU.ForcedBlanking = !!((Memory.FillRAM[0x2100] >> 7) & 1);

				if (!PPU.ForcedBlanking)
				{
					PPU.OAMAddr = PPU.SavedOAMAddr;

					uint8_t tmp = 0;

					if (PPU.OAMPriorityRotation)
						tmp = (PPU.OAMAddr & 0xFE) >> 1;
					if ((PPU.OAMFlip & 1) || PPU.FirstSprite != tmp)
						PPU.FirstSprite = tmp;

					PPU.OAMFlip = 0;
				}

				// FIXME: writing to $4210 will wait 6 cycles.
				Memory.FillRAM[0x4210] = 0x80 | Model->_5A22;
				if (Memory.FillRAM[0x4200] & 0x80)
				{
					// FIXME: triggered at HC=6, checked just before the final CPU cycle,
					// then, when to call S9xOpcode_NMI()?
					CPU.NMIPending = true;
					Timings.NMITriggerPos = 6 + 6;
				}
			}

			S9xReschedule();

			break;

		case HC_HDMA_INIT_EVENT:
			S9xReschedule();

			if (!CPU.V_Counter)
				S9xStartHDMA();

			break;

		case HC_RENDER_EVENT:
			S9xReschedule();

			break;

		case HC_WRAM_REFRESH_EVENT:
			CPU.Cycles += SNES_WRAM_REFRESH_CYCLES;

			S9xReschedule();
	}
}
