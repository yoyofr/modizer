#include <algorithm>
#include "GBA.h"
#include "GBAcpu.h"
#include "GBAinline.h"
#include "Globals.h"
#include "Sound.h"
#include "bios.h"
#include "../common/Port.h"

extern int mapgsf(uint8_t *a, int l, int &s);

int SWITicks = 0;
static int IRQTicks = 0;

static int layerEnableDelay = 0;
bool busPrefetch = false;
bool busPrefetchEnable = false;
uint32_t busPrefetchCount = 0;
static int cpuDmaTicksToUpdate = 0;
bool cpuDmaHack = false;
uint32_t cpuDmaLast = 0;
static int dummyAddress = 0;

int cpuNextEvent = 0;

static bool intState = false;
bool stopState = false;
bool holdState = false;

uint32_t cpuPrefetch[2];

int cpuTotalTicks = 0;

static int lcdTicks = 208;
static uint8_t timerOnOffDelay = 0;
static uint16_t timer0Value = 0;
bool timer0On = false;
int timer0Ticks = 0;
static int timer0Reload = 0;
int timer0ClockReload  = 0;
static uint16_t timer1Value = 0;
bool timer1On = false;
int timer1Ticks = 0;
static int timer1Reload = 0;
int timer1ClockReload  = 0;
static uint16_t timer2Value = 0;
bool timer2On = false;
int timer2Ticks = 0;
static int timer2Reload = 0;
int timer2ClockReload  = 0;
static uint16_t timer3Value = 0;
bool timer3On = false;
int timer3Ticks = 0;
static int timer3Reload = 0;
int timer3ClockReload  = 0;
static uint32_t dma0Source = 0;
static uint32_t dma0Dest = 0;
static uint32_t dma1Source = 0;
static uint32_t dma1Dest = 0;
static uint32_t dma2Source = 0;
static uint32_t dma2Dest = 0;
static uint32_t dma3Source = 0;
static uint32_t dma3Dest = 0;

static const int TIMER_TICKS[] = { 0, 6, 8, 10 };

const uint32_t objTilesAddress[] = { 0x010000, 0x014000, 0x014000 };
static const uint8_t gamepakRamWaitState[] = { 4, 3, 2, 8 };
static const uint8_t gamepakWaitState[] =  { 4, 3, 2, 8 };
static const uint8_t gamepakWaitState0[] = { 2, 1 };
static const uint8_t gamepakWaitState1[] = { 4, 1 };
static const uint8_t gamepakWaitState2[] = { 8, 1 };

uint8_t memoryWait[] = { 0, 0, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0 };
uint8_t memoryWait32[] = { 0, 0, 5, 0, 0, 1, 1, 0, 7, 7, 9, 9, 13, 13, 4, 0 };
uint8_t memoryWaitSeq[] = { 0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 4, 4, 8, 8, 4, 0 };
uint8_t memoryWaitSeq32[] = { 0, 0, 5, 0, 0, 1, 1, 0, 5, 5, 9, 9, 17, 17, 4, 0 };

// The videoMemoryWait constants are used to add some waitstates
// if the opcode access video memory data outside of vblank/hblank
// It seems to happen on only one ticks for each pixel.
// Not used for now (too problematic with current code).
//const u8 videoMemoryWait[16] =
//  {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t biosProtected[4];

#ifdef WORDS_BIGENDIAN
static bool cpuBiosSwapped = false;
#endif

static uint32_t myROM[] =
{
	0xEA000006,
	0xEA000093,
	0xEA000006,
	0x00000000,
	0x00000000,
	0x00000000,
	0xEA000088,
	0x00000000,
	0xE3A00302,
	0xE1A0F000,
	0xE92D5800,
	0xE55EC002,
	0xE28FB03C,
	0xE79BC10C,
	0xE14FB000,
	0xE92D0800,
	0xE20BB080,
	0xE38BB01F,
	0xE129F00B,
	0xE92D4004,
	0xE1A0E00F,
	0xE12FFF1C,
	0xE8BD4004,
	0xE3A0C0D3,
	0xE129F00C,
	0xE8BD0800,
	0xE169F00B,
	0xE8BD5800,
	0xE1B0F00E,
	0x0000009C,
	0x0000009C,
	0x0000009C,
	0x0000009C,
	0x000001F8,
	0x000001F0,
	0x000000AC,
	0x000000A0,
	0x000000FC,
	0x00000168,
	0xE12FFF1E,
	0xE1A03000,
	0xE1A00001,
	0xE1A01003,
	0xE2113102,
	0x42611000,
	0xE033C040,
	0x22600000,
	0xE1B02001,
	0xE15200A0,
	0x91A02082,
	0x3AFFFFFC,
	0xE1500002,
	0xE0A33003,
	0x20400002,
	0xE1320001,
	0x11A020A2,
	0x1AFFFFF9,
	0xE1A01000,
	0xE1A00003,
	0xE1B0C08C,
	0x22600000,
	0x42611000,
	0xE12FFF1E,
	0xE92D0010,
	0xE1A0C000,
	0xE3A01001,
	0xE1500001,
	0x81A000A0,
	0x81A01081,
	0x8AFFFFFB,
	0xE1A0000C,
	0xE1A04001,
	0xE3A03000,
	0xE1A02001,
	0xE15200A0,
	0x91A02082,
	0x3AFFFFFC,
	0xE1500002,
	0xE0A33003,
	0x20400002,
	0xE1320001,
	0x11A020A2,
	0x1AFFFFF9,
	0xE0811003,
	0xE1B010A1,
	0xE1510004,
	0x3AFFFFEE,
	0xE1A00004,
	0xE8BD0010,
	0xE12FFF1E,
	0xE0010090,
	0xE1A01741,
	0xE2611000,
	0xE3A030A9,
	0xE0030391,
	0xE1A03743,
	0xE2833E39,
	0xE0030391,
	0xE1A03743,
	0xE2833C09,
	0xE283301C,
	0xE0030391,
	0xE1A03743,
	0xE2833C0F,
	0xE28330B6,
	0xE0030391,
	0xE1A03743,
	0xE2833C16,
	0xE28330AA,
	0xE0030391,
	0xE1A03743,
	0xE2833A02,
	0xE2833081,
	0xE0030391,
	0xE1A03743,
	0xE2833C36,
	0xE2833051,
	0xE0030391,
	0xE1A03743,
	0xE2833CA2,
	0xE28330F9,
	0xE0000093,
	0xE1A00840,
	0xE12FFF1E,
	0xE3A00001,
	0xE3A01001,
	0xE92D4010,
	0xE3A03000,
	0xE3A04001,
	0xE3500000,
	0x1B000004,
	0xE5CC3301,
	0xEB000002,
	0x0AFFFFFC,
	0xE8BD4010,
	0xE12FFF1E,
	0xE3A0C301,
	0xE5CC3208,
	0xE15C20B8,
	0xE0110002,
	0x10222000,
	0x114C20B8,
	0xE5CC4208,
	0xE12FFF1E,
	0xE92D500F,
	0xE3A00301,
	0xE1A0E00F,
	0xE510F004,
	0xE8BD500F,
	0xE25EF004,
	0xE59FD044,
	0xE92D5000,
	0xE14FC000,
	0xE10FE000,
	0xE92D5000,
	0xE3A0C302,
	0xE5DCE09C,
	0xE35E00A5,
	0x1A000004,
	0x05DCE0B4,
	0x021EE080,
	0xE28FE004,
	0x159FF018,
	0x059FF018,
	0xE59FD018,
	0xE8BD5000,
	0xE169F00C,
	0xE8BD5000,
	0xE25EF004,
	0x03007FF0,
	0x09FE2000,
	0x09FFC000,
	0x03007FE0
};

static int romSize = 0x2000000;

