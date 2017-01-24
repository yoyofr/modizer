#pragma once

#include "GBAinline.h"

int armExecute();
int thumbExecute();

#ifdef __GNUC__
# ifndef __APPLE__
#  define INSN_REGPARM __attribute__((regparm(1)))
# else
#  define INSN_REGPARM /*nothing*/
# endif
# define LIKELY(x) __builtin_expect(!!(x), 1)
# define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
# define INSN_REGPARM /*nothing*/
# define LIKELY(x) (x)
# define UNLIKELY(x) (x)
#endif

inline void UPDATE_REG(uint32_t address, uint16_t value) { WRITE16LE(&ioMem[address], value); }

extern uint32_t cpuPrefetch[2];

inline void ARM_PREFETCH()
{
	cpuPrefetch[0] = CPUReadMemoryQuick(armNextPC);
	cpuPrefetch[1] = CPUReadMemoryQuick(armNextPC + 4);
}

inline void THUMB_PREFETCH()
{
	cpuPrefetch[0] = CPUReadHalfWordQuick(armNextPC);
	cpuPrefetch[1] = CPUReadHalfWordQuick(armNextPC + 2);
}

inline void ARM_PREFETCH_NEXT() { cpuPrefetch[1] = CPUReadMemoryQuick(armNextPC + 4); }

inline void THUMB_PREFETCH_NEXT() { cpuPrefetch[1] = CPUReadHalfWordQuick(armNextPC + 2); }

extern int SWITicks;
extern bool busPrefetch;
extern bool busPrefetchEnable;
extern uint32_t busPrefetchCount;
extern uint8_t memoryWait[16];
extern uint8_t memoryWait32[16];
extern uint8_t memoryWaitSeq[16];
extern uint8_t memoryWaitSeq32[16];
extern uint8_t cpuBitsSet[256];

void CPUSwitchMode(int mode, bool saveState, bool breakLoop = true);
void CPUUpdateCPSR();
void CPUUpdateFlags(bool breakLoop = true);
void CPUUndefinedException();
void CPUSoftwareInterrupt(int comment);

// Waitstates when accessing data
inline int dataTicksAccess16(uint32_t address) // DATA 8/16bits NON SEQ
{
	int addr = (address >> 24) & 15;
	int value = memoryWait[addr];

	if (addr >= 0x08 || addr < 0x02)
	{
		busPrefetchCount = 0;
		busPrefetch = false;
	}
	else if (busPrefetch)
	{
		int waitState = value;
		if (!waitState)
			waitState = 1;
		busPrefetchCount = ((busPrefetchCount + 1) << waitState) - 1;
	}

	return value;
}

inline int dataTicksAccess32(uint32_t address) // DATA 32bits NON SEQ
{
	int addr = (address >> 24) & 15;
	int value = memoryWait32[addr];

	if (addr >= 0x08 || addr < 0x02)
	{
		busPrefetchCount = 0;
		busPrefetch = false;
	}
	else if (busPrefetch)
	{
		int waitState = value;
		if (!waitState)
			waitState = 1;
		busPrefetchCount = ((busPrefetchCount + 1) << waitState) - 1;
	}

	return value;
}

inline int dataTicksAccessSeq32(uint32_t address) // DATA 32bits SEQ
{
	int addr = (address >> 24) & 15;
	int value = memoryWaitSeq32[addr];

	if (addr >= 0x08 || addr < 0x02)
	{
		busPrefetchCount = 0;
		busPrefetch = false;
	}
	else if (busPrefetch)
	{
		int waitState = value;
		if (!waitState)
			waitState = 1;
		busPrefetchCount = ((busPrefetchCount + 1) << waitState) - 1;
	}

	return value;
}

// Waitstates when executing opcode
inline int codeTicksAccess16(uint32_t address) // THUMB NON SEQ
{
	int addr = (address >> 24) & 15;

	if (addr >= 0x08 && addr <= 0x0D)
	{
		if (busPrefetchCount & 0x1)
		{
			if (busPrefetchCount & 0x2)
			{
				busPrefetchCount = ((busPrefetchCount & 0xFF) >> 2) | (busPrefetchCount & 0xFFFFFF00);
				return 0;
			}
			busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
			return memoryWaitSeq[addr] - 1;
		}
		else
		{
			busPrefetchCount = 0;
			return memoryWait[addr];
		}
	}
	else
	{
		busPrefetchCount = 0;
		return memoryWait[addr];
	}
}

inline int codeTicksAccess32(uint32_t address) // ARM NON SEQ
{
	int addr = (address >> 24) & 15;

	if (addr >= 0x08 && addr <= 0x0D)
	{
		if (busPrefetchCount & 0x1)
		{
			if (busPrefetchCount & 0x2)
			{
				busPrefetchCount = ((busPrefetchCount & 0xFF) >> 2) | (busPrefetchCount & 0xFFFFFF00);
				return 0;
			}
			busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
			return memoryWaitSeq[addr] - 1;
		}
		else
		{
			busPrefetchCount = 0;
			return memoryWait32[addr];
		}
	}
	else
	{
		busPrefetchCount = 0;
		return memoryWait32[addr];
	}
}

inline int codeTicksAccessSeq16(uint32_t address) // THUMB SEQ
{
	int addr = (address >> 24) & 15;

	if (addr >= 0x08 && addr <= 0x0D)
	{
		if (busPrefetchCount & 0x1)
		{
			busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
			return 0;
		}
		else if (busPrefetchCount > 0xFF)
		{
			busPrefetchCount = 0;
			return memoryWait[addr];
		}
		else
			return memoryWaitSeq[addr];
	}
	else
	{
		busPrefetchCount = 0;
		return memoryWaitSeq[addr];
	}
}

inline int codeTicksAccessSeq32(uint32_t address) // ARM SEQ
{
	int addr = (address >> 24) & 15;

	if (addr >= 0x08 && addr <= 0x0D)
	{
		if (busPrefetchCount & 0x1)
		{
			if (busPrefetchCount & 0x2)
			{
				busPrefetchCount = ((busPrefetchCount & 0xFF) >> 2) | (busPrefetchCount & 0xFFFFFF00);
				return 0;
			}
			busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
			return memoryWaitSeq[addr];
		}
		else if (busPrefetchCount > 0xFF)
		{
			busPrefetchCount = 0;
			return memoryWait32[addr];
		}
		else
			return memoryWaitSeq32[addr];
	}
	else
		return memoryWaitSeq32[addr];
}
