﻿/*
  KMZ80 DMG-CPU
  by Mamiya
*/

#include "kmz80i.h"

const static OPT_ITEM kmdmg_ot_xx[0x100] = {
/* DMG 00-3F  00?????? */
/*     INC r  00rrr100 */
/*     DEC r  00rrr101 */
/*     LD r,n 00rrr110 */
/* DMG 00-07  00000??? */
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(STO_BC,	LDO_NN,	0,		OP_LD),
	OPTABLE(STO_M,	LDO_A,	ADR_BC,	OP_LD),
	OPTABLE(STO_BC,	LDO_BC,	0,		OP_INC16),
	OPTABLE(STO_B,	LDO_B,	0,		OP_INC),
	OPTABLE(STO_B,	LDO_B,	0,		OP_DEC),
	OPTABLE(STO_B,	LDO_N,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_RLCA),
/* DMG 08-0f  00001??? */
	OPTABLE(STO_MM,	LDO_SP,	ADR_NN,	OP_LD),
	OPTABLE(STO_HL,	LDO_BC,	ADR_HL,	OP_ADD16),
	OPTABLE(STO_A,	LDO_M,	ADR_BC,	OP_LD),
	OPTABLE(STO_BC,	LDO_BC,	0,		OP_DEC16),
	OPTABLE(STO_C,	LDO_C,	0,		OP_INC),
	OPTABLE(STO_C,	LDO_C,	0,		OP_DEC),
	OPTABLE(STO_C,	LDO_N,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_RRCA),
/* DMG 10-17  00010??? */
	OPTABLE(0,		0,		0,		OP_STOP),
	OPTABLE(STO_DE,	LDO_NN,	0,		OP_LD),
	OPTABLE(STO_M,	LDO_A,	ADR_DE,	OP_LD),
	OPTABLE(STO_DE,	LDO_DE,	0,		OP_INC16),
	OPTABLE(STO_D,	LDO_D,	0,		OP_INC),
	OPTABLE(STO_D,	LDO_D,	0,		OP_DEC),
	OPTABLE(STO_D,	LDO_N,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_RLA),
/* DMG 18-1f  00011??? */
	OPTABLE(0,		LDO_N,	0,		OP_JR),
	OPTABLE(STO_HL,	LDO_DE,	ADR_HL,	OP_ADD16),
	OPTABLE(STO_A,	LDO_M,	ADR_DE,	OP_LD),
	OPTABLE(STO_DE,	LDO_DE,	0,		OP_DEC16),
	OPTABLE(STO_E,	LDO_E,	0,		OP_INC),
	OPTABLE(STO_E,	LDO_E,	0,		OP_DEC),
	OPTABLE(STO_E,	LDO_N,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_RRA),
/* DMG 20-27  00100??? */
	OPTABLE(0,		LDO_N,	0,		OP_JRCC),
	OPTABLE(STO_HL,	LDO_NN,	0,		OP_LD),
	OPTABLE(STO_M,	LDO_A,	ADR_HI,	OP_LD),
	OPTABLE(STO_HL,	LDO_HL,	0,		OP_INC16),
	OPTABLE(STO_H,	LDO_H,	0,		OP_INC),
	OPTABLE(STO_H,	LDO_H,	0,		OP_DEC),
	OPTABLE(STO_H,	LDO_N,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_DAA),
/* DMG 28-2f  00101??? */
	OPTABLE(0,		LDO_N,	0,		OP_JRCC),
	OPTABLE(STO_HL,	LDO_HL,	ADR_HL,	OP_ADD16),
	OPTABLE(STO_A,	LDO_M,	ADR_HI,	OP_LD),
	OPTABLE(STO_HL,	LDO_HL,	0,		OP_DEC16),
	OPTABLE(STO_L,	LDO_L,	0,		OP_INC),
	OPTABLE(STO_L,	LDO_L,	0,		OP_DEC),
	OPTABLE(STO_L,	LDO_N,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_CPL),
/* DMG 30-37  00110??? */
	OPTABLE(0,		LDO_N,	0,		OP_JRCC),
	OPTABLE(STO_SP,	LDO_NN,	0,		OP_LD),
	OPTABLE(STO_M,	LDO_A,	ADR_HD,	OP_LD),
	OPTABLE(STO_SP,	LDO_SP,	0,		OP_INC16),
	OPTABLE(STO_M,	LDO_M,	ADR_HL,	OP_INC),
	OPTABLE(STO_M,	LDO_M,	ADR_HL,	OP_DEC),
	OPTABLE(STO_M,	LDO_N,	ADR_HL,	OP_LD),
	OPTABLE(0,		0,		0,		OP_SCF),
/* DMG 38-3f  00111??? */
	OPTABLE(0,		LDO_N,	0,		OP_JRCC),
	OPTABLE(STO_HL,	LDO_SP,	ADR_HL,	OP_ADD16),
	OPTABLE(STO_A,	LDO_M,	ADR_HD,	OP_LD),
	OPTABLE(STO_SP,	LDO_SP,	0,		OP_DEC16),
	OPTABLE(STO_A,	LDO_A,	0,		OP_INC),
	OPTABLE(STO_A,	LDO_A,	0,		OP_DEC),
	OPTABLE(STO_A,	LDO_N,	0,		OP_LD),
	OPTABLE(0,		0,		0,		OP_CCF),

/* DMG 40-7F  01?????? */
/*     LD R,r 01RRRrrr */
/*     HALT   01110110 */
	OPTABLE(STO_B,	LDO_B,	0,		OP_LD),
	OPTABLE(STO_B,	LDO_C,	0,		OP_LD),
	OPTABLE(STO_B,	LDO_D,	0,		OP_LD),
	OPTABLE(STO_B,	LDO_E,	0,		OP_LD),
	OPTABLE(STO_B,	LDO_H,	0,		OP_LD),
	OPTABLE(STO_B,	LDO_L,	0,		OP_LD),
	OPTABLE(STO_B,	LDO_M,	ADR_HL,	OP_LD),
	OPTABLE(STO_B,	LDO_A,	0,		OP_LD),
	OPTABLE(STO_C,	LDO_B,	0,		OP_LD),
	OPTABLE(STO_C,	LDO_C,	0,		OP_LD),
	OPTABLE(STO_C,	LDO_D,	0,		OP_LD),
	OPTABLE(STO_C,	LDO_E,	0,		OP_LD),
	OPTABLE(STO_C,	LDO_H,	0,		OP_LD),
	OPTABLE(STO_C,	LDO_L,	0,		OP_LD),
	OPTABLE(STO_C,	LDO_M,	ADR_HL,	OP_LD),
	OPTABLE(STO_C,	LDO_A,	0,		OP_LD),
	OPTABLE(STO_D,	LDO_B,	0,		OP_LD),
	OPTABLE(STO_D,	LDO_C,	0,		OP_LD),
	OPTABLE(STO_D,	LDO_D,	0,		OP_LD),
	OPTABLE(STO_D,	LDO_E,	0,		OP_LD),
	OPTABLE(STO_D,	LDO_H,	0,		OP_LD),
	OPTABLE(STO_D,	LDO_L,	0,		OP_LD),
	OPTABLE(STO_D,	LDO_M,	ADR_HL,	OP_LD),
	OPTABLE(STO_D,	LDO_A,	0,		OP_LD),
	OPTABLE(STO_E,	LDO_B,	0,		OP_LD),
	OPTABLE(STO_E,	LDO_C,	0,		OP_LD),
	OPTABLE(STO_E,	LDO_D,	0,		OP_LD),
	OPTABLE(STO_E,	LDO_E,	0,		OP_LD),
	OPTABLE(STO_E,	LDO_H,	0,		OP_LD),
	OPTABLE(STO_E,	LDO_L,	0,		OP_LD),
	OPTABLE(STO_E,	LDO_M,	ADR_HL,	OP_LD),
	OPTABLE(STO_E,	LDO_A,	0,		OP_LD),
	OPTABLE(STO_H,	LDO_B,	0,		OP_LD),
	OPTABLE(STO_H,	LDO_C,	0,		OP_LD),
	OPTABLE(STO_H,	LDO_D,	0,		OP_LD),
	OPTABLE(STO_H,	LDO_E,	0,		OP_LD),
	OPTABLE(STO_H,	LDO_H,	0,		OP_LD),
	OPTABLE(STO_H,	LDO_L,	0,		OP_LD),
	OPTABLE(STO_H,	LDO_M,	ADR_HL,	OP_LD),
	OPTABLE(STO_H,	LDO_A,	0,		OP_LD),
	OPTABLE(STO_L,	LDO_B,	0,		OP_LD),
	OPTABLE(STO_L,	LDO_C,	0,		OP_LD),
	OPTABLE(STO_L,	LDO_D,	0,		OP_LD),
	OPTABLE(STO_L,	LDO_E,	0,		OP_LD),
	OPTABLE(STO_L,	LDO_H,	0,		OP_LD),
	OPTABLE(STO_L,	LDO_L,	0,		OP_LD),
	OPTABLE(STO_L,	LDO_M,	ADR_HL,	OP_LD),
	OPTABLE(STO_L,	LDO_A,	0,		OP_LD),
	OPTABLE(STO_M,	LDO_B,	ADR_HL,	OP_LD),
	OPTABLE(STO_M,	LDO_C,	ADR_HL,	OP_LD),
	OPTABLE(STO_M,	LDO_D,	ADR_HL,	OP_LD),
	OPTABLE(STO_M,	LDO_E,	ADR_HL,	OP_LD),
	OPTABLE(STO_M,	LDO_H,	ADR_HL,	OP_LD),
	OPTABLE(STO_M,	LDO_L,	ADR_HL,	OP_LD),
	OPTABLE(0,		0,		0,		OP_HALT),
	OPTABLE(STO_M,	LDO_A,	ADR_HL,	OP_LD),
	OPTABLE(STO_A,	LDO_B,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_C,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_D,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_E,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_H,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_L,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_LD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_LD),

/* DMG 80-BF  10?????? */
/* DMG 80 ADD 10000rrr */
	OPTABLE(STO_A,	LDO_B,	0,		OP_ADD),
	OPTABLE(STO_A,	LDO_C,	0,		OP_ADD),
	OPTABLE(STO_A,	LDO_D,	0,		OP_ADD),
	OPTABLE(STO_A,	LDO_E,	0,		OP_ADD),
	OPTABLE(STO_A,	LDO_H,	0,		OP_ADD),
	OPTABLE(STO_A,	LDO_L,	0,		OP_ADD),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_ADD),
	OPTABLE(STO_A,	LDO_A,	0,		OP_ADD),
