/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <algorithm>
#include "snes9x.h"
#include "memmap.h"
#include "dma.h"
#include "apu/apu.h"
#include "sdd1.h"

extern uint8_t *HDMAMemPointers[8];

static int CyclesUntilNext(int hc, int vc)
{
	int32_t total = 0;
	int vpos = CPU.V_Counter;

	if (vc - vpos > 0)
	{
		// It's still in this frame */
		// Add number of lines
		total += (vc - vpos) * Timings.H_Max_Master;
		// If line 240 is in there and we're odd, subtract a dot
		if (vpos <= 240 && vc > 240 && Timings.InterlaceField && !IPPU.Interlace)
			total -= ONE_DOT_CYCLE;
	}
	else
	{
		if (vc == vpos && hc > CPU.Cycles)
			return hc;

		total += (Timings.V_Max - vpos) * Timings.H_Max_Master;
		if (vpos <= 240 && Timings.InterlaceField && !IPPU.Interlace)
			total -= ONE_DOT_CYCLE;

		total += vc * Timings.H_Max_Master;
		if (vc > 240 && !Timings.InterlaceField && !IPPU.Interlace)
			total -= ONE_DOT_CYCLE;
	}

	total += hc;

	return total;
}

void S9xUpdateIRQPositions(bool initial)
{
	PPU.HTimerPosition = PPU.IRQHBeamPos * ONE_DOT_CYCLE + Timings.IRQTriggerCycles;
	PPU.HTimerPosition -= PPU.IRQHBeamPos ? 0 : ONE_DOT_CYCLE;
	PPU.HTimerPosition += PPU.IRQHBeamPos > 322 ? (ONE_DOT_CYCLE / 2) : 0;
	PPU.HTimerPosition += PPU.IRQHBeamPos > 326 ? (ONE_DOT_CYCLE / 2) : 0;

	PPU.VTimerPosition = PPU.IRQVBeamPos;

	if (PPU.VTimerEnabled && PPU.VTimerPosition >= Timings.V_Max + (IPPU.Interlace ? 1 : 0))
		Timings.NextIRQTimer = 0x0fffffff;
	else if (!PPU.HTimerEnabled && !PPU.VTimerEnabled)
		Timings.NextIRQTimer = 0x0fffffff;
	else if (PPU.HTimerEnabled && !PPU.VTimerEnabled)
	{
		int v_pos = CPU.V_Counter;

		Timings.NextIRQTimer = PPU.HTimerPosition;
		if (CPU.Cycles > Timings.NextIRQTimer - Timings.IRQTriggerCycles)
		{
			Timings.NextIRQTimer += Timings.H_Max;
			++v_pos;
		}

		// Check for short dot scanline
		if (v_pos == 240 && Timings.InterlaceField && !IPPU.Interlace)
		{
			Timings.NextIRQTimer -= PPU.IRQHBeamPos <= 322 ? ONE_DOT_CYCLE / 2 : 0;
			Timings.NextIRQTimer -= PPU.IRQHBeamPos <= 326 ? ONE_DOT_CYCLE / 2 : 0;
		}
	}
	else if (!PPU.HTimerEnabled && PPU.VTimerEnabled)
	{
		if (CPU.V_Counter == PPU.VTimerPosition && initial)
			Timings.NextIRQTimer = CPU.Cycles + Timings.IRQTriggerCycles - ONE_DOT_CYCLE;
		else
			Timings.NextIRQTimer = CyclesUntilNext(Timings.IRQTriggerCycles - ONE_DOT_CYCLE, PPU.VTimerPosition);
	}
	else
	{
		Timings.NextIRQTimer = CyclesUntilNext(PPU.HTimerPosition, PPU.VTimerPosition);

		// Check for short dot scanline
		int field = Timings.InterlaceField;

		if (PPU.VTimerPosition < CPU.V_Counter || (PPU.VTimerPosition == CPU.V_Counter && Timings.NextIRQTimer > Timings.H_Max))
			field = !field;

		if (PPU.VTimerPosition == 240 && field && !IPPU.Interlace)
		{
			Timings.NextIRQTimer -= PPU.IRQHBeamPos <= 322 ? ONE_DOT_CYCLE / 2 : 0;
			Timings.NextIRQTimer -= PPU.IRQHBeamPos <= 326 ? ONE_DOT_CYCLE / 2 : 0;
		}
	}
}

