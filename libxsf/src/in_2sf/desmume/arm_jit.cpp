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

#include "types.h"
#ifdef HAVE_JIT
#if !defined(__x86_64__) && !defined(__LP64) && !defined(__IA64__) && !defined(_M_X64) && !defined(_WIN64) && !defined(_M_IX86) && !defined(__INTEL__) && !defined(__i386__)
#error "ERROR: JIT compiler - unsupported target platform"
#endif
#ifdef _WINDOWS
// **** Windows port
#else
# include <sys/mman.h>
# include <errno.h>
# include <unistd.h>
# include <stddef.h>
# define HAVE_STATIC_CODE_BUFFER
#endif
#include "instructions.h"
#include "instruction_attributes.h"
#include "MMU.h"
#include "MMU_timing.h"
#include "utils/AsmJit/asmjit.h"
#include "arm_jit.h"
#include "bios.h"

#define LOG_JIT_LEVEL 0
#define PROFILER_JIT_LEVEL 0

#if PROFILER_JIT_LEVEL > 0
# include <algorithm>
#endif

using namespace asmjit;

#if LOG_JIT_LEVEL > 0
#define LOG_JIT 1
#define JIT_COMMENT(...) c.comment(__VA_ARGS__)
#define printJIT(buf, val) \
{ \
	JIT_COMMENT("printJIT(\""##buf"\", val);"); \
	GpVar txt = c.newGpVar(kVarTypeIntPtr); \
	GpVar data = c.newGpVar(kVarTypeIntPtr); \
	GpVar io = c.newGpVar(kVarTypeInt32); \
	c.lea(io, x86::dword_ptr_abs(stdout)); \
	c.lea(txt, x86::dword_ptr_abs(&buf)); \
	c.mov(data, *reinterpret_cast<GpVar *>(&val)); \
	auto prn = c.addCall(imm_ptr(fprintf), ASMJIT_CALL_CONV, FuncBuilder3<void, void *, void *, uint32_t>()); \
	prn->setArg(0, io); \
	prn->setArg(1, txt); \
	prn->setArg(2, data); \
	auto prn_flush = c.addCall(imm_ptr(fflush), ASMJIT_CALL_CONV, FuncBuilder1<void, void *>()); \
	prn_flush->setArg(0, io); \
}
#else
#define LOG_JIT 0
#define JIT_COMMENT(...)
#define printJIT(buf, val)
#endif

#ifdef MAPPED_JIT_FUNCS
CACHE_ALIGN JIT_struct JIT;

uintptr_t *JIT_struct::JIT_MEM[2][0x4000] = { { 0 }, { 0 } };

static uintptr_t *JIT_MEM[][32] =
{
	//arm9
	{
		/* 0X*/	DUP2(JIT.ARM9_ITCM),
		/* 1X*/	DUP2(JIT.ARM9_ITCM), // mirror
		/* 2X*/	DUP2(JIT.MAIN_MEM),
		/* 3X*/	DUP2(JIT.SWIRAM),
		/* 4X*/	DUP2(nullptr),
		/* 5X*/	DUP2(nullptr),
		/* 6X*/	nullptr,
				JIT.ARM9_LCDC, // Plain ARM9-CPU Access (LCDC mode) (max 656KB)
		/* 7X*/	DUP2(nullptr),
		/* 8X*/	DUP2(nullptr),
		/* 9X*/	DUP2(nullptr),
		/* AX*/	DUP2(nullptr),
		/* BX*/	DUP2(nullptr),
		/* CX*/	DUP2(nullptr),
		/* DX*/	DUP2(nullptr),
		/* EX*/	DUP2(nullptr),
		/* FX*/	DUP2(JIT.ARM9_BIOS)
	},
	//arm7
	{
		/* 0X*/	DUP2(JIT.ARM7_BIOS),
		/* 1X*/	DUP2(nullptr),
		/* 2X*/	DUP2(JIT.MAIN_MEM),
		/* 3X*/	JIT.SWIRAM,
				JIT.ARM7_ERAM,
		/* 4X*/	nullptr,
				JIT.ARM7_WIRAM,
		/* 5X*/	DUP2(nullptr),
		/* 6X*/	JIT.ARM7_WRAM,		// VRAM allocated as Work RAM to ARM7 (max. 256K)
				nullptr,
		/* 7X*/	DUP2(nullptr),
		/* 8X*/	DUP2(nullptr),
		/* 9X*/	DUP2(nullptr),
		/* AX*/	DUP2(nullptr),
		/* BX*/	DUP2(nullptr),
		/* CX*/	DUP2(nullptr),
		/* DX*/	DUP2(nullptr),
		/* EX*/	DUP2(nullptr),
		/* FX*/	DUP2(nullptr)
	}
};

static uint32_t JIT_MASK[][32] =
{
	//arm9
	{
		/* 0X*/	DUP2(0x00007FFF),
		/* 1X*/	DUP2(0x00007FFF),
		/* 2X*/	DUP2(0x003FFFFF), // FIXME _MMU_MAIN_MEM_MASK
		/* 3X*/	DUP2(0x00007FFF),
		/* 4X*/	DUP2(0x00000000),
		/* 5X*/	DUP2(0x00000000),
		/* 6X*/	0x00000000,
				0x000FFFFF,
		/* 7X*/	DUP2(0x00000000),
		/* 8X*/	DUP2(0x00000000),
		/* 9X*/	DUP2(0x00000000),
		/* AX*/	DUP2(0x00000000),
		/* BX*/	DUP2(0x00000000),
		/* CX*/	DUP2(0x00000000),
		/* DX*/	DUP2(0x00000000),
		/* EX*/	DUP2(0x00000000),
		/* FX*/	DUP2(0x00007FFF)
	},
	//arm7
	{
		/* 0X*/	DUP2(0x00003FFF),
		/* 1X*/	DUP2(0x00000000),
		/* 2X*/	DUP2(0x003FFFFF),
		/* 3X*/	0x00007FFF,
				0x0000FFFF,
		/* 4X*/	0x00000000,
				0x0000FFFF,
		/* 5X*/	DUP2(0x00000000),
		/* 6X*/	0x0003FFFF,
				0x00000000,
		/* 7X*/	DUP2(0x00000000),
		/* 8X*/	DUP2(0x00000000),
		/* 9X*/	DUP2(0x00000000),
		/* AX*/	DUP2(0x00000000),
		/* BX*/	DUP2(0x00000000),
		/* CX*/	DUP2(0x00000000),
		/* DX*/	DUP2(0x00000000),
		/* EX*/	DUP2(0x00000000),
		/* FX*/	DUP2(0x00000000)
	}
};

static void init_jit_mem()
{
	static bool inited = false;
	if (inited)
		return;
	inited = true;
	for (int proc = 0; proc < 2; ++proc)
		for (int i = 0; i < 0x4000; ++i)
			JIT.JIT_MEM[proc][i] = JIT_MEM[proc][i >> 9] + (((i << 14) & JIT_MASK[proc][i >> 9]) >> 1);
}
#else
DS_ALIGN(4096) uintptr_t compiled_funcs[1 << 26] = {0};
#endif

static uint8_t recompile_counts[(1 << 26) / 16];

#ifdef HAVE_STATIC_CODE_BUFFER
// On x86_64, allocate jitted code from a static buffer to ensure that it's within 2GB of .text
// Allows call instructions to use pcrel offsets, as opposed to slower indirect calls.
// Reduces memory needed for function pointers.
// FIXME win64 needs this too, x86_32 doesn't

DS_ALIGN(4096) static uint8_t scratchpad[1 << 25];
static uint8_t *scratchptr;

struct ASMJIT_API StaticCodeGenerator : public Context
{
	StaticCodeGenerator()
	{
		scratchptr = scratchpad;
		int align = reinterpret_cast<uintptr_t>(scratchpad) & (sysconf(_SC_PAGESIZE) - 1);
		int err = mprotect(scratchpad - align, sizeof(scratchpad) + align, PROT_READ | PROT_WRITE | PROT_EXEC);
		if (err)
		{
			fprintf(stderr, "mprotect failed: %s\n", strerror(errno));
			abort();
		}
	}

	uint32_t generate(void **dest, Assembler *assembler)
	{
		uintptr_t size = assembler->getCodeSize();
		if (!size)
		{
			*dest = nullptr;
			return kErrorNoFunction;
		}
		if (size > reinterpret_cast<uintptr_t>(scratchpad + sizeof(scratchpad) - scratchptr))
		{
			fprintf(stderr, "Out of memory for asmjit. Clearing code cache.\n");
			arm_jit_reset(true);
			// If arm_jit_reset didn't involve recompiling op_cmp, we could keep the current function.
			*dest = nullptr;
			return kErrorOk;
		}
		void *p = scratchptr;
		size = assembler->relocCode(p);
		scratchptr += size;
		*dest = p;
		return kErrorOk;
	}
};

static StaticCodeGenerator codegen;
static X86Compiler c(&codegen);
#else
static JitRuntime runtime;
static X86Compiler c(&runtime);
#endif

static void emit_branch(int cond, Label to);
static void _armlog(uint8_t proc, uint32_t addr, uint32_t opcode);

static FileLogger logger(stderr);

static int PROCNUM;
static int *PROCNUM_ptr = &PROCNUM;
static int bb_opcodesize;
static int bb_adr;
static bool bb_thumb;
static GpVar bb_cpu;
static GpVar bb_cycles;
static GpVar bb_total_cycles;
static uint32_t bb_constant_cycles;

#define cpu (&ARMPROC)
#define bb_next_instruction (bb_adr + bb_opcodesize)
#define bb_r15 (bb_adr + 2 * bb_opcodesize)

#define cpu_ptr(x) x86::dword_ptr(bb_cpu, offsetof(armcpu_t, x))
#define cpu_ptr_byte(x, y) x86::byte_ptr(bb_cpu, offsetof(armcpu_t, x) + y)
#define flags_ptr cpu_ptr_byte(CPSR.val, 3)
#define reg_ptr(x) x86::dword_ptr(bb_cpu, offsetof(armcpu_t, R) + 4 * (x))
#define reg_pos_ptr(x) x86::dword_ptr(bb_cpu, offsetof(armcpu_t, R) + 4 * REG_POS(i, (x)))
#define reg_pos_ptrL(x) x86::word_ptr(bb_cpu, offsetof(armcpu_t, R) + 4 * REG_POS(i, (x)))
#define reg_pos_ptrH(x) x86::word_ptr(bb_cpu, offsetof(armcpu_t, R) + 4 * REG_POS(i, (x)) + 2)
#define reg_pos_ptrB(x) x86::byte_ptr(bb_cpu, offsetof(armcpu_t, R) + 4 * REG_POS(i, (x)))
#define reg_pos_thumb(x) x86::dword_ptr(bb_cpu, offsetof(armcpu_t, R) + 4 * ((i >> (x)) & 0x7))
#define reg_pos_thumbB(x) x86::byte_ptr(bb_cpu, offsetof(armcpu_t, R) + 4 * ((i >> (x)) & 0x7))
#define cp15_ptr(x) x86::dword_ptr(bb_cp15, offsetof(armcp15_t, x))
#define mmu_ptr(x) x86::dword_ptr(bb_mmu, offsetof(MMU_struct, x))
#define mmu_ptr_byte(x) x86::byte_ptr(bb_mmu, offsetof(MMU_struct, x))
static inline uint32_t _REG_NUM(uint32_t i, uint32_t n) { return (i >> n) & 0x7; }

#ifndef ASMJIT_X64
#define r64 r32
#endif

// sequencer.reschedule = true;
#define changeCPSR \
{ \
	auto ctxCPSR = c.addCall(imm_ptr(NDS_Reschedule), ASMJIT_CALL_CONV, FuncBuilder0<void>()); \
}

#if PROFILER_JIT_LEVEL > 0
struct PROFILER_COUNTER_INFO
{
	uint64_t count;
	char name[64];
};

struct JIT_PROFILER
{
	JIT_PROFILER::JIT_PROFILER()
	{
		memset(&this->arm_count[0], 0, sizeof(this->arm_count));
		memset(&this->thumb_count[0], 0, sizeof(this->thumb_count));
	}

	uint64_t arm_count[4096];
	uint64_t thumb_count[1024];
} profiler_counter[2];

static GpVar bb_profiler;

#define profiler_counter_arm(opcode) x86::qword_ptr(bb_profiler, offsetof(JIT_PROFILER, arm_count[INSTRUCTION_INDEX(opcode)]))
#define profiler_counter_thumb(opcode) x86::qword_ptr(bb_profiler, offsetof(JIT_PROFILER, thumb_count[opcode>>6]))

#if PROFILER_JIT_LEVEL > 1
struct PROFILER_ENTRY
{
	uint32_t addr;
	uint32_t cycles;
} profiler_entry[2][1<<26];

static GpVar bb_profiler_entry;
#endif

#endif

// -----------------------------------------------------------------------------
//   Shifting macros
// -----------------------------------------------------------------------------
#define SET_NZCV(sign) \
{ \
	JIT_COMMENT("SET_NZCV"); \
	GpVar x = c.newGpVar(kVarTypeInt32); \
	GpVar y = c.newGpVar(kVarTypeInt32); \
	c.sets(x.r8Lo()); \
	c.setz(y.r8Lo()); \
	c.lea(x, x86::ptr(y.r64(), x.r64(), 1)); \
	if (sign) \
		c.setnc(y.r8Lo()); \
	else \
		c.setc(y.r8Lo()); \
	c.lea(x, x86::ptr(y.r64(), x.r64(), 1)); \
	c.seto(y.r8Lo()); \
	c.lea(x, x86::ptr(y.r64(), x.r64(), 1)); \
	c.movzx(y, flags_ptr); \
	c.shl(x, 4); \
	c.and_(y, 0xF); \
	c.or_(x, y); \
	c.mov(flags_ptr, x.r8Lo()); \
	c.unuse(x); \
	c.unuse(y); \
	JIT_COMMENT("end SET_NZCV"); \
}

#define SET_NZC \
{ \
	JIT_COMMENT("SET_NZC"); \
	GpVar x = c.newGpVar(kVarTypeInt32); \
	GpVar y = c.newGpVar(kVarTypeInt32); \
	c.sets(x.r8Lo()); \
	c.setz(y.r8Lo()); \
	c.lea(x, x86::ptr(y.r64(), x.r64(), 1)); \
	if (cf_change) \
	{ \
		c.lea(x, x86::ptr(rcf.r64(), x.r64(), 1)); \
		c.unuse(rcf); \
	} \
	c.movzx(y, flags_ptr); \
	c.shl(x, 6 - cf_change); \
	c.and_(y, cf_change?0x1F:0x3F); \
	c.or_(x, y); \
	c.mov(flags_ptr, x.r8Lo()); \
	JIT_COMMENT("end SET_NZC"); \
}

#define SET_NZC_SHIFTS_ZERO(cf) \
{ \
	JIT_COMMENT("SET_NZC_SHIFTS_ZERO"); \
	c.and_(flags_ptr, 0x1F); \
	if (cf) \
	{ \
		c.shl(rcf, 5); \
		c.or_(rcf, 1 << 6); \
		c.or_(flags_ptr, rcf.r8Lo()); \
	} \
	else \
		c.or_(flags_ptr, 1 << 6); \
	JIT_COMMENT("end SET_NZC_SHIFTS_ZERO"); \
}

#define SET_NZ(clear_cv) \
{ \
	JIT_COMMENT("SET_NZ"); \
	GpVar x = c.newGpVar(kVarTypeIntPtr); \
	GpVar y = c.newGpVar(kVarTypeIntPtr); \
	c.sets(x.r8Lo()); \
	c.setz(y.r8Lo()); \
	c.lea(x, x86::ptr(y.r64(), x.r64(), 1)); \
	c.movzx(y, flags_ptr); \
	c.and_(y, clear_cv?0x0F:0x3F); \
	c.shl(x, 6); \
	c.or_(x, y); \
	c.mov(flags_ptr, x.r8Lo()); \
	JIT_COMMENT("end SET_NZ"); \
}

#define SET_Q \
{ \
	JIT_COMMENT("SET_Q"); \
	GpVar x = c.newGpVar(kVarTypeIntPtr); \
	c.seto(x.r8Lo()); \
	c.shl(x, 3); \
	c.or_(flags_ptr, x.r8Lo()); \
	JIT_COMMENT("end SET_Q"); \
}

#define S_DST_R15 \
{ \
	JIT_COMMENT("S_DST_R15"); \
	GpVar SPSR = c.newGpVar(kVarTypeInt32); \
	GpVar tmp = c.newGpVar(kVarTypeInt32); \
	c.mov(SPSR, cpu_ptr(SPSR.val)); \
	c.mov(tmp, SPSR); \
	c.and_(tmp, 0x1F); \
	auto ctx = c.addCall(imm_ptr(armcpu_switchMode), ASMJIT_CALL_CONV, FuncBuilder2<void, void *, uint8_t>()); \
	ctx->setArg(0, bb_cpu); \
	ctx->setArg(1, tmp); \
	c.mov(cpu_ptr(CPSR.val), SPSR); \
	c.and_(SPSR, 1 << 5); \
	c.shr(SPSR, 5); \
	c.lea(tmp, x86::ptr_abs(0xFFFFFFFC, SPSR.r64(), 1)); \
	c.and_(tmp, reg_ptr(15)); \
	c.mov(cpu_ptr(next_instruction), tmp); \
	c.unuse(tmp); \
	JIT_COMMENT("end S_DST_R15"); \
}

// ============================================================================================= IMM
#define LSL_IMM \
	JIT_COMMENT("LSL_IMM"); \
	bool rhs_is_imm = false; \
	uint32_t imm = (i >> 7) & 0x1F; \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	c.mov(rhs, reg_pos_ptr(0)); \
	if (imm) \
		c.shl(rhs, imm); \
	uint32_t rhs_first = cpu->R[REG_POS(i, 0)] << imm;

#define S_LSL_IMM \
	JIT_COMMENT("S_LSL_IMM"); \
	bool rhs_is_imm = false; \
	uint8_t cf_change = 0; \
	GpVar rcf; \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	uint32_t imm = (i >> 7)&0x1F; \
	c.mov(rhs, reg_pos_ptr(0)); \
	if (imm)  \
	{ \
		cf_change = 1; \
		c.shl(rhs, imm); \
		rcf = c.newGpVar(kVarTypeInt32); \
		c.setc(rcf.r8Lo()); \
	}

#define LSR_IMM \
	JIT_COMMENT("LSR_IMM"); \
	bool rhs_is_imm = false; \
	uint32_t imm = (i >> 7) & 0x1F; \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	if (imm) \
	{ \
		c.mov(rhs, reg_pos_ptr(0)); \
		c.shr(rhs, imm); \
	} \
	else \
		c.mov(rhs, 0); \
	uint32_t rhs_first = imm ? cpu->R[REG_POS(i, 0)] >> imm : 0;

#define S_LSR_IMM \
	JIT_COMMENT("S_LSR_IMM"); \
	bool rhs_is_imm = false; \
	uint8_t cf_change = 1; \
	GpVar rcf = c.newGpVar(kVarTypeInt32); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	uint32_t imm = (i >> 7) & 0x1F; \
	c.mov(rhs, reg_pos_ptr(0)); \
	if (!imm) \
	{ \
		c.test(rhs, 1 << 31); \
		c.setnz(rcf.r8Lo()); \
		c.xor_(rhs, rhs); \
	} \
	else \
	{ \
		c.shr(rhs, imm); \
		c.setc(rcf.r8Lo()); \
	}

#define ASR_IMM \
	JIT_COMMENT("ASR_IMM"); \
	bool rhs_is_imm = false; \
	uint32_t imm = (i >> 7) & 0x1F; \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	c.mov(rhs, reg_pos_ptr(0)); \
	if (!imm) \
		imm = 31; \
	c.sar(rhs, imm); \
	uint32_t rhs_first = cpu->R[REG_POS(i, 0)] >> imm;

#define S_ASR_IMM \
	JIT_COMMENT("S_ASR_IMM"); \
	bool rhs_is_imm = false; \
	uint8_t cf_change = 1; \
	GpVar rcf = c.newGpVar(kVarTypeInt32); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	uint32_t imm = (i >> 7) & 0x1F; \
	c.mov(rhs, reg_pos_ptr(0)); \
	if (!imm) \
		imm = 31; \
	c.sar(rhs, imm); \
	imm == 31 ? c.sets(rcf.r8Lo()) : c.setc(rcf.r8Lo());

#define ROR_IMM \
	JIT_COMMENT("ROR_IMM"); \
	bool rhs_is_imm = false; \
	uint32_t imm = (i >> 7) & 0x1F; \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	c.mov(rhs, reg_pos_ptr(0)); \
	if (!imm) \
	{ \
		c.bt(flags_ptr, 5); \
		c.rcr(rhs, 1); \
	} \
	else \
		c.ror(rhs, imm); \
	uint32_t rhs_first = imm ? ROR(cpu->R[REG_POS(i, 0)], imm) : (cpu->CPSR.bits.C << 31) | (cpu->R[REG_POS(i, 0)] >> 1);

#define S_ROR_IMM \
	JIT_COMMENT("S_ROR_IMM"); \
	bool rhs_is_imm = false; \
	uint8_t cf_change = 1; \
	GpVar rcf = c.newGpVar(kVarTypeInt32); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	uint32_t imm = (i >> 7) & 0x1F; \
	c.mov(rhs, reg_pos_ptr(0)); \
	if (!imm) \
	{ \
		c.bt(flags_ptr, 5); \
		c.rcr(rhs, 1); \
	} \
	else \
		c.ror(rhs, imm); \
	c.setc(rcf.r8Lo());

#define REG_OFF \
	JIT_COMMENT("REG_OFF"); \
	bool rhs_is_imm = false; \
	Mem rhs = reg_pos_ptr(0); \
	uint32_t rhs_first = cpu->R[REG_POS(i, 0)];

#define IMM_VAL \
	JIT_COMMENT("IMM_VAL"); \
	bool rhs_is_imm = true; \
	uint32_t rhs = ROR(i & 0xFF, (i >> 7) & 0x1E); \
	uint32_t rhs_first = rhs;

#define S_IMM_VAL \
	JIT_COMMENT("S_IMM_VAL"); \
	bool rhs_is_imm = true; \
	uint8_t cf_change = 0; \
	GpVar rcf; \
	uint32_t rhs = ROR(i & 0xFF, (i >> 7) & 0x1E); \
	if ((i >> 8) & 0xF) \
	{ \
		cf_change = 1; \
		rcf = c.newGpVar(kVarTypeInt32); \
		c.mov(rcf, BIT31(rhs)); \
	} \
	uint32_t rhs_first = rhs;

#define IMM_OFF \
	JIT_COMMENT("IMM_OFF"); \
	bool rhs_is_imm = true; \
	uint32_t rhs = ((i >> 4) & 0xF0) + (i & 0xF); \
	uint32_t rhs_first = rhs;

#define IMM_OFF_12 \
	JIT_COMMENT("IMM_OFF_12"); \
	bool rhs_is_imm = true; \
	uint32_t rhs = i & 0xFFF; \
	uint32_t rhs_first = rhs;

// ============================================================================================= REG
#define LSX_REG(name, x86inst, sign) \
	JIT_COMMENT(#name); \
	bool rhs_is_imm = false; \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	GpVar imm = c.newGpVar(kVarTypeIntPtr); \
	GpVar tmp = c.newGpVar(kVarTypeIntPtr); \
	if (sign) \
		c.mov(tmp, 31); \
	else \
		c.mov(tmp, 0); \
	c.movzx(imm, reg_pos_ptrB(8)); \
	c.mov(rhs, reg_pos_ptr(0)); \
	c.cmp(imm, 31); \
	if (sign) \
		c.cmovg(imm, tmp); \
	else \
		c.cmovg(rhs, tmp); \
	c.x86inst(rhs, imm); \
	c.unuse(tmp);

#define S_LSX_REG(name, x86inst, sign) \
	JIT_COMMENT(#name); \
	bool rhs_is_imm = false; \
	uint8_t cf_change = 1; \
	GpVar rcf = c.newGpVar(kVarTypeInt32); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	GpVar imm = c.newGpVar(kVarTypeIntPtr); \
	Label __zero = c.newLabel(); \
	Label __lt32 = c.newLabel(); \
	Label __done = c.newLabel(); \
	c.mov(imm, reg_pos_ptr(8)); \
	c.mov(rhs, reg_pos_ptr(0)); \
	c.and_(imm, 0xFF); \
	c.jz(__zero); \
	c.cmp(imm, 32); \
	c.jl(__lt32); \
	if (!sign) \
	{ \
		Label __eq32 = c.newLabel(); \
		c.je(__eq32); \
		/* imm > 32 */ \
		c.mov(rhs, 0); \
		c.mov(rcf, 0); \
		c.jmp(__done); \
		/* imm == 32 */ \
		c.bind(__eq32); \
	} \
	c.x86inst(rhs, 31); \
	c.x86inst(rhs, 1); \
	c.setc(rcf.r8Lo()); \
	c.jmp(__done); \
	/* imm == 0 */ \
	c.bind(__zero); \
	c.test(flags_ptr, 1 << 5); \
	c.setnz(rcf.r8Lo()); \
	c.jmp(__done); \
	/* imm < 32 */ \
	c.bind(__lt32); \
	c.x86inst(rhs, imm); \
	c.setc(rcf.r8Lo()); \
	/* done */ \
	c.bind(__done);

#define LSL_REG LSX_REG(LSL_REG, shl, 0)
#define LSR_REG LSX_REG(LSR_REG, shr, 0)
#define ASR_REG LSX_REG(ASR_REG, sar, 1)
#define S_LSL_REG S_LSX_REG(S_LSL_REG, shl, 0)
#define S_LSR_REG S_LSX_REG(S_LSR_REG, shr, 0)
#define S_ASR_REG S_LSX_REG(S_ASR_REG, sar, 1)

#define ROR_REG \
	JIT_COMMENT("ROR_REG"); \
	bool rhs_is_imm = false; \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	GpVar imm = c.newGpVar(kVarTypeIntPtr); \
	c.mov(rhs, reg_pos_ptr(0)); \
	c.mov(imm, reg_pos_ptrB(8)); \
	c.ror(rhs, imm.r8Lo());

#define S_ROR_REG \
	JIT_COMMENT("S_ROR_REG"); \
	bool rhs_is_imm = false; \
	bool cf_change = 1; \
	GpVar rcf = c.newGpVar(kVarTypeInt32); \
	GpVar imm = c.newGpVar(kVarTypeIntPtr); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	Label __zero = c.newLabel(); \
	Label __zero_1F = c.newLabel(); \
	Label __done = c.newLabel(); \
	c.mov(imm, reg_pos_ptr(8)); \
	c.mov(rhs, reg_pos_ptr(0)); \
	c.and_(imm, 0xFF); \
	c.jz(__zero);\
	c.and_(imm, 0x1F); \
	c.jz(__zero_1F);\
	/* imm&0x1F != 0 */ \
	c.ror(rhs, imm); \
	c.setc(rcf.r8Lo()); \
	c.jmp(__done); \
	/* imm&0x1F == 0 */ \
	c.bind(__zero_1F); \
	c.test(rhs, 1 << 31); \
	c.setnz(rcf.r8Lo()); \
	c.jmp(__done); \
	/* imm == 0 */ \
	c.bind(__zero); \
	c.test(flags_ptr, 1 << 5); \
	c.setnz(rcf.r8Lo()); \
	/* done */ \
	c.bind(__done);

// ==================================================================== common funcs
static void emit_MMU_aluMemCycles(int alu_cycles, GpVar mem_cycles, int population)
{
	if (PROCNUM == ARMCPU_ARM9)
	{
		if (population < alu_cycles)
		{
			GpVar x = c.newGpVar(kVarTypeInt32);
			c.mov(x, alu_cycles);
			c.cmp(mem_cycles, alu_cycles);
			c.cmovl(mem_cycles, x);
		}
	}
	else
		c.add(mem_cycles, alu_cycles);
}

// -----------------------------------------------------------------------------
//   OPs
// -----------------------------------------------------------------------------
#define OP_ARITHMETIC(arg, x86inst, symmetric, flags) \
	arg; \
	GpVar lhs = c.newGpVar(kVarTypeInt32); \
	if (REG_POS(i, 12) == REG_POS(i, 16)) \
		c.x86inst(reg_pos_ptr(12), rhs); \
	else if (symmetric && !rhs_is_imm) \
	{ \
		c.x86inst(*reinterpret_cast<GpVar *>(&rhs), reg_pos_ptr(16)); \
		c.mov(reg_pos_ptr(12), rhs); \
	} \
	else \
	{ \
		c.mov(lhs, reg_pos_ptr(16)); \
		c.x86inst(lhs, rhs); \
		c.mov(reg_pos_ptr(12), lhs); \
	} \
	if (flags) \
	{ \
		if (REG_POS(i, 12) == 15) \
		{ \
			S_DST_R15; \
			bb_constant_cycles += 2; \
			return 1; \
		} \
		SET_NZCV(!symmetric); \
	} \
	else \
	{ \
		if (REG_POS(i, 12) == 15) \
		{ \
			GpVar tmp = c.newGpVar(kVarTypeInt32); \
			c.mov(tmp, reg_ptr(15)); \
			c.mov(cpu_ptr(next_instruction), tmp); \
			bb_constant_cycles += 2; \
		} \
	} \
	return 1;

#define OP_ARITHMETIC_R(arg, x86inst, flags) \
	arg; \
	GpVar lhs = c.newGpVar(kVarTypeInt32); \
	c.mov(lhs, rhs); \
	c.x86inst(lhs, reg_pos_ptr(16)); \
	c.mov(reg_pos_ptr(12), lhs); \
	if (flags) \
	{ \
		if (REG_POS(i, 12) == 15) \
		{ \
			S_DST_R15; \
			bb_constant_cycles += 2; \
			return 1; \
		} \
		SET_NZCV(1); \
	} \
	else \
	{ \
		if (REG_POS(i, 12) == 15) \
		{ \
			GpVar tmp = c.newGpVar(kVarTypeInt32); \
			c.mov(cpu_ptr(next_instruction), lhs); \
			bb_constant_cycles += 2; \
		} \
	} \
	return 1;

#define OP_ARITHMETIC_S(arg, x86inst, symmetric) \
	arg; \
	if (REG_POS(i, 12) == REG_POS(i, 16)) \
		c.x86inst(reg_pos_ptr(12), rhs); \
	else if (symmetric && !rhs_is_imm) \
	{ \
		c.x86inst(*reinterpret_cast<GpVar *>(&rhs), reg_pos_ptr(16)); \
		c.mov(reg_pos_ptr(12), rhs); \
	} \
	else \
	{ \
		GpVar lhs = c.newGpVar(kVarTypeInt32); \
		c.mov(lhs, reg_pos_ptr(16)); \
		c.x86inst(lhs, rhs); \
		c.mov(reg_pos_ptr(12), lhs); \
	} \
	if (REG_POS(i, 12) == 15) \
	{ \
		S_DST_R15; \
		bb_constant_cycles += 2; \
		return 1; \
	} \
	SET_NZC; \
	return 1;

#define GET_CARRY(invert) \
{ \
	c.bt(flags_ptr, 5); \
	if (invert) \
		c.cmc(); \
}

static int OP_AND_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM, and_, 1, 0); }
static int OP_AND_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG, and_, 1, 0); }
static int OP_AND_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM, and_, 1, 0); }
static int OP_AND_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG, and_, 1, 0); }
static int OP_AND_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM, and_, 1, 0); }
static int OP_AND_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG, and_, 1, 0); }
static int OP_AND_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM, and_, 1, 0); }
static int OP_AND_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG, and_, 1, 0); }
static int OP_AND_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL, and_, 1, 0); }

