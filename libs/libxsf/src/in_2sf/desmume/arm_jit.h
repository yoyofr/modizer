/*	Copyright (C) 2006 yopyop
	Copyright (C) 2011 Loren Merritt
	Copyright (C) 2012 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "types.h"

typedef uint32_t (FASTCALL *ArmOpCompiled)();

void arm_jit_reset(bool enable);
void arm_jit_close();
void arm_jit_sync();
template<int PROCNUM> uint32_t arm_jit_compile();

#if defined(_WINDOWS) || defined(DESMUME_COCOA)
# define MAPPED_JIT_FUNCS
#endif

#ifdef MAPPED_JIT_FUNCS
struct JIT_struct
{
	// only include the memory types that code can execute from
	uintptr_t MAIN_MEM[0x800000];
	uintptr_t SWIRAM[0x4000];
	uintptr_t ARM9_ITCM[0x4000];
	uintptr_t ARM9_LCDC[0x52000];
	uintptr_t ARM9_BIOS[0x4000];
	uintptr_t ARM7_BIOS[0x2000];
	uintptr_t ARM7_ERAM[0x8000];
	uintptr_t ARM7_WIRAM[0x8000];
	uintptr_t ARM7_WRAM[0x20000];

	static uintptr_t *JIT_MEM[2][0x4000];
};
extern CACHE_ALIGN JIT_struct JIT;
inline uintptr_t &JIT_COMPILED_FUNC(uint32_t adr, uint32_t PROCNUM) { return JIT.JIT_MEM[PROCNUM][(adr & 0x0FFFC000) >> 14][(adr & 0x00003FFE) >> 1]; }
inline uintptr_t &JIT_COMPILED_FUNC_PREMASKED(uint32_t adr, uint32_t PROCNUM, uint32_t ofs) { return JIT.JIT_MEM[PROCNUM][adr >> 14][((adr & 0x00003FFE) >> 1) + ofs]; }
#define JIT_COMPILED_FUNC_KNOWNBANK(adr, bank, mask, ofs) JIT.bank[(((adr) & (mask)) >> 1) + ofs]
inline bool JIT_MAPPED(uint32_t adr, uint32_t PROCNUM) { return !!JIT.JIT_MEM[PROCNUM][adr >> 14]; }
#else
// actually an array of function pointers, but they fit in 32bit address space, so might as well save memory
extern uintptr_t compiled_funcs[];
// there isn't anything mapped between 07000000 and 0EFFFFFF, so we can mask off bit 27 and get away with a smaller array
inline uintptr_r &JIT_COMPILED_FUNC(uint32_t adr, uint32_t PROCNUM) { compiled_funcs[(adr & 0x07FFFFFE) >> 1]; }
inline uintptr_t &JIT_COMPILED_FUNC_PREMASKED(uint32_t adr, uint32_t PROCNUM, uint32_t ofs) { return JIT_COMPILED_FUNC(adr, PROCNUM); }
#define JIT_COMPILED_FUNC_KNOWNBANK(adr, bank, mask, ofs) JIT_COMPILED_FUNC(adr, PROCNUM)
inline bool JIT_MAPPED(uint32_t adr, uint32_t PROCNUM) { return true; }
#endif