void S9xSetPPU(uint8_t Byte, uint16_t Address)
{
	// MAP_PPU: $2000-$3FFF

	if (CPU.InDMAorHDMA)
	{
		if (CPU.CurrentDMAorHDMAChannel >= 0 && DMA[CPU.CurrentDMAorHDMAChannel].ReverseTransfer)
			// S9xSetPPU() is called to write to DMA[].AAddress
			return;
		else
		{
			// S9xSetPPU() is called to read from $21xx
			// Take care of DMA wrapping
			if (Address > 0x21ff)
				Address = 0x2100 + (Address & 0xff);
		}
	}

	if ((Address & 0xffc0) == 0x2140) // APUIO0, APUIO1, APUIO2, APUIO3
		// write_port will run the APU until given clock before writing value
		S9xAPUWritePort(Address & 3, Byte);
	else if (Address <= 0x2183)
		switch (Address)
		{
			case 0x2100: // INIDISP
				if (Byte != Memory.FillRAM[0x2100] && (Memory.FillRAM[0x2100] & 0x80) != (Byte & 0x80))
					PPU.ForcedBlanking = !!((Byte >> 7) & 1);

				if ((Memory.FillRAM[0x2100] & 0x80) && CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE)
				{
					PPU.OAMAddr = PPU.SavedOAMAddr;

					uint8_t tmp = 0;
					if (PPU.OAMPriorityRotation)
						tmp = (PPU.OAMAddr & 0xfe) >> 1;
					if ((PPU.OAMFlip & 1) || PPU.FirstSprite != tmp)
						PPU.FirstSprite = tmp;

					PPU.OAMFlip = 0;
				}

				break;

			case 0x2102: // OAMADDL
				PPU.OAMAddr = ((Memory.FillRAM[0x2103] & 1) << 8) | Byte;
				PPU.OAMFlip = 0;
				PPU.SavedOAMAddr = PPU.OAMAddr;
				if (PPU.OAMPriorityRotation && PPU.FirstSprite != (PPU.OAMAddr >> 1))
					PPU.FirstSprite = (PPU.OAMAddr & 0xfe) >> 1;

				break;

			case 0x2103: // OAMADDH
				PPU.OAMAddr = ((Byte & 1) << 8) | Memory.FillRAM[0x2102];
				PPU.OAMPriorityRotation = !!(Byte & 0x80);
				if (PPU.OAMPriorityRotation)
				{
					if (PPU.FirstSprite != (PPU.OAMAddr >> 1))
						PPU.FirstSprite = (PPU.OAMAddr & 0xfe) >> 1;
				}
				else if (PPU.FirstSprite)
					PPU.FirstSprite = 0;

				PPU.OAMFlip = 0;
				PPU.SavedOAMAddr = PPU.OAMAddr;

				break;

			case 0x2104: // OAMDATA
				REGISTER_2104(Byte);
				break;

			case 0x2105: // BGMODE
				if (Byte != Memory.FillRAM[0x2105])
					IPPU.Interlace = 0;

				break;

			case 0x210d: // BG1HOFS, M7HOFS
				PPU.M7byte = Byte;
				break;

			case 0x210e: // BG1VOFS, M7VOFS
				PPU.M7byte = Byte;
				break;

			case 0x2115: // VMAIN
				PPU.VMA.High = !!(Byte & 0x80);
				switch (Byte & 3)
				{
					case 0:
						PPU.VMA.Increment = 1;
						break;
					case 1:
						PPU.VMA.Increment = 32;
						break;
					case 2:
					case 3:
						PPU.VMA.Increment = 128;
				}

				if (Byte & 0x0c)
				{
					static const uint16_t Shift[] = { 0, 5, 6, 7 };
					static const uint16_t IncCount[] = { 0, 32, 64, 128 };

					uint8_t i = (Byte & 0x0c) >> 2;
					PPU.VMA.FullGraphicCount = IncCount[i];
					PPU.VMA.Mask1 = IncCount[i] * 8 - 1;
					PPU.VMA.Shift = Shift[i];
				}
				else
					PPU.VMA.FullGraphicCount = 0;
				break;

			case 0x2116: // VMADDL
				PPU.VMA.Address &= 0xff00;
				PPU.VMA.Address |= Byte;

				S9xUpdateVRAMReadBuffer();

				break;

			case 0x2117: // VMADDH
				PPU.VMA.Address &= 0x00ff;
				PPU.VMA.Address |= Byte << 8;

				S9xUpdateVRAMReadBuffer();

				break;

			case 0x2118: // VMDATAL
				REGISTER_2118(Byte);
				break;

			case 0x2119: // VMDATAH
				REGISTER_2119(Byte);
				break;

			case 0x211b: // M7A
				PPU.MatrixA = PPU.M7byte | (Byte << 8);
				PPU.Need16x8Mulitply = true;
				PPU.M7byte = Byte;
				break;

			case 0x211c: // M7B
				PPU.MatrixB = PPU.M7byte | (Byte << 8);
				PPU.Need16x8Mulitply = true;
				PPU.M7byte = Byte;
				break;

			case 0x211d: // M7C
			case 0x211e: // M7D
			case 0x211f: // M7X
			case 0x2120: // M7Y
				PPU.M7byte = Byte;
				break;

			case 0x2121: // CGADD
				PPU.CGFLIPRead = false;
				PPU.CGADD = Byte;
				break;

			case 0x2133: // SETINI
				if (Byte != Memory.FillRAM[0x2133])
				{
					if (Byte & 0x04)
						PPU.ScreenHeight = SNES_HEIGHT_EXTENDED;
					else
						PPU.ScreenHeight = SNES_HEIGHT;

					if ((Memory.FillRAM[0x2133] ^ Byte) & 3)
						IPPU.Interlace = !!(Byte & 1);
				}

				break;

			case 0x2180: // WMDATA
				if (!CPU.InWRAMDMAorHDMA)
					REGISTER_2180(Byte);
				break;

			case 0x2181: // WMADDL
				if (!CPU.InWRAMDMAorHDMA)
				{
					PPU.WRAM &= 0x1ff00;
					PPU.WRAM |= Byte;
				}

				break;

			case 0x2182: // WMADDM
				if (!CPU.InWRAMDMAorHDMA)
				{
					PPU.WRAM &= 0x100ff;
					PPU.WRAM |= Byte << 8;
				}

				break;

			case 0x2183: // WMADDH
				if (!CPU.InWRAMDMAorHDMA)
				{
					PPU.WRAM &= 0x0ffff;
					PPU.WRAM |= Byte << 16;
					PPU.WRAM &= 0x1ffff;
				}
		}

	Memory.FillRAM[Address] = Byte;
}

