/*
must fix:
    callm
    chk
*/
/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */
/*
 *                                  MUSASHI
 *                                Version 3.32
 *
 * A portable Motorola M680x0 processor emulation engine.
 * Copyright Karl Stenerud.  All rights reserved.
 *
 * This code may be freely used for non-commercial purposes as long as this
 * copyright notice remains unaltered in the source code and any binary files
 * containing this code in compiled form.
 *
 * All other licensing terms must be negotiated with the author
 * (Karl Stenerud).
 *
 * The latest version of this code can be obtained at:
 * http://kstenerud.cjb.net
 */

/* Special thanks to Bart Trzynadlowski for his insight into the
 * undocumented features of this chip:
 *
 * http://dynarec.com/~bart/files/68knotes.txt
 */


/* Input file for m68kmake
 * -----------------------
 *
 * All sections begin with 80 X's in a row followed by an end-of-line
 * sequence.
 * After this, m68kmake will expect to find one of the following section
 * identifiers:
 *    M68KMAKE_PROTOTYPE_HEADER      - header for opcode handler prototypes
 *    M68KMAKE_PROTOTYPE_FOOTER      - footer for opcode handler prototypes
 *    M68KMAKE_TABLE_HEADER          - header for opcode handler jumptable
 *    M68KMAKE_TABLE_FOOTER          - footer for opcode handler jumptable
 *    M68KMAKE_TABLE_BODY            - the table itself
 *    M68KMAKE_OPCODE_HANDLER_HEADER - header for opcode handler implementation
 *    M68KMAKE_OPCODE_HANDLER_FOOTER - footer for opcode handler implementation
 *    M68KMAKE_OPCODE_HANDLER_BODY   - body section for opcode handler implementation
 *
 * NOTE: M68KMAKE_OPCODE_HANDLER_BODY must be last in the file and
 *       M68KMAKE_TABLE_BODY must be second last in the file.
 *
 * The M68KMAKE_OPHANDLER_BODY section contains the opcode handler
 * primitives themselves.  Each opcode handler begins with:
 *    M68KMAKE_OP(A, B, C, D)
 *
 * where A is the opcode handler name, B is the size of the operation,
 * C denotes any special processing mode, and D denotes a specific
 * addressing mode.
 * For C and D where nothing is specified, use "."
 *
 * Example:
 *     M68KMAKE_OP(abcd, 8, rr, .)   abcd, size 8, register to register, default EA
 *     M68KMAKE_OP(abcd, 8, mm, ax7) abcd, size 8, memory to memory, register X is A7
 *     M68KMAKE_OP(tst, 16, ., pcix) tst, size 16, PCIX addressing
 *
 * All opcode handler primitives end with a closing curly brace "}" at column 1
 *
 * NOTE: Do not place a M68KMAKE_OP() directive inside the opcode handler,
 *       and do not put a closing curly brace at column 1 unless it is
 *       marking the end of the handler!
 *
 * Inside the handler, m68kmake will recognize M68KMAKE_GET_OPER_xx_xx,
 * M68KMAKE_GET_EA_xx_xx, and M68KMAKE_CC directives, and create multiple
 * opcode handlers to handle variations in the opcode handler.
 * Note: M68KMAKE_CC will only be interpreted in condition code opcodes.
 * As well, M68KMAKE_GET_EA_xx_xx and M68KMAKE_GET_OPER_xx_xx will only
 * be interpreted on instructions where the corresponding table entry
 * specifies multiple effective addressing modes.
 * Example:
 * clr       32  .     .     0100001010......  A+-DXWL...  U U U   12   6   4
 *
 * This table entry says that the clr.l opcde has 7 variations (A+-DXWL).
 * It is run in user or supervisor mode for all CPUs, and uses 12 cycles for
 * 68000, 6 cycles for 68010, and 4 cycles for 68020.
 */

XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_PROTOTYPE_HEADER

#ifndef M68KOPS__HEADER
#define M68KOPS__HEADER

/* ======================================================================== */
/* ============================ OPCODE HANDLERS =========================== */
/* ======================================================================== */



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_PROTOTYPE_FOOTER


/* Build the opcode handler table */
void m68ki_build_opcode_table(void);

extern void (*m68ki_instruction_jump_table[0x10000])(m68ki_cpu_core *m68k); /* opcode handler jump table */
extern unsigned char m68ki_cycles[][0x10000];


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68KOPS__HEADER */



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_TABLE_HEADER

/* ======================================================================== */
/* ========================= OPCODE TABLE BUILDER ========================= */
/* ======================================================================== */

#include "m68kops.h"

#define NUM_CPU_TYPES 5

void  (*m68ki_instruction_jump_table[0x10000])(m68ki_cpu_core *m68k); /* opcode handler jump table */
unsigned char m68ki_cycles[NUM_CPU_TYPES][0x10000]; /* Cycles used by CPU type */

/* This is used to generate the opcode handler jump table */
typedef struct
{
	void (*opcode_handler)(m68ki_cpu_core *m68k);        /* handler function */
	unsigned int  mask;                  /* mask on opcode */
	unsigned int  match;                 /* what to match after masking */
	unsigned char cycles[NUM_CPU_TYPES]; /* cycles each cpu type takes */
} opcode_handler_struct;


/* Opcode handler table */
static const opcode_handler_struct m68k_opcode_handler_table[] =
{
/*   function                      mask    match    000  010  020  040 */



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_TABLE_FOOTER

	{0, 0, 0, {0, 0, 0, 0, 0}}
};


/* Build the opcode handler jump table */
void m68ki_build_opcode_table(void)
{
	const opcode_handler_struct *ostruct;
	int instr;
	int i;
	int j;
	int k;

	for(i = 0; i < 0x10000; i++)
	{
		/* default to illegal */
		m68ki_instruction_jump_table[i] = m68k_op_illegal;
		for(k=0;k<NUM_CPU_TYPES;k++)
			m68ki_cycles[k][i] = 0;
	}

	ostruct = m68k_opcode_handler_table;
	while(ostruct->mask != 0xff00)
	{
		for(i = 0;i < 0x10000;i++)
		{
			if((i & ostruct->mask) == ostruct->match)
			{
				m68ki_instruction_jump_table[i] = ostruct->opcode_handler;
				for(k=0;k<NUM_CPU_TYPES;k++)
					m68ki_cycles[k][i] = ostruct->cycles[k];
			}
		}
		ostruct++;
	}
	while(ostruct->mask == 0xff00)
	{
		for(i = 0;i <= 0xff;i++)
		{
			m68ki_instruction_jump_table[ostruct->match | i] = ostruct->opcode_handler;
			for(k=0;k<NUM_CPU_TYPES;k++)
				m68ki_cycles[k][ostruct->match | i] = ostruct->cycles[k];
		}
		ostruct++;
	}
	while(ostruct->mask == 0xf1f8)
	{
		for(i = 0;i < 8;i++)
		{
			for(j = 0;j < 8;j++)
			{
				instr = ostruct->match | (i << 9) | j;
				m68ki_instruction_jump_table[instr] = ostruct->opcode_handler;
				for(k=0;k<NUM_CPU_TYPES;k++)
					m68ki_cycles[k][instr] = ostruct->cycles[k];
			}
		}
		ostruct++;
	}
	while(ostruct->mask == 0xfff0)
	{
		for(i = 0;i <= 0x0f;i++)
		{
			m68ki_instruction_jump_table[ostruct->match | i] = ostruct->opcode_handler;
			for(k=0;k<NUM_CPU_TYPES;k++)
				m68ki_cycles[k][ostruct->match | i] = ostruct->cycles[k];
		}
		ostruct++;
	}
	while(ostruct->mask == 0xf1ff)
	{
		for(i = 0;i <= 0x07;i++)
		{
			m68ki_instruction_jump_table[ostruct->match | (i << 9)] = ostruct->opcode_handler;
			for(k=0;k<NUM_CPU_TYPES;k++)
				m68ki_cycles[k][ostruct->match | (i << 9)] = ostruct->cycles[k];
		}
		ostruct++;
	}
	while(ostruct->mask == 0xfff8)
	{
		for(i = 0;i <= 0x07;i++)
		{
			m68ki_instruction_jump_table[ostruct->match | i] = ostruct->opcode_handler;
			for(k=0;k<NUM_CPU_TYPES;k++)
				m68ki_cycles[k][ostruct->match | i] = ostruct->cycles[k];
		}
		ostruct++;
	}
	while(ostruct->mask == 0xffff)
	{
		m68ki_instruction_jump_table[ostruct->match] = ostruct->opcode_handler;
		for(k=0;k<NUM_CPU_TYPES;k++)
			m68ki_cycles[k][ostruct->match] = ostruct->cycles[k];
		ostruct++;
	}
}


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_OPCODE_HANDLER_HEADER

#include "m68kcpu.h"
#include "mame.h"
extern void m68040_fpu_op0(m68ki_cpu_core *m68k);
extern void m68040_fpu_op1(m68ki_cpu_core *m68k);
extern void m68881_mmu_ops(m68ki_cpu_core *m68k);

/* ======================================================================== */
/* ========================= INSTRUCTION HANDLERS ========================= */
/* ======================================================================== */



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_OPCODE_HANDLER_FOOTER

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_TABLE_BODY
/*
The following table is arranged as follows:

name:        Opcode mnemonic

size:        Operation size

spec proc:   Special processing mode:
                 .:    normal
                 s:    static operand
                 r:    register operand
                 rr:   register to register
                 mm:   memory to memory
                 er:   effective address to register
                 re:   register to effective address
                 dd:   data register to data register
                 da:   data register to address register
                 aa:   address register to address register
                 cr:   control register to register
                 rc:   register to control register
                 toc:  to condition code register
                 tos:  to status register
                 tou:  to user stack pointer
                 frc:  from condition code register
                 frs:  from status register
                 fru:  from user stack pointer
                 * for move.x, the special processing mode is a specific
                   destination effective addressing mode.

spec ea:     Specific effective addressing mode:
                 .:    normal
                 i:    immediate
                 d:    data register
                 a:    address register
                 ai:   address register indirect
                 pi:   address register indirect with postincrement
                 pd:   address register indirect with predecrement
                 di:   address register indirect with displacement
                 ix:   address register indirect with index
                 aw:   absolute word address
                 al:   absolute long address
                 pcdi: program counter relative with displacement
                 pcix: program counter relative with index
                 a7:   register specified in instruction is A7
                 ax7:  register field X of instruction is A7
                 ay7:  register field Y of instruction is A7
                 axy7: register fields X and Y of instruction are A7

bit pattern: Pattern to recognize this opcode.  "." means don't care.

allowed ea:  List of allowed addressing modes:
                 .: not present
                 A: address register indirect
                 +: ARI (address register indirect) with postincrement
                 -: ARI with predecrement
                 D: ARI with displacement
                 X: ARI with index
                 W: absolute word address
                 L: absolute long address
                 d: program counter indirect with displacement
                 x: program counter indirect with index
                 I: immediate
mode:        CPU operating mode for each cpu type.  U = user or supervisor,
             S = supervisor only, "." = opcode not present.

cpu cycles:  Base number of cycles required to execute this opcode on the
             specified CPU type.
             Use "." if CPU does not have this opcode.
*/


              spec  spec                    allowed ea  mode       cpu cycles
