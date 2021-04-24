/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "snes9x.h"
#include "memmap.h"
#include "apu/apu.h"

static inline void AddCycles(int32_t n) { CPU.Cycles += n; while (CPU.Cycles >= CPU.NextEvent) S9xDoHEventProcessing(); }

#include "cpuaddr.h"
#include "cpuops.h"
#include "cpumacro.h"

/* ADC ********************************************************************* */

template<typename T> static inline void ADC(T)
{
}

template<> inline void ADC<uint16_t>(uint16_t Work16)
{
	ADC16(Work16);
}

template<> inline void ADC<uint8_t>(uint8_t Work8)
{
	ADC8(Work8);
}

template<typename F> static inline void Op69Wrapper(F f)
{
	ADC(f(READ));
}

static void Op69M1()
{
	Op69Wrapper(Immediate8);
}

static void Op69M0()
{
	Op69Wrapper(Immediate16);
}

static void Op69Slow()
{
	if (CheckMemory())
		Op69Wrapper(Immediate8Slow);
	else
		Op69Wrapper(Immediate16Slow);
}

static void Op65M1() { rOP8(Direct, ADC8); }
static void Op65M0() { rOP16(Direct, ADC16, WRAP_BANK); }
static void Op65Slow() { rOPM(DirectSlow, ADC8, ADC16, WRAP_BANK); }

static void Op75E1() { rOP8(DirectIndexedXE1, ADC8); }
static void Op75E0M1() { rOP8(DirectIndexedXE0, ADC8); }
static void Op75E0M0() { rOP16(DirectIndexedXE0, ADC16, WRAP_BANK); }
static void Op75Slow() { rOPM(DirectIndexedXSlow, ADC8, ADC16, WRAP_BANK); }

static void Op72E1() { rOP8(DirectIndirectE1, ADC8); }
static void Op72E0M1() { rOP8(DirectIndirectE0, ADC8); }
static void Op72E0M0() { rOP16(DirectIndirectE0, ADC16); }
static void Op72Slow() { rOPM(DirectIndirectSlow, ADC8, ADC16); }

static void Op61E1() { rOP8(DirectIndexedIndirectE1, ADC8); }
static void Op61E0M1() { rOP8(DirectIndexedIndirectE0, ADC8); }
static void Op61E0M0() { rOP16(DirectIndexedIndirectE0, ADC16); }
static void Op61Slow() { rOPM(DirectIndexedIndirectSlow, ADC8, ADC16); }

static void Op71E1() { rOP8(DirectIndirectIndexedE1, ADC8); }
static void Op71E0M1X1() { rOP8(DirectIndirectIndexedE0X1, ADC8); }
static void Op71E0M0X1() { rOP16(DirectIndirectIndexedE0X1, ADC16); }
static void Op71E0M1X0() { rOP8(DirectIndirectIndexedE0X0, ADC8); }
static void Op71E0M0X0() { rOP16(DirectIndirectIndexedE0X0, ADC16); }
static void Op71Slow() { rOPM(DirectIndirectIndexedSlow, ADC8, ADC16); }

static void Op67M1() { rOP8(DirectIndirectLong, ADC8); }
static void Op67M0() { rOP16(DirectIndirectLong, ADC16); }
static void Op67Slow() { rOPM(DirectIndirectLongSlow, ADC8, ADC16); }

static void Op77M1() { rOP8(DirectIndirectIndexedLong, ADC8); }
static void Op77M0() { rOP16(DirectIndirectIndexedLong, ADC16); }
static void Op77Slow() { rOPM(DirectIndirectIndexedLongSlow, ADC8, ADC16); }

static void Op6DM1() { rOP8(Absolute, ADC8); }
static void Op6DM0() { rOP16(Absolute, ADC16); }
static void Op6DSlow() { rOPM(AbsoluteSlow, ADC8, ADC16); }

static void Op7DM1X1() { rOP8(AbsoluteIndexedXX1, ADC8); }
static void Op7DM0X1() { rOP16(AbsoluteIndexedXX1, ADC16); }
static void Op7DM1X0() { rOP8(AbsoluteIndexedXX0, ADC8); }
static void Op7DM0X0() { rOP16(AbsoluteIndexedXX0, ADC16); }
static void Op7DSlow() { rOPM(AbsoluteIndexedXSlow, ADC8, ADC16); }

static void Op79M1X1() { rOP8(AbsoluteIndexedYX1, ADC8); }
static void Op79M0X1() { rOP16(AbsoluteIndexedYX1, ADC16); }
static void Op79M1X0() { rOP8(AbsoluteIndexedYX0, ADC8); }
static void Op79M0X0() { rOP16(AbsoluteIndexedYX0, ADC16); }
static void Op79Slow() { rOPM(AbsoluteIndexedYSlow, ADC8, ADC16); }

static void Op6FM1() { rOP8(AbsoluteLong, ADC8); }
static void Op6FM0() { rOP16(AbsoluteLong, ADC16); }
static void Op6FSlow() { rOPM(AbsoluteLongSlow, ADC8, ADC16); }

static void Op7FM1() { rOP8(AbsoluteLongIndexedX, ADC8); }
static void Op7FM0() { rOP16(AbsoluteLongIndexedX, ADC16); }
static void Op7FSlow() { rOPM(AbsoluteLongIndexedXSlow, ADC8, ADC16); }

static void Op63M1() { rOP8(StackRelative, ADC8); }
static void Op63M0() { rOP16(StackRelative, ADC16); }
static void Op63Slow() { rOPM(StackRelativeSlow, ADC8, ADC16); }

static void Op73M1() { rOP8(StackRelativeIndirectIndexed, ADC8); }
static void Op73M0() { rOP16(StackRelativeIndirectIndexed, ADC16); }
static void Op73Slow() { rOPM(StackRelativeIndirectIndexedSlow, ADC8, ADC16); }

/* AND ********************************************************************* */

template<typename T, typename F> static inline void Op29Wrapper(T &reg, F f)
{
	reg &= f(READ);
	SetZN(reg);
}

static void Op29M1()
{
	Op29Wrapper(Registers.A.B.l, Immediate8);
}

static void Op29M0()
{
	Op29Wrapper(Registers.A.W, Immediate16);
}

static void Op29Slow()
{
	if (CheckMemory())
		Op29Wrapper(Registers.A.B.l, Immediate8Slow);
	else
		Op29Wrapper(Registers.A.W, Immediate16Slow);
}

static void Op25M1() { rOP8(Direct, AND8); }
static void Op25M0() { rOP16(Direct, AND16, WRAP_BANK); }
static void Op25Slow() { rOPM(DirectSlow, AND8, AND16, WRAP_BANK); }

static void Op35E1() { rOP8(DirectIndexedXE1, AND8); }
static void Op35E0M1() { rOP8(DirectIndexedXE0, AND8); }
static void Op35E0M0() { rOP16(DirectIndexedXE0, AND16, WRAP_BANK); }
static void Op35Slow() { rOPM(DirectIndexedXSlow, AND8, AND16, WRAP_BANK); }

static void Op32E1() { rOP8(DirectIndirectE1, AND8); }
static void Op32E0M1() { rOP8(DirectIndirectE0, AND8); }
static void Op32E0M0() { rOP16(DirectIndirectE0, AND16); }
static void Op32Slow() { rOPM(DirectIndirectSlow, AND8, AND16); }

static void Op21E1() { rOP8(DirectIndexedIndirectE1, AND8); }
static void Op21E0M1() { rOP8(DirectIndexedIndirectE0, AND8); }
static void Op21E0M0() { rOP16(DirectIndexedIndirectE0, AND16); }
static void Op21Slow() { rOPM(DirectIndexedIndirectSlow, AND8, AND16); }

static void Op31E1() { rOP8(DirectIndirectIndexedE1, AND8); }
static void Op31E0M1X1() { rOP8(DirectIndirectIndexedE0X1, AND8); }
static void Op31E0M0X1() { rOP16(DirectIndirectIndexedE0X1, AND16); }
static void Op31E0M1X0() { rOP8(DirectIndirectIndexedE0X0, AND8); }
static void Op31E0M0X0() { rOP16(DirectIndirectIndexedE0X0, AND16); }
static void Op31Slow() { rOPM(DirectIndirectIndexedSlow, AND8, AND16); }

static void Op27M1() { rOP8(DirectIndirectLong, AND8); }
static void Op27M0() { rOP16(DirectIndirectLong, AND16); }
static void Op27Slow() { rOPM(DirectIndirectLongSlow, AND8, AND16); }

static void Op37M1() { rOP8(DirectIndirectIndexedLong, AND8); }
static void Op37M0() { rOP16(DirectIndirectIndexedLong, AND16); }
static void Op37Slow() { rOPM(DirectIndirectIndexedLongSlow, AND8, AND16); }

static void Op2DM1() { rOP8(Absolute, AND8); }
static void Op2DM0() { rOP16(Absolute, AND16); }
static void Op2DSlow() { rOPM(AbsoluteSlow, AND8, AND16); }

static void Op3DM1X1() { rOP8(AbsoluteIndexedXX1, AND8); }
static void Op3DM0X1() { rOP16(AbsoluteIndexedXX1, AND16); }
static void Op3DM1X0() { rOP8(AbsoluteIndexedXX0, AND8); }
static void Op3DM0X0() { rOP16(AbsoluteIndexedXX0, AND16); }
static void Op3DSlow() { rOPM(AbsoluteIndexedXSlow, AND8, AND16); }

static void Op39M1X1() { rOP8(AbsoluteIndexedYX1, AND8); }
static void Op39M0X1() { rOP16(AbsoluteIndexedYX1, AND16); }
static void Op39M1X0() { rOP8(AbsoluteIndexedYX0, AND8); }
static void Op39M0X0() { rOP16(AbsoluteIndexedYX0, AND16); }
static void Op39Slow() { rOPM(AbsoluteIndexedYSlow, AND8, AND16); }

static void Op2FM1() { rOP8(AbsoluteLong, AND8); }
static void Op2FM0() { rOP16(AbsoluteLong, AND16); }
static void Op2FSlow() { rOPM(AbsoluteLongSlow, AND8, AND16); }

static void Op3FM1() { rOP8(AbsoluteLongIndexedX, AND8); }
static void Op3FM0() { rOP16(AbsoluteLongIndexedX, AND16); }
static void Op3FSlow() { rOPM(AbsoluteLongIndexedXSlow, AND8, AND16); }

static void Op23M1() { rOP8(StackRelative, AND8); }
static void Op23M0() { rOP16(StackRelative, AND16); }
static void Op23Slow() { rOPM(StackRelativeSlow, AND8, AND16); }

static void Op33M1() { rOP8(StackRelativeIndirectIndexed, AND8); }
static void Op33M0() { rOP16(StackRelativeIndirectIndexed, AND16); }
static void Op33Slow() { rOPM(StackRelativeIndirectIndexedSlow, AND8, AND16); }

/* ASL ********************************************************************* */

static inline void Op0AM1()
{
	AddCycles(ONE_CYCLE);
	ICPU._Carry = !!(Registers.A.B.l & 0x80);
	Registers.A.B.l <<= 1;
	SetZN(Registers.A.B.l);
}

static inline void Op0AM0()
{
	AddCycles(ONE_CYCLE);
	ICPU._Carry = !!(Registers.A.B.h & 0x80);
	Registers.A.W <<= 1;
	SetZN(Registers.A.W);
}

static void Op0ASlow()
{
	if (CheckMemory())
		Op0AM1();
	else
		Op0AM0();
}

static void Op06M1() { mOP8(Direct, ASL8); }
static void Op06M0() { mOP16(Direct, ASL16, WRAP_BANK); }
static void Op06Slow() { mOPM(DirectSlow, ASL8, ASL16, WRAP_BANK); }

static void Op16E1() { mOP8(DirectIndexedXE1, ASL8); }
static void Op16E0M1() { mOP8(DirectIndexedXE0, ASL8); }
static void Op16E0M0() { mOP16(DirectIndexedXE0, ASL16, WRAP_BANK); }
static void Op16Slow() { mOPM(DirectIndexedXSlow, ASL8, ASL16, WRAP_BANK); }

static void Op0EM1() { mOP8(Absolute, ASL8); }
static void Op0EM0() { mOP16(Absolute, ASL16); }
static void Op0ESlow() { mOPM(AbsoluteSlow, ASL8, ASL16); }

static void Op1EM1X1() { mOP8(AbsoluteIndexedXX1, ASL8); }
static void Op1EM0X1() { mOP16(AbsoluteIndexedXX1, ASL16); }
static void Op1EM1X0() { mOP8(AbsoluteIndexedXX0, ASL8); }
static void Op1EM0X0() { mOP16(AbsoluteIndexedXX0, ASL16); }
static void Op1ESlow() { mOPM(AbsoluteIndexedXSlow, ASL8, ASL16); }

/* BIT ********************************************************************* */

template<typename T, typename F> static inline void Op89Wrapper(const T &reg, F f)
{
	ICPU._Zero = !!(reg & f(READ));
}

static void Op89M1()
{
	Op89Wrapper(Registers.A.B.l, Immediate8);
}

static void Op89M0()
{
	Op89Wrapper(Registers.A.W, Immediate16);
}

static void Op89Slow()
{
	if (CheckMemory())
		Op89Wrapper(Registers.A.B.l, Immediate8Slow);
	else
		Op89Wrapper(Registers.A.W, Immediate16Slow);
}

static void Op24M1() { rOP8(Direct, BIT8); }
static void Op24M0() { rOP16(Direct, BIT16, WRAP_BANK); }
static void Op24Slow() { rOPM(DirectSlow, BIT8, BIT16, WRAP_BANK); }

static void Op34E1() { rOP8(DirectIndexedXE1, BIT8); }
static void Op34E0M1() { rOP8(DirectIndexedXE0, BIT8); }
static void Op34E0M0() { rOP16(DirectIndexedXE0, BIT16, WRAP_BANK); }
static void Op34Slow() { rOPM(DirectIndexedXSlow, BIT8, BIT16, WRAP_BANK); }

static void Op2CM1() { rOP8(Absolute, BIT8); }
static void Op2CM0() { rOP16(Absolute, BIT16); }
static void Op2CSlow() { rOPM(AbsoluteSlow, BIT8, BIT16); }

static void Op3CM1X1() { rOP8(AbsoluteIndexedXX1, BIT8); }
static void Op3CM0X1() { rOP16(AbsoluteIndexedXX1, BIT16); }
static void Op3CM1X0() { rOP8(AbsoluteIndexedXX0, BIT8); }
static void Op3CM0X0() { rOP16(AbsoluteIndexedXX0, BIT16); }
static void Op3CSlow() { rOPM(AbsoluteIndexedXSlow, BIT8, BIT16); }

/* CMP ********************************************************************* */

template<typename Tint, typename Treg, typename F> static inline void CxPxWrapper(const Treg &reg, F f)
{
	Tint intVal = static_cast<Tint>(reg) - static_cast<Tint>(f(READ));
	ICPU._Carry = intVal >= 0;
	SetZN(static_cast<Treg>(intVal));
}

static void OpC9M1()
{
	CxPxWrapper<int16_t>(Registers.A.B.l, Immediate8);
}

static void OpC9M0()
{
	CxPxWrapper<int32_t>(Registers.A.W, Immediate16);
}

static void OpC9Slow()
{
	if (CheckMemory())
		CxPxWrapper<int16_t>(Registers.A.B.l, Immediate8Slow);
	else
		CxPxWrapper<int32_t>(Registers.A.W, Immediate16Slow);
}

static void OpC5M1() { rOP8(Direct, CMP8); }
static void OpC5M0() { rOP16(Direct, CMP16, WRAP_BANK); }
static void OpC5Slow() { rOPM(DirectSlow, CMP8, CMP16, WRAP_BANK); }

static void OpD5E1() { rOP8(DirectIndexedXE1, CMP8); }
static void OpD5E0M1() { rOP8(DirectIndexedXE0, CMP8); }
static void OpD5E0M0() { rOP16(DirectIndexedXE0, CMP16, WRAP_BANK); }
static void OpD5Slow() { rOPM(DirectIndexedXSlow, CMP8, CMP16, WRAP_BANK); }

static void OpD2E1() { rOP8(DirectIndirectE1, CMP8); }
static void OpD2E0M1() { rOP8(DirectIndirectE0, CMP8); }
static void OpD2E0M0() { rOP16(DirectIndirectE0, CMP16); }
static void OpD2Slow() { rOPM(DirectIndirectSlow, CMP8, CMP16); }

static void OpC1E1() { rOP8(DirectIndexedIndirectE1, CMP8); }
static void OpC1E0M1() { rOP8(DirectIndexedIndirectE0, CMP8); }
static void OpC1E0M0() { rOP16(DirectIndexedIndirectE0, CMP16); }
static void OpC1Slow() { rOPM(DirectIndexedIndirectSlow, CMP8, CMP16); }