/* DMG 88 ADC 10001rrr */
	OPTABLE(STO_A,	LDO_B,	0,		OP_ADC),
	OPTABLE(STO_A,	LDO_C,	0,		OP_ADC),
	OPTABLE(STO_A,	LDO_D,	0,		OP_ADC),
	OPTABLE(STO_A,	LDO_E,	0,		OP_ADC),
	OPTABLE(STO_A,	LDO_H,	0,		OP_ADC),
	OPTABLE(STO_A,	LDO_L,	0,		OP_ADC),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_ADC),
	OPTABLE(STO_A,	LDO_A,	0,		OP_ADC),
/* DMG 90 SBB 10010rrr */
	OPTABLE(STO_A,	LDO_B,	0,		OP_SUB),
	OPTABLE(STO_A,	LDO_C,	0,		OP_SUB),
	OPTABLE(STO_A,	LDO_D,	0,		OP_SUB),
	OPTABLE(STO_A,	LDO_E,	0,		OP_SUB),
	OPTABLE(STO_A,	LDO_H,	0,		OP_SUB),
	OPTABLE(STO_A,	LDO_L,	0,		OP_SUB),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_SUB),
	OPTABLE(STO_A,	LDO_A,	0,		OP_SUB),
/* DMG 98 SBC 10011rrr */
	OPTABLE(STO_A,	LDO_B,	0,		OP_SBC),
	OPTABLE(STO_A,	LDO_C,	0,		OP_SBC),
	OPTABLE(STO_A,	LDO_D,	0,		OP_SBC),
	OPTABLE(STO_A,	LDO_E,	0,		OP_SBC),
	OPTABLE(STO_A,	LDO_H,	0,		OP_SBC),
	OPTABLE(STO_A,	LDO_L,	0,		OP_SBC),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_SBC),
	OPTABLE(STO_A,	LDO_A,	0,		OP_SBC),