uint8_t S9xGetPPU(uint16_t Address)
{
	// MAP_PPU: $2000-$3FFF

	if (Address < 0x2100)
		return OpenBus;

	if (CPU.InDMAorHDMA)
	{
		if (CPU.CurrentDMAorHDMAChannel >= 0 && !DMA[CPU.CurrentDMAorHDMAChannel].ReverseTransfer)
			// S9xGetPPU() is called to read from DMA[].AAddress
			return OpenBus;
		else
		{
			// S9xGetPPU() is called to write to $21xx
			// Take care of DMA wrapping
			if (Address > 0x21ff)
				Address = 0x2100 + (Address & 0xff);
		}
	}

	if ((Address & 0xffc0) == 0x2140) // APUIO0, APUIO1, APUIO2, APUIO3
		// read_port will run the APU until given APU time before reading value
		return S9xAPUReadPort(Address & 3);
	else if (Address <= 0x2183)
	{
		uint8_t byte;

		switch (Address)
		{
			case 0x2104: // OAMDATA
			case 0x2105: // BGMODE
			case 0x2106: // MOSAIC
			case 0x2108: // BG2SC
			case 0x2109: // BG3SC
			case 0x210a: // BG4SC
			case 0x2114: // BG4VOFS
			case 0x2115: // VMAIN
			case 0x2116: // VMADDL
			case 0x2118: // VMDATAL
			case 0x2119: // VMDATAH
			case 0x211a: // M7SEL
			case 0x2124: // W34SEL
			case 0x2125: // WOBJSEL
			case 0x2126: // WH0
			case 0x2128: // WH2
			case 0x2129: // WH3
			case 0x212a: // WBGLOG
				return PPU.OpenBus1;

			case 0x2134: // MPYL
			case 0x2135: // MPYM
			case 0x2136: // MPYH
				if (PPU.Need16x8Mulitply)
				{
					int32_t r = static_cast<int32_t>(PPU.MatrixA) * static_cast<int32_t>(PPU.MatrixB >> 8);
					Memory.FillRAM[0x2134] = static_cast<uint8_t>(r);
					Memory.FillRAM[0x2135] = static_cast<uint8_t>(r >> 8);
					Memory.FillRAM[0x2136] = static_cast<uint8_t>(r >> 16);
					PPU.Need16x8Mulitply = false;
				}
				return (PPU.OpenBus1 = Memory.FillRAM[Address]);

			case 0x2137: // SLHV
				return PPU.OpenBus1;

			case 0x2138: // OAMDATAREAD
				if (PPU.OAMAddr & 0x100)
				{
					if (!(PPU.OAMFlip & 1))
						byte = PPU.OAMData[(PPU.OAMAddr & 0x10f) << 1];
					else
					{
						byte = PPU.OAMData[((PPU.OAMAddr & 0x10f) << 1) + 1];
						PPU.OAMAddr = (PPU.OAMAddr + 1) & 0x1ff;
						if (PPU.OAMPriorityRotation && PPU.FirstSprite != (PPU.OAMAddr >> 1))
							PPU.FirstSprite = (PPU.OAMAddr & 0xfe) >> 1;
					}
				}
				else
				{
					if (!(PPU.OAMFlip & 1))
						byte = PPU.OAMData[PPU.OAMAddr << 1];
					else
					{
						byte = PPU.OAMData[(PPU.OAMAddr << 1) + 1];
						++PPU.OAMAddr;
						if (PPU.OAMPriorityRotation && PPU.FirstSprite != (PPU.OAMAddr >> 1))
							PPU.FirstSprite = (PPU.OAMAddr & 0xfe) >> 1;
					}
				}

				PPU.OAMFlip ^= 1;
				return (PPU.OpenBus1 = byte);

			case 0x2139: // VMDATALREAD
				byte = PPU.VRAMReadBuffer & 0xff;
				if (!PPU.VMA.High)
				{
					S9xUpdateVRAMReadBuffer();

					PPU.VMA.Address += PPU.VMA.Increment;
				}
				return (PPU.OpenBus1 = byte);

			case 0x213a: // VMDATAHREAD
				byte = (PPU.VRAMReadBuffer >> 8) & 0xff;
				if (PPU.VMA.High)
				{
					S9xUpdateVRAMReadBuffer();

					PPU.VMA.Address += PPU.VMA.Increment;
				}
				return (PPU.OpenBus1 = byte);

			case 0x213b: // CGDATAREAD
				if (PPU.CGFLIPRead)
					byte = (PPU.OpenBus2 & 0x80) | ((PPU.CGDATA[PPU.CGADD++] >> 8) & 0x7f);
				else
					byte = PPU.CGDATA[PPU.CGADD] & 0xff;
				PPU.CGFLIPRead = !PPU.CGFLIPRead;
				return (PPU.OpenBus2 = byte);

			case 0x213c: // OPHCT
				if (PPU.HBeamFlip)
					byte = PPU.OpenBus2 & 0xfe;
				else
					byte = 0;
				PPU.HBeamFlip = !PPU.HBeamFlip;
				return (PPU.OpenBus2 = byte);

			case 0x213d: // OPVCT
				if (PPU.VBeamFlip)
					byte = PPU.OpenBus2 & 0xfe;
				else
					byte = 0;
				PPU.VBeamFlip = !PPU.VBeamFlip;
				return (PPU.OpenBus2 = byte);

			case 0x213e: // STAT77
				byte = (PPU.OpenBus1 & 0x10) | Model->_5C77;
				return (PPU.OpenBus1 = byte);

			case 0x213f: // STAT78
				PPU.VBeamFlip = PPU.HBeamFlip = false;
				byte = (PPU.OpenBus2 & 0x20) | (Memory.FillRAM[0x213f] & 0xc0) | (Settings.PAL ? 0x10 : 0) | Model->_5C78;
				Memory.FillRAM[0x213f] &= ~0x40;
				return (PPU.OpenBus2 = byte);

			case 0x2180: // WMDATA
				if (!CPU.InWRAMDMAorHDMA)
				{
					byte = Memory.RAM[PPU.WRAM++];
					PPU.WRAM &= 0x1ffff;
				}
				else
					byte = OpenBus;
				return byte;

			default:
				return OpenBus;
		}
	}
	else
	{
		switch (Address)
		{
			case 0x21c2:
				if (Model->_5C77 == 2)
					return 0x20;
				return OpenBus;

			case 0x21c3:
				if (Model->_5C77 == 2)
					return 0;
				return OpenBus;

			default:
				return OpenBus;
		}
	}
}