static int OP_EOR_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM, xor_, 1, 0); }
static int OP_EOR_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG, xor_, 1, 0); }
static int OP_EOR_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM, xor_, 1, 0); }
static int OP_EOR_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG, xor_, 1, 0); }
static int OP_EOR_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM, xor_, 1, 0); }
static int OP_EOR_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG, xor_, 1, 0); }
static int OP_EOR_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM, xor_, 1, 0); }
static int OP_EOR_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG, xor_, 1, 0); }
static int OP_EOR_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL, xor_, 1, 0); }

static int OP_ORR_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM, or_, 1, 0); }
static int OP_ORR_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG, or_, 1, 0); }
static int OP_ORR_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM, or_, 1, 0); }
static int OP_ORR_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG, or_, 1, 0); }
static int OP_ORR_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM, or_, 1, 0); }
static int OP_ORR_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG, or_, 1, 0); }
static int OP_ORR_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM, or_, 1, 0); }
static int OP_ORR_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG, or_, 1, 0); }
static int OP_ORR_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL, or_, 1, 0); }

static int OP_ADD_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM, add, 1, 0); }
static int OP_ADD_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG, add, 1, 0); }
static int OP_ADD_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM, add, 1, 0); }
static int OP_ADD_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG, add, 1, 0); }
static int OP_ADD_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM, add, 1, 0); }
static int OP_ADD_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG, add, 1, 0); }
static int OP_ADD_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM, add, 1, 0); }
static int OP_ADD_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG, add, 1, 0); }
static int OP_ADD_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL, add, 1, 0); }

static int OP_SUB_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM, sub, 0, 0); }
static int OP_SUB_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG, sub, 0, 0); }
static int OP_SUB_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM, sub, 0, 0); }
static int OP_SUB_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG, sub, 0, 0); }
static int OP_SUB_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM, sub, 0, 0); }
static int OP_SUB_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG, sub, 0, 0); }
static int OP_SUB_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM, sub, 0, 0); }
static int OP_SUB_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG, sub, 0, 0); }
static int OP_SUB_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL, sub, 0, 0); }

static int OP_RSB_LSL_IMM(uint32_t i) { OP_ARITHMETIC_R(LSL_IMM, sub, 0); }
static int OP_RSB_LSL_REG(uint32_t i) { OP_ARITHMETIC_R(LSL_REG, sub, 0); }
static int OP_RSB_LSR_IMM(uint32_t i) { OP_ARITHMETIC_R(LSR_IMM, sub, 0); }
static int OP_RSB_LSR_REG(uint32_t i) { OP_ARITHMETIC_R(LSR_REG, sub, 0); }
static int OP_RSB_ASR_IMM(uint32_t i) { OP_ARITHMETIC_R(ASR_IMM, sub, 0); }
static int OP_RSB_ASR_REG(uint32_t i) { OP_ARITHMETIC_R(ASR_REG, sub, 0); }
static int OP_RSB_ROR_IMM(uint32_t i) { OP_ARITHMETIC_R(ROR_IMM, sub, 0); }
static int OP_RSB_ROR_REG(uint32_t i) { OP_ARITHMETIC_R(ROR_REG, sub, 0); }
static int OP_RSB_IMM_VAL(uint32_t i) { OP_ARITHMETIC_R(IMM_VAL, sub, 0); }

// ================================ S instructions
static int OP_AND_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSL_IMM, and_, 1); }
static int OP_AND_S_LSL_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSL_REG, and_, 1); }
static int OP_AND_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSR_IMM, and_, 1); }
static int OP_AND_S_LSR_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSR_REG, and_, 1); }
static int OP_AND_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ASR_IMM, and_, 1); }
static int OP_AND_S_ASR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ASR_REG, and_, 1); }
static int OP_AND_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ROR_IMM, and_, 1); }
static int OP_AND_S_ROR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ROR_REG, and_, 1); }
static int OP_AND_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC_S(S_IMM_VAL, and_, 1); }

static int OP_EOR_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSL_IMM, xor_, 1); }
static int OP_EOR_S_LSL_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSL_REG, xor_, 1); }
static int OP_EOR_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSR_IMM, xor_, 1); }
static int OP_EOR_S_LSR_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSR_REG, xor_, 1); }
static int OP_EOR_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ASR_IMM, xor_, 1); }
static int OP_EOR_S_ASR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ASR_REG, xor_, 1); }
static int OP_EOR_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ROR_IMM, xor_, 1); }
static int OP_EOR_S_ROR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ROR_REG, xor_, 1); }
static int OP_EOR_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC_S(S_IMM_VAL, xor_, 1); }

static int OP_ORR_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSL_IMM, or_, 1); }
static int OP_ORR_S_LSL_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSL_REG, or_, 1); }
static int OP_ORR_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSR_IMM, or_, 1); }
static int OP_ORR_S_LSR_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSR_REG, or_, 1); }
static int OP_ORR_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ASR_IMM, or_, 1); }
static int OP_ORR_S_ASR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ASR_REG, or_, 1); }
static int OP_ORR_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ROR_IMM, or_, 1); }
static int OP_ORR_S_ROR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ROR_REG, or_, 1); }
static int OP_ORR_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC_S(S_IMM_VAL, or_, 1); }

static int OP_ADD_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM, add, 1, 1); }
static int OP_ADD_S_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG, add, 1, 1); }
static int OP_ADD_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM, add, 1, 1); }
static int OP_ADD_S_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG, add, 1, 1); }
static int OP_ADD_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM, add, 1, 1); }
static int OP_ADD_S_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG, add, 1, 1); }
static int OP_ADD_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM, add, 1, 1); }
static int OP_ADD_S_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG, add, 1, 1); }
static int OP_ADD_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL, add, 1, 1); }

static int OP_SUB_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM, sub, 0, 1); }
static int OP_SUB_S_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG, sub, 0, 1); }
static int OP_SUB_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM, sub, 0, 1); }
static int OP_SUB_S_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG, sub, 0, 1); }
static int OP_SUB_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM, sub, 0, 1); }
static int OP_SUB_S_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG, sub, 0, 1); }
static int OP_SUB_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM, sub, 0, 1); }
static int OP_SUB_S_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG, sub, 0, 1); }
static int OP_SUB_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL, sub, 0, 1); }

static int OP_RSB_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC_R(LSL_IMM, sub, 1); }
static int OP_RSB_S_LSL_REG(uint32_t i) { OP_ARITHMETIC_R(LSL_REG, sub, 1); }
static int OP_RSB_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC_R(LSR_IMM, sub, 1); }
static int OP_RSB_S_LSR_REG(uint32_t i) { OP_ARITHMETIC_R(LSR_REG, sub, 1); }
static int OP_RSB_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC_R(ASR_IMM, sub, 1); }
static int OP_RSB_S_ASR_REG(uint32_t i) { OP_ARITHMETIC_R(ASR_REG, sub, 1); }
static int OP_RSB_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC_R(ROR_IMM, sub, 1); }
static int OP_RSB_S_ROR_REG(uint32_t i) { OP_ARITHMETIC_R(ROR_REG, sub, 1); }
static int OP_RSB_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC_R(IMM_VAL, sub, 1); }

static int OP_ADC_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG; GET_CARRY(0), adc, 1, 0); }
static int OP_ADC_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL; GET_CARRY(0), adc, 1, 0); }

static int OP_ADC_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG; GET_CARRY(0), adc, 1, 1); }
static int OP_ADC_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL; GET_CARRY(0), adc, 1, 1); }

static int OP_SBC_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG; GET_CARRY(1), sbb, 0, 0); }
static int OP_SBC_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL; GET_CARRY(1), sbb, 0, 0); }

static int OP_SBC_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG; GET_CARRY(1), sbb, 0, 1); }
static int OP_SBC_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL; GET_CARRY(1), sbb, 0, 1); }

static int OP_RSC_LSL_IMM(uint32_t i) { OP_ARITHMETIC_R(LSL_IMM; GET_CARRY(1), sbb, 0); }
static int OP_RSC_LSL_REG(uint32_t i) { OP_ARITHMETIC_R(LSL_REG; GET_CARRY(1), sbb, 0); }
static int OP_RSC_LSR_IMM(uint32_t i) { OP_ARITHMETIC_R(LSR_IMM; GET_CARRY(1), sbb, 0); }
static int OP_RSC_LSR_REG(uint32_t i) { OP_ARITHMETIC_R(LSR_REG; GET_CARRY(1), sbb, 0); }
static int OP_RSC_ASR_IMM(uint32_t i) { OP_ARITHMETIC_R(ASR_IMM; GET_CARRY(1), sbb, 0); }
static int OP_RSC_ASR_REG(uint32_t i) { OP_ARITHMETIC_R(ASR_REG; GET_CARRY(1), sbb, 0); }
static int OP_RSC_ROR_IMM(uint32_t i) { OP_ARITHMETIC_R(ROR_IMM; GET_CARRY(1), sbb, 0); }
static int OP_RSC_ROR_REG(uint32_t i) { OP_ARITHMETIC_R(ROR_REG; GET_CARRY(1), sbb, 0); }
static int OP_RSC_IMM_VAL(uint32_t i) { OP_ARITHMETIC_R(IMM_VAL; GET_CARRY(1), sbb, 0); }

