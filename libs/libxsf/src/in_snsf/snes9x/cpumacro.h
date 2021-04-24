/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

template<typename Fa, typename Ff> inline void rOP8(Fa addr, Ff func)
{
	uint8_t val = OpenBus = S9xGetByte(addr(READ));
	func(val);
}

template<typename Fa, typename Ff> inline void rOP16(Fa addr, Ff func, s9xwrap_t wrap = WRAP_NONE)
{
	uint16_t val = S9xGetWord(addr(READ), wrap);
	OpenBus = static_cast<uint8_t>(val >> 8);
	func(val);
}

template<typename Fa, typename Ff8, typename Ff16> inline void rOPC(bool cond, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	if (cond)
		rOP8(addr, func8);
	else
		rOP16(addr, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void rOPM(Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	rOPC(CheckMemory(), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void rOPX(Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	rOPC(CheckIndex(), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff> inline void wOP8(Fa addr, Ff func)
{
	func(addr(WRITE));
}

template<typename Fa, typename Ff> inline void wOP16(Fa addr, Ff func, s9xwrap_t wrap = WRAP_NONE)
{
	func(addr(WRITE), wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void wOPC(bool cond, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	if (cond)
		wOP8(addr, func8);
	else
		wOP16(addr, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void wOPM(Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	wOPC(CheckMemory(), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void wOPX(Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	wOPC(CheckIndex(), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff> inline void mOP8(Fa addr, Ff func)
{
	func(addr(MODIFY));
}

template<typename Fa, typename Ff> inline void mOP16(Fa addr, Ff func, s9xwrap_t wrap = WRAP_NONE)
{
	func(addr(MODIFY), wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void mOPC(bool cond, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	if (cond)
		mOP8(addr, func8);
	else
		mOP16(addr, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void mOPM(Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	mOPC(CheckMemory(), addr, func8, func16, wrap);
}

template<typename F> inline void bOP(F rel, bool cond, bool e)
{
	pair newPC;
	newPC.W = rel(JUMP);
	if (cond)
	{
		AddCycles(ONE_CYCLE);
		if (e && Registers.PC.B.xPCh != newPC.B.h)
			AddCycles(ONE_CYCLE);
		if ((Registers.PC.W.xPC & ~MEMMAP_MASK) != (newPC.W & ~MEMMAP_MASK))
			S9xSetPCBase(ICPU.ShiftedPB + newPC.W);
		else
			Registers.PC.W.xPC = newPC.W;
	}
}

inline void SetZN(uint16_t Work16)
{
	ICPU._Zero = !!Work16;
	ICPU._Negative = static_cast<uint8_t>(Work16 >> 8);
}

inline void SetZN(uint8_t Work8)
{
	ICPU._Zero = Work8;
	ICPU._Negative = Work8;
}

inline void ADC16(uint16_t Work16)
{
	if (CheckDecimal())
	{
		uint32_t carry = CheckCarry();

		uint32_t result = (Registers.A.W & 0x000F) + (Work16 & 0x000F) + carry;
		if (result > 0x0009)
			result += 0x0006;
		carry = result > 0x000F;

		result = (Registers.A.W & 0x00F0) + (Work16 & 0x00F0) + (result & 0x000F) + carry * 0x10;
		if (result > 0x009F)
			result += 0x0060;
		carry = result > 0x00FF;

		result = (Registers.A.W & 0x0F00) + (Work16 & 0x0F00) + (result & 0x00FF) + carry * 0x100;
		if (result > 0x09FF)
			result += 0x0600;
		carry = result > 0x0FFF;

		result = (Registers.A.W & 0xF000) + (Work16 & 0xF000) + (result & 0x0FFF) + carry * 0x1000;

		if ((Registers.A.W & 0x8000) == (Work16 & 0x8000) && (Registers.A.W & 0x8000) != (result & 0x8000))
			SetOverflow();
		else
			ClearOverflow();

		if (result > 0x9FFF)
			result += 0x6000;

		if (result > 0xFFFF)
			SetCarry();
		else
			ClearCarry();

		Registers.A.W = result & 0xFFFF;
		SetZN(Registers.A.W);
	}
	else
	{
		uint32_t Ans32 = Registers.A.W + Work16 + CheckCarry();

		ICPU._Carry = Ans32 >= 0x10000;

		if (~(Registers.A.W ^ Work16) & (Work16 ^ static_cast<uint16_t>(Ans32)) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = static_cast<uint16_t>(Ans32);
		SetZN(Registers.A.W);
	}
}

inline void ADC8(uint8_t Work8)
{
	if (CheckDecimal())
	{
		uint32_t carry = CheckCarry();

		uint32_t result = (Registers.A.B.l & 0x0F) + (Work8 & 0x0F) + carry;
		if (result > 0x09)
			result += 0x06;
		carry = result > 0x0F;

		result = (Registers.A.B.l & 0xF0) + (Work8 & 0xF0) + (result & 0x0F) + (carry * 0x10);

		if ((Registers.A.B.l & 0x80) == (Work8 & 0x80) && (Registers.A.B.l & 0x80) != (result & 0x80))
			SetOverflow();
		else
			ClearOverflow();

		if (result > 0x9F)
			result += 0x60;

		if (result > 0xFF)
			SetCarry();
		else
			ClearCarry();

		Registers.A.B.l = result & 0xFF;
		SetZN(Registers.A.B.l);
	}
	else
	{
		uint16_t Ans16 = Registers.A.B.l + Work8 + CheckCarry();

		ICPU._Carry = Ans16 >= 0x100;

		if (~(Registers.A.B.l ^ Work8) & (Work8 ^ static_cast<uint8_t>(Ans16)) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.B.l = static_cast<uint8_t>(Ans16);
		SetZN(Registers.A.B.l);
	}
}

inline void AND16(uint16_t Work16)
{
	Registers.A.W &= Work16;
	SetZN(Registers.A.W);
}

inline void AND8(uint8_t Work8)
{
	Registers.A.B.l &= Work8;
	SetZN(Registers.A.B.l);
}

inline void ASL16(uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w);
	ICPU._Carry = !!(Work16 & 0x8000);
	Work16 <<= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

inline void ASL8(uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress);
	ICPU._Carry = !!(Work8 & 0x80);
	Work8 <<= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

inline void BIT16(uint16_t Work16)
{
	ICPU._Overflow = !!(Work16 & 0x4000);
	ICPU._Negative = static_cast<uint8_t>(Work16 >> 8);
	ICPU._Zero = !!(Work16 & Registers.A.W);
}

inline void BIT8(uint8_t Work8)
{
	ICPU._Overflow = !!(Work8 & 0x40);
	ICPU._Negative = Work8;
	ICPU._Zero = !!(Work8 & Registers.A.B.l);
}

inline void CMP16(uint16_t val)
{
	int32_t Int32 = static_cast<int32_t>(Registers.A.W) - static_cast<int32_t>(val);
	ICPU._Carry = Int32 >= 0;
	SetZN(static_cast<uint16_t>(Int32));
}

inline void CMP8(uint8_t val)
{
	int16_t Int16 = static_cast<int16_t>(Registers.A.B.l) - static_cast<int16_t>(val);
	ICPU._Carry = Int16 >= 0;
	SetZN(static_cast<uint8_t>(Int16));
}

inline void CPX16(uint16_t val)
{
	int32_t Int32 = static_cast<int32_t>(Registers.X.W) - static_cast<int32_t>(val);
	ICPU._Carry = Int32 >= 0;
	SetZN(static_cast<uint16_t>(Int32));
}

inline void CPX8(uint8_t val)
{
	int16_t Int16 = static_cast<int16_t>(Registers.X.B.l) - static_cast<int16_t>(val);
	ICPU._Carry = Int16 >= 0;
	SetZN(static_cast<uint8_t>(Int16));
}

inline void CPY16(uint16_t val)
{
	int32_t Int32 = static_cast<int32_t>(Registers.Y.W) - static_cast<int32_t>(val);
	ICPU._Carry = Int32 >= 0;
	SetZN(static_cast<uint16_t>(Int32));
}

inline void CPY8(uint8_t val)
{
	int16_t Int16 = static_cast<int16_t>(Registers.Y.B.l) - static_cast<int16_t>(val);
	ICPU._Carry = Int16 >= 0;
	SetZN(static_cast<uint8_t>(Int16));
}

inline void DEC16(uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w) - 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

inline void DEC8(uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress) - 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

inline void EOR16(uint16_t val)
{
	Registers.A.W ^= val;
	SetZN(Registers.A.W);
}

inline void EOR8(uint8_t val)
{
	Registers.A.B.l ^= val;
	SetZN(Registers.A.B.l);
}

inline void INC16(uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w) + 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

inline void INC8(uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress) + 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

inline void LDA16(uint16_t val)
{
	Registers.A.W = val;
	SetZN(Registers.A.W);
}

inline void LDA8(uint8_t val)
{
	Registers.A.B.l = val;
	SetZN(Registers.A.B.l);
}

inline void LDX16(uint16_t val)
{
	Registers.X.W = val;
	SetZN(Registers.X.W);
}

inline void LDX8(uint8_t val)
{
	Registers.X.B.l = val;
	SetZN(Registers.X.B.l);
}

inline void LDY16(uint16_t val)
{
	Registers.Y.W = val;
	SetZN(Registers.Y.W);
}

inline void LDY8(uint8_t val)
{
	Registers.Y.B.l = val;
	SetZN(Registers.Y.B.l);
}

inline void LSR16(uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w);
	ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
	SetZN(Work16);
}

inline void LSR8(uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress);
	ICPU._Carry = Work8 & 1;
	Work8 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
	SetZN(Work8);
}

inline void ORA16(uint16_t val)
{
	Registers.A.W |= val;
	SetZN(Registers.A.W);
}

inline void ORA8(uint8_t val)
{
	Registers.A.B.l |= val;
	SetZN(Registers.A.B.l);
}

inline void ROL16(uint32_t OpAddress, s9xwrap_t w)
{
	uint32_t Work32 = (static_cast<uint32_t>(S9xGetWord(OpAddress, w)) << 1) | static_cast<uint32_t>(CheckCarry());
	ICPU._Carry = Work32 >= 0x10000;
	AddCycles(ONE_CYCLE);
	S9xSetWord(static_cast<uint16_t>(Work32), OpAddress, w, WRITE_10);
	OpenBus = Work32 & 0xff;
	SetZN(static_cast<uint16_t>(Work32));
}

inline void ROL8(uint32_t OpAddress)
{
	uint16_t Work16 = (static_cast<uint16_t>(S9xGetByte(OpAddress)) << 1) | static_cast<uint16_t>(CheckCarry());
	ICPU._Carry = Work16 >= 0x100;
	AddCycles(ONE_CYCLE);
	S9xSetByte(static_cast<uint8_t>(Work16), OpAddress);
	OpenBus = Work16 & 0xff;
	SetZN(static_cast<uint8_t>(Work16));
}

inline void ROR16(uint32_t OpAddress, s9xwrap_t w)
{
	uint32_t Work32 = static_cast<uint32_t>(S9xGetWord(OpAddress, w)) | (static_cast<uint32_t>(CheckCarry()) << 16);
	ICPU._Carry = Work32 & 1;
	Work32 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetWord(static_cast<uint16_t>(Work32), OpAddress, w, WRITE_10);
	OpenBus = Work32 & 0xff;
	SetZN(static_cast<uint16_t>(Work32));
}

inline void ROR8(uint32_t OpAddress)
{
	uint16_t Work16 = static_cast<uint16_t>(S9xGetByte(OpAddress)) | (static_cast<uint16_t>(CheckCarry()) << 8);
	ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(ONE_CYCLE);
	S9xSetByte(static_cast<uint8_t>(Work16), OpAddress);
	OpenBus = Work16 & 0xff;
	SetZN(static_cast<uint8_t>(Work16));
}

inline void SBC16(uint16_t Work16)
{
	if (CheckDecimal())
	{
		int carry = CheckCarry();

		Work16 ^= 0xFFFF;

		int result = (Registers.A.W & 0x000F) + (Work16 & 0x000F) + carry;
		if (result < 0x0010)
			result -= 0x0006;
		carry = result > 0x000F;

		result = (Registers.A.W & 0x00F0) + (Work16 & 0x00F0) + (result & 0x000F) + carry * 0x10;
		if (result < 0x0100)
			result -= 0x0060;
		carry = result > 0x00FF;

		result = (Registers.A.W & 0x0F00) + (Work16 & 0x0F00) + (result & 0x00FF) + carry * 0x100;
		if (result < 0x1000)
			result -= 0x0600;
		carry = result > 0x0FFF;

		result = (Registers.A.W & 0xF000) + (Work16 & 0xF000) + (result & 0x0FFF) + carry * 0x1000;

		if (!((Registers.A.W ^ Work16) & 0x8000) && ((Registers.A.W ^ result) & 0x8000))
			SetOverflow();
		else
			ClearOverflow();

		if (result < 0x10000)
			result -= 0x6000;

		if (result > 0xFFFF)
			SetCarry();
		else
			ClearCarry();

		Registers.A.W = result & 0xFFFF;
		SetZN(Registers.A.W);
	}
	else
	{
		int32_t Int32 = static_cast<int32_t>(Registers.A.W) - static_cast<int32_t>(Work16) + static_cast<int32_t>(CheckCarry()) - 1;

		ICPU._Carry = Int32 >= 0;

		if ((Registers.A.W ^ Work16) & (Registers.A.W ^ static_cast<uint16_t>(Int32)) & 0x8000)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.W = static_cast<uint16_t>(Int32);
		SetZN(Registers.A.W);
	}
}

inline void SBC8(uint8_t Work8)
{
	if (CheckDecimal())
	{
		int carry = CheckCarry();

		Work8 ^= 0xFF;

		int result = (Registers.A.B.l & 0x0F) + (Work8 & 0x0F) + carry;
		if (result < 0x10)
			result -= 0x06;
		carry = (result > 0x0F);

		result = (Registers.A.B.l & 0xF0) + (Work8 & 0xF0) + (result & 0x0F) + carry * 0x10;

		if ((Registers.A.B.l & 0x80) == (Work8 & 0x80) && (Registers.A.B.l & 0x80) != (result & 0x80))
			SetOverflow();
		else
			ClearOverflow();

		if (result < 0x100)
			result -= 0x60;

		if (result > 0xFF)
			SetCarry();
		else
			ClearCarry();

		Registers.A.B.l = result & 0xFF;
		SetZN(Registers.A.B.l);
	}
	else
	{
		int16_t Int16 = static_cast<int16_t>(Registers.A.B.l) - static_cast<int16_t>(Work8) + static_cast<int16_t>(CheckCarry()) - 1;

		ICPU._Carry = Int16 >= 0;

		if ((Registers.A.B.l ^ Work8) & (Registers.A.B.l ^ static_cast<uint8_t>(Int16)) & 0x80)
			SetOverflow();
		else
			ClearOverflow();

		Registers.A.B.l = static_cast<uint8_t>(Int16);
		SetZN(Registers.A.B.l);
	}
}

inline void STA16(uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(Registers.A.W, OpAddress, w);
	OpenBus = Registers.A.B.h;
}

inline void STA8(uint32_t OpAddress)
{
	S9xSetByte(Registers.A.B.l, OpAddress);
	OpenBus = Registers.A.B.l;
}

inline void STX16(uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(Registers.X.W, OpAddress, w);
	OpenBus = Registers.X.B.h;
}

inline void STX8(uint32_t OpAddress)
{
	S9xSetByte(Registers.X.B.l, OpAddress);
	OpenBus = Registers.X.B.l;
}

inline void STY16(uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(Registers.Y.W, OpAddress, w);
	OpenBus = Registers.Y.B.h;
}

inline void STY8(uint32_t OpAddress)
{
	S9xSetByte(Registers.Y.B.l, OpAddress);
	OpenBus = Registers.Y.B.l;
}

inline void STZ16(uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(0, OpAddress, w);
	OpenBus = 0;
}

inline void STZ8(uint32_t OpAddress)
{
	S9xSetByte(0, OpAddress);
	OpenBus = 0;
}

inline void TRB16(uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w);
	ICPU._Zero = !!(Work16 & Registers.A.W);
	Work16 &= ~Registers.A.W;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
}

inline void TRB8(uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress);
	ICPU._Zero = Work8 & Registers.A.B.l;
	Work8 &= ~Registers.A.B.l;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
}

inline void TSB16(uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(OpAddress, w);
	ICPU._Zero = !!(Work16 & Registers.A.W);
	Work16 |= Registers.A.W;
	AddCycles(ONE_CYCLE);
	S9xSetWord(Work16, OpAddress, w, WRITE_10);
	OpenBus = Work16 & 0xff;
}

inline void TSB8(uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(OpAddress);
	ICPU._Zero = Work8 & Registers.A.B.l;
	Work8 |= Registers.A.B.l;
	AddCycles(ONE_CYCLE);
	S9xSetByte(Work8, OpAddress);
	OpenBus = Work8;
}
