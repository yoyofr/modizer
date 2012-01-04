// ---------------------------------------------------------------------------
//	FM Sound Generator
//	Copyright (C) cisc 2000, 2001.
// ---------------------------------------------------------------------------
//	x86/VC engine
// ---------------------------------------------------------------------------
//	$Id: fmgenx86.h,v 1.1 2001/04/23 22:25:34 kaoru-k Exp $

// ---------------------------------------------------------------------------

static void __stdcall FM_NextPhase(Operator* op)
{
	op->ShiftPhase2();
}

extern Operator* popr;
extern Channel4* pch4;

#undef THIS
#define THIS	[ebp]this
#define OP(n)	[ebp]this.op[(n) * TYPE Operator]
#define OPR(r)	[r]popr
#define BUF(n)	[ebp]this.buf[(n) * TYPE int]

// ------------------------------------------------- Envelope Generator

#if (FM_SINEPRESIS == 0)
	#define EGCALC_SP		__asm { add eax,eax }
#elif (FM_SINEPRESIS == 1)
	#define EGCALC_SP		__asm { lea eax,[eax*4] }
#elif (FM_SINEPRESIS == 2)
	#define EGCALC_SP		__asm { lea eax,[eax*8] }
#else
	#error FM_SINEPRESIS is out of range.
#endif