static inline int CPUUpdateTicks()
{
	int cpuLoopTicks = lcdTicks;

	if (soundTicks < cpuLoopTicks)
		cpuLoopTicks = soundTicks;

	if (timer0On && timer0Ticks < cpuLoopTicks)
		cpuLoopTicks = timer0Ticks;
	if (timer1On && !(TM1CNT & 4) && timer1Ticks < cpuLoopTicks)
		cpuLoopTicks = timer1Ticks;
	if (timer2On && !(TM2CNT & 4) && timer2Ticks < cpuLoopTicks)
		cpuLoopTicks = timer2Ticks;
	if (timer3On && !(TM3CNT & 4) && timer3Ticks < cpuLoopTicks)
		cpuLoopTicks = timer3Ticks;

	if (SWITicks && SWITicks < cpuLoopTicks)
		cpuLoopTicks = SWITicks;

	if (IRQTicks && IRQTicks < cpuLoopTicks)
		cpuLoopTicks = IRQTicks;

	return cpuLoopTicks;
}

int CPULoadRom()
{
	romSize = 0x2000000;

	memset(&rom[0], 0, 0x2000000);
	memset(&workRAM[0], 0, 0x40000);

	if (cpuIsMultiBoot)
		mapgsf(&workRAM[0], 0x40000, romSize);
	else
		mapgsf(&rom[0], 0x2000000, romSize);

	auto temp = reinterpret_cast<uint16_t *>(&rom[(romSize + 1) & ~1]);
	for (int i = (romSize + 1) & ~1; i < 0x2000000; i += 2)
	{
		WRITE16LE(&temp[0], (i >> 1) & 0xFFFF);
		++temp;
	}

	memset(&bios[0], 0, 0x4000);
	memset(&internalRAM[0], 0, 0x8000);
	memset(&paletteRAM[0], 0, 0x400);
	memset(&vram[0], 0, 0x20000);
	memset(&oam[0], 0, 0x400);
	memset(&ioMem[0], 0, 0x400);

	return romSize;
}

void CPUUpdateCPSR()
{
	uint32_t CPSR = reg[16].I & 0x40;
	if (N_FLAG)
		CPSR |= 0x80000000;
	if (Z_FLAG)
		CPSR |= 0x40000000;
	if (C_FLAG)
		CPSR |= 0x20000000;
	if (V_FLAG)
		CPSR |= 0x10000000;
	if (!armState)
		CPSR |= 0x00000020;
	if (!armIrqEnable)
		CPSR |= 0x80;
	CPSR |= armMode & 0x1F;
	reg[16].I = CPSR;
}

void CPUUpdateFlags(bool breakLoop)
{
	uint32_t CPSR = reg[16].I;

	N_FLAG = !!(CPSR & 0x80000000);
	Z_FLAG = !!(CPSR & 0x40000000);
	C_FLAG = !!(CPSR & 0x20000000);
	V_FLAG = !!(CPSR & 0x10000000);
	armState = !(CPSR & 0x20);
	armIrqEnable = !(CPSR & 0x80);
	if (breakLoop && armIrqEnable && (IF & IE) && (IME & 1))
		cpuNextEvent = cpuTotalTicks;
}

void CPUSwitchMode(int mode, bool saveState, bool breakLoop)
{
	//if(armMode == mode)
	//	return;

	CPUUpdateCPSR();

	switch (armMode)
	{
		case 0x10:
		case 0x1F:
			reg[R13_USR].I = reg[13].I;
			reg[R14_USR].I = reg[14].I;
			reg[17].I = reg[16].I;
			break;
		case 0x11:
			std::swap(reg[R8_FIQ].I, reg[8].I);
			std::swap(reg[R9_FIQ].I, reg[9].I);
			std::swap(reg[R10_FIQ].I, reg[10].I);
			std::swap(reg[R11_FIQ].I, reg[11].I);
			std::swap(reg[R12_FIQ].I, reg[12].I);
			reg[R13_FIQ].I = reg[13].I;
			reg[R14_FIQ].I = reg[14].I;
			reg[SPSR_FIQ].I = reg[17].I;
			break;
		case 0x12:
			reg[R13_IRQ].I = reg[13].I;
			reg[R14_IRQ].I = reg[14].I;
			reg[SPSR_IRQ].I = reg[17].I;
			break;
		case 0x13:
			reg[R13_SVC].I = reg[13].I;
			reg[R14_SVC].I = reg[14].I;
			reg[SPSR_SVC].I = reg[17].I;
			break;
		case 0x17:
			reg[R13_ABT].I = reg[13].I;
			reg[R14_ABT].I = reg[14].I;
			reg[SPSR_ABT].I = reg[17].I;
			break;
		case 0x1b:
			reg[R13_UND].I = reg[13].I;
			reg[R14_UND].I = reg[14].I;
			reg[SPSR_UND].I = reg[17].I;
	}

	uint32_t CPSR = reg[16].I;
	uint32_t SPSR = reg[17].I;

	switch (mode)
	{
		case 0x10:
		case 0x1F:
			reg[13].I = reg[R13_USR].I;
			reg[14].I = reg[R14_USR].I;
			reg[16].I = SPSR;
			break;
		case 0x11:
			std::swap(reg[8].I, reg[R8_FIQ].I);
			std::swap(reg[9].I, reg[R9_FIQ].I);
			std::swap(reg[10].I, reg[R10_FIQ].I);
			std::swap(reg[11].I, reg[R11_FIQ].I);
			std::swap(reg[12].I, reg[R12_FIQ].I);
			reg[13].I = reg[R13_FIQ].I;
			reg[14].I = reg[R14_FIQ].I;
			if (saveState)
				reg[17].I = CPSR;
			else
				reg[17].I = reg[SPSR_FIQ].I;
			break;
		case 0x12:
			reg[13].I = reg[R13_IRQ].I;
			reg[14].I = reg[R14_IRQ].I;
			reg[16].I = SPSR;
			if (saveState)
				reg[17].I = CPSR;
			else
				reg[17].I = reg[SPSR_IRQ].I;
			break;
		case 0x13:
			reg[13].I = reg[R13_SVC].I;
			reg[14].I = reg[R14_SVC].I;
			reg[16].I = SPSR;
			if (saveState)
				reg[17].I = CPSR;
			else
				reg[17].I = reg[SPSR_SVC].I;
			break;
		case 0x17:
			reg[13].I = reg[R13_ABT].I;
			reg[14].I = reg[R14_ABT].I;
			reg[16].I = SPSR;
			if (saveState)
				reg[17].I = CPSR;
			else
				reg[17].I = reg[SPSR_ABT].I;
			break;
		case 0x1b:
			reg[13].I = reg[R13_UND].I;
			reg[14].I = reg[R14_UND].I;
			reg[16].I = SPSR;
			if (saveState)
				reg[17].I = CPSR;
			else
				reg[17].I = reg[SPSR_UND].I;
	}
	armMode = mode;
	CPUUpdateFlags(breakLoop);
	CPUUpdateCPSR();
}

void CPUUndefinedException()
{
	uint32_t PC = reg[15].I;
	bool savedArmState = armState;
	CPUSwitchMode(0x1b, true, false);
	reg[14].I = PC - (savedArmState ? 4 : 2);
	reg[15].I = 0x04;
	armState = true;
	armIrqEnable = false;
	armNextPC = 0x04;
	ARM_PREFETCH();
	reg[15].I += 4;
}

void CPUSoftwareInterrupt()
{
	uint32_t PC = reg[15].I;
	bool savedArmState = armState;
	CPUSwitchMode(0x13, true, false);
	reg[14].I = PC - (savedArmState ? 4 : 2);
	reg[15].I = 0x08;
	armState = true;
	armIrqEnable = false;
	armNextPC = 0x08;
	ARM_PREFETCH();
	reg[15].I += 4;
}

