/****************************************************************************

	NEC V30MZ(V20/V30/V33) emulator

	Small changes made by toshi (Cycle count macros changed  , "THROUGH" macro added)	

	Small changes made by dox@space.pl (Corrected bug in NEG instruction , different AUX flag handling in some opcodes)	

	(Re)Written June-September 2000 by Bryan McPhail (mish@tendril.co.uk) based
	on code by Oliver Bergmann (Raul_Bloodworth@hotmail.com) who based code
	on the i286 emulator by Fabrice Frances which had initial work based on
	David Hedley's pcemu(!).

	This new core features 99% accurate cycle counts for each processor,
	there are still some complex situations where cycle counts are wrong,
	typically where a few instructions have differing counts for odd/even
	source and odd/even destination memory operands.

	Flag settings are also correct for the NEC processors rather than the
	I86 versions.

	Nb:  This emulation should be faster than previous NEC cores, but
	because the old cycle count values were far too high in many cases
	the processor has to do more 'work' than before, so the overall effect
	may	be a slower core.

****************************************************************************/


#include <stdio.h>
#include <string.h>

//#include <windows.h>
#include "wsr_types.h" //YOYOFR

namespace oswan {
#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned int
#define INT8 signed char
#define INT16 signed short
#define INT32 signed int

#include "nec.h"
#include "necintrf.h"

typedef union
{					/* eight general registers */
	UINT16 w[8];	/* viewed as 16 bits registers */
	UINT8  b[16];	/* or as 8 bit registers */
} necbasicregs;

typedef struct
{
	necbasicregs regs;
 	UINT16	sregs[4];

	UINT16	ip;

	INT32	SignVal;
	UINT32	AuxVal, OverVal, ZeroVal, CarryVal, ParityVal; /* 0 or non-0 valued flags */
	UINT8	TF, IF, DF, MF; 	/* 0 or 1 valued flags */	/* OB[19.07.99] added Mode Flag V30 */
	UINT32	int_vector;
	UINT32	pending_irq;
	UINT32	nmi_state;
	UINT32	irq_state;
	int 	(*irq_callback)(int irqline);
} nec_Regs;

/***************************************************************************/
/* cpu state															   */
/***************************************************************************/

int nec_ICount;

static nec_Regs I;

static UINT32 cpu_type;
static UINT32 prefix_base;	/* base address of the latest prefix segment */
char seg_prefix;		/* prefix segment indicator */


/* The interrupt number of a pending external interrupt pending NMI is 2.	*/
/* For INTR interrupts, the level is caught on the bus during an INTA cycle */


#include "necinstr.h"
#include "necea.h"
#include "necmodrm.h"

static int no_interrupt;

static UINT8 parity_table[256];

/***************************************************************************/

void nec_reset (void *param)
{
	unsigned int i,j,c;
	BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };


	memset( &I, 0, sizeof(I) );

	no_interrupt=0;
	I.sregs[CS] = 0xffff;


	for (i = 0;i < 256; i++)
	{
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1) c++;
		parity_table[i] = !(c & 1);
	}

	I.ZeroVal = I.ParityVal = 1;
	SetMD(1);						/* set the mode-flag = native mode */

	for (i = 0; i < 256; i++)
	{
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ( (i & 0x38) >> 3) ;
	}

	for (i = 0xc0; i < 0x100; i++)
	{
		Mod_RM.RM.w[i] = (WREGS)( i & 7 );
		Mod_RM.RM.b[i] = (BREGS)reg_name[i & 7];
	}
}

void nec_exit (void)
{

}




void nec_int(DWORD wektor)
{
  
	DWORD dest_seg, dest_off;

	if(I.IF)
	{
		i_pushf();
		I.TF = I.IF = 0;
		dest_off = ReadWord(wektor);
		dest_seg = ReadWord(wektor+2);
		PUSH(I.sregs[CS]);
		PUSH(I.ip);
		I.ip = (WORD)dest_off;
		I.sregs[CS] = (WORD)dest_seg;
	}
}

static void nec_interrupt(unsigned int_num,BOOLEAN md_flag)
{
	UINT32 dest_seg, dest_off;

	if (int_num == -1)
		return;

	 i_pushf();
	I.TF = I.IF = 0;	
	

	dest_off = ReadWord((int_num)*4);
	dest_seg = ReadWord((int_num)*4+2);

	PUSH(I.sregs[CS]);
	PUSH(I.ip);
	I.ip = (WORD)dest_off;
	I.sregs[CS] = (WORD)dest_seg;

}


/****************************************************************************/
/*							   OPCODES										*/
/****************************************************************************/

#define OP(num,func_name) static void func_name(void)


OP( 0x00, i_add_br8  ) { DEF_br8;	ADDB;	PutbackRMByte(ModRM,dst);	CLKM(3,1);	 	}
OP( 0x01, i_add_wr16 ) { DEF_wr16;	ADDW;	PutbackRMWord(ModRM,dst);	CLKM(3,1);	}
OP( 0x02, i_add_r8b  ) { DEF_r8b;	ADDB;	RegByte(ModRM)=dst;			CLKM(2,1);		}
OP( 0x03, i_add_r16w ) { DEF_r16w;	ADDW;	RegWord(ModRM)=dst;			CLKM(2,1);	}
OP( 0x04, i_add_ald8 ) { DEF_ald8;	ADDB;	I.regs.b[AL]=dst;			CLK(1);				}
OP( 0x05, i_add_axd16) { DEF_axd16;	ADDW;	I.regs.w[AW]=dst;			CLK(1);				}
OP( 0x06, i_push_es  ) { PUSH(I.sregs[ES]);	CLK(2); 	}
OP( 0x07, i_pop_es	 ) { POP(I.sregs[ES]);	CLK(3);	}