void S9xSetCPU(uint8_t Byte, uint16_t Address)
{
	if ((Address & 0xff80) == 0x4300)
	{
		if (CPU.InDMAorHDMA)
			return;

		int d = (Address >> 4) & 0x7;

		switch (Address & 0xf)
		{
			case 0x0: // 0x43x0: DMAPx
				DMA[d].ReverseTransfer = !!(Byte & 0x80);
				DMA[d].HDMAIndirectAddressing = !!(Byte & 0x40);
				DMA[d].UnusedBit43x0 = !!(Byte & 0x20);
				DMA[d].AAddressDecrement = !!(Byte & 0x10);
				DMA[d].AAddressFixed = !!(Byte & 0x08);
				DMA[d].TransferMode = Byte & 7;
				return;

			case 0x1: // 0x43x1: BBADx
				DMA[d].BAddress = Byte;
				return;

			case 0x2: // 0x43x2: A1TxL
				DMA[d].AAddress &= 0xff00;
				DMA[d].AAddress |= Byte;
				return;

			case 0x3: // 0x43x3: A1TxH
				DMA[d].AAddress &= 0xff;
				DMA[d].AAddress |= Byte << 8;
				return;

			case 0x4: // 0x43x4: A1Bx
				DMA[d].ABank = Byte;
				HDMAMemPointers[d] = nullptr;
				return;

			case 0x5: // 0x43x5: DASxL
				DMA[d].DMACount_Or_HDMAIndirectAddress &= 0xff00;
				DMA[d].DMACount_Or_HDMAIndirectAddress |= Byte;
				HDMAMemPointers[d] = nullptr;
				return;

			case 0x6: // 0x43x6: DASxH
				DMA[d].DMACount_Or_HDMAIndirectAddress &= 0xff;
				DMA[d].DMACount_Or_HDMAIndirectAddress |= Byte << 8;
				HDMAMemPointers[d] = nullptr;
				return;

			case 0x7: // 0x43x7: DASBx
				DMA[d].IndirectBank = Byte;
				HDMAMemPointers[d] = nullptr;
				return;

			case 0x8: // 0x43x8: A2AxL
				DMA[d].Address &= 0xff00;
				DMA[d].Address |= Byte;
				HDMAMemPointers[d] = nullptr;
				return;

			case 0x9: // 0x43x9: A2AxH
				DMA[d].Address &= 0xff;
				DMA[d].Address |= Byte << 8;
				HDMAMemPointers[d] = nullptr;
				return;

			case 0xa: // 0x43xa: NLTRx
				if (Byte & 0x7f)
				{
					DMA[d].LineCount = Byte & 0x7f;
					DMA[d].Repeat = !(Byte & 0x80);
				}
				else
				{
					DMA[d].LineCount = 128;
					DMA[d].Repeat = !!(Byte & 0x80);
				}

				return;

			case 0xb: // 0x43xb: ????x
			case 0xf: // 0x43xf: mirror of 0x43xb
				DMA[d].UnknownByte = Byte;
				return;
		}
	}
	else if (Address >= 0x4200)
	{
		uint16_t pos;

		switch (Address)
		{
			case 0x4200: // NMITIMEN
				if (Byte == Memory.FillRAM[0x4200])
					break;

				PPU.VTimerEnabled = !!(Byte & 0x20);
				PPU.HTimerEnabled = !!(Byte & 0x10);

				if (!(Byte & 0x10) && !(Byte & 0x20))
					CPU.IRQLine = CPU.IRQTransition = false;

				if ((Byte & 0x30) != (Memory.FillRAM[0x4200] & 0x30))
					// Only allow instantaneous IRQ if turning it completely on or off
					S9xUpdateIRQPositions(!(Byte & 0x30) || !(Memory.FillRAM[0x4200] & 0x30));

				// NMI can trigger immediately during VBlank as long as NMI_read ($4210) wasn't cleard.
				if ((Byte & 0x80) && !(Memory.FillRAM[0x4200] & 0x80) && CPU.V_Counter >= PPU.ScreenHeight + FIRST_VISIBLE_LINE && (Memory.FillRAM[0x4210] & 0x80))
					// FIXME: triggered at HC+=6, checked just before the final CPU cycle,
					// then, when to call S9xOpcode_NMI()?
					Timings.IRQFlagChanging |= IRQ_TRIGGER_NMI;

				break;

			case 0x4201: // WRIO
				Memory.FillRAM[0x4201] = Memory.FillRAM[0x4213] = Byte;
				break;

			case 0x4203: // WRMPYB
			{
				uint32_t res = Memory.FillRAM[0x4202] * Byte;
				// FIXME: The update occurs 8 machine cycles after $4203 is set.
				Memory.FillRAM[0x4216] = static_cast<uint8_t>(res);
				Memory.FillRAM[0x4217] = static_cast<uint8_t>(res >> 8);
				break;
			}

			case 0x4206: // WRDIVB
			{
				uint16_t a = Memory.FillRAM[0x4204] + (Memory.FillRAM[0x4205] << 8);
				uint16_t div = Byte ? a / Byte : 0xffff;
				uint16_t rem = Byte ? a % Byte : a;
				// FIXME: The update occurs 16 machine cycles after $4206 is set.
				Memory.FillRAM[0x4214] = div & 0xff;
				Memory.FillRAM[0x4215] = div >> 8;
				Memory.FillRAM[0x4216] = rem & 0xff;
				Memory.FillRAM[0x4217] = rem >> 8;
				break;
			}

			case 0x4207: // HTIMEL
				pos = PPU.IRQHBeamPos;
				PPU.IRQHBeamPos = (PPU.IRQHBeamPos & 0xff00) | Byte;
				if (PPU.IRQHBeamPos != pos)
					S9xUpdateIRQPositions(false);
				break;

			case 0x4208: // HTIMEH
				pos = PPU.IRQHBeamPos;
				PPU.IRQHBeamPos = (PPU.IRQHBeamPos & 0xff) | ((Byte & 1) << 8);
				if (PPU.IRQHBeamPos != pos)
					S9xUpdateIRQPositions(false);
				break;

			case 0x4209: // VTIMEL
				pos = PPU.IRQVBeamPos;
				PPU.IRQVBeamPos = (PPU.IRQVBeamPos & 0xff00) | Byte;
				if (PPU.IRQVBeamPos != pos)
					S9xUpdateIRQPositions(true);
				break;

			case 0x420a: // VTIMEH
				pos = PPU.IRQVBeamPos;
				PPU.IRQVBeamPos = (PPU.IRQVBeamPos & 0xff) | ((Byte & 1) << 8);
				if (PPU.IRQVBeamPos != pos)
					S9xUpdateIRQPositions(true);
				break;

			case 0x420b: // MDMAEN
				if (CPU.InDMAorHDMA)
					return;
				// XXX: Not quite right...
				if (Byte)
					CPU.Cycles += Timings.DMACPUSync;
				if (Byte & 0x01)
					S9xDoDMA(0);
				if (Byte & 0x02)
					S9xDoDMA(1);
				if (Byte & 0x04)
					S9xDoDMA(2);
				if (Byte & 0x08)
					S9xDoDMA(3);
				if (Byte & 0x10)
					S9xDoDMA(4);
				if (Byte & 0x20)
					S9xDoDMA(5);
				if (Byte & 0x40)
					S9xDoDMA(6);
				if (Byte & 0x80)
					S9xDoDMA(7);
				break;

			case 0x420c: // HDMAEN
				if (CPU.InDMAorHDMA)
					return;
				Memory.FillRAM[0x420c] = Byte;
				// Yoshi's Island, Genjyu Ryodan, Mortal Kombat, Tales of Phantasia
				PPU.HDMA = Byte & ~PPU.HDMAEnded;
				break;

			case 0x420d: // MEMSEL
				if ((Byte & 1) != (Memory.FillRAM[0x420d] & 1))
				{
					CPU.FastROMSpeed = Byte & 1 ? ONE_CYCLE : SLOW_ONE_CYCLE;
					// we might currently be in FastROMSpeed region, S9xSetPCBase will update CPU.MemSpeed
					S9xSetPCBase(Registers.PC.xPBPC);
				}

				break;

			case 0x4210: // RDNMI
			case 0x4211: // TIMEUP
			case 0x4212: // HVBJOY
			case 0x4213: // RDIO
			case 0x4214: // RDDIVL
			case 0x4215: // RDDIVH
			case 0x4216: // RDMPYL
			case 0x4217: // RDMPYH
			case 0x4218: // JOY1L
			case 0x4219: // JOY1H
			case 0x421a: // JOY2L
			case 0x421b: // JOY2H
			case 0x421c: // JOY3L
			case 0x421d: // JOY3H
			case 0x421e: // JOY4L
			case 0x421f: // JOY4H
				return;

			default:
				if (Settings.SDD1 && Address >= 0x4804 && Address <= 0x4807)
					S9xSetSDD1MemoryMap(Address - 0x4804, Byte & 7);
		}
	}

	Memory.FillRAM[Address] = Byte;
}