void CPUSoftwareInterrupt(int comment)
{
	if (armState)
		comment >>= 16;
	if (comment == 0xfa)
		return;

	switch (comment)
	{
		case 0x00:
			BIOS_SoftReset();
			ARM_PREFETCH();
			break;
		case 0x01:
			BIOS_RegisterRamReset();
			break;
		case 0x02:
			holdState = true;
			cpuNextEvent = cpuTotalTicks;
			break;
		case 0x03:
			break;
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			CPUSoftwareInterrupt();
			break;
		case 0x08:
			BIOS_Sqrt();
			break;
		case 0x09:
			BIOS_ArcTan();
			break;
		case 0x0A:
			BIOS_ArcTan2();
			break;
		case 0x0B:
			{
				int len = (reg[2].I & 0x1FFFFF) >> 1;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + len) & 0xe000000)))
				{
					if ((reg[2].I >> 24) & 1)
					{
						if ((reg[2].I >> 26) & 1)
							SWITicks = (7 + memoryWait32[(reg[1].I >> 24) & 0xF]) * (len >> 1);
						else
							SWITicks = (8 + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
					}
					else
					{
						if ((reg[2].I >> 26) & 1)
							SWITicks = (10 + memoryWait32[(reg[0].I >> 24) & 0xF] + memoryWait32[(reg[1].I >> 24) & 0xF]) * (len >> 1);
						else
							SWITicks = (11 + memoryWait[(reg[0].I >> 24) & 0xF] + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
					}
				}
			}
			BIOS_CpuSet();
			break;
		case 0x0C:
			{
				int len = (reg[2].I & 0x1FFFFF) >> 5;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + len) & 0xe000000)))
				{
					if ((reg[2].I >> 24) & 1)
						SWITicks = (6 + memoryWait32[(reg[1].I >> 24) & 0xF] + 7 * (memoryWaitSeq32[(reg[1].I >> 24) & 0xF] + 1)) * len;
					else
						SWITicks = (9 + memoryWait32[(reg[0].I >> 24) & 0xF] + memoryWait32[(reg[1].I >> 24) & 0xF] + 7 * (memoryWaitSeq32[(reg[0].I >> 24) & 0xF] + memoryWaitSeq32[(reg[1].I >> 24) & 0xF] + 2)) * len;
				}
			}
			BIOS_CpuFastSet();
			break;
		case 0x0D:
			BIOS_GetBiosChecksum();
			break;
		case 0x0E:
			BIOS_BgAffineSet();
			break;
		case 0x0F:
			BIOS_ObjAffineSet();
			break;
		case 0x10:
			{
				int len = CPUReadHalfWord(reg[2].I);
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + len) & 0xe000000)))
					SWITicks = (32 + memoryWait[(reg[0].I >> 24) & 0xF]) * len;
			}
			BIOS_BitUnPack();
			break;
		case 0x11:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 8;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (9 + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
			}
			BIOS_LZ77UnCompWram();
			break;
		case 0x12:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 8;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (19 + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
			}
			BIOS_LZ77UnCompVram();
			break;
		case 0x13:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 8;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (29 + (memoryWait[(reg[0].I >> 24) & 0xF] << 1)) * len;
			}
			BIOS_HuffUnComp();
			break;
		case 0x14:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 8;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (11 + memoryWait[(reg[0].I >> 24) & 0xF] + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
			}
			BIOS_RLUnCompWram();
			break;
		case 0x15:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 9;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (34 + (memoryWait[(reg[0].I >> 24) & 0xF] << 1) + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
			}
			BIOS_RLUnCompVram();
			break;
		case 0x16:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 8;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (13 + memoryWait[(reg[0].I >> 24) & 0xF] + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
			}
			BIOS_Diff8bitUnFilterWram();
			break;
		case 0x17:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 9;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (39 + (memoryWait[(reg[0].I >> 24) & 0xF] << 1) + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
			}
			BIOS_Diff8bitUnFilterVram();
			break;
		case 0x18:
			{
				uint32_t len = CPUReadMemory(reg[0].I) >> 9;
				if (!(!(reg[0].I & 0xe000000) || !((reg[0].I + (len & 0x1fffff)) & 0xe000000)))
					SWITicks = (13 + memoryWait[(reg[0].I >> 24) & 0xF] + memoryWait[(reg[1].I >> 24) & 0xF]) * len;
			}
			BIOS_Diff16bitUnFilter();
			break;
		case 0x19:
			if (reg[0].I)
				soundPause();
			else
				soundResume();
			break;
		case 0x1F:
			BIOS_MidiKey2Freq();
			break;
		case 0x2A:
			BIOS_SndDriverJmpTableCopy();
			// let it go, because we don't really emulate this function
	}
}

static void CPUCompareVCOUNT()
{
	if (VCOUNT == (DISPSTAT >> 8))
	{
		DISPSTAT |= 4;
		UPDATE_REG(0x04, DISPSTAT);

		if (DISPSTAT & 0x20)
		{
			IF |= 4;
			UPDATE_REG(0x202, IF);
		}
	}
	else
	{
		DISPSTAT &= 0xFFFB;
		UPDATE_REG(0x4, DISPSTAT);
	}
	if (layerEnableDelay > 0)
	{
		--layerEnableDelay;
		if (layerEnableDelay == 1)
			layerEnable = layerSettings & DISPCNT;
	}
}

static void doDMA(uint32_t &s, uint32_t &d, uint32_t si, uint32_t di, uint32_t c, int transfer32)
{
	int sm = s >> 24;
	int dm = d >> 24;
	int sw = 0;
	int dw = 0;
	int sc = c;

	cpuDmaHack = true;
	// This is done to get the correct waitstates.
	if (sm > 15)
		sm = 15;
	if (dm > 15)
		dm = 15;

	//if ((sm>=0x05) && (sm<=0x07) || (dm>=0x05) && (dm <=0x07))
	//    blank = (((DISPSTAT | ((DISPSTAT>>1)&1))==1) ?  true : false);

	if (transfer32)
	{
		s &= 0xFFFFFFFC;
		if (s < 0x02000000 && (reg[15].I >> 24))
		{
			while (c)
			{
				CPUWriteMemory(d, 0);
				d += di;
				--c;
			}
		}
		else
		{
			while (c)
			{
				cpuDmaLast = CPUReadMemory(s);
				CPUWriteMemory(d, cpuDmaLast);
				d += di;
				s += si;
				--c;
			}
		}
	}
	else
	{
		s &= 0xFFFFFFFE;
		si = static_cast<int>(si) >> 1;
		di = static_cast<int>(di) >> 1;
		if (s < 0x02000000 && (reg[15].I >> 24))
		{
			while (c)
			{
				CPUWriteHalfWord(d, 0);
				d += di;
				--c;
			}
		}
		else
		{
			while (c)
			{
				cpuDmaLast = CPUReadHalfWord(s);
				CPUWriteHalfWord(d, cpuDmaLast);
				cpuDmaLast |= cpuDmaLast << 16;
				d += di;
				s += si;
				--c;
			}
		}
	}

	int totalTicks = 0;

	if (transfer32)
	{
		sw = 1 + memoryWaitSeq32[sm & 15];
		dw = 1 + memoryWaitSeq32[dm & 15];
		totalTicks = (sw + dw) * (sc - 1) + 6 + memoryWait32[sm & 15] + memoryWaitSeq32[dm & 15];
	}
	else
	{
		sw = 1 + memoryWaitSeq[sm & 15];
		dw = 1 + memoryWaitSeq[dm & 15];
		totalTicks = (sw + dw) * (sc - 1) + 6 + memoryWait[sm & 15] + memoryWaitSeq[dm & 15];
	}

	cpuDmaTicksToUpdate += totalTicks;
	cpuDmaHack = false;
}