static void OpD1E1() { rOP8(DirectIndirectIndexedE1, CMP8); }
static void OpD1E0M1X1() { rOP8(DirectIndirectIndexedE0X1, CMP8); }
static void OpD1E0M0X1() { rOP16(DirectIndirectIndexedE0X1, CMP16); }
static void OpD1E0M1X0() { rOP8(DirectIndirectIndexedE0X0, CMP8); }
static void OpD1E0M0X0() { rOP16(DirectIndirectIndexedE0X0, CMP16); }
static void OpD1Slow() { rOPM(DirectIndirectIndexedSlow, CMP8, CMP16); }

static void OpC7M1() { rOP8(DirectIndirectLong, CMP8); }
static void OpC7M0() { rOP16(DirectIndirectLong, CMP16); }
static void OpC7Slow() { rOPM(DirectIndirectLongSlow, CMP8, CMP16); }

static void OpD7M1() { rOP8(DirectIndirectIndexedLong, CMP8); }
static void OpD7M0() { rOP16(DirectIndirectIndexedLong, CMP16); }
static void OpD7Slow() { rOPM(DirectIndirectIndexedLongSlow, CMP8, CMP16); }

static void OpCDM1() { rOP8(Absolute, CMP8); }
static void OpCDM0() { rOP16(Absolute, CMP16); }
static void OpCDSlow() { rOPM(AbsoluteSlow, CMP8, CMP16); }

static void OpDDM1X1() { rOP8(AbsoluteIndexedXX1, CMP8); }
static void OpDDM0X1() { rOP16(AbsoluteIndexedXX1, CMP16); }
static void OpDDM1X0() { rOP8(AbsoluteIndexedXX0, CMP8); }
static void OpDDM0X0() { rOP16(AbsoluteIndexedXX0, CMP16); }
static void OpDDSlow() { rOPM(AbsoluteIndexedXSlow, CMP8, CMP16); }

static void OpD9M1X1() { rOP8(AbsoluteIndexedYX1, CMP8); }
static void OpD9M0X1() { rOP16(AbsoluteIndexedYX1, CMP16); }
static void OpD9M1X0() { rOP8(AbsoluteIndexedYX0, CMP8); }
static void OpD9M0X0() { rOP16(AbsoluteIndexedYX0, CMP16); }
static void OpD9Slow() { rOPM(AbsoluteIndexedYSlow, CMP8, CMP16); }

static void OpCFM1() { rOP8(AbsoluteLong, CMP8); }
static void OpCFM0() { rOP16(AbsoluteLong, CMP16); }
static void OpCFSlow() { rOPM(AbsoluteLongSlow, CMP8, CMP16); }

static void OpDFM1() { rOP8(AbsoluteLongIndexedX, CMP8); }
static void OpDFM0() { rOP16(AbsoluteLongIndexedX, CMP16); }
static void OpDFSlow() { rOPM(AbsoluteLongIndexedXSlow, CMP8, CMP16); }

static void OpC3M1() { rOP8(StackRelative, CMP8); }
static void OpC3M0() { rOP16(StackRelative, CMP16); }
static void OpC3Slow() { rOPM(StackRelativeSlow, CMP8, CMP16); }

static void OpD3M1() { rOP8(StackRelativeIndirectIndexed, CMP8); }
static void OpD3M0() { rOP16(StackRelativeIndirectIndexed, CMP16); }
static void OpD3Slow() { rOPM(StackRelativeIndirectIndexedSlow, CMP8, CMP16); }

/* CPX ********************************************************************* */

static void OpE0X1()
{
	CxPxWrapper<int16_t>(Registers.X.B.l, Immediate8);
}

static void OpE0X0()
{
	CxPxWrapper<int32_t>(Registers.X.W, Immediate16);
}

static void OpE0Slow()
{
	if (CheckIndex())
		CxPxWrapper<int16_t>(Registers.X.B.l, Immediate8Slow);
	else
		CxPxWrapper<int32_t>(Registers.X.W, Immediate16Slow);
}

static void OpE4X1() { rOP8(Direct, CPX8); }
static void OpE4X0() { rOP16(Direct, CPX16, WRAP_BANK); }
static void OpE4Slow() { rOPX(DirectSlow, CPX8, CPX16, WRAP_BANK); }

static void OpECX1() { rOP8(Absolute, CPX8); }
static void OpECX0() { rOP16(Absolute, CPX16); }
static void OpECSlow() { rOPX(AbsoluteSlow, CPX8, CPX16); }

/* CPY ********************************************************************* */

static void OpC0X1()
{
	CxPxWrapper<int16_t>(Registers.Y.B.l, Immediate8);
}

static void OpC0X0()
{
	CxPxWrapper<int32_t>(Registers.Y.W, Immediate16);
}

static void OpC0Slow()
{
	if (CheckIndex())
		CxPxWrapper<int16_t>(Registers.Y.B.l, Immediate8Slow);
	else
		CxPxWrapper<int32_t>(Registers.Y.W, Immediate16Slow);
}

static void OpC4X1() { rOP8(Direct, CPY8); }
static void OpC4X0() { rOP16(Direct, CPY16, WRAP_BANK); }
static void OpC4Slow() { rOPX(DirectSlow, CPY8, CPY16, WRAP_BANK); }

static void OpCCX1() { rOP8(Absolute, CPY8); }
static void OpCCX0() { rOP16(Absolute, CPY16); }
static void OpCCSlow() { rOPX(AbsoluteSlow, CPY8, CPY16); }

/* DEC ********************************************************************* */

static inline void Op3AM1()
{
	AddCycles(ONE_CYCLE);
	--Registers.A.B.l;
	SetZN(Registers.A.B.l);
}

static inline void Op3AM0()
{
	AddCycles(ONE_CYCLE);
	--Registers.A.W;
	SetZN(Registers.A.W);
}

static void Op3ASlow()
{
	if (CheckMemory())
		Op3AM1();
	else
		Op3AM0();
}

static void OpC6M1() { mOP8(Direct, DEC8); }
static void OpC6M0() { mOP16(Direct, DEC16, WRAP_BANK); }
static void OpC6Slow() { mOPM(DirectSlow, DEC8, DEC16, WRAP_BANK); }

static void OpD6E1() { mOP8(DirectIndexedXE1, DEC8); }
static void OpD6E0M1() { mOP8(DirectIndexedXE0, DEC8); }
static void OpD6E0M0() { mOP16(DirectIndexedXE0, DEC16, WRAP_BANK); }
static void OpD6Slow() { mOPM(DirectIndexedXSlow, DEC8, DEC16, WRAP_BANK); }

static void OpCEM1() { mOP8(Absolute, DEC8); }
static void OpCEM0() { mOP16(Absolute, DEC16); }
static void OpCESlow() { mOPM(AbsoluteSlow, DEC8, DEC16); }

static void OpDEM1X1() { mOP8(AbsoluteIndexedXX1, DEC8); }
static void OpDEM0X1() { mOP16(AbsoluteIndexedXX1, DEC16); }
static void OpDEM1X0() { mOP8(AbsoluteIndexedXX0, DEC8); }
static void OpDEM0X0() { mOP16(AbsoluteIndexedXX0, DEC16); }
static void OpDESlow() { mOPM(AbsoluteIndexedXSlow, DEC8, DEC16); }

/* EOR ********************************************************************* */

template<typename T, typename F> static inline void Op49Wrapper(T &reg, F f)
{
	reg ^= f(READ);
	SetZN(reg);
}

static void Op49M1()
{
	Op49Wrapper(Registers.A.B.l, Immediate8);
}

static void Op49M0()
{
	Op49Wrapper(Registers.A.W, Immediate16);
}

static void Op49Slow()
{
	if (CheckMemory())
		Op49Wrapper(Registers.A.B.l, Immediate8Slow);
	else
		Op49Wrapper(Registers.A.W, Immediate16Slow);
}

static void Op45M1() { rOP8(Direct, EOR8); }
static void Op45M0() { rOP16(Direct, EOR16, WRAP_BANK); }
static void Op45Slow() { rOPM(DirectSlow, EOR8, EOR16, WRAP_BANK); }

static void Op55E1() { rOP8(DirectIndexedXE1, EOR8); }
static void Op55E0M1() { rOP8(DirectIndexedXE0, EOR8); }
static void Op55E0M0() { rOP16(DirectIndexedXE0, EOR16, WRAP_BANK); }
static void Op55Slow() { rOPM(DirectIndexedXSlow, EOR8, EOR16, WRAP_BANK); }

static void Op52E1() { rOP8(DirectIndirectE1, EOR8); }
static void Op52E0M1() { rOP8(DirectIndirectE0, EOR8); }
static void Op52E0M0() { rOP16(DirectIndirectE0, EOR16); }
static void Op52Slow() { rOPM(DirectIndirectSlow, EOR8, EOR16); }

static void Op41E1() { rOP8(DirectIndexedIndirectE1, EOR8); }
static void Op41E0M1() { rOP8(DirectIndexedIndirectE0, EOR8); }
static void Op41E0M0() { rOP16(DirectIndexedIndirectE0, EOR16); }
static void Op41Slow() { rOPM(DirectIndexedIndirectSlow, EOR8, EOR16); }

static void Op51E1() { rOP8(DirectIndirectIndexedE1, EOR8); }
static void Op51E0M1X1() { rOP8(DirectIndirectIndexedE0X1, EOR8); }
static void Op51E0M0X1() { rOP16(DirectIndirectIndexedE0X1, EOR16); }
static void Op51E0M1X0() { rOP8(DirectIndirectIndexedE0X0, EOR8); }
static void Op51E0M0X0() { rOP16(DirectIndirectIndexedE0X0, EOR16); }
static void Op51Slow() { rOPM(DirectIndirectIndexedSlow, EOR8, EOR16); }

static void Op47M1() { rOP8(DirectIndirectLong, EOR8); }
static void Op47M0() { rOP16(DirectIndirectLong, EOR16); }
static void Op47Slow() { rOPM(DirectIndirectLongSlow, EOR8, EOR16); }

static void Op57M1() { rOP8(DirectIndirectIndexedLong, EOR8); }
static void Op57M0() { rOP16(DirectIndirectIndexedLong, EOR16); }
static void Op57Slow() { rOPM(DirectIndirectIndexedLongSlow, EOR8, EOR16); }

static void Op4DM1() { rOP8(Absolute, EOR8); }
static void Op4DM0() { rOP16(Absolute, EOR16); }
static void Op4DSlow() { rOPM(AbsoluteSlow, EOR8, EOR16); }

static void Op5DM1X1() { rOP8(AbsoluteIndexedXX1, EOR8); }
static void Op5DM0X1() { rOP16(AbsoluteIndexedXX1, EOR16); }
static void Op5DM1X0() { rOP8(AbsoluteIndexedXX0, EOR8); }
static void Op5DM0X0() { rOP16(AbsoluteIndexedXX0, EOR16); }
static void Op5DSlow() { rOPM(AbsoluteIndexedXSlow, EOR8, EOR16); }

static void Op59M1X1() { rOP8(AbsoluteIndexedYX1, EOR8); }
static void Op59M0X1() { rOP16(AbsoluteIndexedYX1, EOR16); }
static void Op59M1X0() { rOP8(AbsoluteIndexedYX0, EOR8); }
static void Op59M0X0() { rOP16(AbsoluteIndexedYX0, EOR16); }
static void Op59Slow() { rOPM(AbsoluteIndexedYSlow, EOR8, EOR16); }

static void Op4FM1() { rOP8(AbsoluteLong, EOR8); }
static void Op4FM0() { rOP16(AbsoluteLong, EOR16); }
static void Op4FSlow() { rOPM(AbsoluteLongSlow, EOR8, EOR16); }

static void Op5FM1() { rOP8(AbsoluteLongIndexedX, EOR8); }
static void Op5FM0() { rOP16(AbsoluteLongIndexedX, EOR16); }
static void Op5FSlow() { rOPM(AbsoluteLongIndexedXSlow, EOR8, EOR16); }

static void Op43M1() { rOP8(StackRelative, EOR8); }
static void Op43M0() { rOP16(StackRelative, EOR16); }
static void Op43Slow() { rOPM(StackRelativeSlow, EOR8, EOR16); }

static void Op53M1() { rOP8(StackRelativeIndirectIndexed, EOR8); }
static void Op53M0() { rOP16(StackRelativeIndirectIndexed, EOR16); }
static void Op53Slow() { rOPM(StackRelativeIndirectIndexedSlow, EOR8, EOR16); }

/* INC ********************************************************************* */

static inline void Op1AM1()
{
	AddCycles(ONE_CYCLE);
	++Registers.A.B.l;
	SetZN(Registers.A.B.l);
}

static inline void Op1AM0()
{
	AddCycles(ONE_CYCLE);
	++Registers.A.W;
	SetZN(Registers.A.W);
}

static void Op1ASlow()
{
	if (CheckMemory())
		Op1AM1();
	else
		Op1AM0();
}

static void OpE6M1() { mOP8(Direct, INC8); }
static void OpE6M0() { mOP16(Direct, INC16, WRAP_BANK); }
static void OpE6Slow() { mOPM(DirectSlow, INC8, INC16, WRAP_BANK); }

static void OpF6E1() { mOP8(DirectIndexedXE1, INC8); }
static void OpF6E0M1() { mOP8(DirectIndexedXE0, INC8); }
static void OpF6E0M0() { mOP16(DirectIndexedXE0, INC16, WRAP_BANK); }
static void OpF6Slow() { mOPM(DirectIndexedXSlow, INC8, INC16, WRAP_BANK); }

static void OpEEM1() { mOP8(Absolute, INC8); }
static void OpEEM0() { mOP16(Absolute, INC16); }
static void OpEESlow() { mOPM(AbsoluteSlow, INC8, INC16); }

static void OpFEM1X1() { mOP8(AbsoluteIndexedXX1, INC8); }
static void OpFEM0X1() { mOP16(AbsoluteIndexedXX1, INC16); }
static void OpFEM1X0() { mOP8(AbsoluteIndexedXX0, INC8); }
static void OpFEM0X0() { mOP16(AbsoluteIndexedXX0, INC16); }
static void OpFESlow() { mOPM(AbsoluteIndexedXSlow, INC8, INC16); }

/* LDA ********************************************************************* */

template<typename T, typename F> static inline void LDWrapper(T &reg, F f)
{
	reg = f(READ);
	SetZN(reg);
}

static void OpA9M1()
{
	LDWrapper(Registers.A.B.l, Immediate8);
}

static void OpA9M0()
{
	LDWrapper(Registers.A.W, Immediate16);
}

static void OpA9Slow()
{
	if (CheckMemory())
		LDWrapper(Registers.A.B.l, Immediate8Slow);
	else
		LDWrapper(Registers.A.W, Immediate16Slow);
}

static void OpA5M1() { rOP8(Direct, LDA8); }
static void OpA5M0() { rOP16(Direct, LDA16, WRAP_BANK); }
static void OpA5Slow() { rOPM(DirectSlow, LDA8, LDA16, WRAP_BANK); }

static void OpB5E1() { rOP8(DirectIndexedXE1, LDA8); }
static void OpB5E0M1() { rOP8(DirectIndexedXE0, LDA8); }
static void OpB5E0M0() { rOP16(DirectIndexedXE0, LDA16, WRAP_BANK); }
static void OpB5Slow() { rOPM(DirectIndexedXSlow, LDA8, LDA16, WRAP_BANK); }

static void OpB2E1() { rOP8(DirectIndirectE1, LDA8); }
static void OpB2E0M1() { rOP8(DirectIndirectE0, LDA8); }
static void OpB2E0M0() { rOP16(DirectIndirectE0, LDA16); }
static void OpB2Slow() { rOPM(DirectIndirectSlow, LDA8, LDA16); }

static void OpA1E1() { rOP8(DirectIndexedIndirectE1, LDA8); }
static void OpA1E0M1() { rOP8(DirectIndexedIndirectE0, LDA8); }
static void OpA1E0M0() { rOP16(DirectIndexedIndirectE0, LDA16); }
static void OpA1Slow() { rOPM(DirectIndexedIndirectSlow, LDA8, LDA16); }

static void OpB1E1() { rOP8(DirectIndirectIndexedE1, LDA8); }
static void OpB1E0M1X1() { rOP8(DirectIndirectIndexedE0X1, LDA8); }
static void OpB1E0M0X1() { rOP16(DirectIndirectIndexedE0X1, LDA16); }
static void OpB1E0M1X0() { rOP8(DirectIndirectIndexedE0X0, LDA8); }
static void OpB1E0M0X0() { rOP16(DirectIndirectIndexedE0X0, LDA16); }
static void OpB1Slow() { rOPM(DirectIndirectIndexedSlow, LDA8, LDA16); }

static void OpA7M1() { rOP8(DirectIndirectLong, LDA8); }
static void OpA7M0() { rOP16(DirectIndirectLong, LDA16); }
static void OpA7Slow() { rOPM(DirectIndirectLongSlow, LDA8, LDA16); }

static void OpB7M1() { rOP8(DirectIndirectIndexedLong, LDA8); }
static void OpB7M0() { rOP16(DirectIndirectIndexedLong, LDA16); }
static void OpB7Slow() { rOPM(DirectIndirectIndexedLongSlow, LDA8, LDA16); }