static int OP_RSC_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC_R(LSL_IMM; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_LSL_REG(uint32_t i) { OP_ARITHMETIC_R(LSL_REG; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC_R(LSR_IMM; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_LSR_REG(uint32_t i) { OP_ARITHMETIC_R(LSR_REG; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC_R(ASR_IMM; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_ASR_REG(uint32_t i) { OP_ARITHMETIC_R(ASR_REG; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC_R(ROR_IMM; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_ROR_REG(uint32_t i) { OP_ARITHMETIC_R(ROR_REG; GET_CARRY(1), sbb, 1); }
static int OP_RSC_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC_R(IMM_VAL; GET_CARRY(1), sbb, 1); }

static int OP_BIC_LSL_IMM(uint32_t i) { OP_ARITHMETIC(LSL_IMM; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_LSL_REG(uint32_t i) { OP_ARITHMETIC(LSL_REG; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_LSR_IMM(uint32_t i) { OP_ARITHMETIC(LSR_IMM; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_LSR_REG(uint32_t i) { OP_ARITHMETIC(LSR_REG; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_ASR_IMM(uint32_t i) { OP_ARITHMETIC(ASR_IMM; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_ASR_REG(uint32_t i) { OP_ARITHMETIC(ASR_REG; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_ROR_IMM(uint32_t i) { OP_ARITHMETIC(ROR_IMM; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_ROR_REG(uint32_t i) { OP_ARITHMETIC(ROR_REG; c.not_(rhs), and_, 1, 0); }
static int OP_BIC_IMM_VAL(uint32_t i) { OP_ARITHMETIC(IMM_VAL; rhs = ~rhs,  and_, 1, 0); }

static int OP_BIC_S_LSL_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSL_IMM; c.not_(rhs), and_, 1); }
static int OP_BIC_S_LSL_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSL_REG; c.not_(rhs), and_, 1); }
static int OP_BIC_S_LSR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_LSR_IMM; c.not_(rhs), and_, 1); }
static int OP_BIC_S_LSR_REG(uint32_t i) { OP_ARITHMETIC_S(S_LSR_REG; c.not_(rhs), and_, 1); }
static int OP_BIC_S_ASR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ASR_IMM; c.not_(rhs), and_, 1); }
static int OP_BIC_S_ASR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ASR_REG; c.not_(rhs), and_, 1); }
static int OP_BIC_S_ROR_IMM(uint32_t i) { OP_ARITHMETIC_S(S_ROR_IMM; c.not_(rhs), and_, 1); }
static int OP_BIC_S_ROR_REG(uint32_t i) { OP_ARITHMETIC_S(S_ROR_REG; c.not_(rhs), and_, 1); }
static int OP_BIC_S_IMM_VAL(uint32_t i) { OP_ARITHMETIC_S(S_IMM_VAL; rhs = ~rhs,  and_, 1); }

// -----------------------------------------------------------------------------
//   TST
// -----------------------------------------------------------------------------
#define OP_TST_(arg) \
	arg; \
	c.test(reg_pos_ptr(16), rhs); \
	SET_NZC; \
	return 1;

static int OP_TST_LSL_IMM(uint32_t i) { OP_TST_(S_LSL_IMM); }
static int OP_TST_LSL_REG(uint32_t i) { OP_TST_(S_LSL_REG); }
static int OP_TST_LSR_IMM(uint32_t i) { OP_TST_(S_LSR_IMM); }
static int OP_TST_LSR_REG(uint32_t i) { OP_TST_(S_LSR_REG); }
static int OP_TST_ASR_IMM(uint32_t i) { OP_TST_(S_ASR_IMM); }
static int OP_TST_ASR_REG(uint32_t i) { OP_TST_(S_ASR_REG); }
static int OP_TST_ROR_IMM(uint32_t i) { OP_TST_(S_ROR_IMM); }
static int OP_TST_ROR_REG(uint32_t i) { OP_TST_(S_ROR_REG); }
static int OP_TST_IMM_VAL(uint32_t i) { OP_TST_(S_IMM_VAL); }

// -----------------------------------------------------------------------------
//   TEQ
// -----------------------------------------------------------------------------
#define OP_TEQ_(arg) \
	arg; \
	if (!rhs_is_imm) \
		c.xor_(*reinterpret_cast<GpVar *>(&rhs), reg_pos_ptr(16)); \
	else \
	{ \
		GpVar x = c.newGpVar(kVarTypeInt32); \
		c.mov(x, rhs); \
		c.xor_(x, reg_pos_ptr(16)); \
	} \
	SET_NZC; \
	return 1;

static int OP_TEQ_LSL_IMM(uint32_t i) { OP_TEQ_(S_LSL_IMM); }
static int OP_TEQ_LSL_REG(uint32_t i) { OP_TEQ_(S_LSL_REG); }
static int OP_TEQ_LSR_IMM(uint32_t i) { OP_TEQ_(S_LSR_IMM); }
static int OP_TEQ_LSR_REG(uint32_t i) { OP_TEQ_(S_LSR_REG); }
static int OP_TEQ_ASR_IMM(uint32_t i) { OP_TEQ_(S_ASR_IMM); }
static int OP_TEQ_ASR_REG(uint32_t i) { OP_TEQ_(S_ASR_REG); }
static int OP_TEQ_ROR_IMM(uint32_t i) { OP_TEQ_(S_ROR_IMM); }
static int OP_TEQ_ROR_REG(uint32_t i) { OP_TEQ_(S_ROR_REG); }
static int OP_TEQ_IMM_VAL(uint32_t i) { OP_TEQ_(S_IMM_VAL); }

// -----------------------------------------------------------------------------
//   CMP
// -----------------------------------------------------------------------------
#define OP_CMP(arg) \
	arg; \
	c.cmp(reg_pos_ptr(16), rhs); \
	SET_NZCV(1); \
	return 1;

static int OP_CMP_LSL_IMM(uint32_t i) { OP_CMP(LSL_IMM); }
static int OP_CMP_LSL_REG(uint32_t i) { OP_CMP(LSL_REG); }
static int OP_CMP_LSR_IMM(uint32_t i) { OP_CMP(LSR_IMM); }
static int OP_CMP_LSR_REG(uint32_t i) { OP_CMP(LSR_REG); }
static int OP_CMP_ASR_IMM(uint32_t i) { OP_CMP(ASR_IMM); }
static int OP_CMP_ASR_REG(uint32_t i) { OP_CMP(ASR_REG); }
static int OP_CMP_ROR_IMM(uint32_t i) { OP_CMP(ROR_IMM); }
static int OP_CMP_ROR_REG(uint32_t i) { OP_CMP(ROR_REG); }
static int OP_CMP_IMM_VAL(uint32_t i) { OP_CMP(IMM_VAL); }

#undef OP_CMP

// -----------------------------------------------------------------------------
//   CMN
// -----------------------------------------------------------------------------
#define OP_CMN(arg) \
	arg; \
	uint32_t rhs_imm = *reinterpret_cast<uint32_t *>(&rhs); \
	int sign = rhs_is_imm && (rhs_imm != -rhs_imm); \
	if (sign) \
		c.cmp(reg_pos_ptr(16), -rhs_imm); \
	else \
	{ \
		GpVar lhs = c.newGpVar(kVarTypeInt32); \
		c.mov(lhs, reg_pos_ptr(16)); \
		c.add(lhs, rhs); \
	} \
	SET_NZCV(sign); \
	return 1;

static int OP_CMN_LSL_IMM(uint32_t i) { OP_CMN(LSL_IMM); }
static int OP_CMN_LSL_REG(uint32_t i) { OP_CMN(LSL_REG); }
static int OP_CMN_LSR_IMM(uint32_t i) { OP_CMN(LSR_IMM); }
static int OP_CMN_LSR_REG(uint32_t i) { OP_CMN(LSR_REG); }
static int OP_CMN_ASR_IMM(uint32_t i) { OP_CMN(ASR_IMM); }
static int OP_CMN_ASR_REG(uint32_t i) { OP_CMN(ASR_REG); }
static int OP_CMN_ROR_IMM(uint32_t i) { OP_CMN(ROR_IMM); }
static int OP_CMN_ROR_REG(uint32_t i) { OP_CMN(ROR_REG); }
static int OP_CMN_IMM_VAL(uint32_t i) { OP_CMN(IMM_VAL); }

#undef OP_CMN

// -----------------------------------------------------------------------------
//   MOV
// -----------------------------------------------------------------------------
#define OP_MOV(arg) \
	arg; \
	c.mov(reg_pos_ptr(12), rhs); \
	if (REG_POS(i, 12) == 15) \
	{ \
		c.mov(cpu_ptr(next_instruction), rhs); \
		return 1; \
	} \
	return 1;

static int OP_MOV_LSL_IMM(uint32_t i) { if (i == 0xE1A00000) { /* nop */ JIT_COMMENT("nop"); return 1; } OP_MOV(LSL_IMM); }
static int OP_MOV_LSL_REG(uint32_t i) { OP_MOV(LSL_REG; if (REG_POS(i, 0) == 15) c.add(rhs, 4);); }
static int OP_MOV_LSR_IMM(uint32_t i) { OP_MOV(LSR_IMM); }
static int OP_MOV_LSR_REG(uint32_t i) { OP_MOV(LSR_REG; if (REG_POS(i, 0) == 15) c.add(rhs, 4);); }
static int OP_MOV_ASR_IMM(uint32_t i) { OP_MOV(ASR_IMM); }
static int OP_MOV_ASR_REG(uint32_t i) { OP_MOV(ASR_REG); }
static int OP_MOV_ROR_IMM(uint32_t i) { OP_MOV(ROR_IMM); }
static int OP_MOV_ROR_REG(uint32_t i) { OP_MOV(ROR_REG); }
static int OP_MOV_IMM_VAL(uint32_t i) { OP_MOV(IMM_VAL); }

#define OP_MOV_S(arg) \
	arg; \
	c.mov(reg_pos_ptr(12), rhs); \
	if (REG_POS(i, 12) == 15) \
	{ \
		S_DST_R15; \
		bb_constant_cycles += 2; \
		return 1; \
	} \
	if (!rhs_is_imm) \
		c.cmp(*reinterpret_cast<GpVar *>(&rhs), 0); \
	else \
		c.cmp(reg_pos_ptr(12), 0); \
	SET_NZC; \
	return 1;

static int OP_MOV_S_LSL_IMM(uint32_t i) { OP_MOV_S(S_LSL_IMM); }
static int OP_MOV_S_LSL_REG(uint32_t i) { OP_MOV_S(S_LSL_REG; if (REG_POS(i, 0) == 15) c.add(rhs, 4);); }
static int OP_MOV_S_LSR_IMM(uint32_t i) { OP_MOV_S(S_LSR_IMM); }
static int OP_MOV_S_LSR_REG(uint32_t i) { OP_MOV_S(S_LSR_REG; if (REG_POS(i, 0) == 15) c.add(rhs, 4);); }
static int OP_MOV_S_ASR_IMM(uint32_t i) { OP_MOV_S(S_ASR_IMM); }
static int OP_MOV_S_ASR_REG(uint32_t i) { OP_MOV_S(S_ASR_REG); }
static int OP_MOV_S_ROR_IMM(uint32_t i) { OP_MOV_S(S_ROR_IMM); }
static int OP_MOV_S_ROR_REG(uint32_t i) { OP_MOV_S(S_ROR_REG); }
static int OP_MOV_S_IMM_VAL(uint32_t i) { OP_MOV_S(S_IMM_VAL); }

// -----------------------------------------------------------------------------
//   MVN
// -----------------------------------------------------------------------------
static int OP_MVN_LSL_IMM(uint32_t i) { OP_MOV(LSL_IMM; c.not_(rhs)); }
static int OP_MVN_LSL_REG(uint32_t i) { OP_MOV(LSL_REG; c.not_(rhs)); }
static int OP_MVN_LSR_IMM(uint32_t i) { OP_MOV(LSR_IMM; c.not_(rhs)); }
static int OP_MVN_LSR_REG(uint32_t i) { OP_MOV(LSR_REG; c.not_(rhs)); }
static int OP_MVN_ASR_IMM(uint32_t i) { OP_MOV(ASR_IMM; c.not_(rhs)); }
static int OP_MVN_ASR_REG(uint32_t i) { OP_MOV(ASR_REG; c.not_(rhs)); }
static int OP_MVN_ROR_IMM(uint32_t i) { OP_MOV(ROR_IMM; c.not_(rhs)); }
static int OP_MVN_ROR_REG(uint32_t i) { OP_MOV(ROR_REG; c.not_(rhs)); }
static int OP_MVN_IMM_VAL(uint32_t i) { OP_MOV(IMM_VAL; rhs = ~rhs); }

static int OP_MVN_S_LSL_IMM(uint32_t i) { OP_MOV_S(S_LSL_IMM; c.not_(rhs)); }
static int OP_MVN_S_LSL_REG(uint32_t i) { OP_MOV_S(S_LSL_REG; c.not_(rhs)); }
static int OP_MVN_S_LSR_IMM(uint32_t i) { OP_MOV_S(S_LSR_IMM; c.not_(rhs)); }
static int OP_MVN_S_LSR_REG(uint32_t i) { OP_MOV_S(S_LSR_REG; c.not_(rhs)); }
static int OP_MVN_S_ASR_IMM(uint32_t i) { OP_MOV_S(S_ASR_IMM; c.not_(rhs)); }
static int OP_MVN_S_ASR_REG(uint32_t i) { OP_MOV_S(S_ASR_REG; c.not_(rhs)); }
static int OP_MVN_S_ROR_IMM(uint32_t i) { OP_MOV_S(S_ROR_IMM; c.not_(rhs)); }
static int OP_MVN_S_ROR_REG(uint32_t i) { OP_MOV_S(S_ROR_REG; c.not_(rhs)); }
static int OP_MVN_S_IMM_VAL(uint32_t i) { OP_MOV_S(S_IMM_VAL; rhs = ~rhs); }

//- ----------------------------------------------------------------------------
//   QADD / QDADD / QSUB / QDSUB
// -----------------------------------------------------------------------------
// TODO
static int OP_QADD(uint32_t i) { printf("JIT: unimplemented OP_QADD\n"); return 0; }
static int OP_QSUB(uint32_t i) { printf("JIT: unimplemented OP_QSUB\n"); return 0; }
static int OP_QDADD(uint32_t i) { printf("JIT: unimplemented OP_QDADD\n"); return 0; }
static int OP_QDSUB(uint32_t i) { printf("JIT: unimplemented OP_QDSUB\n"); return 0; }

// -----------------------------------------------------------------------------
//   MUL
// -----------------------------------------------------------------------------
static void MUL_Mxx_END(GpVar x, bool sign, int cycles)
{
	if (sign)
	{
		GpVar y = c.newGpVar(kVarTypeInt32);
		c.mov(y, x);
		c.sar(x, 31);
		c.xor_(x, y);
	}
	c.or_(x, 1);
	c.bsr(bb_cycles, x);
	c.shr(bb_cycles, 3);
	c.add(bb_cycles, cycles + 1);
}

#define OP_MUL_(op, width, sign, accum, flags) \
	GpVar lhs = c.newGpVar(kVarTypeInt32); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	GpVar hi; \
	if (width) \
	{ \
		hi = c.newGpVar(kVarTypeInt32); \
		c.xor_(hi, hi); \
	} \
	c.mov(lhs, reg_pos_ptr(0)); \
	c.mov(rhs, reg_pos_ptr(8)); \
	op; \
	if (width && accum) \
	{ \
		if (flags) \
		{ \
			c.add(lhs, reg_pos_ptr(12)); \
			c.adc(hi, reg_pos_ptr(16)); \
			c.mov(reg_pos_ptr(12), lhs); \
			c.mov(reg_pos_ptr(16), hi); \
			c.cmp(hi, lhs); \
			SET_NZ(0); \
		} \
		else \
		{ \
			c.add(reg_pos_ptr(12), lhs); \
			c.adc(reg_pos_ptr(16), hi); \
		} \
	} \
	else if (width) \
	{ \
		c.mov(reg_pos_ptr(12), lhs); \
		c.mov(reg_pos_ptr(16), hi); \
		if (flags) \
		{ \
			c.cmp(hi, lhs); \
			SET_NZ(0); \
		} \
	} \
	else \
	{ \
		if (accum) \
			c.add(lhs, reg_pos_ptr(12)); \
		c.mov(reg_pos_ptr(16), lhs); \
		if (flags) \
		{ \
			c.cmp(lhs, 0); \
			SET_NZ(0); \
		} \
	} \
	MUL_Mxx_END(rhs, sign, 1 + width + accum); \
	return 1;

static int OP_MUL(uint32_t i) { OP_MUL_(c.imul(lhs,rhs), 0, 1, 0, 0); }
static int OP_MLA(uint32_t i) { OP_MUL_(c.imul(lhs,rhs), 0, 1, 1, 0); }
static int OP_UMULL(uint32_t i) { OP_MUL_(c.mul(hi,lhs,rhs), 1, 0, 0, 0); }
static int OP_UMLAL(uint32_t i) { OP_MUL_(c.mul(hi,lhs,rhs), 1, 0, 1, 0); }
static int OP_SMULL(uint32_t i) { OP_MUL_(c.imul(hi,lhs,rhs), 1, 1, 0, 0); }
static int OP_SMLAL(uint32_t i) { OP_MUL_(c.imul(hi,lhs,rhs), 1, 1, 1, 0); }

static int OP_MUL_S(uint32_t i) { OP_MUL_(c.imul(lhs,rhs), 0, 1, 0, 1); }
static int OP_MLA_S(uint32_t i) { OP_MUL_(c.imul(lhs,rhs), 0, 1, 1, 1); }
static int OP_UMULL_S(uint32_t i) { OP_MUL_(c.mul(hi,lhs,rhs), 1, 0, 0, 1); }
static int OP_UMLAL_S(uint32_t i) { OP_MUL_(c.mul(hi,lhs,rhs), 1, 0, 1, 1); }
static int OP_SMULL_S(uint32_t i) { OP_MUL_(c.imul(hi,lhs,rhs), 1, 1, 0, 1); }
static int OP_SMLAL_S(uint32_t i) { OP_MUL_(c.imul(hi,lhs,rhs), 1, 1, 1, 1); }

#define OP_MULxy_(op, x, y, width, accum, flags) \
	GpVar lhs = c.newGpVar(kVarTypeInt32); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	GpVar hi; \
	c.movsx(lhs, reg_pos_ptr##x(0)); \
	c.movsx(rhs, reg_pos_ptr##y(8)); \
	if (width) \
		hi = c.newGpVar(kVarTypeInt32); \
	op; \
	if (width && accum) \
	{ \
		if (flags) \
		{ \
			c.add(lhs, reg_pos_ptr(12)); \
			c.adc(hi, reg_pos_ptr(16)); \
			c.mov(reg_pos_ptr(12), lhs); \
			c.mov(reg_pos_ptr(16), hi); \
			SET_Q; \
		} \
		else \
		{ \
			c.add(reg_pos_ptr(12), lhs); \
			c.adc(reg_pos_ptr(16), hi); \
		} \
	} \
	else if (width) \
	{ \
		c.mov(reg_pos_ptr(12), lhs); \
		c.mov(reg_pos_ptr(16), hi); \
		if (flags) \
			SET_Q; \
	} \
	else \
	{ \
		if (accum) \
			c.add(lhs, reg_pos_ptr(12));  \
		c.mov(reg_pos_ptr(16), lhs); \
		if (flags) \
			SET_Q; \
	} \
	return 1;


// -----------------------------------------------------------------------------
//   SMUL
// -----------------------------------------------------------------------------
static int OP_SMUL_B_B(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), L, L, 0, 0, 0); }
static int OP_SMUL_B_T(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), L, H, 0, 0, 0); }
static int OP_SMUL_T_B(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), H, L, 0, 0, 0); }
static int OP_SMUL_T_T(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), H, H, 0, 0, 0); }

// -----------------------------------------------------------------------------
//   SMLA
// -----------------------------------------------------------------------------
static int OP_SMLA_B_B(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), L, L, 0, 1, 1); }
static int OP_SMLA_B_T(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), L, H, 0, 1, 1); }
static int OP_SMLA_T_B(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), H, L, 0, 1, 1); }
static int OP_SMLA_T_T(uint32_t i) { OP_MULxy_(c.imul(lhs, rhs), H, H, 0, 1, 1); }

// -----------------------------------------------------------------------------
//   SMLAL
// -----------------------------------------------------------------------------
static int OP_SMLAL_B_B(uint32_t i) { OP_MULxy_(c.imul(hi,lhs,rhs), L, L, 1, 1, 1); }
static int OP_SMLAL_B_T(uint32_t i) { OP_MULxy_(c.imul(hi,lhs,rhs), L, H, 1, 1, 1); }
static int OP_SMLAL_T_B(uint32_t i) { OP_MULxy_(c.imul(hi,lhs,rhs), H, L, 1, 1, 1); }
static int OP_SMLAL_T_T(uint32_t i) { OP_MULxy_(c.imul(hi,lhs,rhs), H, H, 1, 1, 1); }

// -----------------------------------------------------------------------------
//   SMULW / SMLAW
// -----------------------------------------------------------------------------
#ifdef ASMJIT_X64
#define OP_SMxxW_(x, accum, flags) \
	GpVar lhs = c.newGpVar(kVarTypeIntPtr); \
	GpVar rhs = c.newGpVar(kVarTypeIntPtr); \
	c.movsx(lhs, reg_pos_ptr##x(8)); \
	c.movsxd(rhs, reg_pos_ptr(0)); \
	c.imul(lhs, rhs);  \
	c.sar(lhs, 16); \
	if (accum) \
		c.add(lhs, reg_pos_ptr(12)); \
	c.mov(reg_pos_ptr(16), lhs.r32()); \
	if (flags) \
		SET_Q; \
	return 1;
#else
#define OP_SMxxW_(x, accum, flags) \
	GpVar lhs = c.newGpVar(kVarTypeInt32); \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	GpVar hi = c.newGpVar(kVarTypeInt32); \
	c.movsx(lhs, reg_pos_ptr##x(8)); \
	c.mov(rhs, reg_pos_ptr(0)); \
	c.imul(hi, lhs, rhs);  \
	c.shr(lhs, 16); \
	c.shl(hi, 16); \
	c.or_(lhs, hi); \
	if (accum) \
		c.add(lhs, reg_pos_ptr(12)); \
	c.mov(reg_pos_ptr(16), lhs); \
	if (flags) \
		SET_Q; \
	return 1;
#endif

static int OP_SMULW_B(uint32_t i) { OP_SMxxW_(L, 0, 0); }
static int OP_SMULW_T(uint32_t i) { OP_SMxxW_(H, 0, 0); }
static int OP_SMLAW_B(uint32_t i) { OP_SMxxW_(L, 1, 1); }
static int OP_SMLAW_T(uint32_t i) { OP_SMxxW_(H, 1, 1); }

// -----------------------------------------------------------------------------
//   MRS / MSR
// -----------------------------------------------------------------------------
static int OP_MRS_CPSR(uint32_t i)
{
	GpVar x = c.newGpVar(kVarTypeInt32);
	c.mov(x, cpu_ptr(CPSR));
	c.mov(reg_pos_ptr(12), x);
	return 1;
}

static int OP_MRS_SPSR(uint32_t i)
{
	GpVar x = c.newGpVar(kVarTypeInt32);
	c.mov(x, cpu_ptr(SPSR));
	c.mov(reg_pos_ptr(12), x);
	return 1;
}

// TODO: SPSR: if(cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS) return 1;
#define OP_MSR_(reg, args, sw) \
	GpVar operand = c.newGpVar(kVarTypeInt32); \
	args; \
	c.mov(operand, rhs); \
	switch ((i >> 16) & 0xF) \
	{ \
		case 0x1: /* bit 16 */ \
		{ \
			GpVar mode = c.newGpVar(kVarTypeInt32); \
			Label __skip = c.newLabel(); \
			c.mov(mode, cpu_ptr(CPSR)); \
			c.and_(mode, 0x1F); \
			c.cmp(mode, USR); \
			c.je(__skip); \
			if (sw) \
			{ \
				c.mov(mode, rhs); \
				c.and_(mode, 0x1F); \
				auto ctx = c.addCall(imm_ptr(armcpu_switchMode), ASMJIT_CALL_CONV, \
					FuncBuilder2<void, void *, uint8_t>()); \
				ctx->setArg(0, bb_cpu); \
				ctx->setArg(1, mode); \
			} \
			Mem xPSR_memB = cpu_ptr_byte(reg, 0); \
			c.mov(xPSR_memB, operand.r8Lo()); \
			changeCPSR; \
			c.bind(__skip); \
			return 1; \
		} \
		case 0x2: /* bit 17 */ \
		{ \
			GpVar mode = c.newGpVar(kVarTypeInt32); \
			Label __skip = c.newLabel(); \
			c.mov(mode, cpu_ptr(CPSR)); \
			c.and_(mode, 0x1F); \
			c.cmp(mode, USR); \
			c.je(__skip); \
			Mem xPSR_memB = cpu_ptr_byte(reg, 1); \
			c.shr(operand, 8); \
			c.mov(xPSR_memB, operand.r8Lo()); \
			changeCPSR; \
			c.bind(__skip); \
			return 1; \
		} \
		case 0x4: /* bit 18 */ \
		{ \
			GpVar mode = c.newGpVar(kVarTypeInt32); \
			Label __skip = c.newLabel(); \
			c.mov(mode, cpu_ptr(CPSR)); \
			c.and_(mode, 0x1F); \
			c.cmp(mode, USR); \
			c.je(__skip); \
			Mem xPSR_memB = cpu_ptr_byte(reg, 2); \
			c.shr(operand, 16); \
			c.mov(xPSR_memB, operand.r8Lo()); \
			changeCPSR; \
			c.bind(__skip); \
			return 1; \
		} \
		case 0x8: /* bit 19 */ \
		{ \
			Mem xPSR_memB = cpu_ptr_byte(reg, 3); \
			c.shr(operand, 24); \
			c.mov(xPSR_memB, operand.r8Lo()); \
			changeCPSR; \
			return 1; \
		} \
	} \
\
	static uint32_t byte_mask = (BIT16(i) ? 0x000000FF : 0x00000000) | (BIT17(i) ? 0x0000FF00 : 0x00000000) | (BIT18(i) ? 0x00FF0000 : 0x00000000) | (BIT19(i) ? 0xFF000000 : 0x00000000); \
	static uint32_t byte_mask_USR = BIT19(i) ? 0xFF000000 : 0x00000000; \
\
	Mem xPSR_mem = cpu_ptr(reg.val); \
	GpVar xPSR = c.newGpVar(kVarTypeInt32); \
	GpVar mode = c.newGpVar(kVarTypeInt32); \
	Label __USR = c.newLabel(); \
	Label __done = c.newLabel(); \
	c.mov(mode, cpu_ptr(CPSR.val)); \
	c.and_(mode, 0x1F); \
	c.cmp(mode, USR); \
	c.je(__USR); \
	/* mode != USR */ \
	if (sw && BIT16(i)) \
	{ \
		/* armcpu_switchMode */ \
		c.mov(mode, rhs); \
		c.and_(mode, 0x1F); \
		auto ctx = c.addCall(imm_ptr(armcpu_switchMode), ASMJIT_CALL_CONV, \
			FuncBuilder2<void, void *, uint8_t>()); \
		ctx->setArg(0, bb_cpu); \
		ctx->setArg(1, mode); \
	} \
	/* cpu->CPSR.val = (cpu->CPSR.val & ~byte_mask) | (operand & byte_mask); */ \
	c.mov(xPSR, xPSR_mem); \
	c.and_(operand, byte_mask); \
	c.and_(xPSR, ~byte_mask); \
	c.or_(xPSR, operand); \
	c.mov(xPSR_mem, xPSR); \
	c.jmp(__done); \
	/* mode == USR */ \
	c.bind(__USR); \
	c.mov(xPSR, xPSR_mem); \
	c.and_(operand, byte_mask_USR); \
	c.and_(xPSR, ~byte_mask_USR); \
	c.or_(xPSR, operand); \
	c.mov(xPSR_mem, xPSR); \
	c.bind(__done); \
	changeCPSR; \
	return 1;

static int OP_MSR_CPSR(uint32_t i) { OP_MSR_(CPSR, REG_OFF, 1); }
static int OP_MSR_SPSR(uint32_t i) { OP_MSR_(SPSR, REG_OFF, 0); }
static int OP_MSR_CPSR_IMM_VAL(uint32_t i) { OP_MSR_(CPSR, IMM_VAL, 1); }
static int OP_MSR_SPSR_IMM_VAL(uint32_t i) { OP_MSR_(SPSR, IMM_VAL, 0); }

// -----------------------------------------------------------------------------
//   LDR
// -----------------------------------------------------------------------------
typedef uint32_t (FASTCALL *OpLDR)(uint32_t, uint32_t *);

// 98% of all memory accesses land in the same region as the first execution of
// that instruction, so keep multiple copies with different fastpaths.
// The copies don't need to differ in any way; the point is merely to cooperate
// with x86 branch prediction.

enum
{
	MEMTYPE_GENERIC, // no assumptions
	MEMTYPE_MAIN,
	MEMTYPE_DTCM,
	MEMTYPE_ERAM,
	MEMTYPE_SWIRAM,
	MEMTYPE_OTHER // memory that is known to not be MAIN, DTCM, ERAM, or SWIRAM
};

static uint32_t classify_adr(uint32_t adr, bool store)
{
	if (PROCNUM == ARMCPU_ARM9 && (adr & ~0x3FFF) == MMU.DTCMRegion)
		return MEMTYPE_DTCM;
	else if ((adr & 0x0F000000) == 0x02000000)
		return MEMTYPE_MAIN;
	else if (PROCNUM == ARMCPU_ARM7 && !store && (adr & 0xFF800000) == 0x03800000)
		return MEMTYPE_ERAM;
	else if (PROCNUM == ARMCPU_ARM7 && !store && (adr & 0xFF800000) == 0x03000000)
		return MEMTYPE_SWIRAM;
	else
		return MEMTYPE_GENERIC;
}

template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_LDR(uint32_t adr, uint32_t *dstreg)
{
	uint32_t data = READ32(cpu->mem_if->data, adr);
	if (adr & 3)
		data = ROR(data, 8 * (adr & 3));
	*dstreg = data;
	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(3, adr);
}

template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_LDRH(uint32_t adr, uint32_t *dstreg)
{
	*dstreg = READ16(cpu->mem_if->data, adr);
	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(3, adr);
}

template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_LDRSH(uint32_t adr, uint32_t *dstreg)
{
	*dstreg = static_cast<int16_t>(READ16(cpu->mem_if->data, adr));
	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(3, adr);
}

template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_LDRB(uint32_t adr, uint32_t *dstreg)
{
	*dstreg = READ8(cpu->mem_if->data, adr);
	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(3, adr);
}

template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_LDRSB(uint32_t adr, uint32_t *dstreg)
{
	*dstreg = static_cast<int8_t>(READ8(cpu->mem_if->data, adr));
	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(3, adr);
}

#define T(op) op<0, 0>, op<0, 1>, op<0, 2>, nullptr, nullptr, op<1, 0>, op<1, 1>, nullptr, op<1, 3>, op<1, 4>
static const OpLDR LDR_tab[2][5] = { T(OP_LDR) };
static const OpLDR LDRH_tab[2][5] = { T(OP_LDRH) };
static const OpLDR LDRSH_tab[2][5] = { T(OP_LDRSH) };
static const OpLDR LDRB_tab[2][5] = { T(OP_LDRB) };
static const OpLDR LDRSB_tab[2][5] = { T(OP_LDRSB) };
#undef T

static uint32_t add(uint32_t lhs, uint32_t rhs) { return lhs + rhs; }
static uint32_t sub(uint32_t lhs, uint32_t rhs) { return lhs - rhs; }

#define OP_LDR_(mem_op, arg, sign_op, writeback) \
	GpVar adr = c.newGpVar(kVarTypeInt32); \
	GpVar dst = c.newGpVar(kVarTypeIntPtr); \
	c.mov(adr, reg_pos_ptr(16)); \
	c.lea(dst, reg_pos_ptr(12)); \
	arg; \
	if (!rhs_is_imm || *reinterpret_cast<uint32_t *>(&rhs)) \
	{ \
		if (!writeback) \
			c.sign_op(adr, rhs); \
		else if (writeback < 0) \
		{ \
			c.sign_op(adr, rhs); \
			c.mov(reg_pos_ptr(16), adr); \
		} \
		else if (writeback > 0) \
		{ \
			GpVar tmp_reg = c.newGpVar(kVarTypeInt32); \
			c.mov(tmp_reg, adr); \
			c.sign_op(tmp_reg, rhs); \
			c.mov(reg_pos_ptr(16), tmp_reg); \
		} \
	} \
	uint32_t adr_first = sign_op(cpu->R[REG_POS(i, 16)], rhs_first); \
	auto ctx = c.addCall(imm_ptr(mem_op##_tab[PROCNUM][classify_adr(adr_first, 0)]), ASMJIT_CALL_CONV, \
		FuncBuilder2<uint32_t, uint32_t, uint32_t *>()); \
	ctx->setArg(0, adr); \
	ctx->setArg(1, dst); \
	ctx->setRet(0, bb_cycles); \
	if (REG_POS(i, 12) == 15) \
	{ \
		GpVar tmp = c.newGpVar(kVarTypeInt32); \
		c.mov(tmp, reg_ptr(15)); \
		if (!PROCNUM) \
		{ \
			GpVar thumb = c.newGpVar(kVarTypeIntPtr); \
			c.movzx(thumb, reg_pos_ptrB(16)); \
			c.and_(thumb, 1); \
			c.shl(thumb, 5); \
			c.or_(cpu_ptr(CPSR), thumb.r64()); \
			c.and_(tmp, 0xFFFFFFFE); \
		} \
		else \
			c.and_(tmp, 0xFFFFFFFC); \
		c.mov(cpu_ptr(next_instruction), tmp); \
	} \
	return 1;

// LDR
static int OP_LDR_P_IMM_OFF(uint32_t i) { OP_LDR_(LDR, IMM_OFF_12, add, 0); }
static int OP_LDR_M_IMM_OFF(uint32_t i) { OP_LDR_(LDR, IMM_OFF_12, sub, 0); }
static int OP_LDR_P_LSL_IMM_OFF(uint32_t i) { OP_LDR_(LDR, LSL_IMM, add, 0); }
static int OP_LDR_M_LSL_IMM_OFF(uint32_t i) { OP_LDR_(LDR, LSL_IMM, sub, 0); }
static int OP_LDR_P_LSR_IMM_OFF(uint32_t i) { OP_LDR_(LDR, LSR_IMM, add, 0); }
static int OP_LDR_M_LSR_IMM_OFF(uint32_t i) { OP_LDR_(LDR, LSR_IMM, sub, 0); }
static int OP_LDR_P_ASR_IMM_OFF(uint32_t i) { OP_LDR_(LDR, ASR_IMM, add, 0); }
static int OP_LDR_M_ASR_IMM_OFF(uint32_t i) { OP_LDR_(LDR, ASR_IMM, sub, 0); }
static int OP_LDR_P_ROR_IMM_OFF(uint32_t i) { OP_LDR_(LDR, ROR_IMM, add, 0); }
static int OP_LDR_M_ROR_IMM_OFF(uint32_t i) { OP_LDR_(LDR, ROR_IMM, sub, 0); }

static int OP_LDR_P_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, IMM_OFF_12, add, -1); }
static int OP_LDR_M_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, IMM_OFF_12, sub, -1); }
static int OP_LDR_P_LSL_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, LSL_IMM, add, -1); }
static int OP_LDR_M_LSL_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, LSL_IMM, sub, -1); }
static int OP_LDR_P_LSR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, LSR_IMM, add, -1); }
static int OP_LDR_M_LSR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, LSR_IMM, sub, -1); }
static int OP_LDR_P_ASR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, ASR_IMM, add, -1); }
static int OP_LDR_M_ASR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, ASR_IMM, sub, -1); }
static int OP_LDR_P_ROR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, ROR_IMM, add, -1); }
static int OP_LDR_M_ROR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDR, ROR_IMM, sub, -1); }
static int OP_LDR_P_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, IMM_OFF_12, add, 1); }
static int OP_LDR_M_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, IMM_OFF_12, sub, 1); }
static int OP_LDR_P_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, LSL_IMM, add, 1); }
static int OP_LDR_M_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, LSL_IMM, sub, 1); }
static int OP_LDR_P_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, LSR_IMM, add, 1); }
static int OP_LDR_M_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, LSR_IMM, sub, 1); }
static int OP_LDR_P_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, ASR_IMM, add, 1); }
static int OP_LDR_M_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, ASR_IMM, sub, 1); }
static int OP_LDR_P_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, ROR_IMM, add, 1); }
static int OP_LDR_M_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDR, ROR_IMM, sub, 1); }