void CPUCheckDMA(int reason, int dmamask)
{
	// DMA 0
	if ((DM0CNT_H & 0x8000) && (dmamask & 1) && ((DM0CNT_H >> 12) & 3) == reason)
	{
		uint32_t sourceIncrement = 4;
		uint32_t destIncrement = 4;
		switch ((DM0CNT_H >> 7) & 3)
		{
			case 1:
				sourceIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				sourceIncrement = 0;
		}
		switch ((DM0CNT_H >> 5) & 3)
		{
			case 1:
				destIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				destIncrement = 0;
		}
		doDMA(dma0Source, dma0Dest, sourceIncrement, destIncrement, DM0CNT_L ? DM0CNT_L : 0x4000, DM0CNT_H & 0x0400);

		if (DM0CNT_H & 0x4000)
		{
			IF |= 0x0100;
			UPDATE_REG(0x202, IF);
			cpuNextEvent = cpuTotalTicks;
		}

		if (((DM0CNT_H >> 5) & 3) == 3)
			dma0Dest = DM0DAD_L | (DM0DAD_H << 16);

		if (!(DM0CNT_H & 0x0200) || !reason)
		{
			DM0CNT_H &= 0x7FFF;
			UPDATE_REG(0xBA, DM0CNT_H);
		}
	}

	// DMA 1
	if ((DM1CNT_H & 0x8000) && (dmamask & 2) && ((DM1CNT_H >> 12) & 3) == reason)
	{
		uint32_t sourceIncrement = 4;
		uint32_t destIncrement = 4;
		switch ((DM1CNT_H >> 7) & 3)
		{
			case 1:
				sourceIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				sourceIncrement = 0;
		}
		switch ((DM1CNT_H >> 5) & 3)
		{
			case 1:
				destIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				destIncrement = 0;
		}
		if (reason == 3)
			doDMA(dma1Source, dma1Dest, sourceIncrement, 0, 4, 0x0400);
		else
			doDMA(dma1Source, dma1Dest, sourceIncrement, destIncrement, DM1CNT_L ? DM1CNT_L : 0x4000, DM1CNT_H & 0x0400);

		if (DM1CNT_H & 0x4000)
		{
			IF |= 0x0200;
			UPDATE_REG(0x202, IF);
			cpuNextEvent = cpuTotalTicks;
		}

		if (((DM1CNT_H >> 5) & 3) == 3)
			dma1Dest = DM1DAD_L | (DM1DAD_H << 16);

		if (!(DM1CNT_H & 0x0200) || !reason)
		{
			DM1CNT_H &= 0x7FFF;
			UPDATE_REG(0xC6, DM1CNT_H);
		}
	}

	// DMA 2
	if ((DM2CNT_H & 0x8000) && (dmamask & 4) && ((DM2CNT_H >> 12) & 3) == reason)
	{
		uint32_t sourceIncrement = 4;
		uint32_t destIncrement = 4;
		switch ((DM2CNT_H >> 7) & 3)
		{
			case 1:
				sourceIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				sourceIncrement = 0;
		}
		switch ((DM2CNT_H >> 5) & 3)
		{
			case 1:
				destIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				destIncrement = 0;
		}
		if (reason == 3)
			doDMA(dma2Source, dma2Dest, sourceIncrement, 0, 4, 0x0400);
		else
			doDMA(dma2Source, dma2Dest, sourceIncrement, destIncrement, DM2CNT_L ? DM2CNT_L : 0x4000, DM2CNT_H & 0x0400);

		if (DM2CNT_H & 0x4000)
		{
			IF |= 0x0400;
			UPDATE_REG(0x202, IF);
			cpuNextEvent = cpuTotalTicks;
		}

		if (((DM2CNT_H >> 5) & 3) == 3)
			dma2Dest = DM2DAD_L | (DM2DAD_H << 16);

		if (!(DM2CNT_H & 0x0200) || !reason)
		{
			DM2CNT_H &= 0x7FFF;
			UPDATE_REG(0xD2, DM2CNT_H);
		}
	}

	// DMA 3
	if ((DM3CNT_H & 0x8000) && (dmamask & 8) && ((DM3CNT_H >> 12) & 3) == reason)
	{
		uint32_t sourceIncrement = 4;
		uint32_t destIncrement = 4;
		switch ((DM3CNT_H >> 7) & 3)
		{
			case 1:
				sourceIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				sourceIncrement = 0;
		}
		switch ((DM3CNT_H >> 5) & 3)
		{
			case 1:
				destIncrement = static_cast<uint32_t>(-4);
				break;
			case 2:
				destIncrement = 0;
		}
		doDMA(dma3Source, dma3Dest, sourceIncrement, destIncrement, DM3CNT_L ? DM3CNT_L : 0x10000, DM3CNT_H & 0x0400);

		if (DM3CNT_H & 0x4000)
		{
			IF |= 0x0800;
			UPDATE_REG(0x202, IF);
			cpuNextEvent = cpuTotalTicks;
		}

		if (((DM3CNT_H >> 5) & 3) == 3)
			dma3Dest = DM3DAD_L | (DM3DAD_H << 16);

		if (!(DM3CNT_H & 0x0200) || !reason)
		{
			DM3CNT_H &= 0x7FFF;
			UPDATE_REG(0xDE, DM3CNT_H);
		}
	}
}