OP( 0x08, i_or_br8	 ) { DEF_br8;	ORB;	PutbackRMByte(ModRM,dst);	CLKM(3,1);		}
OP( 0x09, i_or_wr16  ) { DEF_wr16;	ORW;	PutbackRMWord(ModRM,dst);	CLKM(3,1);	}
OP( 0x0a, i_or_r8b	 ) { DEF_r8b;	ORB;	RegByte(ModRM)=dst;			CLKM(2,1);		}
OP( 0x0b, i_or_r16w  ) { DEF_r16w;	ORW;	RegWord(ModRM)=dst;			CLKM(2,1);	}
OP( 0x0c, i_or_ald8  ) { DEF_ald8;	ORB;	I.regs.b[AL]=dst;			CLK(1);				}
OP( 0x0d, i_or_axd16 ) { DEF_axd16;	ORW;	I.regs.w[AW]=dst;			CLK(1);				}
OP( 0x0e, i_push_cs  ) { PUSH(I.sregs[CS]);	CLK(2);	}
OP( 0x0f, i_pre_nec  ) { UINT32 ModRM, tmp, tmp2; /* pop cs at V30MZ? */
	switch (FETCH) {
		case 0x10 : BITOP_BYTE;	CLKS(3,3,4); tmp2 = I.regs.b[CL] & 0x7;	I.ZeroVal = (tmp & (1<<tmp2)) ? 1 : 0;	I.CarryVal=I.OverVal=0; break; /* Test */
		case 0x11 : BITOP_WORD;	CLKS(3,3,4); tmp2 = I.regs.b[CL] & 0xf;	I.ZeroVal = (tmp & (1<<tmp2)) ? 1 : 0;	I.CarryVal=I.OverVal=0; break; /* Test */
		case 0x12 : BITOP_BYTE;	CLKS(5,5,4); tmp2 = I.regs.b[CL] & 0x7;	tmp &= ~(1<<tmp2);	PutbackRMByte(ModRM,tmp);	break; /* Clr */
		case 0x13 : BITOP_WORD;	CLKS(5,5,4); tmp2 = I.regs.b[CL] & 0xf;	tmp &= ~(1<<tmp2);	PutbackRMWord(ModRM,tmp);	break; /* Clr */
		case 0x14 : BITOP_BYTE;	CLKS(4,4,4); tmp2 = I.regs.b[CL] & 0x7;	tmp |= (1<<tmp2);	PutbackRMByte(ModRM,tmp);	break; /* Set */
		case 0x15 : BITOP_WORD;	CLKS(4,4,4); tmp2 = I.regs.b[CL] & 0xf;	tmp |= (1<<tmp2);	PutbackRMWord(ModRM,tmp);	break; /* Set */
		case 0x16 : BITOP_BYTE;	CLKS(4,4,4); tmp2 = I.regs.b[CL] & 0x7;	BIT_NOT;			PutbackRMByte(ModRM,tmp);	break; /* Not */
		case 0x17 : BITOP_WORD;	CLKS(4,4,4); tmp2 = I.regs.b[CL] & 0xf;	BIT_NOT;			PutbackRMWord(ModRM,tmp);	break; /* Not */

		case 0x18 : BITOP_BYTE;	CLKS(4,4,4); tmp2 = (FETCH) & 0x7;	I.ZeroVal = (tmp & (1<<tmp2)) ? 1 : 0;	I.CarryVal=I.OverVal=0; break; /* Test */
		case 0x19 : BITOP_WORD;	CLKS(4,4,4); tmp2 = (FETCH) & 0xf;	I.ZeroVal = (tmp & (1<<tmp2)) ? 1 : 0;	I.CarryVal=I.OverVal=0; break; /* Test */
		case 0x1a : BITOP_BYTE;	CLKS(6,6,4); tmp2 = (FETCH) & 0x7;	tmp &= ~(1<<tmp2);		PutbackRMByte(ModRM,tmp);	break; /* Clr */
		case 0x1b : BITOP_WORD;	CLKS(6,6,4); tmp2 = (FETCH) & 0xf;	tmp &= ~(1<<tmp2);		PutbackRMWord(ModRM,tmp);	break; /* Clr */
		case 0x1c : BITOP_BYTE;	CLKS(5,5,4); tmp2 = (FETCH) & 0x7;	tmp |= (1<<tmp2);		PutbackRMByte(ModRM,tmp);	break; /* Set */
		case 0x1d : BITOP_WORD;	CLKS(5,5,4); tmp2 = (FETCH) & 0xf;	tmp |= (1<<tmp2);		PutbackRMWord(ModRM,tmp);	break; /* Set */
		case 0x1e : BITOP_BYTE;	CLKS(5,5,4); tmp2 = (FETCH) & 0x7;	BIT_NOT;				PutbackRMByte(ModRM,tmp);	break; /* Not */
		case 0x1f : BITOP_WORD;	CLKS(5,5,4); tmp2 = (FETCH) & 0xf;	BIT_NOT;				PutbackRMWord(ModRM,tmp);	break; /* Not */

		case 0x20 :	ADD4S; CLKS(7,7,2); break;
		case 0x22 :	SUB4S; CLKS(7,7,2); break;
		case 0x26 :	CMP4S; CLKS(7,7,2); break;
		case 0x28 : ModRM = FETCH; tmp = GetRMByte(ModRM); tmp <<= 4; tmp |= I.regs.b[AL] & 0xf; I.regs.b[AL] = (I.regs.b[AL] & 0xf0) | ((tmp>>8)&0xf); tmp &= 0xff; PutbackRMByte(ModRM,tmp); CLKM(9,15); break;
		case 0x2a : ModRM = FETCH; tmp = GetRMByte(ModRM); tmp2 = (I.regs.b[AL] & 0xf)<<4; I.regs.b[AL] = (I.regs.b[AL] & 0xf0) | (tmp&0xf); tmp = tmp2 | (tmp>>4);	PutbackRMByte(ModRM,tmp); CLKM(13,19); break;
		case 0x31 : ModRM = FETCH; ModRM=0; break;
		case 0x33 : ModRM = FETCH; ModRM=0; break;
		case 0x92 : CLK(2); break; /* V25/35 FINT */
		case 0xe0 : ModRM = FETCH; ModRM=0; break;
		case 0xf0 : ModRM = FETCH; ModRM=0; break;
		case 0xff : ModRM = FETCH; ModRM=0; break;
		default:	break;
	}
}

OP( 0x10, i_adc_br8  ) { DEF_br8;	src+=CF;	ADDB;	PutbackRMByte(ModRM,dst);	CLKM(3,1); 		}
OP( 0x11, i_adc_wr16 ) { DEF_wr16;	src+=CF;	ADDW;	PutbackRMWord(ModRM,dst);	CLKM(3,1);	}
OP( 0x12, i_adc_r8b  ) { DEF_r8b;	src+=CF;	ADDB;	RegByte(ModRM)=dst;			CLKM(2,1); 		}
OP( 0x13, i_adc_r16w ) { DEF_r16w;	src+=CF;	ADDW;	RegWord(ModRM)=dst;			CLKM(2,1);	}
OP( 0x14, i_adc_ald8 ) { DEF_ald8;	src+=CF;	ADDB;	I.regs.b[AL]=dst;			CLK(1);				}
OP( 0x15, i_adc_axd16) { DEF_axd16;	src+=CF;	ADDW;	I.regs.w[AW]=dst;			CLK(1);				}
OP( 0x16, i_push_ss  ) { PUSH(I.sregs[SS]);		CLK(2);	}
OP( 0x17, i_pop_ss	 ) { POP(I.sregs[SS]);		CLK(3);	no_interrupt=1; }

OP( 0x18, i_sbb_br8  ) { DEF_br8;	src+=CF;	SUBB;	PutbackRMByte(ModRM,dst);	CLKM(3,1); 		}
OP( 0x19, i_sbb_wr16 ) { DEF_wr16;	src+=CF;	SUBW;	PutbackRMWord(ModRM,dst);	CLKM(3,1);	}
OP( 0x1a, i_sbb_r8b  ) { DEF_r8b;	src+=CF;	SUBB;	RegByte(ModRM)=dst;			CLKM(2,1); 		}
OP( 0x1b, i_sbb_r16w ) { DEF_r16w;	src+=CF;	SUBW;	RegWord(ModRM)=dst;			CLKM(2,1);	}
OP( 0x1c, i_sbb_ald8 ) { DEF_ald8;	src+=CF;	SUBB;	I.regs.b[AL]=dst;			CLK(1); 				}
OP( 0x1d, i_sbb_axd16) { DEF_axd16;	src+=CF;	SUBW;	I.regs.w[AW]=dst;			CLK(1);	}
OP( 0x1e, i_push_ds  ) { PUSH(I.sregs[DS]);		CLK(2);	}
OP( 0x1f, i_pop_ds	 ) { POP(I.sregs[DS]);		CLK(3);	}

OP( 0x20, i_and_br8  ) { DEF_br8;	ANDB;	PutbackRMByte(ModRM,dst);	CLKM(3,1); 		}
OP( 0x21, i_and_wr16 ) { DEF_wr16;	ANDW;	PutbackRMWord(ModRM,dst);	CLKM(3,1);	}
OP( 0x22, i_and_r8b  ) { DEF_r8b;	ANDB;	RegByte(ModRM)=dst;			CLKM(2,1);		}
OP( 0x23, i_and_r16w ) { DEF_r16w;	ANDW;	RegWord(ModRM)=dst;			CLKM(2,1);	}
OP( 0x24, i_and_ald8 ) { DEF_ald8;	ANDB;	I.regs.b[AL]=dst;			CLK(1);				}
OP( 0x25, i_and_axd16) { DEF_axd16;	ANDW;	I.regs.w[AW]=dst;			CLK(1);	}
OP( 0x26, i_es		 ) { seg_prefix=TRUE;	prefix_base=I.sregs[ES]<<4;	CLK(1);		nec_instruction[FETCHOP]();	seg_prefix=FALSE; }
OP( 0x27, i_daa 	 ) { ADJ4(6,0x60);									CLK(10);	}

OP( 0x28, i_sub_br8  ) { DEF_br8;	SUBB;	PutbackRMByte(ModRM,dst);	CLKM(3,1); 		}
OP( 0x29, i_sub_wr16 ) { DEF_wr16;	SUBW;	PutbackRMWord(ModRM,dst);	CLKM(3,1);	}
OP( 0x2a, i_sub_r8b  ) { DEF_r8b;	SUBB;	RegByte(ModRM)=dst;			CLKM(2,1); 		}
OP( 0x2b, i_sub_r16w ) { DEF_r16w;	SUBW;	RegWord(ModRM)=dst;			CLKM(2,1);	}
OP( 0x2c, i_sub_ald8 ) { DEF_ald8;	SUBB;	I.regs.b[AL]=dst;			CLK(1); 				}
OP( 0x2d, i_sub_axd16) { DEF_axd16;	SUBW;	I.regs.w[AW]=dst;			CLK(1);	}
OP( 0x2e, i_cs		 ) { seg_prefix=TRUE;	prefix_base=I.sregs[CS]<<4;	CLK(1);		nec_instruction[FETCHOP]();	seg_prefix=FALSE; }
OP( 0x2f, i_das 	 ) { ADJ4(-6,-0x60);								CLK(10);	}

OP( 0x30, i_xor_br8  ) { DEF_br8;	XORB;	PutbackRMByte(ModRM,dst);	CLKM(3,1);		}
OP( 0x31, i_xor_wr16 ) { DEF_wr16;	XORW;	PutbackRMWord(ModRM,dst);	CLKM(3,1);	}
OP( 0x32, i_xor_r8b  ) { DEF_r8b;	XORB;	RegByte(ModRM)=dst;			CLKM(2,1); 		}
OP( 0x33, i_xor_r16w ) { DEF_r16w;	XORW;	RegWord(ModRM)=dst;			CLKM(2,1);	}
OP( 0x34, i_xor_ald8 ) { DEF_ald8;	XORB;	I.regs.b[AL]=dst;			CLK(1); 				}
OP( 0x35, i_xor_axd16) { DEF_axd16;	XORW;	I.regs.w[AW]=dst;			CLK(1);	}
OP( 0x36, i_ss		 ) { seg_prefix=TRUE;	prefix_base=I.sregs[SS]<<4;	CLK(1);		nec_instruction[FETCHOP]();	seg_prefix=FALSE; }
OP( 0x37, i_aaa 	 ) { ADJB(6,1);										CLK(9); 	}