/* DMG A0 AND 10100rrr */
	OPTABLE(STO_A,	LDO_B,	0,		OP_AND),
	OPTABLE(STO_A,	LDO_C,	0,		OP_AND),
	OPTABLE(STO_A,	LDO_D,	0,		OP_AND),
	OPTABLE(STO_A,	LDO_E,	0,		OP_AND),
	OPTABLE(STO_A,	LDO_H,	0,		OP_AND),
	OPTABLE(STO_A,	LDO_L,	0,		OP_AND),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_AND),
	OPTABLE(STO_A,	LDO_A,	0,		OP_AND),
/* DMG A8 XOR 10101rrr */
	OPTABLE(STO_A,	LDO_B,	0,		OP_XOR),
	OPTABLE(STO_A,	LDO_C,	0,		OP_XOR),
	OPTABLE(STO_A,	LDO_D,	0,		OP_XOR),
	OPTABLE(STO_A,	LDO_E,	0,		OP_XOR),
	OPTABLE(STO_A,	LDO_H,	0,		OP_XOR),
	OPTABLE(STO_A,	LDO_L,	0,		OP_XOR),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_XOR),
	OPTABLE(STO_A,	LDO_A,	0,		OP_XOR),

/* DMG B0 OR  10110rrr */
	OPTABLE(STO_A,	LDO_B,	0,		OP_OR),
	OPTABLE(STO_A,	LDO_C,	0,		OP_OR),
	OPTABLE(STO_A,	LDO_D,	0,		OP_OR),
	OPTABLE(STO_A,	LDO_E,	0,		OP_OR),
	OPTABLE(STO_A,	LDO_H,	0,		OP_OR),
	OPTABLE(STO_A,	LDO_L,	0,		OP_OR),
	OPTABLE(STO_A,	LDO_M,	ADR_HL,	OP_OR),
	OPTABLE(STO_A,	LDO_A,	0,		OP_OR),