void CPUUpdateRegister(uint32_t address, uint16_t value)
{
	switch (address)
	{
		case 0x00:
			{
				if ((value & 7) > 5)
					// display modes above 0-5 are prohibited
					DISPCNT = value & 7;
				bool change = !!((DISPCNT ^ value) & 0x80);
				uint16_t changeBGon = (~DISPCNT & value) & 0x0F00; // these layers are being activated

				DISPCNT = value & 0xFFF7; // bit 3 can only be accessed by the BIOS to enable GBC mode
				UPDATE_REG(0x00, DISPCNT);

				if (changeBGon)
				{
					layerEnableDelay = 4;
					layerEnable = layerSettings & value & ~changeBGon;
				}
				else
				{
					layerEnable = layerSettings & value;
					// CPUUpdateTicks();
				}

				if (change && !(value & 0x80))
				{
					if (!(DISPSTAT & 1))
					{
						//lcdTicks = 1008;
						//VCOUNT = 0;
						//UPDATE_REG(0x06, VCOUNT);
						DISPSTAT &= 0xFFFC;
						UPDATE_REG(0x04, DISPSTAT);
						CPUCompareVCOUNT();
					}
				}
			}
			break;
		case 0x04:
			DISPSTAT = (value & 0xFF38) | (DISPSTAT & 7);
			UPDATE_REG(0x04, DISPSTAT);
			break;
		case 0x06:
			// not writable
			break;
		case 0x08:
			BG0CNT = value & 0xDFCF;
			UPDATE_REG(0x08, BG0CNT);
			break;
		case 0x0A:
			BG1CNT = value & 0xDFCF;
			UPDATE_REG(0x0A, BG1CNT);
			break;
		case 0x0C:
			BG2CNT = value & 0xFFCF;
			UPDATE_REG(0x0C, BG2CNT);
			break;
		case 0x0E:
			BG3CNT = value & 0xFFCF;
			UPDATE_REG(0x0E, BG3CNT);
			break;
		case 0x10:
			BG0HOFS = value & 511;
			UPDATE_REG(0x10, BG0HOFS);
			break;
		case 0x12:
			BG0VOFS = value & 511;
			UPDATE_REG(0x12, BG0VOFS);
			break;
		case 0x14:
			BG1HOFS = value & 511;
			UPDATE_REG(0x14, BG1HOFS);
			break;
		case 0x16:
			BG1VOFS = value & 511;
			UPDATE_REG(0x16, BG1VOFS);
			break;
		case 0x18:
			BG2HOFS = value & 511;
			UPDATE_REG(0x18, BG2HOFS);
			break;
		case 0x1A:
			BG2VOFS = value & 511;
			UPDATE_REG(0x1A, BG2VOFS);
			break;
		case 0x1C:
			BG3HOFS = value & 511;
			UPDATE_REG(0x1C, BG3HOFS);
			break;
		case 0x1E:
			BG3VOFS = value & 511;
			UPDATE_REG(0x1E, BG3VOFS);
			break;
		case 0x20:
			BG2PA = value;
			UPDATE_REG(0x20, BG2PA);
			break;
		case 0x22:
			BG2PB = value;
			UPDATE_REG(0x22, BG2PB);
			break;
		case 0x24:
			BG2PC = value;
			UPDATE_REG(0x24, BG2PC);
			break;
		case 0x26:
			BG2PD = value;
			UPDATE_REG(0x26, BG2PD);
			break;
		case 0x28:
			BG2X_L = value;
			UPDATE_REG(0x28, BG2X_L);
			break;
		case 0x2A:
			BG2X_H = value & 0xFFF;
			UPDATE_REG(0x2A, BG2X_H);
			break;
		case 0x2C:
			BG2Y_L = value;
			UPDATE_REG(0x2C, BG2Y_L);
			break;
		case 0x2E:
			BG2Y_H = value & 0xFFF;
			UPDATE_REG(0x2E, BG2Y_H);
			break;
		case 0x30:
			BG3PA = value;
			UPDATE_REG(0x30, BG3PA);
			break;
		case 0x32:
			BG3PB = value;
			UPDATE_REG(0x32, BG3PB);
			break;
		case 0x34:
			BG3PC = value;
			UPDATE_REG(0x34, BG3PC);
			break;
		case 0x36:
			BG3PD = value;
			UPDATE_REG(0x36, BG3PD);
			break;
		case 0x38:
			BG3X_L = value;
			UPDATE_REG(0x38, BG3X_L);
			break;
		case 0x3A:
			BG3X_H = value & 0xFFF;
			UPDATE_REG(0x3A, BG3X_H);
			break;
		case 0x3C:
			BG3Y_L = value;
			UPDATE_REG(0x3C, BG3Y_L);
			break;
		case 0x3E:
			BG3Y_H = value & 0xFFF;
			UPDATE_REG(0x3E, BG3Y_H);
			break;
		case 0x40:
			WIN0H = value;
			UPDATE_REG(0x40, WIN0H);
			break;
		case 0x42:
			WIN1H = value;
			UPDATE_REG(0x42, WIN1H);
			break;
		case 0x44:
			WIN0V = value;
			UPDATE_REG(0x44, WIN0V);
			break;
		case 0x46:
			WIN1V = value;
			UPDATE_REG(0x46, WIN1V);
			break;
		case 0x48:
			WININ = value & 0x3F3F;
			UPDATE_REG(0x48, WININ);
			break;
		case 0x4A:
			WINOUT = value & 0x3F3F;
			UPDATE_REG(0x4A, WINOUT);
			break;
		case 0x4C:
			MOSAIC = value;
			UPDATE_REG(0x4C, MOSAIC);
			break;
		case 0x50:
			BLDMOD = value & 0x3FFF;
			UPDATE_REG(0x50, BLDMOD);
			break;
		case 0x52:
			COLEV = value & 0x1F1F;
			UPDATE_REG(0x52, COLEV);
			break;
		case 0x54:
			COLY = value & 0x1F;
			UPDATE_REG(0x54, COLY);
			break;
		case 0x60:
		case 0x62:
		case 0x64:
		case 0x68:
		case 0x6c:
		case 0x70:
		case 0x72:
		case 0x74:
		case 0x78:
		case 0x7c:
		case 0x80:
		case 0x84:
			soundEvent(address & 0xFF, static_cast<uint8_t>(value & 0xFF));
			soundEvent((address & 0xFF) + 1, static_cast<uint8_t>(value >> 8));
			break;
		case 0x82:
		case 0x88:
		case 0xa0:
		case 0xa2:
		case 0xa4:
		case 0xa6:
		case 0x90:
		case 0x92:
		case 0x94:
		case 0x96:
		case 0x98:
		case 0x9a:
		case 0x9c:
		case 0x9e:
			soundEvent(address & 0xFF, value);
			break;
		case 0xB0:
			DM0SAD_L = value;
			UPDATE_REG(0xB0, DM0SAD_L);
			break;
		case 0xB2:
			DM0SAD_H = value & 0x07FF;
			UPDATE_REG(0xB2, DM0SAD_H);
			break;
		case 0xB4:
			DM0DAD_L = value;
			UPDATE_REG(0xB4, DM0DAD_L);
			break;
		case 0xB6:
			DM0DAD_H = value & 0x07FF;
			UPDATE_REG(0xB6, DM0DAD_H);
			break;
		case 0xB8:
			DM0CNT_L = value & 0x3FFF;
			UPDATE_REG(0xB8, 0);
			break;
		case 0xBA:
			{
				bool start = !!((DM0CNT_H ^ value) & 0x8000);
				value &= 0xF7E0;

				DM0CNT_H = value;
				UPDATE_REG(0xBA, DM0CNT_H);

				if (start && (value & 0x8000))
				{
					dma0Source = DM0SAD_L | (DM0SAD_H << 16);
					dma0Dest = DM0DAD_L | (DM0DAD_H << 16);
					CPUCheckDMA(0, 1);
				}
			}
			break;
		case 0xBC:
			DM1SAD_L = value;
			UPDATE_REG(0xBC, DM1SAD_L);
			break;
		case 0xBE:
			DM1SAD_H = value & 0x0FFF;
			UPDATE_REG(0xBE, DM1SAD_H);
			break;
		case 0xC0:
			DM1DAD_L = value;
			UPDATE_REG(0xC0, DM1DAD_L);
			break;
		case 0xC2:
			DM1DAD_H = value & 0x07FF;
			UPDATE_REG(0xC2, DM1DAD_H);
			break;
		case 0xC4:
			DM1CNT_L = value & 0x3FFF;
			UPDATE_REG(0xC4, 0);
			break;
		case 0xC6:
			{
				bool start = !!((DM1CNT_H ^ value) & 0x8000);
				value &= 0xF7E0;

				DM1CNT_H = value;
				UPDATE_REG(0xC6, DM1CNT_H);

				if (start && (value & 0x8000))
				{
					dma1Source = DM1SAD_L | (DM1SAD_H << 16);
					dma1Dest = DM1DAD_L | (DM1DAD_H << 16);
					CPUCheckDMA(0, 2);
				}
			}
			break;
		case 0xC8:
			DM2SAD_L = value;
			UPDATE_REG(0xC8, DM2SAD_L);
			break;
		case 0xCA:
			DM2SAD_H = value & 0x0FFF;
			UPDATE_REG(0xCA, DM2SAD_H);
			break;
		case 0xCC:
			DM2DAD_L = value;
			UPDATE_REG(0xCC, DM2DAD_L);
			break;
		case 0xCE:
			DM2DAD_H = value & 0x07FF;
			UPDATE_REG(0xCE, DM2DAD_H);
			break;
		case 0xD0:
			DM2CNT_L = value & 0x3FFF;
			UPDATE_REG(0xD0, 0);
			break;
		case 0xD2:
			{
				bool start = !!((DM2CNT_H ^ value) & 0x8000);
				value &= 0xF7E0;

				DM2CNT_H = value;
				UPDATE_REG(0xD2, DM2CNT_H);

				if (start && (value & 0x8000))
				{
					dma2Source = DM2SAD_L | (DM2SAD_H << 16);
					dma2Dest = DM2DAD_L | (DM2DAD_H << 16);
					CPUCheckDMA(0, 4);
				}
			}
			break;
		case 0xD4:
			DM3SAD_L = value;
			UPDATE_REG(0xD4, DM3SAD_L);
			break;
		case 0xD6:
			DM3SAD_H = value & 0x0FFF;
			UPDATE_REG(0xD6, DM3SAD_H);
			break;
		case 0xD8:
			DM3DAD_L = value;
			UPDATE_REG(0xD8, DM3DAD_L);
			break;
		case 0xDA:
			DM3DAD_H = value & 0x0FFF;
			UPDATE_REG(0xDA, DM3DAD_H);
			break;
		case 0xDC:
			DM3CNT_L = value;
			UPDATE_REG(0xDC, 0);
			break;
		case 0xDE:
			{
				bool start = !!((DM3CNT_H ^ value) & 0x8000);
				value &= 0xFFE0;

				DM3CNT_H = value;
				UPDATE_REG(0xDE, DM3CNT_H);

				if (start && (value & 0x8000))
				{
					dma3Source = DM3SAD_L | (DM3SAD_H << 16);
					dma3Dest = DM3DAD_L | (DM3DAD_H << 16);
					CPUCheckDMA(0, 8);
				}
			}
			break;
		case 0x100:
			timer0Reload = value;
			break;
		case 0x102:
			timer0Value = value;
			timerOnOffDelay |= 1;
			cpuNextEvent = cpuTotalTicks;
			break;
		case 0x104:
			timer1Reload = value;
			break;
		case 0x106:
			timer1Value = value;
			timerOnOffDelay |= 2;
			cpuNextEvent = cpuTotalTicks;
			break;
		case 0x108:
			timer2Reload = value;
			break;
		case 0x10A:
			timer2Value = value;
			timerOnOffDelay |= 4;
			cpuNextEvent = cpuTotalTicks;
			break;
		case 0x10C:
			timer3Reload = value;
			break;
		case 0x10E:
			timer3Value = value;
			timerOnOffDelay |= 8;
			cpuNextEvent = cpuTotalTicks;
			break;

		case 0x130:
			P1 |= value & 0x3FF;
			UPDATE_REG(0x130, P1);
			break;

		case 0x132:
			UPDATE_REG(0x132, value & 0xC3FF);
			break;

		case 0x200:
			IE = value & 0x3FFF;
			UPDATE_REG(0x200, IE);
			if ((IME & 1) && (IF & IE) && armIrqEnable)
				cpuNextEvent = cpuTotalTicks;
			break;
		case 0x202:
			IF ^= value & IF;
			UPDATE_REG(0x202, IF);
			break;
		case 0x204:
			memoryWait[0x0e] = memoryWaitSeq[0x0e] = gamepakRamWaitState[value & 3];

			memoryWait[0x08] = memoryWait[0x09] = gamepakWaitState[(value >> 2) & 3];
			memoryWaitSeq[0x08] = memoryWaitSeq[0x09] = gamepakWaitState0[(value >> 4) & 1];

			memoryWait[0x0a] = memoryWait[0x0b] = gamepakWaitState[(value >> 5) & 3];
			memoryWaitSeq[0x0a] = memoryWaitSeq[0x0b] = gamepakWaitState1[(value >> 7) & 1];

			memoryWait[0x0c] = memoryWait[0x0d] = gamepakWaitState[(value >> 8) & 3];
			memoryWaitSeq[0x0c] = memoryWaitSeq[0x0d] = gamepakWaitState2[(value >> 10) & 1];

			for (int i = 8; i < 15; ++i)
			{
				memoryWait32[i] = memoryWait[i] + memoryWaitSeq[i] + 1;
				memoryWaitSeq32[i] = memoryWaitSeq[i] * 2 + 1;
			}

			busPrefetchEnable = (value & 0x4000) == 0x4000;
			busPrefetch = false;
			busPrefetchCount = 0;
			UPDATE_REG(0x204, value & 0x7FFF);
			break;
		case 0x208:
			IME = value & 1;
			UPDATE_REG(0x208, IME);
			if ((IME & 1) && (IF & IE) && armIrqEnable)
				cpuNextEvent = cpuTotalTicks;
			break;
		case 0x300:
			if (value)
				value &= 0xFFFE;
			UPDATE_REG(0x300, value);
			break;
		default:
			UPDATE_REG(address & 0x3FE, value);
	}
}