uint8_t S9xGetCPU(uint16_t Address)
{
	if (Address < 0x4200)
		return OpenBus;
	else if ((Address & 0xff80) == 0x4300)
	{
		if (CPU.InDMAorHDMA)
			return OpenBus;

		int d = (Address >> 4) & 0x7;

		switch (Address & 0xf)
		{
			case 0x0: // 0x43x0: DMAPx
				return (DMA[d].ReverseTransfer ? 0x80 : 0) | (DMA[d].HDMAIndirectAddressing ? 0x40 : 0) | (DMA[d].UnusedBit43x0 ? 0x20 : 0) | (DMA[d].AAddressDecrement ? 0x10 : 0) |
					(DMA[d].AAddressFixed ? 0x08 : 0) | (DMA[d].TransferMode & 7);

			case 0x1: // 0x43x1: BBADx
				return DMA[d].BAddress;

			case 0x2: // 0x43x2: A1TxL
				return DMA[d].AAddress & 0xff;

			case 0x3: // 0x43x3: A1TxH
				return DMA[d].AAddress >> 8;

			case 0x4: // 0x43x4: A1Bx
				return DMA[d].ABank;

			case 0x5: // 0x43x5: DASxL
				return DMA[d].DMACount_Or_HDMAIndirectAddress & 0xff;

			case 0x6: // 0x43x6: DASxH
				return DMA[d].DMACount_Or_HDMAIndirectAddress >> 8;

			case 0x7: // 0x43x7: DASBx
				return DMA[d].IndirectBank;

			case 0x8: // 0x43x8: A2AxL
				return DMA[d].Address & 0xff;

			case 0x9: // 0x43x9: A2AxH
				return DMA[d].Address >> 8;

			case 0xa: // 0x43xa: NLTRx
				return DMA[d].LineCount ^ (DMA[d].Repeat ? 0x00 : 0x80);

			case 0xb: // 0x43xb: ????x
			case 0xf: // 0x43xf: mirror of 0x43xb
				return DMA[d].UnknownByte;

			default:
				return OpenBus;
		}
	}
	else
	{
		uint8_t byte;

		switch (Address)
		{
			case 0x4210: // RDNMI
				byte = Memory.FillRAM[0x4210];
				Memory.FillRAM[0x4210] = Model->_5A22;
				return (byte & 0x80) | (OpenBus & 0x70) | Model->_5A22;

			case 0x4211: // TIMEUP
				byte = CPU.IRQLine ? 0x80 : 0;
				CPU.IRQLine = CPU.IRQTransition = false;
				return byte | (OpenBus & 0x7f);

			case 0x4212: // HVBJOY
				return REGISTER_4212() | (OpenBus & 0x3e);

			case 0x4213: // RDIO
				return Memory.FillRAM[0x4213];

			case 0x4214: // RDDIVL
			case 0x4215: // RDDIVH
			case 0x4216: // RDMPYL
			case 0x4217: // RDMPYH
				return Memory.FillRAM[Address];

			case 0x4218: // JOY1L
			case 0x4219: // JOY1H
			case 0x421a: // JOY2L
			case 0x421b: // JOY2H
			case 0x421c: // JOY3L
			case 0x421d: // JOY3H
			case 0x421e: // JOY4L
			case 0x421f: // JOY4H
				return Memory.FillRAM[Address];

			default:
				if (Settings.SDD1 && Address >= 0x4800 && Address <= 0x4807)
					return Memory.FillRAM[Address];
				return OpenBus;
		}
	}
}