static void OpADM1() { rOP8(Absolute, LDA8); }
static void OpADM0() { rOP16(Absolute, LDA16); }
static void OpADSlow() { rOPM(AbsoluteSlow, LDA8, LDA16); }

static void OpBDM1X1() { rOP8(AbsoluteIndexedXX1, LDA8); }
static void OpBDM0X1() { rOP16(AbsoluteIndexedXX1, LDA16); }
static void OpBDM1X0() { rOP8(AbsoluteIndexedXX0, LDA8); }
static void OpBDM0X0() { rOP16(AbsoluteIndexedXX0, LDA16); }
static void OpBDSlow() { rOPM(AbsoluteIndexedXSlow, LDA8, LDA16); }

static void OpB9M1X1() { rOP8(AbsoluteIndexedYX1, LDA8); }
static void OpB9M0X1() { rOP16(AbsoluteIndexedYX1, LDA16); }
static void OpB9M1X0() { rOP8(AbsoluteIndexedYX0, LDA8); }
static void OpB9M0X0() { rOP16(AbsoluteIndexedYX0, LDA16); }
static void OpB9Slow() { rOPM(AbsoluteIndexedYSlow, LDA8, LDA16); }

static void OpAFM1() { rOP8(AbsoluteLong, LDA8); }
static void OpAFM0() { rOP16(AbsoluteLong, LDA16); }
static void OpAFSlow() { rOPM(AbsoluteLongSlow, LDA8, LDA16); }

static void OpBFM1() { rOP8(AbsoluteLongIndexedX, LDA8); }
static void OpBFM0() { rOP16(AbsoluteLongIndexedX, LDA16); }
static void OpBFSlow() { rOPM(AbsoluteLongIndexedXSlow, LDA8, LDA16); }

static void OpA3M1() { rOP8(StackRelative, LDA8); }
static void OpA3M0() { rOP16(StackRelative, LDA16); }
static void OpA3Slow() { rOPM(StackRelativeSlow, LDA8, LDA16); }

static void OpB3M1() { rOP8(StackRelativeIndirectIndexed, LDA8); }
static void OpB3M0() { rOP16(StackRelativeIndirectIndexed, LDA16); }
static void OpB3Slow() { rOPM(StackRelativeIndirectIndexedSlow, LDA8, LDA16); }

/* LDX ********************************************************************* */

static void OpA2X1()
{
	LDWrapper(Registers.X.B.l, Immediate8);
}

static void OpA2X0()
{
	LDWrapper(Registers.X.W, Immediate16);
}

static void OpA2Slow()
{
	if (CheckIndex())
		LDWrapper(Registers.X.B.l, Immediate8Slow);
	else
		LDWrapper(Registers.X.W, Immediate16Slow);
}

static void OpA6X1() { rOP8(Direct, LDX8); }
static void OpA6X0() { rOP16(Direct, LDX16, WRAP_BANK); }
static void OpA6Slow() { rOPX(DirectSlow, LDX8, LDX16, WRAP_BANK); }

static void OpB6E1() { rOP8(DirectIndexedYE1, LDX8); }
static void OpB6E0X1() { rOP8(DirectIndexedYE0, LDX8); }
static void OpB6E0X0() { rOP16(DirectIndexedYE0, LDX16, WRAP_BANK); }
static void OpB6Slow() { rOPX(DirectIndexedYSlow, LDX8, LDX16, WRAP_BANK); }

static void OpAEX1() { rOP8(Absolute, LDX8); }
static void OpAEX0() { rOP16(Absolute, LDX16, WRAP_BANK); }
static void OpAESlow() { rOPX(AbsoluteSlow, LDX8, LDX16, WRAP_BANK); }

static void OpBEX1() { rOP8(AbsoluteIndexedYX1, LDX8); }
static void OpBEX0() { rOP16(AbsoluteIndexedYX0, LDX16, WRAP_BANK); }
static void OpBESlow() { rOPX(AbsoluteIndexedYSlow, LDX8, LDX16, WRAP_BANK); }

/* LDY ********************************************************************* */

static void OpA0X1()
{
	LDWrapper(Registers.Y.B.l, Immediate8);
}

static void OpA0X0()
{
	LDWrapper(Registers.Y.W, Immediate16);
}

static void OpA0Slow()
{
	if (CheckIndex())
		LDWrapper(Registers.Y.B.l, Immediate8Slow);
	else
		LDWrapper(Registers.Y.W, Immediate16Slow);
}

static void OpA4X1() { rOP8(Direct, LDY8); }
static void OpA4X0() { rOP16(Direct, LDY16, WRAP_BANK); }
static void OpA4Slow() { rOPX(DirectSlow, LDY8, LDY16, WRAP_BANK); }

static void OpB4E1() { rOP8(DirectIndexedXE1, LDY8); }
static void OpB4E0X1() { rOP8(DirectIndexedXE0, LDY8); }
static void OpB4E0X0() { rOP16(DirectIndexedXE0, LDY16, WRAP_BANK); }
static void OpB4Slow() { rOPX(DirectIndexedXSlow, LDY8, LDY16, WRAP_BANK); }

static void OpACX1() { rOP8(Absolute, LDY8); }
static void OpACX0() { rOP16(Absolute, LDY16, WRAP_BANK); }
static void OpACSlow() { rOPX(AbsoluteSlow, LDY8, LDY16, WRAP_BANK); }

static void OpBCX1() { rOP8(AbsoluteIndexedXX1, LDY8); }
static void OpBCX0() { rOP16(AbsoluteIndexedXX0, LDY16, WRAP_BANK); }
static void OpBCSlow() { rOPX(AbsoluteIndexedXSlow, LDY8, LDY16, WRAP_BANK); }

/* LSR ********************************************************************* */

static inline void Op4AM1()
{
	AddCycles(ONE_CYCLE);
	ICPU._Carry = Registers.A.B.l & 1;
	Registers.A.B.l >>= 1;
	SetZN(Registers.A.B.l);
}

static inline void Op4AM0()
{
	AddCycles(ONE_CYCLE);
	ICPU._Carry = Registers.A.W & 1;
	Registers.A.W >>= 1;
	SetZN(Registers.A.W);
}

static void Op4ASlow()
{
	if (CheckMemory())
		Op4AM1();
	else
		Op4AM0();
}

static void Op46M1() { mOP8(Direct, LSR8); }
static void Op46M0() { mOP16(Direct, LSR16, WRAP_BANK); }
static void Op46Slow() { mOPM(DirectSlow, LSR8, LSR16, WRAP_BANK); }

static void Op56E1() { mOP8(DirectIndexedXE1, LSR8); }
static void Op56E0M1() { mOP8(DirectIndexedXE0, LSR8); }
static void Op56E0M0() { mOP16(DirectIndexedXE0, LSR16, WRAP_BANK); }
static void Op56Slow() { mOPM(DirectIndexedXSlow, LSR8, LSR16, WRAP_BANK); }

static void Op4EM1() { mOP8(Absolute, LSR8); }
static void Op4EM0() { mOP16(Absolute, LSR16); }
static void Op4ESlow() { mOPM(AbsoluteSlow, LSR8, LSR16); }

static void Op5EM1X1() { mOP8(AbsoluteIndexedXX1, LSR8); }
static void Op5EM0X1() { mOP16(AbsoluteIndexedXX1, LSR16); }
static void Op5EM1X0() { mOP8(AbsoluteIndexedXX0, LSR8); }
static void Op5EM0X0() { mOP16(AbsoluteIndexedXX0, LSR16); }
static void Op5ESlow() { mOPM(AbsoluteIndexedXSlow, LSR8, LSR16); }

/* ORA ********************************************************************* */

template<typename T, typename F> static inline void Op09Wrapper(T &reg, F f)
{
	reg |= f(READ);
	SetZN(reg);
}

static void Op09M1()
{
	Op09Wrapper(Registers.A.B.l, Immediate8);
}

static void Op09M0()
{
	Op09Wrapper(Registers.A.W, Immediate16);
}

static void Op09Slow()
{
	if (CheckMemory())
		Op09Wrapper(Registers.A.B.l, Immediate8Slow);
	else
		Op09Wrapper(Registers.A.W, Immediate16Slow);
}

static void Op05M1() { rOP8(Direct, ORA8); }
static void Op05M0() { rOP16(Direct, ORA16, WRAP_BANK); }
static void Op05Slow() { rOPM(DirectSlow, ORA8, ORA16, WRAP_BANK); }

static void Op15E1() { rOP8(DirectIndexedXE1, ORA8); }
static void Op15E0M1() { rOP8(DirectIndexedXE0, ORA8); }
static void Op15E0M0() { rOP16(DirectIndexedXE0, ORA16, WRAP_BANK); }
static void Op15Slow() { rOPM(DirectIndexedXSlow, ORA8, ORA16, WRAP_BANK); }

static void Op12E1() { rOP8(DirectIndirectE1, ORA8); }
static void Op12E0M1() { rOP8(DirectIndirectE0, ORA8); }
static void Op12E0M0() { rOP16(DirectIndirectE0, ORA16); }
static void Op12Slow() { rOPM(DirectIndirectSlow, ORA8, ORA16); }

static void Op01E1() { rOP8(DirectIndexedIndirectE1, ORA8); }
static void Op01E0M1() { rOP8(DirectIndexedIndirectE0, ORA8); }
static void Op01E0M0() { rOP16(DirectIndexedIndirectE0, ORA16); }
static void Op01Slow() { rOPM(DirectIndexedIndirectSlow, ORA8, ORA16); }

static void Op11E1() { rOP8(DirectIndirectIndexedE1, ORA8); }
static void Op11E0M1X1() { rOP8(DirectIndirectIndexedE0X1, ORA8); }
static void Op11E0M0X1() { rOP16(DirectIndirectIndexedE0X1, ORA16); }
static void Op11E0M1X0() { rOP8(DirectIndirectIndexedE0X0, ORA8); }
static void Op11E0M0X0() { rOP16(DirectIndirectIndexedE0X0, ORA16); }
static void Op11Slow() { rOPM(DirectIndirectIndexedSlow, ORA8, ORA16); }

static void Op07M1() { rOP8(DirectIndirectLong, ORA8); }
static void Op07M0() { rOP16(DirectIndirectLong, ORA16); }
static void Op07Slow() { rOPM(DirectIndirectLongSlow, ORA8, ORA16); }

static void Op17M1() { rOP8(DirectIndirectIndexedLong, ORA8); }
static void Op17M0() { rOP16(DirectIndirectIndexedLong, ORA16); }
static void Op17Slow() { rOPM(DirectIndirectIndexedLongSlow, ORA8, ORA16); }

static void Op0DM1() { rOP8(Absolute, ORA8); }
static void Op0DM0() { rOP16(Absolute, ORA16); }
static void Op0DSlow() { rOPM(AbsoluteSlow, ORA8, ORA16); }

static void Op1DM1X1() { rOP8(AbsoluteIndexedXX1, ORA8); }
static void Op1DM0X1() { rOP16(AbsoluteIndexedXX1, ORA16); }
static void Op1DM1X0() { rOP8(AbsoluteIndexedXX0, ORA8); }
static void Op1DM0X0() { rOP16(AbsoluteIndexedXX0, ORA16); }
static void Op1DSlow() { rOPM(AbsoluteIndexedXSlow, ORA8, ORA16); }

static void Op19M1X1() { rOP8(AbsoluteIndexedYX1, ORA8); }
static void Op19M0X1() { rOP16(AbsoluteIndexedYX1, ORA16); }
static void Op19M1X0() { rOP8(AbsoluteIndexedYX0, ORA8); }
static void Op19M0X0() { rOP16(AbsoluteIndexedYX0, ORA16); }
static void Op19Slow() { rOPM(AbsoluteIndexedYSlow, ORA8, ORA16); }

static void Op0FM1() { rOP8(AbsoluteLong, ORA8); }
static void Op0FM0() { rOP16(AbsoluteLong, ORA16); }
static void Op0FSlow() { rOPM(AbsoluteLongSlow, ORA8, ORA16); }

static void Op1FM1() { rOP8(AbsoluteLongIndexedX, ORA8); }
static void Op1FM0() { rOP16(AbsoluteLongIndexedX, ORA16); }
static void Op1FSlow() { rOPM(AbsoluteLongIndexedXSlow, ORA8, ORA16); }

static void Op03M1() { rOP8(StackRelative, ORA8); }
static void Op03M0() { rOP16(StackRelative, ORA16); }
static void Op03Slow() { rOPM(StackRelativeSlow, ORA8, ORA16); }

static void Op13M1() { rOP8(StackRelativeIndirectIndexed, ORA8); }
static void Op13M0() { rOP16(StackRelativeIndirectIndexed, ORA16); }
static void Op13Slow() { rOPM(StackRelativeIndirectIndexedSlow, ORA8, ORA16); }

/* ROL ********************************************************************* */

static inline void Op2AM1()
{
	AddCycles(ONE_CYCLE);
	uint16_t w = (static_cast<uint16_t>(Registers.A.B.l) << 1) | static_cast<uint16_t>(CheckCarry());
	ICPU._Carry = w >= 0x100;
	Registers.A.B.l = static_cast<uint8_t>(w);
	SetZN(Registers.A.B.l);
}

static inline void Op2AM0()
{
	AddCycles(ONE_CYCLE);
	uint32_t w = (static_cast<uint32_t>(Registers.A.W) << 1) | static_cast<uint32_t>(CheckCarry());
	ICPU._Carry = w >= 0x10000;
	Registers.A.W = static_cast<uint16_t>(w);
	SetZN(Registers.A.W);
}

static void Op2ASlow()
{
	if (CheckMemory())
		Op2AM1();
	else
		Op2AM0();
}

static void Op26M1() { mOP8(Direct, ROL8); }
static void Op26M0() { mOP16(Direct, ROL16, WRAP_BANK); }
static void Op26Slow() { mOPM(DirectSlow, ROL8, ROL16, WRAP_BANK); }

static void Op36E1() { mOP8(DirectIndexedXE1, ROL8); }
static void Op36E0M1() { mOP8(DirectIndexedXE0, ROL8); }
static void Op36E0M0() { mOP16(DirectIndexedXE0, ROL16, WRAP_BANK); }
static void Op36Slow() { mOPM(DirectIndexedXSlow, ROL8, ROL16, WRAP_BANK); }

static void Op2EM1() { mOP8(Absolute, ROL8); }
static void Op2EM0() { mOP16(Absolute, ROL16); }
static void Op2ESlow() { mOPM(AbsoluteSlow, ROL8, ROL16); }

static void Op3EM1X1() { mOP8(AbsoluteIndexedXX1, ROL8); }
static void Op3EM0X1() { mOP16(AbsoluteIndexedXX1, ROL16); }
static void Op3EM1X0() { mOP8(AbsoluteIndexedXX0, ROL8); }
static void Op3EM0X0() { mOP16(AbsoluteIndexedXX0, ROL16); }
static void Op3ESlow() { mOPM(AbsoluteIndexedXSlow, ROL8, ROL16); }

/* ROR ********************************************************************* */

static inline void Op6AM1()
{
	AddCycles(ONE_CYCLE);
	uint16_t w = static_cast<uint16_t>(Registers.A.B.l) | (static_cast<uint16_t>(CheckCarry()) << 8);
	ICPU._Carry = w & 1;
	w >>= 1;
	Registers.A.B.l = static_cast<uint8_t>(w);
	SetZN(Registers.A.B.l);
}

static inline void Op6AM0()
{
	AddCycles(ONE_CYCLE);
	uint32_t w = static_cast<uint32_t>(Registers.A.W) | (static_cast<uint32_t>(CheckCarry()) << 16);
	ICPU._Carry = w & 1;
	w >>= 1;
	Registers.A.W = static_cast<uint16_t>(w);
	SetZN(Registers.A.W);
}

static void Op6ASlow()
{
	if (CheckMemory())
		Op6AM1();
	else
		Op6AM0();
}

static void Op66M1() { mOP8(Direct, ROR8); }
static void Op66M0() { mOP16(Direct, ROR16, WRAP_BANK); }
static void Op66Slow() { mOPM(DirectSlow, ROR8, ROR16, WRAP_BANK); }

static void Op76E1() { mOP8(DirectIndexedXE1, ROR8); }
static void Op76E0M1() { mOP8(DirectIndexedXE0, ROR8); }
static void Op76E0M0() { mOP16(DirectIndexedXE0, ROR16, WRAP_BANK); }
static void Op76Slow() { mOPM(DirectIndexedXSlow, ROR8, ROR16, WRAP_BANK); }

static void Op6EM1() { mOP8(Absolute, ROR8); }
static void Op6EM0() { mOP16(Absolute, ROR16); }
static void Op6ESlow() { mOPM(AbsoluteSlow, ROR8, ROR16); }

static void Op7EM1X1() { mOP8(AbsoluteIndexedXX1, ROR8); }
static void Op7EM0X1() { mOP16(AbsoluteIndexedXX1, ROR16); }
static void Op7EM1X0() { mOP8(AbsoluteIndexedXX0, ROR8); }
static void Op7EM0X0() { mOP16(AbsoluteIndexedXX0, ROR16); }
static void Op7ESlow() { mOPM(AbsoluteIndexedXSlow, ROR8, ROR16); }