/* DMG B8 CP  10111rrr */
	OPTABLE(0,		LDO_B,	0,		OP_CP),
	OPTABLE(0,		LDO_C,	0,		OP_CP),
	OPTABLE(0,		LDO_D,	0,		OP_CP),
	OPTABLE(0,		LDO_E,	0,		OP_CP),
	OPTABLE(0,		LDO_H,	0,		OP_CP),
	OPTABLE(0,		LDO_L,	0,		OP_CP),
	OPTABLE(0,		LDO_M,	ADR_HL,	OP_CP),
	OPTABLE(0,		LDO_A,	0,		OP_CP),

/* DMG C0-FF  11?????? */
/* DMG C0-C7  11000??? */
	OPTABLE(0,		0,		0,		OP_RETCC),
	OPTABLE(STO_BC,	LDO_ST,	0,		OP_POP),
	OPTABLE(0,		LDO_NN,	0,		OP_JPCC),
	OPTABLE(0,		LDO_NN,	0,		OP_JP),
	OPTABLE(0,		LDO_NN,	0,		OP_CALLCC),
	OPTABLE(STO_ST,	LDO_BC,	0,		OP_PUSH),
	OPTABLE(STO_A,	LDO_N,	0,		OP_ADD),
	OPTABLE(0,		0,		0,		OP_RST),
