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

static void S9xSoftResetCPU()
{
	CPU.Cycles = 182; // Or 188. This is the cycle count just after the jump to the Reset Vector.
	CPU.PrevCycles = CPU.Cycles;
	CPU.V_Counter = 0;
	CPU.Flags &= DEBUG_MODE_FLAG | TRACE_FLAG;
	CPU.PCBase = nullptr;
	CPU.NMIPending = CPU.IRQLine = CPU.IRQTransition = false;
	CPU.MemSpeed = SLOW_ONE_CYCLE;
	CPU.MemSpeedx2 = SLOW_ONE_CYCLE * 2;
	CPU.FastROMSpeed = SLOW_ONE_CYCLE;
	CPU.InDMA = CPU.InHDMA = CPU.InDMAorHDMA = CPU.InWRAMDMAorHDMA = false;
	CPU.HDMARanInDMA = 0;
	CPU.CurrentDMAorHDMAChannel = -1;
	CPU.WhichEvent = HC_RENDER_EVENT;
	CPU.NextEvent  = Timings.RenderPos;
	CPU.WaitingForInterrupt = false;

	Registers.PC.xPBPC = 0;
	Registers.PC.B.xPB = 0;
	Registers.PC.W.xPC = S9xGetWord(0xfffc);
	OpenBus = Registers.PC.B.xPCh;
	Registers.D.W = 0;
	Registers.DB = 0;
	Registers.S.B.h = 1;
	Registers.S.B.l -= 3;
	Registers.X.B.h = Registers.Y.B.h = 0;

	ICPU.ShiftedPB = ICPU.ShiftedDB = 0;
	SetFlags(MemoryFlag | IndexFlag | IRQ | Emulation);
	ClearFlags(Decimal);

	Timings.InterlaceField = false;
	Timings.H_Max = Timings.H_Max_Master;
	Timings.V_Max = Timings.V_Max_Master;
	Timings.NMITriggerPos = 0xffff;
	Timings.NextIRQTimer = 0x0fffffff;
	Timings.IRQFlagChanging = IRQ_NONE;

	Timings.WRAMRefreshPos = Model->_5A22 == 2 ? SNES_WRAM_REFRESH_HC_v2 : SNES_WRAM_REFRESH_HC_v1;

	S9xSetPCBase(Registers.PC.xPBPC);

	ICPU.S9xOpcodes = S9xOpcodesE1;
	ICPU.S9xOpLengths = S9xOpLengthsM1X1;

	S9xUnpackStatus();
}

static void S9xResetCPU()
{
	S9xSoftResetCPU();
	Registers.S.B.l = 0xff;
	Registers.P.W = Registers.A.W = Registers.X.W = Registers.Y.W = 0;
	SetFlags(MemoryFlag | IndexFlag | IRQ | Emulation);
	ClearFlags(Decimal);
}

void S9xReset()
{
	std::fill_n(&Memory.RAM[0], 0x20000, 0x55);
	std::fill_n(&Memory.VRAM[0], 0x10000, 0);
	std::fill_n(&Memory.FillRAM[0], 0x8000, 0);

	S9xResetCPU();
	S9xResetPPU();
	S9xResetDMA();
	S9xResetAPU();
}