// LDRH
static int OP_LDRH_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRH, IMM_OFF, add, 0); }
static int OP_LDRH_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRH, IMM_OFF, sub, 0); }
static int OP_LDRH_P_REG_OFF(uint32_t i) { OP_LDR_(LDRH, REG_OFF, add, 0); }
static int OP_LDRH_M_REG_OFF(uint32_t i) { OP_LDR_(LDRH, REG_OFF, sub, 0); }

static int OP_LDRH_PRE_INDE_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRH, IMM_OFF, add, -1); }
static int OP_LDRH_PRE_INDE_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRH, IMM_OFF, sub, -1); }
static int OP_LDRH_PRE_INDE_P_REG_OFF(uint32_t i) { OP_LDR_(LDRH, REG_OFF, add, -1); }
static int OP_LDRH_PRE_INDE_M_REG_OFF(uint32_t i) { OP_LDR_(LDRH, REG_OFF, sub, -1); }
static int OP_LDRH_POS_INDE_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRH, IMM_OFF, add, 1); }
static int OP_LDRH_POS_INDE_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRH, IMM_OFF, sub, 1); }
static int OP_LDRH_POS_INDE_P_REG_OFF(uint32_t i) { OP_LDR_(LDRH, REG_OFF, add, 1); }
static int OP_LDRH_POS_INDE_M_REG_OFF(uint32_t i) { OP_LDR_(LDRH, REG_OFF, sub, 1); }

// LDRSH
static int OP_LDRSH_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRSH, IMM_OFF, add, 0); }
static int OP_LDRSH_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRSH, IMM_OFF, sub, 0); }
static int OP_LDRSH_P_REG_OFF(uint32_t i) { OP_LDR_(LDRSH, REG_OFF, add, 0); }
static int OP_LDRSH_M_REG_OFF(uint32_t i) { OP_LDR_(LDRSH, REG_OFF, sub, 0); }

static int OP_LDRSH_PRE_INDE_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRSH, IMM_OFF, add, -1); }
static int OP_LDRSH_PRE_INDE_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRSH, IMM_OFF, sub, -1); }
static int OP_LDRSH_PRE_INDE_P_REG_OFF(uint32_t i) { OP_LDR_(LDRSH, REG_OFF, add, -1); }
static int OP_LDRSH_PRE_INDE_M_REG_OFF(uint32_t i) { OP_LDR_(LDRSH, REG_OFF, sub, -1); }
static int OP_LDRSH_POS_INDE_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRSH, IMM_OFF, add, 1); }
static int OP_LDRSH_POS_INDE_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRSH, IMM_OFF, sub, 1); }
static int OP_LDRSH_POS_INDE_P_REG_OFF(uint32_t i) { OP_LDR_(LDRSH, REG_OFF, add, 1); }
static int OP_LDRSH_POS_INDE_M_REG_OFF(uint32_t i) { OP_LDR_(LDRSH, REG_OFF, sub, 1); }

// LDRB
static int OP_LDRB_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, IMM_OFF_12, add, 0); }
static int OP_LDRB_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, IMM_OFF_12, sub, 0); }
static int OP_LDRB_P_LSL_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, LSL_IMM, add, 0); }
static int OP_LDRB_M_LSL_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, LSL_IMM, sub, 0); }
static int OP_LDRB_P_LSR_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, LSR_IMM, add, 0); }
static int OP_LDRB_M_LSR_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, LSR_IMM, sub, 0); }
static int OP_LDRB_P_ASR_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, ASR_IMM, add, 0); }
static int OP_LDRB_M_ASR_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, ASR_IMM, sub, 0); }
static int OP_LDRB_P_ROR_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, ROR_IMM, add, 0); }
static int OP_LDRB_M_ROR_IMM_OFF(uint32_t i) { OP_LDR_(LDRB, ROR_IMM, sub, 0); }

static int OP_LDRB_P_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, IMM_OFF_12, add, -1); }
static int OP_LDRB_M_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, IMM_OFF_12, sub, -1); }
static int OP_LDRB_P_LSL_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, LSL_IMM, add, -1); }
static int OP_LDRB_M_LSL_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, LSL_IMM, sub, -1); }
static int OP_LDRB_P_LSR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, LSR_IMM, add, -1); }
static int OP_LDRB_M_LSR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, LSR_IMM, sub, -1); }
static int OP_LDRB_P_ASR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, ASR_IMM, add, -1); }
static int OP_LDRB_M_ASR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, ASR_IMM, sub, -1); }
static int OP_LDRB_P_ROR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, ROR_IMM, add, -1); }
static int OP_LDRB_M_ROR_IMM_OFF_PREIND(uint32_t i) { OP_LDR_(LDRB, ROR_IMM, sub, -1); }
static int OP_LDRB_P_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, IMM_OFF_12, add, 1); }
static int OP_LDRB_M_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, IMM_OFF_12, sub, 1); }
static int OP_LDRB_P_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, LSL_IMM, add, 1); }
static int OP_LDRB_M_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, LSL_IMM, sub, 1); }
static int OP_LDRB_P_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, LSR_IMM, add, 1); }
static int OP_LDRB_M_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, LSR_IMM, sub, 1); }
static int OP_LDRB_P_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, ASR_IMM, add, 1); }
static int OP_LDRB_M_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, ASR_IMM, sub, 1); }
static int OP_LDRB_P_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, ROR_IMM, add, 1); }
static int OP_LDRB_M_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_LDR_(LDRB, ROR_IMM, sub, 1); }

// LDRSB
static int OP_LDRSB_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRSB, IMM_OFF, add, 0); }
static int OP_LDRSB_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRSB, IMM_OFF, sub, 0); }
static int OP_LDRSB_P_REG_OFF(uint32_t i) { OP_LDR_(LDRSB, REG_OFF, add, 0); }
static int OP_LDRSB_M_REG_OFF(uint32_t i) { OP_LDR_(LDRSB, REG_OFF, sub, 0); }

static int OP_LDRSB_PRE_INDE_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRSB, IMM_OFF, add, -1); }
static int OP_LDRSB_PRE_INDE_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRSB, IMM_OFF, sub, -1); }
static int OP_LDRSB_PRE_INDE_P_REG_OFF(uint32_t i) { OP_LDR_(LDRSB, REG_OFF, add, -1); }
static int OP_LDRSB_PRE_INDE_M_REG_OFF(uint32_t i) { OP_LDR_(LDRSB, REG_OFF, sub, -1); }
static int OP_LDRSB_POS_INDE_P_IMM_OFF(uint32_t i) { OP_LDR_(LDRSB, IMM_OFF, add, 1); }
static int OP_LDRSB_POS_INDE_M_IMM_OFF(uint32_t i) { OP_LDR_(LDRSB, IMM_OFF, sub, 1); }
static int OP_LDRSB_POS_INDE_P_REG_OFF(uint32_t i) { OP_LDR_(LDRSB, REG_OFF, add, 1); }
static int OP_LDRSB_POS_INDE_M_REG_OFF(uint32_t i) { OP_LDR_(LDRSB, REG_OFF, sub, 1); }

// -----------------------------------------------------------------------------
//   STR
// -----------------------------------------------------------------------------
template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_STR(uint32_t adr, uint32_t data)
{
	WRITE32(cpu->mem_if->data, adr, data);
	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(2, adr);
}

template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_STRH(uint32_t adr, uint32_t data)
{
	WRITE16(cpu->mem_if->data, adr, data);
	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(2, adr);
}

template<int PROCNUM, int memtype> static uint32_t FASTCALL OP_STRB(uint32_t adr, uint32_t data)
{
	WRITE8(cpu->mem_if->data, adr, data);
	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(2, adr);
}

typedef uint32_t (FASTCALL *OpSTR)(uint32_t, uint32_t);
#define T(op) op<0, 0>, op<0, 1>, op<0, 2>, op<1, 0>, op<1, 1>, nullptr
static const OpSTR STR_tab[2][3] = { T(OP_STR) };
static const OpSTR STRH_tab[2][3] = { T(OP_STRH) };
static const OpSTR STRB_tab[2][3] = { T(OP_STRB) };
#undef T

#define OP_STR_(mem_op, arg, sign_op, writeback) \
	GpVar adr = c.newGpVar(kVarTypeInt32); \
	GpVar data = c.newGpVar(kVarTypeInt32); \
	c.mov(adr, reg_pos_ptr(16)); \
	c.mov(data, reg_pos_ptr(12)); \
	arg; \
	if (!rhs_is_imm || *reinterpret_cast<uint32_t *>(&rhs)) \
	{ \
		if (!writeback) \
			c.sign_op(adr, rhs); \
		else if (writeback < 0) \
		{ \
			c.sign_op(adr, rhs); \
			c.mov(reg_pos_ptr(16), adr); \
		} \
		else if (writeback > 0) \
		{ \
			GpVar tmp_reg = c.newGpVar(kVarTypeInt32); \
			c.mov(tmp_reg, adr); \
			c.sign_op(tmp_reg, rhs); \
			c.mov(reg_pos_ptr(16), tmp_reg); \
		} \
	} \
	uint32_t adr_first = sign_op(cpu->R[REG_POS(i,16)], rhs_first); \
	auto ctx = c.addCall(imm_ptr(mem_op##_tab[PROCNUM][classify_adr(adr_first, 1)]), ASMJIT_CALL_CONV, \
		FuncBuilder2<uint32_t, uint32_t, uint32_t>()); \
	ctx->setArg(0, adr); \
	ctx->setArg(1, data); \
	ctx->setRet(0, bb_cycles); \
	return 1;

static int OP_STR_P_IMM_OFF(uint32_t i) { OP_STR_(STR, IMM_OFF_12, add, 0); }
static int OP_STR_M_IMM_OFF(uint32_t i) { OP_STR_(STR, IMM_OFF_12, sub, 0); }
static int OP_STR_P_LSL_IMM_OFF(uint32_t i) { OP_STR_(STR, LSL_IMM, add, 0); }
static int OP_STR_M_LSL_IMM_OFF(uint32_t i) { OP_STR_(STR, LSL_IMM, sub, 0); }
static int OP_STR_P_LSR_IMM_OFF(uint32_t i) { OP_STR_(STR, LSR_IMM, add, 0); }
static int OP_STR_M_LSR_IMM_OFF(uint32_t i) { OP_STR_(STR, LSR_IMM, sub, 0); }
static int OP_STR_P_ASR_IMM_OFF(uint32_t i) { OP_STR_(STR, ASR_IMM, add, 0); }
static int OP_STR_M_ASR_IMM_OFF(uint32_t i) { OP_STR_(STR, ASR_IMM, sub, 0); }
static int OP_STR_P_ROR_IMM_OFF(uint32_t i) { OP_STR_(STR, ROR_IMM, add, 0); }
static int OP_STR_M_ROR_IMM_OFF(uint32_t i) { OP_STR_(STR, ROR_IMM, sub, 0); }

static int OP_STR_P_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, IMM_OFF_12, add, -1); }
static int OP_STR_M_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, IMM_OFF_12, sub, -1); }
static int OP_STR_P_LSL_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, LSL_IMM, add, -1); }
static int OP_STR_M_LSL_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, LSL_IMM, sub, -1); }
static int OP_STR_P_LSR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, LSR_IMM, add, -1); }
static int OP_STR_M_LSR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, LSR_IMM, sub, -1); }
static int OP_STR_P_ASR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, ASR_IMM, add, -1); }
static int OP_STR_M_ASR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, ASR_IMM, sub, -1); }
static int OP_STR_P_ROR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, ROR_IMM, add, -1); }
static int OP_STR_M_ROR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STR, ROR_IMM, sub, -1); }
static int OP_STR_P_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, IMM_OFF_12, add, 1); }
static int OP_STR_M_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, IMM_OFF_12, sub, 1); }
static int OP_STR_P_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, LSL_IMM, add, 1); }
static int OP_STR_M_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, LSL_IMM, sub, 1); }
static int OP_STR_P_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, LSR_IMM, add, 1); }
static int OP_STR_M_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, LSR_IMM, sub, 1); }
static int OP_STR_P_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, ASR_IMM, add, 1); }
static int OP_STR_M_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, ASR_IMM, sub, 1); }
static int OP_STR_P_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, ROR_IMM, add, 1); }
static int OP_STR_M_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STR, ROR_IMM, sub, 1); }

static int OP_STRH_P_IMM_OFF(uint32_t i) { OP_STR_(STRH, IMM_OFF, add, 0); }
static int OP_STRH_M_IMM_OFF(uint32_t i) { OP_STR_(STRH, IMM_OFF, sub, 0); }
static int OP_STRH_P_REG_OFF(uint32_t i) { OP_STR_(STRH, REG_OFF, add, 0); }
static int OP_STRH_M_REG_OFF(uint32_t i) { OP_STR_(STRH, REG_OFF, sub, 0); }

static int OP_STRH_PRE_INDE_P_IMM_OFF(uint32_t i) { OP_STR_(STRH, IMM_OFF, add, -1); }
static int OP_STRH_PRE_INDE_M_IMM_OFF(uint32_t i) { OP_STR_(STRH, IMM_OFF, sub, -1); }
static int OP_STRH_PRE_INDE_P_REG_OFF(uint32_t i) { OP_STR_(STRH, REG_OFF, add, -1); }
static int OP_STRH_PRE_INDE_M_REG_OFF(uint32_t i) { OP_STR_(STRH, REG_OFF, sub, -1); }
static int OP_STRH_POS_INDE_P_IMM_OFF(uint32_t i) { OP_STR_(STRH, IMM_OFF, add, 1); }
static int OP_STRH_POS_INDE_M_IMM_OFF(uint32_t i) { OP_STR_(STRH, IMM_OFF, sub, 1); }
static int OP_STRH_POS_INDE_P_REG_OFF(uint32_t i) { OP_STR_(STRH, REG_OFF, add, 1); }
static int OP_STRH_POS_INDE_M_REG_OFF(uint32_t i) { OP_STR_(STRH, REG_OFF, sub, 1); }

static int OP_STRB_P_IMM_OFF(uint32_t i) { OP_STR_(STRB, IMM_OFF_12, add, 0); }
static int OP_STRB_M_IMM_OFF(uint32_t i) { OP_STR_(STRB, IMM_OFF_12, sub, 0); }
static int OP_STRB_P_LSL_IMM_OFF(uint32_t i) { OP_STR_(STRB, LSL_IMM, add, 0); }
static int OP_STRB_M_LSL_IMM_OFF(uint32_t i) { OP_STR_(STRB, LSL_IMM, sub, 0); }
static int OP_STRB_P_LSR_IMM_OFF(uint32_t i) { OP_STR_(STRB, LSR_IMM, add, 0); }
static int OP_STRB_M_LSR_IMM_OFF(uint32_t i) { OP_STR_(STRB, LSR_IMM, sub, 0); }
static int OP_STRB_P_ASR_IMM_OFF(uint32_t i) { OP_STR_(STRB, ASR_IMM, add, 0); }
static int OP_STRB_M_ASR_IMM_OFF(uint32_t i) { OP_STR_(STRB, ASR_IMM, sub, 0); }
static int OP_STRB_P_ROR_IMM_OFF(uint32_t i) { OP_STR_(STRB, ROR_IMM, add, 0); }
static int OP_STRB_M_ROR_IMM_OFF(uint32_t i) { OP_STR_(STRB, ROR_IMM, sub, 0); }

static int OP_STRB_P_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, IMM_OFF_12, add, -1); }
static int OP_STRB_M_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, IMM_OFF_12, sub, -1); }
static int OP_STRB_P_LSL_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, LSL_IMM, add, -1); }
static int OP_STRB_M_LSL_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, LSL_IMM, sub, -1); }
static int OP_STRB_P_LSR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, LSR_IMM, add, -1); }
static int OP_STRB_M_LSR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, LSR_IMM, sub, -1); }
static int OP_STRB_P_ASR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, ASR_IMM, add, -1); }
static int OP_STRB_M_ASR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, ASR_IMM, sub, -1); }
static int OP_STRB_P_ROR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, ROR_IMM, add, -1); }
static int OP_STRB_M_ROR_IMM_OFF_PREIND(uint32_t i) { OP_STR_(STRB, ROR_IMM, sub, -1); }
static int OP_STRB_P_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, IMM_OFF_12, add, 1); }
static int OP_STRB_M_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, IMM_OFF_12, sub, 1); }
static int OP_STRB_P_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, LSL_IMM, add, 1); }
static int OP_STRB_M_LSL_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, LSL_IMM, sub, 1); }
static int OP_STRB_P_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, LSR_IMM, add, 1); }
static int OP_STRB_M_LSR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, LSR_IMM, sub, 1); }
static int OP_STRB_P_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, ASR_IMM, add, 1); }
static int OP_STRB_M_ASR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, ASR_IMM, sub, 1); }
static int OP_STRB_P_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, ROR_IMM, add, 1); }
static int OP_STRB_M_ROR_IMM_OFF_POSTIND(uint32_t i) { OP_STR_(STRB, ROR_IMM, sub, 1); }

// -----------------------------------------------------------------------------
//   LDRD / STRD
// -----------------------------------------------------------------------------
typedef uint32_t FASTCALL (*LDRD_STRD_REG)(uint32_t);

template<int PROCNUM, uint8_t Rnum> static uint32_t FASTCALL OP_LDRD_REG(uint32_t adr)
{
	cpu->R[Rnum] = READ32(cpu->mem_if->data, adr);
	cpu->R[Rnum + 1] = READ32(cpu->mem_if->data, adr + 4);
	return MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(adr) + MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(adr + 4);
}

template<int PROCNUM, uint8_t Rnum> static uint32_t FASTCALL OP_STRD_REG(uint32_t adr)
{
	WRITE32(cpu->mem_if->data, adr, cpu->R[Rnum]);
	WRITE32(cpu->mem_if->data, adr + 4, cpu->R[Rnum + 1]);
	return MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(adr) + MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(adr + 4);
}