/* DMG C8-CF  11001??? */
	OPTABLE(0,		0,		0,		OP_RETCC),
	OPTABLE(0,		LDO_ST,	0,		OP_RET),
	OPTABLE(0,		LDO_NN,	0,		OP_JPCC),
	OPTABLE(0,		0,		0,		OP_PREFIX_CB),
	OPTABLE(0,		LDO_NN,	0,		OP_CALLCC),
	OPTABLE(0,		LDO_NN,	0,		OP_CALL),
	OPTABLE(STO_A,	LDO_N,	0,		OP_ADC),
	OPTABLE(0,		0,		0,		OP_RST),
/* DMG D0-D7  11010??? */
	OPTABLE(0,		0,		0,		OP_RETCC),
	OPTABLE(STO_DE,	LDO_ST,	0,		OP_POP),
	OPTABLE(0,		LDO_NN,	0,		OP_JPCC),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(0,		LDO_NN,	0,		OP_CALLCC),
	OPTABLE(STO_ST,	LDO_DE,	0,		OP_PUSH),
	OPTABLE(STO_A,	LDO_N,	0,		OP_SUB),
	OPTABLE(0,		0,		0,		OP_RST),
/* DMG D8-DF  11011??? */
	OPTABLE(0,		0,		0,		OP_RETCC),
	OPTABLE(0,		LDO_ST,	0,		OP_RETI),
	OPTABLE(0,		LDO_NN,	0,		OP_JPCC),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(0,		LDO_NN,	0,		OP_CALLCC),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(STO_A,	LDO_N,	0,		OP_SBC),
	OPTABLE(0,		0,		0,		OP_RST),
/* DMG E0-E7  11010??? */
	OPTABLE(STO_M,	LDO_A,	ADR_HN,	OP_LD),
	OPTABLE(STO_HL,	LDO_ST,	0,		OP_POP),
	OPTABLE(STO_M,	LDO_A,	ADR_HC,	OP_LD),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(STO_ST,	LDO_HL,	0,		OP_PUSH),
	OPTABLE(STO_A,	LDO_N,	0,		OP_AND),
	OPTABLE(0,		0,		0,		OP_RST),
/* DMG E8-EF  11011??? */
	OPTABLE(STO_SP,	LDO_SN,	ADR_SP,	OP_ADDSP),
	OPTABLE(0,		LDO_HL,	0,		OP_JP),
	OPTABLE(STO_M,	LDO_A,	ADR_NN,	OP_LD),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(STO_A,	LDO_N,	0,		OP_XOR),
	OPTABLE(0,		0,		0,		OP_RST),
/* DMG F0-F7  11110??? */
	OPTABLE(STO_A,	LDO_M,	ADR_HN,	OP_LD),
	OPTABLE(STO_AFDMG,	LDO_ST,	0,		OP_POP),
	OPTABLE(STO_A,	LDO_M,	ADR_HC,	OP_LD),
	OPTABLE(0,		0,		0,		OP_DI),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(STO_ST,	LDO_AFDMG,	0,		OP_PUSH),
	OPTABLE(STO_A,	LDO_N,	0,		OP_OR),
	OPTABLE(0,		0,		0,		OP_RST),
/* DMG F8-FF  11111??? */
	OPTABLE(STO_HL,	LDO_SN,	ADR_SP,	OP_ADDSP),
	OPTABLE(STO_SP,	LDO_HL,	0,		OP_LD),
	OPTABLE(STO_A,	LDO_M,	ADR_NN,	OP_LD),
	OPTABLE(0,		0,		0,		OP_EI),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(0,		0,		0,		OP_NOP),
	OPTABLE(0,		LDO_N,	0,		OP_CP),
	OPTABLE(0,		0,		0,		OP_RST),
};

const static Uint8 kmdmg_ot_cbxx[0x20] = {
	OP_RLC, OP_RRC, OP_RL,  OP_RR,  OP_SLA, OP_SRA, OP_SWAP, OP_SRL,
	OP_BIT, OP_BIT, OP_BIT, OP_BIT, OP_BIT, OP_BIT, OP_BIT, OP_BIT,
	OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES, OP_RES,
	OP_SET, OP_SET, OP_SET, OP_SET, OP_SET, OP_SET, OP_SET, OP_SET,
};