OP( 0x38, i_cmp_br8  ) { DEF_br8;	SUBB;					CLKM(2,1); }
OP( 0x39, i_cmp_wr16 ) { DEF_wr16;	SUBW;					CLKM(2,1);	}
OP( 0x3a, i_cmp_r8b  ) { DEF_r8b;	SUBB;					CLKM(2,1); }
OP( 0x3b, i_cmp_r16w ) { DEF_r16w;	SUBW;					CLKM(2,1);	}
OP( 0x3c, i_cmp_ald8 ) { DEF_ald8;	SUBB;					CLK(1); }
OP( 0x3d, i_cmp_axd16) { DEF_axd16;	SUBW;					CLK(1);	}
OP( 0x3e, i_ds		 ) { seg_prefix=TRUE;	prefix_base=I.sregs[DS]<<4;	CLK(1);		nec_instruction[FETCHOP]();	seg_prefix=FALSE; }
OP( 0x3f, i_aas 	 ) { ADJB(-6,-1);						CLK(9);	}

OP( 0x40, i_inc_ax	) { IncWordReg(AW);						CLK(1);	}
OP( 0x41, i_inc_cx	) { IncWordReg(CW);						CLK(1);	}
OP( 0x42, i_inc_dx	) { IncWordReg(DW);						CLK(1);	}
OP( 0x43, i_inc_bx	) { IncWordReg(BW);						CLK(1);	}
OP( 0x44, i_inc_sp	) { IncWordReg(SP);						CLK(1);	}
OP( 0x45, i_inc_bp	) { IncWordReg(BP);						CLK(1);	}
OP( 0x46, i_inc_si	) { IncWordReg(IX);						CLK(1);	}
OP( 0x47, i_inc_di	) { IncWordReg(IY);						CLK(1);	}

OP( 0x48, i_dec_ax	) { DecWordReg(AW);						CLK(1);	}
OP( 0x49, i_dec_cx	) { DecWordReg(CW);						CLK(1);	}
OP( 0x4a, i_dec_dx	) { DecWordReg(DW);						CLK(1);	}
OP( 0x4b, i_dec_bx	) { DecWordReg(BW);						CLK(1);	}
OP( 0x4c, i_dec_sp	) { DecWordReg(SP);						CLK(1);	}
OP( 0x4d, i_dec_bp	) { DecWordReg(BP);						CLK(1);	}
OP( 0x4e, i_dec_si	) { DecWordReg(IX);						CLK(1);	}
OP( 0x4f, i_dec_di	) { DecWordReg(IY);						CLK(1);	}

OP( 0x50, i_push_ax ) { PUSH(I.regs.w[AW]);					CLK(1); }
OP( 0x51, i_push_cx ) { PUSH(I.regs.w[CW]);					CLK(1); }
OP( 0x52, i_push_dx ) { PUSH(I.regs.w[DW]);					CLK(1); }
OP( 0x53, i_push_bx ) { PUSH(I.regs.w[BW]);					CLK(1); }
OP( 0x54, i_push_sp ) { PUSH(I.regs.w[SP]);					CLK(1); }
OP( 0x55, i_push_bp ) { PUSH(I.regs.w[BP]);					CLK(1); }
OP( 0x56, i_push_si ) { PUSH(I.regs.w[IX]);					CLK(1); }
OP( 0x57, i_push_di ) { PUSH(I.regs.w[IY]);					CLK(1); }

OP( 0x58, i_pop_ax	) { POP(I.regs.w[AW]);					CLK(1); }
OP( 0x59, i_pop_cx	) { POP(I.regs.w[CW]);					CLK(1); }
OP( 0x5a, i_pop_dx	) { POP(I.regs.w[DW]);					CLK(1); }
OP( 0x5b, i_pop_bx	) { POP(I.regs.w[BW]);					CLK(1); }
OP( 0x5c, i_pop_sp	) { POP(I.regs.w[SP]);					CLK(1); }
OP( 0x5d, i_pop_bp	) { POP(I.regs.w[BP]);					CLK(1); }
OP( 0x5e, i_pop_si	) { POP(I.regs.w[IX]);					CLK(1); }
OP( 0x5f, i_pop_di	) { POP(I.regs.w[IY]);					CLK(1); }

OP( 0x60, i_pusha  ) {
	unsigned tmp=I.regs.w[SP];
	PUSH(I.regs.w[AW]);
	PUSH(I.regs.w[CW]);
	PUSH(I.regs.w[DW]);
	PUSH(I.regs.w[BW]);
	PUSH(tmp);
	PUSH(I.regs.w[BP]);
	PUSH(I.regs.w[IX]);
	PUSH(I.regs.w[IY]);
	CLK(9);
}
OP( 0x61, i_popa  ) {
	unsigned tmp;
	POP(I.regs.w[IY]);
	POP(I.regs.w[IX]);
	POP(I.regs.w[BP]);
	POP(tmp);
	POP(I.regs.w[BW]);
	POP(I.regs.w[DW]);
	POP(I.regs.w[CW]);
	POP(I.regs.w[AW]);
	CLK(8);
}
OP( 0x62, i_chkind	) {
	UINT32 low,high,tmp;
	GetModRM;
	low = GetRMWord(ModRM);
	high= GetnextRMWord;
	tmp= RegWord(ModRM);
	if (tmp<low || tmp>high) {
		nec_interrupt(5,0);
		CLK(7);
	}
 	CLK(13);
}