void S9xResetPPU()
{
	S9xSoftResetPPU();
	PPU.M7byte = 0;
}

void S9xSoftResetPPU()
{
	PPU.VMA.High = false;
	PPU.VMA.Increment = 1;
	PPU.VMA.Address = PPU.VMA.FullGraphicCount = PPU.VMA.Shift = 0;

	PPU.WRAM = 0;

	PPU.CGFLIPRead = false;
	PPU.CGADD = 0;

	for (int c = 0; c < 256; ++c)
	{
		IPPU.Red[c] = (c & 7) << 2;
		IPPU.Green[c] = ((c >> 3) & 7) << 2;
		IPPU.Blue[c] = ((c >> 6) & 2) << 3;
		PPU.CGDATA[c] = IPPU.Red[c] | (IPPU.Green[c] << 5) | (IPPU.Blue[c] << 10);
	}

	PPU.OAMAddr = PPU.SavedOAMAddr = 0;
	PPU.OAMPriorityRotation = false;
	PPU.OAMFlip = 0;
	PPU.OAMWriteRegister = 0;
	std::fill_n(&PPU.OAMData[0], sizeof(PPU.OAMData), 0);

	PPU.FirstSprite = 0;

	PPU.HTimerEnabled = PPU.VTimerEnabled = false;
	PPU.HTimerPosition = Timings.H_Max + 1;
	PPU.VTimerPosition = Timings.V_Max + 1;
	PPU.IRQHBeamPos = PPU.IRQVBeamPos = 0x1ff;

	PPU.HBeamFlip = PPU.VBeamFlip = false;

	PPU.MatrixA = PPU.MatrixB = 0;

	PPU.ForcedBlanking = true;

	PPU.ScreenHeight = SNES_HEIGHT;

	PPU.Need16x8Mulitply = false;

	PPU.HDMA = PPU.HDMAEnded = 0;

	PPU.OpenBus1 = PPU.OpenBus2 = 0;

	PPU.VRAMReadBuffer = 0; // XXX: FIXME: anything better?
	IPPU.Interlace = false;

	for (int c = 0; c < 0x8000; c += 0x100)
		std::fill_n(&Memory.FillRAM[c], 0x100, c >> 8);
	std::fill_n(&Memory.FillRAM[0x2100], 0x0100, 0);
	std::fill_n(&Memory.FillRAM[0x4200], 0x0100, 0);
	std::fill_n(&Memory.FillRAM[0x4000], 0x0100, 0);
	// For BS Suttehakkun 2...
	std::fill_n(&Memory.FillRAM[0x1000], 0x1000, 0);

	Memory.FillRAM[0x4201] = Memory.FillRAM[0x4213] = 0xff;
	Memory.FillRAM[0x2126] = Memory.FillRAM[0x2128] = 1;
}