/* SBC ********************************************************************* */

template<typename T> static inline void SBC(T)
{
}

template<> inline void SBC<uint16_t>(uint16_t Work16)
{
	SBC16(Work16);
}

template<> inline void SBC<uint8_t>(uint8_t Work8)
{
	SBC8(Work8);
}

template<typename F> static inline void OpE9Wrapper(F f)
{
	SBC(f(READ));
}

static void OpE9M1()
{
	OpE9Wrapper(Immediate8);
}

static void OpE9M0()
{
	OpE9Wrapper(Immediate16);
}

static void OpE9Slow()
{
	if (CheckMemory())
		OpE9Wrapper(Immediate8Slow);
	else
		OpE9Wrapper(Immediate16Slow);
}

static void OpE5M1() { rOP8(Direct, SBC8); }
static void OpE5M0() { rOP16(Direct, SBC16, WRAP_BANK); }
static void OpE5Slow() { rOPM(DirectSlow, SBC8, SBC16, WRAP_BANK); }

static void OpF5E1() { rOP8(DirectIndexedXE1, SBC8); }
static void OpF5E0M1() { rOP8(DirectIndexedXE0, SBC8); }
static void OpF5E0M0() { rOP16(DirectIndexedXE0, SBC16, WRAP_BANK); }
static void OpF5Slow() { rOPM(DirectIndexedXSlow, SBC8, SBC16, WRAP_BANK); }

static void OpF2E1() { rOP8(DirectIndirectE1, SBC8); }
static void OpF2E0M1() { rOP8(DirectIndirectE0, SBC8); }
static void OpF2E0M0() { rOP16(DirectIndirectE0, SBC16); }
static void OpF2Slow() { rOPM(DirectIndirectSlow, SBC8, SBC16); }

static void OpE1E1() { rOP8(DirectIndexedIndirectE1, SBC8); }
static void OpE1E0M1() { rOP8(DirectIndexedIndirectE0, SBC8); }
static void OpE1E0M0() { rOP16(DirectIndexedIndirectE0, SBC16); }
static void OpE1Slow() { rOPM(DirectIndexedIndirectSlow, SBC8, SBC16); }

static void OpF1E1() { rOP8(DirectIndirectIndexedE1, SBC8); }
static void OpF1E0M1X1() { rOP8(DirectIndirectIndexedE0X1, SBC8); }
static void OpF1E0M0X1() { rOP16(DirectIndirectIndexedE0X1, SBC16); }
static void OpF1E0M1X0() { rOP8(DirectIndirectIndexedE0X0, SBC8); }
static void OpF1E0M0X0() { rOP16(DirectIndirectIndexedE0X0, SBC16); }
static void OpF1Slow() { rOPM(DirectIndirectIndexedSlow, SBC8, SBC16); }

static void OpE7M1() { rOP8(DirectIndirectLong, SBC8); }
static void OpE7M0() { rOP16(DirectIndirectLong, SBC16); }
static void OpE7Slow() { rOPM(DirectIndirectLongSlow, SBC8, SBC16); }

static void OpF7M1() { rOP8(DirectIndirectIndexedLong, SBC8); }
static void OpF7M0() { rOP16(DirectIndirectIndexedLong, SBC16); }
static void OpF7Slow() { rOPM(DirectIndirectIndexedLongSlow, SBC8, SBC16); }

static void OpEDM1() { rOP8(Absolute, SBC8); }
static void OpEDM0() { rOP16(Absolute, SBC16); }
static void OpEDSlow() { rOPM(AbsoluteSlow, SBC8, SBC16); }

static void OpFDM1X1() { rOP8(AbsoluteIndexedXX1, SBC8); }
static void OpFDM0X1() { rOP16(AbsoluteIndexedXX1, SBC16); }
static void OpFDM1X0() { rOP8(AbsoluteIndexedXX0, SBC8); }
static void OpFDM0X0() { rOP16(AbsoluteIndexedXX0, SBC16); }
static void OpFDSlow() { rOPM(AbsoluteIndexedXSlow, SBC8, SBC16); }

static void OpF9M1X1() { rOP8(AbsoluteIndexedYX1, SBC8); }
static void OpF9M0X1() { rOP16(AbsoluteIndexedYX1, SBC16); }
static void OpF9M1X0() { rOP8(AbsoluteIndexedYX0, SBC8); }
static void OpF9M0X0() { rOP16(AbsoluteIndexedYX0, SBC16); }
static void OpF9Slow() { rOPM(AbsoluteIndexedYSlow, SBC8, SBC16); }

static void OpEFM1() { rOP8(AbsoluteLong, SBC8); }
static void OpEFM0() { rOP16(AbsoluteLong, SBC16); }
static void OpEFSlow() { rOPM(AbsoluteLongSlow, SBC8, SBC16); }

static void OpFFM1() { rOP8(AbsoluteLongIndexedX, SBC8); }
static void OpFFM0() { rOP16(AbsoluteLongIndexedX, SBC16); }
static void OpFFSlow() { rOPM(AbsoluteLongIndexedXSlow, SBC8, SBC16); }

static void OpE3M1() { rOP8(StackRelative, SBC8); }
static void OpE3M0() { rOP16(StackRelative, SBC16); }
static void OpE3Slow() { rOPM(StackRelativeSlow, SBC8, SBC16); }

static void OpF3M1() { rOP8(StackRelativeIndirectIndexed, SBC8); }
static void OpF3M0() { rOP16(StackRelativeIndirectIndexed, SBC16); }
static void OpF3Slow() { rOPM(StackRelativeIndirectIndexedSlow, SBC8, SBC16); }

/* STA ********************************************************************* */

static void Op85M1() { wOP8(Direct, STA8); }
static void Op85M0() { wOP16(Direct, STA16, WRAP_BANK); }
static void Op85Slow() { wOPM(DirectSlow, STA8, STA16, WRAP_BANK); }

static void Op95E1() { wOP8(DirectIndexedXE1, STA8); }
static void Op95E0M1() { wOP8(DirectIndexedXE0, STA8); }
static void Op95E0M0() { wOP16(DirectIndexedXE0, STA16, WRAP_BANK); }
static void Op95Slow() { wOPM(DirectIndexedXSlow, STA8, STA16, WRAP_BANK); }

static void Op92E1() { wOP8(DirectIndirectE1, STA8); }
static void Op92E0M1() { wOP8(DirectIndirectE0, STA8); }
static void Op92E0M0() { wOP16(DirectIndirectE0, STA16); }
static void Op92Slow() { wOPM(DirectIndirectSlow, STA8, STA16); }

static void Op81E1() { wOP8(DirectIndexedIndirectE1, STA8); }
static void Op81E0M1() { wOP8(DirectIndexedIndirectE0, STA8); }
static void Op81E0M0() { wOP16(DirectIndexedIndirectE0, STA16); }
static void Op81Slow() { wOPM(DirectIndexedIndirectSlow, STA8, STA16); }

static void Op91E1() { wOP8(DirectIndirectIndexedE1, STA8); }
static void Op91E0M1X1() { wOP8(DirectIndirectIndexedE0X1, STA8); }
static void Op91E0M0X1() { wOP16(DirectIndirectIndexedE0X1, STA16); }
static void Op91E0M1X0() { wOP8(DirectIndirectIndexedE0X0, STA8); }
static void Op91E0M0X0() { wOP16(DirectIndirectIndexedE0X0, STA16); }
static void Op91Slow() { wOPM(DirectIndirectIndexedSlow, STA8, STA16); }

static void Op87M1() { wOP8(DirectIndirectLong, STA8); }
static void Op87M0() { wOP16(DirectIndirectLong, STA16); }
static void Op87Slow() { wOPM(DirectIndirectLongSlow, STA8, STA16); }

static void Op97M1() { wOP8(DirectIndirectIndexedLong, STA8); }
static void Op97M0() { wOP16(DirectIndirectIndexedLong, STA16); }
static void Op97Slow() { wOPM(DirectIndirectIndexedLongSlow, STA8, STA16); }

static void Op8DM1() { wOP8(Absolute, STA8); }
static void Op8DM0() { wOP16(Absolute, STA16); }
static void Op8DSlow() { wOPM(AbsoluteSlow, STA8, STA16); }

static void Op9DM1X1() { wOP8(AbsoluteIndexedXX1, STA8); }
static void Op9DM0X1() { wOP16(AbsoluteIndexedXX1, STA16); }
static void Op9DM1X0() { wOP8(AbsoluteIndexedXX0, STA8); }
static void Op9DM0X0() { wOP16(AbsoluteIndexedXX0, STA16); }
static void Op9DSlow() { wOPM(AbsoluteIndexedXSlow, STA8, STA16); }

static void Op99M1X1() { wOP8(AbsoluteIndexedYX1, STA8); }
static void Op99M0X1() { wOP16(AbsoluteIndexedYX1, STA16); }
static void Op99M1X0() { wOP8(AbsoluteIndexedYX0, STA8); }
static void Op99M0X0() { wOP16(AbsoluteIndexedYX0, STA16); }
static void Op99Slow() { wOPM(AbsoluteIndexedYSlow, STA8, STA16); }

static void Op8FM1() { wOP8(AbsoluteLong, STA8); }
static void Op8FM0() { wOP16(AbsoluteLong, STA16); }
static void Op8FSlow() { wOPM(AbsoluteLongSlow, STA8, STA16); }

static void Op9FM1() { wOP8(AbsoluteLongIndexedX, STA8); }
static void Op9FM0() { wOP16(AbsoluteLongIndexedX, STA16); }
static void Op9FSlow() { wOPM(AbsoluteLongIndexedXSlow, STA8, STA16); }

static void Op83M1() { wOP8(StackRelative, STA8); }
static void Op83M0() { wOP16(StackRelative, STA16); }
static void Op83Slow() { wOPM(StackRelativeSlow, STA8, STA16); }

static void Op93M1() { wOP8(StackRelativeIndirectIndexed, STA8); }
static void Op93M0() { wOP16(StackRelativeIndirectIndexed, STA16); }
static void Op93Slow() { wOPM(StackRelativeIndirectIndexedSlow, STA8, STA16); }

/* STX ********************************************************************* */

static void Op86X1() { wOP8(Direct, STX8); }
static void Op86X0() { wOP16(Direct, STX16, WRAP_BANK); }
static void Op86Slow() { wOPX(DirectSlow, STX8, STX16, WRAP_BANK); }

static void Op96E1() { wOP8(DirectIndexedYE1, STX8); }
static void Op96E0X1() { wOP8(DirectIndexedYE0, STX8); }
static void Op96E0X0() { wOP16(DirectIndexedYE0, STX16, WRAP_BANK); }
static void Op96Slow() { wOPX(DirectIndexedYSlow, STX8, STX16, WRAP_BANK); }

static void Op8EX1() { wOP8(Absolute, STX8); }
static void Op8EX0() { wOP16(Absolute, STX16, WRAP_BANK); }
static void Op8ESlow() { wOPX(AbsoluteSlow, STX8, STX16, WRAP_BANK); }

/* STY ********************************************************************* */

static void Op84X1() { wOP8(Direct, STY8); }
static void Op84X0() { wOP16(Direct, STY16, WRAP_BANK); }
static void Op84Slow() { wOPX(DirectSlow, STY8, STY16, WRAP_BANK); }

static void Op94E1() { wOP8(DirectIndexedXE1, STY8); }
static void Op94E0X1() { wOP8(DirectIndexedXE0, STY8); }
static void Op94E0X0() { wOP16(DirectIndexedXE0, STY16, WRAP_BANK); }
static void Op94Slow() { wOPX(DirectIndexedXSlow, STY8, STY16, WRAP_BANK); }

static void Op8CX1() { wOP8(Absolute, STY8); }
static void Op8CX0() { wOP16(Absolute, STY16, WRAP_BANK); }
static void Op8CSlow() { wOPX(AbsoluteSlow, STY8, STY16, WRAP_BANK); }

/* STZ ********************************************************************* */

static void Op64M1() { wOP8(Direct, STZ8); }
static void Op64M0() { wOP16(Direct, STZ16, WRAP_BANK); }
static void Op64Slow() { wOPM(DirectSlow, STZ8, STZ16, WRAP_BANK); }

static void Op74E1() { wOP8(DirectIndexedXE1, STZ8); }
static void Op74E0M1() { wOP8(DirectIndexedXE0, STZ8); }
static void Op74E0M0() { wOP16(DirectIndexedXE0, STZ16, WRAP_BANK); }
static void Op74Slow() { wOPM(DirectIndexedXSlow, STZ8, STZ16, WRAP_BANK); }

static void Op9CM1() { wOP8(Absolute, STZ8); }
static void Op9CM0() { wOP16(Absolute, STZ16); }
static void Op9CSlow() { wOPM(AbsoluteSlow, STZ8, STZ16); }

static void Op9EM1X1() { wOP8(AbsoluteIndexedXX1, STZ8); }
static void Op9EM0X1() { wOP16(AbsoluteIndexedXX1, STZ16); }
static void Op9EM1X0() { wOP8(AbsoluteIndexedXX0, STZ8); }
static void Op9EM0X0() { wOP16(AbsoluteIndexedXX0, STZ16); }
static void Op9ESlow() { wOPM(AbsoluteIndexedXSlow, STZ8, STZ16); }

/* TRB ********************************************************************* */

static void Op14M1() { mOP8(Direct, TRB8); }
static void Op14M0() { mOP16(Direct, TRB16, WRAP_BANK); }
static void Op14Slow() { mOPM(DirectSlow, TRB8, TRB16, WRAP_BANK); }

static void Op1CM1() { mOP8(Absolute, TRB8); }
static void Op1CM0() { mOP16(Absolute, TRB16, WRAP_BANK); }
static void Op1CSlow() { mOPM(AbsoluteSlow, TRB8, TRB16, WRAP_BANK); }

/* TSB ********************************************************************* */

static void Op04M1() { mOP8(Direct, TSB8); }
static void Op04M0() { mOP16(Direct, TSB16, WRAP_BANK); }
static void Op04Slow() { mOPM(DirectSlow, TSB8, TSB16, WRAP_BANK); }

static void Op0CM1() { mOP8(Absolute, TSB8); }
static void Op0CM0() { mOP16(Absolute, TSB16, WRAP_BANK); }
static void Op0CSlow() { mOPM(AbsoluteSlow, TSB8, TSB16, WRAP_BANK); }

/* Branch Instructions ***************************************************** */

// BCC
static void Op90E0() { bOP(Relative, !CheckCarry(), false); }
static void Op90E1() { bOP(Relative, !CheckCarry(), true); }
static void Op90Slow() { bOP(RelativeSlow, !CheckCarry(), CheckEmulation()); }

// BCS
static void OpB0E0() { bOP(Relative, CheckCarry(), false); }
static void OpB0E1() { bOP(Relative, CheckCarry(), true); }
static void OpB0Slow() { bOP(RelativeSlow, CheckCarry(), CheckEmulation()); }

// BEQ
static void OpF0E0() { bOP(Relative, CheckZero(), false); }
static void OpF0E1() { bOP(Relative, CheckZero(), true); }
static void OpF0Slow() { bOP(RelativeSlow, CheckZero(), CheckEmulation()); }

// BMI
static void Op30E0() { bOP(Relative, CheckNegative(), false); }
static void Op30E1() { bOP(Relative, CheckNegative(), true); }
static void Op30Slow() { bOP(RelativeSlow, CheckNegative(), CheckEmulation()); }

// BNE
static void OpD0E0() { bOP(Relative, !CheckZero(), false); }
static void OpD0E1() { bOP(Relative, !CheckZero(), true); }
static void OpD0Slow() { bOP(RelativeSlow, !CheckZero(), CheckEmulation()); }

// BPL
static void Op10E0() { bOP(Relative, !CheckNegative(), false); }
static void Op10E1() { bOP(Relative, !CheckNegative(), true); }
static void Op10Slow() { bOP(RelativeSlow, !CheckNegative(), CheckEmulation()); }

// BRA
static void Op80E0() { bOP(Relative, true, false); }
static void Op80E1() { bOP(Relative, true, true); }
static void Op80Slow() { bOP(RelativeSlow, true, CheckEmulation()); }

// BVC
static void Op50E0() { bOP(Relative, !CheckOverflow(), false); }
static void Op50E1() { bOP(Relative, !CheckOverflow(), true); }
static void Op50Slow() { bOP(RelativeSlow, !CheckOverflow(), CheckEmulation()); }

// BVS
static void Op70E0() { bOP(Relative, CheckOverflow(), false); }
static void Op70E1() { bOP(Relative, CheckOverflow(), true); }
static void Op70Slow() { bOP(RelativeSlow, CheckOverflow(), CheckEmulation()); }

// BRL
template<typename F> static inline void Op82Wrapper(F f)
{
	S9xSetPCBase(ICPU.ShiftedPB + f(JUMP));
}

static void Op82()
{
	Op82Wrapper(RelativeLong);
}

