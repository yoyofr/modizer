/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include "ppu.h"

struct SOpcodes
{
	void (*S9xOpcode)();
};

struct SICPU
{
	SOpcodes *S9xOpcodes;
	uint8_t *S9xOpLengths;
	uint8_t _Carry;
	uint8_t _Zero;
	uint8_t _Negative;
	uint8_t _Overflow;
	uint32_t ShiftedPB;
	uint32_t ShiftedDB;
};

extern SICPU ICPU;

extern SOpcodes S9xOpcodesE1[256];
extern SOpcodes S9xOpcodesM1X1[256];
extern SOpcodes S9xOpcodesM1X0[256];
extern SOpcodes S9xOpcodesM0X1[256];
extern SOpcodes S9xOpcodesM0X0[256];
extern SOpcodes S9xOpcodesSlow[256];
extern uint8_t S9xOpLengthsM1X1[256];
extern uint8_t S9xOpLengthsM1X0[256];
extern uint8_t S9xOpLengthsM0X1[256];
extern uint8_t S9xOpLengthsM0X0[256];

void S9xMainLoop();
void S9xReset();
void S9xDoHEventProcessing();

#include "65c816.h"

inline void S9xUnpackStatus()
{
	ICPU._Zero = !(Registers.P.B.l & Zero);
	ICPU._Negative = Registers.P.B.l & Negative;
	ICPU._Carry = Registers.P.B.l & Carry;
	ICPU._Overflow = (Registers.P.B.l & Overflow) >> 6;
}

inline void S9xPackStatus()
{
	Registers.P.B.l &= ~(Zero | Negative | Carry | Overflow);
	Registers.P.B.l |= ICPU._Carry | ((!ICPU._Zero) << 1) | (ICPU._Negative & 0x80) | (ICPU._Overflow << 6);
}

inline void S9xFixCycles()
{
	if (CheckEmulation())
	{
		ICPU.S9xOpcodes = S9xOpcodesE1;
		ICPU.S9xOpLengths = S9xOpLengthsM1X1;
	}
	else if (CheckMemory())
	{
		if (CheckIndex())
		{
			ICPU.S9xOpcodes = S9xOpcodesM1X1;
			ICPU.S9xOpLengths = S9xOpLengthsM1X1;
		}
		else
		{
			ICPU.S9xOpcodes = S9xOpcodesM1X0;
			ICPU.S9xOpLengths = S9xOpLengthsM1X0;
		}
	}
	else
	{
		if (CheckIndex())
		{
			ICPU.S9xOpcodes = S9xOpcodesM0X1;
			ICPU.S9xOpLengths = S9xOpLengthsM0X1;
		}
		else
		{
			ICPU.S9xOpcodes = S9xOpcodesM0X0;
			ICPU.S9xOpLengths = S9xOpLengthsM0X0;
		}
	}
}