#define T(op, proc) op<proc, 0>, op<proc, 1>, op<proc, 2>, op<proc, 3>, op<proc, 4>, op<proc, 5>, op<proc, 6>, op<proc, 7>, \
	op<proc, 8>, op<proc, 9>, op<proc, 10>, op<proc, 11>, op<proc, 12>, op<proc, 13>, op<proc, 14>, op<proc, 15>
static const LDRD_STRD_REG op_ldrd_tab[2][16] = { { T(OP_LDRD_REG, 0) }, { T(OP_LDRD_REG, 1) } };
static const LDRD_STRD_REG op_strd_tab[2][16] = { { T(OP_STRD_REG, 0) }, { T(OP_STRD_REG, 1) } };
#undef T

static int OP_LDRD_STRD_POST_INDEX(uint32_t i)
{
	uint8_t Rd_num = REG_POS(i, 12);

	if (Rd_num == 14)
	{
		printf("OP_LDRD_STRD_POST_INDEX: use R14!!!!\n");
		return 0; // TODO: exception
	}
	if (Rd_num & 0x1)
	{
		printf("OP_LDRD_STRD_POST_INDEX: ERROR!!!!\n");
		return 0; // TODO: exception
	}
	GpVar Rd = c.newGpVar(kVarTypeInt32);
	GpVar addr = c.newGpVar(kVarTypeInt32);

	c.mov(Rd, reg_pos_ptr(16));
	c.mov(addr, reg_pos_ptr(16));

	// I bit - immediate or register
	if (BIT22(i))
	{
		IMM_OFF;
		BIT23(i) ? c.add(reg_pos_ptr(16), rhs) : c.sub(reg_pos_ptr(16), rhs);
	}
	else
	{
		GpVar idx = c.newGpVar(kVarTypeInt32);
		c.mov(idx, reg_pos_ptr(0));
		BIT23(i) ? c.add(reg_pos_ptr(16), idx) : c.sub(reg_pos_ptr(16), idx);
	}

	auto ctx = c.addCall(imm_ptr(BIT5(i) ? op_strd_tab[PROCNUM][Rd_num] : op_ldrd_tab[PROCNUM][Rd_num]),
		ASMJIT_CALL_CONV, FuncBuilder1<uint32_t, uint32_t>());
	ctx->setArg(0, addr);
	ctx->setRet(0, bb_cycles);
	emit_MMU_aluMemCycles(3, bb_cycles, 0);
	return 1;
}

static int OP_LDRD_STRD_OFFSET_PRE_INDEX(uint32_t i)
{
	uint8_t Rd_num = REG_POS(i, 12);

	if (Rd_num == 14)
	{
		printf("OP_LDRD_STRD_OFFSET_PRE_INDEX: use R14!!!!\n");
		return 0; // TODO: exception
	}
	if (Rd_num & 0x1)
	{
		printf("OP_LDRD_STRD_OFFSET_PRE_INDEX: ERROR!!!!\n");
		return 0; // TODO: exception
	}
	GpVar Rd = c.newGpVar(kVarTypeInt32);
	GpVar addr = c.newGpVar(kVarTypeInt32);

	c.mov(Rd, reg_pos_ptr(16));
	c.mov(addr, reg_pos_ptr(16));

	// I bit - immediate or register
	if (BIT22(i))
	{
		IMM_OFF;
		BIT23(i) ? c.add(addr, rhs) : c.sub(addr, rhs);
	}
	else
		BIT23(i) ? c.add(addr, reg_pos_ptr(0)) : c.sub(addr, reg_pos_ptr(0));

	if (BIT5(i)) // Store
	{
		auto ctx = c.addCall(imm_ptr(op_strd_tab[PROCNUM][Rd_num]), ASMJIT_CALL_CONV,
			FuncBuilder1<uint32_t, uint32_t>());
		ctx->setArg(0, addr);
		ctx->setRet(0, bb_cycles);
		if (BIT21(i)) // W bit - writeback
			c.mov(reg_pos_ptr(16), addr);
		emit_MMU_aluMemCycles(3, bb_cycles, 0);
	}
	else // Load
	{
		if (BIT21(i)) // W bit - writeback
			c.mov(reg_pos_ptr(16), addr);
		auto ctx = c.addCall(imm_ptr(op_ldrd_tab[PROCNUM][Rd_num]), ASMJIT_CALL_CONV,
			FuncBuilder1<uint32_t, uint32_t>());
		ctx->setArg(0, addr);
		ctx->setRet(0, bb_cycles);
		emit_MMU_aluMemCycles(3, bb_cycles, 0);
	}
	return 1;
}

// -----------------------------------------------------------------------------
//   SWP/SWPB
// -----------------------------------------------------------------------------
template<int PROCNUM> static uint32_t FASTCALL op_swp(uint32_t adr, uint32_t *Rd, uint32_t Rs)
{
	uint32_t tmp = ROR(READ32(cpu->mem_if->data, adr), (adr & 3) << 3);
	WRITE32(cpu->mem_if->data, adr, Rs);
	*Rd = tmp;
	return MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(adr) + MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(adr);
}

template<int PROCNUM> static uint32_t FASTCALL op_swpb(uint32_t adr, uint32_t *Rd, uint32_t Rs)
{
	uint32_t tmp = READ8(cpu->mem_if->data, adr);
	WRITE8(cpu->mem_if->data, adr, Rs);
	*Rd = tmp;
	return MMU_memAccessCycles<PROCNUM, 8, MMU_AD_READ>(adr) + MMU_memAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(adr);
}

typedef uint32_t FASTCALL (*OP_SWP_SWPB)(uint32_t, uint32_t *, uint32_t);
static const OP_SWP_SWPB op_swp_tab[2][2] = { { op_swp<0>, op_swp<1> }, { op_swpb<0>, op_swpb<1> } };

static int op_swp_(uint32_t i, int b)
{
	GpVar addr = c.newGpVar(kVarTypeInt32);
	GpVar Rd = c.newGpVar(kVarTypeIntPtr);
	GpVar Rs = c.newGpVar(kVarTypeInt32);
	c.mov(addr, reg_pos_ptr(16));
	c.lea(Rd, reg_pos_ptr(12));
	if (b)
		c.movzx(Rs, reg_pos_ptrB(0));
	else
		c.mov(Rs, reg_pos_ptr(0));
	auto ctx = c.addCall(imm_ptr(op_swp_tab[b][PROCNUM]), ASMJIT_CALL_CONV,
		FuncBuilder3<uint32_t, uint32_t, uint32_t *, uint32_t>());
	ctx->setArg(0, addr);
	ctx->setArg(1, Rd);
	ctx->setArg(2, Rs);
	ctx->setRet(0, bb_cycles);
	emit_MMU_aluMemCycles(4, bb_cycles, 0);
	return 1;
}

static int OP_SWP(uint32_t i) { return op_swp_(i, 0); }
static int OP_SWPB(uint32_t i) { return op_swp_(i, 1); }

// -----------------------------------------------------------------------------
//   LDMIA / LDMIB / LDMDA / LDMDB / STMIA / STMIB / STMDA / STMDB
// -----------------------------------------------------------------------------
static uint32_t popcount(uint32_t x)
{
	uint32_t pop = 0;
	for (; x; x >>= 1)
		pop += x & 1;
	return pop;
}

static uint64_t get_reg_list(uint32_t reg_mask, int dir)
{
	uint64_t regs = 0;
	for (int j = 0; j < 16; ++j)
	{
		int k = dir < 0 ? j : 15 - j;
		if (BIT_N(reg_mask, k))
			regs = (regs << 4) | k;
	}
	return regs;
}

#ifdef ASMJIT_X64
// generic needs to spill regs and main doesn't; if it's inlined gcc isn't smart enough to keep the spills out of the common case.
#define LDM_INLINE
#else
// spills either way, and we might as well save codesize by not having separate functions
#define LDM_INLINE inline
#endif

template<int PROCNUM, bool store, int dir> static LDM_INLINE FASTCALL uint32_t OP_LDM_STM_generic(uint32_t adr, uint64_t regs, int n)
{
	uint32_t cycles = 0;
	adr &= ~3;
	do
	{
		if (store)
			_MMU_write32<PROCNUM>(adr, cpu->R[regs & 0xF]);
		else
			cpu->R[regs & 0xF] = _MMU_read32<PROCNUM>(adr);
		cycles += MMU_memAccessCycles<PROCNUM, 32, store ? MMU_AD_WRITE : MMU_AD_READ>(adr);
		adr += 4 * dir;
		regs >>= 4;
	} while (--n > 0);
	return cycles;
}

#ifdef ENABLE_ADVANCED_TIMING
#define ADV_CYCLES cycles += MMU_memAccessCycles<PROCNUM, 32, store ? MMU_AD_WRITE : MMU_AD_READ>(adr);
#else
#define ADV_CYCLES
#endif

template<int PROCNUM, bool store, int dir> static LDM_INLINE FASTCALL uint32_t OP_LDM_STM_other(uint32_t adr, uint64_t regs, int n)
{
	uint32_t cycles = 0;
	adr &= ~3;
#ifndef ENABLE_ADVANCED_TIMING
	cycles = n * MMU_memAccessCycles<PROCNUM, 32, store ? MMU_AD_WRITE : MMU_AD_READ>(adr);
#endif
	do
	{
		if (PROCNUM == ARMCPU_ARM9)
		{
			if (store)
				_MMU_ARM9_write32(adr, cpu->R[regs & 0xF]);
			else
				cpu->R[regs & 0xF] = _MMU_ARM9_read32(adr);
		}
		else
		{
			if (store)
				_MMU_ARM7_write32(adr, cpu->R[regs & 0xF]);
			else
				cpu->R[regs & 0xF] = _MMU_ARM7_read32(adr);
		}
		ADV_CYCLES;
		adr += 4 * dir;
		regs >>= 4;
	} while (--n > 0);
	return cycles;
}

template<int PROCNUM, bool store, int dir, bool null_compiled> static FORCEINLINE FASTCALL uint32_t OP_LDM_STM_main(uint32_t adr, uint64_t regs, int n, uint8_t *ptr, uint32_t cycles)
{
#ifdef ENABLE_ADVANCED_TIMING
	cycles = 0;
#endif
	uintptr_t *func = &JIT_COMPILED_FUNC(adr, PROCNUM);

#define OP(j) \
{ \
	/* no need to zero functions in DTCM, since we can't execute from it */ \
	if (null_compiled && store) \
	{ \
		*func = 0; \
		*(func + 1) = 0; \
	} \
	int Rd = (static_cast<uintptr_t>(regs) >> (j * 4)) & 0xF; \
	if (store) \
		*reinterpret_cast<uint32_t *>(ptr) = cpu->R[Rd]; \
	else \
		cpu->R[Rd] = *reinterpret_cast<uint32_t *>(ptr); \
	ADV_CYCLES; \
	func += 2 * dir; \
	adr += 4 * dir; \
	ptr += 4 * dir; \
}

	do
	{
		OP(0);
		if (n == 1)
			break;
		OP(1);
		if (n == 2)
			break;
		OP(2);
		if (n == 3)
			break;
		OP(3);
		regs >>= 16;
		n -= 4;
	} while (n > 0);
	return cycles;
#undef OP
#undef ADV_CYCLES
}

template<int PROCNUM, bool store, int dir> static uint32_t FASTCALL OP_LDM_STM(uint32_t adr, uint64_t regs, int n)
{
	// TODO use classify_adr?
	uint32_t cycles;
	uint8_t *ptr;

	if ((adr ^ (adr + (dir > 0 ? (n - 1) * 4 : -15 * 4))) & ~0x3FFF) // a little conservative, but we don't want to run too many comparisons
		// the memory region spans a page boundary, so we can't factor the address translation out of the loop
		return OP_LDM_STM_generic<PROCNUM, store, dir>(adr, regs, n);
	else if (PROCNUM == ARMCPU_ARM9 && (adr & ~0x3FFF) == MMU.DTCMRegion)
	{
		// don't special-case DTCM cycles, even though that would be both faster and more accurate,
		// because that wouldn't match the non-jitted version with !ACCOUNT_FOR_DATA_TCM_SPEED
		ptr = MMU.ARM9_DTCM + (adr & 0x3FFC);
		cycles = n * MMU_memAccessCycles<PROCNUM, 32, store ? MMU_AD_WRITE : MMU_AD_READ>(adr);
		if (store)
			return OP_LDM_STM_main<PROCNUM, store, dir, 0>(adr, regs, n, ptr, cycles);
	}
	else if ((adr & 0x0F000000) == 0x02000000)
	{
		ptr = MMU.MAIN_MEM + (adr & _MMU_MAIN_MEM_MASK32);
		cycles = n * (PROCNUM == ARMCPU_ARM9 ? 4 : 2);
	}
	else if (PROCNUM == ARMCPU_ARM7 && !store && (adr & 0xFF800000) == 0x03800000)
	{
		ptr = MMU.ARM7_ERAM + (adr & 0xFFFC);
		cycles = n;
	}
	else if (PROCNUM == ARMCPU_ARM7 && !store && (adr & 0xFF800000) == 0x03000000)
	{
		ptr = MMU.SWIRAM + (adr & 0x7FFC);
		cycles = n;
	}
	else
		return OP_LDM_STM_other<PROCNUM, store, dir>(adr, regs, n);

	return OP_LDM_STM_main<PROCNUM, store, dir, store>(adr, regs, n, ptr, cycles);
}

typedef uint32_t FASTCALL (*LDMOpFunc)(uint32_t, uint64_t, int);
static const LDMOpFunc op_ldm_stm_tab[2][2][2] =
{
	{
		{ OP_LDM_STM<0, 0, -1>, OP_LDM_STM<0, 0, 1> },
		{ OP_LDM_STM<0, 1, -1>, OP_LDM_STM<0, 1, 1> },
	},
	{
		{ OP_LDM_STM<1, 0, -1>, OP_LDM_STM<1, 0, 1> },
		{ OP_LDM_STM<1, 1, -1>, OP_LDM_STM<1, 1, 1> },
	}
};

static void call_ldm_stm(GpVar adr, uint32_t bitmask, bool store, int dir)
{
	if (bitmask)
	{
		GpVar n = c.newGpVar(kVarTypeInt32);
		c.mov(n, popcount(bitmask));
#ifdef ASMJIT_X64
		GpVar regs = c.newGpVar(kVarTypeIntPtr);
		c.mov(regs, get_reg_list(bitmask, dir));
		auto ctx = c.addCall(imm_ptr(op_ldm_stm_tab[PROCNUM][store][dir > 0]), ASMJIT_CALL_CONV,
			FuncBuilder3<uint32_t, uint32_t, uint64_t, int>());
		ctx->setArg(0, adr);
		ctx->setArg(1, regs);
		ctx->setArg(2, n);
#else
		// same prototype, but we have to handle splitting of a u64 arg manually
		GpVar regs_lo = c.newGpVar(kVarTypeInt32);
		GpVar regs_hi = c.newGpVar(kVarTypeInt32);
		c.mov(regs_lo, get_reg_list(bitmask, dir) & 0xFFFFFFFF);
		c.mov(regs_hi, get_reg_list(bitmask, dir) >> 32);
		auto ctx = c.addCall(imm_ptr(op_ldm_stm_tab[PROCNUM][store][dir > 0]), ASMJIT_CALL_CONV,
			FuncBuilder4<uint32_t, uint32_t, uint32_t, uint32_t, int>());
		ctx->setArg(0, adr);
		ctx->setArg(1, regs_lo);
		ctx->setArg(2, regs_hi);
		ctx->setArg(3, n);
#endif
		ctx->setRet(0, bb_cycles);
	}
	else
		++bb_constant_cycles;
}

static int op_bx(Mem srcreg, bool blx, bool test_thumb);
static int op_bx_thumb(Mem srcreg, bool blx, bool test_thumb);

static int op_ldm_stm(uint32_t i, bool store, int dir, bool before, bool writeback)
{
	uint32_t bitmask = i & 0xFFFF;
	uint32_t pop = popcount(bitmask);

	GpVar adr = c.newGpVar(kVarTypeInt32);
	c.mov(adr, reg_pos_ptr(16));
	if (before)
		c.add(adr, 4*dir);

	call_ldm_stm(adr, bitmask, store, dir);

	if (BIT15(i) && !store)
		op_bx(reg_ptr(15), 0, PROCNUM == ARMCPU_ARM9);

	if (writeback)
	{

		if (store || !(i & (1 << REG_POS(i, 16))))
		{
			JIT_COMMENT("--- writeback");
			c.add(reg_pos_ptr(16), 4 * dir * pop);
		}
		else
		{
			uint32_t bitlist = (~((2 << REG_POS(i, 16)) - 1)) & 0xFFFF;
			if (i & bitlist)
			{
				JIT_COMMENT("--- writeback");
				c.add(adr, 4 * dir * (pop - before));
				c.mov(reg_pos_ptr(16), adr);
			}
		}
	}

	emit_MMU_aluMemCycles(store ? 1 : 2, bb_cycles, pop);
	return 1;
}

static int OP_LDMIA(uint32_t i) { return op_ldm_stm(i, 0, +1, 0, 0); }
static int OP_LDMIB(uint32_t i) { return op_ldm_stm(i, 0, +1, 1, 0); }
static int OP_LDMDA(uint32_t i) { return op_ldm_stm(i, 0, -1, 0, 0); }
static int OP_LDMDB(uint32_t i) { return op_ldm_stm(i, 0, -1, 1, 0); }
static int OP_LDMIA_W(uint32_t i) { return op_ldm_stm(i, 0, +1, 0, 1); }
static int OP_LDMIB_W(uint32_t i) { return op_ldm_stm(i, 0, +1, 1, 1); }
static int OP_LDMDA_W(uint32_t i) { return op_ldm_stm(i, 0, -1, 0, 1); }
static int OP_LDMDB_W(uint32_t i) { return op_ldm_stm(i, 0, -1, 1, 1); }

static int OP_STMIA(uint32_t i) { return op_ldm_stm(i, 1, +1, 0, 0); }
static int OP_STMIB(uint32_t i) { return op_ldm_stm(i, 1, +1, 1, 0); }
static int OP_STMDA(uint32_t i) { return op_ldm_stm(i, 1, -1, 0, 0); }
static int OP_STMDB(uint32_t i) { return op_ldm_stm(i, 1, -1, 1, 0); }
static int OP_STMIA_W(uint32_t i) { return op_ldm_stm(i, 1, +1, 0, 1); }
static int OP_STMIB_W(uint32_t i) { return op_ldm_stm(i, 1, +1, 1, 1); }
static int OP_STMDA_W(uint32_t i) { return op_ldm_stm(i, 1, -1, 0, 1); }
static int OP_STMDB_W(uint32_t i) { return op_ldm_stm(i, 1, -1, 1, 1); }

static int op_ldm_stm2(uint32_t i, bool store, int dir, bool before, bool writeback)
{
	uint32_t bitmask = i & 0xFFFF;
	uint32_t pop = popcount(bitmask);
	bool bit15 = !!BIT15(i);

	//printf("ARM%c: %s R%d:%08X, bitmask %02X\n", PROCNUM?'7':'9', (store?"STM":"LDM"), REG_POS(i, 16), cpu->R[REG_POS(i, 16)], bitmask);
	uint32_t adr_first = cpu->R[REG_POS(i, 16)];

	GpVar adr = c.newGpVar(kVarTypeInt32);
	GpVar oldmode = c.newGpVar(kVarTypeInt32);

	c.mov(adr, reg_pos_ptr(16));
	if (before)
		c.add(adr, 4*dir);

	if (!bit15 || store)
	{
		//if((cpu->CPSR.bits.mode==USR)||(cpu->CPSR.bits.mode==SYS)) { printf("ERROR1\n"); return 1; }
		//oldmode = armcpu_switchMode(cpu, SYS);
		c.mov(oldmode, SYS);
		auto ctx = c.addCall(imm_ptr(armcpu_switchMode), ASMJIT_CALL_CONV,
			FuncBuilder2<uint32_t, uint8_t *, uint8_t>());
		ctx->setArg(0, bb_cpu);
		ctx->setArg(1, oldmode);
		ctx->setRet(0, oldmode);
	}

	call_ldm_stm(adr, bitmask, store, dir);

	if (!bit15 || store)
	{
		//armcpu_switchMode(cpu, oldmode);
		auto ctx = c.addCall(imm_ptr(armcpu_switchMode), ASMJIT_CALL_CONV,
			FuncBuilder2<void, uint8_t *, uint8_t>());
		ctx->setArg(0, bb_cpu);
		ctx->setArg(1, oldmode);
	}
	else
		S_DST_R15;

	// FIXME
	if (writeback)
	{
		if (store || !(i & (1 << REG_POS(i, 16))))
			c.add(reg_pos_ptr(16), 4 * dir * pop);
		else
		{
			uint32_t bitlist = (~((2 << REG_POS(i, 16)) - 1)) & 0xFFFF;
			if (i & bitlist)
			{
				c.add(adr, 4 * dir * (pop - before));
				c.mov(reg_pos_ptr(16), adr);
			}
		}
	}

	emit_MMU_aluMemCycles(store ? 1 : 2, bb_cycles, pop);
	return 1;
}

static int OP_LDMIA2(uint32_t i) { return op_ldm_stm2(i, 0, +1, 0, 0); }
static int OP_LDMIB2(uint32_t i) { return op_ldm_stm2(i, 0, +1, 1, 0); }
static int OP_LDMDA2(uint32_t i) { return op_ldm_stm2(i, 0, -1, 0, 0); }
static int OP_LDMDB2(uint32_t i) { return op_ldm_stm2(i, 0, -1, 1, 0); }
static int OP_LDMIA2_W(uint32_t i) { return op_ldm_stm2(i, 0, +1, 0, 1); }
static int OP_LDMIB2_W(uint32_t i) { return op_ldm_stm2(i, 0, +1, 1, 1); }
static int OP_LDMDA2_W(uint32_t i) { return op_ldm_stm2(i, 0, -1, 0, 1); }
static int OP_LDMDB2_W(uint32_t i) { return op_ldm_stm2(i, 0, -1, 1, 1); }

static int OP_STMIA2(uint32_t i) { return op_ldm_stm2(i, 1, +1, 0, 0); }
static int OP_STMIB2(uint32_t i) { return op_ldm_stm2(i, 1, +1, 1, 0); }
static int OP_STMDA2(uint32_t i) { return op_ldm_stm2(i, 1, -1, 0, 0); }
static int OP_STMDB2(uint32_t i) { return op_ldm_stm2(i, 1, -1, 1, 0); }
static int OP_STMIA2_W(uint32_t i) { return op_ldm_stm2(i, 1, +1, 0, 1); }
static int OP_STMIB2_W(uint32_t i) { return op_ldm_stm2(i, 1, +1, 1, 1); }
static int OP_STMDA2_W(uint32_t i) { return op_ldm_stm2(i, 1, -1, 0, 1); }
static int OP_STMDB2_W(uint32_t i) { return op_ldm_stm2(i, 1, -1, 1, 1); }

// -----------------------------------------------------------------------------
//   Branch
// -----------------------------------------------------------------------------

static inline uint32_t SIGNEXTEND_11(uint32_t i) { return static_cast<uint32_t>((static_cast<int32_t>(i) << 21) >> 21); }
static inline uint32_t SIGNEXTEND_24(uint32_t i) { return static_cast<uint32_t>((static_cast<int32_t>(i) << 8) >> 8); }

static int op_b(uint32_t i, bool bl)
{
	uint32_t dst = bb_r15 + (SIGNEXTEND_24(i) << 2);
	if (CONDITION(i) == 0xF)
	{
		if (bl)
			dst += 2;
		c.or_(cpu_ptr_byte(CPSR, 0), 1 << 5);
	}
	if (bl || CONDITION(i) == 0xF)
		c.mov(reg_ptr(14), bb_next_instruction);

	c.mov(cpu_ptr(instruct_adr), dst);
	return 1;
}

static int OP_B(uint32_t i) { return op_b(i, 0); }
static int OP_BL(uint32_t i) { return op_b(i, 1); }