static void Op82Slow()
{
	Op82Wrapper(RelativeLongSlow);
}

/* Flag Instructions ******************************************************* */

// CLC
static void Op18()
{
	ClearCarry();
	AddCycles(ONE_CYCLE);
}

// SEC
static void Op38()
{
	SetCarry();
	AddCycles(ONE_CYCLE);
}

// CLD
static void OpD8()
{
	ClearDecimal();
	AddCycles(ONE_CYCLE);
}

// SED
static void OpF8()
{
	SetDecimal();
	AddCycles(ONE_CYCLE);
}

// CLI
static void Op58()
{
	AddCycles(ONE_CYCLE);

	Timings.IRQFlagChanging |= IRQ_CLEAR_FLAG;
}

// SEI
static void Op78()
{
	AddCycles(ONE_CYCLE);

	Timings.IRQFlagChanging |= IRQ_SET_FLAG;
}

// CLV
static void OpB8()
{
	ClearOverflow();
	AddCycles(ONE_CYCLE);
}

/* DEX/DEY ***************************************************************** */

template<typename T> static inline void DEWrapper(T &reg)
{
	AddCycles(ONE_CYCLE);
	--reg;
	SetZN(reg);
}

static inline void OpCAX1()
{
	DEWrapper(Registers.X.B.l);
}

static inline void OpCAX0()
{
	DEWrapper(Registers.X.W);
}

static void OpCASlow()
{
	if (CheckIndex())
		OpCAX1();
	else
		OpCAX0();
}

static inline void Op88X1()
{
	DEWrapper(Registers.Y.B.l);
}

static inline void Op88X0()
{
	DEWrapper(Registers.Y.W);
}

static void Op88Slow()
{
	if (CheckIndex())
		Op88X1();
	else
		Op88X0();
}

/* INX/INY ***************************************************************** */

template<typename T> static inline void INWrapper(T &reg)
{
	AddCycles(ONE_CYCLE);
	++reg;
	SetZN(reg);
}

static inline void OpE8X1()
{
	INWrapper(Registers.X.B.l);
}

static inline void OpE8X0()
{
	INWrapper(Registers.X.W);
}

static void OpE8Slow()
{
	if (CheckIndex())
		OpE8X1();
	else
		OpE8X0();
}

static inline void OpC8X1()
{
	INWrapper(Registers.Y.B.l);
}

static inline void OpC8X0()
{
	INWrapper(Registers.Y.W);
}

static void OpC8Slow()
{
	if (CheckIndex())
		OpC8X1();
	else
		OpC8X0();
}

/* NOP ********************************************************************* */

static void OpEA()
{
	AddCycles(ONE_CYCLE);
}

/* PUSH Instructions ******************************************************* */

static inline void PushW(uint16_t w)
{
	S9xSetWord(w, Registers.S.W - 1, WRAP_BANK, WRITE_10);
	Registers.S.W -= 2;
}

static inline void PushWE(uint16_t w)
{
	--Registers.S.B.l;
	S9xSetWord(w, Registers.S.W, WRAP_PAGE, WRITE_10);
	--Registers.S.B.l;
}

static inline void PushB(uint8_t b)
{
	S9xSetByte(b, Registers.S.W--);
}

static inline void PushBE(uint8_t b)
{
	S9xSetByte(b, Registers.S.W);
	--Registers.S.B.l;
}

template<typename F> static inline void PEWrapper(F f, bool cond)
{
	uint16_t val = static_cast<uint16_t>(f(NONE));
	PushW(val);
	OpenBus = val & 0xff;
	if (cond)
		Registers.S.B.h = 1;
}

// PEA
static void OpF4E0()
{
	PEWrapper(Absolute, false);
}

static void OpF4E1()
{
	// Note: PEA is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PEWrapper(Absolute, true);
}

static void OpF4Slow()
{
	PEWrapper(AbsoluteSlow, CheckEmulation());
}

// PEI
static void OpD4E0()
{
	PEWrapper(DirectIndirectE0, false);
}

static void OpD4E1()
{
	// Note: PEI is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PEWrapper(DirectIndirectE1, true);
}

static void OpD4Slow()
{
	PEWrapper(DirectIndirectSlow, CheckEmulation());
}

// PER
static void Op62E0()
{
	PEWrapper(RelativeLong, false);
}

static void Op62E1()
{
	// Note: PER is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PEWrapper(RelativeLong, true);
}

static void Op62Slow()
{
	PEWrapper(RelativeLongSlow, CheckEmulation());
}

template<typename Treg, typename Tob, typename F> static inline void PHWrapper(const Treg &pushReg, const Tob &obReg, F f, bool cond)
{
	AddCycles(ONE_CYCLE);
	f(pushReg);
	OpenBus = obReg;
	if (cond)
		Registers.S.B.h = 1;
}

// PHA
static inline void Op48E1()
{
	PHWrapper(Registers.A.B.l, Registers.A.B.l, PushBE, false);
}

static inline void Op48E0M1()
{
	PHWrapper(Registers.A.B.l, Registers.A.B.l, PushB, false);
}

static inline void Op48E0M0()
{
	PHWrapper(Registers.A.W, Registers.A.B.l, PushW, false);
}

static void Op48Slow()
{
	if (CheckEmulation())
		Op48E1();
	else if (CheckMemory())
		Op48E0M1();
	else
		Op48E0M0();
}

// PHB
static inline void Op8BE1()
{
	PHWrapper(Registers.DB, Registers.DB, PushBE, false);
}

static inline void Op8BE0()
{
	PHWrapper(Registers.DB, Registers.DB, PushB, false);
}

static void Op8BSlow()
{
	if (CheckEmulation())
		Op8BE1();
	else
		Op8BE0();
}

// PHD
static void Op0BE0()
{
	PHWrapper(Registers.D.W, Registers.D.B.l, PushW, false);
}

static void Op0BE1()
{
	// Note: PHD is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PHWrapper(Registers.D.W, Registers.D.B.l, PushW, true);
}

static void Op0BSlow()
{
	PHWrapper(Registers.D.W, Registers.D.B.l, PushW, CheckEmulation());
}

// PHK
static inline void Op4BE1()
{
	PHWrapper(Registers.PC.B.xPB, Registers.PC.B.xPB, PushBE, false);
}

static inline void Op4BE0()
{
	PHWrapper(Registers.PC.B.xPB, Registers.PC.B.xPB, PushB, false);
}

static void Op4BSlow()
{
	if (CheckEmulation())
		Op4BE1();
	else
		Op4BE0();
}

// PHP
static inline void Op08E1()
{
	S9xPackStatus();
	PHWrapper(Registers.P.B.l, Registers.P.B.l, PushBE, false);
}

static inline void Op08E0()
{
	S9xPackStatus();
	PHWrapper(Registers.P.B.l, Registers.P.B.l, PushB, false);
}

static void Op08Slow()
{
	if (CheckEmulation())
		Op08E1();
	else
		Op08E0();
}

// PHX
static inline void OpDAE1()
{
	PHWrapper(Registers.X.B.l, Registers.X.B.l, PushBE, false);
}

static inline void OpDAE0X1()
{
	PHWrapper(Registers.X.B.l, Registers.X.B.l, PushB, false);
}

static inline void OpDAE0X0()
{
	PHWrapper(Registers.X.W, Registers.X.B.l, PushW, false);
}

static void OpDASlow()
{
	if (CheckEmulation())
		OpDAE1();
	else if (CheckIndex())
		OpDAE0X1();
	else
		OpDAE0X0();
}

// PHY
static inline void Op5AE1()
{
	PHWrapper(Registers.Y.B.l, Registers.Y.B.l, PushBE, false);
}

static inline void Op5AE0X1()
{
	PHWrapper(Registers.Y.B.l, Registers.Y.B.l, PushB, false);
}

static inline void Op5AE0X0()
{
	PHWrapper(Registers.Y.W, Registers.Y.B.l, PushW, false);
}

static void Op5ASlow()
{
	if (CheckEmulation())
		Op5AE1();
	else if (CheckIndex())
		Op5AE0X1();
	else
		Op5AE0X0();
}

/* PULL Instructions ******************************************************* */

static inline void PullW(uint16_t &w)
{
	w = S9xGetWord(Registers.S.W + 1, WRAP_BANK);
	Registers.S.W += 2;
}

static inline void PullWE(uint16_t &w)
{
	++Registers.S.B.l;
	w = S9xGetWord(Registers.S.W, WRAP_PAGE);
	++Registers.S.B.l;
}

static inline void PullB(uint8_t &b)
{
	b = S9xGetByte(++Registers.S.W);
}

static inline void PullBE(uint8_t &b)
{
	++Registers.S.B.l;
	b = S9xGetByte(Registers.S.W);
}

// PLA
template<typename Treg, typename Tob, typename F> static inline void Op68Wrapper(Treg &pullReg, const Tob &obReg, F f)
{
	AddCycles(TWO_CYCLES);
	f(pullReg);
	SetZN(pullReg);
	OpenBus = obReg;
}

static inline void Op68E1()
{
	Op68Wrapper(Registers.A.B.l, Registers.A.B.l, PullBE);
}

static inline void Op68E0M1()
{
	Op68Wrapper(Registers.A.B.l, Registers.A.B.l, PullB);
}

static inline void Op68E0M0()
{
	Op68Wrapper(Registers.A.W, Registers.A.B.h, PullW);
}

static void Op68Slow()
{
	if (CheckEmulation())
		Op68E1();
	else if (CheckMemory())
		Op68E0M1();
	else
		Op68E0M0();
}

// PLB
template<typename F> static inline void OpABWrapper(F f)
{
	AddCycles(TWO_CYCLES);
	f(Registers.DB);
	SetZN(Registers.DB);
	ICPU.ShiftedDB = Registers.DB << 16;
	OpenBus = Registers.DB;
}

static inline void OpABE1()
{
	OpABWrapper(PullBE);
}

static inline void OpABE0()
{
	OpABWrapper(PullB);
}

static void OpABSlow()
{
	if (CheckEmulation())
		OpABE1();
	else
		OpABE0();
}

// PLD
static inline void Op2BWrapper(bool cond)
{
	AddCycles(TWO_CYCLES);
	PullW(Registers.D.W);
	SetZN(Registers.D.W);
	OpenBus = Registers.D.B.h;
	if (cond)
		Registers.S.B.h = 1;
}

static void Op2BE0()
{
	Op2BWrapper(false);
}

static void Op2BE1()
{
	// Note: PLD is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	Op2BWrapper(true);
}

static void Op2BSlow()
{
	Op2BWrapper(CheckEmulation());
}

// PLP
static void Op28E1()
{
	AddCycles(TWO_CYCLES);
	PullBE(Registers.P.B.l);
	OpenBus = Registers.P.B.l;
	SetFlags(MemoryFlag | IndexFlag);
	S9xUnpackStatus();
	S9xFixCycles();
}

static void Op28E0()
{
	AddCycles(TWO_CYCLES);
	PullB(Registers.P.B.l);
	OpenBus = Registers.P.B.l;
	S9xUnpackStatus();

	if (CheckIndex())
		Registers.X.B.h = Registers.Y.B.h = 0;

	S9xFixCycles();
}

static void Op28Slow()
{
	AddCycles(TWO_CYCLES);

	if (CheckEmulation())
	{
		PullBE(Registers.P.B.l);
		OpenBus = Registers.P.B.l;
		SetFlags(MemoryFlag | IndexFlag);
	}
	else
	{
		PullB(Registers.P.B.l);
		OpenBus = Registers.P.B.l;
	}

	S9xUnpackStatus();

	if (CheckIndex())
		Registers.X.B.h = Registers.Y.B.h = 0;

	S9xFixCycles();
}

// PLX
static inline void OpFAE1()
{
	AddCycles(TWO_CYCLES);
	PullBE(Registers.X.B.l);
	SetZN(Registers.X.B.l);
	OpenBus = Registers.X.B.l;
}

static inline void OpFAE0X1()
{
	AddCycles(TWO_CYCLES);
	PullB(Registers.X.B.l);
	SetZN(Registers.X.B.l);
	OpenBus = Registers.X.B.l;
}

static inline void OpFAE0X0()
{
	AddCycles(TWO_CYCLES);
	PullW(Registers.X.W);
	SetZN(Registers.X.W);
	OpenBus = Registers.X.B.h;
}

static void OpFASlow()
{
	if (CheckEmulation())
		OpFAE1();
	else if (CheckIndex())
		OpFAE0X1();
	else
		OpFAE0X0();
}

// PLY
static inline void Op7AE1()
{
	AddCycles(TWO_CYCLES);
	PullBE(Registers.Y.B.l);
	SetZN(Registers.Y.B.l);
	OpenBus = Registers.Y.B.l;
}

static inline void Op7AE0X1()
{
	AddCycles(TWO_CYCLES);
	PullB(Registers.Y.B.l);
	SetZN(Registers.Y.B.l);
	OpenBus = Registers.Y.B.l;
}

static inline void Op7AE0X0()
{
	AddCycles(TWO_CYCLES);
	PullW(Registers.Y.W);
	SetZN(Registers.Y.W);
	OpenBus = Registers.Y.B.h;
}

static void Op7ASlow()
{
	if (CheckEmulation())
		Op7AE1();
	else if (CheckIndex())
		Op7AE0X1();
	else
		Op7AE0X0();
}

/* Transfer Instructions *************************************************** */

template<typename T> static inline void TransferWrapper(T &reg1, const T &reg2)
{
	AddCycles(ONE_CYCLE);
	reg1 = reg2;
	SetZN(reg1);
}

// TAX
static inline void OpAAX1()
{
	TransferWrapper(Registers.X.B.l, Registers.A.B.l);
}

static inline void OpAAX0()
{
	TransferWrapper(Registers.X.W, Registers.A.W);
}

static void OpAASlow()
{
	if (CheckIndex())
		OpAAX1();
	else
		OpAAX0();
}

// TAY
static inline void OpA8X1()
{
	TransferWrapper(Registers.Y.B.l, Registers.A.B.l);
}

static inline void OpA8X0()
{
	TransferWrapper(Registers.Y.W, Registers.A.W);
}

static void OpA8Slow()
{
	if (CheckIndex())
		OpA8X1();
	else
		OpA8X0();
}

// TCD
static void Op5B()
{
	TransferWrapper(Registers.D.W, Registers.A.W);
}

// TCS
static void Op1B()
{
	AddCycles(ONE_CYCLE);
	Registers.S.W = Registers.A.W;
	if (CheckEmulation())
		Registers.S.B.h = 1;
}

// TDC
static void Op7B()
{
	TransferWrapper(Registers.A.W, Registers.D.W);
}

// TSC
static void Op3B()
{
	TransferWrapper(Registers.A.W, Registers.S.W);
}

// TSX
static inline void OpBAX1()
{
	TransferWrapper(Registers.X.B.l, Registers.S.B.l);
}

static inline void OpBAX0()
{
	TransferWrapper(Registers.X.W, Registers.S.W);
}

static void OpBASlow()
{
	if (CheckIndex())
		OpBAX1();
	else
		OpBAX0();
}

// TXA
static inline void Op8AM1()
{
	TransferWrapper(Registers.A.B.l, Registers.X.B.l);
}

static inline void Op8AM0()
{
	TransferWrapper(Registers.A.W, Registers.X.W);
}

static void Op8ASlow()
{
	if (CheckMemory())
		Op8AM1();
	else
		Op8AM0();
}

// TXS
static void Op9A()
{
	AddCycles(ONE_CYCLE);
	Registers.S.W = Registers.X.W;
	if (CheckEmulation())
		Registers.S.B.h = 1;
}

// TXY
static inline void Op9BX1()
{
	TransferWrapper(Registers.Y.B.l, Registers.X.B.l);
}

static inline void Op9BX0()
{
	TransferWrapper(Registers.Y.W, Registers.X.W);
}

static void Op9BSlow()
{
	if (CheckIndex())
		Op9BX1();
	else
		Op9BX0();
}

// TYA
static inline void Op98M1()
{
	TransferWrapper(Registers.A.B.l, Registers.Y.B.l);
}

static inline void Op98M0()
{
	TransferWrapper(Registers.A.W, Registers.Y.W);
}

static void Op98Slow()
{
	if (CheckMemory())
		Op98M1();
	else
		Op98M0();
}

// TYX
static inline void OpBBX1()
{
	TransferWrapper(Registers.X.B.l, Registers.Y.B.l);
}

static inline void OpBBX0()
{
	TransferWrapper(Registers.X.W, Registers.Y.W);
}

static void OpBBSlow()
{
	if (CheckIndex())
		OpBBX1();
	else
		OpBBX0();
}

/* XCE ********************************************************************* */

static void OpFB()
{
	AddCycles(ONE_CYCLE);

	uint8_t A1 = ICPU._Carry;
	uint8_t A2 = Registers.P.B.h;

	ICPU._Carry = A2 & 1;
	Registers.P.B.h = A1;

	if (CheckEmulation())
	{
		SetFlags(MemoryFlag | IndexFlag);
		Registers.S.B.h = 1;
	}

	if (CheckIndex())
		Registers.X.B.h = Registers.Y.B.h = 0;

	S9xFixCycles();
}