static void applyTimer()
{
	if (timerOnOffDelay & 1)
	{
		timer0ClockReload = TIMER_TICKS[timer0Value & 3];
		if (!timer0On && (timer0Value & 0x80))
		{
			// reload the counter
			TM0D = timer0Reload;
			timer0Ticks = (0x10000 - TM0D) << timer0ClockReload;
			UPDATE_REG(0x100, TM0D);
		}
		timer0On = !!(timer0Value & 0x80);
		TM0CNT = timer0Value & 0xC7;
		UPDATE_REG(0x102, TM0CNT);
		//CPUUpdateTicks();
	}
	if (timerOnOffDelay & 2)
	{
		timer1ClockReload = TIMER_TICKS[timer1Value & 3];
		if (!timer1On && (timer1Value & 0x80))
		{
			// reload the counter
			TM1D = timer1Reload;
			timer1Ticks = (0x10000 - TM1D) << timer1ClockReload;
			UPDATE_REG(0x104, TM1D);
		}
		timer1On = !!(timer1Value & 0x80);
		TM1CNT = timer1Value & 0xC7;
		UPDATE_REG(0x106, TM1CNT);
	}
	if (timerOnOffDelay & 4)
	{
		timer2ClockReload = TIMER_TICKS[timer2Value & 3];
		if (!timer2On && (timer2Value & 0x80))
		{
			// reload the counter
			TM2D = timer2Reload;
			timer2Ticks = (0x10000 - TM2D) << timer2ClockReload;
			UPDATE_REG(0x108, TM2D);
		}
		timer2On = !!(timer2Value & 0x80);
		TM2CNT = timer2Value & 0xC7;
		UPDATE_REG(0x10A, TM2CNT);
	}
	if (timerOnOffDelay & 8)
	{
		timer3ClockReload = TIMER_TICKS[timer3Value & 3];
		if (!timer3On && (timer3Value & 0x80))
		{
			// reload the counter
			TM3D = timer3Reload;
			timer3Ticks = (0x10000 - TM3D) << timer3ClockReload;
			UPDATE_REG(0x10C, TM3D);
		}
		timer3On = !!(timer3Value & 0x80);
		TM3CNT = timer3Value & 0xC7;
		UPDATE_REG(0x10E, TM3CNT);
	}
	cpuNextEvent = CPUUpdateTicks();
	timerOnOffDelay = 0;
}

uint8_t cpuBitsSet[256];

void CPUInit()
{
#ifdef WORDS_BIGENDIAN
	if (!cpuBiosSwapped)
	{
		for (unsigned i = 0; i < sizeof(myROM) / 4; ++i)
			WRITE32LE(&myROM[i], myROM[i]);
		cpuBiosSwapped = true;
	}
#endif

	memcpy(&bios[0], myROM, sizeof(myROM));

	biosProtected[0] = 0x00;
	biosProtected[1] = 0xf0;
	biosProtected[2] = 0x29;
	biosProtected[3] = 0xe1;

	for (int i = 0; i < 256; ++i)
	{
		int count = 0;
		for (int j = 0; j < 8; ++j)
			if (i & (1 << j))
				++count;
		cpuBitsSet[i] = count;
	}

	std::fill_n(&ioReadable[0], 0x304, true);
	std::fill(&ioReadable[0x10], &ioReadable[0x48], false);
	std::fill(&ioReadable[0x4c], &ioReadable[0x50], false);
	std::fill(&ioReadable[0x54], &ioReadable[0x60], false);
	std::fill(&ioReadable[0x8c], &ioReadable[0x90], false);
	std::fill(&ioReadable[0xa0], &ioReadable[0xb8], false);
	std::fill(&ioReadable[0xbc], &ioReadable[0xc4], false);
	std::fill(&ioReadable[0xc8], &ioReadable[0xd0], false);
	std::fill(&ioReadable[0xd4], &ioReadable[0xdc], false);
	std::fill(&ioReadable[0xe0], &ioReadable[0x100], false);
	std::fill(&ioReadable[0x110], &ioReadable[0x120], false);
	std::fill(&ioReadable[0x12c], &ioReadable[0x130], false);
	std::fill(&ioReadable[0x138], &ioReadable[0x140], false);
	std::fill(&ioReadable[0x144], &ioReadable[0x150], false);
	std::fill(&ioReadable[0x15c], &ioReadable[0x200], false);
	std::fill(&ioReadable[0x20c], &ioReadable[0x300], false);
	std::fill(&ioReadable[0x304], &ioReadable[0x400], false);

	if (romSize < 0x1fe2000)
	{
		*reinterpret_cast<uint16_t *>(&rom[0x1fe209c]) = 0xdffa; // SWI 0xFA
		*reinterpret_cast<uint16_t *>(&rom[0x1fe209e]) = 0x4770; // BX LR
	}
}