/* OP 0x64 - 0x67 is nop at V30MZ */
OP( 0x64, i_repnc  ) { 	UINT32 next = FETCHOP;	UINT16 c = I.regs.w[CW];
	switch(next) { /* Segments */
		case 0x26:	seg_prefix=TRUE;	prefix_base=I.sregs[ES]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x2e:	seg_prefix=TRUE;	prefix_base=I.sregs[CS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x36:	seg_prefix=TRUE;	prefix_base=I.sregs[SS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x3e:	seg_prefix=TRUE;	prefix_base=I.sregs[DS]<<4;	next = FETCHOP;	CLK(2); break;
	}

	switch(next) {
		case 0x6c:	CLK(2); if (c) do { i_insb();  c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0x6d:	CLK(2); if (c) do { i_insw();  c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0x6e:	CLK(2); if (c) do { i_outsb(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0x6f:	CLK(2); if (c) do { i_outsw(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xa4:	CLK(2); if (c) do { i_movsb(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xa5:	CLK(2); if (c) do { i_movsw(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xa6:	CLK(2); if (c) do { i_cmpsb(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xa7:	CLK(2); if (c) do { i_cmpsw(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xaa:	CLK(2); if (c) do { i_stosb(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xab:	CLK(2); if (c) do { i_stosw(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xac:	CLK(2); if (c) do { i_lodsb(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xad:	CLK(2); if (c) do { i_lodsw(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xae:	CLK(2); if (c) do { i_scasb(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		case 0xaf:	CLK(2); if (c) do { i_scasw(); c--; } while (c>0 && !CF); I.regs.w[CW]=c; break;
		default:		nec_instruction[next]();
	}
	seg_prefix=FALSE;
}

OP( 0x65, i_repc  ) { 	UINT32 next = FETCHOP;	UINT16 c = I.regs.w[CW];
	switch(next) { /* Segments */
		case 0x26:	seg_prefix=TRUE;	prefix_base=I.sregs[ES]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x2e:	seg_prefix=TRUE;	prefix_base=I.sregs[CS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x36:	seg_prefix=TRUE;	prefix_base=I.sregs[SS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x3e:	seg_prefix=TRUE;	prefix_base=I.sregs[DS]<<4;	next = FETCHOP;	CLK(2); break;
	}

	switch(next) {
		case 0x6c:	CLK(2); if (c) do { i_insb();  c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0x6d:	CLK(2); if (c) do { i_insw();  c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0x6e:	CLK(2); if (c) do { i_outsb(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0x6f:	CLK(2); if (c) do { i_outsw(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0xa4:	CLK(2); if (c) do { i_movsb(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0xa5:	CLK(2); if (c) do { i_movsw(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0xa6:	CLK(2); if (c) do { i_cmpsb(); c--; } while (c>0 && CF);	I.regs.w[CW]=c; break;
		case 0xa7:	CLK(2); if (c) do { i_cmpsw(); c--; } while (c>0 && CF);	I.regs.w[CW]=c; break;
		case 0xaa:	CLK(2); if (c) do { i_stosb(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0xab:	CLK(2); if (c) do { i_stosw(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0xac:	CLK(2); if (c) do { i_lodsb(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0xad:	CLK(2); if (c) do { i_lodsw(); c--; } while (c>0 && CF);	I.regs.w[CW]=c;	break;
		case 0xae:	CLK(2); if (c) do { i_scasb(); c--; } while (c>0 && CF);	I.regs.w[CW]=c; break;
		case 0xaf:	CLK(2); if (c) do { i_scasw(); c--; } while (c>0 && CF);	I.regs.w[CW]=c; break;
		default:	nec_instruction[next]();
	}
	seg_prefix=FALSE;
}

OP( 0x68, i_push_d16 ) { UINT32 tmp;	FETCHWORD(tmp); PUSH(tmp);	CLK(1);	}
OP( 0x69, i_imul_d16 ) { UINT32 tmp;	DEF_r16w; 	FETCHWORD(tmp); dst = (INT32)((INT16)src)*(INT32)((INT16)tmp); I.CarryVal = I.OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1);	  RegWord(ModRM)=(WORD)dst; 	CLKM(4,3);}
OP( 0x6a, i_push_d8  ) { UINT32 tmp = (WORD)((INT16)((INT8)FETCH)); 	PUSH(tmp);	CLK(1);	}
OP( 0x6b, i_imul_d8  ) { UINT32 src2; DEF_r16w; src2= (WORD)((INT16)((INT8)FETCH)); dst = (INT32)((INT16)src)*(INT32)((INT16)src2); I.CarryVal = I.OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1); RegWord(ModRM)=(WORD)dst; CLKM(4,3); }
OP( 0x6c, i_insb	 ) { PutMemB(ES,I.regs.w[IY],read_port(I.regs.w[DW])); I.regs.w[IY]+= -2 * I.DF + 1; CLK(6); }
OP( 0x6d, i_insw	 ) { PutMemB(ES,I.regs.w[IY],read_port(I.regs.w[DW])); PutMemB(ES,(I.regs.w[IY]+1)&0xffff,read_port((I.regs.w[DW]+1)&0xffff)); I.regs.w[IY]+= -4 * I.DF + 2; CLK(6); }
OP( 0x6e, i_outsb	 ) { write_port(I.regs.w[DW],GetMemB(DS,I.regs.w[IX])); I.regs.w[IX]+= -2 * I.DF + 1; CLK(7); }
OP( 0x6f, i_outsw	 ) { write_port(I.regs.w[DW],GetMemB(DS,I.regs.w[IX])); write_port((I.regs.w[DW]+1)&0xffff,GetMemB(DS,(I.regs.w[IX]+1)&0xffff)); I.regs.w[IX]+= -4 * I.DF + 2; CLK(7); }

OP( 0x70, i_jo		) { JMP( OF);				CLK(1); }
OP( 0x71, i_jno 	) { JMP(!OF);				CLK(1); }
OP( 0x72, i_jc		) { JMP( CF);				CLK(1); }
OP( 0x73, i_jnc 	) { JMP(!CF);				CLK(1); }
OP( 0x74, i_jz		) { JMP( ZF);				CLK(1); }
OP( 0x75, i_jnz 	) { JMP(!ZF);				CLK(1); }
OP( 0x76, i_jce 	) { JMP(CF || ZF);			CLK(1); }
OP( 0x77, i_jnce	) { JMP(!(CF || ZF));		CLK(1); }
OP( 0x78, i_js		) { JMP( SF);				CLK(1); }
OP( 0x79, i_jns 	) { JMP(!SF);				CLK(1); }
OP( 0x7a, i_jp		) { JMP( PF);				CLK(1); }
OP( 0x7b, i_jnp 	) { JMP(!PF);				CLK(1); }
OP( 0x7c, i_jl		) { JMP((SF!=OF)&&(!ZF));	CLK(1); }
OP( 0x7d, i_jnl 	) { JMP((ZF)||(SF==OF));	CLK(1); }
OP( 0x7e, i_jle 	) { JMP((ZF)||(SF!=OF)); 	CLK(1); }
OP( 0x7f, i_jnle	) { JMP((SF==OF)&&(!ZF));	CLK(1); }

OP( 0x80, i_80pre	) { UINT32 dst, src; GetModRM; dst = GetRMByte(ModRM); src = FETCH;
	CLKM(3,1)
	switch (ModRM & 0x38) {
		case 0x00: ADDB;			PutbackRMByte(ModRM,dst);	break;
		case 0x08: ORB;				PutbackRMByte(ModRM,dst);	break;
		case 0x10: src+=CF;	ADDB;	PutbackRMByte(ModRM,dst);	break;
		case 0x18: src+=CF;	SUBB;	PutbackRMByte(ModRM,dst);	break;
		case 0x20: ANDB;			PutbackRMByte(ModRM,dst);	break;
		case 0x28: SUBB;			PutbackRMByte(ModRM,dst);	break;
		case 0x30: XORB;			PutbackRMByte(ModRM,dst);	break;
		case 0x38: SUBB;			break;	/* CMP */
	}
}

OP( 0x81, i_81pre	) { UINT32 dst, src; GetModRM; dst = GetRMWord(ModRM); src = FETCH; src+= (FETCH << 8);
	CLKM(3,1)
	switch (ModRM & 0x38) {
		case 0x00: ADDW;			PutbackRMWord(ModRM,dst);	break;
		case 0x08: ORW;				PutbackRMWord(ModRM,dst);	break;
		case 0x10: src+=CF;	ADDW;	PutbackRMWord(ModRM,dst);	break;
		case 0x18: src+=CF;	SUBW;	PutbackRMWord(ModRM,dst);	break;
		case 0x20: ANDW;			PutbackRMWord(ModRM,dst);	break;
		case 0x28: SUBW;			PutbackRMWord(ModRM,dst);	break;
		case 0x30: XORW;			PutbackRMWord(ModRM,dst);	break;
		case 0x38: SUBW;			break;	/* CMP */
	}
}

OP( 0x82, i_82pre	) { UINT32 dst, src; GetModRM; dst = GetRMByte(ModRM); src = (BYTE)((INT8)FETCH);
	CLKM(3,1)
	switch (ModRM & 0x38) {
		case 0x00: ADDB;			PutbackRMByte(ModRM,dst);	break;
		case 0x08: ORB;				PutbackRMByte(ModRM,dst);	break;
		case 0x10: src+=CF;	ADDB;	PutbackRMByte(ModRM,dst);	break;
		case 0x18: src+=CF;	SUBB;	PutbackRMByte(ModRM,dst);	break;
		case 0x20: ANDB;			PutbackRMByte(ModRM,dst);	break;
		case 0x28: SUBB;			PutbackRMByte(ModRM,dst);	break;
		case 0x30: XORB;			PutbackRMByte(ModRM,dst);	break;
		case 0x38: SUBB;			break;	/* CMP */
	}
}

OP( 0x83, i_83pre	) { UINT32 dst, src; GetModRM; dst = GetRMWord(ModRM); src = (WORD)((INT16)((INT8)FETCH));
	CLKM(3,1)
	switch (ModRM & 0x38) {
		case 0x00: ADDW;			PutbackRMWord(ModRM,dst);	break;
		case 0x08: ORW;				PutbackRMWord(ModRM,dst);	break;
		case 0x10: src+=CF;	ADDW;	PutbackRMWord(ModRM,dst);	break;
		case 0x18: src+=CF;	SUBW;	PutbackRMWord(ModRM,dst);	break;
		case 0x20: ANDW;			PutbackRMWord(ModRM,dst);	break;
		case 0x28: SUBW;			PutbackRMWord(ModRM,dst);	break;
		case 0x30: XORW;			PutbackRMWord(ModRM,dst);	break;
		case 0x38: SUBW;			break;	/* CMP */
	}
}

OP( 0x84, i_test_br8  ) { DEF_br8;	ANDB;	CLKM(2,1);		}
OP( 0x85, i_test_wr16 ) { DEF_wr16;	ANDW;	CLKM(2,1);	}
OP( 0x86, i_xchg_br8  ) { DEF_br8;	RegByte(ModRM)=dst; PutbackRMByte(ModRM,src); CLKM(5,3); }
OP( 0x87, i_xchg_wr16 ) { DEF_wr16;	RegWord(ModRM)=dst; PutbackRMWord(ModRM,src); CLKM(5,3); }

OP( 0x88, i_mov_br8   ) { UINT8  src; GetModRM; src = RegByte(ModRM); 	PutRMByte(ModRM,src); 	CLKM(1,1); 			}
OP( 0x89, i_mov_wr16  ) { UINT16 src; GetModRM; src = RegWord(ModRM); 	PutRMWord(ModRM,src);	CLKM(1,1); 	}
OP( 0x8a, i_mov_r8b   ) { UINT8  src; GetModRM; src = GetRMByte(ModRM);	RegByte(ModRM)=src;		CLKM(1,1); 		}
OP( 0x8b, i_mov_r16w  ) { UINT16 src; GetModRM; src = GetRMWord(ModRM);	RegWord(ModRM)=src; 	CLKM(1,1); 	}
OP( 0x8c, i_mov_wsreg ) { GetModRM; PutRMWord(ModRM,I.sregs[(ModRM & 0x38) >> 3]);				CLKM(1,1); }
OP( 0x8d, i_lea 	  ) { UINT16 ModRM = FETCH; (void)(*GetEA[ModRM])(); RegWord(ModRM)=EO; 	CLK(1); }
OP( 0x8e, i_mov_sregw ) { UINT16 src; GetModRM; src = GetRMWord(ModRM); CLKM(3,2);
	switch (ModRM & 0x38) {
		case 0x00: I.sregs[ES] = src; break; /* mov es,ew */
		case 0x08: I.sregs[CS] = src; break; /* mov cs,ew */
		case 0x10: I.sregs[SS] = src; break; /* mov ss,ew */
		case 0x18: I.sregs[DS] = src; break; /* mov ds,ew */
		default:  ;
	}
	no_interrupt=1;
}
OP( 0x8f, i_popw ) { UINT16 tmp; GetModRM; POP(tmp); PutRMWord(ModRM,tmp); CLKM(3,1); }
OP( 0x90, i_nop  ) { CLK(1);
	/* Cycle skip for idle loops (0: NOP  1:  JMP 0) */
	if (no_interrupt==0 && nec_ICount>0 && (PEEKOP((I.sregs[CS]<<4)+I.ip))==0xeb && (PEEK((I.sregs[CS]<<4)+I.ip+1))==0xfd)
		nec_ICount%=15;
}
OP( 0x91, i_xchg_axcx ) { XchgAWReg(CW); CLK(3); }
OP( 0x92, i_xchg_axdx ) { XchgAWReg(DW); CLK(3); }
OP( 0x93, i_xchg_axbx ) { XchgAWReg(BW); CLK(3); }
OP( 0x94, i_xchg_axsp ) { XchgAWReg(SP); CLK(3); }
OP( 0x95, i_xchg_axbp ) { XchgAWReg(BP); CLK(3); }
OP( 0x96, i_xchg_axsi ) { XchgAWReg(IX); CLK(3); }
OP( 0x97, i_xchg_axdi ) { XchgAWReg(IY); CLK(3); }

OP( 0x98, i_cbw 	  ) { I.regs.b[AH] = (I.regs.b[AL] & 0x80) ? 0xff : 0;	CLK(1);	}
OP( 0x99, i_cwd 	  ) { I.regs.w[DW] = (I.regs.b[AH] & 0x80) ? 0xffff : 0;	CLK(1);	}
OP( 0x9a, i_call_far  ) { UINT32 tmp, tmp2;	FETCHWORD(tmp); FETCHWORD(tmp2); PUSH(I.sregs[CS]); PUSH(I.ip); I.ip = (WORD)tmp; I.sregs[CS] = (WORD)tmp2; CLK(10); }
OP( 0x9b, i_wait	  ) { ; }
OP( 0x9c, i_pushf	  ) { PUSH( CompressFlags() ); CLK(2); }
OP( 0x9d, i_popf	  ) { UINT32 tmp; POP(tmp); ExpandFlags(tmp); CLK(3);}
OP( 0x9e, i_sahf	  ) { UINT32 tmp = (CompressFlags() & 0xff00) | (I.regs.b[AH] & 0xd5); ExpandFlags(tmp); CLK(4); }
OP( 0x9f, i_lahf	  ) { I.regs.b[AH] = CompressFlags() & 0xff; CLK(2); }

OP( 0xa0, i_mov_aldisp ) { UINT32 addr; FETCHWORD(addr); I.regs.b[AL] = GetMemB(DS, addr); CLK(1); }
OP( 0xa1, i_mov_axdisp ) { UINT32 addr; FETCHWORD(addr); I.regs.b[AL] = GetMemB(DS, addr); I.regs.b[AH] = GetMemB(DS, (addr+1)&0xffff); CLK(1); }
OP( 0xa2, i_mov_dispal ) { UINT32 addr; FETCHWORD(addr); PutMemB(DS, addr, I.regs.b[AL]);  CLK(1); }
OP( 0xa3, i_mov_dispax ) { UINT32 addr; FETCHWORD(addr); PutMemB(DS, addr, I.regs.b[AL]);  PutMemB(DS, (addr+1)&0xffff, I.regs.b[AH]); CLK(1); }
OP( 0xa4, i_movsb	   ) { UINT32 tmp = GetMemB(DS,I.regs.w[IX]); PutMemB(ES,I.regs.w[IY], tmp); I.regs.w[IY] += -2 * I.DF + 1; I.regs.w[IX] += -2 * I.DF + 1; CLK(5); }
OP( 0xa5, i_movsw	   ) { UINT32 tmp = GetMemW(DS,I.regs.w[IX]); PutMemW(ES,I.regs.w[IY], tmp); I.regs.w[IY] += -4 * I.DF + 2; I.regs.w[IX] += -4 * I.DF + 2; CLK(5); }
OP( 0xa6, i_cmpsb	   ) { UINT32 src = GetMemB(ES, I.regs.w[IY]); UINT32 dst = GetMemB(DS, I.regs.w[IX]); SUBB; I.regs.w[IY] += -2 * I.DF + 1; I.regs.w[IX] += -2 * I.DF + 1; CLK(6); }
OP( 0xa7, i_cmpsw	   ) { UINT32 src = GetMemW(ES, I.regs.w[IY]); UINT32 dst = GetMemW(DS, I.regs.w[IX]); SUBW; I.regs.w[IY] += -4 * I.DF + 2; I.regs.w[IX] += -4 * I.DF + 2; CLK(6); }

OP( 0xa8, i_test_ald8  ) { DEF_ald8;  ANDB; CLK(1); }
OP( 0xa9, i_test_axd16 ) { DEF_axd16; ANDW; CLK(1); }
OP( 0xaa, i_stosb	   ) { PutMemB(ES,I.regs.w[IY],I.regs.b[AL]); 	I.regs.w[IY] += -2 * I.DF + 1; CLK(3);	}
OP( 0xab, i_stosw	   ) { PutMemW(ES,I.regs.w[IY],I.regs.w[AW]); 	I.regs.w[IY] += -4 * I.DF + 2; CLK(3);	}
OP( 0xac, i_lodsb	   ) { I.regs.b[AL] = GetMemB(DS,I.regs.w[IX]); I.regs.w[IX] += -2 * I.DF + 1; CLK(3);	}
OP( 0xad, i_lodsw	   ) { I.regs.w[AW] = GetMemW(DS,I.regs.w[IX]); I.regs.w[IX] += -4 * I.DF + 2; CLK(3); }
OP( 0xae, i_scasb	   ) { UINT32 src = GetMemB(ES, I.regs.w[IY]); 	UINT32 dst = I.regs.b[AL]; SUBB; I.regs.w[IY] += -2 * I.DF + 1; CLK(4);  }
OP( 0xaf, i_scasw	   ) { UINT32 src = GetMemW(ES, I.regs.w[IY]); 	UINT32 dst = I.regs.w[AW]; SUBW; I.regs.w[IY] += -4 * I.DF + 2; CLK(4); }

OP( 0xb0, i_mov_ald8  ) { I.regs.b[AL] = FETCH;	CLK(1); }
OP( 0xb1, i_mov_cld8  ) { I.regs.b[CL] = FETCH; CLK(1); }
OP( 0xb2, i_mov_dld8  ) { I.regs.b[DL] = FETCH; CLK(1); }
OP( 0xb3, i_mov_bld8  ) { I.regs.b[BL] = FETCH; CLK(1); }
OP( 0xb4, i_mov_ahd8  ) { I.regs.b[AH] = FETCH; CLK(1); }
OP( 0xb5, i_mov_chd8  ) { I.regs.b[CH] = FETCH; CLK(1); }
OP( 0xb6, i_mov_dhd8  ) { I.regs.b[DH] = FETCH; CLK(1); }
OP( 0xb7, i_mov_bhd8  ) { I.regs.b[BH] = FETCH;	CLK(1); }

OP( 0xb8, i_mov_axd16 ) { I.regs.b[AL] = FETCH;	 I.regs.b[AH] = FETCH;	CLK(1); }
OP( 0xb9, i_mov_cxd16 ) { I.regs.b[CL] = FETCH;	 I.regs.b[CH] = FETCH;	CLK(1); }
OP( 0xba, i_mov_dxd16 ) { I.regs.b[DL] = FETCH;	 I.regs.b[DH] = FETCH;	CLK(1); }
OP( 0xbb, i_mov_bxd16 ) { I.regs.b[BL] = FETCH;	 I.regs.b[BH] = FETCH;	CLK(1); }
OP( 0xbc, i_mov_spd16 ) { I.regs.b[SPL] = FETCH; I.regs.b[SPH] = FETCH;	CLK(1); }
OP( 0xbd, i_mov_bpd16 ) { I.regs.b[BPL] = FETCH; I.regs.b[BPH] = FETCH; CLK(1); }
OP( 0xbe, i_mov_sid16 ) { I.regs.b[IXL] = FETCH; I.regs.b[IXH] = FETCH;	CLK(1); }
OP( 0xbf, i_mov_did16 ) { I.regs.b[IYL] = FETCH; I.regs.b[IYH] = FETCH;	CLK(1); }

OP( 0xc0, i_rotshft_bd8 ) {
	UINT32 src, dst; UINT8 c;
	GetModRM; src = (unsigned)GetRMByte(ModRM); dst=src;
	c=FETCH;
	c&=0x1f;
	CLKM(5,3);
	if (c) switch (ModRM & 0x38) {
		case 0x00: do { ROL_BYTE;  c--; } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x08: do { ROR_BYTE;  c--; } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x10: do { ROLC_BYTE; c--; } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x18: do { RORC_BYTE; c--; } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x20: SHL_BYTE(c);	I.AuxVal = 1; break;//
		case 0x28: SHR_BYTE(c);	I.AuxVal = 1; break;//
		case 0x30:	break;
		case 0x38: SHRA_BYTE(c); break;
	}
}

OP( 0xc1, i_rotshft_wd8 ) {
	UINT32 src, dst;  UINT8 c;
	GetModRM; src = (unsigned)GetRMWord(ModRM); dst=src;
	c=FETCH;
	c&=0x1f;
	CLKM(5,3);
	if (c) switch (ModRM & 0x38) {
		case 0x00: do { ROL_WORD;  c--; } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x08: do { ROR_WORD;  c--; } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x10: do { ROLC_WORD; c--; } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x18: do { RORC_WORD; c--; } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x20: SHL_WORD(c);	I.AuxVal = 1; break;
		case 0x28: SHR_WORD(c);	I.AuxVal = 1; break;
		case 0x30:	break;
		case 0x38: SHRA_WORD(c); break;
	}
}

OP( 0xc2, i_ret_d16  ) { UINT32 count = FETCH; count += FETCH << 8; POP(I.ip); I.regs.w[SP]+=count; CLK(6); }
OP( 0xc3, i_ret 	 ) { POP(I.ip); CLK(6); }
OP( 0xc4, i_les_dw	 ) { GetModRM; WORD tmp = GetRMWord(ModRM); RegWord(ModRM)=tmp; I.sregs[ES] = GetnextRMWord; CLK(6); }
OP( 0xc5, i_lds_dw	 ) { GetModRM; WORD tmp = GetRMWord(ModRM); RegWord(ModRM)=tmp; I.sregs[DS] = GetnextRMWord; CLK(6); }
OP( 0xc6, i_mov_bd8  ) { GetModRM; PutImmRMByte(ModRM); CLK(1); }
OP( 0xc7, i_mov_wd16 ) { GetModRM; PutImmRMWord(ModRM); CLK(1); }

OP( 0xc8, i_enter ) {
	UINT32 nb = FETCH;
	UINT32 i,level;

	CLK(19);
	nb += FETCH << 8;
	level = FETCH;
	PUSH(I.regs.w[BP]);
	I.regs.w[BP]=I.regs.w[SP];
	I.regs.w[SP] -= nb;
	for (i=1;i<level;i++) {
	PUSH(GetMemW(SS,I.regs.w[BP]-i*2));
	CLK(4);
	}
	if (level) PUSH(I.regs.w[BP]);
}
OP( 0xc9, i_leave ) {
	I.regs.w[SP]=I.regs.w[BP];
	POP(I.regs.w[BP]);
	CLK(2);
}
OP( 0xca, i_retf_d16  ) { UINT32 count = FETCH; count += FETCH << 8; POP(I.ip); POP(I.sregs[CS]); I.regs.w[SP]+=count; CLK(9); }
OP( 0xcb, i_retf	  ) { POP(I.ip); POP(I.sregs[CS]); CLK(8); }
OP( 0xcc, i_int3	  ) { nec_interrupt(3,0); CLK(9); }
OP( 0xcd, i_int 	  ) { nec_interrupt(FETCH,0); CLK(10); }
OP( 0xce, i_into	  ) { if (OF) { nec_interrupt(4,0); CLK(13); } else CLK(6); }
OP( 0xcf, i_iret	  ) { POP(I.ip); POP(I.sregs[CS]); i_popf(); CLK(10); }

OP( 0xd0, i_rotshft_b ) {
	UINT32 src, dst; GetModRM; src = (UINT32)GetRMByte(ModRM); dst=src;
	CLKM(3,1);
	switch (ModRM & 0x38) {
		case 0x00: ROL_BYTE;  PutbackRMByte(ModRM,(BYTE)dst); I.OverVal = (src^dst)&0x80; break;
		case 0x08: ROR_BYTE;  PutbackRMByte(ModRM,(BYTE)dst); I.OverVal = (src^dst)&0x80; break;
		case 0x10: ROLC_BYTE; PutbackRMByte(ModRM,(BYTE)dst); I.OverVal = (src^dst)&0x80; break;
		case 0x18: RORC_BYTE; PutbackRMByte(ModRM,(BYTE)dst); I.OverVal = (src^dst)&0x80; break;
		case 0x20: SHL_BYTE(1); I.OverVal = (src^dst)&0x80;I.AuxVal = 1; break;
		case 0x28: SHR_BYTE(1); I.OverVal = (src^dst)&0x80;I.AuxVal = 1; break;
		case 0x30:	break;
		case 0x38: SHRA_BYTE(1); I.OverVal = 0; break;
	}
}

OP( 0xd1, i_rotshft_w ) {
	UINT32 src, dst; GetModRM; src = (UINT32)GetRMWord(ModRM); dst=src;
	CLKM(3,1);
	switch (ModRM & 0x38) {
		case 0x00: ROL_WORD;  PutbackRMWord(ModRM,(WORD)dst); I.OverVal = (src^dst)&0x8000; break;
		case 0x08: ROR_WORD;  PutbackRMWord(ModRM,(WORD)dst); I.OverVal = (src^dst)&0x8000; break;
		case 0x10: ROLC_WORD; PutbackRMWord(ModRM,(WORD)dst); I.OverVal = (src^dst)&0x8000; break;
		case 0x18: RORC_WORD; PutbackRMWord(ModRM,(WORD)dst); I.OverVal = (src^dst)&0x8000; break;
		case 0x20: SHL_WORD(1); I.AuxVal = 1;I.OverVal = (src^dst)&0x8000;	break;
		case 0x28: SHR_WORD(1); I.AuxVal = 1;I.OverVal = (src^dst)&0x8000;	break;
		case 0x30: break;
		case 0x38: SHRA_WORD(1); I.AuxVal = 1;I.OverVal = 0; break;
	}
}

OP( 0xd2, i_rotshft_bcl ) {
	UINT32 src, dst; UINT8 c; GetModRM; src = (UINT32)GetRMByte(ModRM); dst=src;
	c=I.regs.b[CL];
	CLKM(5,3);
	c&=0x1f;
	if (c) switch (ModRM & 0x38) {
		case 0x00: do { ROL_BYTE;  c--; CLK(1); } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x08: do { ROR_BYTE;  c--; CLK(1); } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x10: do { ROLC_BYTE; c--; CLK(1); } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x18: do { RORC_BYTE; c--; CLK(1); } while (c>0); PutbackRMByte(ModRM,(BYTE)dst); break;
		case 0x20: SHL_BYTE(c);	I.AuxVal = 1; break;
		case 0x28: SHR_BYTE(c); I.AuxVal = 1;break;
		case 0x30: break;
		case 0x38: SHRA_BYTE(c); break;
	}
}

OP( 0xd3, i_rotshft_wcl ) {
	UINT32 src, dst; UINT8 c; GetModRM; src = (UINT32)GetRMWord(ModRM); dst=src;
	c=I.regs.b[CL];
	c&=0x1f;
	CLKM(5,3);
	if (c) switch (ModRM & 0x38) {
		case 0x00: do { ROL_WORD;  c--; CLK(1); } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x08: do { ROR_WORD;  c--; CLK(1); } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x10: do { ROLC_WORD; c--; CLK(1); } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x18: do { RORC_WORD; c--; CLK(1); } while (c>0); PutbackRMWord(ModRM,(WORD)dst); break;
		case 0x20: SHL_WORD(c);	I.AuxVal = 1; break;
		case 0x28: SHR_WORD(c);	I.AuxVal = 1; break;
		case 0x30: break;
		case 0x38: SHRA_WORD(c); break;
	}
}

OP( 0xd4, i_aam    ) { UINT32 mult=FETCH; mult=0; I.regs.b[AH] = I.regs.b[AL] / 10; I.regs.b[AL] %= 10; SetSZPF_Word(I.regs.w[AW]); CLK(17); }
OP( 0xd5, i_aad    ) { UINT32 mult=FETCH; mult=0; I.regs.b[AL] = I.regs.b[AH] * 10 + I.regs.b[AL]; I.regs.b[AH] = 0; SetSZPF_Byte(I.regs.b[AL]); CLK(6); }
OP( 0xd6, i_setalc ) { I.regs.b[AL] = (CF)?0xff:0x00; CLK(3);  } /* nop at V30MZ? */
OP( 0xd7, i_trans  ) { UINT32 dest = (I.regs.w[BW]+I.regs.b[AL])&0xffff; I.regs.b[AL] = GetMemB(DS, dest); CLK(5); }
OP( 0xd8, i_fpo    ) { GetModRM; CLK(3);	 } /* nop at V30MZ? */

OP( 0xe0, i_loopne ) { INT8 disp = (INT8)FETCH; I.regs.w[CW]--; if (!ZF && I.regs.w[CW]) { I.ip = (WORD)(I.ip+disp);  CLK(6); } else CLK(3); }
OP( 0xe1, i_loope  ) { INT8 disp = (INT8)FETCH; I.regs.w[CW]--; if ( ZF && I.regs.w[CW]) { I.ip = (WORD)(I.ip+disp);  CLK(6); } else CLK(3); }
OP( 0xe2, i_loop   ) { INT8 disp = (INT8)FETCH; I.regs.w[CW]--; if (I.regs.w[CW]) { I.ip = (WORD)(I.ip+disp);  CLK(5); } else CLK(2); }
OP( 0xe3, i_jcxz   ) { INT8 disp = (INT8)FETCH; if (I.regs.w[CW] == 0) { I.ip = (WORD)(I.ip+disp);	CLK(4); } else CLK(1); }
OP( 0xe4, i_inal   ) { UINT8 port = FETCH; I.regs.b[AL] = read_port(port); CLK(6);	}
OP( 0xe5, i_inax   ) { UINT8 port = FETCH; I.regs.b[AL] = read_port(port); I.regs.b[AH] = read_port(port+1); CLK(6); }
OP( 0xe6, i_outal  ) { UINT8 port = FETCH; write_port(port, I.regs.b[AL]); CLK(6);	}
OP( 0xe7, i_outax  ) { UINT8 port = FETCH; write_port(port, I.regs.b[AL]); write_port(port+1, I.regs.b[AH]); CLK(6);	}

OP( 0xe8, i_call_d16 ) { UINT32 tmp; FETCHWORD(tmp); PUSH(I.ip); I.ip = (WORD)(I.ip+(INT16)tmp); CLK(5); }
OP( 0xe9, i_jmp_d16  ) { UINT32 tmp; FETCHWORD(tmp); I.ip = (WORD)(I.ip+(INT16)tmp); CLK(4); }
OP( 0xea, i_jmp_far  ) { UINT32 tmp,tmp1; FETCHWORD(tmp); FETCHWORD(tmp1); I.sregs[CS] = (WORD)tmp1; 	I.ip = (WORD)tmp; CLK(7);	}
OP( 0xeb, i_jmp_d8	 ) { int tmp = (int)((INT8)FETCH); CLK(4);
	if (tmp==-2 && no_interrupt==0 && nec_ICount>0) nec_ICount%=12; /* cycle skip */
	I.ip = (WORD)(I.ip+tmp);
}
OP( 0xec, i_inaldx	 ) { I.regs.b[AL] = read_port(I.regs.w[DW]); CLK(6);}
OP( 0xed, i_inaxdx	 ) { UINT32 port = I.regs.w[DW];	I.regs.b[AL] = read_port(port);	I.regs.b[AH] = read_port(port+1); CLK(6); }
OP( 0xee, i_outdxal  ) { write_port(I.regs.w[DW], I.regs.b[AL]); CLK(6);	}
OP( 0xef, i_outdxax  ) { UINT32 port = I.regs.w[DW];	write_port(port, I.regs.b[AL]);	write_port(port+1, I.regs.b[AH]); CLK(6); }

OP( 0xf0, i_lock	 ) {  no_interrupt=1; CLK(1); }
#define THROUGH 				\
	if(nec_ICount<0){			\
		if(seg_prefix)			\
			I.ip-=(UINT16)3;	\
		else					\
			I.ip-=(UINT16)2;	\
		break;}

OP( 0xf2, i_repne	 ) { UINT32 next = FETCHOP; UINT16 c = I.regs.w[CW];
	switch(next) { /* Segments */
		case 0x26:	seg_prefix=TRUE;	prefix_base=I.sregs[ES]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x2e:	seg_prefix=TRUE;	prefix_base=I.sregs[CS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x36:	seg_prefix=TRUE;	prefix_base=I.sregs[SS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x3e:	seg_prefix=TRUE;	prefix_base=I.sregs[DS]<<4;	next = FETCHOP;	CLK(2); break;
	}

	switch(next) {
		case 0x6c:	CLK(2); if (c) do { i_insb();  c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0x6d:	CLK(2); if (c) do { i_insw();  c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0x6e:	CLK(2); if (c) do { i_outsb(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0x6f:	CLK(2); if (c) do { i_outsw(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xa4:	CLK(2); if (c) do { i_movsb(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xa5:	CLK(2); if (c) do { i_movsw(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xa6:	CLK(5); if (c) do { THROUGH; i_cmpsb(); c--; CLK(3); } while (c>0 && ZF==0);	I.regs.w[CW]=c; break;
		case 0xa7:	CLK(5); if (c) do { THROUGH; i_cmpsw(); c--; CLK(3); } while (c>0 && ZF==0);	I.regs.w[CW]=c; break;
		case 0xaa:	CLK(2); if (c) do { i_stosb(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xab:	CLK(2); if (c) do { i_stosw(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xac:	CLK(2); if (c) do { i_lodsb(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xad:	CLK(2); if (c) do { i_lodsw(); c--; } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xae:	CLK(5); if (c) do { THROUGH; i_scasb(); c--; CLK(5); } while (c>0 && ZF==0);	I.regs.w[CW]=c; break;
		case 0xaf:	CLK(5); if (c) do { THROUGH; i_scasw(); c--; CLK(5); } while (c>0 && ZF==0);	I.regs.w[CW]=c; break;
		default:		nec_instruction[next]();
	}
	seg_prefix=FALSE;
}
OP( 0xf3, i_repe	 ) { UINT32 next = FETCHOP; UINT16 c = I.regs.w[CW];
	switch(next) { /* Segments */
		case 0x26:	seg_prefix=TRUE;	prefix_base=I.sregs[ES]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x2e:	seg_prefix=TRUE;	prefix_base=I.sregs[CS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x36:	seg_prefix=TRUE;	prefix_base=I.sregs[SS]<<4;	next = FETCHOP;	CLK(2); break;
		case 0x3e:	seg_prefix=TRUE;	prefix_base=I.sregs[DS]<<4;	next = FETCHOP;	CLK(2); break;
	}

	switch(next) {
		case 0x6c:	CLK(5); if (c) do { THROUGH; i_insb();	c--; CLK( 0); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0x6d:	CLK(5); if (c) do { THROUGH; i_insw();	c--; CLK( 0); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0x6e:	CLK(5); if (c) do { THROUGH; i_outsb(); c--; CLK(-1); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0x6f:	CLK(5); if (c) do { THROUGH; i_outsw(); c--; CLK(-1); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xa4:	CLK(5); if (c) do { THROUGH; i_movsb(); c--; CLK( 2); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xa5:	CLK(5); if (c) do { THROUGH; i_movsw(); c--; CLK( 2); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xa6:	CLK(5); if (c) do { THROUGH; i_cmpsb(); c--; CLK( 4); } while (c>0 && ZF==1);	I.regs.w[CW]=c; break;
		case 0xa7:	CLK(5); if (c) do { THROUGH; i_cmpsw(); c--; CLK( 4); } while (c>0 && ZF==1);	I.regs.w[CW]=c; break;
		case 0xaa:	CLK(5); if (c) do { THROUGH; i_stosb(); c--; CLK( 3); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xab:	CLK(5); if (c) do { THROUGH; i_stosw(); c--; CLK( 3); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xac:	CLK(5); if (c) do { THROUGH; i_lodsb(); c--; CLK( 3); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xad:	CLK(5); if (c) do { THROUGH; i_lodsw(); c--; CLK( 3); } while (c>0);	I.regs.w[CW]=c;	break;
		case 0xae:	CLK(5); if (c) do { THROUGH; i_scasb(); c--; CLK( 4); } while (c>0 && ZF==1);	I.regs.w[CW]=c; break;
		case 0xaf:	CLK(5); if (c) do { THROUGH; i_scasw(); c--; CLK( 4); } while (c>0 && ZF==1);	I.regs.w[CW]=c; break;
		default:	 nec_instruction[next]();
	}
	seg_prefix=FALSE;
}
OP( 0xf4, i_hlt ) { nec_ICount=0; }





OP( 0xf5, i_cmc ) { I.CarryVal = !CF; CLK(4); }
OP( 0xf6, i_f6pre ) { UINT32 tmp; UINT32 uresult,uresult2; INT32 result,result2;
	GetModRM; tmp = GetRMByte(ModRM);
	switch (ModRM & 0x38) {
		case 0x00: tmp &= FETCH; I.CarryVal = I.OverVal = I.AuxVal=0; SetSZPF_Byte(tmp); CLKM(2,1); break; /* TEST */
		case 0x08:	break;
 		case 0x10: PutbackRMByte(ModRM,~tmp); CLKM(3,1); break; /* NOT */
		
		case 0x18: I.CarryVal=(tmp!=0);tmp=(~tmp)+1; SetSZPF_Byte(tmp); PutbackRMByte(ModRM,tmp&0xff); CLKM(3,1); break; /* NEG */
		case 0x20: uresult = I.regs.b[AL]*tmp; I.regs.w[AW]=(WORD)uresult; I.CarryVal=I.OverVal=(I.regs.b[AH]!=0); CLKM(4,3); break; /* MULU */
		case 0x28: result = (INT16)((INT8)I.regs.b[AL])*(INT16)((INT8)tmp); I.regs.w[AW]=(WORD)result; I.CarryVal=I.OverVal=(I.regs.b[AH]!=0); CLKM(4,3); break; /* MUL */
		case 0x30: if (tmp) { DIVUB; } else nec_interrupt(0,0); CLKM(16,15); break;
		case 0x38: if (tmp) { DIVB;  } else nec_interrupt(0,0); CLKM(18,17); break;
   }
}

OP( 0xf7, i_f7pre	) { UINT32 tmp,tmp2; UINT32 uresult,uresult2; INT32 result,result2;
	GetModRM; tmp = GetRMWord(ModRM);
	switch (ModRM & 0x38) {
		case 0x00: FETCHWORD(tmp2); tmp &= tmp2; I.CarryVal = I.OverVal = I.AuxVal=0; SetSZPF_Word(tmp); CLKM(2,1); break; /* TEST */
		case 0x08: break;
 		case 0x10: PutbackRMWord(ModRM,~tmp); CLKM(3,1); break; /* NOT */
		case 0x18: I.CarryVal=(tmp!=0); tmp=(~tmp)+1; SetSZPF_Word(tmp); PutbackRMWord(ModRM,tmp&0xffff); CLKM(3,1); break; /* NEG */
		case 0x20: uresult = I.regs.w[AW]*tmp; I.regs.w[AW]=uresult&0xffff; I.regs.w[DW]=((UINT32)uresult)>>16; I.CarryVal=I.OverVal=(I.regs.w[DW]!=0); CLKM(4,3); break; /* MULU */
		case 0x28: result = (INT32)((INT16)I.regs.w[AW])*(INT32)((INT16)tmp); I.regs.w[AW]=result&0xffff; I.regs.w[DW]=result>>16; I.CarryVal=I.OverVal=(I.regs.w[DW]!=0); CLKM(4,3); break; /* MUL */
		case 0x30: if (tmp) { DIVUW; } else nec_interrupt(0,0); CLKM(24,23); break;
		case 0x38: if (tmp) { DIVW;  } else nec_interrupt(0,0); CLKM(25,24); break;
 	}
}

OP( 0xf8, i_clc   ) { I.CarryVal = 0;	CLK(4);	}
OP( 0xf9, i_stc   ) { I.CarryVal = 1;	CLK(4);	}
OP( 0xfa, i_di	  ) { SetIF(0);			CLK(4);	}
OP( 0xfb, i_ei	  ) { SetIF(1);			CLK(4);	}
OP( 0xfc, i_cld   ) { SetDF(0);			CLK(4);	}
OP( 0xfd, i_std   ) { SetDF(1);			CLK(4);	}
OP( 0xfe, i_fepre ) { UINT32 tmp, tmp1; GetModRM; tmp=GetRMByte(ModRM);
	switch(ModRM & 0x38) {
		case 0x00: tmp1 = tmp+1; I.OverVal = (tmp==0x7f); SetAF(tmp1,tmp,1); SetSZPF_Byte(tmp1); PutbackRMByte(ModRM,(BYTE)tmp1); CLKM(3,1); break; /* INC */
		case 0x08: tmp1 = tmp-1; I.OverVal = (tmp==0x80); SetAF(tmp1,tmp,1); SetSZPF_Byte(tmp1); PutbackRMByte(ModRM,(BYTE)tmp1); CLKM(3,1); break; /* DEC */
	}
}
OP( 0xff, i_ffpre ) { UINT32 tmp, tmp1; GetModRM; tmp=GetRMWord(ModRM);
	switch(ModRM & 0x38) {
		case 0x00: tmp1 = tmp+1; I.OverVal = (tmp==0x7fff); SetAF(tmp1,tmp,1); SetSZPF_Word(tmp1); PutbackRMWord(ModRM,(WORD)tmp1); CLKM(3,1); break; /* INC */
		case 0x08: tmp1 = tmp-1; I.OverVal = (tmp==0x8000); SetAF(tmp1,tmp,1); SetSZPF_Word(tmp1); PutbackRMWord(ModRM,(WORD)tmp1); CLKM(3,1); break; /* DEC */
		case 0x10: PUSH(I.ip);	I.ip = (WORD)tmp; CLKM(6,5); break; /* CALL */
		case 0x18: tmp1 = I.sregs[CS]; I.sregs[CS] = GetnextRMWord; PUSH(tmp1); PUSH(I.ip); I.ip = tmp; CLKM(12,1); break; /* CALL FAR */
		case 0x20: I.ip = tmp;	CLKM(5,4); break; /* JMP */
		case 0x28: I.ip = tmp; I.sregs[CS] = GetnextRMWord; CLKM(10,1); break; /* JMP FAR */
		case 0x30: PUSH(tmp); CLKM(2,1); break;
		default:  ;
	}
}

static void i_invalid(void)
{
	CLK(10);
}

/*****************************************************************************/


unsigned nec_get_reg(int regnum)
{
	switch( regnum )
	{
		case NEC_IP: return I.ip;
		case NEC_SP: return I.regs.w[SP];
		case NEC_FLAGS: return CompressFlags();
		case NEC_AW: return I.regs.w[AW];
		case NEC_CW: return I.regs.w[CW];
		case NEC_DW: return I.regs.w[DW];
		case NEC_BW: return I.regs.w[BW];
		case NEC_BP: return I.regs.w[BP];
		case NEC_IX: return I.regs.w[IX];
		case NEC_IY: return I.regs.w[IY];
		case NEC_ES: return I.sregs[ES];
		case NEC_CS: return I.sregs[CS];
		case NEC_SS: return I.sregs[SS];
		case NEC_DS: return I.sregs[DS];
		case NEC_VECTOR: return I.int_vector;
		case NEC_PENDING: return I.pending_irq;
		case NEC_NMI_STATE: return I.nmi_state;
		case NEC_IRQ_STATE: return I.irq_state;
	}
	return 0;
}

void nec_set_irq_line(int irqline, int state);

void nec_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case NEC_IP: I.ip = val; break;
		case NEC_SP: I.regs.w[SP] = val; break;
		case NEC_FLAGS: ExpandFlags(val); break;
		case NEC_AW: I.regs.w[AW] = val; break;
		case NEC_CW: I.regs.w[CW] = val; break;
		case NEC_DW: I.regs.w[DW] = val; break;
		case NEC_BW: I.regs.w[BW] = val; break;
		case NEC_BP: I.regs.w[BP] = val; break;
		case NEC_IX: I.regs.w[IX] = val; break;
		case NEC_IY: I.regs.w[IY] = val; break;
		case NEC_ES: I.sregs[ES] = val; break;
		case NEC_CS: I.sregs[CS] = val; break;
		case NEC_SS: I.sregs[SS] = val; break;
		case NEC_DS: I.sregs[DS] = val; break;
		case NEC_VECTOR: I.int_vector = val; break;
	}
}


int nec_execute(int cycles)
{
	

	nec_ICount=cycles;
//	cpu_type=V30;

	while(nec_ICount>=0) {

		nec_instruction[FETCHOP]();
//		nec_ICount++;
	}

	return cycles - nec_ICount;
}

}