/* BRK ********************************************************************* */

static void Op00()
{
	AddCycles(CPU.MemSpeed);

	if (!CheckEmulation())
	{
		PushB(Registers.PC.B.xPB);
		PushW(Registers.PC.W.xPC + 1);
		S9xPackStatus();
		PushB(Registers.P.B.l);
	}
	else
	{
		PushWE(Registers.PC.W.xPC + 1);
		S9xPackStatus();
		PushBE(Registers.P.B.l);
	}
	OpenBus = Registers.P.B.l;
	ClearDecimal();
	SetIRQ();

	uint16_t addr = S9xGetWord(0xFFFE);

	S9xSetPCBase(addr);
	OpenBus = addr >> 8;
}

/* IRQ ********************************************************************* */

void S9xOpcode_IRQ()
{
	// IRQ and NMI do an opcode fetch as their first "IO" cycle.
	AddCycles(CPU.MemSpeed + ONE_CYCLE);

	uint16_t addr;
	if (!CheckEmulation())
	{
		PushB(Registers.PC.B.xPB);
		PushW(Registers.PC.W.xPC);
		S9xPackStatus();
		PushB(Registers.P.B.l);
		OpenBus = Registers.P.B.l;
		ClearDecimal();
		SetIRQ();

		addr = S9xGetWord(0xFFEE);
	}
	else
	{
		PushWE(Registers.PC.W.xPC);
		S9xPackStatus();
		PushBE(Registers.P.B.l);
		OpenBus = Registers.P.B.l;
		ClearDecimal();
		SetIRQ();

		addr = S9xGetWord(0xFFFE);
	}
	OpenBus = addr >> 8;
	S9xSetPCBase(addr);
}

/* NMI ********************************************************************* */

void S9xOpcode_NMI()
{
	// IRQ and NMI do an opcode fetch as their first "IO" cycle.
	AddCycles(CPU.MemSpeed + ONE_CYCLE);

	uint16_t addr;
	if (!CheckEmulation())
	{
		PushB(Registers.PC.B.xPB);
		PushW(Registers.PC.W.xPC);
		S9xPackStatus();
		PushB(Registers.P.B.l);
		OpenBus = Registers.P.B.l;
		ClearDecimal();
		SetIRQ();

		addr = S9xGetWord(0xFFEA);
	}
	else
	{
		PushWE(Registers.PC.W.xPC);
		S9xPackStatus();
		PushBE(Registers.P.B.l);
		OpenBus = Registers.P.B.l;
		ClearDecimal();
		SetIRQ();

		addr = S9xGetWord(0xFFFA);
	}
	OpenBus = addr >> 8;
	S9xSetPCBase(addr);
}

/* COP ********************************************************************* */

static void Op02()
{
	AddCycles(CPU.MemSpeed);

	if (!CheckEmulation())
	{
		PushB(Registers.PC.B.xPB);
		PushW(Registers.PC.W.xPC + 1);
		S9xPackStatus();
		PushB(Registers.P.B.l);
	}
	else
	{
		PushWE(Registers.PC.W.xPC + 1);
		S9xPackStatus();
		PushBE(Registers.P.B.l);
	}
	OpenBus = Registers.P.B.l;
	ClearDecimal();
	SetIRQ();

	uint16_t addr = S9xGetWord(0xFFF4);

	S9xSetPCBase(addr);
	OpenBus = addr >> 8;
}

/* JML ********************************************************************* */

template<typename F> static inline void JMLWrapper(F f)
{
	S9xSetPCBase(f(JUMP));
}

static void OpDC()
{
	JMLWrapper(AbsoluteIndirectLong);
}

static void OpDCSlow()
{
	JMLWrapper(AbsoluteIndirectLongSlow);
}

static void Op5C()
{
	JMLWrapper(AbsoluteLong);
}

static void Op5CSlow()
{
	JMLWrapper(AbsoluteLongSlow);
}

/* JMP ********************************************************************* */

template<typename F> static inline void JMPWrapper(F f)
{
	S9xSetPCBase(ICPU.ShiftedPB + static_cast<uint16_t>(f(JUMP)));
}

static void Op4C()
{
	JMPWrapper(Absolute);
}

static void Op4CSlow()
{
	JMPWrapper(AbsoluteSlow);
}

static void Op6C()
{
	JMPWrapper(AbsoluteIndirect);
}

static void Op6CSlow()
{
	JMPWrapper(AbsoluteIndirectSlow);
}

static void Op7C()
{
	JMPWrapper(AbsoluteIndexedIndirect);
}

static void Op7CSlow()
{
	JMPWrapper(AbsoluteIndexedIndirectSlow);
}

/* JSL/RTL ***************************************************************** */

template<typename F> static inline void Op22Wrapper(F f, bool cond)
{
	uint32_t addr = f(JSR);
	PushB(Registers.PC.B.xPB);
	PushW(Registers.PC.W.xPC - 1);
	if (cond)
		Registers.S.B.h = 1;
	S9xSetPCBase(addr);
}

static void Op22E0()
{
	Op22Wrapper(AbsoluteLong, false);
}

static void Op22E1()
{
	// Note: JSL is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	Op22Wrapper(AbsoluteLong, true);
}

static void Op22Slow()
{
	Op22Wrapper(AbsoluteLongSlow, CheckEmulation());
}

static inline void Op6BWrapper(bool cond)
{
	AddCycles(TWO_CYCLES);
	PullW(Registers.PC.W.xPC);
	PullB(Registers.PC.B.xPB);
	if (cond)
		Registers.S.B.h = 1;
	++Registers.PC.W.xPC;
	S9xSetPCBase(Registers.PC.xPBPC);
}

static void Op6BE0()
{
	Op6BWrapper(false);
}

static void Op6BE1()
{
	// Note: RTL is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	Op6BWrapper(true);
}

static void Op6BSlow()
{
	Op6BWrapper(CheckEmulation());
}

/* JSR/RTS ***************************************************************** */

template<typename F> static inline void Op20Wrapper(F f)
{
	uint16_t addr = Absolute(JSR);
	AddCycles(ONE_CYCLE);
	f(Registers.PC.W.xPC - 1);
	S9xSetPCBase(ICPU.ShiftedPB + addr);
}

static inline void Op20E1()
{
	Op20Wrapper(PushWE);
}

static void Op20E0()
{
	Op20Wrapper(PushW);
}

static inline void Op20Slow()
{
	if (CheckEmulation())
		Op20E1();
	else
		Op20E0();
}

template<typename F> static inline void OpFCWrapper(F f, bool cond)
{
	uint16_t addr = f(JSR);
	PushW(Registers.PC.W.xPC - 1);
	if (cond)
		Registers.S.B.h = 1;
	S9xSetPCBase(ICPU.ShiftedPB + addr);
}

static void OpFCE0()
{
	OpFCWrapper(AbsoluteIndexedIndirect, false);
}

static void OpFCE1()
{
	// Note: JSR (a,X) is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	OpFCWrapper(AbsoluteIndexedIndirect, true);
}

static void OpFCSlow()
{
	OpFCWrapper(AbsoluteIndexedIndirectSlow, CheckEmulation());
}

template<typename F> static inline void Op60Wrapper(F f)
{
	AddCycles(TWO_CYCLES);
	f(Registers.PC.W.xPC);
	AddCycles(ONE_CYCLE);
	++Registers.PC.W.xPC;
	S9xSetPCBase(Registers.PC.xPBPC);
}

static inline void Op60E1()
{
	Op60Wrapper(PullWE);
}

static inline void Op60E0()
{
	Op60Wrapper(PullW);
}

static void Op60Slow()
{
	if (CheckEmulation())
		Op60E1();
	else
		Op60E0();
}

/* MVN/MVP ***************************************************************** */

template<typename F> static inline void MVWrapper(F f, bool cond, int inc, bool setOpenBusOnFirstF = false)
{
	uint32_t SrcBank;

	Registers.DB = f(NONE);
	if (setOpenBusOnFirstF)
		OpenBus = Registers.DB;
	ICPU.ShiftedDB = Registers.DB << 16;
	OpenBus = SrcBank = f(NONE);

	S9xSetByte(OpenBus = S9xGetByte((SrcBank << 16) + Registers.X.W), ICPU.ShiftedDB + Registers.Y.W);

	if (cond)
	{
		Registers.X.B.l += inc;
		Registers.Y.B.l += inc;
	}
	else
	{
		Registers.X.W += inc;
		Registers.Y.W += inc;
	}

	--Registers.A.W;
	if (Registers.A.W != 0xffff)
		Registers.PC.W.xPC -= 3;

	AddCycles(TWO_CYCLES);
}

static void Op54X1()
{
	MVWrapper(Immediate8, true, 1);
}

static void Op54X0()
{
	MVWrapper(Immediate8, false, 1);
}

static void Op54Slow()
{
	MVWrapper(Immediate8Slow, CheckIndex(), 1, true);
}

static void Op44X1()
{
	MVWrapper(Immediate8, true, -1);
}

static void Op44X0()
{
	MVWrapper(Immediate8, false, -1);
}

static void Op44Slow()
{
	MVWrapper(Immediate8Slow, CheckIndex(), -1, true);
}

/* REP/SEP ***************************************************************** */

template<typename F> static inline void OpC2Wrapper(F f)
{
	uint8_t Work8 = ~f(READ);
	Registers.P.B.l &= Work8;
	ICPU._Carry &= Work8;
	ICPU._Overflow &= Work8 >> 6;
	ICPU._Negative &= Work8;
	ICPU._Zero |= ~Work8 & Zero;

	AddCycles(ONE_CYCLE);

	if (CheckEmulation())
		SetFlags(MemoryFlag | IndexFlag);

	if (CheckIndex())
		Registers.X.B.h = Registers.Y.B.h = 0;

	S9xFixCycles();
}

static void OpC2()
{
	OpC2Wrapper(Immediate8);
}

static void OpC2Slow()
{
	OpC2Wrapper(Immediate8Slow);
}

template<typename F> static inline void OpE2Wrapper(F f)
{
	uint8_t Work8 = f(READ);
	Registers.P.B.l |= Work8;
	ICPU._Carry |= Work8 & 1;
	ICPU._Overflow |= (Work8 >> 6) & 1;
	ICPU._Negative |= Work8;
	if (Work8 & Zero)
		ICPU._Zero = 0;

	AddCycles(ONE_CYCLE);

	if (CheckEmulation())
		SetFlags(MemoryFlag | IndexFlag);

	if (CheckIndex())
		Registers.X.B.h = Registers.Y.B.h = 0;

	S9xFixCycles();
}

static void OpE2()
{
	OpE2Wrapper(Immediate8);
}

static void OpE2Slow()
{
	OpE2Wrapper(Immediate8Slow);
}

/* XBA ********************************************************************* */

static void OpEB()
{
	std::swap(Registers.A.B.l, Registers.A.B.h);
	SetZN(Registers.A.B.l);
	AddCycles(TWO_CYCLES);
}

/* RTI ********************************************************************* */

static void Op40Slow()
{
	AddCycles(TWO_CYCLES);

	if (!CheckEmulation())
	{
		PullB(Registers.P.B.l);
		S9xUnpackStatus();
		PullW(Registers.PC.W.xPC);
		PullB(Registers.PC.B.xPB);
		OpenBus = Registers.PC.B.xPB;
		ICPU.ShiftedPB = Registers.PC.B.xPB << 16;
	}
	else
	{
		PullBE(Registers.P.B.l);
		S9xUnpackStatus();
		PullWE(Registers.PC.W.xPC);
		OpenBus = Registers.PC.B.xPCh;
		SetFlags(MemoryFlag | IndexFlag);
	}

	S9xSetPCBase(Registers.PC.xPBPC);

	if (CheckIndex())
		Registers.X.B.h = Registers.Y.B.h = 0;

	S9xFixCycles();
}

/* STP/WAI ***************************************************************** */

// WAI
static void OpCB()
{
	CPU.WaitingForInterrupt = true;

	--Registers.PC.W.xPC;
	AddCycles(ONE_CYCLE);
}

// STP
static void OpDB()
{
	--Registers.PC.W.xPC;
	CPU.Flags |= DEBUG_MODE_FLAG | HALTED_FLAG;
}

/* WDM (Reserved S9xOpcode) ************************************************ */

static void Op42()
{
	S9xGetWord(Registers.PC.xPBPC);
	++Registers.PC.W.xPC;
}

/* CPU-S9xOpcodes Definitions ************************************************/

struct SOpcodes S9xOpcodesM1X1[256] =
{
	{ Op00 },        { Op01E0M1 },    { Op02 },        { Op03M1 },      { Op04M1 },
	{ Op05M1 },      { Op06M1 },      { Op07M1 },      { Op08E0 },      { Op09M1 },
	{ Op0AM1 },      { Op0BE0 },      { Op0CM1 },      { Op0DM1 },      { Op0EM1 },
	{ Op0FM1 },      { Op10E0 },      { Op11E0M1X1 },  { Op12E0M1 },    { Op13M1 },
	{ Op14M1 },      { Op15E0M1 },    { Op16E0M1 },    { Op17M1 },      { Op18 },
	{ Op19M1X1 },    { Op1AM1 },      { Op1B },        { Op1CM1 },      { Op1DM1X1 },
	{ Op1EM1X1 },    { Op1FM1 },      { Op20E0 },      { Op21E0M1 },    { Op22E0 },
	{ Op23M1 },      { Op24M1 },      { Op25M1 },      { Op26M1 },      { Op27M1 },
	{ Op28E0 },      { Op29M1 },      { Op2AM1 },      { Op2BE0 },      { Op2CM1 },
	{ Op2DM1 },      { Op2EM1 },      { Op2FM1 },      { Op30E0 },      { Op31E0M1X1 },
	{ Op32E0M1 },    { Op33M1 },      { Op34E0M1 },    { Op35E0M1 },    { Op36E0M1 },
	{ Op37M1 },      { Op38 },        { Op39M1X1 },    { Op3AM1 },      { Op3B },
	{ Op3CM1X1 },    { Op3DM1X1 },    { Op3EM1X1 },    { Op3FM1 },      { Op40Slow },
	{ Op41E0M1 },    { Op42 },        { Op43M1 },      { Op44X1 },      { Op45M1 },
	{ Op46M1 },      { Op47M1 },      { Op48E0M1 },    { Op49M1 },      { Op4AM1 },
	{ Op4BE0 },      { Op4C },        { Op4DM1 },      { Op4EM1 },      { Op4FM1 },
	{ Op50E0 },      { Op51E0M1X1 },  { Op52E0M1 },    { Op53M1 },      { Op54X1 },
	{ Op55E0M1 },    { Op56E0M1 },    { Op57M1 },      { Op58 },        { Op59M1X1 },
	{ Op5AE0X1 },    { Op5B },        { Op5C },        { Op5DM1X1 },    { Op5EM1X1 },
	{ Op5FM1 },      { Op60E0 },      { Op61E0M1 },    { Op62E0 },      { Op63M1 },
	{ Op64M1 },      { Op65M1 },      { Op66M1 },      { Op67M1 },      { Op68E0M1 },
	{ Op69M1 },      { Op6AM1 },      { Op6BE0 },      { Op6C },        { Op6DM1 },
	{ Op6EM1 },      { Op6FM1 },      { Op70E0 },      { Op71E0M1X1 },  { Op72E0M1 },
	{ Op73M1 },      { Op74E0M1 },    { Op75E0M1 },    { Op76E0M1 },    { Op77M1 },
	{ Op78 },        { Op79M1X1 },    { Op7AE0X1 },    { Op7B },        { Op7C },
	{ Op7DM1X1 },    { Op7EM1X1 },    { Op7FM1 },      { Op80E0 },      { Op81E0M1 },
	{ Op82 },        { Op83M1 },      { Op84X1 },      { Op85M1 },      { Op86X1 },
	{ Op87M1 },      { Op88X1 },      { Op89M1 },      { Op8AM1 },      { Op8BE0 },
	{ Op8CX1 },      { Op8DM1 },      { Op8EX1 },      { Op8FM1 },      { Op90E0 },
	{ Op91E0M1X1 },  { Op92E0M1 },    { Op93M1 },      { Op94E0X1 },    { Op95E0M1 },
	{ Op96E0X1 },    { Op97M1 },      { Op98M1 },      { Op99M1X1 },    { Op9A },
	{ Op9BX1 },      { Op9CM1 },      { Op9DM1X1 },    { Op9EM1X1 },    { Op9FM1 },
	{ OpA0X1 },      { OpA1E0M1 },    { OpA2X1 },      { OpA3M1 },      { OpA4X1 },
	{ OpA5M1 },      { OpA6X1 },      { OpA7M1 },      { OpA8X1 },      { OpA9M1 },
	{ OpAAX1 },      { OpABE0 },      { OpACX1 },      { OpADM1 },      { OpAEX1 },
	{ OpAFM1 },      { OpB0E0 },      { OpB1E0M1X1 },  { OpB2E0M1 },    { OpB3M1 },
	{ OpB4E0X1 },    { OpB5E0M1 },    { OpB6E0X1 },    { OpB7M1 },      { OpB8 },
	{ OpB9M1X1 },    { OpBAX1 },      { OpBBX1 },      { OpBCX1 },      { OpBDM1X1 },
	{ OpBEX1 },      { OpBFM1 },      { OpC0X1 },      { OpC1E0M1 },    { OpC2 },
	{ OpC3M1 },      { OpC4X1 },      { OpC5M1 },      { OpC6M1 },      { OpC7M1 },
	{ OpC8X1 },      { OpC9M1 },      { OpCAX1 },      { OpCB },        { OpCCX1 },
	{ OpCDM1 },      { OpCEM1 },      { OpCFM1 },      { OpD0E0 },      { OpD1E0M1X1 },
	{ OpD2E0M1 },    { OpD3M1 },      { OpD4E0 },      { OpD5E0M1 },    { OpD6E0M1 },
	{ OpD7M1 },      { OpD8 },        { OpD9M1X1 },    { OpDAE0X1 },    { OpDB },
	{ OpDC },        { OpDDM1X1 },    { OpDEM1X1 },    { OpDFM1 },      { OpE0X1 },
	{ OpE1E0M1 },    { OpE2 },        { OpE3M1 },      { OpE4X1 },      { OpE5M1 },
	{ OpE6M1 },      { OpE7M1 },      { OpE8X1 },      { OpE9M1 },      { OpEA },
	{ OpEB },        { OpECX1 },      { OpEDM1 },      { OpEEM1 },      { OpEFM1 },
	{ OpF0E0 },      { OpF1E0M1X1 },  { OpF2E0M1 },    { OpF3M1 },      { OpF4E0 },
	{ OpF5E0M1 },    { OpF6E0M1 },    { OpF7M1 },      { OpF8 },        { OpF9M1X1 },
	{ OpFAE0X1 },    { OpFB },        { OpFCE0 },      { OpFDM1X1 },    { OpFEM1X1 },
	{ OpFFM1 }
};