static int op_bx(Mem srcreg, bool blx, bool test_thumb)
{
	GpVar dst = c.newGpVar(kVarTypeInt32);
	c.mov(dst, srcreg);

	if (test_thumb)
	{
		GpVar mask = c.newGpVar(kVarTypeInt32);
		GpVar thumb = dst;
		dst = c.newGpVar(kVarTypeInt32);
		c.mov(dst, thumb);
		c.and_(thumb, 1);
		c.lea(mask, x86::ptr_abs(0xFFFFFFFC, thumb.r64(), 1));
		c.shl(thumb, 5);
		c.or_(cpu_ptr_byte(CPSR, 0), thumb.r8Lo());
		c.and_(dst, mask);
	}
	else
		c.and_(dst, 0xFFFFFFFC);

	if (blx)
		c.mov(reg_ptr(14), bb_next_instruction);
	c.mov(cpu_ptr(instruct_adr), dst);
	return 1;
}

// TODO: exeption when Rm=PC
static int OP_BX(uint32_t i) { return op_bx(reg_pos_ptr(0), 0, 1); }
static int OP_BLX_REG(uint32_t i) { return op_bx(reg_pos_ptr(0), 1, 1); }

// -----------------------------------------------------------------------------
//   CLZ
// -----------------------------------------------------------------------------
static int OP_CLZ(uint32_t i)
{
	GpVar res = c.newGpVar(kVarTypeInt32);
	c.mov(res, 0x3F);
	c.bsr(res, reg_pos_ptr(0));
	c.xor_(res, 0x1F);
	c.mov(reg_pos_ptr(12), res);

	return 1;
}

// -----------------------------------------------------------------------------
//   MCR / MRC
// -----------------------------------------------------------------------------
#define maskPrecalc \
{ \
	auto ctxM = c.addCall(imm_ptr(maskPrecalc), ASMJIT_CALL_CONV, FuncBuilder0<void>()); \
}
static int OP_MCR(uint32_t i)
{
	if (PROCNUM == ARMCPU_ARM7)
		return 0;

	uint32_t cpnum = REG_POS(i, 8);
	if (cpnum != 15)
	{
		// TODO - exception?
		printf("JIT: MCR P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated coprocessor)\n", cpnum, REG_POS(i, 12), REG_POS(i, 16), REG_POS(i, 0), (i >> 21) & 0x7, (i >> 5) & 0x7);
		return 2;
	}
	if (REG_POS(i, 12) == 15)
	{
		printf("JIT: MCR Rd=R15\n");
		return 2;
	}

	uint8_t CRn =  REG_POS(i, 16); // Cn
	uint8_t CRm =  REG_POS(i, 0); // Cm
	uint8_t opcode1 = (i >> 21) & 0x7; // opcode1
	uint8_t opcode2 = (i >> 5) & 0x7; // opcode2

	GpVar bb_cp15 = c.newGpVar(kVarTypeIntPtr);
	GpVar data = c.newGpVar(kVarTypeInt32);
	c.mov(data, reg_pos_ptr(12));
	c.mov(bb_cp15, reinterpret_cast<uintptr_t>(&cp15));

	bool bUnknown = false;
	switch (CRn)
	{
		case 1:
			if (!opcode1 && !opcode2 && !CRm)
			{
				GpVar tmp = c.newGpVar(kVarTypeInt32);
				// On the NDS bit0,2,7,12..19 are R/W, Bit3..6 are always set, all other bits are always zero.
				//MMU.ARM9_RW_MODE = BIT7(val);
				GpVar bb_mmu = c.newGpVar(kVarTypeIntPtr);
				c.mov(bb_mmu, reinterpret_cast<uintptr_t>(&MMU));
				Mem rwmode = mmu_ptr_byte(ARM9_RW_MODE);
				Mem ldtbit = cpu_ptr_byte(LDTBit, 0);
				c.test(data, 1 << 7);
				c.setnz(rwmode);
				//cpu->intVector = 0xFFFF0000 * (BIT13(val));
				GpVar vec = c.newGpVar(kVarTypeInt32);
				c.mov(tmp, 0xFFFF0000);
				c.xor_(vec, vec);
				c.test(data, 1 << 13);
				c.cmovnz(vec, tmp);
				c.mov(cpu_ptr(intVector), vec);
				//cpu->LDTBit = !BIT15(val); //TBit
				c.test(data, 1 << 15);
				c.setz(ldtbit);
				//ctrl = (val & 0x000FF085) | 0x00000078;
				c.and_(data, 0x000FF085);
				c.or_(data, 0x00000078);
				c.mov(cp15_ptr(ctrl), data);
				break;
			}
			bUnknown = true;
			break;
		case 2:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 0:
						// DCConfig = val;
						c.mov(cp15_ptr(DCConfig), data);
						break;
					case 1:
						// ICConfig = val;
						c.mov(cp15_ptr(ICConfig), data);
						break;
					default:
						bUnknown = true;
						break;
				}
				break;
			}
			bUnknown = true;
			break;
		case 3:
			if (!opcode1 && !opcode2 && !CRm)
			{
				//writeBuffCtrl = val;
				c.mov(cp15_ptr(writeBuffCtrl), data);
				break;
			}
			bUnknown = true;
			break;
		case 5:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 2:
						//DaccessPerm = val;
						c.mov(cp15_ptr(DaccessPerm), data);
						maskPrecalc;
						break;
					case 3:
						//IaccessPerm = val;
						c.mov(cp15_ptr(IaccessPerm), data);
						maskPrecalc;
						break;
					default:
						bUnknown = true;
						break;
				}
			}
			bUnknown = true;
			break;
		case 6:
			if (!opcode1 && !opcode2)
			{
				switch (CRm)
				{
					case 0:
						//protectBaseSize0 = val;
						c.mov(cp15_ptr(protectBaseSize0), data);
						maskPrecalc;
						break;
					case 1:
						//protectBaseSize1 = val;
						c.mov(cp15_ptr(protectBaseSize1), data);
						maskPrecalc;
						break;
					case 2:
						//protectBaseSize2 = val;
						c.mov(cp15_ptr(protectBaseSize2), data);
						maskPrecalc;
						break;
					case 3:
						//protectBaseSize3 = val;
						c.mov(cp15_ptr(protectBaseSize3), data);
						maskPrecalc;
						break;
					case 4:
						//protectBaseSize4 = val;
						c.mov(cp15_ptr(protectBaseSize4), data);
						maskPrecalc;
						break;
					case 5:
						//protectBaseSize5 = val;
						c.mov(cp15_ptr(protectBaseSize5), data);
						maskPrecalc;
						break;
					case 6:
						//protectBaseSize6 = val;
						c.mov(cp15_ptr(protectBaseSize6), data);
						maskPrecalc;
						break;
					case 7:
						//protectBaseSize7 = val;
						c.mov(cp15_ptr(protectBaseSize7), data);
						maskPrecalc;
						break;
					default:
						bUnknown = true;
						break;
				}
			}
			bUnknown = true;
			break;
		case 7:
			if (!CRm && !opcode1 && opcode2 == 4)
			{
				//CP15wait4IRQ;
				c.mov(cpu_ptr(waitIRQ), true);
				c.mov(cpu_ptr(halt_IE_and_IF), true);
				//IME set deliberately omitted: only SWI sets IME to 1
				break;
			}
			bUnknown = true;
			break;
		case 9:
			if (!opcode1)
			{
				switch (CRm)
				{
					case 0:
						switch (opcode2)
						{
							case 0:
								//DcacheLock = val;
								c.mov(cp15_ptr(DcacheLock), data);
								break;
							case 1:
								//IcacheLock = val;
								c.mov(cp15_ptr(IcacheLock), data);
								break;
							default:
								bUnknown = true;
								break;
						}
					case 1:
						switch (opcode2)
						{
							case 0:
							{
								//MMU.DTCMRegion = DTCMRegion = val & 0x0FFFF000;
								c.and_(data, 0x0FFFF000);
								GpVar bb_mmu = c.newGpVar(kVarTypeIntPtr);
								c.mov(bb_mmu, reinterpret_cast<uintptr_t>(&MMU));
								c.mov(mmu_ptr(DTCMRegion), data);
								c.mov(cp15_ptr(DTCMRegion), data);
								break;
							}
							case 1:
							{
								//ITCMRegion = val;
								//ITCM base is not writeable!
								GpVar bb_mmu = c.newGpVar(kVarTypeIntPtr);
								c.mov(bb_mmu, reinterpret_cast<uintptr_t>(&MMU));
								c.mov(mmu_ptr(ITCMRegion), 0);
								c.mov(cp15_ptr(ITCMRegion), data);
								break;
							}
							default:
								bUnknown = true;
								break;
						}
				}
				break;
			}
			bUnknown = true;
			break;
		default:
			bUnknown = true;
	}

	if (bUnknown)
	{
		//printf("Unknown MCR command: MRC P15, 0, R%i, C%i, C%i, %i, %i\n", REG_POS(i, 12), CRn, CRm, opcode1, opcode2);
		return 1;
	}

	return 1;
}

static int OP_MRC(uint32_t i)
{
	if (PROCNUM == ARMCPU_ARM7)
		return 0;

	uint32_t cpnum = REG_POS(i, 8);
	if (cpnum != 15)
	{
		printf("MRC P%i, 0, R%i, C%i, C%i, %i, %i (don't allocated coprocessor)\n", cpnum, REG_POS(i, 12), REG_POS(i, 16), REG_POS(i, 0), (i>>21)&0x7, (i>>5)&0x7);
		return 2;
	}

	uint8_t CRn =  REG_POS(i, 16); // Cn
	uint8_t CRm =  REG_POS(i, 0); // Cm
	uint8_t opcode1 = (i >> 21) & 0x7; // opcode1
	uint8_t opcode2 = (i >> 5) & 0x7; // opcode2

	GpVar bb_cp15 = c.newGpVar(kVarTypeIntPtr);
	GpVar data = c.newGpVar(kVarTypeInt32);

	c.mov(bb_cp15, (uintptr_t)&cp15);

	bool bUnknown = false;
	switch (CRn)
	{
		case 0:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 1:
						// *R = cacheType;
						c.mov(data, cp15_ptr(cacheType));
						break;
					case 2:
						// *R = TCMSize;
						c.mov(data, cp15_ptr(TCMSize));
						break;
					default:		// FIXME
						// *R = IDCode;
						c.mov(data, cp15_ptr(IDCode));
						break;
				}
				break;
			}
			bUnknown = true;
			break;
		case 1:
			if (!opcode1 && !opcode2 && !CRm)
			{
				// *R = ctrl;
				c.mov(data, cp15_ptr(ctrl));
				break;
			}
			bUnknown = true;
			break;
		case 2:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 0:
						// *R = DCConfig;
						c.mov(data, cp15_ptr(DCConfig));
						break;
					case 1:
						// *R = ICConfig;
						c.mov(data, cp15_ptr(ICConfig));
						break;
					default:
						bUnknown = true;
						break;
				}
				break;
			}
			bUnknown = true;
			break;
		case 3:
			if (!opcode1 && !opcode2 && !CRm)
			{
				// *R = writeBuffCtrl;
				c.mov(data, cp15_ptr(writeBuffCtrl));
				break;
			}
			bUnknown = true;
			break;
		case 5:
			if (!opcode1 && !CRm)
			{
				switch (opcode2)
				{
					case 2:
						// *R = DaccessPerm;
						c.mov(data, cp15_ptr(DaccessPerm));
						break;
					case 3:
						// *R = IaccessPerm;
						c.mov(data, cp15_ptr(IaccessPerm));
						break;
					default:
						bUnknown = true;
						break;
				}
				break;
			}
			bUnknown = true;
			break;
		case 6:
			if (!opcode1 && !opcode2)
			{
				switch(CRm)
				{
					case 0:
						// *R = protectBaseSize0;
						c.mov(data, cp15_ptr(protectBaseSize0));
						break;
					case 1:
						// *R = protectBaseSize1;
						c.mov(data, cp15_ptr(protectBaseSize1));
						break;
					case 2:
						// *R = protectBaseSize2;
						c.mov(data, cp15_ptr(protectBaseSize2));
						break;
					case 3:
						// *R = protectBaseSize3;
						c.mov(data, cp15_ptr(protectBaseSize3));
						break;
					case 4:
						// *R = protectBaseSize4;
						c.mov(data, cp15_ptr(protectBaseSize4));
						break;
					case 5:
						// *R = protectBaseSize5;
						c.mov(data, cp15_ptr(protectBaseSize5));
						break;
					case 6:
						// *R = protectBaseSize6;
						c.mov(data, cp15_ptr(protectBaseSize6));
						break;
					case 7:
						// *R = protectBaseSize7;
						c.mov(data, cp15_ptr(protectBaseSize7));
						break;
					default:
						bUnknown = true;
						break;
				}
				break;
			}
			bUnknown = true;
			break;
		case 9:
			if (!opcode1)
			{
				switch (CRm)
				{
					case 0:
						switch (opcode2)
						{
							case 0:
								//*R = DcacheLock;
								c.mov(data, cp15_ptr(DcacheLock));
								break;
							case 1:
								//*R = IcacheLock;
								c.mov(data, cp15_ptr(IcacheLock));
								break;
							default:
								bUnknown = true;
								break;
						}
					case 1:
						switch (opcode2)
						{
							case 0:
								//*R = DTCMRegion;
								c.mov(data, cp15_ptr(DTCMRegion));
								break;
							case 1:
								//*R = ITCMRegion;
								c.mov(data, cp15_ptr(ITCMRegion));
								break;
							default:
								bUnknown = true;
								break;
						}
				}
				break;
			}
			bUnknown = true;
			break;
		default:
			bUnknown = true;
	}

	if (bUnknown)
	{
		//printf("Unknown MRC command: MRC P15, 0, R%i, C%i, C%i, %i, %i\n", REG_POS(i, 12), CRn, CRm, opcode1, opcode2);
		return 1;
	}

	if (REG_POS(i, 12) == 15) // set NZCV
	{
		//CPSR.bits.N = BIT31(data);
		//CPSR.bits.Z = BIT30(data);
		//CPSR.bits.C = BIT29(data);
		//CPSR.bits.V = BIT28(data);
		c.and_(data, 0xF0000000);
		c.and_(cpu_ptr(CPSR), 0x0FFFFFFF);
		c.or_(cpu_ptr(CPSR), data);
	}
	else
		c.mov(reg_pos_ptr(12), data);

	return 1;
}

uint32_t op_swi(uint8_t swinum)
{
	if (cpu->swi_tab)
	{
#if defined(_M_X64) || defined(__x86_64__)
		// TODO:
		return 0;
#else
		auto ctx = c.addCall(imm_ptr(ARM_swi_tab[PROCNUM][swinum]), ASMJIT_CALL_CONV, FuncBuilder0<uint32_t>());
		ctx->setRet(0, bb_cycles);
		c.add(bb_cycles, 3);
		return 1;
#endif
	}

	GpVar oldCPSR = c.newGpVar(kVarTypeInt32);
	GpVar mode = c.newGpVar(kVarTypeInt32);
	Mem CPSR = cpu_ptr(CPSR.val);
	JIT_COMMENT("store CPSR to x86 stack");
	c.mov(oldCPSR, CPSR);
	JIT_COMMENT("enter SVC mode");
	c.mov(mode, imm(SVC));
	auto ctx = c.addCall(imm_ptr(armcpu_switchMode), ASMJIT_CALL_CONV, FuncBuilder2<void, void *, uint8_t>());
	ctx->setArg(0, bb_cpu);
	ctx->setArg(1, mode);
	c.unuse(mode);
	JIT_COMMENT("store next instruction address to R14");
	c.mov(reg_ptr(14), bb_next_instruction);
	JIT_COMMENT("save old CPSR as new SPSR");
	c.mov(cpu_ptr(SPSR.val), oldCPSR);
	JIT_COMMENT("CPSR: clear T, set I");
	GpVar _cpsr = c.newGpVar(kVarTypeInt32);
	c.mov(_cpsr, CPSR);
	c.and_(_cpsr, ~(1 << 5)); /* clear T */
	c.or_(_cpsr, 1 << 7); /* set I */
	c.mov(CPSR, _cpsr);
	c.unuse(_cpsr);
	JIT_COMMENT("set next instruction");
	c.mov(cpu_ptr(next_instruction), imm(cpu->intVector + 0x08));

	return 1;
}

static int OP_SWI(uint32_t i) { return op_swi((i >> 16) & 0x1F); }

// -----------------------------------------------------------------------------
//   BKPT
// -----------------------------------------------------------------------------
static int OP_BKPT(uint32_t i) { printf("JIT: unimplemented OP_BKPT\n"); return 0; }

// -----------------------------------------------------------------------------
//   THUMB
// -----------------------------------------------------------------------------
#define OP_SHIFTS_IMM(x86inst) \
	GpVar rcf = c.newGpVar(kVarTypeInt32); \
	uint8_t cf_change = 1; \
	uint32_t rhs = (i >> 6) & 0x1F; \
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3)) \
		c.x86inst(reg_pos_thumb(0), rhs); \
	else \
	{ \
		GpVar lhs = c.newGpVar(kVarTypeInt32); \
		c.mov(lhs, reg_pos_thumb(3)); \
		c.x86inst(lhs, rhs); \
		c.mov(reg_pos_thumb(0), lhs); \
		c.unuse(lhs); \
	} \
	c.setc(rcf.r8Lo()); \
	SET_NZC; \
	return 1;

#define OP_SHIFTS_REG(x86inst, bit) \
	uint8_t cf_change = 1; \
	GpVar imm = c.newGpVar(kVarTypeIntPtr); \
	GpVar rcf = c.newGpVar(kVarTypeInt32); \
	Label __eq32 = c.newLabel(); \
	Label __ls32 = c.newLabel(); \
	Label __zero = c.newLabel(); \
	Label __done = c.newLabel(); \
\
	c.mov(imm, reg_pos_thumb(3)); \
	c.and_(imm, 0xFF); \
	c.jz(__zero); \
	c.cmp(imm, 32); \
	c.jl(__ls32); \
	c.je(__eq32); \
	/* imm > 32 */ \
	c.mov(reg_pos_thumb(0), 0); \
	SET_NZC_SHIFTS_ZERO(0); \
	c.jmp(__done); \
	/* imm == 32 */ \
	c.bind(__eq32); \
	c.test(reg_pos_thumb(0), 1 << bit); \
	c.setnz(rcf.r8Lo()); \
	c.mov(reg_pos_thumb(0), 0); \
	SET_NZC_SHIFTS_ZERO(1); \
	c.jmp(__done); \
	/* imm == 0 */ \
	c.bind(__zero); \
	c.cmp(reg_pos_thumb(0), 0); \
	SET_NZ(0); \
	c.jmp(__done); \
	/* imm < 32 */ \
	c.bind(__ls32); \
	c.x86inst(reg_pos_thumb(0), imm); \
	c.setc(rcf.r8Lo()); \
	SET_NZC; \
	c.bind(__done); \
	return 1;

#define OP_LOGIC(x86inst, _conv) \
	GpVar rhs = c.newGpVar(kVarTypeInt32); \
	c.mov(rhs, reg_pos_thumb(3)); \
	if (_conv == 1) \
		c.not_(rhs); \
	c.x86inst(reg_pos_thumb(0), rhs); \
	SET_NZ(0); \
	return 1;

// -----------------------------------------------------------------------------
//   LSL / LSR / ASR / ROR
// -----------------------------------------------------------------------------
static int OP_LSL_0(uint32_t i)
{
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3))
		c.cmp(reg_pos_thumb(0), 0);
	else
	{
		GpVar rhs = c.newGpVar(kVarTypeInt32);
		c.mov(rhs, reg_pos_thumb(3));
		c.mov(reg_pos_thumb(0), rhs);
		c.cmp(rhs, 0);
	}
	SET_NZ(0);
	return 1;
}
static int OP_LSL(uint32_t i) { OP_SHIFTS_IMM(shl); }
static int OP_LSL_REG(uint32_t i) { OP_SHIFTS_REG(shl, 0); }
static int OP_LSR_0(uint32_t i)
{
	GpVar rcf = c.newGpVar(kVarTypeInt32);
	c.test(reg_pos_thumb(3), 1 << 31);
	c.setnz(rcf.r8Lo());
	SET_NZC_SHIFTS_ZERO(1);
	c.mov(reg_pos_thumb(0), 0);
	return 1;
}
static int OP_LSR(uint32_t i) { OP_SHIFTS_IMM(shr); }
static int OP_LSR_REG(uint32_t i) { OP_SHIFTS_REG(shr, 31); }
static int OP_ASR_0(uint32_t i)
{
	uint8_t cf_change = 1;
	GpVar rcf = c.newGpVar(kVarTypeInt32);
	GpVar rhs = c.newGpVar(kVarTypeInt32);
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3))
		c.sar(reg_pos_thumb(0), 31);
	else
	{
		c.mov(rhs, reg_pos_thumb(3));
		c.sar(rhs, 31);
		c.mov(reg_pos_thumb(0), rhs);
	}
	c.sets(rcf.r8Lo());
	SET_NZC;
	return 1;
}
static int OP_ASR(uint32_t i) { OP_SHIFTS_IMM(sar); }
static int OP_ASR_REG(uint32_t i)
{
	uint8_t cf_change = 1;
	Label __gr0 = c.newLabel();
	Label __lt32 = c.newLabel();
	Label __done = c.newLabel();
	Label __setFlags = c.newLabel();
	GpVar imm = c.newGpVar(kVarTypeIntPtr);
	GpVar rcf = c.newGpVar(kVarTypeInt32);
	c.mov(imm, reg_pos_thumb(3));
	c.and_(imm, 0xFF);
	c.jnz(__gr0);
	/* imm == 0 */
	c.cmp(reg_pos_thumb(0), 0);
	SET_NZ(0);
	c.jmp(__done);
	/* imm > 0 */
	c.bind(__gr0);
	c.cmp(imm, 32);
	c.jl(__lt32);
	/* imm > 31 */
	c.sar(reg_pos_thumb(0), 31);
	c.sets(rcf.r8Lo());
	c.jmp(__setFlags);
	/* imm < 32 */
	c.bind(__lt32);
	c.sar(reg_pos_thumb(0), imm);
	c.setc(rcf.r8Lo());
	c.bind(__setFlags);
	SET_NZC;
	c.bind(__done);
	return 1;
}

// -----------------------------------------------------------------------------
//   ROR
// -----------------------------------------------------------------------------
static int OP_ROR_REG(uint32_t i)
{
	uint8_t cf_change = 1;
	GpVar imm = c.newGpVar(kVarTypeIntPtr);
	GpVar rcf = c.newGpVar(kVarTypeInt32);
	Label __zero = c.newLabel();
	Label __zero_1F = c.newLabel();
	Label __done = c.newLabel();

	c.mov(imm, reg_pos_thumb(3));
	c.and_(imm, 0xFF);
	c.jz(__zero);
	c.and_(imm, 0x1F);
	c.jz(__zero_1F);
	c.ror(reg_pos_thumb(0), imm);
	c.setc(rcf.r8Lo());
	SET_NZC;
	c.jmp(__done);
	/* imm & 0x1F == 0 */
	c.bind(__zero_1F);
	c.cmp(reg_pos_thumb(0), 0);
	c.sets(rcf.r8Lo());
	SET_NZC;
	c.jmp(__done);
	/* imm == 0 */
	c.bind(__zero);
	c.cmp(reg_pos_thumb(0), 0);
	SET_NZ(0);
	c.bind(__done);

	return 1;
}

// -----------------------------------------------------------------------------
//   AND / ORR / EOR / BIC
// -----------------------------------------------------------------------------
static int OP_AND(uint32_t i) { OP_LOGIC(and_, 0); }
static int OP_ORR(uint32_t i) { OP_LOGIC(or_,  0); }
static int OP_EOR(uint32_t i) { OP_LOGIC(xor_, 0); }
static int OP_BIC(uint32_t i) { OP_LOGIC(and_, 1); }