const static Uint8 kmdmg_ct[0x510] = {
/* DMG 追加クロック */
/* XX		0 1 2 3 4 5 6 7  8 9 A B C D E F */
/* 0 */		0,0,0,4,0,0,0,0, 0,4,0,4,0,0,0,0,
/* 1 */	   28,0,0,4,0,0,0,0, 4,4,0,4,0,0,0,0,
/* 2 */		0,0,0,4,0,0,0,0, 0,4,0,4,0,0,0,0,
/* 3 */		0,0,0,4,0,0,0,0, 0,4,0,4,0,0,0,0,
/* 4 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 5 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 6 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 7 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 8 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 9 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* A */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* B */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* C */		4,0,0,4,0,4,0,4, 4,4,0,0,0,4,0,4,
/* D */		4,0,0,0,0,4,0,4, 4,4,0,0,0,0,0,4,
/* E */		0,0,4,0,0,4,0,4, 8,0,0,0,0,0,0,4,
/* F */		0,0,4,0,0,4,0,4, 8,0,0,0,0,0,0,4,

/* XXXX		0 1 2 3 4 5 6 7  8 9 A B C D E F */
/* 0 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 1 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 2 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 3 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 4 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 5 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 6 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 7 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 8 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 9 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* A */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* B */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* C */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* D */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* E */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* F */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

/* CBXX		0 1 2 3 4 5 6 7  8 9 A B C D E F */
/* 0 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 1 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 2 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 3 */		4,4,4,4,4,4,4,4, 0,0,0,0,0,0,0,0,
/* 4 */		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
/* 5 */		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
/* 6 */		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
/* 7 */		4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,
/* 8 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 9 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* A */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* B */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* C */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* D */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* E */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* F */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

/* XXXX		0 1 2 3 4 5 6 7  8 9 A B C D E F */
/* 0 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 1 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 2 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 3 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 4 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 5 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 6 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 7 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 8 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 9 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* A */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* B */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* C */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* D */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* E */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* F */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

/* XXXX		0 1 2 3 4 5 6 7  8 9 A B C D E F */
/* 0 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 1 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 2 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 3 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 4 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 5 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 6 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 7 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 8 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* 9 */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* A */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* B */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* C */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* D */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* E */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
/* F */		0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

	0,	/* 0x500 DJNZ */
	0,	/* 0x501  不成立 */
	1,	/* 0x502 CALL cc */
	0,	/* 0x503  不成立 */
	1,	/* 0x504 JR cc */
	0,	/* 0x505  不成立 */
	1,	/* 0x506 JP cc */
	0,	/* 0x507  不成立 */
	0,	/* 0x508 RET cc */
	0,	/* 0x509  不成立 */
	0,	/* 0x50A CPDR CPIR INDR INIR LDDR LDIR ODIR OTIR */
	0,	/* 0x50B  不成立 */
	0,	/* 0x50C */
	0,	/* 0x50D  不成立 */
	0,	/* 0x50E OTIMR OTDMR */
	0,	/* 0x50F  不成立 */
};

static Uint32 kmdmg_memread(KMZ80_CONTEXT *context, Uint32 a)
{
	CYCLEMEM;
	return context->memread(context->user, a);
}

static void kmdmg_memwrite(KMZ80_CONTEXT *context, Uint32 a, Uint32 d)
{
	CYCLEMEM;
	context->memwrite(context->user, a, d);
}


void kmdmg_reset(KMZ80_CONTEXT *context) {
	extern void kmz80_reset_common(KMZ80_CONTEXT *context);
	kmz80_reset_common(context);
	EXFLAG = 0/*EXF_ICEXIST*/;
	IMODE = 3;
	M1CYCLE = 0;
	MEMCYCLE = 4;
	IOCYCLE = 4;
	context->opt = (void *)kmdmg_ot_xx;
	context->optcb = (void *)kmdmg_ot_cbxx;
	context->opted = (void *)0;
	context->cyt = (void *)kmdmg_ct;
	SYSMEMREAD = kmdmg_memread;
	SYSMEMWRITE = kmdmg_memwrite;
}