struct SOpcodes S9xOpcodesE1[256] =
{
	{ Op00 },        { Op01E1 },      { Op02 },        { Op03M1 },      { Op04M1 },
	{ Op05M1 },      { Op06M1 },      { Op07M1 },      { Op08E1 },      { Op09M1 },
	{ Op0AM1 },      { Op0BE1 },      { Op0CM1 },      { Op0DM1 },      { Op0EM1 },
	{ Op0FM1 },      { Op10E1 },      { Op11E1 },      { Op12E1 },      { Op13M1 },
	{ Op14M1 },      { Op15E1 },      { Op16E1 },      { Op17M1 },      { Op18 },
	{ Op19M1X1 },    { Op1AM1 },      { Op1B },        { Op1CM1 },      { Op1DM1X1 },
	{ Op1EM1X1 },    { Op1FM1 },      { Op20E1 },      { Op21E1 },      { Op22E1 },
	{ Op23M1 },      { Op24M1 },      { Op25M1 },      { Op26M1 },      { Op27M1 },
	{ Op28E1 },      { Op29M1 },      { Op2AM1 },      { Op2BE1 },      { Op2CM1 },
	{ Op2DM1 },      { Op2EM1 },      { Op2FM1 },      { Op30E1 },      { Op31E1 },
	{ Op32E1 },      { Op33M1 },      { Op34E1 },      { Op35E1 },      { Op36E1 },
	{ Op37M1 },      { Op38 },        { Op39M1X1 },    { Op3AM1 },      { Op3B },
	{ Op3CM1X1 },    { Op3DM1X1 },    { Op3EM1X1 },    { Op3FM1 },      { Op40Slow },
	{ Op41E1 },      { Op42 },        { Op43M1 },      { Op44X1 },      { Op45M1 },
	{ Op46M1 },      { Op47M1 },      { Op48E1 },      { Op49M1 },      { Op4AM1 },
	{ Op4BE1 },      { Op4C },        { Op4DM1 },      { Op4EM1 },      { Op4FM1 },
	{ Op50E1 },      { Op51E1 },      { Op52E1 },      { Op53M1 },      { Op54X1 },
	{ Op55E1 },      { Op56E1 },      { Op57M1 },      { Op58 },        { Op59M1X1 },
	{ Op5AE1 },      { Op5B },        { Op5C },        { Op5DM1X1 },    { Op5EM1X1 },
	{ Op5FM1 },      { Op60E1 },      { Op61E1 },      { Op62E1 },      { Op63M1 },
	{ Op64M1 },      { Op65M1 },      { Op66M1 },      { Op67M1 },      { Op68E1 },
	{ Op69M1 },      { Op6AM1 },      { Op6BE1 },      { Op6C },        { Op6DM1 },
	{ Op6EM1 },      { Op6FM1 },      { Op70E1 },      { Op71E1 },      { Op72E1 },
	{ Op73M1 },      { Op74E1 },      { Op75E1 },      { Op76E1 },      { Op77M1 },
	{ Op78 },        { Op79M1X1 },    { Op7AE1 },      { Op7B },        { Op7C },
	{ Op7DM1X1 },    { Op7EM1X1 },    { Op7FM1 },      { Op80E1 },      { Op81E1 },
	{ Op82 },        { Op83M1 },      { Op84X1 },      { Op85M1 },      { Op86X1 },
	{ Op87M1 },      { Op88X1 },      { Op89M1 },      { Op8AM1 },      { Op8BE1 },
	{ Op8CX1 },      { Op8DM1 },      { Op8EX1 },      { Op8FM1 },      { Op90E1 },
	{ Op91E1 },      { Op92E1 },      { Op93M1 },      { Op94E1 },      { Op95E1 },
	{ Op96E1 },      { Op97M1 },      { Op98M1 },      { Op99M1X1 },    { Op9A },
	{ Op9BX1 },      { Op9CM1 },      { Op9DM1X1 },    { Op9EM1X1 },    { Op9FM1 },
	{ OpA0X1 },      { OpA1E1 },      { OpA2X1 },      { OpA3M1 },      { OpA4X1 },
	{ OpA5M1 },      { OpA6X1 },      { OpA7M1 },      { OpA8X1 },      { OpA9M1 },
	{ OpAAX1 },      { OpABE1 },      { OpACX1 },      { OpADM1 },      { OpAEX1 },
	{ OpAFM1 },      { OpB0E1 },      { OpB1E1 },      { OpB2E1 },      { OpB3M1 },
	{ OpB4E1 },      { OpB5E1 },      { OpB6E1 },      { OpB7M1 },      { OpB8 },
	{ OpB9M1X1 },    { OpBAX1 },      { OpBBX1 },      { OpBCX1 },      { OpBDM1X1 },
	{ OpBEX1 },      { OpBFM1 },      { OpC0X1 },      { OpC1E1 },      { OpC2 },
	{ OpC3M1 },      { OpC4X1 },      { OpC5M1 },      { OpC6M1 },      { OpC7M1 },
	{ OpC8X1 },      { OpC9M1 },      { OpCAX1 },      { OpCB },        { OpCCX1 },
	{ OpCDM1 },      { OpCEM1 },      { OpCFM1 },      { OpD0E1 },      { OpD1E1 },
	{ OpD2E1 },      { OpD3M1 },      { OpD4E1 },      { OpD5E1 },      { OpD6E1 },
	{ OpD7M1 },      { OpD8 },        { OpD9M1X1 },    { OpDAE1 },      { OpDB },
	{ OpDC },        { OpDDM1X1 },    { OpDEM1X1 },    { OpDFM1 },      { OpE0X1 },
	{ OpE1E1 },      { OpE2 },        { OpE3M1 },      { OpE4X1 },      { OpE5M1 },
	{ OpE6M1 },      { OpE7M1 },      { OpE8X1 },      { OpE9M1 },      { OpEA },
	{ OpEB },        { OpECX1 },      { OpEDM1 },      { OpEEM1 },      { OpEFM1 },
	{ OpF0E1 },      { OpF1E1 },      { OpF2E1 },      { OpF3M1 },      { OpF4E1 },
	{ OpF5E1 },      { OpF6E1 },      { OpF7M1 },      { OpF8 },        { OpF9M1X1 },
	{ OpFAE1 },      { OpFB },        { OpFCE1 },      { OpFDM1X1 },    { OpFEM1X1 },
	{ OpFFM1 }
};

struct SOpcodes S9xOpcodesM1X0[256] =
{
	{ Op00 },        { Op01E0M1 },    { Op02 },        { Op03M1 },      { Op04M1 },
	{ Op05M1 },      { Op06M1 },      { Op07M1 },      { Op08E0 },      { Op09M1 },
	{ Op0AM1 },      { Op0BE0 },      { Op0CM1 },      { Op0DM1 },      { Op0EM1 },
	{ Op0FM1 },      { Op10E0 },      { Op11E0M1X0 },  { Op12E0M1 },    { Op13M1 },
	{ Op14M1 },      { Op15E0M1 },    { Op16E0M1 },    { Op17M1 },      { Op18 },
	{ Op19M1X0 },    { Op1AM1 },      { Op1B },        { Op1CM1 },      { Op1DM1X0 },
	{ Op1EM1X0 },    { Op1FM1 },      { Op20E0 },      { Op21E0M1 },    { Op22E0 },
	{ Op23M1 },      { Op24M1 },      { Op25M1 },      { Op26M1 },      { Op27M1 },
	{ Op28E0 },      { Op29M1 },      { Op2AM1 },      { Op2BE0 },      { Op2CM1 },
	{ Op2DM1 },      { Op2EM1 },      { Op2FM1 },      { Op30E0 },      { Op31E0M1X0 },
	{ Op32E0M1 },    { Op33M1 },      { Op34E0M1 },    { Op35E0M1 },    { Op36E0M1 },
	{ Op37M1 },      { Op38 },        { Op39M1X0 },    { Op3AM1 },      { Op3B },
	{ Op3CM1X0 },    { Op3DM1X0 },    { Op3EM1X0 },    { Op3FM1 },      { Op40Slow },
	{ Op41E0M1 },    { Op42 },        { Op43M1 },      { Op44X0 },      { Op45M1 },
	{ Op46M1 },      { Op47M1 },      { Op48E0M1 },    { Op49M1 },      { Op4AM1 },
	{ Op4BE0 },      { Op4C },        { Op4DM1 },      { Op4EM1 },      { Op4FM1 },
	{ Op50E0 },      { Op51E0M1X0 },  { Op52E0M1 },    { Op53M1 },      { Op54X0 },
	{ Op55E0M1 },    { Op56E0M1 },    { Op57M1 },      { Op58 },        { Op59M1X0 },
	{ Op5AE0X0 },    { Op5B },        { Op5C },        { Op5DM1X0 },    { Op5EM1X0 },
	{ Op5FM1 },      { Op60E0 },      { Op61E0M1 },    { Op62E0 },      { Op63M1 },
	{ Op64M1 },      { Op65M1 },      { Op66M1 },      { Op67M1 },      { Op68E0M1 },
	{ Op69M1 },      { Op6AM1 },      { Op6BE0 },      { Op6C },        { Op6DM1 },
	{ Op6EM1 },      { Op6FM1 },      { Op70E0 },      { Op71E0M1X0 },  { Op72E0M1 },
	{ Op73M1 },      { Op74E0M1 },    { Op75E0M1 },    { Op76E0M1 },    { Op77M1 },
	{ Op78 },        { Op79M1X0 },    { Op7AE0X0 },    { Op7B },        { Op7C },
	{ Op7DM1X0 },    { Op7EM1X0 },    { Op7FM1 },      { Op80E0 },      { Op81E0M1 },
	{ Op82 },        { Op83M1 },      { Op84X0 },      { Op85M1 },      { Op86X0 },
	{ Op87M1 },      { Op88X0 },      { Op89M1 },      { Op8AM1 },      { Op8BE0 },
	{ Op8CX0 },      { Op8DM1 },      { Op8EX0 },      { Op8FM1 },      { Op90E0 },
	{ Op91E0M1X0 },  { Op92E0M1 },    { Op93M1 },      { Op94E0X0 },    { Op95E0M1 },
	{ Op96E0X0 },    { Op97M1 },      { Op98M1 },      { Op99M1X0 },    { Op9A },
	{ Op9BX0 },      { Op9CM1 },      { Op9DM1X0 },    { Op9EM1X0 },    { Op9FM1 },
	{ OpA0X0 },      { OpA1E0M1 },    { OpA2X0 },      { OpA3M1 },      { OpA4X0 },
	{ OpA5M1 },      { OpA6X0 },      { OpA7M1 },      { OpA8X0 },      { OpA9M1 },
	{ OpAAX0 },      { OpABE0 },      { OpACX0 },      { OpADM1 },      { OpAEX0 },
	{ OpAFM1 },      { OpB0E0 },      { OpB1E0M1X0 },  { OpB2E0M1 },    { OpB3M1 },
	{ OpB4E0X0 },    { OpB5E0M1 },    { OpB6E0X0 },    { OpB7M1 },      { OpB8 },
	{ OpB9M1X0 },    { OpBAX0 },      { OpBBX0 },      { OpBCX0 },      { OpBDM1X0 },
	{ OpBEX0 },      { OpBFM1 },      { OpC0X0 },      { OpC1E0M1 },    { OpC2 },
	{ OpC3M1 },      { OpC4X0 },      { OpC5M1 },      { OpC6M1 },      { OpC7M1 },
	{ OpC8X0 },      { OpC9M1 },      { OpCAX0 },      { OpCB },        { OpCCX0 },
	{ OpCDM1 },      { OpCEM1 },      { OpCFM1 },      { OpD0E0 },      { OpD1E0M1X0 },
	{ OpD2E0M1 },    { OpD3M1 },      { OpD4E0 },      { OpD5E0M1 },    { OpD6E0M1 },
	{ OpD7M1 },      { OpD8 },        { OpD9M1X0 },    { OpDAE0X0 },    { OpDB },
	{ OpDC },        { OpDDM1X0 },    { OpDEM1X0 },    { OpDFM1 },      { OpE0X0 },
	{ OpE1E0M1 },    { OpE2 },        { OpE3M1 },      { OpE4X0 },      { OpE5M1 },
	{ OpE6M1 },      { OpE7M1 },      { OpE8X0 },      { OpE9M1 },      { OpEA },
	{ OpEB },        { OpECX0 },      { OpEDM1 },      { OpEEM1 },      { OpEFM1 },
	{ OpF0E0 },      { OpF1E0M1X0 },  { OpF2E0M1 },    { OpF3M1 },      { OpF4E0 },
	{ OpF5E0M1 },    { OpF6E0M1 },    { OpF7M1 },      { OpF8 },        { OpF9M1X0 },
	{ OpFAE0X0 },    { OpFB },        { OpFCE0 },      { OpFDM1X0 },    { OpFEM1X0 },
	{ OpFFM1 }
};