// -----------------------------------------------------------------------------
//   NEG
// -----------------------------------------------------------------------------
static int OP_NEG(uint32_t i)
{
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3))
		c.neg(reg_pos_thumb(0));
	else
	{
		GpVar tmp = c.newGpVar(kVarTypeInt32);
		c.mov(tmp, reg_pos_thumb(3));
		c.neg(tmp);
		c.mov(reg_pos_thumb(0), tmp);
	}
	SET_NZCV(1);
	return 1;
}

//-----------------------------------------------------------------------------
//   ADD
//-----------------------------------------------------------------------------
static int OP_ADD_IMM3(uint32_t i)
{
	uint32_t imm3 = (i >> 6) & 0x07;

	if (!imm3) // mov 2
	{
		GpVar tmp = c.newGpVar(kVarTypeInt32);
		c.mov(tmp, reg_pos_thumb(3));
		c.mov(reg_pos_thumb(0), tmp);
		c.cmp(tmp, 0);
		SET_NZ(1);
		return 1;
	}
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3))
		c.add(reg_pos_thumb(0), imm3);
	else
	{
		GpVar tmp = c.newGpVar(kVarTypeInt32);
		c.mov(tmp, reg_pos_thumb(3));
		c.add(tmp, imm3);
		c.mov(reg_pos_thumb(0), tmp);
	}
	SET_NZCV(0);
	return 1;
}
static int OP_ADD_IMM8(uint32_t i)
{
	c.add(reg_pos_thumb(8), (i & 0xFF));
	SET_NZCV(0);

	return 1;
}
static int OP_ADD_REG(uint32_t i)
{
	//cpu->R[REG_NUM(i, 0)] = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3))
	{
		GpVar tmp = c.newGpVar(kVarTypeInt32);
		c.mov(tmp, reg_pos_thumb(6));
		c.add(reg_pos_thumb(0), tmp);
	}
	else
	{
		if (_REG_NUM(i, 0) == _REG_NUM(i, 6))
		{
			GpVar tmp = c.newGpVar(kVarTypeInt32);
			c.mov(tmp, reg_pos_thumb(3));
			c.add(reg_pos_thumb(0), tmp);
		}
		else
		{
			GpVar tmp = c.newGpVar(kVarTypeInt32);
			c.mov(tmp, reg_pos_thumb(3));
			c.add(tmp, reg_pos_thumb(6));
			c.mov(reg_pos_thumb(0), tmp);
		}
	}
	SET_NZCV(0);
	return 1;
}
static int OP_ADD_SPE(uint32_t i)
{
	uint32_t Rd = _REG_NUM(i, 0) | ((i >> 4) & 8);
	//cpu->R[Rd] += cpu->R[REG_POS(i, 3)];
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_ptr(Rd));
	c.add(tmp, reg_pos_ptr(3));
	c.mov(reg_ptr(Rd), tmp);

	if (Rd == 15)
		c.mov(cpu_ptr(next_instruction), tmp);

	return 1;
}

static int OP_ADD_2PC(uint32_t i)
{
	uint32_t imm = (i & 0xFF) << 2;
	c.mov(reg_pos_thumb(8), (bb_r15 & 0xFFFFFFFC) + imm);
	return 1;
}

static int OP_ADD_2SP(uint32_t i)
{
	uint32_t imm = (i & 0xFF) << 2;
	//cpu->R[REG_NUM(i, 8)] = cpu->R[13] + ((i&0xFF)<<2);
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_ptr(13));
	if (imm)
		c.add(tmp, imm);
	c.mov(reg_pos_thumb(8), tmp);

	return 1;
}

// -----------------------------------------------------------------------------
//   SUB
// -----------------------------------------------------------------------------
static int OP_SUB_IMM3(uint32_t i)
{
	uint32_t imm3 = (i >> 6) & 0x07;

	// cpu->R[REG_NUM(i, 0)] = cpu->R[REG_NUM(i, 3)] - imm3;
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3))
		c.sub(reg_pos_thumb(0), imm3);
	else
	{
		GpVar tmp = c.newGpVar(kVarTypeInt32);
		c.mov(tmp, reg_pos_thumb(3));
		c.sub(tmp, imm3);
		c.mov(reg_pos_thumb(0), tmp);
	}
	SET_NZCV(1);
	return 1;
}
static int OP_SUB_IMM8(uint32_t i)
{
	//cpu->R[REG_NUM(i, 8)] -= imm8;
	c.sub(reg_pos_thumb(8), i & 0xFF);
	SET_NZCV(1);
	return 1;
}
static int OP_SUB_REG(uint32_t i)
{
	//cpu->R[REG_NUM(i, 0)] = cpu->R[REG_NUM(i, 3)] - cpu->R[REG_NUM(i, 6)];
	if (_REG_NUM(i, 0) == _REG_NUM(i, 3))
	{
		GpVar tmp = c.newGpVar(kVarTypeInt32);
		c.mov(tmp, reg_pos_thumb(6));
		c.sub(reg_pos_thumb(0), tmp);
	}
	else
	{
		GpVar tmp = c.newGpVar(kVarTypeInt32);
		c.mov(tmp, reg_pos_thumb(3));
		c.sub(tmp, reg_pos_thumb(6));
		c.mov(reg_pos_thumb(0), tmp);
	}
	SET_NZCV(1);
	return 1;
}

// -----------------------------------------------------------------------------
//   ADC
// -----------------------------------------------------------------------------
static int OP_ADC_REG(uint32_t i)
{
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_thumb(3));
	GET_CARRY(0);
	c.adc(reg_pos_thumb(0), tmp);
	SET_NZCV(0);
	return 1;
}

// -----------------------------------------------------------------------------
//   SBC
// -----------------------------------------------------------------------------
static int OP_SBC_REG(uint32_t i)
{
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_thumb(3));
	GET_CARRY(1);
	c.sbb(reg_pos_thumb(0), tmp);
	SET_NZCV(1);
	return 1;
}

// -----------------------------------------------------------------------------
//   MOV / MVN
// -----------------------------------------------------------------------------
static int OP_MOV_IMM8(uint32_t i)
{
	c.mov(reg_pos_thumb(8), i & 0xFF);
	c.cmp(reg_pos_thumb(8), 0);
	SET_NZ(0);
	return 1;
}

static int OP_MOV_SPE(uint32_t i)
{
	uint32_t Rd = _REG_NUM(i, 0) | ((i >> 4) & 8);
	//cpu->R[Rd] = cpu->R[REG_POS(i, 3)];
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_ptr(3));
	c.mov(reg_ptr(Rd), tmp);
	if (Rd == 15)
	{
		c.mov(cpu_ptr(next_instruction), tmp);
		bb_constant_cycles += 2;
	}

	return 1;
}

static int OP_MVN(uint32_t i)
{
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_thumb(3));
	c.not_(tmp);
	c.cmp(tmp, 0);
	c.mov(reg_pos_thumb(0), tmp);
	SET_NZ(0);
	return 1;
}

// -----------------------------------------------------------------------------
//   MUL
// -----------------------------------------------------------------------------
static int OP_MUL_REG(uint32_t i)
{
	GpVar lhs = c.newGpVar(kVarTypeInt32);
	c.mov(lhs, reg_pos_thumb(0));
	c.imul(lhs, reg_pos_thumb(3));
	c.cmp(lhs, 0);
	c.mov(reg_pos_thumb(0), lhs);
	SET_NZ(0);
	if (PROCNUM == ARMCPU_ARM7)
		c.mov(bb_cycles, 4);
	else
		MUL_Mxx_END(lhs, 0, 1);
	return 1;
}

// -----------------------------------------------------------------------------
//   CMP / CMN
// -----------------------------------------------------------------------------
static int OP_CMP_IMM8(uint32_t i)
{
	c.cmp(reg_pos_thumb(8), i & 0xFF);
	SET_NZCV(1);
	return 1;
}

static int OP_CMP(uint32_t i)
{
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_thumb(3));
	c.cmp(reg_pos_thumb(0), tmp);
	SET_NZCV(1);
	return 1;
}

static int OP_CMP_SPE(uint32_t i)
{
	uint32_t Rn = (i & 7) | ((i >> 4) & 8);
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_ptr(3));
	c.cmp(reg_ptr(Rn), tmp);
	SET_NZCV(1);
	return 1;
}

static int OP_CMN(uint32_t i)
{
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_thumb(0));
	c.add(tmp, reg_pos_thumb(3));
	SET_NZCV(0);
	return 1;
}

// -----------------------------------------------------------------------------
//   TST
// -----------------------------------------------------------------------------
static int OP_TST(uint32_t i)
{
	GpVar tmp = c.newGpVar(kVarTypeInt32);
	c.mov(tmp, reg_pos_thumb(3));
	c.test(reg_pos_thumb(0), tmp);
	SET_NZ(0);
	return 1;
}

// -----------------------------------------------------------------------------
//   STR / LDR / STRB / LDRB
// -----------------------------------------------------------------------------
#define STR_THUMB(mem_op, offset) \
	GpVar addr = c.newGpVar(kVarTypeInt32); \
	GpVar data = c.newGpVar(kVarTypeInt32); \
	uint32_t adr_first = cpu->R[_REG_NUM(i, 3)]; \