void CPUReset()
{
	// clean registers
	memset(&reg[0], 0, sizeof(reg));
	// clean OAM
	memset(&oam[0], 0, 0x400);
	// clean palette
	memset(&paletteRAM[0], 0, 0x400);
	// clean vram
	memset(&vram[0], 0, 0x20000);
	// clean io memory
	memset(&ioMem[0], 0, 0x400);

	DISPCNT = 0x0080;
	DISPSTAT = 0x0000;
	VCOUNT = 0x007E;
	BG0CNT = 0x0000;
	BG1CNT = 0x0000;
	BG2CNT = 0x0000;
	BG3CNT = 0x0000;
	BG0HOFS = 0x0000;
	BG0VOFS = 0x0000;
	BG1HOFS = 0x0000;
	BG1VOFS = 0x0000;
	BG2HOFS = 0x0000;
	BG2VOFS = 0x0000;
	BG3HOFS = 0x0000;
	BG3VOFS = 0x0000;
	BG2PA = 0x0100;
	BG2PB = 0x0000;
	BG2PC = 0x0000;
	BG2PD = 0x0100;
	BG2X_L = 0x0000;
	BG2X_H = 0x0000;
	BG2Y_L = 0x0000;
	BG2Y_H = 0x0000;
	BG3PA = 0x0100;
	BG3PB = 0x0000;
	BG3PC = 0x0000;
	BG3PD = 0x0100;
	BG3X_L = 0x0000;
	BG3X_H = 0x0000;
	BG3Y_L = 0x0000;
	BG3Y_H = 0x0000;
	WIN0H = 0x0000;
	WIN1H = 0x0000;
	WIN0V = 0x0000;
	WIN1V = 0x0000;
	WININ = 0x0000;
	WINOUT = 0x0000;
	MOSAIC = 0x0000;
	BLDMOD = 0x0000;
	COLEV = 0x0000;
	COLY = 0x0000;
	DM0SAD_L = 0x0000;
	DM0SAD_H = 0x0000;
	DM0DAD_L = 0x0000;
	DM0DAD_H = 0x0000;
	DM0CNT_L = 0x0000;
	DM0CNT_H = 0x0000;
	DM1SAD_L = 0x0000;
	DM1SAD_H = 0x0000;
	DM1DAD_L = 0x0000;
	DM1DAD_H = 0x0000;
	DM1CNT_L = 0x0000;
	DM1CNT_H = 0x0000;
	DM2SAD_L = 0x0000;
	DM2SAD_H = 0x0000;
	DM2DAD_L = 0x0000;
	DM2DAD_H = 0x0000;
	DM2CNT_L = 0x0000;
	DM2CNT_H = 0x0000;
	DM3SAD_L = 0x0000;
	DM3SAD_H = 0x0000;
	DM3DAD_L = 0x0000;
	DM3DAD_H = 0x0000;
	DM3CNT_L = 0x0000;
	DM3CNT_H = 0x0000;
	TM0D = 0x0000;
	TM0CNT = 0x0000;
	TM1D = 0x0000;
	TM1CNT = 0x0000;
	TM2D = 0x0000;
	TM2CNT = 0x0000;
	TM3D = 0x0000;
	TM3CNT = 0x0000;
	P1 = 0x03FF;
	IE = 0x0000;
	IF = 0x0000;
	IME = 0x0000;

	armMode = 0x1F;

	reg[13].I = 0x03007F00;
	reg[15].I = cpuIsMultiBoot ? 0x02000000 : 0x08000000;
	reg[16].I = 0x00000000;
	reg[R13_IRQ].I = 0x03007FA0;
	reg[R13_SVC].I = 0x03007FE0;
	armIrqEnable = true;
	armState = true;
	C_FLAG = V_FLAG = N_FLAG = Z_FLAG = false;
	UPDATE_REG(0x00, DISPCNT);
	UPDATE_REG(0x06, VCOUNT);
	UPDATE_REG(0x20, BG2PA);
	UPDATE_REG(0x26, BG2PD);
	UPDATE_REG(0x30, BG3PA);
	UPDATE_REG(0x36, BG3PD);
	UPDATE_REG(0x130, P1);
	UPDATE_REG(0x88, 0x200);

	// disable FIQ
	reg[16].I |= 0x40;

	CPUUpdateCPSR();

	armNextPC = reg[15].I;
	reg[15].I += 4;

	// reset internal state
	holdState = false;

	biosProtected[0] = 0x00;
	biosProtected[1] = 0xf0;
	biosProtected[2] = 0x29;
	biosProtected[3] = 0xe1;

	lcdTicks = 208;
	timer0On = false;
	timer0Ticks = 0;
	timer0Reload = 0;
	timer0ClockReload  = 0;
	timer1On = false;
	timer1Ticks = 0;
	timer1Reload = 0;
	timer1ClockReload  = 0;
	timer2On = false;
	timer2Ticks = 0;
	timer2Reload = 0;
	timer2ClockReload  = 0;
	timer3On = false;
	timer3Ticks = 0;
	timer3Reload = 0;
	timer3ClockReload  = 0;
	dma0Source = 0;
	dma0Dest = 0;
	dma1Source = 0;
	dma1Dest = 0;
	dma2Source = 0;
	dma2Dest = 0;
	dma3Source = 0;
	dma3Dest = 0;
	layerEnable = DISPCNT & layerSettings;

	for (int i = 0; i < 256; ++i)
	{
		map[i].address = reinterpret_cast<uint8_t *>(&dummyAddress);
		map[i].mask = 0;
	}

	map[0].address = &bios[0];
	map[0].mask = 0x3FFF;
	map[2].address = &workRAM[0];
	map[2].mask = 0x3FFFF;
	map[3].address = &internalRAM[0];
	map[3].mask = 0x7FFF;
	map[4].address = &ioMem[0];
	map[4].mask = 0x3FF;
	map[5].address = &paletteRAM[0];
	map[5].mask = 0x3FF;
	map[6].address = &vram[0];
	map[6].mask = 0x1FFFF;
	map[7].address = &oam[0];
	map[7].mask = 0x3FF;
	map[8].address = &rom[0];
	map[8].mask = 0x1FFFFFF;
	map[9].address = &rom[0];
	map[9].mask = 0x1FFFFFF;
	map[10].address = &rom[0];
	map[10].mask = 0x1FFFFFF;
	map[12].address = &rom[0];
	map[12].mask = 0x1FFFFFF;

	soundReset();

	// make sure registers are correctly initialized if not using BIOS
	BIOS_RegisterRamReset(cpuIsMultiBoot ? 0xfe : 0xff);

	ARM_PREFETCH();

	cpuDmaHack = false;

	SWITicks = 0;
}

static void CPUInterrupt()
{
	uint32_t PC = reg[15].I;
	bool savedState = armState;
	CPUSwitchMode(0x12, true, false);
	reg[14].I = PC;
	if (!savedState)
		reg[14].I += 2;
	reg[15].I = 0x18;
	armState = true;
	armIrqEnable = false;

	armNextPC = reg[15].I;
	reg[15].I += 4;
	ARM_PREFETCH();

	//if (!holdState)
	biosProtected[0] = 0x02;
	biosProtected[1] = 0xc0;
	biosProtected[2] = 0x5e;
	biosProtected[3] = 0xe5;
}