struct SOpcodes S9xOpcodesM0X0[256] =
{
	{ Op00 },        { Op01E0M0 },    { Op02 },        { Op03M0 },      { Op04M0 },
	{ Op05M0 },      { Op06M0 },      { Op07M0 },      { Op08E0 },      { Op09M0 },
	{ Op0AM0 },      { Op0BE0 },      { Op0CM0 },      { Op0DM0 },      { Op0EM0 },
	{ Op0FM0 },      { Op10E0 },      { Op11E0M0X0 },  { Op12E0M0 },    { Op13M0 },
	{ Op14M0 },      { Op15E0M0 },    { Op16E0M0 },    { Op17M0 },      { Op18 },
	{ Op19M0X0 },    { Op1AM0 },      { Op1B },        { Op1CM0 },      { Op1DM0X0 },
	{ Op1EM0X0 },    { Op1FM0 },      { Op20E0 },      { Op21E0M0 },    { Op22E0 },
	{ Op23M0 },      { Op24M0 },      { Op25M0 },      { Op26M0 },      { Op27M0 },
	{ Op28E0 },      { Op29M0 },      { Op2AM0 },      { Op2BE0 },      { Op2CM0 },
	{ Op2DM0 },      { Op2EM0 },      { Op2FM0 },      { Op30E0 },      { Op31E0M0X0 },
	{ Op32E0M0 },    { Op33M0 },      { Op34E0M0 },    { Op35E0M0 },    { Op36E0M0 },
	{ Op37M0 },      { Op38 },        { Op39M0X0 },    { Op3AM0 },      { Op3B },
	{ Op3CM0X0 },    { Op3DM0X0 },    { Op3EM0X0 },    { Op3FM0 },      { Op40Slow },
	{ Op41E0M0 },    { Op42 },        { Op43M0 },      { Op44X0 },      { Op45M0 },
	{ Op46M0 },      { Op47M0 },      { Op48E0M0 },    { Op49M0 },      { Op4AM0 },
	{ Op4BE0 },      { Op4C },        { Op4DM0 },      { Op4EM0 },      { Op4FM0 },
	{ Op50E0 },      { Op51E0M0X0 },  { Op52E0M0 },    { Op53M0 },      { Op54X0 },
	{ Op55E0M0 },    { Op56E0M0 },    { Op57M0 },      { Op58 },        { Op59M0X0 },
	{ Op5AE0X0 },    { Op5B },        { Op5C },        { Op5DM0X0 },    { Op5EM0X0 },
	{ Op5FM0 },      { Op60E0 },      { Op61E0M0 },    { Op62E0 },      { Op63M0 },
	{ Op64M0 },      { Op65M0 },      { Op66M0 },      { Op67M0 },      { Op68E0M0 },
	{ Op69M0 },      { Op6AM0 },      { Op6BE0 },      { Op6C },        { Op6DM0 },
	{ Op6EM0 },      { Op6FM0 },      { Op70E0 },      { Op71E0M0X0 },  { Op72E0M0 },
	{ Op73M0 },      { Op74E0M0 },    { Op75E0M0 },    { Op76E0M0 },    { Op77M0 },
	{ Op78 },        { Op79M0X0 },    { Op7AE0X0 },    { Op7B },        { Op7C },
	{ Op7DM0X0 },    { Op7EM0X0 },    { Op7FM0 },      { Op80E0 },      { Op81E0M0 },
	{ Op82 },        { Op83M0 },      { Op84X0 },      { Op85M0 },      { Op86X0 },
	{ Op87M0 },      { Op88X0 },      { Op89M0 },      { Op8AM0 },      { Op8BE0 },
	{ Op8CX0 },      { Op8DM0 },      { Op8EX0 },      { Op8FM0 },      { Op90E0 },
	{ Op91E0M0X0 },  { Op92E0M0 },    { Op93M0 },      { Op94E0X0 },    { Op95E0M0 },
	{ Op96E0X0 },    { Op97M0 },      { Op98M0 },      { Op99M0X0 },    { Op9A },
	{ Op9BX0 },      { Op9CM0 },      { Op9DM0X0 },    { Op9EM0X0 },    { Op9FM0 },
	{ OpA0X0 },      { OpA1E0M0 },    { OpA2X0 },      { OpA3M0 },      { OpA4X0 },
	{ OpA5M0 },      { OpA6X0 },      { OpA7M0 },      { OpA8X0 },      { OpA9M0 },
	{ OpAAX0 },      { OpABE0 },      { OpACX0 },      { OpADM0 },      { OpAEX0 },
	{ OpAFM0 },      { OpB0E0 },      { OpB1E0M0X0 },  { OpB2E0M0 },    { OpB3M0 },
	{ OpB4E0X0 },    { OpB5E0M0 },    { OpB6E0X0 },    { OpB7M0 },      { OpB8 },
	{ OpB9M0X0 },    { OpBAX0 },      { OpBBX0 },      { OpBCX0 },      { OpBDM0X0 },
	{ OpBEX0 },      { OpBFM0 },      { OpC0X0 },      { OpC1E0M0 },    { OpC2 },
	{ OpC3M0 },      { OpC4X0 },      { OpC5M0 },      { OpC6M0 },      { OpC7M0 },
	{ OpC8X0 },      { OpC9M0 },      { OpCAX0 },      { OpCB },        { OpCCX0 },
	{ OpCDM0 },      { OpCEM0 },      { OpCFM0 },      { OpD0E0 },      { OpD1E0M0X0 },
	{ OpD2E0M0 },    { OpD3M0 },      { OpD4E0 },      { OpD5E0M0 },    { OpD6E0M0 },
	{ OpD7M0 },      { OpD8 },        { OpD9M0X0 },    { OpDAE0X0 },    { OpDB },
	{ OpDC },        { OpDDM0X0 },    { OpDEM0X0 },    { OpDFM0 },      { OpE0X0 },
	{ OpE1E0M0 },    { OpE2 },        { OpE3M0 },      { OpE4X0 },      { OpE5M0 },
	{ OpE6M0 },      { OpE7M0 },      { OpE8X0 },      { OpE9M0 },      { OpEA },
	{ OpEB },        { OpECX0 },      { OpEDM0 },      { OpEEM0 },      { OpEFM0 },
	{ OpF0E0 },      { OpF1E0M0X0 },  { OpF2E0M0 },    { OpF3M0 },      { OpF4E0 },
	{ OpF5E0M0 },    { OpF6E0M0 },    { OpF7M0 },      { OpF8 },        { OpF9M0X0 },
	{ OpFAE0X0 },    { OpFB },        { OpFCE0 },      { OpFDM0X0 },    { OpFEM0X0 },
	{ OpFFM0 }
};

struct SOpcodes S9xOpcodesM0X1[256] =
{
	{ Op00 },        { Op01E0M0 },    { Op02 },        { Op03M0 },      { Op04M0 },
	{ Op05M0 },      { Op06M0 },      { Op07M0 },      { Op08E0 },      { Op09M0 },
	{ Op0AM0 },      { Op0BE0 },      { Op0CM0 },      { Op0DM0 },      { Op0EM0 },
	{ Op0FM0 },      { Op10E0 },      { Op11E0M0X1 },  { Op12E0M0 },    { Op13M0 },
	{ Op14M0 },      { Op15E0M0 },    { Op16E0M0 },    { Op17M0 },      { Op18 },
	{ Op19M0X1 },    { Op1AM0 },      { Op1B },        { Op1CM0 },      { Op1DM0X1 },
	{ Op1EM0X1 },    { Op1FM0 },      { Op20E0 },      { Op21E0M0 },    { Op22E0 },
	{ Op23M0 },      { Op24M0 },      { Op25M0 },      { Op26M0 },      { Op27M0 },
	{ Op28E0 },      { Op29M0 },      { Op2AM0 },      { Op2BE0 },      { Op2CM0 },
	{ Op2DM0 },      { Op2EM0 },      { Op2FM0 },      { Op30E0 },      { Op31E0M0X1 },
	{ Op32E0M0 },    { Op33M0 },      { Op34E0M0 },    { Op35E0M0 },    { Op36E0M0 },
	{ Op37M0 },      { Op38 },        { Op39M0X1 },    { Op3AM0 },      { Op3B },
	{ Op3CM0X1 },    { Op3DM0X1 },    { Op3EM0X1 },    { Op3FM0 },      { Op40Slow },
	{ Op41E0M0 },    { Op42 },        { Op43M0 },      { Op44X1 },      { Op45M0 },
	{ Op46M0 },      { Op47M0 },      { Op48E0M0 },    { Op49M0 },      { Op4AM0 },
	{ Op4BE0 },      { Op4C },        { Op4DM0 },      { Op4EM0 },      { Op4FM0 },
	{ Op50E0 },      { Op51E0M0X1 },  { Op52E0M0 },    { Op53M0 },      { Op54X1 },
	{ Op55E0M0 },    { Op56E0M0 },    { Op57M0 },      { Op58 },        { Op59M0X1 },
	{ Op5AE0X1 },    { Op5B },        { Op5C },        { Op5DM0X1 },    { Op5EM0X1 },
	{ Op5FM0 },      { Op60E0 },      { Op61E0M0 },    { Op62E0 },      { Op63M0 },
	{ Op64M0 },      { Op65M0 },      { Op66M0 },      { Op67M0 },      { Op68E0M0 },
	{ Op69M0 },      { Op6AM0 },      { Op6BE0 },      { Op6C },        { Op6DM0 },
	{ Op6EM0 },      { Op6FM0 },      { Op70E0 },      { Op71E0M0X1 },  { Op72E0M0 },
	{ Op73M0 },      { Op74E0M0 },    { Op75E0M0 },    { Op76E0M0 },    { Op77M0 },
	{ Op78 },        { Op79M0X1 },    { Op7AE0X1 },    { Op7B },        { Op7C },
	{ Op7DM0X1 },    { Op7EM0X1 },    { Op7FM0 },      { Op80E0 },      { Op81E0M0 },
	{ Op82 },        { Op83M0 },      { Op84X1 },      { Op85M0 },      { Op86X1 },
	{ Op87M0 },      { Op88X1 },      { Op89M0 },      { Op8AM0 },      { Op8BE0 },
	{ Op8CX1 },      { Op8DM0 },      { Op8EX1 },      { Op8FM0 },      { Op90E0 },
	{ Op91E0M0X1 },  { Op92E0M0 },    { Op93M0 },      { Op94E0X1 },    { Op95E0M0 },
	{ Op96E0X1 },    { Op97M0 },      { Op98M0 },      { Op99M0X1 },    { Op9A },
	{ Op9BX1 },      { Op9CM0 },      { Op9DM0X1 },    { Op9EM0X1 },    { Op9FM0 },
	{ OpA0X1 },      { OpA1E0M0 },    { OpA2X1 },      { OpA3M0 },      { OpA4X1 },
	{ OpA5M0 },      { OpA6X1 },      { OpA7M0 },      { OpA8X1 },      { OpA9M0 },
	{ OpAAX1 },      { OpABE0 },      { OpACX1 },      { OpADM0 },      { OpAEX1 },
	{ OpAFM0 },      { OpB0E0 },      { OpB1E0M0X1 },  { OpB2E0M0 },    { OpB3M0 },
	{ OpB4E0X1 },    { OpB5E0M0 },    { OpB6E0X1 },    { OpB7M0 },      { OpB8 },
	{ OpB9M0X1 },    { OpBAX1 },      { OpBBX1 },      { OpBCX1 },      { OpBDM0X1 },
	{ OpBEX1 },      { OpBFM0 },      { OpC0X1 },      { OpC1E0M0 },    { OpC2 },
	{ OpC3M0 },      { OpC4X1 },      { OpC5M0 },      { OpC6M0 },      { OpC7M0 },
	{ OpC8X1 },      { OpC9M0 },      { OpCAX1 },      { OpCB },        { OpCCX1 },
	{ OpCDM0 },      { OpCEM0 },      { OpCFM0 },      { OpD0E0 },      { OpD1E0M0X1 },
	{ OpD2E0M0 },    { OpD3M0 },      { OpD4E0 },      { OpD5E0M0 },    { OpD6E0M0 },
	{ OpD7M0 },      { OpD8 },        { OpD9M0X1 },    { OpDAE0X1 },    { OpDB },
	{ OpDC },        { OpDDM0X1 },    { OpDEM0X1 },    { OpDFM0 },      { OpE0X1 },
	{ OpE1E0M0 },    { OpE2 },        { OpE3M0 },      { OpE4X1 },      { OpE5M0 },
	{ OpE6M0 },      { OpE7M0 },      { OpE8X1 },      { OpE9M0 },      { OpEA },
	{ OpEB },        { OpECX1 },      { OpEDM0 },      { OpEEM0 },      { OpEFM0 },
	{ OpF0E0 },      { OpF1E0M0X1 },  { OpF2E0M0 },    { OpF3M0 },      { OpF4E0 },
	{ OpF5E0M0 },    { OpF6E0M0 },    { OpF7M0 },      { OpF8 },        { OpF9M0X1 },
	{ OpFAE0X1 },    { OpFB },        { OpFCE0 },      { OpFDM0X1 },    { OpFEM0X1 },
	{ OpFFM0 }
};

struct SOpcodes S9xOpcodesSlow[256] =
{
	{ Op00 },        { Op01Slow },    { Op02 },        { Op03Slow },    { Op04Slow },
	{ Op05Slow },    { Op06Slow },    { Op07Slow },    { Op08Slow },    { Op09Slow },
	{ Op0ASlow },    { Op0BSlow },    { Op0CSlow },    { Op0DSlow },    { Op0ESlow },
	{ Op0FSlow },    { Op10Slow },    { Op11Slow },    { Op12Slow },    { Op13Slow },
	{ Op14Slow },    { Op15Slow },    { Op16Slow },    { Op17Slow },    { Op18 },
	{ Op19Slow },    { Op1ASlow },    { Op1B },        { Op1CSlow },    { Op1DSlow },
	{ Op1ESlow },    { Op1FSlow },    { Op20Slow },    { Op21Slow },    { Op22Slow },
	{ Op23Slow },    { Op24Slow },    { Op25Slow },    { Op26Slow },    { Op27Slow },
	{ Op28Slow },    { Op29Slow },    { Op2ASlow },    { Op2BSlow },    { Op2CSlow },
	{ Op2DSlow },    { Op2ESlow },    { Op2FSlow },    { Op30Slow },    { Op31Slow },
	{ Op32Slow },    { Op33Slow },    { Op34Slow },    { Op35Slow },    { Op36Slow },
	{ Op37Slow },    { Op38 },        { Op39Slow },    { Op3ASlow },    { Op3B },
	{ Op3CSlow },    { Op3DSlow },    { Op3ESlow },    { Op3FSlow },    { Op40Slow },
	{ Op41Slow },    { Op42 },        { Op43Slow },    { Op44Slow },    { Op45Slow },
	{ Op46Slow },    { Op47Slow },    { Op48Slow },    { Op49Slow },    { Op4ASlow },
	{ Op4BSlow },    { Op4CSlow },    { Op4DSlow },    { Op4ESlow },    { Op4FSlow },
	{ Op50Slow },    { Op51Slow },    { Op52Slow },    { Op53Slow },    { Op54Slow },
	{ Op55Slow },    { Op56Slow },    { Op57Slow },    { Op58 },        { Op59Slow },
	{ Op5ASlow },    { Op5B },        { Op5CSlow },    { Op5DSlow },    { Op5ESlow },
	{ Op5FSlow },    { Op60Slow },    { Op61Slow },    { Op62Slow },    { Op63Slow },
	{ Op64Slow },    { Op65Slow },    { Op66Slow },    { Op67Slow },    { Op68Slow },
	{ Op69Slow },    { Op6ASlow },    { Op6BSlow },    { Op6CSlow },    { Op6DSlow },
	{ Op6ESlow },    { Op6FSlow },    { Op70Slow },    { Op71Slow },    { Op72Slow },
	{ Op73Slow },    { Op74Slow },    { Op75Slow },    { Op76Slow },    { Op77Slow },
	{ Op78 },        { Op79Slow },    { Op7ASlow },    { Op7B },        { Op7CSlow },
	{ Op7DSlow },    { Op7ESlow },    { Op7FSlow },    { Op80Slow },    { Op81Slow },
	{ Op82Slow },    { Op83Slow },    { Op84Slow },    { Op85Slow },    { Op86Slow },
	{ Op87Slow },    { Op88Slow },    { Op89Slow },    { Op8ASlow },    { Op8BSlow },
	{ Op8CSlow },    { Op8DSlow },    { Op8ESlow },    { Op8FSlow },    { Op90Slow },
	{ Op91Slow },    { Op92Slow },    { Op93Slow },    { Op94Slow },    { Op95Slow },
	{ Op96Slow },    { Op97Slow },    { Op98Slow },    { Op99Slow },    { Op9A },
	{ Op9BSlow },    { Op9CSlow },    { Op9DSlow },    { Op9ESlow },    { Op9FSlow },
	{ OpA0Slow },    { OpA1Slow },    { OpA2Slow },    { OpA3Slow },    { OpA4Slow },
	{ OpA5Slow },    { OpA6Slow },    { OpA7Slow },    { OpA8Slow },    { OpA9Slow },
	{ OpAASlow },    { OpABSlow },    { OpACSlow },    { OpADSlow },    { OpAESlow },
	{ OpAFSlow },    { OpB0Slow },    { OpB1Slow },    { OpB2Slow },    { OpB3Slow },
	{ OpB4Slow },    { OpB5Slow },    { OpB6Slow },    { OpB7Slow },    { OpB8 },
	{ OpB9Slow },    { OpBASlow },    { OpBBSlow },    { OpBCSlow },    { OpBDSlow },
	{ OpBESlow },    { OpBFSlow },    { OpC0Slow },    { OpC1Slow },    { OpC2Slow },
	{ OpC3Slow },    { OpC4Slow },    { OpC5Slow },    { OpC6Slow },    { OpC7Slow },
	{ OpC8Slow },    { OpC9Slow },    { OpCASlow },    { OpCB },        { OpCCSlow },
	{ OpCDSlow },    { OpCESlow },    { OpCFSlow },    { OpD0Slow },    { OpD1Slow },
	{ OpD2Slow },    { OpD3Slow },    { OpD4Slow },    { OpD5Slow },    { OpD6Slow },
	{ OpD7Slow },    { OpD8 },        { OpD9Slow },    { OpDASlow },    { OpDB },
	{ OpDCSlow },    { OpDDSlow },    { OpDESlow },    { OpDFSlow },    { OpE0Slow },
	{ OpE1Slow },    { OpE2Slow },    { OpE3Slow },    { OpE4Slow },    { OpE5Slow },
	{ OpE6Slow },    { OpE7Slow },    { OpE8Slow },    { OpE9Slow },    { OpEA },
	{ OpEB },        { OpECSlow },    { OpEDSlow },    { OpEESlow },    { OpEFSlow },
	{ OpF0Slow },    { OpF1Slow },    { OpF2Slow },    { OpF3Slow },    { OpF4Slow },
	{ OpF5Slow },    { OpF6Slow },    { OpF7Slow },    { OpF8 },        { OpF9Slow },
	{ OpFASlow },    { OpFB },        { OpFCSlow },    { OpFDSlow },    { OpFESlow },
	{ OpFFSlow }
};