#define EGCALC(r, b) \
__asm {	add	OPR(r).egstep,b << (11 + FM_EGBITS) } \
__asm {	cmp	OPR(r).phase,1			} \
__asm {	jz	opc##r##_a				} \
__asm {		mov	eax,OPR(r).eglevel	} \
__asm {		add	eax,OPR(r).egtransd	} \
__asm {		cmp	eax,OPR(r).eglvnext	} \
__asm {		mov	OPR(r).eglevel,eax	} \
__asm {		jb	opc##r##_n			} \
__asm {		jmp	opc##r##_np			} \
opc##r##_a: \
__asm {		mov	eax,OPR(r).eglevel	} \
__asm {		mov	ecx,OPR(r).egtransa	} \
__asm {		mov	ebx,eax				} \
__asm {		dec	eax					} \
__asm {		shr	ebx,cl				} \
__asm {		sub	eax,ebx				} \
__asm {		mov	OPR(r).eglevel,eax	} \
__asm {		jg	opc##r##_n			} \
opc##r##_np: \
__asm {		push r					} \
__asm {		call FM_NextPhase		} \
__asm {	mov	eax,OPR(r).eglevel		} \
opc##r##_n: \
__asm {	add	eax,OPR(r).tlout		} \
__asm {	cmp	eax,FM_CLENTS/2-0x200	} \
__asm {	jb	opc##r##_n1				} \
__asm {	mov	eax,FM_CLENTS/2-0x201	} \
opc##r##_n1: \
EGCALC_SP \
__asm {	mov	OPR(r).egout,eax		} \
__asm {	ret							}

static void __declspec(naked) FM_EGCS()
{
	EGCALC(esi, 3);
}

static void __declspec(naked) FM_EGCD()
{
	EGCALC(edi, 3);
}

#if 0					// opna.cpp の FM_USE_CALC_2E に対応
static void __declspec(naked) FM_EGCS1()
{
	EGCALC(esi, 1);
}

static void __declspec(naked) FM_EGCD1()
{
	EGCALC(edi, 1);
}
#else
	#define FM_EGCS1 FM_EGCS
	#define FM_EGCD1 FM_EGCD
#endif

// ------------------------------------------------Operator without LFO

#define OPCALC(n, r)	\
__asm { mov	eax,OPR(r).egstepd } \
__asm { sub	OPR(r).egstep,eax } \
__asm { ja	opc##n } \
__asm { call FM_EGCS } \
opc##n: \
__asm {	mov	eax,OPR(r).pgcount } \
__asm {	mov	ebx,THIS.in[(n-1)*4] } \
__asm {	mov	ebx,[ebx] } \
__asm {	sal	ebx,2+IS2EC_SHIFT } \
__asm {	add	ebx,eax } \
__asm {	add	eax,OPR(r).pgdcount } \
__asm {	shr	ebx,20+FM_PGBITS-FM_OPSINBITS-2 } \
__asm {	mov	OPR(r).pgcount,eax } \
__asm {	and	ebx,(FM_OPSINENTS-1)*4 } \
__asm {	mov	eax,sinetable[ebx] } \
__asm { mov ecx,THIS.out[(n-1)*4] } \
__asm {	add	eax,OPR(r).egout } \
__asm {	mov	ebx,cltable[eax*4] }

ISample __declspec(naked) Channel4::Calc()
{
	__asm
	{
		push ebp
		push ebx
		push esi
		push edi
		
		xor		eax,eax
		mov		ebp,ecx						// = this
		mov		BUF(1),eax
		mov		BUF(2),eax
		mov		BUF(3),eax

		lea		esi,OP(0)

		mov		eax,OPR(esi).egstepd
		sub		OPR(esi).egstep,eax
		ja		opc0
		call	FM_EGCS
	opc0:
		mov		eax,OPR(esi).out
		mov		ebx,OPR(esi).out2
		mov		OPR(esi).out2,eax
		mov		ecx,THIS.fb
		mov		BUF(0),eax
		add		ebx,eax
		sal		ebx,1+IS2EC_SHIFT
		mov		eax,OPR(esi).pgcount
		sar		ebx,cl
		add		ebx,eax
		add		eax,OPR(esi).pgdcount
		mov		OPR(esi).pgcount,eax
		shr		ebx,20+FM_PGBITS-FM_OPSINBITS-2
		and		ebx,(FM_OPSINENTS-1)*4
		mov		eax,sinetable[ebx]
		add		eax,OPR(esi).egout
		mov		ebx,cltable[eax*4]
		mov		OPR(esi).out,ebx
	}

//op2:
	__asm {	add esi,TYPE Operator }
	OPCALC(1, esi)
	__asm {	add	[ecx],ebx }

	__asm {	add esi,TYPE Operator }
	OPCALC(2, esi)
	__asm {	add	[ecx],ebx }
	
	__asm {	add esi,TYPE Operator }
	OPCALC(3, esi)
	__asm
	{
		mov eax,[ecx]
		add eax,OPR(esi).out
		pop	edi
		mov OPR(esi).out,ebx
		pop	esi
		pop	ebx
		mov ecx,ebp
		pop	ebp
		ret 0
	}
}

#undef OPCALC

// ---------------------------------------------------Operator with LFO

#define OPCALC(n, r)	\
__asm { mov	eax,OPR(r).egstepd		} \
__asm { sub	OPR(r).egstep,eax		} \
__asm { ja	opc##n					} \
__asm { push edx					} \
__asm { call FM_EGCS				} \
__asm { pop edx						} \
opc##n: \
__asm { mov	eax,OPR(r).pgdcountl	} \
__asm { imul eax,edx				} \
__asm { sar	eax,5					} \
__asm { mov	ebx,THIS.in[(n-1)*4]	} \
__asm { add	eax,OPR(r).pgdcount		} \
__asm { mov	ebx,[ebx]				} \
__asm { add	eax,OPR(r).pgcount		} \
__asm { sal	ebx,2+IS2EC_SHIFT		} \
__asm { mov	OP(n).pgcount,eax		} \
__asm { add	ebx,eax					} \
__asm { mov	ecx,THIS.out[(n-1)*4]	} \
__asm { shr	ebx,20+FM_PGBITS-FM_OPSINBITS } \
__asm { and	ebx,FM_OPSINENTS-1		} \
__asm { mov	eax,sinetable[ebx*4]	} \
__asm { add	eax,OPR(r).egout		} \
__asm { mov	ebx,OPR(r).ams			} \
__asm { add	eax,[ebx+edi*4]			} \
__asm { mov	ebx,cltable[eax*4]		}

ISample __declspec(naked) Channel4::CalcL()
{
	__asm
	{
		push ebp
		push ebx
		push esi
		push edi
		
		xor		eax,eax
		mov		ebp,ecx						// = this
		mov		BUF(1),eax
		mov		BUF(2),eax
		mov		BUF(3),eax

		mov		eax,pml
		mov		ebx,THIS.pms

		lea		esi,OP(0)
		mov		edi,aml
		mov		edx,[ebx+eax*4]

		mov		eax,OPR(esi).egstepd
		sub		OPR(esi).egstep,eax
		ja		opc0

		push	edx
		call	FM_EGCS
		pop		edx
	opc0:
		mov		eax,OPR(esi).out
		mov		ebx,OPR(esi).out2
		mov		OPR(esi).out2,eax
		mov		ecx,THIS.fb
		add		ebx,eax
		sal		ebx,1+IS2EC_SHIFT
		mov		BUF(0),eax
		
		mov		eax,OPR(esi).pgdcountl
		sar		ebx,cl
		imul	eax,edx
		sar		eax,5
		add		eax,OPR(esi).pgcount
		add		eax,OPR(esi).pgdcount
		add		ebx,eax
		mov		OPR(esi).pgcount,eax
		
		shr		ebx,20+FM_PGBITS-FM_OPSINBITS-2
		and		ebx,(FM_OPSINENTS-1)*4
		mov		eax,sinetable[ebx]
		add		eax,OPR(esi).egout
		
		mov		ebx,OPR(esi).ams
		add		eax,[ebx+edi*4]
		
		mov		ebx,cltable[eax*4]
		mov		OPR(esi).out,ebx
	}

//op2:
	__asm {	add esi,TYPE Operator }
	OPCALC(1, esi)
	__asm {	add	[ecx],ebx }

	__asm {	add esi,TYPE Operator }
	OPCALC(2, esi)
	__asm {	add	[ecx],ebx }
	
	__asm {	add esi,TYPE Operator }
	OPCALC(3, esi)
	__asm
	{
		mov eax,[ecx]
		add eax,OPR(esi).out
		pop	edi
		mov OPR(esi).out,ebx
		pop	esi
		pop	ebx
		mov ecx,ebp
		pop	ebp
		ret 0
	}
}

#undef OPCALC