void CPULoop(int ticks)
{
	int clockTicks;
	int timerOverflow = 0;
	// variable used by the CPU core
	cpuTotalTicks = 0;

	cpuNextEvent = CPUUpdateTicks();
	if (cpuNextEvent > ticks)
		cpuNextEvent = ticks;

	for (;;)
	{
		if (!holdState && !SWITicks)
		{
			if (armState)
			{
				if (!armExecute())
					return;
			}
			else
			{
				if (!thumbExecute())
					return;
			}
			clockTicks = 0;
		}
		else
			clockTicks = CPUUpdateTicks();

		cpuTotalTicks += clockTicks;

		if (cpuTotalTicks >= cpuNextEvent)
		{
			int remainingTicks = cpuTotalTicks - cpuNextEvent;

			if (SWITicks)
			{
				SWITicks -= clockTicks;
				if (SWITicks < 0)
					SWITicks = 0;
			}

			clockTicks = cpuNextEvent;
			cpuTotalTicks = 0;

		updateLoop:
			if (IRQTicks)
			{
				IRQTicks -= clockTicks;
				if (IRQTicks < 0)
					IRQTicks = 0;
			}

			lcdTicks -= clockTicks;

			if (lcdTicks <= 0)
			{
				if (DISPSTAT & 1) // V-BLANK
				{
					// if in V-Blank mode, keep computing...
					if (DISPSTAT & 2)
					{
						lcdTicks += 1008;
						++VCOUNT;
						UPDATE_REG(0x06, VCOUNT);
						DISPSTAT &= 0xFFFD;
						UPDATE_REG(0x04, DISPSTAT);
						CPUCompareVCOUNT();
					}
					else
					{
						lcdTicks += 224;
						DISPSTAT |= 2;
						UPDATE_REG(0x04, DISPSTAT);
						if (DISPSTAT & 16)
						{
							IF |= 2;
							UPDATE_REG(0x202, IF);
						}
					}

					if (VCOUNT > 227) //Reaching last line
					{
						DISPSTAT &= 0xFFFC;
						UPDATE_REG(0x04, DISPSTAT);
						VCOUNT = 0;
						UPDATE_REG(0x06, VCOUNT);
						CPUCompareVCOUNT();
					}
				}
				else
				{
					if (DISPSTAT & 2)
					{
						// if in H-Blank, leave it and move to drawing mode
						++VCOUNT;
						UPDATE_REG(0x06, VCOUNT);

						lcdTicks += 1008;
						DISPSTAT &= 0xFFFD;
						if (VCOUNT == 160)
						{
							DISPSTAT |= 1;
							DISPSTAT &= 0xFFFD;
							UPDATE_REG(0x04, DISPSTAT);
							if (DISPSTAT & 0x0008)
							{
								IF |= 1;
								UPDATE_REG(0x202, IF);
							}
							CPUCheckDMA(1, 0x0f);
						}

						UPDATE_REG(0x04, DISPSTAT);
						CPUCompareVCOUNT();
					}
					else
					{
						// entering H-Blank
						DISPSTAT |= 2;
						UPDATE_REG(0x04, DISPSTAT);
						lcdTicks += 224;
						CPUCheckDMA(2, 0x0f);
						if (DISPSTAT & 16)
						{
							IF |= 2;
							UPDATE_REG(0x202, IF);
						}
					}
				}
			}

			// we shouldn't be doing sound in stop state, but we loose synchronization
			// if sound is disabled, so in stop state, soundTick will just produce
			// mute sound
			soundTicks -= clockTicks;
			if (soundTicks <= 0)
			{
				psoundTickfn();
				soundTicks += SOUND_CLOCK_TICKS;
			}

			if (!stopState)
			{
				if (timer0On)
				{
					timer0Ticks -= clockTicks;
					if (timer0Ticks <= 0)
					{
						timer0Ticks += (0x10000 - timer0Reload) << timer0ClockReload;
						timerOverflow |= 1;
						soundTimerOverflow(0);
						if (TM0CNT & 0x40)
						{
							IF |= 0x08;
							UPDATE_REG(0x202, IF);
						}
					}
					TM0D = 0xFFFF - (timer0Ticks >> timer0ClockReload);
					UPDATE_REG(0x100, TM0D);
				}

				if (timer1On)
				{
					if (TM1CNT & 4)
					{
						if (timerOverflow & 1)
						{
							++TM1D;
							if (!TM1D)
							{
								TM1D += timer1Reload;
								timerOverflow |= 2;
								soundTimerOverflow(1);
								if (TM1CNT & 0x40)
								{
									IF |= 0x10;
									UPDATE_REG(0x202, IF);
								}
							}
							UPDATE_REG(0x104, TM1D);
						}
					}
					else
					{
						timer1Ticks -= clockTicks;
						if (timer1Ticks <= 0)
						{
							timer1Ticks += (0x10000 - timer1Reload) << timer1ClockReload;
							timerOverflow |= 2;
							soundTimerOverflow(1);
							if (TM1CNT & 0x40)
							{
								IF |= 0x10;
								UPDATE_REG(0x202, IF);
							}
						}
						TM1D = 0xFFFF - (timer1Ticks >> timer1ClockReload);
						UPDATE_REG(0x104, TM1D);
					}
				}

				if (timer2On)
				{
					if (TM2CNT & 4)
					{
						if (timerOverflow & 2)
						{
							++TM2D;
							if (!TM2D)
							{
								TM2D += timer2Reload;
								timerOverflow |= 4;
								if (TM2CNT & 0x40)
								{
									IF |= 0x20;
									UPDATE_REG(0x202, IF);
								}
							}
							UPDATE_REG(0x108, TM2D);
						}
					}
					else
					{
						timer2Ticks -= clockTicks;
						if (timer2Ticks <= 0)
						{
							timer2Ticks += (0x10000 - timer2Reload) << timer2ClockReload;
							timerOverflow |= 4;
							if (TM2CNT & 0x40)
							{
								IF |= 0x20;
								UPDATE_REG(0x202, IF);
							}
						}
						TM2D = 0xFFFF - (timer2Ticks >> timer2ClockReload);
						UPDATE_REG(0x108, TM2D);
					}
				}

				if (timer3On)
				{
					if (TM3CNT & 4)
					{
						if (timerOverflow & 4)
						{
							++TM3D;
							if (!TM3D)
							{
								TM3D += timer3Reload;
								if (TM3CNT & 0x40)
								{
									IF |= 0x40;
									UPDATE_REG(0x202, IF);
								}
							}
							UPDATE_REG(0x10C, TM3D);
						}
					}
					else
					{
						timer3Ticks -= clockTicks;
						if (timer3Ticks <= 0)
						{
							timer3Ticks += (0x10000 - timer3Reload) << timer3ClockReload;
							if (TM3CNT & 0x40)
							{
								IF |= 0x40;
								UPDATE_REG(0x202, IF);
							}
						}
						TM3D = 0xFFFF - (timer3Ticks >> timer3ClockReload);
						UPDATE_REG(0x10C, TM3D);
					}
				}
			}

			timerOverflow = 0;

			ticks -= clockTicks;

			cpuNextEvent = CPUUpdateTicks();

			if (cpuDmaTicksToUpdate > 0)
			{
				if (cpuDmaTicksToUpdate > cpuNextEvent)
					clockTicks = cpuNextEvent;
				else
					clockTicks = cpuDmaTicksToUpdate;
				cpuDmaTicksToUpdate -= clockTicks;
				if (cpuDmaTicksToUpdate < 0)
					cpuDmaTicksToUpdate = 0;
				goto updateLoop;
			}

			if (IF && (IME & 1) && armIrqEnable)
			{
				int res = IF & IE;
				if (stopState)
					res &= 0x3080;
				if (res)
				{
					if (intState)
					{
						if (!IRQTicks)
						{
							CPUInterrupt();
							intState = false;
							holdState = false;
							stopState = false;
						}
					}
					else
					{
						if (!holdState)
						{
							intState = true;
							IRQTicks = 7;
							if (cpuNextEvent> IRQTicks)
								cpuNextEvent = IRQTicks;
						}
						else
						{
							CPUInterrupt();
							holdState = false;
							stopState = false;
						}
					}

					// Stops the SWI Ticks emulation if an IRQ is executed
					// (to avoid problems with nested IRQ/SWI)
					if (SWITicks)
						SWITicks = 0;
				}
			}

			if (remainingTicks > 0)
			{
				if (remainingTicks > cpuNextEvent)
					clockTicks = cpuNextEvent;
				else
					clockTicks = remainingTicks;
				remainingTicks -= clockTicks;
				if (remainingTicks < 0)
					remainingTicks = 0;
				goto updateLoop;
			}

			if (timerOnOffDelay)
				applyTimer();

			if (cpuNextEvent > ticks)
				cpuNextEvent = ticks;

			if (ticks <= 0)
				break;
		}
	}
}