name    size  proc   ea   bit pattern       A+-DXWLdxI  0 1 2 3 4  000 010 020 030 040  comments
======  ====  ====  ====  ================  ==========  = = = = =  === === === === === =============
M68KMAKE_TABLE_START
1010       0  .     .     1010............  ..........  U U U U U   4   4   4   4   4
1111       0  .     .     1111............  ..........  U U U U U   4   4   4   4   4
040fpu0   32  .     .     11110010........  ..........  . . . . U   .   .   .   .   0
040fpu1   32  .     .     11110011........  ..........  . . . . U   .   .   .   .   0
abcd       8  rr    .     1100...100000...  ..........  U U U U U   6   6   4   4   4
abcd       8  mm    ax7   1100111100001...  ..........  U U U U U  18  18  16  16  16
abcd       8  mm    ay7   1100...100001111  ..........  U U U U U  18  18  16  16  16
abcd       8  mm    axy7  1100111100001111  ..........  U U U U U  18  18  16  16  16
abcd       8  mm    .     1100...100001...  ..........  U U U U U  18  18  16  16  16
add        8  er    d     1101...000000...  ..........  U U U U U   4   4   2   2   2
add        8  er    .     1101...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
add       16  er    d     1101...001000...  ..........  U U U U U   4   4   2   2   2
add       16  er    a     1101...001001...  ..........  U U U U U   4   4   2   2   2
add       16  er    .     1101...001......  A+-DXWLdxI  U U U U U   4   4   2   2   2
add       32  er    d     1101...010000...  ..........  U U U U U   6   6   2   2   2
add       32  er    a     1101...010001...  ..........  U U U U U   6   6   2   2   2
add       32  er    .     1101...010......  A+-DXWLdxI  U U U U U   6   6   2   2   2
add        8  re    .     1101...100......  A+-DXWL...  U U U U U   8   8   4   4   4
add       16  re    .     1101...101......  A+-DXWL...  U U U U U   8   8   4   4   4
add       32  re    .     1101...110......  A+-DXWL...  U U U U U  12  12   4   4   4
adda      16  .     d     1101...011000...  ..........  U U U U U   8   8   2   2   2
adda      16  .     a     1101...011001...  ..........  U U U U U   8   8   2   2   2
adda      16  .     .     1101...011......  A+-DXWLdxI  U U U U U   8   8   2   2   2
adda      32  .     d     1101...111000...  ..........  U U U U U   6   6   2   2   2
adda      32  .     a     1101...111001...  ..........  U U U U U   6   6   2   2   2
adda      32  .     .     1101...111......  A+-DXWLdxI  U U U U U   6   6   2   2   2
addi       8  .     d     0000011000000...  ..........  U U U U U   8   8   2   2   2
addi       8  .     .     0000011000......  A+-DXWL...  U U U U U  12  12   4   4   4
addi      16  .     d     0000011001000...  ..........  U U U U U   8   8   2   2   2
addi      16  .     .     0000011001......  A+-DXWL...  U U U U U  12  12   4   4   4
addi      32  .     d     0000011010000...  ..........  U U U U U  16  14   2   2   2
addi      32  .     .     0000011010......  A+-DXWL...  U U U U U  20  20   4   4   4
addq       8  .     d     0101...000000...  ..........  U U U U U   4   4   2   2   2
addq       8  .     .     0101...000......  A+-DXWL...  U U U U U   8   8   4   4   4
addq      16  .     d     0101...001000...  ..........  U U U U U   4   4   2   2   2
addq      16  .     a     0101...001001...  ..........  U U U U U   4   4   2   2   2
addq      16  .     .     0101...001......  A+-DXWL...  U U U U U   8   8   4   4   4
addq      32  .     d     0101...010000...  ..........  U U U U U   8   8   2   2   2
addq      32  .     a     0101...010001...  ..........  U U U U U   8   8   2   2   2
addq      32  .     .     0101...010......  A+-DXWL...  U U U U U  12  12   4   4   4
addx       8  rr    .     1101...100000...  ..........  U U U U U   4   4   2   2   2
addx      16  rr    .     1101...101000...  ..........  U U U U U   4   4   2   2   2
addx      32  rr    .     1101...110000...  ..........  U U U U U   8   6   2   2   2
addx       8  mm    ax7   1101111100001...  ..........  U U U U U  18  18  12  12  12
addx       8  mm    ay7   1101...100001111  ..........  U U U U U  18  18  12  12  12
addx       8  mm    axy7  1101111100001111  ..........  U U U U U  18  18  12  12  12
addx       8  mm    .     1101...100001...  ..........  U U U U U  18  18  12  12  12
addx      16  mm    .     1101...101001...  ..........  U U U U U  18  18  12  12  12
addx      32  mm    .     1101...110001...  ..........  U U U U U  30  30  12  12  12
and        8  er    d     1100...000000...  ..........  U U U U U   4   4   2   2   2
and        8  er    .     1100...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
and       16  er    d     1100...001000...  ..........  U U U U U   4   4   2   2   2
and       16  er    .     1100...001......  A+-DXWLdxI  U U U U U   4   4   2   2   2
and       32  er    d     1100...010000...  ..........  U U U U U   6   6   2   2   2
and       32  er    .     1100...010......  A+-DXWLdxI  U U U U U   6   6   2   2   2
and        8  re    .     1100...100......  A+-DXWL...  U U U U U   8   8   4   4   4
and       16  re    .     1100...101......  A+-DXWL...  U U U U U   8   8   4   4   4
and       32  re    .     1100...110......  A+-DXWL...  U U U U U  12  12   4   4   4
andi      16  toc   .     0000001000111100  ..........  U U U U U  20  16  12  12  12
andi      16  tos   .     0000001001111100  ..........  S S S S S  20  16  12  12  12
andi       8  .     d     0000001000000...  ..........  U U U U U   8   8   2   2   2
andi       8  .     .     0000001000......  A+-DXWL...  U U U U U  12  12   4   4   4
andi      16  .     d     0000001001000...  ..........  U U U U U   8   8   2   2   2
andi      16  .     .     0000001001......  A+-DXWL...  U U U U U  12  12   4   4   4
andi      32  .     d     0000001010000...  ..........  U U U U U  14  14   2   2   2
andi      32  .     .     0000001010......  A+-DXWL...  U U U U U  20  20   4   4   4
asr        8  s     .     1110...000000...  ..........  U U U U U   6   6   6   6   6
asr       16  s     .     1110...001000...  ..........  U U U U U   6   6   6   6   6
asr       32  s     .     1110...010000...  ..........  U U U U U   8   8   6   6   6
asr        8  r     .     1110...000100...  ..........  U U U U U   6   6   6   6   6
asr       16  r     .     1110...001100...  ..........  U U U U U   6   6   6   6   6
asr       32  r     .     1110...010100...  ..........  U U U U U   8   8   6   6   6
asr       16  .     .     1110000011......  A+-DXWL...  U U U U U   8   8   5   5   5
asl        8  s     .     1110...100000...  ..........  U U U U U   6   6   8   8   8
asl       16  s     .     1110...101000...  ..........  U U U U U   6   6   8   8   8
asl       32  s     .     1110...110000...  ..........  U U U U U   8   8   8   8   8
asl        8  r     .     1110...100100...  ..........  U U U U U   6   6   8   8   8
asl       16  r     .     1110...101100...  ..........  U U U U U   6   6   8   8   8
asl       32  r     .     1110...110100...  ..........  U U U U U   8   8   8   8   8
asl       16  .     .     1110000111......  A+-DXWL...  U U U U U   8   8   6   6   6
bcc        8  .     .     0110............  ..........  U U U U U  10  10   6   6   6
bcc       16  .     .     0110....00000000  ..........  U U U U U  10  10   6   6   6
bcc       32  .     .     0110....11111111  ..........  U U U U U  10  10   6   6   6
bchg       8  r     .     0000...101......  A+-DXWL...  U U U U U   8   8   4   4   4
bchg      32  r     d     0000...101000...  ..........  U U U U U   8   8   4   4   4
bchg       8  s     .     0000100001......  A+-DXWL...  U U U U U  12  12   4   4   4
bchg      32  s     d     0000100001000...  ..........  U U U U U  12  12   4   4   4
bclr       8  r     .     0000...110......  A+-DXWL...  U U U U U   8  10   4   4   4
bclr      32  r     d     0000...110000...  ..........  U U U U U  10  10   4   4   4
bclr       8  s     .     0000100010......  A+-DXWL...  U U U U U  12  12   4   4   4
bclr      32  s     d     0000100010000...  ..........  U U U U U  14  14   4   4   4
bfchg     32  .     d     1110101011000...  ..........  . . U U U   .   .  12  12  12  timing not quite correct
bfchg     32  .     .     1110101011......  A..DXWL...  . . U U U   .   .  20  20  20
bfclr     32  .     d     1110110011000...  ..........  . . U U U   .   .  12  12  12
bfclr     32  .     .     1110110011......  A..DXWL...  . . U U U   .   .  20  20  20
bfexts    32  .     d     1110101111000...  ..........  . . U U U   .   .   8   8   8
bfexts    32  .     .     1110101111......  A..DXWLdx.  . . U U U   .   .  15  15  15
bfextu    32  .     d     1110100111000...  ..........  . . U U U   .   .   8   8   8
bfextu    32  .     .     1110100111......  A..DXWLdx.  . . U U U   .   .  15  15  15
bfffo     32  .     d     1110110111000...  ..........  . . U U U   .   .  18  18  18
bfffo     32  .     .     1110110111......  A..DXWLdx.  . . U U U   .   .  28  28  28
bfins     32  .     d     1110111111000...  ..........  . . U U U   .   .  10  10  10
bfins     32  .     .     1110111111......  A..DXWL...  . . U U U   .   .  17  17  17
bfset     32  .     d     1110111011000...  ..........  . . U U U   .   .  12  12  12
bfset     32  .     .     1110111011......  A..DXWL...  . . U U U   .   .  20  20  20
bftst     32  .     d     1110100011000...  ..........  . . U U U   .   .   6   6   6
bftst     32  .     .     1110100011......  A..DXWLdx.  . . U U U   .   .  13  13  13
bkpt       0  .     .     0100100001001...  ..........  . U U U U   .  10  10  10  10
bra        8  .     .     01100000........  ..........  U U U U U  10  10  10  10  10
bra       16  .     .     0110000000000000  ..........  U U U U U  10  10  10  10  10
bra       32  .     .     0110000011111111  ..........  U U U U U  10  10  10  10  10
bset      32  r     d     0000...111000...  ..........  U U U U U   8   8   4   4   4
bset       8  r     .     0000...111......  A+-DXWL...  U U U U U   8   8   4   4   4
bset       8  s     .     0000100011......  A+-DXWL...  U U U U U  12  12   4   4   4
bset      32  s     d     0000100011000...  ..........  U U U U U  12  12   4   4   4
bsr        8  .     .     01100001........  ..........  U U U U U  18  18   7   7   7
bsr       16  .     .     0110000100000000  ..........  U U U U U  18  18   7   7   7
bsr       32  .     .     0110000111111111  ..........  U U U U U  18  18   7   7   7
btst       8  r     .     0000...100......  A+-DXWLdxI  U U U U U   4   4   4   4   4
btst      32  r     d     0000...100000...  ..........  U U U U U   6   6   4   4   4
btst       8  s     .     0000100000......  A+-DXWLdx.  U U U U U   8   8   4   4   4
btst      32  s     d     0000100000000...  ..........  U U U U U  10  10   4   4   4
callm     32  .     .     0000011011......  A..DXWLdx.  . . U U U   .   .  60  60  60  not properly emulated
cas        8  .     .     0000101011......  A+-DXWL...  . . U U U   .   .  12  12  12
cas       16  .     .     0000110011......  A+-DXWL...  . . U U U   .   .  12  12  12
cas       32  .     .     0000111011......  A+-DXWL...  . . U U U   .   .  12  12  12
cas2      16  .     .     0000110011111100  ..........  . . U U U   .   .  12  12  12
cas2      32  .     .     0000111011111100  ..........  . . U U U   .   .  12  12  12
chk       16  .     d     0100...110000...  ..........  U U U U U  10   8   8   8   8
chk       16  .     .     0100...110......  A+-DXWLdxI  U U U U U  10   8   8   8   8
chk       32  .     d     0100...100000...  ..........  . . U U U   .   .   8   8   8
chk       32  .     .     0100...100......  A+-DXWLdxI  . . U U U   .   .   8   8   8
chk2cmp2   8  .     pcdi  0000000011111010  ..........  . . U U U   .   .  23  23  23
chk2cmp2   8  .     pcix  0000000011111011  ..........  . . U U U   .   .  23  23  23
chk2cmp2   8  .     .     0000000011......  A..DXWL...  . . U U U   .   .  18  18  18
chk2cmp2  16  .     pcdi  0000001011111010  ..........  . . U U U   .   .  23  23  23
chk2cmp2  16  .     pcix  0000001011111011  ..........  . . U U U   .   .  23  23  23
chk2cmp2  16  .     .     0000001011......  A..DXWL...  . . U U U   .   .  18  18  18
chk2cmp2  32  .     pcdi  0000010011111010  ..........  . . U U U   .   .  23  23  23
chk2cmp2  32  .     pcix  0000010011111011  ..........  . . U U U   .   .  23  23  23
chk2cmp2  32  .     .     0000010011......  A..DXWL...  . . U U U   .   .  18  18  18
clr        8  .     d     0100001000000...  ..........  U U U U U   4   4   2   2   2
clr        8  .     .     0100001000......  A+-DXWL...  U U U U U   8   4   4   4   4
clr       16  .     d     0100001001000...  ..........  U U U U U   4   4   2   2   2
clr       16  .     .     0100001001......  A+-DXWL...  U U U U U   8   4   4   4   4
clr       32  .     d     0100001010000...  ..........  U U U U U   6   6   2   2   2
clr       32  .     .     0100001010......  A+-DXWL...  U U U U U  12   6   4   4   4
cmp        8  .     d     1011...000000...  ..........  U U U U U   4   4   2   2   2
cmp        8  .     .     1011...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
cmp       16  .     d     1011...001000...  ..........  U U U U U   4   4   2   2   2
cmp       16  .     a     1011...001001...  ..........  U U U U U   4   4   2   2   2
cmp       16  .     .     1011...001......  A+-DXWLdxI  U U U U U   4   4   2   2   2
cmp       32  .     d     1011...010000...  ..........  U U U U U   6   6   2   2   2
cmp       32  .     a     1011...010001...  ..........  U U U U U   6   6   2   2   2
cmp       32  .     .     1011...010......  A+-DXWLdxI  U U U U U   6   6   2   2   2
cmpa      16  .     d     1011...011000...  ..........  U U U U U   6   6   4   4   4
cmpa      16  .     a     1011...011001...  ..........  U U U U U   6   6   4   4   4
cmpa      16  .     .     1011...011......  A+-DXWLdxI  U U U U U   6   6   4   4   4
cmpa      32  .     d     1011...111000...  ..........  U U U U U   6   6   4   4   4
cmpa      32  .     a     1011...111001...  ..........  U U U U U   6   6   4   4   4
cmpa      32  .     .     1011...111......  A+-DXWLdxI  U U U U U   6   6   4   4   4
cmpi       8  .     d     0000110000000...  ..........  U U U U U   8   8   2   2   2
cmpi       8  .     .     0000110000......  A+-DXWL...  U U U U U   8   8   2   2   2
cmpi       8  .     pcdi  0000110000111010  ..........  . . U U U   .   .   7   7   7
cmpi       8  .     pcix  0000110000111011  ..........  . . U U U   .   .   9   9   9
cmpi      16  .     d     0000110001000...  ..........  U U U U U   8   8   2   2   2
cmpi      16  .     .     0000110001......  A+-DXWL...  U U U U U   8   8   2   2   2
cmpi      16  .     pcdi  0000110001111010  ..........  . . U U U   .   .   7   7   7
cmpi      16  .     pcix  0000110001111011  ..........  . . U U U   .   .   9   9   9
cmpi      32  .     d     0000110010000...  ..........  U U U U U  14  12   2   2   2
cmpi      32  .     .     0000110010......  A+-DXWL...  U U U U U  12  12   2   2   2
cmpi      32  .     pcdi  0000110010111010  ..........  . . U U U   .   .   7   7   7
cmpi      32  .     pcix  0000110010111011  ..........  . . U U U   .   .   9   9   9
cmpm       8  .     ax7   1011111100001...  ..........  U U U U U  12  12   9   9   9
cmpm       8  .     ay7   1011...100001111  ..........  U U U U U  12  12   9   9   9
cmpm       8  .     axy7  1011111100001111  ..........  U U U U U  12  12   9   9   9
cmpm       8  .     .     1011...100001...  ..........  U U U U U  12  12   9   9   9
cmpm      16  .     .     1011...101001...  ..........  U U U U U  12  12   9   9   9
cmpm      32  .     .     1011...110001...  ..........  U U U U U  20  20   9   9   9
cpbcc     32  .     .     1111...01.......  ..........  . . U U .   .   .   4   4   .  unemulated
cpdbcc    32  .     .     1111...001001...  ..........  . . U U .   .   .   4   4   .  unemulated
cpgen     32  .     .     1111...000......  ..........  . . U U .   .   .   4   4   .  unemulated
cpscc     32  .     .     1111...001......  ..........  . . U U .   .   .   4   4   .  unemulated
cptrapcc  32  .     .     1111...001111...  ..........  . . U U .   .   .   4   4   .  unemulated
dbt       16  .     .     0101000011001...  ..........  U U U U U  12  12   6   6   6
dbf       16  .     .     0101000111001...  ..........  U U U U U  12  12   6   6   6
dbcc      16  .     .     0101....11001...  ..........  U U U U U  12  12   6   6   6
divs      16  .     d     1000...111000...  ..........  U U U U U 158 122  56  56  56
divs      16  .     .     1000...111......  A+-DXWLdxI  U U U U U 158 122  56  56  56
divu      16  .     d     1000...011000...  ..........  U U U U U 140 108  44  44  44
divu      16  .     .     1000...011......  A+-DXWLdxI  U U U U U 140 108  44  44  44
divl      32  .     d     0100110001000...  ..........  . . U U U   .   .  84  84  84
divl      32  .     .     0100110001......  A+-DXWLdxI  . . U U U   .   .  84  84  84
eor        8  .     d     1011...100000...  ..........  U U U U U   4   4   2   2   2
eor        8  .     .     1011...100......  A+-DXWL...  U U U U U   8   8   4   4   4
eor       16  .     d     1011...101000...  ..........  U U U U U   4   4   2   2   2
eor       16  .     .     1011...101......  A+-DXWL...  U U U U U   8   8   4   4   4
eor       32  .     d     1011...110000...  ..........  U U U U U   8   6   2   2   2
eor       32  .     .     1011...110......  A+-DXWL...  U U U U U  12  12   4   4   4
eori      16  toc   .     0000101000111100  ..........  U U U U U  20  16  12  12  12
eori      16  tos   .     0000101001111100  ..........  S S S S S  20  16  12  12  12
eori       8  .     d     0000101000000...  ..........  U U U U U   8   8   2   2   2
eori       8  .     .     0000101000......  A+-DXWL...  U U U U U  12  12   4   4   4
eori      16  .     d     0000101001000...  ..........  U U U U U   8   8   2   2   2
eori      16  .     .     0000101001......  A+-DXWL...  U U U U U  12  12   4   4   4
eori      32  .     d     0000101010000...  ..........  U U U U U  16  14   2   2   2
eori      32  .     .     0000101010......  A+-DXWL...  U U U U U  20  20   4   4   4
exg       32  dd    .     1100...101000...  ..........  U U U U U   6   6   2   2   2
exg       32  aa    .     1100...101001...  ..........  U U U U U   6   6   2   2   2
exg       32  da    .     1100...110001...  ..........  U U U U U   6   6   2   2   2
ext       16  .     .     0100100010000...  ..........  U U U U U   4   4   4   4   4
ext       32  .     .     0100100011000...  ..........  U U U U U   4   4   4   4   4
extb      32  .     .     0100100111000...  ..........  . . U U U   .   .   4   4   4
illegal    0  .     .     0100101011111100  ..........  U U U U U   4   4   4   4   4
jmp       32  .     .     0100111011......  A..DXWLdx.  U U U U U   4   4   0   0   0
jsr       32  .     .     0100111010......  A..DXWLdx.  U U U U U  12  12   0   0   0
lea       32  .     .     0100...111......  A..DXWLdx.  U U U U U   0   0   2   2   2
link      16  .     a7    0100111001010111  ..........  U U U U U  16  16   5   5   5
link      16  .     .     0100111001010...  ..........  U U U U U  16  16   5   5   5
link      32  .     a7    0100100000001111  ..........  . . U U U   .   .   6   6   6
link      32  .     .     0100100000001...  ..........  . . U U U   .   .   6   6   6
lsr        8  s     .     1110...000001...  ..........  U U U U U   6   6   4   4   4
lsr       16  s     .     1110...001001...  ..........  U U U U U   6   6   4   4   4
lsr       32  s     .     1110...010001...  ..........  U U U U U   8   8   4   4   4
lsr        8  r     .     1110...000101...  ..........  U U U U U   6   6   6   6   6
lsr       16  r     .     1110...001101...  ..........  U U U U U   6   6   6   6   6
lsr       32  r     .     1110...010101...  ..........  U U U U U   8   8   6   6   6
lsr       16  .     .     1110001011......  A+-DXWL...  U U U U U   8   8   5   5   5
lsl        8  s     .     1110...100001...  ..........  U U U U U   6   6   4   4   4
lsl       16  s     .     1110...101001...  ..........  U U U U U   6   6   4   4   4
lsl       32  s     .     1110...110001...  ..........  U U U U U   8   8   4   4   4
lsl        8  r     .     1110...100101...  ..........  U U U U U   6   6   6   6   6
lsl       16  r     .     1110...101101...  ..........  U U U U U   6   6   6   6   6
lsl       32  r     .     1110...110101...  ..........  U U U U U   8   8   6   6   6
lsl       16  .     .     1110001111......  A+-DXWL...  U U U U U   8   8   5   5   5
move       8  d     d     0001...000000...  ..........  U U U U U   4   4   2   2   2
move       8  d     .     0001...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
move       8  ai    d     0001...010000...  ..........  U U U U U   8   8   4   4   4
move       8  ai    .     0001...010......  A+-DXWLdxI  U U U U U   8   8   4   4   4
move       8  pi    d     0001...011000...  ..........  U U U U U   8   8   4   4   4
move       8  pi    .     0001...011......  A+-DXWLdxI  U U U U U   8   8   4   4   4
move       8  pi7   d     0001111011000...  ..........  U U U U U   8   8   4   4   4
move       8  pi7   .     0001111011......  A+-DXWLdxI  U U U U U   8   8   4   4   4
move       8  pd    d     0001...100000...  ..........  U U U U U   8   8   5   5   5
move       8  pd    .     0001...100......  A+-DXWLdxI  U U U U U   8   8   5   5   5
move       8  pd7   d     0001111100000...  ..........  U U U U U   8   8   5   5   5
move       8  pd7   .     0001111100......  A+-DXWLdxI  U U U U U   8   8   5   5   5
move       8  di    d     0001...101000...  ..........  U U U U U  12  12   5   5   5
move       8  di    .     0001...101......  A+-DXWLdxI  U U U U U  12  12   5   5   5
move       8  ix    d     0001...110000...  ..........  U U U U U  14  14   7   7   7
move       8  ix    .     0001...110......  A+-DXWLdxI  U U U U U  14  14   7   7   7
move       8  aw    d     0001000111000...  ..........  U U U U U  12  12   4   4   4
move       8  aw    .     0001000111......  A+-DXWLdxI  U U U U U  12  12   4   4   4
move       8  al    d     0001001111000...  ..........  U U U U U  16  16   6   6   6
move       8  al    .     0001001111......  A+-DXWLdxI  U U U U U  16  16   6   6   6
move      16  d     d     0011...000000...  ..........  U U U U U   4   4   2   2   2
move      16  d     a     0011...000001...  ..........  U U U U U   4   4   2   2   2
move      16  d     .     0011...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
move      16  ai    d     0011...010000...  ..........  U U U U U   8   8   4   4   4
move      16  ai    a     0011...010001...  ..........  U U U U U   8   8   4   4   4
move      16  ai    .     0011...010......  A+-DXWLdxI  U U U U U   8   8   4   4   4
move      16  pi    d     0011...011000...  ..........  U U U U U   8   8   4   4   4
move      16  pi    a     0011...011001...  ..........  U U U U U   8   8   4   4   4
move      16  pi    .     0011...011......  A+-DXWLdxI  U U U U U   8   8   4   4   4
move      16  pd    d     0011...100000...  ..........  U U U U U   8   8   5   5   5
move      16  pd    a     0011...100001...  ..........  U U U U U   8   8   5   5   5
move      16  pd    .     0011...100......  A+-DXWLdxI  U U U U U   8   8   5   5   5
move      16  di    d     0011...101000...  ..........  U U U U U  12  12   5   5   5
move      16  di    a     0011...101001...  ..........  U U U U U  12  12   5   5   5
move      16  di    .     0011...101......  A+-DXWLdxI  U U U U U  12  12   5   5   5
move      16  ix    d     0011...110000...  ..........  U U U U U  14  14   7   7   7
move      16  ix    a     0011...110001...  ..........  U U U U U  14  14   7   7   7
move      16  ix    .     0011...110......  A+-DXWLdxI  U U U U U  14  14   7   7   7
move      16  aw    d     0011000111000...  ..........  U U U U U  12  12   4   4   4
move      16  aw    a     0011000111001...  ..........  U U U U U  12  12   4   4   4
move      16  aw    .     0011000111......  A+-DXWLdxI  U U U U U  12  12   4   4   4
move      16  al    d     0011001111000...  ..........  U U U U U  16  16   6   6   6
move      16  al    a     0011001111001...  ..........  U U U U U  16  16   6   6   6
move      16  al    .     0011001111......  A+-DXWLdxI  U U U U U  16  16   6   6   6
move      32  d     d     0010...000000...  ..........  U U U U U   4   4   2   2   2
move      32  d     a     0010...000001...  ..........  U U U U U   4   4   2   2   2
move      32  d     .     0010...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
move      32  ai    d     0010...010000...  ..........  U U U U U  12  12   4   4   4
move      32  ai    a     0010...010001...  ..........  U U U U U  12  12   4   4   4
move      32  ai    .     0010...010......  A+-DXWLdxI  U U U U U  12  12   4   4   4
move      32  pi    d     0010...011000...  ..........  U U U U U  12  12   4   4   4
move      32  pi    a     0010...011001...  ..........  U U U U U  12  12   4   4   4
move      32  pi    .     0010...011......  A+-DXWLdxI  U U U U U  12  12   4   4   4
move      32  pd    d     0010...100000...  ..........  U U U U U  12  14   5   5   5
move      32  pd    a     0010...100001...  ..........  U U U U U  12  14   5   5   5
move      32  pd    .     0010...100......  A+-DXWLdxI  U U U U U  12  14   5   5   5
move      32  di    d     0010...101000...  ..........  U U U U U  16  16   5   5   5
move      32  di    a     0010...101001...  ..........  U U U U U  16  16   5   5   5
move      32  di    .     0010...101......  A+-DXWLdxI  U U U U U  16  16   5   5   5
move      32  ix    d     0010...110000...  ..........  U U U U U  18  18   7   7   7
move      32  ix    a     0010...110001...  ..........  U U U U U  18  18   7   7   7
move      32  ix    .     0010...110......  A+-DXWLdxI  U U U U U  18  18   7   7   7
move      32  aw    d     0010000111000...  ..........  U U U U U  16  16   4   4   4
move      32  aw    a     0010000111001...  ..........  U U U U U  16  16   4   4   4
move      32  aw    .     0010000111......  A+-DXWLdxI  U U U U U  16  16   4   4   4
move      32  al    d     0010001111000...  ..........  U U U U U  20  20   6   6   6
move      32  al    a     0010001111001...  ..........  U U U U U  20  20   6   6   6
move      32  al    .     0010001111......  A+-DXWLdxI  U U U U U  20  20   6   6   6
movea     16  .     d     0011...001000...  ..........  U U U U U   4   4   2   2   2
movea     16  .     a     0011...001001...  ..........  U U U U U   4   4   2   2   2
movea     16  .     .     0011...001......  A+-DXWLdxI  U U U U U   4   4   2   2   2
movea     32  .     d     0010...001000...  ..........  U U U U U   4   4   2   2   2
movea     32  .     a     0010...001001...  ..........  U U U U U   4   4   2   2   2
movea     32  .     .     0010...001......  A+-DXWLdxI  U U U U U   4   4   2   2   2
move      16  frc   d     0100001011000...  ..........  . U U U U   .   4   4   4   4
move      16  frc   .     0100001011......  A+-DXWL...  . U U U U   .   8   4   4   4
move      16  toc   d     0100010011000...  ..........  U U U U U  12  12   4   4   4
move      16  toc   .     0100010011......  A+-DXWLdxI  U U U U U  12  12   4   4   4
move      16  frs   d     0100000011000...  ..........  U S S S S   6   4   8   8   8  U only for 000
move      16  frs   .     0100000011......  A+-DXWL...  U S S S S   8   8   8   8   8  U only for 000
move      16  tos   d     0100011011000...  ..........  S S S S S  12  12   8   8   8
move      16  tos   .     0100011011......  A+-DXWLdxI  S S S S S  12  12   8   8   8
move      32  fru   .     0100111001101...  ..........  S S S S S   4   6   2   2   2
move      32  tou   .     0100111001100...  ..........  S S S S S   4   6   2   2   2
movec     32  cr    .     0100111001111010  ..........  . S S S S   .  12   6   6   6
movec     32  rc    .     0100111001111011  ..........  . S S S S   .  10  12  12  12
movem     16  re    pd    0100100010100...  ..........  U U U U U   8   8   4   4   4
movem     16  re    .     0100100010......  A..DXWL...  U U U U U   8   8   4   4   4
movem     32  re    pd    0100100011100...  ..........  U U U U U   8   8   4   4   4
movem     32  re    .     0100100011......  A..DXWL...  U U U U U   8   8   4   4   4
movem     16  er    pi    0100110010011...  ..........  U U U U U  12  12   8   8   8
movem     16  er    pcdi  0100110010111010  ..........  U U U U U  16  16   9   9   9
movem     16  er    pcix  0100110010111011  ..........  U U U U U  18  18  11  11  11
movem     16  er    .     0100110010......  A..DXWL...  U U U U U  12  12   8   8   8
movem     32  er    pi    0100110011011...  ..........  U U U U U  12  12   8   8   8
movem     32  er    pcdi  0100110011111010  ..........  U U U U U  16  16   9   9   9
movem     32  er    pcix  0100110011111011  ..........  U U U U U  18  18  11  11  11
movem     32  er    .     0100110011......  A..DXWL...  U U U U U  12  12   8   8   8
movep     16  er    .     0000...100001...  ..........  U U U U U  16  16  12  12  12
movep     32  er    .     0000...101001...  ..........  U U U U U  24  24  18  18  18
movep     16  re    .     0000...110001...  ..........  U U U U U  16  16  11  11  11
movep     32  re    .     0000...111001...  ..........  U U U U U  24  24  17  17  17
moveq     32  .     .     0111...0........  ..........  U U U U U   4   4   2   2   2
moves      8  .     .     0000111000......  A+-DXWL...  . S S S S   .  14   5   5   5
moves     16  .     .     0000111001......  A+-DXWL...  . S S S S   .  14   5   5   5
moves     32  .     .     0000111010......  A+-DXWL...  . S S S S   .  16   5   5   5
move16    32  .     .     1111011000100...  ..........  . . . . U   .   .   .   .   4  TODO: correct timing
muls      16  .     d     1100...111000...  ..........  U U U U U  54  32  27  27  27
muls      16  .     .     1100...111......  A+-DXWLdxI  U U U U U  54  32  27  27  27
mulu      16  .     d     1100...011000...  ..........  U U U U U  54  30  27  27  27
mulu      16  .     .     1100...011......  A+-DXWLdxI  U U U U U  54  30  27  27  27
mull      32  .     d     0100110000000...  ..........  . . U U U   .   .  43  43  43
mull      32  .     .     0100110000......  A+-DXWLdxI  . . U U U   .   .  43  43  43
nbcd       8  .     d     0100100000000...  ..........  U U U U U   6   6   6   6   6
nbcd       8  .     .     0100100000......  A+-DXWL...  U U U U U   8   8   6   6   6
neg        8  .     d     0100010000000...  ..........  U U U U U   4   4   2   2   2
neg        8  .     .     0100010000......  A+-DXWL...  U U U U U   8   8   4   4   4
neg       16  .     d     0100010001000...  ..........  U U U U U   4   4   2   2   2
neg       16  .     .     0100010001......  A+-DXWL...  U U U U U   8   8   4   4   4
neg       32  .     d     0100010010000...  ..........  U U U U U   6   6   2   2   2
neg       32  .     .     0100010010......  A+-DXWL...  U U U U U  12  12   4   4   4
negx       8  .     d     0100000000000...  ..........  U U U U U   4   4   2   2   2
negx       8  .     .     0100000000......  A+-DXWL...  U U U U U   8   8   4   4   4
negx      16  .     d     0100000001000...  ..........  U U U U U   4   4   2   2   2
negx      16  .     .     0100000001......  A+-DXWL...  U U U U U   8   8   4   4   4
negx      32  .     d     0100000010000...  ..........  U U U U U   6   6   2   2   2
negx      32  .     .     0100000010......  A+-DXWL...  U U U U U  12  12   4   4   4
nop        0  .     .     0100111001110001  ..........  U U U U U   4   4   2   2   2
not        8  .     d     0100011000000...  ..........  U U U U U   4   4   2   2   2
not        8  .     .     0100011000......  A+-DXWL...  U U U U U   8   8   4   4   4
not       16  .     d     0100011001000...  ..........  U U U U U   4   4   2   2   2
not       16  .     .     0100011001......  A+-DXWL...  U U U U U   8   8   4   4   4
not       32  .     d     0100011010000...  ..........  U U U U U   6   6   2   2   2
not       32  .     .     0100011010......  A+-DXWL...  U U U U U  12  12   4   4   4
or         8  er    d     1000...000000...  ..........  U U U U U   4   4   2   2   2
or         8  er    .     1000...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
or        16  er    d     1000...001000...  ..........  U U U U U   4   4   2   2   2
or        16  er    .     1000...001......  A+-DXWLdxI  U U U U U   4   4   2   2   2
or        32  er    d     1000...010000...  ..........  U U U U U   6   6   2   2   2
or        32  er    .     1000...010......  A+-DXWLdxI  U U U U U   6   6   2   2   2
or         8  re    .     1000...100......  A+-DXWL...  U U U U U   8   8   4   4   4
or        16  re    .     1000...101......  A+-DXWL...  U U U U U   8   8   4   4   4
or        32  re    .     1000...110......  A+-DXWL...  U U U U U  12  12   4   4   4
ori       16  toc   .     0000000000111100  ..........  U U U U U  20  16  12  12  12
ori       16  tos   .     0000000001111100  ..........  S S S S S  20  16  12  12  12
ori        8  .     d     0000000000000...  ..........  U U U U U   8   8   2   2   2
ori        8  .     .     0000000000......  A+-DXWL...  U U U U U  12  12   4   4   4
ori       16  .     d     0000000001000...  ..........  U U U U U   8   8   2   2   2
ori       16  .     .     0000000001......  A+-DXWL...  U U U U U  12  12   4   4   4
ori       32  .     d     0000000010000...  ..........  U U U U U  16  14   2   2   2
ori       32  .     .     0000000010......  A+-DXWL...  U U U U U  20  20   4   4   4
pack      16  rr    .     1000...101000...  ..........  . . U U U   .   .   6   6   6
pack      16  mm    ax7   1000111101001...  ..........  . . U U U   .   .  13  13  13
pack      16  mm    ay7   1000...101001111  ..........  . . U U U   .   .  13  13  13
pack      16  mm    axy7  1000111101001111  ..........  . . U U U   .   .  13  13  13
pack      16  mm    .     1000...101001...  ..........  . . U U U   .   .  13  13  13
pea       32  .     .     0100100001......  A..DXWLdx.  U U U U U   6   6   5   5   5
pflush    32  .     .     1111010100011000  ..........  . . . . S   .   .   .   .   4   TODO: correct timing
pmmu      32  .     .     1111000.........  ..........  . . S S S   .   .   8   8   8
reset      0  .     .     0100111001110000  ..........  S S S S S   0   0   0   0   0
ror        8  s     .     1110...000011...  ..........  U U U U U   6   6   8   8   8
ror       16  s     .     1110...001011...  ..........  U U U U U   6   6   8   8   8
ror       32  s     .     1110...010011...  ..........  U U U U U   8   8   8   8   8
ror        8  r     .     1110...000111...  ..........  U U U U U   6   6   8   8   8
ror       16  r     .     1110...001111...  ..........  U U U U U   6   6   8   8   8
ror       32  r     .     1110...010111...  ..........  U U U U U   8   8   8   8   8
ror       16  .     .     1110011011......  A+-DXWL...  U U U U U   8   8   7   7   7
rol        8  s     .     1110...100011...  ..........  U U U U U   6   6   8   8   8
rol       16  s     .     1110...101011...  ..........  U U U U U   6   6   8   8   8
rol       32  s     .     1110...110011...  ..........  U U U U U   8   8   8   8   8
rol        8  r     .     1110...100111...  ..........  U U U U U   6   6   8   8   8
rol       16  r     .     1110...101111...  ..........  U U U U U   6   6   8   8   8
rol       32  r     .     1110...110111...  ..........  U U U U U   8   8   8   8   8
rol       16  .     .     1110011111......  A+-DXWL...  U U U U U   8   8   7   7   7
roxr       8  s     .     1110...000010...  ..........  U U U U U   6   6  12  12  12
roxr      16  s     .     1110...001010...  ..........  U U U U U   6   6  12  12  12
roxr      32  s     .     1110...010010...  ..........  U U U U U   8   8  12  12  12
roxr       8  r     .     1110...000110...  ..........  U U U U U   6   6  12  12  12
roxr      16  r     .     1110...001110...  ..........  U U U U U   6   6  12  12  12
roxr      32  r     .     1110...010110...  ..........  U U U U U   8   8  12  12  12
roxr      16  .     .     1110010011......  A+-DXWL...  U U U U U   8   8   5   5   5
roxl       8  s     .     1110...100010...  ..........  U U U U U   6   6  12  12  12
roxl      16  s     .     1110...101010...  ..........  U U U U U   6   6  12  12  12
roxl      32  s     .     1110...110010...  ..........  U U U U U   8   8  12  12  12
roxl       8  r     .     1110...100110...  ..........  U U U U U   6   6  12  12  12
roxl      16  r     .     1110...101110...  ..........  U U U U U   6   6  12  12  12
roxl      32  r     .     1110...110110...  ..........  U U U U U   8   8  12  12  12
roxl      16  .     .     1110010111......  A+-DXWL...  U U U U U   8   8   5   5   5
rtd       32  .     .     0100111001110100  ..........  . U U U U   .  16  10  10  10
rte       32  .     .     0100111001110011  ..........  S S S S S  20  24  20  20  20  bus fault not emulated
rtm       32  .     .     000001101100....  ..........  . . U U U   .   .  19  19  19  not properly emulated
rtr       32  .     .     0100111001110111  ..........  U U U U U  20  20  14  14  14
rts       32  .     .     0100111001110101  ..........  U U U U U  16  16  10  10  10
sbcd       8  rr    .     1000...100000...  ..........  U U U U U   6   6   4   4   4
sbcd       8  mm    ax7   1000111100001...  ..........  U U U U U  18  18  16  16  16
sbcd       8  mm    ay7   1000...100001111  ..........  U U U U U  18  18  16  16  16
sbcd       8  mm    axy7  1000111100001111  ..........  U U U U U  18  18  16  16  16
sbcd       8  mm    .     1000...100001...  ..........  U U U U U  18  18  16  16  16
st         8  .     d     0101000011000...  ..........  U U U U U   6   4   4   4   4
st         8  .     .     0101000011......  A+-DXWL...  U U U U U   8   8   6   6   6
sf         8  .     d     0101000111000...  ..........  U U U U U   4   4   4   4   4
sf         8  .     .     0101000111......  A+-DXWL...  U U U U U   8   8   6   6   6
scc        8  .     d     0101....11000...  ..........  U U U U U   4   4   4   4   4
scc        8  .     .     0101....11......  A+-DXWL...  U U U U U   8   8   6   6   6
stop       0  .     .     0100111001110010  ..........  S S S S S   4   4   8   8   8
sub        8  er    d     1001...000000...  ..........  U U U U U   4   4   2   2   2
sub        8  er    .     1001...000......  A+-DXWLdxI  U U U U U   4   4   2   2   2
sub       16  er    d     1001...001000...  ..........  U U U U U   4   4   2   2   2
sub       16  er    a     1001...001001...  ..........  U U U U U   4   4   2   2   2
sub       16  er    .     1001...001......  A+-DXWLdxI  U U U U U   4   4   2   2   2
sub       32  er    d     1001...010000...  ..........  U U U U U   6   6   2   2   2
sub       32  er    a     1001...010001...  ..........  U U U U U   6   6   2   2   2
sub       32  er    .     1001...010......  A+-DXWLdxI  U U U U U   6   6   2   2   2
sub        8  re    .     1001...100......  A+-DXWL...  U U U U U   8   8   4   4   4
sub       16  re    .     1001...101......  A+-DXWL...  U U U U U   8   8   4   4   4
sub       32  re    .     1001...110......  A+-DXWL...  U U U U U  12  12   4   4   4
suba      16  .     d     1001...011000...  ..........  U U U U U   8   8   2   2   2
suba      16  .     a     1001...011001...  ..........  U U U U U   8   8   2   2   2
suba      16  .     .     1001...011......  A+-DXWLdxI  U U U U U   8   8   2   2   2
suba      32  .     d     1001...111000...  ..........  U U U U U   6   6   2   2   2
suba      32  .     a     1001...111001...  ..........  U U U U U   6   6   2   2   2
suba      32  .     .     1001...111......  A+-DXWLdxI  U U U U U   6   6   2   2   2
subi       8  .     d     0000010000000...  ..........  U U U U U   8   8   2   2   2
subi       8  .     .     0000010000......  A+-DXWL...  U U U U U  12  12   4   4   4
subi      16  .     d     0000010001000...  ..........  U U U U U   8   8   2   2   2
subi      16  .     .     0000010001......  A+-DXWL...  U U U U U  12  12   4   4   4
subi      32  .     d     0000010010000...  ..........  U U U U U  16  14   2   2   2
subi      32  .     .     0000010010......  A+-DXWL...  U U U U U  20  20   4   4   4
subq       8  .     d     0101...100000...  ..........  U U U U U   4   4   2   2   2
subq       8  .     .     0101...100......  A+-DXWL...  U U U U U   8   8   4   4   4
subq      16  .     d     0101...101000...  ..........  U U U U U   4   4   2   2   2
subq      16  .     a     0101...101001...  ..........  U U U U U   8   4   2   2   2
subq      16  .     .     0101...101......  A+-DXWL...  U U U U U   8   8   4   4   4
subq      32  .     d     0101...110000...  ..........  U U U U U   8   8   2   2   2
subq      32  .     a     0101...110001...  ..........  U U U U U   8   8   2   2   2
subq      32  .     .     0101...110......  A+-DXWL...  U U U U U  12  12   4   4   4
subx       8  rr    .     1001...100000...  ..........  U U U U U   4   4   2   2   2
subx      16  rr    .     1001...101000...  ..........  U U U U U   4   4   2   2   2
subx      32  rr    .     1001...110000...  ..........  U U U U U   8   6   2   2   2
subx       8  mm    ax7   1001111100001...  ..........  U U U U U  18  18  12  12  12
subx       8  mm    ay7   1001...100001111  ..........  U U U U U  18  18  12  12  12
subx       8  mm    axy7  1001111100001111  ..........  U U U U U  18  18  12  12  12
subx       8  mm    .     1001...100001...  ..........  U U U U U  18  18  12  12  12
subx      16  mm    .     1001...101001...  ..........  U U U U U  18  18  12  12  12
subx      32  mm    .     1001...110001...  ..........  U U U U U  30  30  12  12  12
swap      32  .     .     0100100001000...  ..........  U U U U U   4   4   4   4   4
tas        8  .     d     0100101011000...  ..........  U U U U U   4   4   4   4   4
tas        8  .     .     0100101011......  A+-DXWL...  U U U U U  14  14  12  12  12
trap       0  .     .     010011100100....  ..........  U U U U U   4   4   4   4   4
trapt      0  .     .     0101000011111100  ..........  . . U U U   .   .   4   4   4
trapt     16  .     .     0101000011111010  ..........  . . U U U   .   .   6   6   6
trapt     32  .     .     0101000011111011  ..........  . . U U U   .   .   8   8   8
trapf      0  .     .     0101000111111100  ..........  . . U U U   .   .   4   4   4
trapf     16  .     .     0101000111111010  ..........  . . U U U   .   .   6   6   6
trapf     32  .     .     0101000111111011  ..........  . . U U U   .   .   8   8   8
trapcc     0  .     .     0101....11111100  ..........  . . U U U   .   .   4   4   4
trapcc    16  .     .     0101....11111010  ..........  . . U U U   .   .   6   6   6
trapcc    32  .     .     0101....11111011  ..........  . . U U U   .   .   8   8   8
trapv      0  .     .     0100111001110110  ..........  U U U U U   4   4   4   4   4
tst        8  .     d     0100101000000...  ..........  U U U U U   4   4   2   2   2
tst        8  .     .     0100101000......  A+-DXWL...  U U U U U   4   4   2   2   2
tst        8  .     pcdi  0100101000111010  ..........  . . U U U   .   .   7   7   7
tst        8  .     pcix  0100101000111011  ..........  . . U U U   .   .   9   9   9
tst        8  .     i     0100101000111100  ..........  . . U U U   .   .   6   6   6
tst       16  .     d     0100101001000...  ..........  U U U U U   4   4   2   2   2
tst       16  .     a     0100101001001...  ..........  . . U U U   .   .   2   2   2
tst       16  .     .     0100101001......  A+-DXWL...  U U U U U   4   4   2   2   2
tst       16  .     pcdi  0100101001111010  ..........  . . U U U   .   .   7   7   7
tst       16  .     pcix  0100101001111011  ..........  . . U U U   .   .   9   9   9
tst       16  .     i     0100101001111100  ..........  . . U U U   .   .   6   6   6
tst       32  .     d     0100101010000...  ..........  U U U U U   4   4   2   2   2
tst       32  .     a     0100101010001...  ..........  . . U U U   .   .   2   2   2
tst       32  .     .     0100101010......  A+-DXWL...  U U U U U   4   4   2   2   2
tst       32  .     pcdi  0100101010111010  ..........  . . U U U   .   .   7   7   7
tst       32  .     pcix  0100101010111011  ..........  . . U U U   .   .   9   9   9
tst       32  .     i     0100101010111100  ..........  . . U U U   .   .   6   6   6
unlk      32  .     a7    0100111001011111  ..........  U U U U U  12  12   6   6   6
unlk      32  .     .     0100111001011...  ..........  U U U U U  12  12   6   6   6
unpk      16  rr    .     1000...110000...  ..........  . . U U U   .   .   8   8   8
unpk      16  mm    ax7   1000111110001...  ..........  . . U U U   .   .  13  13  13
unpk      16  mm    ay7   1000...110001111  ..........  . . U U U   .   .  13  13  13
unpk      16  mm    axy7  1000111110001111  ..........  . . U U U   .   .  13  13  13
unpk      16  mm    .     1000...110001...  ..........  . . U U U   .   .  13  13  13



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_OPCODE_HANDLER_BODY

