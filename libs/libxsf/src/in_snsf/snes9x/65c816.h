/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

inline constexpr uint16_t Carry = 1;
inline constexpr uint16_t Zero = 2;
inline constexpr uint16_t IRQ = 4;
inline constexpr uint16_t Decimal = 8;
inline constexpr uint16_t IndexFlag = 16;
inline constexpr uint16_t MemoryFlag = 32;
inline constexpr uint16_t Overflow = 64;
inline constexpr uint16_t Negative = 128;
inline constexpr uint16_t Emulation = 256;

union pair
{
#ifdef LSB_FIRST
	struct { uint8_t l, h; } B;
#else
	struct { uint8_t h, l; } B;
#endif
	uint16_t W;
};

union PC_t
{
#ifdef LSB_FIRST
	struct { uint8_t xPCl, xPCh, xPB, z; } B;
	struct { uint16_t xPC, d; } W;
#else
	struct { uint8_t z, xPB, xPCh, xPCl; } B;
	struct { uint16_t d, xPC; } W;
#endif
	uint32_t xPBPC;
};

struct SRegisters
{
	uint8_t DB;
	pair P;
	pair A;
	pair D;
	pair S;
	pair X;
	pair Y;
	PC_t PC;
};

extern SRegisters Registers;

inline void SetCarry() { ICPU._Carry = 1; }
inline void ClearCarry() { ICPU._Carry = 0; }
inline void SetIRQ() { Registers.P.B.l |= IRQ; }
inline void ClearIRQ() { Registers.P.B.l &= ~IRQ; }
inline void SetDecimal() { Registers.P.B.l |= Decimal; }
inline void ClearDecimal() { Registers.P.B.l &= ~Decimal; }
inline void SetOverflow() { ICPU._Overflow = 1; }
inline void ClearOverflow() { ICPU._Overflow = 0; }

inline bool CheckCarry() { return !!ICPU._Carry; }
inline bool CheckZero() { return !ICPU._Zero; }
inline bool CheckDecimal() { return !!(Registers.P.B.l & Decimal); }
inline bool CheckIndex() { return !!(Registers.P.B.l & IndexFlag); }
inline bool CheckMemory() { return !!(Registers.P.B.l & MemoryFlag); }
inline bool CheckOverflow() { return !!(ICPU._Overflow); }
inline bool CheckNegative() { return !!(ICPU._Negative & 0x80); }
inline bool CheckEmulation() { return !!(Registers.P.W & Emulation); }

inline void SetFlags(uint16_t f) { Registers.P.W |= f; }
inline void ClearFlags(uint16_t f) { Registers.P.W &= ~f; }
inline bool CheckFlag(uint16_t f) { return !!(Registers.P.W & f); }