\
	c.mov(addr, reg_pos_thumb(3)); \
	if ((offset) != -1) \
	{ \
		if ((offset)) \
		{ \
			c.add(addr, static_cast<uint32_t>((offset))); \
			adr_first += static_cast<uint32_t>((offset)); \
		} \
	} \
	else \
	{ \
		c.add(addr, reg_pos_thumb(6)); \
		adr_first += cpu->R[_REG_NUM(i, 6)]; \
	} \
	c.mov(data, reg_pos_thumb(0)); \
	auto ctx = c.addCall(imm_ptr(mem_op##_tab[PROCNUM][classify_adr(adr_first, 1)]), ASMJIT_CALL_CONV, \
		FuncBuilder2<void, uint32_t, uint32_t>()); \
	ctx->setArg(0, addr); \
	ctx->setArg(1, data); \
	ctx->setRet(0, bb_cycles); \
	return 1;

#define LDR_THUMB(mem_op, offset) \
	GpVar addr = c.newGpVar(kVarTypeInt32); \
	GpVar data = c.newGpVar(kVarTypeIntPtr); \
	uint32_t adr_first = cpu->R[_REG_NUM(i, 3)]; \
\
	c.mov(addr, reg_pos_thumb(3)); \
	if ((offset) != -1) \
	{ \
		if ((offset)) \
		{ \
			c.add(addr, static_cast<uint32_t>((offset))); \
			adr_first += static_cast<uint32_t>((offset)); \
		} \
	} \
	else \
	{ \
		c.add(addr, reg_pos_thumb(6)); \
		adr_first += cpu->R[_REG_NUM(i, 6)]; \
	} \
	c.lea(data, reg_pos_thumb(0)); \
	auto ctx = c.addCall(imm_ptr(mem_op##_tab[PROCNUM][classify_adr(adr_first, 0)]), ASMJIT_CALL_CONV, \
		FuncBuilder2<void, uint32_t, uint32_t *>()); \
	ctx->setArg(0, addr); \
	ctx->setArg(1, data); \
	ctx->setRet(0, bb_cycles); \
	return 1;

static int OP_STRB_IMM_OFF(uint32_t i) { STR_THUMB(STRB, (i >> 6) & 0x1F); }
static int OP_LDRB_IMM_OFF(uint32_t i) { LDR_THUMB(LDRB, (i >> 6) & 0x1F); }
static int OP_STRB_REG_OFF(uint32_t i) { STR_THUMB(STRB, -1); }
static int OP_LDRB_REG_OFF(uint32_t i) { LDR_THUMB(LDRB, -1); }
static int OP_LDRSB_REG_OFF(uint32_t i) { LDR_THUMB(LDRSB, -1); }

static int OP_STRH_IMM_OFF(uint32_t i) { STR_THUMB(STRH, (i >> 5) & 0x3E); }
static int OP_LDRH_IMM_OFF(uint32_t i) { LDR_THUMB(LDRH, (i >> 5) & 0x3E); }
static int OP_STRH_REG_OFF(uint32_t i) { STR_THUMB(STRH, -1); }
static int OP_LDRH_REG_OFF(uint32_t i) { LDR_THUMB(LDRH, -1); }
static int OP_LDRSH_REG_OFF(uint32_t i) { LDR_THUMB(LDRSH, -1); }

static int OP_STR_IMM_OFF(uint32_t i) { STR_THUMB(STR, (i >> 4) & 0x7C); }
static int OP_LDR_IMM_OFF(uint32_t i) { LDR_THUMB(LDR, (i >> 4) & 0x7C); } // FIXME: tempValue = (tempValue>>adr) | (tempValue<<(32-adr));
static int OP_STR_REG_OFF(uint32_t i) { STR_THUMB(STR, -1); }
static int OP_LDR_REG_OFF(uint32_t i) { LDR_THUMB(LDR, -1); }

static int OP_STR_SPREL(uint32_t i)
{
	uint32_t imm = (i & 0xFF) << 2;
	uint32_t adr_first = cpu->R[13] + imm;

	GpVar addr = c.newGpVar(kVarTypeInt32);
	c.mov(addr, reg_ptr(13));
	if (imm)
		c.add(addr, imm);
	GpVar data = c.newGpVar(kVarTypeInt32);
	c.mov(data, reg_pos_thumb(8));
	auto ctx = c.addCall(imm_ptr(STR_tab[PROCNUM][classify_adr(adr_first, 1)]), ASMJIT_CALL_CONV,
		FuncBuilder2<void, uint32_t, uint32_t>());
	ctx->setArg(0, addr);
	ctx->setArg(1, data);
	ctx->setRet(0, bb_cycles);
	return 1;
}

static int OP_LDR_SPREL(uint32_t i)
{
	uint32_t imm = (i & 0xFF) << 2;
	uint32_t adr_first = cpu->R[13] + imm;

	GpVar addr = c.newGpVar(kVarTypeInt32);
	c.mov(addr, reg_ptr(13));
	if (imm)
		c.add(addr, imm);
	GpVar data = c.newGpVar(kVarTypeIntPtr);
	c.lea(data, reg_pos_thumb(8));
	auto ctx = c.addCall(imm_ptr(LDR_tab[PROCNUM][classify_adr(adr_first, 0)]), ASMJIT_CALL_CONV,
		FuncBuilder2<void, uint32_t, uint32_t *>());
	ctx->setArg(0, addr);
	ctx->setArg(1, data);
	ctx->setRet(0, bb_cycles);
	return 1;
}

static int OP_LDR_PCREL(uint32_t i)
{
	uint32_t imm = (i & 0xFF) << 2;
	uint32_t adr_first = (bb_r15 & 0xFFFFFFFC) + imm;
	GpVar addr = c.newGpVar(kVarTypeInt32);
	GpVar data = c.newGpVar(kVarTypeIntPtr);
	c.mov(addr, adr_first);
	c.lea(data, reg_pos_thumb(8));
	auto ctx = c.addCall(imm_ptr(LDR_tab[PROCNUM][classify_adr(adr_first, 0)]), ASMJIT_CALL_CONV,
		FuncBuilder2<void, uint32_t, uint32_t *>());
	ctx->setArg(0, addr);
	ctx->setArg(1, data);
	ctx->setRet(0, bb_cycles);
	return 1;
}

// -----------------------------------------------------------------------------
//   STMIA / LDMIA
// -----------------------------------------------------------------------------
static int op_ldm_stm_thumb(uint32_t i, bool store)
{
	uint32_t bitmask = i & 0xFF;
	uint32_t pop = popcount(bitmask);

	//if (BIT_N(i, _REG_NUM(i, 8)))
	//	printf("WARNING - %sIA with Rb in Rlist (THUMB)\n", store?"STM":"LDM");

	GpVar adr = c.newGpVar(kVarTypeInt32);
	c.mov(adr, reg_pos_thumb(8));

	call_ldm_stm(adr, bitmask, store, 1);

	// ARM_REF:	THUMB: Causes base register write-back, and is not optional
	// ARM_REF:	If the base register <Rn> is specified in <registers>, the final value of <Rn> is the loaded value
	//			(not the written-back value).
	if (store)
		c.add(reg_pos_thumb(8), 4 * pop);
	else
	{
		if (!BIT_N(i, _REG_NUM(i, 8)))
			c.add(reg_pos_thumb(8), 4 * pop);
	}

	emit_MMU_aluMemCycles(store ? 2 : 3, bb_cycles, pop);
	return 1;
}

static int OP_LDMIA_THUMB(uint32_t i) { return op_ldm_stm_thumb(i, 0); }
static int OP_STMIA_THUMB(uint32_t i) { return op_ldm_stm_thumb(i, 1); }

// -----------------------------------------------------------------------------
//   Adjust SP
// -----------------------------------------------------------------------------
static int OP_ADJUST_P_SP(uint32_t i) { c.add(reg_ptr(13), (i & 0x7F) << 2); return 1; }
static int OP_ADJUST_M_SP(uint32_t i) { c.sub(reg_ptr(13), (i & 0x7F) << 2); return 1; }

// -----------------------------------------------------------------------------
//   PUSH / POP
// -----------------------------------------------------------------------------
static int op_push_pop(uint32_t i, bool store, bool pc_lr)
{
	uint32_t bitmask = i & 0xFF;
	bitmask |= pc_lr << (store ? 14 : 15);
	uint32_t pop = popcount(bitmask);
	int dir = store ? -1 : 1;

	GpVar adr = c.newGpVar(kVarTypeInt32);
	c.mov(adr, reg_ptr(13));
	if (store)
		c.sub(adr, 4);

	call_ldm_stm(adr, bitmask, store, dir);

	if (pc_lr && !store)
		op_bx_thumb(reg_ptr(15), 0, PROCNUM == ARMCPU_ARM9);
	c.add(reg_ptr(13), 4 * dir * pop);

	emit_MMU_aluMemCycles(store ? (pc_lr ? 4 : 3) : (pc_lr ? 5 : 2), bb_cycles, pop);
	return 1;
}

static int OP_PUSH(uint32_t i) { return op_push_pop(i, 1, 0); }
static int OP_PUSH_LR(uint32_t i) { return op_push_pop(i, 1, 1); }
static int OP_POP(uint32_t i) { return op_push_pop(i, 0, 0); }
static int OP_POP_PC(uint32_t i)  { return op_push_pop(i, 0, 1); }

// -----------------------------------------------------------------------------
//   Branch
// -----------------------------------------------------------------------------
static int OP_B_COND(uint32_t i)
{
	Label skip = c.newLabel();

	uint32_t dst = bb_r15 + ((i & 0xFF) << 1);

	c.mov(cpu_ptr(instruct_adr), bb_next_instruction);

	emit_branch((i >> 8) & 0xF, skip);
	c.mov(cpu_ptr(instruct_adr), dst);
	c.add(bb_total_cycles, 2);
	c.bind(skip);

	return 1;
}

static int OP_B_UNCOND(uint32_t i)
{
	uint32_t dst = bb_r15 + (SIGNEXTEND_11(i) << 1);
	c.mov(cpu_ptr(instruct_adr), dst);
	return 1;
}

static int OP_BLX(uint32_t i)
{
	GpVar dst = c.newGpVar(kVarTypeInt32);
	c.mov(dst, reg_ptr(14));
	c.add(dst, (i & 0x7FF) << 1);
	c.and_(dst, 0xFFFFFFFC);
	c.mov(cpu_ptr(instruct_adr), dst);
	c.mov(reg_ptr(14), bb_next_instruction | 1);
	// reset T bit
	c.and_(cpu_ptr_byte(CPSR, 0), ~(1 << 5));
	return 1;
}

static int OP_BL_10( uint32_t i)
{
	uint32_t dst = bb_r15 + (SIGNEXTEND_11(i) << 12);
	c.mov(reg_ptr(14), dst);
	return 1;
}

static int OP_BL_11(uint32_t i)
{
	GpVar dst = c.newGpVar(kVarTypeInt32);
	c.mov(dst, reg_ptr(14));
	c.add(dst, (i & 0x7FF) << 1);
	c.mov(cpu_ptr(instruct_adr), dst);
	c.mov(reg_ptr(14), bb_next_instruction | 1);
	return 1;
}

static int op_bx_thumb(Mem srcreg, bool blx, bool test_thumb)
{
	GpVar dst = c.newGpVar(kVarTypeInt32);
	GpVar thumb = c.newGpVar(kVarTypeInt32);
	c.mov(dst, srcreg);
	c.mov(thumb, dst); // * cpu->CPSR.bits.T = BIT0(Rm);
	c.and_(thumb, 1); // *
	if (blx)
		c.mov(reg_ptr(14), bb_next_instruction | 1);
	if (test_thumb)
	{
		GpVar mask = c.newGpVar(kVarTypeInt32);
		c.lea(mask, x86::ptr_abs(0xFFFFFFFC, thumb.r64(), 1));
		c.and_(dst, mask);
	}
	else
		c.and_(dst, 0xFFFFFFFE);

	GpVar tmp = c.newGpVar(kVarTypeInt32); // *
	c.mov(tmp, cpu_ptr_byte(CPSR, 0)); // *
	c.and_(tmp, ~(1 << 5)); // *
	c.shl(thumb, 5); // *
	c.or_(tmp, thumb); // *
	c.mov(cpu_ptr_byte(CPSR, 0), tmp.r8Lo()); // ******************************

	c.mov(cpu_ptr(instruct_adr), dst);
	return 1;
}

static int OP_BX_THUMB(uint32_t i) { if (REG_POS(i, 3) == 15) c.mov(reg_ptr(15), bb_r15); return op_bx_thumb(reg_pos_ptr(3), 0, 0); }
static int OP_BLX_THUMB(uint32_t i) { return op_bx_thumb(reg_pos_ptr(3), 1, 1); }

static int OP_SWI_THUMB(uint32_t i) { return op_swi(i & 0x1F); }

// -----------------------------------------------------------------------------
//   Unimplemented; fall back to the C versions
// -----------------------------------------------------------------------------

#define OP_UND           nullptr
#define OP_LDREX         nullptr
#define OP_STREX         nullptr
#define OP_LDC_P_IMM_OFF nullptr
#define OP_LDC_M_IMM_OFF nullptr
#define OP_LDC_P_PREIND  nullptr
#define OP_LDC_M_PREIND  nullptr
#define OP_LDC_P_POSTIND nullptr
#define OP_LDC_M_POSTIND nullptr
#define OP_LDC_OPTION    nullptr
#define OP_STC_P_IMM_OFF nullptr
#define OP_STC_M_IMM_OFF nullptr
#define OP_STC_P_PREIND  nullptr
#define OP_STC_M_PREIND  nullptr
#define OP_STC_P_POSTIND nullptr
#define OP_STC_M_POSTIND nullptr
#define OP_STC_OPTION    nullptr
#define OP_CDP           nullptr

#define OP_UND_THUMB     nullptr
#define OP_BKPT_THUMB    nullptr

// -----------------------------------------------------------------------------
//   Dispatch table
// -----------------------------------------------------------------------------

typedef int (*ArmOpCompiler)(uint32_t);
static const ArmOpCompiler arm_instruction_compilers[4096] =
{
#define TABDECL(x) x
#include "instruction_tabdef.inc"
#undef TABDECL
};

static const ArmOpCompiler thumb_instruction_compilers[1024] =
{
#define TABDECL(x) x
#include "thumb_tabdef.inc"
#undef TABDECL
};

//-----------------------------------------------------------------------------
//   Generic instruction wrapper
//-----------------------------------------------------------------------------

template<int PROCNUM, int thumb> static uint32_t FASTCALL OP_DECODE()
{
	uint32_t cycles;
	uint32_t adr = cpu->instruct_adr;
	if (thumb)
	{
		cpu->next_instruction = adr + 2;
		cpu->R[15] = adr + 4;
		uint32_t opcode = _MMU_read16<PROCNUM, MMU_AT_CODE>(adr);
		_armlog(PROCNUM, adr, opcode);
		cycles = thumb_instructions_set[PROCNUM][opcode >> 6](opcode);
	}
	else
	{
		cpu->next_instruction = adr + 4;
		cpu->R[15] = adr + 8;
		uint32_t opcode = _MMU_read32<PROCNUM, MMU_AT_CODE>(adr);
		_armlog(PROCNUM, adr, opcode);
		if (CONDITION(opcode) == 0xE || TEST_COND(CONDITION(opcode), CODE(opcode), cpu->CPSR))
			cycles = arm_instructions_set[PROCNUM][INSTRUCTION_INDEX(opcode)](opcode);
		else
			cycles = 1;
	}
	cpu->instruct_adr = cpu->next_instruction;
	return cycles;
}

static const ArmOpCompiled op_decode[][2] = { { OP_DECODE<0, 0>, OP_DECODE<0, 1> }, { OP_DECODE<1, 0>, OP_DECODE<1, 1> } };

// -----------------------------------------------------------------------------
//   Compiler
// -----------------------------------------------------------------------------

static uint32_t instr_attributes(uint32_t opcode)
{
	return bb_thumb ? thumb_attributes[opcode >> 6] : instruction_attributes[INSTRUCTION_INDEX(opcode)];
}

static bool instr_is_branch(uint32_t opcode)
{
	uint32_t x = instr_attributes(opcode);
	if (bb_thumb)
		return (x & BRANCH_ALWAYS) || ((x & BRANCH_POS0) && ((opcode & 7) | ((opcode >> 4) & 8)) == 15) || (x & BRANCH_SWI) || (x & JIT_BYPASS);
	else
		return (x & BRANCH_ALWAYS) || ((x & BRANCH_POS12) && REG_POS(opcode, 12) == 15) || ((x & BRANCH_LDM) && BIT15(opcode)) || (x & BRANCH_SWI) || (x & JIT_BYPASS);
}

static bool instr_uses_r15(uint32_t opcode)
{
	uint32_t x = instr_attributes(opcode);
	if (bb_thumb)
		return ((x & SRCREG_POS0) && ((opcode & 7) | ((opcode >> 4) & 8)) == 15) || ((x & SRCREG_POS3) && REG_POS(opcode, 3) == 15) || (x & JIT_BYPASS);
	else
		return ((x & SRCREG_POS0) && REG_POS(opcode, 0) == 15) || ((x & SRCREG_POS8) && REG_POS(opcode, 8) == 15) || ((x & SRCREG_POS12) && REG_POS(opcode, 12) == 15) ||
			((x & SRCREG_POS16) && REG_POS(opcode, 16) == 15) || ((x & SRCREG_STM) && BIT15(opcode)) || (x & JIT_BYPASS);
}

static bool instr_is_conditional(uint32_t opcode)
{
	if (bb_thumb)
		return false;

	return !(CONDITION(opcode) == 0xE || (CONDITION(opcode) == 0xF && CODE(opcode) == 5));
}

static int instr_cycles(uint32_t opcode)
{
	uint32_t x = instr_attributes(opcode);
	uint32_t c = x & INSTR_CYCLES_MASK;
	if (c == INSTR_CYCLES_VARIABLE)
	{
		if ((x & BRANCH_SWI) && !cpu->swi_tab)
			return 3;

		return 0;
	}
	if (instr_is_branch(opcode) && !(instr_attributes(opcode) & (BRANCH_ALWAYS | BRANCH_LDM)))
		c += 2;
	return c;
}

static bool instr_does_prefetch(uint32_t opcode)
{
	uint32_t x = instr_attributes(opcode);
	if(bb_thumb)
		return thumb_instruction_compilers[opcode >> 6] && (x & BRANCH_ALWAYS);
	else
		return instr_is_branch(opcode) && arm_instruction_compilers[INSTRUCTION_INDEX(opcode)] && ((x & BRANCH_ALWAYS) || (x & BRANCH_LDM));
}

static void sync_r15(uint32_t opcode, bool is_last, bool force)
{
	if (instr_does_prefetch(opcode))
	{
		assert(!instr_uses_r15(opcode));
		if (force)
		{
			JIT_COMMENT("sync_r15: force instruct_adr %08Xh (PREFETCH)", bb_adr);
			c.mov(cpu_ptr(instruct_adr), bb_next_instruction);
		}
	}
	else
	{
		if (force || (instr_attributes(opcode) & JIT_BYPASS) || (instr_attributes(opcode) & BRANCH_SWI) || (is_last && !instr_is_branch(opcode)))
		{
			JIT_COMMENT("sync_r15: next_instruction %08Xh - %s%s%s%s", bb_next_instruction,
				force ? " FORCE" : "",
				(instr_attributes(opcode) & JIT_BYPASS) ? " BYPASS" : "",
				(instr_attributes(opcode) & BRANCH_SWI) ? " SWI" : "",
				is_last && !instr_is_branch(opcode) ? " LAST" : ""
			);
			c.mov(cpu_ptr(next_instruction), bb_next_instruction);
		}
		if (instr_uses_r15(opcode))
		{
			JIT_COMMENT("sync_r15: R15 %08Xh (USES R15)", bb_r15);
			c.mov(reg_ptr(15), bb_r15);
		}
		if (instr_attributes(opcode) & JIT_BYPASS)
		{
			JIT_COMMENT("sync_r15: instruct_adr %08Xh (JIT_BYPASS)", bb_adr);
			c.mov(cpu_ptr(instruct_adr), bb_adr);
		}
	}
}

static void emit_branch(int cond, Label to)
{
	JIT_COMMENT("emit_branch cond %02X", cond);
	static const uint8_t cond_bit[] = { 0x40, 0x40, 0x20, 0x20, 0x80, 0x80, 0x10, 0x10 };
	if (cond < 8)
	{
		c.test(flags_ptr, cond_bit[cond]);
		(cond & 1) ? c.jnz(to) : c.jz(to);
	}
	else
	{
		GpVar x = c.newGpVar(kVarTypeIntPtr);
		c.movzx(x, flags_ptr);
		c.and_(x, 0xF0);
#if defined(_M_X64) || defined(__x86_64__)
		c.add(x, offsetof(armcpu_t,cond_table) + cond);
		c.test(x86::byte_ptr(bb_cpu, x), 1);
#else
		c.test(x86::byte_ptr_abs(reinterpret_cast<Ptr>(&arm_cond_table[0]) + cond, x, 0), 1);
#endif
		c.unuse(x);
		c.jz(to);
	}
}

static void emit_armop_call(uint32_t opcode)
{
	ArmOpCompiler fc = bb_thumb ? thumb_instruction_compilers[opcode >> 6] : arm_instruction_compilers[INSTRUCTION_INDEX(opcode)];
	if (fc && fc(opcode))
		return;

	JIT_COMMENT("call interpreter");
	GpVar arg = c.newGpVar(kVarTypeInt32);
	c.mov(arg, opcode);
	OpFunc f = bb_thumb ? thumb_instructions_set[PROCNUM][opcode >> 6] : arm_instructions_set[PROCNUM][INSTRUCTION_INDEX(opcode)];
	auto ctx = c.addCall(imm_ptr(f), ASMJIT_CALL_CONV, FuncBuilder1<uint32_t, uint32_t>());
	ctx->setArg(0, arg);
	ctx->setRet(0, bb_cycles);
}

static void _armlog(uint8_t proc, uint32_t addr, uint32_t opcode)
{
#if 0
#if 0
	fprintf(stderr, "\t\t;R0:%08X R1:%08X R2:%08X R3:%08X R4:%08X R5:%08X R6:%08X R7:%08X R8:%08X R9:%08X\n\t\t;R10:%08X R11:%08X R12:%08X R13:%08X R14:%08X R15:%08X| next %08X, N:%i Z:%i C:%i V:%i\n",
		cpu->R[0],  cpu->R[1],  cpu->R[2],  cpu->R[3],  cpu->R[4],  cpu->R[5],  cpu->R[6],  cpu->R[7],
		cpu->R[8],  cpu->R[9],  cpu->R[10],  cpu->R[11],  cpu->R[12],  cpu->R[13],  cpu->R[14],  cpu->R[15],
		cpu->next_instruction, cpu->CPSR.bits.N, cpu->CPSR.bits.Z, cpu->CPSR.bits.C, cpu->CPSR.bits.V);
#endif
	#define INDEX22(i) ((((i)>>16)&0xFF0)|(((i)>>4)&0xF))
	char dasmbuf[4096];
	if(cpu->CPSR.bits.T)
		des_thumb_instructions_set[((opcode)>>6)&1023](addr, opcode, dasmbuf);
	else
		des_arm_instructions_set[INDEX22(opcode)](addr, opcode, dasmbuf);
	#undef INDEX22
	fprintf(stderr, "%s%c %08X\t%08X \t%s\n", cpu->CPSR.bits.T?"THUMB":"ARM", proc?'7':'9', addr, opcode, dasmbuf);
#endif
}

template<int PROCNUM> static uint32_t compile_basicblock()
{
#if LOG_JIT
	bool has_variable_cycles = false;
#endif
	uint32_t interpreted_cycles = 0;
	uint32_t start_adr = cpu->instruct_adr;
	uint32_t opcode = 0;

	bb_thumb = cpu->CPSR.bits.T;
	bb_opcodesize = bb_thumb ? 2 : 4;

	if (!JIT_MAPPED(start_adr & 0x0FFFFFFF, PROCNUM))
	{
		printf("JIT: use unmapped memory address %08X\n", start_adr);
		execute = false;
		return 1;
	}

#if LOG_JIT
	fprintf(stderr, "adr %08Xh %s%c\n", start_adr, ARMPROC.CPSR.bits.T ? "THUMB":"ARM", PROCNUM?'7':'9');
#endif

	c.reset();
	c.addFunc(ASMJIT_CALL_CONV, FuncBuilder0<int>());
	c.getFunc()->setHint(kFuncHintNaked, true);
	c.getFunc()->setHint(kX86FuncHintPushPop, true);

	JIT_COMMENT("CPU ptr");
	bb_cpu = c.newGpVar(kVarTypeIntPtr);
	c.mov(bb_cpu, (uintptr_t)&ARMPROC);

	JIT_COMMENT("reset bb_total_cycles");
	bb_total_cycles = c.newGpVar(kVarTypeIntPtr);
	c.mov(bb_total_cycles, 0);

#if PROFILER_JIT_LEVEL > 0
	JIT_COMMENT("Profiler ptr");
	bb_profiler = c.newGpVar(kVarTypeIntPtr);
	c.mov(bb_profiler, reinterpret_cast<uintptr_t>(&profiler_counter[PROCNUM]));
#endif

	bb_constant_cycles = 0;
	for (uint32_t i = 0, bEndBlock = 0; !bEndBlock; ++i)
	{
		bb_adr = start_adr + (i * bb_opcodesize);
		if (bb_thumb)
			opcode = _MMU_read16<PROCNUM, MMU_AT_CODE>(bb_adr);
		else
			opcode = _MMU_read32<PROCNUM, MMU_AT_CODE>(bb_adr);

#if LOG_JIT
		char dasmbuf[1024] = {0};
		if (bb_thumb)
			des_thumb_instructions_set[opcode >> 6](bb_adr, opcode, dasmbuf);
		else
			des_arm_instructions_set[INSTRUCTION_INDEX(opcode)](bb_adr, opcode, dasmbuf);
		fprintf(stderr, "%08X\t%s\t\t; %s \n", bb_adr, dasmbuf, disassemble(opcode));
#endif

		uint32_t cycles = instr_cycles(opcode);

		bEndBlock = i >= CommonSettings.jit_max_block_size - 1 || instr_is_branch(opcode);

#if LOG_JIT
		if (instr_is_conditional(opcode) && cycles > 1 || !cycles)
			has_variable_cycles = true;
#endif
		bb_cycles = c.newGpVar(kVarTypeIntPtr);

		bb_constant_cycles += instr_is_conditional(opcode) ? 1 : cycles;

		JIT_COMMENT("%s (PC:%08X)", disassemble(opcode), bb_adr);

#if PROFILER_JIT_LEVEL > 0
		JIT_COMMENT("*** profiler - counter");
		if (bb_thumb)
			c.add(profiler_counter_thumb(opcode), 1);
		else
			c.add(profiler_counter_arm(opcode), 1);
#endif
		if (instr_is_conditional(opcode))
		{
			// 25% of conditional instructions are immediately followed by
			// another with the same condition, but merging them into a
			// single branch has negligible effect on speed.
			if (bEndBlock)
				sync_r15(opcode, 1, 1);
			Label skip = c.newLabel();
			emit_branch(CONDITION(opcode), skip);
			if (!bEndBlock)
				sync_r15(opcode, 0, 0);
			emit_armop_call(opcode);

			if (!cycles)
			{
				JIT_COMMENT("variable cycles");
				c.lea(bb_total_cycles, x86::ptr(bb_total_cycles.r64(), bb_cycles.r64(), 0));
			}
			c.bind(skip);
		}
		else
		{
			sync_r15(opcode, !!bEndBlock, false);
			emit_armop_call(opcode);
			if (!cycles)
			{
				JIT_COMMENT("variable cycles");
				c.lea(bb_total_cycles, x86::ptr(bb_total_cycles.r64(), bb_cycles.r64(), 0));
			}
		}
		interpreted_cycles += op_decode[PROCNUM][bb_thumb]();
	}

	if (!instr_does_prefetch(opcode))
	{
		JIT_COMMENT("!instr_does_prefetch: copy next_instruction (%08X) to instruct_adr (%08X)", cpu->next_instruction, cpu->instruct_adr);
		GpVar x = c.newGpVar(kVarTypeInt32);
		c.mov(x, cpu_ptr(next_instruction));
		c.mov(cpu_ptr(instruct_adr), x);
		c.unuse(x);
		//c.mov(cpu_ptr(instruct_adr), bb_adr);
		//c.mov(cpu_ptr(instruct_adr), bb_next_instruction);
	}

	JIT_COMMENT("total cycles (block)");

	if (bb_constant_cycles > 0)
		c.add(bb_total_cycles, bb_constant_cycles);

#if PROFILER_JIT_LEVEL > 1
	JIT_COMMENT("*** profiler - cycles");
	uint32_t padr = (start_adr & 0x07FFFFFE) >> 1;
	bb_profiler_entry = c.newGpVar(kVarTypeIntPtr);
	c.mov(bb_profiler_entry, reinterpret_cast<uintptr_t>(&profiler_entry[PROCNUM][padr]));
	c.add(X86Mem(bb_profiler_entry, offsetof(PROFILER_ENTRY, cycles), 4), bb_total_cycles);
	profiler_entry[PROCNUM][padr].addr = start_adr;
#endif

	c.ret(bb_total_cycles);
#if LOG_JIT
	fprintf(stderr, "cycles %d%s\n", bb_constant_cycles, has_variable_cycles ? " + variable" : "");
#endif
	c.endFunc();

	ArmOpCompiled f = static_cast<ArmOpCompiled>(c.make());
	if (c.getError())
	{
		fprintf(stderr, "JIT error: %s\n", ErrorUtil::asString(c.getError()));
		f = op_decode[PROCNUM][bb_thumb];
	}
#if LOG_JIT
	uintptr_t baddr = reinterpret_cast<uintptr_t>(f);
	fprintf(stderr, "Block address %08lX\n\n", baddr);
	fflush(stderr);
#endif

	JIT_COMPILED_FUNC(start_adr, PROCNUM) = reinterpret_cast<uintptr_t>(f);
	return interpreted_cycles;
}

template<int PROCNUM> uint32_t arm_jit_compile()
{
	*PROCNUM_ptr = PROCNUM;

	// prevent endless recompilation of self-modifying code, which would be a memleak since we only free code all at once.
	// also allows us to clear compiled_funcs[] while leaving it sparsely allocated, if the OS does memory overcommit.
	uint32_t adr = cpu->instruct_adr;
	uint32_t mask_adr = (adr & 0x07FFFFFE) >> 4;
	if (((recompile_counts[mask_adr >> 1] >> 4 * (mask_adr & 1)) & 0xF) > 8)
	{
		ArmOpCompiled f = op_decode[PROCNUM][cpu->CPSR.bits.T];
		JIT_COMPILED_FUNC(adr, PROCNUM) = reinterpret_cast<uintptr_t>(f);
		return f();
	}
	recompile_counts[mask_adr >> 1] += 1 << 4 * (mask_adr & 1);

	return compile_basicblock<PROCNUM>();
}

template uint32_t arm_jit_compile<0>();
template uint32_t arm_jit_compile<1>();

void arm_jit_reset(bool enable)
{
#if LOG_JIT
	c.setLogger(&logger);
#ifdef _WINDOWS
	freopen("\\desmume_jit.log", "w", stderr);
#endif
#endif
#ifdef HAVE_STATIC_CODE_BUFFER
	scratchptr = scratchpad;
#endif
	printf("CPU mode: %s\n", enable ? "JIT" : "Interpreter");

	if (enable)
	{
		printf("JIT max block size %d instruction(s)\n", CommonSettings.jit_max_block_size);
#ifdef MAPPED_JIT_FUNCS
		// these pointers are allocated by asmjit and need freeing
		#define JITFREE(x)  for (size_t iii = 0; iii < ARRAY_SIZE((x)); ++iii) if ((x)[iii]) runtime.getMemMgr()->release(reinterpret_cast<void *>((x)[iii])); memset((x), 0, sizeof((x)));
			JITFREE(JIT.MAIN_MEM);
			JITFREE(JIT.SWIRAM);
			JITFREE(JIT.ARM9_ITCM);
			JITFREE(JIT.ARM9_LCDC);
			JITFREE(JIT.ARM9_BIOS);
			JITFREE(JIT.ARM7_BIOS);
			JITFREE(JIT.ARM7_ERAM);
			JITFREE(JIT.ARM7_WIRAM);
			JITFREE(JIT.ARM7_WRAM);
		#undef JITFREE

		memset(recompile_counts, 0, sizeof(recompile_counts));
		init_jit_mem();
#else
		for (int i = 0; i < sizeof(recompile_counts) / 8; ++i)
			if (reinterpret_cast<uint64_t *>(recompile_counts)[i])
			{
				reinterpret_cast<uint64_t *>(recompile_counts)[i] = nullptr;
				memset(compiled_funcs + 128 * i, 0, 128 * sizeof(*compiled_funcs));
			}
#endif
	}

	c.reset();

#if PROFILER_JIT_LEVEL > 0
	reconstruct(&profiler_counter[0]);
	reconstruct(&profiler_counter[1]);
#if PROFILER_JIT_LEVEL > 1
	for (uint8_t t = 0; t < 2; ++t)
	{
		for (uint32_t i = 0; i < (1 << 26); ++i)
			memset(&profiler_entry[t][i], 0, sizeof(PROFILER_ENTRY));
	}
#endif
#endif
}

#if PROFILER_JIT_LEVEL > 0
static int pcmp(PROFILER_COUNTER_INFO *info1, PROFILER_COUNTER_INFO *info2)
{
	return info2->count - info1->count;
}

#if PROFILER_JIT_LEVEL > 1
static int pcmp_entry(PROFILER_ENTRY *info1, PROFILER_ENTRY *info2)
{
	return info1->cycles - info2->cycles;
}
#endif
#endif

void arm_jit_close()
{
#if PROFILER_JIT_LEVEL > 0
	printf("Generating profile report...");

	for (uint8_t proc = 0; proc < 2; ++proc)
	{
		extern GameInfo gameInfo;
		uint16_t last[2] = { 0 };

		auto arm_info = std::unique_ptr<PROFILER_COUNTER_INFO[]>(new PROFILER_COUNTER_INFO[4096]);
		auto thumb_info = std::unique_ptr<PROFILER_COUNTER_INFO[]>(new PROFILER_COUNTER_INFO[1024]);
		memset(&arm_info[0], 0, sizeof(PROFILER_COUNTER_INFO) * 4096);
		memset(&thumb_info[0], 0, sizeof(PROFILER_COUNTER_INFO) * 1024);

		// ARM
		last[0] = 0;
		for (uint16_t i = 0; i < 4096; ++i)
		{
			uint16_t t = 0;
			if (!profiler_counter[proc].arm_count[i])
				continue;

			for (t = 0; t < last[0]; ++t)
			{
				if (!strcmp(arm_instruction_names[i], arm_info[t].name))
				{
					arm_info[t].count += profiler_counter[proc].arm_count[i];
					break;
				}
			}
			if (t == last[0])
			{
				strcpy(arm_info[last[0]++].name, arm_instruction_names[i]);
				arm_info[t].count = profiler_counter[proc].arm_count[i];
			}
		}

		// THUMB
		last[1] = 0;
		for (uint16_t i = 0; i < 1024; ++i)
		{
			uint16_t t = 0;
			if (!profiler_counter[proc].thumb_count[i])
				continue;

			for (t = 0; t < last[1]; ++t)
				if (!strcmp(thumb_instruction_names[i], thumb_info[t].name))
				{
					thumb_info[t].count += profiler_counter[proc].thumb_count[i];
					break;
				}
			if (t == last[1])
			{
				strcpy(thumb_info[last[1]++].name, thumb_instruction_names[i]);
				thumb_info[t].count = profiler_counter[proc].thumb_count[i];
			}
		}

		std::qsort(&arm_info[0], last[0], sizeof(PROFILER_COUNTER_INFO), (int (*)(const void *, const void *))pcmp);
		std::qsort(&thumb_info[0], last[1], sizeof(PROFILER_COUNTER_INFO), (int (*)(const void *, const void *))pcmp);

		char buf[MAX_PATH] = { 0 };
		sprintf(buf, "\\desmume_jit%c_counter.profiler", !proc ? '9' : '7');
		FILE *fp = fopen(buf, "w");
		if (fp)
		{
			if (!gameInfo.isHomebrew)
			{
				fprintf(fp, "Name:   %s\n", gameInfo.ROMname);
				fprintf(fp, "Serial: %s\n", gameInfo.ROMserial);
			}
			else
				fprintf(fp, "Homebrew\n");
			fprintf(fp, "CPU: ARM%c\n\n", !proc ? '9' : '7');

			if (last[0])
			{
				fprintf(fp, "========================================== ARM ==========================================\n");
				for (int i = 0; i < last[0]; ++i)
					fprintf(fp, "%30s: %20ld\n", arm_info[i].name, arm_info[i].count);
				fprintf(fp, "\n");
			}

			if (last[1])
			{
				fprintf(fp, "========================================== THUMB ==========================================\n");
				for (int i = 0; i < last[1]; ++i)
					fprintf(fp, "%30s: %20ld\n", thumb_info[i].name, thumb_info[i].count);
				fprintf(fp, "\n");
			}

			fclose(fp);
		}

#if PROFILER_JIT_LEVEL > 1
		sprintf(buf, "\\desmume_jit%c_entry.profiler", !proc ? '9' : '7');
		fp = fopen(buf, "w");
		if (fp)
		{
			uint32_t count = 0;

			fprintf(fp, "Entrypoints (cycles):\n");
			auto tmp = std::unique_ptr<PROFILER_ENTRY[]>(new PROFILER_ENTRY[1 << 26]);
			memset(tmp, 0, sizeof(PROFILER_ENTRY) * (1 << 26));
			for (uint32_t i = 0; i < (1 << 26); ++i)
			{
				if (!profiler_entry[proc][i].cycles)
					continue;
				memcpy(&tmp[count++], &profiler_entry[proc][i], sizeof(PROFILER_ENTRY));
			}
			std::qsort(tmp, count, sizeof(PROFILER_ENTRY), (int (*)(const void *, const void *))pcmp_entry);
			if (!gameInfo.isHomebrew)
			{
				fprintf(fp, "Name:   %s\n", gameInfo.ROMname);
				fprintf(fp, "Serial: %s\n", gameInfo.ROMserial);
			}
			else
				fprintf(fp, "Homebrew\n");
			fprintf(fp, "CPU: ARM%c\n\n", !proc ? '9' : '7');

			while (count-- > 0)
				fprintf(fp, "%08X: %20ld\n", tmp[count].addr, tmp[count].cycles);

			fclose(fp);
		}
#endif
	}
	printf(" done.\n");
#endif
}
#endif // HAVE_JIT