M68KMAKE_OP(1010, 0, ., .)
{
	m68ki_exception_1010(m68k);
}


M68KMAKE_OP(1111, 0, ., .)
{
	m68ki_exception_1111(m68k);
}


M68KMAKE_OP(040fpu0, 32, ., .)
{
	if(CPU_TYPE_IS_030_PLUS(m68k->cpu_type))
	{
		m68040_fpu_op0(m68k);
		return;
	}
	m68ki_exception_1111(m68k);
}


M68KMAKE_OP(040fpu1, 32, ., .)
{
	if(CPU_TYPE_IS_030_PLUS(m68k->cpu_type))
	{
		m68040_fpu_op1(m68k);
		return;
	}
	m68ki_exception_1111(m68k);
}



M68KMAKE_OP(abcd, 8, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = DY;
	UINT32 dst = *r_dst;
	UINT32 res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1(m68k);

	m68k->v_flag = ~res; /* Undefined V behavior */

	if(res > 9)
		res += 6;
	res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
	m68k->x_flag = m68k->c_flag = (res > 0x99) << 8;
	if(m68k->c_flag)
		res -= 0xa0;

	m68k->v_flag &= res; /* Undefined V behavior part II */
	m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}


M68KMAKE_OP(abcd, 8, mm, ax7)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1(m68k);

	m68k->v_flag = ~res; /* Undefined V behavior */

	if(res > 9)
		res += 6;
	res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
	m68k->x_flag = m68k->c_flag = (res > 0x99) << 8;
	if(m68k->c_flag)
		res -= 0xa0;

	m68k->v_flag &= res; /* Undefined V behavior part II */
	m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(abcd, 8, mm, ay7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1(m68k);

	m68k->v_flag = ~res; /* Undefined V behavior */

	if(res > 9)
		res += 6;
	res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
	m68k->x_flag = m68k->c_flag = (res > 0x99) << 8;
	if(m68k->c_flag)
		res -= 0xa0;

	m68k->v_flag &= res; /* Undefined V behavior part II */
	m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(abcd, 8, mm, axy7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1(m68k);

	m68k->v_flag = ~res; /* Undefined V behavior */

	if(res > 9)
		res += 6;
	res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
	m68k->x_flag = m68k->c_flag = (res > 0x99) << 8;
	if(m68k->c_flag)
		res -= 0xa0;

	m68k->v_flag &= res; /* Undefined V behavior part II */
	m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(abcd, 8, mm, .)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(src) + LOW_NIBBLE(dst) + XFLAG_AS_1(m68k);

	m68k->v_flag = ~res; /* Undefined V behavior */

	if(res > 9)
		res += 6;
	res += HIGH_NIBBLE(src) + HIGH_NIBBLE(dst);
	m68k->x_flag = m68k->c_flag = (res > 0x99) << 8;
	if(m68k->c_flag)
		res -= 0xa0;

	m68k->v_flag &= res; /* Undefined V behavior part II */
	m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(add, 8, er, d)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_8(DY);
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(add, 8, er, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = M68KMAKE_GET_OPER_AY_8;
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(add, 16, er, d)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_16(DY);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(add, 16, er, a)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_16(AY);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(add, 16, er, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = M68KMAKE_GET_OPER_AY_16;
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(add, 32, er, d)
{
	UINT32* r_dst = &DX;
	UINT32 src = DY;
	UINT32 dst = *r_dst;
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(add, 32, er, a)
{
	UINT32* r_dst = &DX;
	UINT32 src = AY;
	UINT32 dst = *r_dst;
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(add, 32, er, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = M68KMAKE_GET_OPER_AY_32;
	UINT32 dst = *r_dst;
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(add, 8, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = MASK_OUT_ABOVE_8(DX);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(add, 16, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = MASK_OUT_ABOVE_16(DX);
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(add, 32, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 src = DX;
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	m68ki_write_32(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(adda, 16, ., d)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + MAKE_INT_16(DY));
}


M68KMAKE_OP(adda, 16, ., a)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + MAKE_INT_16(AY));
}


M68KMAKE_OP(adda, 16, ., .)
{
	UINT32* r_dst = &AX;
	UINT32 src = MAKE_INT_16(M68KMAKE_GET_OPER_AY_16);

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + src);
}


M68KMAKE_OP(adda, 32, ., d)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + DY);
}


M68KMAKE_OP(adda, 32, ., a)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + AY);
}


M68KMAKE_OP(adda, 32, ., .)
{
	UINT32* r_dst = &AX;
	UINT32 src = M68KMAKE_GET_OPER_AY_32;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + src);
}


M68KMAKE_OP(addi, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = OPER_I_8(m68k);
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(addi, 8, ., .)
{
	UINT32 src = OPER_I_8(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(addi, 16, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = OPER_I_16(m68k);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(addi, 16, ., .)
{
	UINT32 src = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(addi, 32, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = OPER_I_32(m68k);
	UINT32 dst = *r_dst;
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(addi, 32, ., .)
{
	UINT32 src = OPER_I_32(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	m68ki_write_32(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(addq, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(addq, 8, ., .)
{
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(addq, 16, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(addq, 16, ., a)
{
	UINT32* r_dst = &AY;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + (((m68k->ir >> 9) - 1) & 7) + 1);
}


M68KMAKE_OP(addq, 16, ., .)
{
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(addq, 32, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 dst = *r_dst;
	UINT32 res = src + dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(addq, 32, ., a)
{
	UINT32* r_dst = &AY;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst + (((m68k->ir >> 9) - 1) & 7) + 1);
}


M68KMAKE_OP(addq, 32, ., .)
{
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = src + dst;


	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	m68ki_write_32(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(addx, 8, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_8(DY);
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}


M68KMAKE_OP(addx, 16, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_16(DY);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);

	res = MASK_OUT_ABOVE_16(res);
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
}


M68KMAKE_OP(addx, 32, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = DY;
	UINT32 dst = *r_dst;
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);

	res = MASK_OUT_ABOVE_32(res);
	m68k->not_z_flag |= res;

	*r_dst = res;
}


M68KMAKE_OP(addx, 8, mm, ax7)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(addx, 8, mm, ay7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(addx, 8, mm, axy7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(addx, 8, mm, .)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->v_flag = VFLAG_ADD_8(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(addx, 16, mm, .)
{
	UINT32 src = OPER_AY_PD_16(m68k);
	UINT32 ea  = EA_AX_PD_16(m68k);
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_16(res);
	m68k->v_flag = VFLAG_ADD_16(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);

	res = MASK_OUT_ABOVE_16(res);
	m68k->not_z_flag |= res;

	m68ki_write_16(m68k, ea, res);
}


M68KMAKE_OP(addx, 32, mm, .)
{
	UINT32 src = OPER_AY_PD_32(m68k);
	UINT32 ea  = EA_AX_PD_32(m68k);
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = src + dst + XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_ADD_32(src, dst, res);
	m68k->x_flag = m68k->c_flag = CFLAG_ADD_32(src, dst, res);

	res = MASK_OUT_ABOVE_32(res);
	m68k->not_z_flag |= res;

	m68ki_write_32(m68k, ea, res);
}


M68KMAKE_OP(and, 8, er, d)
{
	m68k->not_z_flag = MASK_OUT_ABOVE_8(DX &= (DY | 0xffffff00));

	m68k->n_flag = NFLAG_8(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(and, 8, er, .)
{
	m68k->not_z_flag = MASK_OUT_ABOVE_8(DX &= (M68KMAKE_GET_OPER_AY_8 | 0xffffff00));

	m68k->n_flag = NFLAG_8(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(and, 16, er, d)
{
	m68k->not_z_flag = MASK_OUT_ABOVE_16(DX &= (DY | 0xffff0000));

	m68k->n_flag = NFLAG_16(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(and, 16, er, .)
{
	m68k->not_z_flag = MASK_OUT_ABOVE_16(DX &= (M68KMAKE_GET_OPER_AY_16 | 0xffff0000));

	m68k->n_flag = NFLAG_16(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(and, 32, er, d)
{
	m68k->not_z_flag = DX &= DY;

	m68k->n_flag = NFLAG_32(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(and, 32, er, .)
{
	m68k->not_z_flag = DX &= M68KMAKE_GET_OPER_AY_32;

	m68k->n_flag = NFLAG_32(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(and, 8, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 res = DX & m68ki_read_8(m68k, ea);

	m68k->n_flag = NFLAG_8(res);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(and, 16, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 res = DX & m68ki_read_16(m68k, ea);

	m68k->n_flag = NFLAG_16(res);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(and, 32, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 res = DX & m68ki_read_32(m68k, ea);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;

	m68ki_write_32(m68k, ea, res);
}


M68KMAKE_OP(andi, 8, ., d)
{
	m68k->not_z_flag = MASK_OUT_ABOVE_8(DY &= (OPER_I_8(m68k) | 0xffffff00));

	m68k->n_flag = NFLAG_8(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(andi, 8, ., .)
{
	UINT32 src = OPER_I_8(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 res = src & m68ki_read_8(m68k, ea);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(andi, 16, ., d)
{
	m68k->not_z_flag = MASK_OUT_ABOVE_16(DY &= (OPER_I_16(m68k) | 0xffff0000));

	m68k->n_flag = NFLAG_16(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(andi, 16, ., .)
{
	UINT32 src = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 res = src & m68ki_read_16(m68k, ea);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;

	m68ki_write_16(m68k, ea, res);
}


M68KMAKE_OP(andi, 32, ., d)
{
	m68k->not_z_flag = DY &= (OPER_I_32(m68k));

	m68k->n_flag = NFLAG_32(m68k->not_z_flag);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(andi, 32, ., .)
{
	UINT32 src = OPER_I_32(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 res = src & m68ki_read_32(m68k, ea);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;

	m68ki_write_32(m68k, ea, res);
}


M68KMAKE_OP(andi, 16, toc, .)
{
	m68ki_set_ccr(m68k, m68ki_get_ccr(m68k) & OPER_I_16(m68k));
}


M68KMAKE_OP(andi, 16, tos, .)
{
	if(m68k->s_flag)
	{
		UINT32 src = OPER_I_16(m68k);
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_set_sr(m68k, m68ki_get_sr(m68k) & src);
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(asr, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	if(GET_MSB_8(src))
		res |= m68ki_shift_8_table[shift];

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->x_flag = m68k->c_flag = src << (9-shift);
}


M68KMAKE_OP(asr, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	if(GET_MSB_16(src))
		res |= m68ki_shift_16_table[shift];

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->x_flag = m68k->c_flag = src << (9-shift);
}


M68KMAKE_OP(asr, 32, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = *r_dst;
	UINT32 res = src >> shift;

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	if(GET_MSB_32(src))
		res |= m68ki_shift_32_table[shift];

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->x_flag = m68k->c_flag = src << (9-shift);
}


M68KMAKE_OP(asr, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 8)
		{
			if(GET_MSB_8(src))
				res |= m68ki_shift_8_table[shift];

			*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

			m68k->x_flag = m68k->c_flag = src << (9-shift);
			m68k->n_flag = NFLAG_8(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		if(GET_MSB_8(src))
		{
			*r_dst |= 0xff;
			m68k->c_flag = CFLAG_SET;
			m68k->x_flag = XFLAG_SET;
			m68k->n_flag = NFLAG_SET;
			m68k->not_z_flag = ZFLAG_CLEAR;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst &= 0xffffff00;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->x_flag = XFLAG_CLEAR;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_8(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(asr, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 16)
		{
			if(GET_MSB_16(src))
				res |= m68ki_shift_16_table[shift];

			*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

			m68k->c_flag = m68k->x_flag = (src >> (shift - 1))<<8;
			m68k->n_flag = NFLAG_16(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		if(GET_MSB_16(src))
		{
			*r_dst |= 0xffff;
			m68k->c_flag = CFLAG_SET;
			m68k->x_flag = XFLAG_SET;
			m68k->n_flag = NFLAG_SET;
			m68k->not_z_flag = ZFLAG_CLEAR;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst &= 0xffff0000;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->x_flag = XFLAG_CLEAR;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_16(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(asr, 32, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = *r_dst;
	UINT32 res = src >> shift;

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 32)
		{
			if(GET_MSB_32(src))
				res |= m68ki_shift_32_table[shift];

			*r_dst = res;

			m68k->c_flag = m68k->x_flag = (src >> (shift - 1))<<8;
			m68k->n_flag = NFLAG_32(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		if(GET_MSB_32(src))
		{
			*r_dst = 0xffffffff;
			m68k->c_flag = CFLAG_SET;
			m68k->x_flag = XFLAG_SET;
			m68k->n_flag = NFLAG_SET;
			m68k->not_z_flag = ZFLAG_CLEAR;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst = 0;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->x_flag = XFLAG_CLEAR;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_32(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(asr, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = src >> 1;

	if(GET_MSB_16(src))
		res |= 0x8000;

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = m68k->x_flag = src << 8;
}


M68KMAKE_OP(asl, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_8(src << shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->x_flag = m68k->c_flag = src << shift;
	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	src &= m68ki_shift_8_table[shift + 1];
	m68k->v_flag = (!(src == 0 || (src == m68ki_shift_8_table[shift + 1] && shift < 8)))<<7;
}


M68KMAKE_OP(asl, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_16(src << shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src >> (8-shift);
	src &= m68ki_shift_16_table[shift + 1];
	m68k->v_flag = (!(src == 0 || src == m68ki_shift_16_table[shift + 1]))<<7;
}


M68KMAKE_OP(asl, 32, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = *r_dst;
	UINT32 res = MASK_OUT_ABOVE_32(src << shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src >> (24-shift);
	src &= m68ki_shift_32_table[shift + 1];
	m68k->v_flag = (!(src == 0 || src == m68ki_shift_32_table[shift + 1]))<<7;
}


M68KMAKE_OP(asl, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_8(src << shift);

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 8)
		{
			*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
			m68k->x_flag = m68k->c_flag = src << shift;
			m68k->n_flag = NFLAG_8(res);
			m68k->not_z_flag = res;
			src &= m68ki_shift_8_table[shift + 1];
			m68k->v_flag = (!(src == 0 || src == m68ki_shift_8_table[shift + 1]))<<7;
			return;
		}

		*r_dst &= 0xffffff00;
		m68k->x_flag = m68k->c_flag = ((shift == 8 ? src & 1 : 0))<<8;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = (!(src == 0))<<7;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_8(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(asl, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_16(src << shift);

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 16)
		{
			*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
			m68k->x_flag = m68k->c_flag = (src << shift) >> 8;
			m68k->n_flag = NFLAG_16(res);
			m68k->not_z_flag = res;
			src &= m68ki_shift_16_table[shift + 1];
			m68k->v_flag = (!(src == 0 || src == m68ki_shift_16_table[shift + 1]))<<7;
			return;
		}

		*r_dst &= 0xffff0000;
		m68k->x_flag = m68k->c_flag = ((shift == 16 ? src & 1 : 0))<<8;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = (!(src == 0))<<7;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_16(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(asl, 32, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = *r_dst;
	UINT32 res = MASK_OUT_ABOVE_32(src << shift);

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 32)
		{
			*r_dst = res;
			m68k->x_flag = m68k->c_flag = (src >> (32 - shift)) << 8;
			m68k->n_flag = NFLAG_32(res);
			m68k->not_z_flag = res;
			src &= m68ki_shift_32_table[shift + 1];
			m68k->v_flag = (!(src == 0 || src == m68ki_shift_32_table[shift + 1]))<<7;
			return;
		}

		*r_dst = 0;
		m68k->x_flag = m68k->c_flag = ((shift == 32 ? src & 1 : 0))<<8;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = (!(src == 0))<<7;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_32(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(asl, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = MASK_OUT_ABOVE_16(src << 1);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src >> 7;
	src &= 0xc000;
	m68k->v_flag = (!(src == 0 || src == 0xc000))<<7;
}


M68KMAKE_OP(bcc, 8, ., .)
{
	if(M68KMAKE_CC)
	{
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_branch_8(m68k, MASK_OUT_ABOVE_8(m68k->ir));
		return;
	}
	m68k->remaining_cycles -= m68k->cyc_bcc_notake_b;
}


M68KMAKE_OP(bcc, 16, ., .)
{
	if(M68KMAKE_CC)
	{
		UINT32 offset = OPER_I_16(m68k);
		REG_PC -= 2;
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_branch_16(m68k, offset);
		return;
	}
	REG_PC += 2;
	m68k->remaining_cycles -= m68k->cyc_bcc_notake_w;
}


M68KMAKE_OP(bcc, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		if(M68KMAKE_CC)
		{
			UINT32 offset = OPER_I_32(m68k);
			REG_PC -= 4;
			m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
			m68ki_branch_32(m68k, offset);
			return;
		}
		REG_PC += 4;
		return;
	}
	else
	{
		if(M68KMAKE_CC)
		{
			m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
			m68ki_branch_8(m68k, MASK_OUT_ABOVE_8(m68k->ir));
			return;
		}
		m68k->remaining_cycles -= m68k->cyc_bcc_notake_b;
	}
}


M68KMAKE_OP(bchg, 32, r, d)
{
	UINT32* r_dst = &DY;
	UINT32 mask = 1 << (DX & 0x1f);

	m68k->not_z_flag = *r_dst & mask;
	*r_dst ^= mask;
}


M68KMAKE_OP(bchg, 8, r, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);
	UINT32 mask = 1 << (DX & 7);

	m68k->not_z_flag = src & mask;
	m68ki_write_8(m68k, ea, src ^ mask);
}


M68KMAKE_OP(bchg, 32, s, d)
{
	UINT32* r_dst = &DY;
	UINT32 mask = 1 << (OPER_I_8(m68k) & 0x1f);

	m68k->not_z_flag = *r_dst & mask;
	*r_dst ^= mask;
}


M68KMAKE_OP(bchg, 8, s, .)
{
	UINT32 mask = 1 << (OPER_I_8(m68k) & 7);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);

	m68k->not_z_flag = src & mask;
	m68ki_write_8(m68k, ea, src ^ mask);
}


M68KMAKE_OP(bclr, 32, r, d)
{
	UINT32* r_dst = &DY;
	UINT32 mask = 1 << (DX & 0x1f);

	m68k->not_z_flag = *r_dst & mask;
	*r_dst &= ~mask;
}


M68KMAKE_OP(bclr, 8, r, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);
	UINT32 mask = 1 << (DX & 7);

	m68k->not_z_flag = src & mask;
	m68ki_write_8(m68k, ea, src & ~mask);
}


M68KMAKE_OP(bclr, 32, s, d)
{
	UINT32* r_dst = &DY;
	UINT32 mask = 1 << (OPER_I_8(m68k) & 0x1f);

	m68k->not_z_flag = *r_dst & mask;
	*r_dst &= ~mask;
}


M68KMAKE_OP(bclr, 8, s, .)
{
	UINT32 mask = 1 << (OPER_I_8(m68k) & 7);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);

	m68k->not_z_flag = src & mask;
	m68ki_write_8(m68k, ea, src & ~mask);
}


M68KMAKE_OP(bfchg, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32* data = &DY;
		UINT64 mask;


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];

		offset &= 31;
		width = ((width-1) & 31) + 1;

		mask = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask = ROR_32(mask, offset);

		m68k->n_flag = NFLAG_32(*data<<offset);
		m68k->not_z_flag = *data & mask;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		*data ^= mask;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfchg, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32 mask_base;
		UINT32 data_long;
		UINT32 mask_long;
		UINT32 data_byte = 0;
		UINT32 mask_byte = 0;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;


		if(BIT_B(word2))
			offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		offset %= 8;
		if(offset < 0)
		{
			offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;

		mask_base = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask_long = mask_base >> offset;

		data_long = m68ki_read_32(m68k, ea);
		m68k->n_flag = NFLAG_32(data_long << offset);
		m68k->not_z_flag = data_long & mask_long;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		m68ki_write_32(m68k, ea, data_long ^ mask_long);

		if((width + offset) > 32)
		{
			mask_byte = MASK_OUT_ABOVE_8(mask_base);
			data_byte = m68ki_read_8(m68k, ea+4);
			m68k->not_z_flag |= (data_byte & mask_byte);
			m68ki_write_8(m68k, ea+4, data_byte ^ mask_byte);
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfclr, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32* data = &DY;
		UINT64 mask;


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];


		offset &= 31;
		width = ((width-1) & 31) + 1;


		mask = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask = ROR_32(mask, offset);

		m68k->n_flag = NFLAG_32(*data<<offset);
		m68k->not_z_flag = *data & mask;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		*data &= ~mask;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfclr, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32 mask_base;
		UINT32 data_long;
		UINT32 mask_long;
		UINT32 data_byte = 0;
		UINT32 mask_byte = 0;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;


		if(BIT_B(word2))
			offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		offset %= 8;
		if(offset < 0)
		{
			offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;

		mask_base = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask_long = mask_base >> offset;

		data_long = m68ki_read_32(m68k, ea);
		m68k->n_flag = NFLAG_32(data_long << offset);
		m68k->not_z_flag = data_long & mask_long;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		m68ki_write_32(m68k, ea, data_long & ~mask_long);

		if((width + offset) > 32)
		{
			mask_byte = MASK_OUT_ABOVE_8(mask_base);
			data_byte = m68ki_read_8(m68k, ea+4);
			m68k->not_z_flag |= (data_byte & mask_byte);
			m68ki_write_8(m68k, ea+4, data_byte & ~mask_byte);
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfexts, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT64 data = DY;


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];

		offset &= 31;
		width = ((width-1) & 31) + 1;

		data = ROL_32(data, offset);
		m68k->n_flag = NFLAG_32(data);
		data = MAKE_INT_32(data) >> (32 - width);

		m68k->not_z_flag = data;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		REG_D[(word2>>12)&7] = data;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfexts, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32 data;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;


		if(BIT_B(word2))
			offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		offset %= 8;
		if(offset < 0)
		{
			offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;

		data = m68ki_read_32(m68k, ea);

		data = MASK_OUT_ABOVE_32(data<<offset);

		if((offset+width) > 32)
			data |= (m68ki_read_8(m68k, ea+4) << offset) >> 8;

		m68k->n_flag = NFLAG_32(data);
		data  = MAKE_INT_32(data) >> (32 - width);

		m68k->not_z_flag = data;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		REG_D[(word2 >> 12) & 7] = data;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfextu, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT64 data = DY;


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];

		offset &= 31;
		width = ((width-1) & 31) + 1;

		data = ROL_32(data, offset);
		m68k->n_flag = NFLAG_32(data);
		data >>= 32 - width;

		m68k->not_z_flag = data;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		REG_D[(word2>>12)&7] = data;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfextu, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32 data;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;


		if(BIT_B(word2))
		offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		offset %= 8;
		if(offset < 0)
		{
			offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;

		data = m68ki_read_32(m68k, ea);
		data = MASK_OUT_ABOVE_32(data<<offset);

		if((offset+width) > 32)
			data |= (m68ki_read_8(m68k, ea+4) << offset) >> 8;

		m68k->n_flag = NFLAG_32(data);
		data  >>= (32 - width);

		m68k->not_z_flag = data;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		REG_D[(word2 >> 12) & 7] = data;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfffo, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT64 data = DY;
		UINT32 bit;


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];

		offset &= 31;
		width = ((width-1) & 31) + 1;

		data = ROL_32(data, offset);
		m68k->n_flag = NFLAG_32(data);
		data >>= 32 - width;

		m68k->not_z_flag = data;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		for(bit = 1<<(width-1);bit && !(data & bit);bit>>= 1)
			offset++;

		REG_D[(word2>>12)&7] = offset;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfffo, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		INT32 local_offset;
		UINT32 width = word2;
		UINT32 data;
		UINT32 bit;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;


		if(BIT_B(word2))
			offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		local_offset = offset % 8;
		if(local_offset < 0)
		{
			local_offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;

		data = m68ki_read_32(m68k, ea);
		data = MASK_OUT_ABOVE_32(data<<local_offset);

		if((local_offset+width) > 32)
			data |= (m68ki_read_8(m68k, ea+4) << local_offset) >> 8;

		m68k->n_flag = NFLAG_32(data);
		data  >>= (32 - width);

		m68k->not_z_flag = data;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		for(bit = 1<<(width-1);bit && !(data & bit);bit>>= 1)
			offset++;

		REG_D[(word2>>12)&7] = offset;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfins, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32* data = &DY;
		UINT64 mask;
		UINT64 insert = REG_D[(word2>>12)&7];


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];


		offset &= 31;
		width = ((width-1) & 31) + 1;


		mask = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask = ROR_32(mask, offset);

		insert = MASK_OUT_ABOVE_32(insert << (32 - width));
		m68k->n_flag = NFLAG_32(insert);
		m68k->not_z_flag = insert;
		insert = ROR_32(insert, offset);

		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		*data &= ~mask;
		*data |= insert;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfins, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32 insert_base = REG_D[(word2>>12)&7];
		UINT32 insert_long;
		UINT32 insert_byte;
		UINT32 mask_base;
		UINT32 data_long;
		UINT32 mask_long;
		UINT32 data_byte = 0;
		UINT32 mask_byte = 0;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;


		if(BIT_B(word2))
			offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		offset %= 8;
		if(offset < 0)
		{
			offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;

		mask_base = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask_long = mask_base >> offset;

		insert_base = MASK_OUT_ABOVE_32(insert_base << (32 - width));
		m68k->n_flag = NFLAG_32(insert_base);
		m68k->not_z_flag = insert_base;
		insert_long = insert_base >> offset;

		data_long = m68ki_read_32(m68k, ea);
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		m68ki_write_32(m68k, ea, (data_long & ~mask_long) | insert_long);

		if((width + offset) > 32)
		{
			mask_byte = MASK_OUT_ABOVE_8(mask_base);
			insert_byte = MASK_OUT_ABOVE_8(insert_base);
			data_byte = m68ki_read_8(m68k, ea+4);
			m68k->not_z_flag |= (data_byte & mask_byte);
			m68ki_write_8(m68k, ea+4, (data_byte & ~mask_byte) | insert_byte);
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfset, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32* data = &DY;
		UINT64 mask;


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];


		offset &= 31;
		width = ((width-1) & 31) + 1;


		mask = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask = ROR_32(mask, offset);

		m68k->n_flag = NFLAG_32(*data<<offset);
		m68k->not_z_flag = *data & mask;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		*data |= mask;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bfset, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32 mask_base;
		UINT32 data_long;
		UINT32 mask_long;
		UINT32 data_byte = 0;
		UINT32 mask_byte = 0;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;


		if(BIT_B(word2))
			offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		offset %= 8;
		if(offset < 0)
		{
			offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;


		mask_base = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask_long = mask_base >> offset;

		data_long = m68ki_read_32(m68k, ea);
		m68k->n_flag = NFLAG_32(data_long << offset);
		m68k->not_z_flag = data_long & mask_long;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		m68ki_write_32(m68k, ea, data_long | mask_long);

		if((width + offset) > 32)
		{
			mask_byte = MASK_OUT_ABOVE_8(mask_base);
			data_byte = m68ki_read_8(m68k, ea+4);
			m68k->not_z_flag |= (data_byte & mask_byte);
			m68ki_write_8(m68k, ea+4, data_byte | mask_byte);
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bftst, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32* data = &DY;
		UINT64 mask;


		if(BIT_B(word2))
			offset = REG_D[offset&7];
		if(BIT_5(word2))
			width = REG_D[width&7];


		offset &= 31;
		width = ((width-1) & 31) + 1;


		mask = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask = ROR_32(mask, offset);

		m68k->n_flag = NFLAG_32(*data<<offset);
		m68k->not_z_flag = *data & mask;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bftst, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		INT32 offset = (word2>>6)&31;
		UINT32 width = word2;
		UINT32 mask_base;
		UINT32 data_long;
		UINT32 mask_long;
		UINT32 data_byte = 0;
		UINT32 mask_byte = 0;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;

		if(BIT_B(word2))
			offset = MAKE_INT_32(REG_D[offset&7]);
		if(BIT_5(word2))
			width = REG_D[width&7];

		/* Offset is signed so we have to use ugly math =( */
		ea += offset / 8;
		offset %= 8;
		if(offset < 0)
		{
			offset += 8;
			ea--;
		}
		width = ((width-1) & 31) + 1;


		mask_base = MASK_OUT_ABOVE_32(0xffffffff << (32 - width));
		mask_long = mask_base >> offset;

		data_long = m68ki_read_32(m68k, ea);
		m68k->n_flag = ((data_long & (0x80000000 >> offset))<<offset)>>24;
		m68k->not_z_flag = data_long & mask_long;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;

		if((width + offset) > 32)
		{
			mask_byte = MASK_OUT_ABOVE_8(mask_base);
			data_byte = m68ki_read_8(m68k, ea+4);
			m68k->not_z_flag |= (data_byte & mask_byte);
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bkpt, 0, ., .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		if (m68k->bkpt_ack_callback != NULL)
			(*m68k->bkpt_ack_callback)(m68k->device, CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type) ? m68k->ir & 7 : 0);
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(bra, 8, ., .)
{
	m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
	m68ki_branch_8(m68k, MASK_OUT_ABOVE_8(m68k->ir));
	if(REG_PC == REG_PPC && m68k->remaining_cycles > 0)
		m68k->remaining_cycles = 0;
}


M68KMAKE_OP(bra, 16, ., .)
{
	UINT32 offset = OPER_I_16(m68k);
	REG_PC -= 2;
	m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
	m68ki_branch_16(m68k, offset);
	if(REG_PC == REG_PPC && m68k->remaining_cycles > 0)
		m68k->remaining_cycles = 0;
}


M68KMAKE_OP(bra, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 offset = OPER_I_32(m68k);
		REG_PC -= 4;
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_branch_32(m68k, offset);
		if(REG_PC == REG_PPC && m68k->remaining_cycles > 0)
			m68k->remaining_cycles = 0;
		return;
	}
	else
	{
		m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
		m68ki_branch_8(m68k, MASK_OUT_ABOVE_8(m68k->ir));
		if(REG_PC == REG_PPC && m68k->remaining_cycles > 0)
			m68k->remaining_cycles = 0;
	}
}


M68KMAKE_OP(bset, 32, r, d)
{
	UINT32* r_dst = &DY;
	UINT32 mask = 1 << (DX & 0x1f);

	m68k->not_z_flag = *r_dst & mask;
	*r_dst |= mask;
}


M68KMAKE_OP(bset, 8, r, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);
	UINT32 mask = 1 << (DX & 7);

	m68k->not_z_flag = src & mask;
	m68ki_write_8(m68k, ea, src | mask);
}


M68KMAKE_OP(bset, 32, s, d)
{
	UINT32* r_dst = &DY;
	UINT32 mask = 1 << (OPER_I_8(m68k) & 0x1f);

	m68k->not_z_flag = *r_dst & mask;
	*r_dst |= mask;
}


M68KMAKE_OP(bset, 8, s, .)
{
	UINT32 mask = 1 << (OPER_I_8(m68k) & 7);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);

	m68k->not_z_flag = src & mask;
	m68ki_write_8(m68k, ea, src | mask);
}


M68KMAKE_OP(bsr, 8, ., .)
{
	m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
	m68ki_push_32(m68k, REG_PC);
	m68ki_branch_8(m68k, MASK_OUT_ABOVE_8(m68k->ir));
}


M68KMAKE_OP(bsr, 16, ., .)
{
	UINT32 offset = OPER_I_16(m68k);
	m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
	m68ki_push_32(m68k, REG_PC);
	REG_PC -= 2;
	m68ki_branch_16(m68k, offset);
}


M68KMAKE_OP(bsr, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 offset = OPER_I_32(m68k);
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_push_32(m68k, REG_PC);
		REG_PC -= 4;
		m68ki_branch_32(m68k, offset);
		return;
	}
	else
	{
		m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
		m68ki_push_32(m68k, REG_PC);
		m68ki_branch_8(m68k, MASK_OUT_ABOVE_8(m68k->ir));
	}
}


M68KMAKE_OP(btst, 32, r, d)
{
	m68k->not_z_flag = DY & (1 << (DX & 0x1f));
}


M68KMAKE_OP(btst, 8, r, .)
{
	m68k->not_z_flag = M68KMAKE_GET_OPER_AY_8 & (1 << (DX & 7));
}


M68KMAKE_OP(btst, 32, s, d)
{
	m68k->not_z_flag = DY & (1 << (OPER_I_8(m68k) & 0x1f));
}


M68KMAKE_OP(btst, 8, s, .)
{
	UINT32 bit = OPER_I_8(m68k) & 7;

	m68k->not_z_flag = M68KMAKE_GET_OPER_AY_8 & (1 << bit);
}


M68KMAKE_OP(callm, 32, ., .)
{
	/* note: watch out for pcrelative modes */
	if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
	{
		UINT32 ea = M68KMAKE_GET_EA_AY_32;

		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		REG_PC += 2;
(void)ea;	/* just to avoid an 'unused variable' warning */
		logerror("%s at %08x: called unimplemented instruction %04x (callm)\n",
					 m68k->device->tag, REG_PC - 2, m68k->ir);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cas, 8, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 ea = M68KMAKE_GET_EA_AY_8;
		UINT32 dest = m68ki_read_8(m68k, ea);
		UINT32* compare = &REG_D[word2 & 7];
		UINT32 res = dest - MASK_OUT_ABOVE_8(*compare);

		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
		m68k->v_flag = VFLAG_SUB_8(*compare, dest, res);
		m68k->c_flag = CFLAG_8(res);

		if(COND_NE(m68k))
			*compare = MASK_OUT_BELOW_8(*compare) | dest;
		else
		{
			m68k->remaining_cycles -= 3;
			m68ki_write_8(m68k, ea, MASK_OUT_ABOVE_8(REG_D[(word2 >> 6) & 7]));
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cas, 16, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 ea = M68KMAKE_GET_EA_AY_16;
		UINT32 dest = m68ki_read_16(m68k, ea);
		UINT32* compare = &REG_D[word2 & 7];
		UINT32 res = dest - MASK_OUT_ABOVE_16(*compare);

		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
		m68k->v_flag = VFLAG_SUB_16(*compare, dest, res);
		m68k->c_flag = CFLAG_16(res);

		if(COND_NE(m68k))
			*compare = MASK_OUT_BELOW_16(*compare) | dest;
		else
		{
			m68k->remaining_cycles -= 3;
			m68ki_write_16(m68k, ea, MASK_OUT_ABOVE_16(REG_D[(word2 >> 6) & 7]));
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cas, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 ea = M68KMAKE_GET_EA_AY_32;
		UINT32 dest = m68ki_read_32(m68k, ea);
		UINT32* compare = &REG_D[word2 & 7];
		UINT32 res = dest - *compare;

		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
		m68k->v_flag = VFLAG_SUB_32(*compare, dest, res);
		m68k->c_flag = CFLAG_SUB_32(*compare, dest, res);

		if(COND_NE(m68k))
			*compare = dest;
		else
		{
			m68k->remaining_cycles -= 3;
			m68ki_write_32(m68k, ea, REG_D[(word2 >> 6) & 7]);
		}
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cas2, 16, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_32(m68k);
		UINT32* compare1 = &REG_D[(word2 >> 16) & 7];
		UINT32 ea1 = REG_DA[(word2 >> 28) & 15];
		UINT32 dest1 = m68ki_read_16(m68k, ea1);
		UINT32 res1 = dest1 - MASK_OUT_ABOVE_16(*compare1);
		UINT32* compare2 = &REG_D[word2 & 7];
		UINT32 ea2 = REG_DA[(word2 >> 12) & 15];
		UINT32 dest2 = m68ki_read_16(m68k, ea2);
		UINT32 res2;

		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68k->n_flag = NFLAG_16(res1);
		m68k->not_z_flag = MASK_OUT_ABOVE_16(res1);
		m68k->v_flag = VFLAG_SUB_16(*compare1, dest1, res1);
		m68k->c_flag = CFLAG_16(res1);

		if(COND_EQ(m68k))
		{
			res2 = dest2 - MASK_OUT_ABOVE_16(*compare2);

			m68k->n_flag = NFLAG_16(res2);
			m68k->not_z_flag = MASK_OUT_ABOVE_16(res2);
			m68k->v_flag = VFLAG_SUB_16(*compare2, dest2, res2);
			m68k->c_flag = CFLAG_16(res2);

			if(COND_EQ(m68k))
			{
				m68k->remaining_cycles -= 3;
				m68ki_write_16(m68k, ea1, REG_D[(word2 >> 22) & 7]);
				m68ki_write_16(m68k, ea2, REG_D[(word2 >> 6) & 7]);
				return;
			}
		}
		*compare1 = BIT_1F(word2) ? MAKE_INT_16(dest1) : MASK_OUT_BELOW_16(*compare1) | dest1;
		*compare2 = BIT_F(word2) ? MAKE_INT_16(dest2) : MASK_OUT_BELOW_16(*compare2) | dest2;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cas2, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_32(m68k);
		UINT32* compare1 = &REG_D[(word2 >> 16) & 7];
		UINT32 ea1 = REG_DA[(word2 >> 28) & 15];
		UINT32 dest1 = m68ki_read_32(m68k, ea1);
		UINT32 res1 = dest1 - *compare1;
		UINT32* compare2 = &REG_D[word2 & 7];
		UINT32 ea2 = REG_DA[(word2 >> 12) & 15];
		UINT32 dest2 = m68ki_read_32(m68k, ea2);
		UINT32 res2;

		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68k->n_flag = NFLAG_32(res1);
		m68k->not_z_flag = MASK_OUT_ABOVE_32(res1);
		m68k->v_flag = VFLAG_SUB_32(*compare1, dest1, res1);
		m68k->c_flag = CFLAG_SUB_32(*compare1, dest1, res1);

		if(COND_EQ(m68k))
		{
			res2 = dest2 - *compare2;

			m68k->n_flag = NFLAG_32(res2);
			m68k->not_z_flag = MASK_OUT_ABOVE_32(res2);
			m68k->v_flag = VFLAG_SUB_32(*compare2, dest2, res2);
			m68k->c_flag = CFLAG_SUB_32(*compare2, dest2, res2);

			if(COND_EQ(m68k))
			{
				m68k->remaining_cycles -= 3;
				m68ki_write_32(m68k, ea1, REG_D[(word2 >> 22) & 7]);
				m68ki_write_32(m68k, ea2, REG_D[(word2 >> 6) & 7]);
				return;
			}
		}
		*compare1 = dest1;
		*compare2 = dest2;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk, 16, ., d)
{
	INT32 src = MAKE_INT_16(DX);
	INT32 bound = MAKE_INT_16(DY);

	m68k->not_z_flag = ZFLAG_16(src); /* Undocumented */
	m68k->v_flag = VFLAG_CLEAR;   /* Undocumented */
	m68k->c_flag = CFLAG_CLEAR;   /* Undocumented */

	if(src >= 0 && src <= bound)
	{
		return;
	}
	m68k->n_flag = (src < 0)<<7;
	m68ki_exception_trap(m68k, EXCEPTION_CHK);
}


M68KMAKE_OP(chk, 16, ., .)
{
	INT32 src = MAKE_INT_16(DX);
	INT32 bound = MAKE_INT_16(M68KMAKE_GET_OPER_AY_16);

	m68k->not_z_flag = ZFLAG_16(src); /* Undocumented */
	m68k->v_flag = VFLAG_CLEAR;   /* Undocumented */
	m68k->c_flag = CFLAG_CLEAR;   /* Undocumented */

	if(src >= 0 && src <= bound)
	{
		return;
	}
	m68k->n_flag = (src < 0)<<7;
	m68ki_exception_trap(m68k, EXCEPTION_CHK);
}


M68KMAKE_OP(chk, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		INT32 src = MAKE_INT_32(DX);
		INT32 bound = MAKE_INT_32(DY);

		m68k->not_z_flag = ZFLAG_32(src); /* Undocumented */
		m68k->v_flag = VFLAG_CLEAR;   /* Undocumented */
		m68k->c_flag = CFLAG_CLEAR;   /* Undocumented */

		if(src >= 0 && src <= bound)
		{
			return;
		}
		m68k->n_flag = (src < 0)<<7;
		m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		INT32 src = MAKE_INT_32(DX);
		INT32 bound = MAKE_INT_32(M68KMAKE_GET_OPER_AY_32);

		m68k->not_z_flag = ZFLAG_32(src); /* Undocumented */
		m68k->v_flag = VFLAG_CLEAR;   /* Undocumented */
		m68k->c_flag = CFLAG_CLEAR;   /* Undocumented */

		if(src >= 0 && src <= bound)
		{
			return;
		}
		m68k->n_flag = (src < 0)<<7;
		m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 8, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15]&0xff;
		UINT32 ea = EA_PCDI_8(m68k);
		UINT32 lower_bound = m68ki_read_pcrel_8(m68k, ea);
		UINT32 upper_bound = m68ki_read_pcrel_8(m68k, ea + 1);

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_8(compare) - MAKE_INT_8(lower_bound);
		else
			m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		m68k->c_flag = upper_bound - compare;
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 8, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15]&0xff;
		UINT32 ea = EA_PCIX_8(m68k);
		UINT32 lower_bound = m68ki_read_pcrel_8(m68k, ea);
		UINT32 upper_bound = m68ki_read_pcrel_8(m68k, ea + 1);

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_8(compare) - MAKE_INT_8(lower_bound);
		else
			m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		m68k->c_flag = upper_bound - compare;
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 8, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15]&0xff;
		UINT32 ea = M68KMAKE_GET_EA_AY_8;
		UINT32 lower_bound = m68ki_read_8(m68k, ea);
		UINT32 upper_bound = m68ki_read_8(m68k, ea + 1);

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_8(compare) - MAKE_INT_8(lower_bound);
		else
			m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		m68k->c_flag = upper_bound - compare;
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 16, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15]&0xffff;
		UINT32 ea = EA_PCDI_16(m68k);
		UINT32 lower_bound = m68ki_read_pcrel_16(m68k, ea);
		UINT32 upper_bound = m68ki_read_pcrel_16(m68k, ea + 2);

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_16(compare) - MAKE_INT_16(lower_bound);
		else
			m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		m68k->c_flag = CFLAG_16(m68k->c_flag);
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_16(upper_bound) - MAKE_INT_16(compare);
		else
			m68k->c_flag = upper_bound - compare;
		m68k->c_flag = CFLAG_16(m68k->c_flag);
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 16, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15]&0xffff;
		UINT32 ea = EA_PCIX_16(m68k);
		UINT32 lower_bound = m68ki_read_pcrel_16(m68k, ea);
		UINT32 upper_bound = m68ki_read_pcrel_16(m68k, ea + 2);

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_16(compare) - MAKE_INT_16(lower_bound);
		else
			m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		m68k->c_flag = CFLAG_16(m68k->c_flag);
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_16(upper_bound) - MAKE_INT_16(compare);
		else
			m68k->c_flag = upper_bound - compare;
		m68k->c_flag = CFLAG_16(m68k->c_flag);
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 16, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15]&0xffff;
		UINT32 ea = M68KMAKE_GET_EA_AY_16;
		UINT32 lower_bound = m68ki_read_16(m68k, ea);
		UINT32 upper_bound = m68ki_read_16(m68k, ea + 2);

		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_16(compare) - MAKE_INT_16(lower_bound);
		else
			m68k->c_flag = compare - lower_bound;

		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		m68k->c_flag = CFLAG_16(m68k->c_flag);
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}
		if(!BIT_F(word2))
			m68k->c_flag = MAKE_INT_16(upper_bound) - MAKE_INT_16(compare);
		else
			m68k->c_flag = upper_bound - compare;

		m68k->c_flag = CFLAG_16(m68k->c_flag);
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 32, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15];
		UINT32 ea = EA_PCDI_32(m68k);
		UINT32 lower_bound = m68ki_read_pcrel_32(m68k, ea);
		UINT32 upper_bound = m68ki_read_pcrel_32(m68k, ea + 4);

		m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		m68k->c_flag = CFLAG_SUB_32(lower_bound, compare, m68k->c_flag);
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		m68k->c_flag = upper_bound - compare;
		m68k->c_flag = CFLAG_SUB_32(compare, upper_bound, m68k->c_flag);
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 32, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15];
		UINT32 ea = EA_PCIX_32(m68k);
		UINT32 lower_bound = m68ki_read_pcrel_32(m68k, ea);
		UINT32 upper_bound = m68ki_read_pcrel_32(m68k, ea + 4);

		m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		m68k->c_flag = CFLAG_SUB_32(lower_bound, compare, m68k->c_flag);
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		m68k->c_flag = upper_bound - compare;
		m68k->c_flag = CFLAG_SUB_32(compare, upper_bound, m68k->c_flag);
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(chk2cmp2, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT32 compare = REG_DA[(word2 >> 12) & 15];
		UINT32 ea = M68KMAKE_GET_EA_AY_32;
		UINT32 lower_bound = m68ki_read_32(m68k, ea);
		UINT32 upper_bound = m68ki_read_32(m68k, ea + 4);

		m68k->c_flag = compare - lower_bound;
		m68k->not_z_flag = !((upper_bound==compare) | (lower_bound==compare));
		m68k->c_flag = CFLAG_SUB_32(lower_bound, compare, m68k->c_flag);
		if(COND_CS(m68k))
		{
			if(BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
			return;
		}

		m68k->c_flag = upper_bound - compare;
		m68k->c_flag = CFLAG_SUB_32(compare, upper_bound, m68k->c_flag);
		if(COND_CS(m68k) && BIT_B(word2))
				m68ki_exception_trap(m68k, EXCEPTION_CHK);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(clr, 8, ., d)
{
	DY &= 0xffffff00;

	m68k->n_flag = NFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->not_z_flag = ZFLAG_SET;
}


M68KMAKE_OP(clr, 8, ., .)
{
	m68ki_write_8(m68k, M68KMAKE_GET_EA_AY_8, 0);

	m68k->n_flag = NFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->not_z_flag = ZFLAG_SET;
}


M68KMAKE_OP(clr, 16, ., d)
{
	DY &= 0xffff0000;

	m68k->n_flag = NFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->not_z_flag = ZFLAG_SET;
}


M68KMAKE_OP(clr, 16, ., .)
{
	m68ki_write_16(m68k, M68KMAKE_GET_EA_AY_16, 0);

	m68k->n_flag = NFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->not_z_flag = ZFLAG_SET;
}


M68KMAKE_OP(clr, 32, ., d)
{
	DY = 0;

	m68k->n_flag = NFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->not_z_flag = ZFLAG_SET;
}


M68KMAKE_OP(clr, 32, ., .)
{
	m68ki_write_32(m68k, M68KMAKE_GET_EA_AY_32, 0);

	m68k->n_flag = NFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->not_z_flag = ZFLAG_SET;
}


M68KMAKE_OP(cmp, 8, ., d)
{
	UINT32 src = MASK_OUT_ABOVE_8(DY);
	UINT32 dst = MASK_OUT_ABOVE_8(DX);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmp, 8, ., .)
{
	UINT32 src = M68KMAKE_GET_OPER_AY_8;
	UINT32 dst = MASK_OUT_ABOVE_8(DX);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmp, 16, ., d)
{
	UINT32 src = MASK_OUT_ABOVE_16(DY);
	UINT32 dst = MASK_OUT_ABOVE_16(DX);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->c_flag = CFLAG_16(res);
}


M68KMAKE_OP(cmp, 16, ., a)
{
	UINT32 src = MASK_OUT_ABOVE_16(AY);
	UINT32 dst = MASK_OUT_ABOVE_16(DX);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->c_flag = CFLAG_16(res);
}


M68KMAKE_OP(cmp, 16, ., .)
{
	UINT32 src = M68KMAKE_GET_OPER_AY_16;
	UINT32 dst = MASK_OUT_ABOVE_16(DX);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->c_flag = CFLAG_16(res);
}


M68KMAKE_OP(cmp, 32, ., d)
{
	UINT32 src = DY;
	UINT32 dst = DX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmp, 32, ., a)
{
	UINT32 src = AY;
	UINT32 dst = DX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmp, 32, ., .)
{
	UINT32 src = M68KMAKE_GET_OPER_AY_32;
	UINT32 dst = DX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpa, 16, ., d)
{
	UINT32 src = MAKE_INT_16(DY);
	UINT32 dst = AX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpa, 16, ., a)
{
	UINT32 src = MAKE_INT_16(AY);
	UINT32 dst = AX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpa, 16, ., .)
{
	UINT32 src = MAKE_INT_16(M68KMAKE_GET_OPER_AY_16);
	UINT32 dst = AX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpa, 32, ., d)
{
	UINT32 src = DY;
	UINT32 dst = AX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpa, 32, ., a)
{
	UINT32 src = AY;
	UINT32 dst = AX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpa, 32, ., .)
{
	UINT32 src = M68KMAKE_GET_OPER_AY_32;
	UINT32 dst = AX;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpi, 8, ., d)
{
	UINT32 src = OPER_I_8(m68k);
	UINT32 dst = MASK_OUT_ABOVE_8(DY);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmpi, 8, ., .)
{
	UINT32 src = OPER_I_8(m68k);
	UINT32 dst = M68KMAKE_GET_OPER_AY_8;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmpi, 8, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 src = OPER_I_8(m68k);
		UINT32 dst = OPER_PCDI_8(m68k);
		UINT32 res = dst - src;

		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
		m68k->v_flag = VFLAG_SUB_8(src, dst, res);
		m68k->c_flag = CFLAG_8(res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cmpi, 8, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 src = OPER_I_8(m68k);
		UINT32 dst = OPER_PCIX_8(m68k);
		UINT32 res = dst - src;

		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
		m68k->v_flag = VFLAG_SUB_8(src, dst, res);
		m68k->c_flag = CFLAG_8(res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cmpi, 16, ., d)
{
	UINT32 src = OPER_I_16(m68k);
	UINT32 dst = MASK_OUT_ABOVE_16(DY);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->c_flag = CFLAG_16(res);
}


M68KMAKE_OP(cmpi, 16, ., .)
{
	UINT32 src = OPER_I_16(m68k);
	UINT32 dst = M68KMAKE_GET_OPER_AY_16;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->c_flag = CFLAG_16(res);
}


M68KMAKE_OP(cmpi, 16, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 src = OPER_I_16(m68k);
		UINT32 dst = OPER_PCDI_16(m68k);
		UINT32 res = dst - src;

		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
		m68k->v_flag = VFLAG_SUB_16(src, dst, res);
		m68k->c_flag = CFLAG_16(res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cmpi, 16, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 src = OPER_I_16(m68k);
		UINT32 dst = OPER_PCIX_16(m68k);
		UINT32 res = dst - src;

		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
		m68k->v_flag = VFLAG_SUB_16(src, dst, res);
		m68k->c_flag = CFLAG_16(res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cmpi, 32, ., d)
{
	UINT32 src = OPER_I_32(m68k);
	UINT32 dst = DY;
	UINT32 res = dst - src;

	if (m68k->cmpild_instr_callback != NULL)
		(*m68k->cmpild_instr_callback)(m68k->device, src, m68k->ir & 7);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpi, 32, ., .)
{
	UINT32 src = OPER_I_32(m68k);
	UINT32 dst = M68KMAKE_GET_OPER_AY_32;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cmpi, 32, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 src = OPER_I_32(m68k);
		UINT32 dst = OPER_PCDI_32(m68k);
		UINT32 res = dst - src;

		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
		m68k->v_flag = VFLAG_SUB_32(src, dst, res);
		m68k->c_flag = CFLAG_SUB_32(src, dst, res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cmpi, 32, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 src = OPER_I_32(m68k);
		UINT32 dst = OPER_PCIX_32(m68k);
		UINT32 res = dst - src;

		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
		m68k->v_flag = VFLAG_SUB_32(src, dst, res);
		m68k->c_flag = CFLAG_SUB_32(src, dst, res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(cmpm, 8, ., ax7)
{
	UINT32 src = OPER_AY_PI_8(m68k);
	UINT32 dst = OPER_A7_PI_8(m68k);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmpm, 8, ., ay7)
{
	UINT32 src = OPER_A7_PI_8(m68k);
	UINT32 dst = OPER_AX_PI_8(m68k);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmpm, 8, ., axy7)
{
	UINT32 src = OPER_A7_PI_8(m68k);
	UINT32 dst = OPER_A7_PI_8(m68k);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmpm, 8, ., .)
{
	UINT32 src = OPER_AY_PI_8(m68k);
	UINT32 dst = OPER_AX_PI_8(m68k);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->c_flag = CFLAG_8(res);
}


M68KMAKE_OP(cmpm, 16, ., .)
{
	UINT32 src = OPER_AY_PI_16(m68k);
	UINT32 dst = OPER_AX_PI_16(m68k);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->c_flag = CFLAG_16(res);
}


M68KMAKE_OP(cmpm, 32, ., .)
{
	UINT32 src = OPER_AY_PI_32(m68k);
	UINT32 dst = OPER_AX_PI_32(m68k);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->c_flag = CFLAG_SUB_32(src, dst, res);
}


M68KMAKE_OP(cpbcc, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		logerror( "%s at %08x: called unimplemented instruction %04x (cpbcc)\n",
					 m68k->device->tag, REG_PC - 2, m68k->ir);
		return;
	}
	m68ki_exception_1111(m68k);
}


M68KMAKE_OP(cpdbcc, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		logerror("%s at %08x: called unimplemented instruction %04x (cpdbcc)\n",
					 m68k->device->tag, REG_PC - 2, m68k->ir);
		return;
	}
	m68ki_exception_1111(m68k);
}


M68KMAKE_OP(cpgen, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		logerror("%s at %08x: called unimplemented instruction %04x (cpgen)\n",
					 m68k->device->tag, REG_PC - 2, m68k->ir);
		return;
	}
	m68ki_exception_1111(m68k);
}


M68KMAKE_OP(cpscc, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		logerror("%s at %08x: called unimplemented instruction %04x (cpscc)\n",
					 m68k->device->tag, REG_PC - 2, m68k->ir);
		return;
	}
	m68ki_exception_1111(m68k);
}


M68KMAKE_OP(cptrapcc, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		logerror("%s at %08x: called unimplemented instruction %04x (cptrapcc)\n",
					 m68k->device->tag, REG_PC - 2, m68k->ir);
		return;
	}
	m68ki_exception_1111(m68k);
}


M68KMAKE_OP(dbt, 16, ., .)
{
	REG_PC += 2;
}


M68KMAKE_OP(dbf, 16, ., .)
{
	UINT32* r_dst = &DY;
	UINT32 res = MASK_OUT_ABOVE_16(*r_dst - 1);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
	if(res != 0xffff)
	{
		UINT32 offset = OPER_I_16(m68k);
		REG_PC -= 2;
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_branch_16(m68k, offset);
		m68k->remaining_cycles -= m68k->cyc_dbcc_f_noexp;
		return;
	}
	REG_PC += 2;
	m68k->remaining_cycles -= m68k->cyc_dbcc_f_exp;
}


M68KMAKE_OP(dbcc, 16, ., .)
{
	if(M68KMAKE_NOT_CC)
	{
		UINT32* r_dst = &DY;
		UINT32 res = MASK_OUT_ABOVE_16(*r_dst - 1);

		*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
		if(res != 0xffff)
		{
			UINT32 offset = OPER_I_16(m68k);
			REG_PC -= 2;
			m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
			m68ki_branch_16(m68k, offset);
			m68k->remaining_cycles -= m68k->cyc_dbcc_f_noexp;
			return;
		}
		REG_PC += 2;
		m68k->remaining_cycles -= m68k->cyc_dbcc_f_exp;
		return;
	}
	REG_PC += 2;
}


M68KMAKE_OP(divs, 16, ., d)
{
	UINT32* r_dst = &DX;
	INT32 src = MAKE_INT_16(DY);
	INT32 quotient;
	INT32 remainder;

	if(src != 0)
	{
		if((UINT32)*r_dst == 0x80000000 && src == -1)
		{
			m68k->not_z_flag = 0;
			m68k->n_flag = NFLAG_CLEAR;
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			*r_dst = 0;
			return;
		}

		quotient = MAKE_INT_32(*r_dst) / src;
		remainder = MAKE_INT_32(*r_dst) % src;

		if(quotient == MAKE_INT_16(quotient))
		{
			m68k->not_z_flag = quotient;
			m68k->n_flag = NFLAG_16(quotient);
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			*r_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
			return;
		}
		m68k->v_flag = VFLAG_SET;
		return;
	}
	m68ki_exception_trap(m68k, EXCEPTION_ZERO_DIVIDE);
}


M68KMAKE_OP(divs, 16, ., .)
{
	UINT32* r_dst = &DX;
	INT32 src = MAKE_INT_16(M68KMAKE_GET_OPER_AY_16);
	INT32 quotient;
	INT32 remainder;

	if(src != 0)
	{
		if((UINT32)*r_dst == 0x80000000 && src == -1)
		{
			m68k->not_z_flag = 0;
			m68k->n_flag = NFLAG_CLEAR;
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			*r_dst = 0;
			return;
		}

		quotient = MAKE_INT_32(*r_dst) / src;
		remainder = MAKE_INT_32(*r_dst) % src;

		if(quotient == MAKE_INT_16(quotient))
		{
			m68k->not_z_flag = quotient;
			m68k->n_flag = NFLAG_16(quotient);
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			*r_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
			return;
		}
		m68k->v_flag = VFLAG_SET;
		return;
	}
	m68ki_exception_trap(m68k, EXCEPTION_ZERO_DIVIDE);
}


M68KMAKE_OP(divu, 16, ., d)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_16(DY);

	if(src != 0)
	{
		UINT32 quotient = *r_dst / src;
		UINT32 remainder = *r_dst % src;

		if(quotient < 0x10000)
		{
			m68k->not_z_flag = quotient;
			m68k->n_flag = NFLAG_16(quotient);
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			*r_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
			return;
		}
		m68k->v_flag = VFLAG_SET;
		return;
	}
	m68ki_exception_trap(m68k, EXCEPTION_ZERO_DIVIDE);
}


M68KMAKE_OP(divu, 16, ., .)
{
	UINT32* r_dst = &DX;
	UINT32 src = M68KMAKE_GET_OPER_AY_16;

	if(src != 0)
	{
		UINT32 quotient = *r_dst / src;
		UINT32 remainder = *r_dst % src;

		if(quotient < 0x10000)
		{
			m68k->not_z_flag = quotient;
			m68k->n_flag = NFLAG_16(quotient);
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			*r_dst = MASK_OUT_ABOVE_32(MASK_OUT_ABOVE_16(quotient) | (remainder << 16));
			return;
		}
		m68k->v_flag = VFLAG_SET;
		return;
	}
	m68ki_exception_trap(m68k, EXCEPTION_ZERO_DIVIDE);
}


M68KMAKE_OP(divl, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT64 divisor   = DY;
		UINT64 dividend  = 0;
		UINT64 quotient  = 0;
		UINT64 remainder = 0;

		if(divisor != 0)
		{
			if(BIT_A(word2))	/* 64 bit */
			{
				dividend = REG_D[word2 & 7];
				dividend <<= 32;
				dividend |= REG_D[(word2 >> 12) & 7];

				if(BIT_B(word2))	   /* signed */
				{
					quotient  = (UINT64)((INT64)dividend / (INT64)((INT32)divisor));
					remainder = (UINT64)((INT64)dividend % (INT64)((INT32)divisor));
					if((INT64)quotient != (INT64)((INT32)quotient))
					{
						m68k->v_flag = VFLAG_SET;
						return;
					}
				}
				else					/* unsigned */
				{
					quotient = dividend / divisor;
					if(quotient > 0xffffffff)
					{
						m68k->v_flag = VFLAG_SET;
						return;
					}
					remainder = dividend % divisor;
				}
			}
			else	/* 32 bit */
			{
				dividend = REG_D[(word2 >> 12) & 7];
				if(BIT_B(word2))	   /* signed */
				{
					quotient  = (UINT64)((INT64)((INT32)dividend) / (INT64)((INT32)divisor));
					remainder = (UINT64)((INT64)((INT32)dividend) % (INT64)((INT32)divisor));
				}
				else					/* unsigned */
				{
					quotient = dividend / divisor;
					remainder = dividend % divisor;
				}
			}

			REG_D[word2 & 7] = remainder;
			REG_D[(word2 >> 12) & 7] = quotient;

			m68k->n_flag = NFLAG_32(quotient);
			m68k->not_z_flag = quotient;
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			return;
		}
		m68ki_exception_trap(m68k, EXCEPTION_ZERO_DIVIDE);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(divl, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT64 divisor = M68KMAKE_GET_OPER_AY_32;
		UINT64 dividend  = 0;
		UINT64 quotient  = 0;
		UINT64 remainder = 0;

		if(divisor != 0)
		{
			if(BIT_A(word2))	/* 64 bit */
			{
				dividend = REG_D[word2 & 7];
				dividend <<= 32;
				dividend |= REG_D[(word2 >> 12) & 7];

				if(BIT_B(word2))	   /* signed */
				{
					quotient  = (UINT64)((INT64)dividend / (INT64)((INT32)divisor));
					remainder = (UINT64)((INT64)dividend % (INT64)((INT32)divisor));
					if((INT64)quotient != (INT64)((INT32)quotient))
					{
						m68k->v_flag = VFLAG_SET;
						return;
					}
				}
				else					/* unsigned */
				{
					quotient = dividend / divisor;
					if(quotient > 0xffffffff)
					{
						m68k->v_flag = VFLAG_SET;
						return;
					}
					remainder = dividend % divisor;
				}
			}
			else	/* 32 bit */
			{
				dividend = REG_D[(word2 >> 12) & 7];
				if(BIT_B(word2))	   /* signed */
				{
					quotient  = (UINT64)((INT64)((INT32)dividend) / (INT64)((INT32)divisor));
					remainder = (UINT64)((INT64)((INT32)dividend) % (INT64)((INT32)divisor));
				}
				else					/* unsigned */
				{
					quotient = dividend / divisor;
					remainder = dividend % divisor;
				}
			}

			REG_D[word2 & 7] = remainder;
			REG_D[(word2 >> 12) & 7] = quotient;

			m68k->n_flag = NFLAG_32(quotient);
			m68k->not_z_flag = quotient;
			m68k->v_flag = VFLAG_CLEAR;
			m68k->c_flag = CFLAG_CLEAR;
			return;
		}
		m68ki_exception_trap(m68k, EXCEPTION_ZERO_DIVIDE);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(eor, 8, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY ^= MASK_OUT_ABOVE_8(DX));

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eor, 8, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 res = MASK_OUT_ABOVE_8(DX ^ m68ki_read_8(m68k, ea));

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eor, 16, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY ^= MASK_OUT_ABOVE_16(DX));

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eor, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 res = MASK_OUT_ABOVE_16(DX ^ m68ki_read_16(m68k, ea));

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eor, 32, ., d)
{
	UINT32 res = DY ^= DX;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eor, 32, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 res = DX ^ m68ki_read_32(m68k, ea);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eori, 8, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY ^= OPER_I_8(m68k));

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eori, 8, ., .)
{
	UINT32 src = OPER_I_8(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 res = src ^ m68ki_read_8(m68k, ea);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eori, 16, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY ^= OPER_I_16(m68k));

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eori, 16, ., .)
{
	UINT32 src = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 res = src ^ m68ki_read_16(m68k, ea);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eori, 32, ., d)
{
	UINT32 res = DY ^= OPER_I_32(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eori, 32, ., .)
{
	UINT32 src = OPER_I_32(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 res = src ^ m68ki_read_32(m68k, ea);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(eori, 16, toc, .)
{
	m68ki_set_ccr(m68k, m68ki_get_ccr(m68k) ^ OPER_I_16(m68k));
}


M68KMAKE_OP(eori, 16, tos, .)
{
	if(m68k->s_flag)
	{
		UINT32 src = OPER_I_16(m68k);
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_set_sr(m68k, m68ki_get_sr(m68k) ^ src);
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(exg, 32, dd, .)
{
	UINT32* reg_a = &DX;
	UINT32* reg_b = &DY;
	UINT32 tmp = *reg_a;
	*reg_a = *reg_b;
	*reg_b = tmp;
}


M68KMAKE_OP(exg, 32, aa, .)
{
	UINT32* reg_a = &AX;
	UINT32* reg_b = &AY;
	UINT32 tmp = *reg_a;
	*reg_a = *reg_b;
	*reg_b = tmp;
}


M68KMAKE_OP(exg, 32, da, .)
{
	UINT32* reg_a = &DX;
	UINT32* reg_b = &AY;
	UINT32 tmp = *reg_a;
	*reg_a = *reg_b;
	*reg_b = tmp;
}


M68KMAKE_OP(ext, 16, ., .)
{
	UINT32* r_dst = &DY;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | MASK_OUT_ABOVE_8(*r_dst) | (GET_MSB_8(*r_dst) ? 0xff00 : 0);

	m68k->n_flag = NFLAG_16(*r_dst);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(*r_dst);
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(ext, 32, ., .)
{
	UINT32* r_dst = &DY;

	*r_dst = MASK_OUT_ABOVE_16(*r_dst) | (GET_MSB_16(*r_dst) ? 0xffff0000 : 0);

	m68k->n_flag = NFLAG_32(*r_dst);
	m68k->not_z_flag = *r_dst;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(extb, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32* r_dst = &DY;

		*r_dst = MASK_OUT_ABOVE_8(*r_dst) | (GET_MSB_8(*r_dst) ? 0xffffff00 : 0);

		m68k->n_flag = NFLAG_32(*r_dst);
		m68k->not_z_flag = *r_dst;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(illegal, 0, ., .)
{
	m68ki_exception_illegal(m68k);
}

M68KMAKE_OP(jmp, 32, ., .)
{
	m68ki_jump(m68k, M68KMAKE_GET_EA_AY_32);
	m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
	if(REG_PC == REG_PPC && m68k->remaining_cycles > 0)
		m68k->remaining_cycles = 0;
}


M68KMAKE_OP(jsr, 32, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
	m68ki_push_32(m68k, REG_PC);
	m68ki_jump(m68k, ea);
}


M68KMAKE_OP(lea, 32, ., .)
{
	AX = M68KMAKE_GET_EA_AY_32;
}


M68KMAKE_OP(link, 16, ., a7)
{
	REG_A[7] -= 4;
	m68ki_write_32(m68k, REG_A[7], REG_A[7]);
	REG_A[7] = MASK_OUT_ABOVE_32(REG_A[7] + MAKE_INT_16(OPER_I_16(m68k)));
}


M68KMAKE_OP(link, 16, ., .)
{
	UINT32* r_dst = &AY;

	m68ki_push_32(m68k, *r_dst);
	*r_dst = REG_A[7];
	REG_A[7] = MASK_OUT_ABOVE_32(REG_A[7] + MAKE_INT_16(OPER_I_16(m68k)));
}


M68KMAKE_OP(link, 32, ., a7)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		REG_A[7] -= 4;
		m68ki_write_32(m68k, REG_A[7], REG_A[7]);
		REG_A[7] = MASK_OUT_ABOVE_32(REG_A[7] + OPER_I_32(m68k));
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(link, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32* r_dst = &AY;

		m68ki_push_32(m68k, *r_dst);
		*r_dst = REG_A[7];
		REG_A[7] = MASK_OUT_ABOVE_32(REG_A[7] + OPER_I_32(m68k));
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(lsr, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_CLEAR;
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src << (9-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsr, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_CLEAR;
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src << (9-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsr, 32, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = *r_dst;
	UINT32 res = src >> shift;

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = res;

	m68k->n_flag = NFLAG_CLEAR;
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src << (9-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsr, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift <= 8)
		{
			*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
			m68k->x_flag = m68k->c_flag = src << (9-shift);
			m68k->n_flag = NFLAG_CLEAR;
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst &= 0xffffff00;
		m68k->x_flag = XFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_8(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsr, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = src >> shift;

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift <= 16)
		{
			*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
			m68k->c_flag = m68k->x_flag = (src >> (shift - 1))<<8;
			m68k->n_flag = NFLAG_CLEAR;
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst &= 0xffff0000;
		m68k->x_flag = XFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_16(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsr, 32, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = *r_dst;
	UINT32 res = src >> shift;

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 32)
		{
			*r_dst = res;
			m68k->c_flag = m68k->x_flag = (src >> (shift - 1))<<8;
			m68k->n_flag = NFLAG_CLEAR;
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst = 0;
		m68k->x_flag = m68k->c_flag = (shift == 32 ? GET_MSB_32(src)>>23 : 0);
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_32(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsr, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = src >> 1;

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_CLEAR;
	m68k->not_z_flag = res;
	m68k->c_flag = m68k->x_flag = src << 8;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsl, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_8(src << shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src << shift;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsl, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_16(src << shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src >> (8-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsl, 32, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = *r_dst;
	UINT32 res = MASK_OUT_ABOVE_32(src << shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src >> (24-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsl, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_8(src << shift);

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift <= 8)
		{
			*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
			m68k->x_flag = m68k->c_flag = src << shift;
			m68k->n_flag = NFLAG_8(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst &= 0xffffff00;
		m68k->x_flag = XFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_8(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsl, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_16(src << shift);

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift <= 16)
		{
			*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
			m68k->x_flag = m68k->c_flag = (src << shift) >> 8;
			m68k->n_flag = NFLAG_16(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst &= 0xffff0000;
		m68k->x_flag = XFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_16(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsl, 32, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = DX & 0x3f;
	UINT32 src = *r_dst;
	UINT32 res = MASK_OUT_ABOVE_32(src << shift);

	if(shift != 0)
	{
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

		if(shift < 32)
		{
			*r_dst = res;
			m68k->x_flag = m68k->c_flag = (src >> (32 - shift)) << 8;
			m68k->n_flag = NFLAG_32(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}

		*r_dst = 0;
		m68k->x_flag = m68k->c_flag = ((shift == 32 ? src & 1 : 0))<<8;
		m68k->n_flag = NFLAG_CLEAR;
		m68k->not_z_flag = ZFLAG_SET;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_32(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(lsl, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = MASK_OUT_ABOVE_16(src << 1);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->x_flag = m68k->c_flag = src >> 7;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, d, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32* r_dst = &DX;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, d, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32* r_dst = &DX;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, ai, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_AX_AI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, ai, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_AX_AI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pi7, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_A7_PI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pi, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_AX_PI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pi7, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_A7_PI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pi, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_AX_PI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pd7, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_A7_PD_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pd, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_AX_PD_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pd7, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_A7_PD_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, pd, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_AX_PD_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, di, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_AX_DI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, di, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_AX_DI_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, ix, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_AX_IX_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, ix, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_AX_IX_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, aw, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_AW_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, aw, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_AW_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, al, d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);
	UINT32 ea = EA_AL_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 8, al, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;
	UINT32 ea = EA_AL_8(m68k);

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, d, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32* r_dst = &DX;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, d, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32* r_dst = &DX;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, d, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32* r_dst = &DX;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, ai, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32 ea = EA_AX_AI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, ai, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32 ea = EA_AX_AI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, ai, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32 ea = EA_AX_AI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, pi, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32 ea = EA_AX_PI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, pi, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32 ea = EA_AX_PI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, pi, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32 ea = EA_AX_PI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, pd, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32 ea = EA_AX_PD_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, pd, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32 ea = EA_AX_PD_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, pd, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32 ea = EA_AX_PD_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, di, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32 ea = EA_AX_DI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, di, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32 ea = EA_AX_DI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, di, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32 ea = EA_AX_DI_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, ix, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32 ea = EA_AX_IX_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, ix, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32 ea = EA_AX_IX_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, ix, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32 ea = EA_AX_IX_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, aw, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32 ea = EA_AW_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, aw, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32 ea = EA_AW_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, aw, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32 ea = EA_AW_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, al, d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);
	UINT32 ea = EA_AL_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, al, a)
{
	UINT32 res = MASK_OUT_ABOVE_16(AY);
	UINT32 ea = EA_AL_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 16, al, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;
	UINT32 ea = EA_AL_16(m68k);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, d, d)
{
	UINT32 res = DY;
	UINT32* r_dst = &DX;

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, d, a)
{
	UINT32 res = AY;
	UINT32* r_dst = &DX;

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, d, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32* r_dst = &DX;

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, ai, d)
{
	UINT32 res = DY;
	UINT32 ea = EA_AX_AI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, ai, a)
{
	UINT32 res = AY;
	UINT32 ea = EA_AX_AI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, ai, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32 ea = EA_AX_AI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, pi, d)
{
	UINT32 res = DY;
	UINT32 ea = EA_AX_PI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, pi, a)
{
	UINT32 res = AY;
	UINT32 ea = EA_AX_PI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, pi, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32 ea = EA_AX_PI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, pd, d)
{
	UINT32 res = DY;
	UINT32 ea = EA_AX_PD_32(m68k);

	m68ki_write_16(m68k, ea+2, res & 0xFFFF );
	m68ki_write_16(m68k, ea, (res >> 16) & 0xFFFF );

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, pd, a)
{
	UINT32 res = AY;
	UINT32 ea = EA_AX_PD_32(m68k);

	m68ki_write_16(m68k, ea+2, res & 0xFFFF );
	m68ki_write_16(m68k, ea, (res >> 16) & 0xFFFF );

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, pd, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32 ea = EA_AX_PD_32(m68k);

	m68ki_write_16(m68k, ea+2, res & 0xFFFF );
	m68ki_write_16(m68k, ea, (res >> 16) & 0xFFFF );

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, di, d)
{
	UINT32 res = DY;
	UINT32 ea = EA_AX_DI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, di, a)
{
	UINT32 res = AY;
	UINT32 ea = EA_AX_DI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, di, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32 ea = EA_AX_DI_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, ix, d)
{
	UINT32 res = DY;
	UINT32 ea = EA_AX_IX_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, ix, a)
{
	UINT32 res = AY;
	UINT32 ea = EA_AX_IX_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, ix, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32 ea = EA_AX_IX_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, aw, d)
{
	UINT32 res = DY;
	UINT32 ea = EA_AW_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, aw, a)
{
	UINT32 res = AY;
	UINT32 ea = EA_AW_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, aw, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32 ea = EA_AW_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, al, d)
{
	UINT32 res = DY;
	UINT32 ea = EA_AL_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, al, a)
{
	UINT32 res = AY;
	UINT32 ea = EA_AL_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move, 32, al, .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;
	UINT32 ea = EA_AL_32(m68k);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(movea, 16, ., d)
{
	AX = MAKE_INT_16(DY);
}


M68KMAKE_OP(movea, 16, ., a)
{
	AX = MAKE_INT_16(AY);
}


M68KMAKE_OP(movea, 16, ., .)
{
	AX = MAKE_INT_16(M68KMAKE_GET_OPER_AY_16);
}


M68KMAKE_OP(movea, 32, ., d)
{
	AX = DY;
}


M68KMAKE_OP(movea, 32, ., a)
{
	AX = AY;
}


M68KMAKE_OP(movea, 32, ., .)
{
	AX = M68KMAKE_GET_OPER_AY_32;
}


M68KMAKE_OP(move, 16, frc, d)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		DY = MASK_OUT_BELOW_16(DY) | m68ki_get_ccr(m68k);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(move, 16, frc, .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		m68ki_write_16(m68k, M68KMAKE_GET_EA_AY_16, m68ki_get_ccr(m68k));
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(move, 16, toc, d)
{
	m68ki_set_ccr(m68k, DY);
}


M68KMAKE_OP(move, 16, toc, .)
{
	m68ki_set_ccr(m68k, M68KMAKE_GET_OPER_AY_16);
}


M68KMAKE_OP(move, 16, frs, d)
{
	if(CPU_TYPE_IS_000(m68k->cpu_type) || m68k->s_flag)	/* NS990408 */
	{
		DY = MASK_OUT_BELOW_16(DY) | m68ki_get_sr(m68k);
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(move, 16, frs, .)
{
	if(CPU_TYPE_IS_000(m68k->cpu_type) || m68k->s_flag)	/* NS990408 */
	{
		UINT32 ea = M68KMAKE_GET_EA_AY_16;
		m68ki_write_16(m68k, ea, m68ki_get_sr(m68k));
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(move, 16, tos, d)
{
	if(m68k->s_flag)
	{
		m68ki_set_sr(m68k, DY);
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(move, 16, tos, .)
{
	if(m68k->s_flag)
	{
		UINT32 new_sr = M68KMAKE_GET_OPER_AY_16;
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_set_sr(m68k, new_sr);
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(move, 32, fru, .)
{
	if(m68k->s_flag)
	{
		AY = REG_USP;
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(move, 32, tou, .)
{
	if(m68k->s_flag)
	{
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		REG_USP = AY;
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(movec, 32, cr, .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		if(m68k->s_flag)
		{
			UINT32 word2 = OPER_I_16(m68k);

			m68ki_trace_t0();		   /* auto-disable (see m68kcpu.h) */
			switch (word2 & 0xfff)
			{
			case 0x000:			   /* SFC */
				REG_DA[(word2 >> 12) & 15] = m68k->sfc;
				return;
			case 0x001:			   /* DFC */
				REG_DA[(word2 >> 12) & 15] = m68k->dfc;
				return;
			case 0x002:			   /* CACR */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					REG_DA[(word2 >> 12) & 15] = m68k->cacr;
					return;
				}
				return;
			case 0x800:			   /* USP */
				REG_DA[(word2 >> 12) & 15] = REG_USP;
				return;
			case 0x801:			   /* VBR */
				REG_DA[(word2 >> 12) & 15] = m68k->vbr;
				return;
			case 0x802:			   /* CAAR */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					REG_DA[(word2 >> 12) & 15] = m68k->caar;
					return;
				}
				m68ki_exception_illegal(m68k);
				break;
			case 0x803:			   /* MSP */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					REG_DA[(word2 >> 12) & 15] = m68k->m_flag ? REG_SP : REG_MSP;
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x804:			   /* ISP */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					REG_DA[(word2 >> 12) & 15] = m68k->m_flag ? REG_ISP : REG_SP;
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x003:				/* TC */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x004:				/* ITT0 */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x005:				/* ITT1 */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x006:				/* DTT0 */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x007:				/* DTT1 */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x805:				/* MMUSR */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x806:				/* URP */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x807:				/* SRP */
				if(CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			default:
				m68ki_exception_illegal(m68k);
				return;
			}
		}
		m68ki_exception_privilege_violation(m68k);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(movec, 32, rc, .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		if(m68k->s_flag)
		{
			UINT32 word2 = OPER_I_16(m68k);

			m68ki_trace_t0();		   /* auto-disable (see m68kcpu.h) */
			switch (word2 & 0xfff)
			{
			case 0x000:			   /* SFC */
				m68k->sfc = REG_DA[(word2 >> 12) & 15] & 7;
				return;
			case 0x001:			   /* DFC */
				m68k->dfc = REG_DA[(word2 >> 12) & 15] & 7;
				return;
			case 0x002:			   /* CACR */
				/* Only EC020 and later have CACR */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					/* 68030 can write all bits except 5-7, 040 can write all */
					if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
					{
						m68k->cacr = REG_DA[(word2 >> 12) & 15];
					}
					else if (CPU_TYPE_IS_030_PLUS(m68k->cpu_type))
					{
						m68k->cacr = REG_DA[(word2 >> 12) & 15] & 0xff1f;
					}
					else
					{
						m68k->cacr = REG_DA[(word2 >> 12) & 15] & 0x0f;
					}
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x800:			   /* USP */
				REG_USP = REG_DA[(word2 >> 12) & 15];
				return;
			case 0x801:			   /* VBR */
				m68k->vbr = REG_DA[(word2 >> 12) & 15];
				return;
			case 0x802:			   /* CAAR */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					m68k->caar = REG_DA[(word2 >> 12) & 15];
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x803:			   /* MSP */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					/* we are in supervisor mode so just check for M flag */
					if(!m68k->m_flag)
					{
						REG_MSP = REG_DA[(word2 >> 12) & 15];
						return;
					}
					REG_SP = REG_DA[(word2 >> 12) & 15];
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x804:			   /* ISP */
				if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
				{
					if(!m68k->m_flag)
					{
						REG_SP = REG_DA[(word2 >> 12) & 15];
						return;
					}
					REG_ISP = REG_DA[(word2 >> 12) & 15];
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x003:			/* TC */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x004:			/* ITT0 */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x005:			/* ITT1 */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x006:			/* DTT0 */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x007:			/* DTT1 */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x805:			/* MMUSR */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x806:			/* URP */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			case 0x807:			/* SRP */
				if (CPU_TYPE_IS_040_PLUS(m68k->cpu_type))
				{
					/* TODO */
					return;
				}
				m68ki_exception_illegal(m68k);
				return;
			default:
				m68ki_exception_illegal(m68k);
				return;
			}
		}
		m68ki_exception_privilege_violation(m68k);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(movem, 16, re, pd)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = AY;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			ea -= 2;
			m68ki_write_16(m68k, ea, MASK_OUT_ABOVE_16(REG_DA[15-i]));
			count++;
		}
	AY = ea;

	m68k->remaining_cycles -= count<<m68k->cyc_movem_w;
}


M68KMAKE_OP(movem, 16, re, .)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			m68ki_write_16(m68k, ea, MASK_OUT_ABOVE_16(REG_DA[i]));
			ea += 2;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_w;
}


M68KMAKE_OP(movem, 32, re, pd)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = AY;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			ea -= 4;
			m68ki_write_16(m68k, ea+2, REG_DA[15-i] & 0xFFFF );
			m68ki_write_16(m68k, ea, (REG_DA[15-i] >> 16) & 0xFFFF );
			count++;
		}
	AY = ea;

	m68k->remaining_cycles -= count<<m68k->cyc_movem_l;
}


M68KMAKE_OP(movem, 32, re, .)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			m68ki_write_32(m68k, ea, REG_DA[i]);
			ea += 4;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_l;
}


M68KMAKE_OP(movem, 16, er, pi)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = AY;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = MAKE_INT_16(MASK_OUT_ABOVE_16(m68ki_read_16(m68k, ea)));
			ea += 2;
			count++;
		}
	AY = ea;

	m68k->remaining_cycles -= count<<m68k->cyc_movem_w;
}


M68KMAKE_OP(movem, 16, er, pcdi)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = EA_PCDI_16(m68k);
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = MAKE_INT_16(MASK_OUT_ABOVE_16(m68ki_read_pcrel_16(m68k, ea)));
			ea += 2;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_w;
}


M68KMAKE_OP(movem, 16, er, pcix)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = EA_PCIX_16(m68k);
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = MAKE_INT_16(MASK_OUT_ABOVE_16(m68ki_read_pcrel_16(m68k, ea)));
			ea += 2;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_w;
}


M68KMAKE_OP(movem, 16, er, .)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = MAKE_INT_16(MASK_OUT_ABOVE_16(m68ki_read_16(m68k, ea)));
			ea += 2;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_w;
}


M68KMAKE_OP(movem, 32, er, pi)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = AY;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = m68ki_read_32(m68k, ea);
			ea += 4;
			count++;
		}
	AY = ea;

	m68k->remaining_cycles -= count<<m68k->cyc_movem_l;
}


M68KMAKE_OP(movem, 32, er, pcdi)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = EA_PCDI_32(m68k);
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = m68ki_read_pcrel_32(m68k, ea);
			ea += 4;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_l;
}


M68KMAKE_OP(movem, 32, er, pcix)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = EA_PCIX_32(m68k);
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = m68ki_read_pcrel_32(m68k, ea);
			ea += 4;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_l;
}


M68KMAKE_OP(movem, 32, er, .)
{
	UINT32 i = 0;
	UINT32 register_list = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 count = 0;

	for(; i < 16; i++)
		if(register_list & (1 << i))
		{
			REG_DA[i] = m68ki_read_32(m68k, ea);
			ea += 4;
			count++;
		}

	m68k->remaining_cycles -= count<<m68k->cyc_movem_l;
}


M68KMAKE_OP(movep, 16, re, .)
{
	UINT32 ea = EA_AY_DI_16(m68k);
	UINT32 src = DX;

	m68ki_write_8(m68k, ea, MASK_OUT_ABOVE_8(src >> 8));
	m68ki_write_8(m68k, ea += 2, MASK_OUT_ABOVE_8(src));
}


M68KMAKE_OP(movep, 32, re, .)
{
	UINT32 ea = EA_AY_DI_32(m68k);
	UINT32 src = DX;

	m68ki_write_8(m68k, ea, MASK_OUT_ABOVE_8(src >> 24));
	m68ki_write_8(m68k, ea += 2, MASK_OUT_ABOVE_8(src >> 16));
	m68ki_write_8(m68k, ea += 2, MASK_OUT_ABOVE_8(src >> 8));
	m68ki_write_8(m68k, ea += 2, MASK_OUT_ABOVE_8(src));
}


M68KMAKE_OP(movep, 16, er, .)
{
	UINT32 ea = EA_AY_DI_16(m68k);
	UINT32* r_dst = &DX;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | ((m68ki_read_8(m68k, ea) << 8) + m68ki_read_8(m68k, ea + 2));
}


M68KMAKE_OP(movep, 32, er, .)
{
	UINT32 ea = EA_AY_DI_32(m68k);

	DX = (m68ki_read_8(m68k, ea) << 24) + (m68ki_read_8(m68k, ea + 2) << 16)
		+ (m68ki_read_8(m68k, ea + 4) << 8) + m68ki_read_8(m68k, ea + 6);
}


M68KMAKE_OP(moves, 8, ., .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		if(m68k->s_flag)
		{
			UINT32 word2 = OPER_I_16(m68k);
			UINT32 ea = M68KMAKE_GET_EA_AY_8;

			m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
			if(BIT_B(word2))		   /* Register to memory */
			{
				m68ki_write_8_fc(m68k, ea, m68k->dfc, MASK_OUT_ABOVE_8(REG_DA[(word2 >> 12) & 15]));
				return;
			}
			if(BIT_F(word2))		   /* Memory to address register */
			{
				REG_A[(word2 >> 12) & 7] = MAKE_INT_8(m68ki_read_8_fc(m68k, ea, m68k->sfc));
				if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
					m68k->remaining_cycles -= 2;
				return;
			}
			/* Memory to data register */
			REG_D[(word2 >> 12) & 7] = MASK_OUT_BELOW_8(REG_D[(word2 >> 12) & 7]) | m68ki_read_8_fc(m68k, ea, m68k->sfc);
			if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
				m68k->remaining_cycles -= 2;
			return;
		}
		m68ki_exception_privilege_violation(m68k);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(moves, 16, ., .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		if(m68k->s_flag)
		{
			UINT32 word2 = OPER_I_16(m68k);
			UINT32 ea = M68KMAKE_GET_EA_AY_16;

			m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
			if(BIT_B(word2))		   /* Register to memory */
			{
				m68ki_write_16_fc(m68k, ea, m68k->dfc, MASK_OUT_ABOVE_16(REG_DA[(word2 >> 12) & 15]));
				return;
			}
			if(BIT_F(word2))		   /* Memory to address register */
			{
				REG_A[(word2 >> 12) & 7] = MAKE_INT_16(m68ki_read_16_fc(m68k, ea, m68k->sfc));
				if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
					m68k->remaining_cycles -= 2;
				return;
			}
			/* Memory to data register */
			REG_D[(word2 >> 12) & 7] = MASK_OUT_BELOW_16(REG_D[(word2 >> 12) & 7]) | m68ki_read_16_fc(m68k, ea, m68k->sfc);
			if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
				m68k->remaining_cycles -= 2;
			return;
		}
		m68ki_exception_privilege_violation(m68k);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(moves, 32, ., .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		if(m68k->s_flag)
		{
			UINT32 word2 = OPER_I_16(m68k);
			UINT32 ea = M68KMAKE_GET_EA_AY_32;

			m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
			if(BIT_B(word2))		   /* Register to memory */
			{
				m68ki_write_32_fc(m68k, ea, m68k->dfc, REG_DA[(word2 >> 12) & 15]);
				if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
					m68k->remaining_cycles -= 2;
				return;
			}
			/* Memory to register */
			REG_DA[(word2 >> 12) & 15] = m68ki_read_32_fc(m68k, ea, m68k->sfc);
			if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
				m68k->remaining_cycles -= 2;
			return;
		}
		m68ki_exception_privilege_violation(m68k);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(moveq, 32, ., .)
{
	UINT32 res = DX = MAKE_INT_8(MASK_OUT_ABOVE_8(m68k->ir));

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(move16, 32, ., .)
{
	UINT16 w2 = OPER_I_16(m68k);
	int ax = m68k->ir & 7;
	int ay = (w2 >> 12) & 7;

	m68ki_write_32(m68k, REG_A[ay],    m68ki_read_32(m68k, REG_A[ax]));
	m68ki_write_32(m68k, REG_A[ay]+4,  m68ki_read_32(m68k, REG_A[ax]+4));
	m68ki_write_32(m68k, REG_A[ay]+8,  m68ki_read_32(m68k, REG_A[ax]+8));
	m68ki_write_32(m68k, REG_A[ay]+12, m68ki_read_32(m68k, REG_A[ax]+12));

	REG_A[ax] += 16;
	REG_A[ay] += 16;
}


M68KMAKE_OP(muls, 16, ., d)
{
	UINT32* r_dst = &DX;
	UINT32 res = MASK_OUT_ABOVE_32(MAKE_INT_16(DY) * MAKE_INT_16(MASK_OUT_ABOVE_16(*r_dst)));

	*r_dst = res;

	m68k->not_z_flag = res;
	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(muls, 16, ., .)
{
	UINT32* r_dst = &DX;
	UINT32 res = MASK_OUT_ABOVE_32(MAKE_INT_16(M68KMAKE_GET_OPER_AY_16) * MAKE_INT_16(MASK_OUT_ABOVE_16(*r_dst)));

	*r_dst = res;

	m68k->not_z_flag = res;
	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(mulu, 16, ., d)
{
	UINT32* r_dst = &DX;
	UINT32 res = MASK_OUT_ABOVE_16(DY) * MASK_OUT_ABOVE_16(*r_dst);

	*r_dst = res;

	m68k->not_z_flag = res;
	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(mulu, 16, ., .)
{
	UINT32* r_dst = &DX;
	UINT32 res = M68KMAKE_GET_OPER_AY_16 * MASK_OUT_ABOVE_16(*r_dst);

	*r_dst = res;

	m68k->not_z_flag = res;
	m68k->n_flag = NFLAG_32(res);
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(mull, 32, ., d)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT64 src = DY;
		UINT64 dst = REG_D[(word2 >> 12) & 7];
		UINT64 res;

		m68k->c_flag = CFLAG_CLEAR;

		if(BIT_B(word2))			   /* signed */
		{
			res = (INT64)((INT32)src) * (INT64)((INT32)dst);
			if(!BIT_A(word2))
			{
				m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
				m68k->n_flag = NFLAG_32(res);
				m68k->v_flag = ((INT64)res != (INT32)res)<<7;
				REG_D[(word2 >> 12) & 7] = m68k->not_z_flag;
				return;
			}
			m68k->not_z_flag = MASK_OUT_ABOVE_32(res) | (res>>32);
			m68k->n_flag = NFLAG_64(res);
			m68k->v_flag = VFLAG_CLEAR;
			REG_D[word2 & 7] = (res >> 32);
			REG_D[(word2 >> 12) & 7] = MASK_OUT_ABOVE_32(res);
			return;
		}

		res = src * dst;
		if(!BIT_A(word2))
		{
			m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
			m68k->n_flag = NFLAG_32(res);
			m68k->v_flag = (res > 0xffffffff)<<7;
			REG_D[(word2 >> 12) & 7] = m68k->not_z_flag;
			return;
		}
		m68k->not_z_flag = MASK_OUT_ABOVE_32(res) | (res>>32);
		m68k->n_flag = NFLAG_64(res);
		m68k->v_flag = VFLAG_CLEAR;
		REG_D[word2 & 7] = (res >> 32);
		REG_D[(word2 >> 12) & 7] = MASK_OUT_ABOVE_32(res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(mull, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 word2 = OPER_I_16(m68k);
		UINT64 src = M68KMAKE_GET_OPER_AY_32;
		UINT64 dst = REG_D[(word2 >> 12) & 7];
		UINT64 res;

		m68k->c_flag = CFLAG_CLEAR;

		if(BIT_B(word2))			   /* signed */
		{
			res = (INT64)((INT32)src) * (INT64)((INT32)dst);
			if(!BIT_A(word2))
			{
				m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
				m68k->n_flag = NFLAG_32(res);
				m68k->v_flag = ((INT64)res != (INT32)res)<<7;
				REG_D[(word2 >> 12) & 7] = m68k->not_z_flag;
				return;
			}
			m68k->not_z_flag = MASK_OUT_ABOVE_32(res) | (res>>32);
			m68k->n_flag = NFLAG_64(res);
			m68k->v_flag = VFLAG_CLEAR;
			REG_D[word2 & 7] = (res >> 32);
			REG_D[(word2 >> 12) & 7] = MASK_OUT_ABOVE_32(res);
			return;
		}

		res = src * dst;
		if(!BIT_A(word2))
		{
			m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
			m68k->n_flag = NFLAG_32(res);
			m68k->v_flag = (res > 0xffffffff)<<7;
			REG_D[(word2 >> 12) & 7] = m68k->not_z_flag;
			return;
		}
		m68k->not_z_flag = MASK_OUT_ABOVE_32(res) | (res>>32);
		m68k->n_flag = NFLAG_64(res);
		m68k->v_flag = VFLAG_CLEAR;
		REG_D[word2 & 7] = (res >> 32);
		REG_D[(word2 >> 12) & 7] = MASK_OUT_ABOVE_32(res);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(nbcd, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 dst = *r_dst;
	UINT32 res = MASK_OUT_ABOVE_8(0x9a - dst - XFLAG_AS_1(m68k));

	if(res != 0x9a)
	{
		m68k->v_flag = ~res; /* Undefined V behavior */

		if((res & 0x0f) == 0xa)
			res = (res & 0xf0) + 0x10;

		res = MASK_OUT_ABOVE_8(res);

		m68k->v_flag &= res; /* Undefined V behavior part II */

		*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

		m68k->not_z_flag |= res;
		m68k->c_flag = CFLAG_SET;
		m68k->x_flag = XFLAG_SET;
	}
	else
	{
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->x_flag = XFLAG_CLEAR;
	}
	m68k->n_flag = NFLAG_8(res);	/* Undefined N behavior */
}


M68KMAKE_OP(nbcd, 8, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = MASK_OUT_ABOVE_8(0x9a - dst - XFLAG_AS_1(m68k));

	if(res != 0x9a)
	{
		m68k->v_flag = ~res; /* Undefined V behavior */

		if((res & 0x0f) == 0xa)
			res = (res & 0xf0) + 0x10;

		res = MASK_OUT_ABOVE_8(res);

		m68k->v_flag &= res; /* Undefined V behavior part II */

		m68ki_write_8(m68k, ea, MASK_OUT_ABOVE_8(res));

		m68k->not_z_flag |= res;
		m68k->c_flag = CFLAG_SET;
		m68k->x_flag = XFLAG_SET;
	}
	else
	{
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		m68k->x_flag = XFLAG_CLEAR;
	}
	m68k->n_flag = NFLAG_8(res);	/* Undefined N behavior */
}


M68KMAKE_OP(neg, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = 0 - MASK_OUT_ABOVE_8(*r_dst);

	m68k->n_flag = NFLAG_8(res);
	m68k->c_flag = m68k->x_flag = CFLAG_8(res);
	m68k->v_flag = *r_dst & res;
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(neg, 8, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);
	UINT32 res = 0 - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->c_flag = m68k->x_flag = CFLAG_8(res);
	m68k->v_flag = src & res;
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(neg, 16, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = 0 - MASK_OUT_ABOVE_16(*r_dst);

	m68k->n_flag = NFLAG_16(res);
	m68k->c_flag = m68k->x_flag = CFLAG_16(res);
	m68k->v_flag = (*r_dst & res)>>8;
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(neg, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = 0 - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->c_flag = m68k->x_flag = CFLAG_16(res);
	m68k->v_flag = (src & res)>>8;
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(neg, 32, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = 0 - *r_dst;

	m68k->n_flag = NFLAG_32(res);
	m68k->c_flag = m68k->x_flag = CFLAG_SUB_32(*r_dst, 0, res);
	m68k->v_flag = (*r_dst & res)>>24;
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(neg, 32, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 src = m68ki_read_32(m68k, ea);
	UINT32 res = 0 - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->c_flag = m68k->x_flag = CFLAG_SUB_32(src, 0, res);
	m68k->v_flag = (src & res)>>24;
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	m68ki_write_32(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(negx, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = 0 - MASK_OUT_ABOVE_8(*r_dst) - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = *r_dst & res;

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}


M68KMAKE_OP(negx, 8, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = m68ki_read_8(m68k, ea);
	UINT32 res = 0 - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = src & res;

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(negx, 16, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = 0 - MASK_OUT_ABOVE_16(*r_dst) - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = (*r_dst & res)>>8;

	res = MASK_OUT_ABOVE_16(res);
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
}


M68KMAKE_OP(negx, 16, ., .)
{
	UINT32 ea  = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = 0 - MASK_OUT_ABOVE_16(src) - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = (src & res)>>8;

	res = MASK_OUT_ABOVE_16(res);
	m68k->not_z_flag |= res;

	m68ki_write_16(m68k, ea, res);
}


M68KMAKE_OP(negx, 32, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = 0 - MASK_OUT_ABOVE_32(*r_dst) - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(*r_dst, 0, res);
	m68k->v_flag = (*r_dst & res)>>24;

	res = MASK_OUT_ABOVE_32(res);
	m68k->not_z_flag |= res;

	*r_dst = res;
}


M68KMAKE_OP(negx, 32, ., .)
{
	UINT32 ea  = M68KMAKE_GET_EA_AY_32;
	UINT32 src = m68ki_read_32(m68k, ea);
	UINT32 res = 0 - MASK_OUT_ABOVE_32(src) - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, 0, res);
	m68k->v_flag = (src & res)>>24;

	res = MASK_OUT_ABOVE_32(res);
	m68k->not_z_flag |= res;

	m68ki_write_32(m68k, ea, res);
}


M68KMAKE_OP(nop, 0, ., .)
{
	m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
}


M68KMAKE_OP(not, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = MASK_OUT_ABOVE_8(~*r_dst);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(not, 8, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 res = MASK_OUT_ABOVE_8(~m68ki_read_8(m68k, ea));

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(not, 16, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = MASK_OUT_ABOVE_16(~*r_dst);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(not, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 res = MASK_OUT_ABOVE_16(~m68ki_read_16(m68k, ea));

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(not, 32, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 res = *r_dst = MASK_OUT_ABOVE_32(~*r_dst);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(not, 32, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 res = MASK_OUT_ABOVE_32(~m68ki_read_32(m68k, ea));

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 8, er, d)
{
	UINT32 res = MASK_OUT_ABOVE_8((DX |= MASK_OUT_ABOVE_8(DY)));

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 8, er, .)
{
	UINT32 res = MASK_OUT_ABOVE_8((DX |= M68KMAKE_GET_OPER_AY_8));

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 16, er, d)
{
	UINT32 res = MASK_OUT_ABOVE_16((DX |= MASK_OUT_ABOVE_16(DY)));

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 16, er, .)
{
	UINT32 res = MASK_OUT_ABOVE_16((DX |= M68KMAKE_GET_OPER_AY_16));

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 32, er, d)
{
	UINT32 res = DX |= DY;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 32, er, .)
{
	UINT32 res = DX |= M68KMAKE_GET_OPER_AY_32;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 8, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 res = MASK_OUT_ABOVE_8(DX | m68ki_read_8(m68k, ea));

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 16, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 res = MASK_OUT_ABOVE_16(DX | m68ki_read_16(m68k, ea));

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(or, 32, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 res = DX | m68ki_read_32(m68k, ea);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ori, 8, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_8((DY |= OPER_I_8(m68k)));

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ori, 8, ., .)
{
	UINT32 src = OPER_I_8(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 res = MASK_OUT_ABOVE_8(src | m68ki_read_8(m68k, ea));

	m68ki_write_8(m68k, ea, res);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ori, 16, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY |= OPER_I_16(m68k));

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ori, 16, ., .)
{
	UINT32 src = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 res = MASK_OUT_ABOVE_16(src | m68ki_read_16(m68k, ea));

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ori, 32, ., d)
{
	UINT32 res = DY |= OPER_I_32(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ori, 32, ., .)
{
	UINT32 src = OPER_I_32(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 res = src | m68ki_read_32(m68k, ea);

	m68ki_write_32(m68k, ea, res);

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ori, 16, toc, .)
{
	m68ki_set_ccr(m68k, m68ki_get_ccr(m68k) | OPER_I_16(m68k));
}


M68KMAKE_OP(ori, 16, tos, .)
{
	if(m68k->s_flag)
	{
		UINT32 src = OPER_I_16(m68k);
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68ki_set_sr(m68k, m68ki_get_sr(m68k) | src);
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(pack, 16, rr, .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: DX and DY are reversed in Motorola's docs */
		UINT32 src = DY + OPER_I_16(m68k);
		UINT32* r_dst = &DX;

		*r_dst = MASK_OUT_BELOW_8(*r_dst) | ((src >> 4) & 0x00f0) | (src & 0x000f);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(pack, 16, mm, ax7)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: AX and AY are reversed in Motorola's docs */
		UINT32 ea_src = EA_AY_PD_8(m68k);
		UINT32 src = m68ki_read_8(m68k, ea_src);
		ea_src = EA_AY_PD_8(m68k);
		src = ((src << 8) | m68ki_read_8(m68k, ea_src)) + OPER_I_16(m68k);

		m68ki_write_8(m68k, EA_A7_PD_8(m68k), ((src >> 4) & 0x00f0) | (src & 0x000f));
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(pack, 16, mm, ay7)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: AX and AY are reversed in Motorola's docs */
		UINT32 ea_src = EA_A7_PD_8(m68k);
		UINT32 src = m68ki_read_8(m68k, ea_src);
		ea_src = EA_A7_PD_8(m68k);
		src = ((src << 8) | m68ki_read_8(m68k, ea_src)) + OPER_I_16(m68k);

		m68ki_write_8(m68k, EA_AX_PD_8(m68k), ((src >> 4) & 0x00f0) | (src & 0x000f));
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(pack, 16, mm, axy7)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 ea_src = EA_A7_PD_8(m68k);
		UINT32 src = m68ki_read_8(m68k, ea_src);
		ea_src = EA_A7_PD_8(m68k);
		src = ((src << 8) | m68ki_read_8(m68k, ea_src)) + OPER_I_16(m68k);

		m68ki_write_8(m68k, EA_A7_PD_8(m68k), ((src >> 4) & 0x00f0) | (src & 0x000f));
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(pack, 16, mm, .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: AX and AY are reversed in Motorola's docs */
		UINT32 ea_src = EA_AY_PD_8(m68k);
		UINT32 src = m68ki_read_8(m68k, ea_src);
		ea_src = EA_AY_PD_8(m68k);
		src = ((src << 8) | m68ki_read_8(m68k, ea_src)) + OPER_I_16(m68k);

		m68ki_write_8(m68k, EA_AX_PD_8(m68k), ((src >> 4) & 0x00f0) | (src & 0x000f));
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(pea, 32, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;

	m68ki_push_32(m68k, ea);
}

M68KMAKE_OP(pflush, 32, ., .)
{
	if ((CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type)) && (m68k->has_pmmu))
	{
		logerror("68040: unhandled PFLUSH\n");
		return;
	}
	m68ki_exception_1111(m68k);
}

M68KMAKE_OP(pmmu, 32, ., .)
{
	if ((CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type)) && (m68k->has_pmmu))
	{
		m68881_mmu_ops(m68k);
	}
	else
	{
		m68ki_exception_1111(m68k);
	}
}

M68KMAKE_OP(reset, 0, ., .)
{
	if(m68k->s_flag)
	{
		if (m68k->reset_instr_callback != NULL)
			(*m68k->reset_instr_callback)(m68k->device);
		m68k->remaining_cycles -= m68k->cyc_reset;
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(ror, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 shift = orig_shift & 7;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = ROR_8(src, shift);

	if(orig_shift != 0)
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src << (9-orig_shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ror, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = ROR_16(src, shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src << (9-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ror, 32, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT64 src = *r_dst;
	UINT32 res = ROR_32(src, shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src << (9-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ror, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;
	UINT32 shift = orig_shift & 7;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = ROR_8(src, shift);

	if(orig_shift != 0)
	{
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
		m68k->c_flag = src << (8-((shift-1)&7));
		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_8(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ror, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;
	UINT32 shift = orig_shift & 15;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = ROR_16(src, shift);

	if(orig_shift != 0)
	{
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
		m68k->c_flag = (src >> ((shift - 1) & 15)) << 8;
		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_16(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ror, 32, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;
	UINT32 shift = orig_shift & 31;
	UINT64 src = *r_dst;
	UINT32 res = ROR_32(src, shift);

	if(orig_shift != 0)
	{
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		*r_dst = res;
		m68k->c_flag = (src >> ((shift - 1) & 31)) << 8;
		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_32(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(ror, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = ROR_16(src, 1);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src << 8;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rol, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 shift = orig_shift & 7;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = ROL_8(src, shift);

	if(orig_shift != 0)
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src << orig_shift;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rol, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = ROL_16(src, shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src >> (8-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rol, 32, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT64 src = *r_dst;
	UINT32 res = ROL_32(src, shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src >> (24-shift);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rol, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;
	UINT32 shift = orig_shift & 7;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = ROL_8(src, shift);

	if(orig_shift != 0)
	{
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		if(shift != 0)
		{
			*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
			m68k->c_flag = src << shift;
			m68k->n_flag = NFLAG_8(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}
		m68k->c_flag = (src & 1)<<8;
		m68k->n_flag = NFLAG_8(src);
		m68k->not_z_flag = src;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_8(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rol, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;
	UINT32 shift = orig_shift & 15;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = MASK_OUT_ABOVE_16(ROL_16(src, shift));

	if(orig_shift != 0)
	{
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		if(shift != 0)
		{
			*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
			m68k->c_flag = (src << shift) >> 8;
			m68k->n_flag = NFLAG_16(res);
			m68k->not_z_flag = res;
			m68k->v_flag = VFLAG_CLEAR;
			return;
		}
		m68k->c_flag = (src & 1)<<8;
		m68k->n_flag = NFLAG_16(src);
		m68k->not_z_flag = src;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_16(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rol, 32, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;
	UINT32 shift = orig_shift & 31;
	UINT64 src = *r_dst;
	UINT32 res = ROL_32(src, shift);

	if(orig_shift != 0)
	{
		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		*r_dst = res;

		m68k->c_flag = (src >> ((32 - shift) & 0x1f)) << 8;
		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = CFLAG_CLEAR;
	m68k->n_flag = NFLAG_32(src);
	m68k->not_z_flag = src;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rol, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = MASK_OUT_ABOVE_16(ROL_16(src, 1));

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->c_flag = src >> 7;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxr, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = ROR_9(src | (XFLAG_AS_1(m68k) << 8), shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	m68k->c_flag = m68k->x_flag = res;
	res = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxr, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = ROR_17(src | (XFLAG_AS_1(m68k) << 16), shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	m68k->c_flag = m68k->x_flag = res >> 8;
	res = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxr, 32, s, .)
{
	UINT32*  r_dst = &DY;
	UINT32   shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT64 src   = *r_dst;
	UINT64 res   = src | (((UINT64)XFLAG_AS_1(m68k)) << 32);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	res = ROR_33_64(res, shift);

	m68k->c_flag = m68k->x_flag = res >> 24;
	res = MASK_OUT_ABOVE_32(res);

	*r_dst =  res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxr, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;

	if(orig_shift != 0)
	{
		UINT32 shift = orig_shift % 9;
		UINT32 src   = MASK_OUT_ABOVE_8(*r_dst);
		UINT32 res   = ROR_9(src | (XFLAG_AS_1(m68k) << 8), shift);

		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		m68k->c_flag = m68k->x_flag = res;
		res = MASK_OUT_ABOVE_8(res);

		*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = m68k->x_flag;
	m68k->n_flag = NFLAG_8(*r_dst);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(*r_dst);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxr, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;

	if(orig_shift != 0)
	{
		UINT32 shift = orig_shift % 17;
		UINT32 src   = MASK_OUT_ABOVE_16(*r_dst);
		UINT32 res   = ROR_17(src | (XFLAG_AS_1(m68k) << 16), shift);

		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		m68k->c_flag = m68k->x_flag = res >> 8;
		res = MASK_OUT_ABOVE_16(res);

		*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = m68k->x_flag;
	m68k->n_flag = NFLAG_16(*r_dst);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(*r_dst);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxr, 32, r, .)
{
	UINT32*  r_dst = &DY;
	UINT32   orig_shift = DX & 0x3f;

	if(orig_shift != 0)
	{
		UINT32   shift = orig_shift % 33;
		UINT64 src   = *r_dst;
		UINT64 res   = src | (((UINT64)XFLAG_AS_1(m68k)) << 32);

		res = ROR_33_64(res, shift);

		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		m68k->c_flag = m68k->x_flag = res >> 24;
		res = MASK_OUT_ABOVE_32(res);

		*r_dst = res;
		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = m68k->x_flag;
	m68k->n_flag = NFLAG_32(*r_dst);
	m68k->not_z_flag = *r_dst;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxr, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = ROR_17(src | (XFLAG_AS_1(m68k) << 16), 1);

	m68k->c_flag = m68k->x_flag = res >> 8;
	res = MASK_OUT_ABOVE_16(res);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxl, 8, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = ROL_9(src | (XFLAG_AS_1(m68k) << 8), shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	m68k->c_flag = m68k->x_flag = res;
	res = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxl, 16, s, .)
{
	UINT32* r_dst = &DY;
	UINT32 shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 src = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = ROL_17(src | (XFLAG_AS_1(m68k) << 16), shift);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	m68k->c_flag = m68k->x_flag = res >> 8;
	res = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxl, 32, s, .)
{
	UINT32*  r_dst = &DY;
	UINT32   shift = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT64 src   = *r_dst;
	UINT64 res   = src | (((UINT64)XFLAG_AS_1(m68k)) << 32);

	if(shift != 0)
		m68k->remaining_cycles -= shift<<m68k->cyc_shift;

	res = ROL_33_64(res, shift);

	m68k->c_flag = m68k->x_flag = res >> 24;
	res = MASK_OUT_ABOVE_32(res);

	*r_dst = res;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxl, 8, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;


	if(orig_shift != 0)
	{
		UINT32 shift = orig_shift % 9;
		UINT32 src   = MASK_OUT_ABOVE_8(*r_dst);
		UINT32 res   = ROL_9(src | (XFLAG_AS_1(m68k) << 8), shift);

		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		m68k->c_flag = m68k->x_flag = res;
		res = MASK_OUT_ABOVE_8(res);

		*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = m68k->x_flag;
	m68k->n_flag = NFLAG_8(*r_dst);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(*r_dst);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxl, 16, r, .)
{
	UINT32* r_dst = &DY;
	UINT32 orig_shift = DX & 0x3f;

	if(orig_shift != 0)
	{
		UINT32 shift = orig_shift % 17;
		UINT32 src   = MASK_OUT_ABOVE_16(*r_dst);
		UINT32 res   = ROL_17(src | (XFLAG_AS_1(m68k) << 16), shift);

		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		m68k->c_flag = m68k->x_flag = res >> 8;
		res = MASK_OUT_ABOVE_16(res);

		*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = m68k->x_flag;
	m68k->n_flag = NFLAG_16(*r_dst);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(*r_dst);
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxl, 32, r, .)
{
	UINT32*  r_dst = &DY;
	UINT32   orig_shift = DX & 0x3f;

	if(orig_shift != 0)
	{
		UINT32   shift = orig_shift % 33;
		UINT64 src   = *r_dst;
		UINT64 res   = src | (((UINT64)XFLAG_AS_1(m68k)) << 32);

		res = ROL_33_64(res, shift);

		m68k->remaining_cycles -= orig_shift<<m68k->cyc_shift;

		m68k->c_flag = m68k->x_flag = res >> 24;
		res = MASK_OUT_ABOVE_32(res);

		*r_dst = res;
		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		return;
	}

	m68k->c_flag = m68k->x_flag;
	m68k->n_flag = NFLAG_32(*r_dst);
	m68k->not_z_flag = *r_dst;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(roxl, 16, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = m68ki_read_16(m68k, ea);
	UINT32 res = ROL_17(src | (XFLAG_AS_1(m68k) << 16), 1);

	m68k->c_flag = m68k->x_flag = res >> 8;
	res = MASK_OUT_ABOVE_16(res);

	m68ki_write_16(m68k, ea, res);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(rtd, 32, ., .)
{
	if(CPU_TYPE_IS_010_PLUS(m68k->cpu_type))
	{
		UINT32 new_pc = m68ki_pull_32(m68k);

		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		REG_A[7] = MASK_OUT_ABOVE_32(REG_A[7] + MAKE_INT_16(OPER_I_16(m68k)));
		m68ki_jump(m68k, new_pc);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(rte, 32, ., .)
{
	if(m68k->s_flag)
	{
		UINT32 new_sr;
		UINT32 new_pc;
		UINT32 format_word;

		if (m68k->rte_instr_callback != NULL)
			(*m68k->rte_instr_callback)(m68k->device);
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */

		if(CPU_TYPE_IS_000(m68k->cpu_type))
		{
			new_sr = m68ki_pull_16(m68k);
			new_pc = m68ki_pull_32(m68k);
			m68ki_jump(m68k, new_pc);
			m68ki_set_sr(m68k, new_sr);

			m68k->instr_mode = INSTRUCTION_YES;
			m68k->run_mode = RUN_MODE_NORMAL;

			return;
		}

		if(CPU_TYPE_IS_010(m68k->cpu_type))
		{
			format_word = m68ki_read_16(m68k, REG_A[7]+6) >> 12;
			if(format_word == 0)
			{
				new_sr = m68ki_pull_16(m68k);
				new_pc = m68ki_pull_32(m68k);
				m68ki_fake_pull_16(m68k);	/* format word */
				m68ki_jump(m68k, new_pc);
				m68ki_set_sr(m68k, new_sr);
				m68k->instr_mode = INSTRUCTION_YES;
				m68k->run_mode = RUN_MODE_NORMAL;
				return;
			}
			m68k->instr_mode = INSTRUCTION_YES;
			m68k->run_mode = RUN_MODE_NORMAL;
			/* Not handling bus fault (9) */
			m68ki_exception_format_error(m68k);
			return;
		}

		/* Otherwise it's 020 */
rte_loop:
		format_word = m68ki_read_16(m68k, REG_A[7]+6) >> 12;
		switch(format_word)
		{
			case 0: /* Normal */
				new_sr = m68ki_pull_16(m68k);
				new_pc = m68ki_pull_32(m68k);
				m68ki_fake_pull_16(m68k);	/* format word */
				m68ki_jump(m68k, new_pc);
				m68ki_set_sr(m68k, new_sr);
				m68k->instr_mode = INSTRUCTION_YES;
				m68k->run_mode = RUN_MODE_NORMAL;
				return;
			case 1: /* Throwaway */
				new_sr = m68ki_pull_16(m68k);
				m68ki_fake_pull_32(m68k);	/* program counter */
				m68ki_fake_pull_16(m68k);	/* format word */
				m68ki_set_sr_noint(m68k, new_sr);
				goto rte_loop;
			case 2: /* Trap */
				new_sr = m68ki_pull_16(m68k);
				new_pc = m68ki_pull_32(m68k);
				m68ki_fake_pull_16(m68k);	/* format word */
				m68ki_fake_pull_32(m68k);	/* address */
				m68ki_jump(m68k, new_pc);
				m68ki_set_sr(m68k, new_sr);
				m68k->instr_mode = INSTRUCTION_YES;
				m68k->run_mode = RUN_MODE_NORMAL;
				return;
		}
		/* Not handling long or short bus fault */
		m68k->instr_mode = INSTRUCTION_YES;
		m68k->run_mode = RUN_MODE_NORMAL;
		m68ki_exception_format_error(m68k);
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(rtm, 32, ., .)
{
	if(CPU_TYPE_IS_020_VARIANT(m68k->cpu_type))
	{
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		logerror("%s at %08x: called unimplemented instruction %04x (rtm)\n",
					 m68k->device->tag, REG_PC - 2, m68k->ir);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(rtr, 32, ., .)
{
	m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
	m68ki_set_ccr(m68k, m68ki_pull_16(m68k));
	m68ki_jump(m68k, m68ki_pull_32(m68k));
}


M68KMAKE_OP(rts, 32, ., .)
{
	m68ki_trace_t0();				   /* auto-disable (see m68kcpu.h) */
	m68ki_jump(m68k, m68ki_pull_32(m68k));
}


M68KMAKE_OP(sbcd, 8, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = DY;
	UINT32 dst = *r_dst;
	UINT32 res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1(m68k);

//  m68k->v_flag = ~res; /* Undefined V behavior */
	m68k->v_flag = VFLAG_CLEAR;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to assume cleared. */

	if(res > 9)
		res -= 6;
	res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
	if(res > 0x99)
	{
		res += 0xa0;
		m68k->x_flag = m68k->c_flag = CFLAG_SET;
		m68k->n_flag = NFLAG_SET;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to follow carry. */
	}
	else
		m68k->n_flag = m68k->x_flag = m68k->c_flag = 0;

	res = MASK_OUT_ABOVE_8(res);

//  m68k->v_flag &= res; /* Undefined V behavior part II */
//  m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}


M68KMAKE_OP(sbcd, 8, mm, ax7)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1(m68k);

//  m68k->v_flag = ~res; /* Undefined V behavior */
	m68k->v_flag = VFLAG_CLEAR;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to return zero. */

	if(res > 9)
		res -= 6;
	res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
	if(res > 0x99)
	{
		res += 0xa0;
		m68k->x_flag = m68k->c_flag = CFLAG_SET;
		m68k->n_flag = NFLAG_SET;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to follow carry. */
	}
	else
		m68k->n_flag = m68k->x_flag = m68k->c_flag = 0;

	res = MASK_OUT_ABOVE_8(res);

//  m68k->v_flag &= res; /* Undefined V behavior part II */
//  m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(sbcd, 8, mm, ay7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1(m68k);

//  m68k->v_flag = ~res; /* Undefined V behavior */
	m68k->v_flag = VFLAG_CLEAR;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to return zero. */

	if(res > 9)
		res -= 6;
	res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
	if(res > 0x99)
	{
		res += 0xa0;
		m68k->x_flag = m68k->c_flag = CFLAG_SET;
		m68k->n_flag = NFLAG_SET;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to follow carry. */
	}
	else
		m68k->n_flag = m68k->x_flag = m68k->c_flag = 0;

	res = MASK_OUT_ABOVE_8(res);

//  m68k->v_flag &= res; /* Undefined V behavior part II */
//  m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(sbcd, 8, mm, axy7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1(m68k);

//  m68k->v_flag = ~res; /* Undefined V behavior */
	m68k->v_flag = VFLAG_CLEAR;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to return zero. */

	if(res > 9)
		res -= 6;
	res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
	if(res > 0x99)
	{
		res += 0xa0;
		m68k->x_flag = m68k->c_flag = CFLAG_SET;
		m68k->n_flag = NFLAG_SET;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to follow carry. */
	}
	else
		m68k->n_flag = m68k->x_flag = m68k->c_flag = 0;

	res = MASK_OUT_ABOVE_8(res);

//  m68k->v_flag &= res; /* Undefined V behavior part II */
//  m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(sbcd, 8, mm, .)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = LOW_NIBBLE(dst) - LOW_NIBBLE(src) - XFLAG_AS_1(m68k);

//  m68k->v_flag = ~res; /* Undefined V behavior */
	m68k->v_flag = VFLAG_CLEAR;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to return zero. */

	if(res > 9)
		res -= 6;
	res += HIGH_NIBBLE(dst) - HIGH_NIBBLE(src);
	if(res > 0x99)
	{
		res += 0xa0;
		m68k->x_flag = m68k->c_flag = CFLAG_SET;
		m68k->n_flag = NFLAG_SET;	/* Undefined in Motorola's M68000PM/AD rev.1 and safer to follow carry. */
	}
	else
		m68k->n_flag = m68k->x_flag = m68k->c_flag = 0;

	res = MASK_OUT_ABOVE_8(res);

//  m68k->v_flag &= res; /* Undefined V behavior part II */
//  m68k->n_flag = NFLAG_8(res); /* Undefined N behavior */
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(st, 8, ., d)
{
	DY |= 0xff;
}


M68KMAKE_OP(st, 8, ., .)
{
	m68ki_write_8(m68k, M68KMAKE_GET_EA_AY_8, 0xff);
}


M68KMAKE_OP(sf, 8, ., d)
{
	DY &= 0xffffff00;
}


M68KMAKE_OP(sf, 8, ., .)
{
	m68ki_write_8(m68k, M68KMAKE_GET_EA_AY_8, 0);
}


M68KMAKE_OP(scc, 8, ., d)
{
	if(M68KMAKE_CC)
	{
		DY |= 0xff;
		m68k->remaining_cycles -= m68k->cyc_scc_r_true;
		return;
	}
	DY &= 0xffffff00;
}


M68KMAKE_OP(scc, 8, ., .)
{
	m68ki_write_8(m68k, M68KMAKE_GET_EA_AY_8, M68KMAKE_CC ? 0xff : 0);
}


M68KMAKE_OP(stop, 0, ., .)
{
	if(m68k->s_flag)
	{
		UINT32 new_sr = OPER_I_16(m68k);
		m68ki_trace_t0();			   /* auto-disable (see m68kcpu.h) */
		m68k->stopped |= STOP_LEVEL_STOP;
		m68ki_set_sr(m68k, new_sr);
		m68k->remaining_cycles = 0;
		return;
	}
	m68ki_exception_privilege_violation(m68k);
}


M68KMAKE_OP(sub, 8, er, d)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_8(DY);
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(sub, 8, er, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = M68KMAKE_GET_OPER_AY_8;
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(sub, 16, er, d)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_16(DY);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(sub, 16, er, a)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_16(AY);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(sub, 16, er, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = M68KMAKE_GET_OPER_AY_16;
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(sub, 32, er, d)
{
	UINT32* r_dst = &DX;
	UINT32 src = DY;
	UINT32 dst = *r_dst;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(sub, 32, er, a)
{
	UINT32* r_dst = &DX;
	UINT32 src = AY;
	UINT32 dst = *r_dst;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(sub, 32, er, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = M68KMAKE_GET_OPER_AY_32;
	UINT32 dst = *r_dst;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(sub, 8, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 src = MASK_OUT_ABOVE_8(DX);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(sub, 16, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 src = MASK_OUT_ABOVE_16(DX);
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(sub, 32, re, .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 src = DX;
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);

	m68ki_write_32(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(suba, 16, ., d)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - MAKE_INT_16(DY));
}


M68KMAKE_OP(suba, 16, ., a)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - MAKE_INT_16(AY));
}


M68KMAKE_OP(suba, 16, ., .)
{
	UINT32* r_dst = &AX;
	UINT32 src = MAKE_INT_16(M68KMAKE_GET_OPER_AY_16);

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - src);
}


M68KMAKE_OP(suba, 32, ., d)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - DY);
}


M68KMAKE_OP(suba, 32, ., a)
{
	UINT32* r_dst = &AX;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - AY);
}


M68KMAKE_OP(suba, 32, ., .)
{
	UINT32* r_dst = &AX;
	UINT32 src = M68KMAKE_GET_OPER_AY_32;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - src);
}


M68KMAKE_OP(subi, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = OPER_I_8(m68k);
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(subi, 8, ., .)
{
	UINT32 src = OPER_I_8(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(subi, 16, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = OPER_I_16(m68k);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(subi, 16, ., .)
{
	UINT32 src = OPER_I_16(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(subi, 32, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = OPER_I_32(m68k);
	UINT32 dst = *r_dst;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(subi, 32, ., .)
{
	UINT32 src = OPER_I_32(m68k);
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);

	m68ki_write_32(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(subq, 8, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(subq, 8, ., .)
{
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	m68ki_write_8(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(subq, 16, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | m68k->not_z_flag;
}


M68KMAKE_OP(subq, 16, ., a)
{
	UINT32* r_dst = &AY;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - ((((m68k->ir >> 9) - 1) & 7) + 1));
}


M68KMAKE_OP(subq, 16, ., .)
{
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 ea = M68KMAKE_GET_EA_AY_16;
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);

	m68ki_write_16(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(subq, 32, ., d)
{
	UINT32* r_dst = &DY;
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 dst = *r_dst;
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);

	*r_dst = m68k->not_z_flag;
}


M68KMAKE_OP(subq, 32, ., a)
{
	UINT32* r_dst = &AY;

	*r_dst = MASK_OUT_ABOVE_32(*r_dst - ((((m68k->ir >> 9) - 1) & 7) + 1));
}


M68KMAKE_OP(subq, 32, ., .)
{
	UINT32 src = (((m68k->ir >> 9) - 1) & 7) + 1;
	UINT32 ea = M68KMAKE_GET_EA_AY_32;
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = dst - src;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = MASK_OUT_ABOVE_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);

	m68ki_write_32(m68k, ea, m68k->not_z_flag);
}


M68KMAKE_OP(subx, 8, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_8(DY);
	UINT32 dst = MASK_OUT_ABOVE_8(*r_dst);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | res;
}


M68KMAKE_OP(subx, 16, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = MASK_OUT_ABOVE_16(DY);
	UINT32 dst = MASK_OUT_ABOVE_16(*r_dst);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);

	res = MASK_OUT_ABOVE_16(res);
	m68k->not_z_flag |= res;

	*r_dst = MASK_OUT_BELOW_16(*r_dst) | res;
}


M68KMAKE_OP(subx, 32, rr, .)
{
	UINT32* r_dst = &DX;
	UINT32 src = DY;
	UINT32 dst = *r_dst;
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);

	res = MASK_OUT_ABOVE_32(res);
	m68k->not_z_flag |= res;

	*r_dst = res;
}


M68KMAKE_OP(subx, 8, mm, ax7)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(subx, 8, mm, ay7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(subx, 8, mm, axy7)
{
	UINT32 src = OPER_A7_PD_8(m68k);
	UINT32 ea  = EA_A7_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(subx, 8, mm, .)
{
	UINT32 src = OPER_AY_PD_8(m68k);
	UINT32 ea  = EA_AX_PD_8(m68k);
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_8(res);
	m68k->x_flag = m68k->c_flag = CFLAG_8(res);
	m68k->v_flag = VFLAG_SUB_8(src, dst, res);

	res = MASK_OUT_ABOVE_8(res);
	m68k->not_z_flag |= res;

	m68ki_write_8(m68k, ea, res);
}


M68KMAKE_OP(subx, 16, mm, .)
{
	UINT32 src = OPER_AY_PD_16(m68k);
	UINT32 ea  = EA_AX_PD_16(m68k);
	UINT32 dst = m68ki_read_16(m68k, ea);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_16(res);
	m68k->x_flag = m68k->c_flag = CFLAG_16(res);
	m68k->v_flag = VFLAG_SUB_16(src, dst, res);

	res = MASK_OUT_ABOVE_16(res);
	m68k->not_z_flag |= res;

	m68ki_write_16(m68k, ea, res);
}


M68KMAKE_OP(subx, 32, mm, .)
{
	UINT32 src = OPER_AY_PD_32(m68k);
	UINT32 ea  = EA_AX_PD_32(m68k);
	UINT32 dst = m68ki_read_32(m68k, ea);
	UINT32 res = dst - src - XFLAG_AS_1(m68k);

	m68k->n_flag = NFLAG_32(res);
	m68k->x_flag = m68k->c_flag = CFLAG_SUB_32(src, dst, res);
	m68k->v_flag = VFLAG_SUB_32(src, dst, res);

	res = MASK_OUT_ABOVE_32(res);
	m68k->not_z_flag |= res;

	m68ki_write_32(m68k, ea, res);
}


M68KMAKE_OP(swap, 32, ., .)
{
	UINT32* r_dst = &DY;

	m68k->not_z_flag = MASK_OUT_ABOVE_32(*r_dst<<16);
	*r_dst = (*r_dst>>16) | m68k->not_z_flag;

	m68k->not_z_flag = *r_dst;
	m68k->n_flag = NFLAG_32(*r_dst);
	m68k->c_flag = CFLAG_CLEAR;
	m68k->v_flag = VFLAG_CLEAR;
}


M68KMAKE_OP(tas, 8, ., d)
{
	UINT32* r_dst = &DY;

	m68k->not_z_flag = MASK_OUT_ABOVE_8(*r_dst);
	m68k->n_flag = NFLAG_8(*r_dst);
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
	*r_dst |= 0x80;
}


M68KMAKE_OP(tas, 8, ., .)
{
	UINT32 ea = M68KMAKE_GET_EA_AY_8;
	UINT32 dst = m68ki_read_8(m68k, ea);
	UINT32 allow_writeback = TRUE;

	m68k->not_z_flag = dst;
	m68k->n_flag = NFLAG_8(dst);
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;

	/* The Genesis/Megadrive games Gargoyles and Ex-Mutants need the TAS writeback
       disabled in order to function properly.  Some Amiga software may also rely
       on this, but only when accessing specific addresses so additional functionality
       will be needed. */
	if (m68k->tas_instr_callback != NULL)
		allow_writeback = (*m68k->tas_instr_callback)(m68k->device);

	if (allow_writeback)
		m68ki_write_8(m68k, ea, dst | 0x80);
}


M68KMAKE_OP(trap, 0, ., .)
{
	/* Trap#n stacks exception frame type 0 */
	m68ki_exception_trapN(m68k, EXCEPTION_TRAP_BASE + (m68k->ir & 0xf));	/* HJB 990403 */
}


M68KMAKE_OP(trapt, 0, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		m68ki_exception_trap(m68k, EXCEPTION_TRAPV);	/* HJB 990403 */
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapt, 16, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		m68ki_exception_trap(m68k, EXCEPTION_TRAPV);	/* HJB 990403 */
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapt, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		m68ki_exception_trap(m68k, EXCEPTION_TRAPV);	/* HJB 990403 */
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapf, 0, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapf, 16, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		REG_PC += 2;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapf, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		REG_PC += 4;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapcc, 0, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		if(M68KMAKE_CC)
			m68ki_exception_trap(m68k, EXCEPTION_TRAPV);	/* HJB 990403 */
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapcc, 16, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		if(M68KMAKE_CC)
		{
			m68ki_exception_trap(m68k, EXCEPTION_TRAPV);	/* HJB 990403 */
			return;
		}
		REG_PC += 2;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapcc, 32, ., .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		if(M68KMAKE_CC)
		{
			m68ki_exception_trap(m68k, EXCEPTION_TRAPV);	/* HJB 990403 */
			return;
		}
		REG_PC += 4;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(trapv, 0, ., .)
{
	if(COND_VC(m68k))
	{
		return;
	}
	m68ki_exception_trap(m68k, EXCEPTION_TRAPV);  /* HJB 990403 */
}


M68KMAKE_OP(tst, 8, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_8(DY);

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(tst, 8, ., .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_8;

	m68k->n_flag = NFLAG_8(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(tst, 8, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_PCDI_8(m68k);

		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 8, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_PCIX_8(m68k);

		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 8, ., i)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_I_8(m68k);

		m68k->n_flag = NFLAG_8(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 16, ., d)
{
	UINT32 res = MASK_OUT_ABOVE_16(DY);

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(tst, 16, ., a)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = MAKE_INT_16(AY);

		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 16, ., .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_16;

	m68k->n_flag = NFLAG_16(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(tst, 16, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_PCDI_16(m68k);

		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 16, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_PCIX_16(m68k);

		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 16, ., i)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_I_16(m68k);

		m68k->n_flag = NFLAG_16(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 32, ., d)
{
	UINT32 res = DY;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(tst, 32, ., a)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = AY;

		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 32, ., .)
{
	UINT32 res = M68KMAKE_GET_OPER_AY_32;

	m68k->n_flag = NFLAG_32(res);
	m68k->not_z_flag = res;
	m68k->v_flag = VFLAG_CLEAR;
	m68k->c_flag = CFLAG_CLEAR;
}


M68KMAKE_OP(tst, 32, ., pcdi)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_PCDI_32(m68k);

		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 32, ., pcix)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_PCIX_32(m68k);

		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(tst, 32, ., i)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 res = OPER_I_32(m68k);

		m68k->n_flag = NFLAG_32(res);
		m68k->not_z_flag = res;
		m68k->v_flag = VFLAG_CLEAR;
		m68k->c_flag = CFLAG_CLEAR;
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(unlk, 32, ., a7)
{
	REG_A[7] = m68ki_read_32(m68k, REG_A[7]);
}


M68KMAKE_OP(unlk, 32, ., .)
{
	UINT32* r_dst = &AY;

	REG_A[7] = *r_dst;
	*r_dst = m68ki_pull_32(m68k);
}


M68KMAKE_OP(unpk, 16, rr, .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: DX and DY are reversed in Motorola's docs */
		UINT32 src = DY;
		UINT32* r_dst = &DX;

		*r_dst = MASK_OUT_BELOW_16(*r_dst) | (((((src << 4) & 0x0f00) | (src & 0x000f)) + OPER_I_16(m68k)) & 0xffff);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(unpk, 16, mm, ax7)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: AX and AY are reversed in Motorola's docs */
		UINT32 src = OPER_AY_PD_8(m68k);
		UINT32 ea_dst;

		src = (((src << 4) & 0x0f00) | (src & 0x000f)) + OPER_I_16(m68k);
		ea_dst = EA_A7_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, (src >> 8) & 0xff);
		ea_dst = EA_A7_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, src & 0xff);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(unpk, 16, mm, ay7)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: AX and AY are reversed in Motorola's docs */
		UINT32 src = OPER_A7_PD_8(m68k);
		UINT32 ea_dst;

		src = (((src << 4) & 0x0f00) | (src & 0x000f)) + OPER_I_16(m68k);
		ea_dst = EA_AX_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, (src >> 8) & 0xff);
		ea_dst = EA_AX_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, src & 0xff);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(unpk, 16, mm, axy7)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		UINT32 src = OPER_A7_PD_8(m68k);
		UINT32 ea_dst;

		src = (((src << 4) & 0x0f00) | (src & 0x000f)) + OPER_I_16(m68k);
		ea_dst = EA_A7_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, (src >> 8) & 0xff);
		ea_dst = EA_A7_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, src & 0xff);
		return;
	}
	m68ki_exception_illegal(m68k);
}


M68KMAKE_OP(unpk, 16, mm, .)
{
	if(CPU_TYPE_IS_EC020_PLUS(m68k->cpu_type))
	{
		/* Note: AX and AY are reversed in Motorola's docs */
		UINT32 src = OPER_AY_PD_8(m68k);
		UINT32 ea_dst;

		src = (((src << 4) & 0x0f00) | (src & 0x000f)) + OPER_I_16(m68k);
		ea_dst = EA_AX_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, (src >> 8) & 0xff);
		ea_dst = EA_AX_PD_8(m68k);
		m68ki_write_8(m68k, ea_dst, src & 0xff);
		return;
	}
	m68ki_exception_illegal(m68k);
}



XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
M68KMAKE_END
