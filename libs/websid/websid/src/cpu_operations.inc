/*
* Emulates MOS Technology 6510 CPU operations.
*
* This file is directly included into cpu.c and it is pretty entangled
* with the respective code.
*
* Implementation was originally based on the code from the old TinySid, but
* it has meanwhile been rewritten almost completely:
*
*    - added impls for illegal 6510 op codes, fixed errors in V-flag calculation,
*      added handling for 6510 addressing "bugs"
*    - correct total cycle-time calculation for ALL 6510 OP codes
*    - switched from a "per OP" based stepping to an approximated "per clock" mode
*      (as a base for "badline" handling, etc)
*
*
* The main "APIs" provided are the prefetchOperation() function (which determines
* the timing of a specific operation) and the runPrefetchedOp() function (which
* provides the operation's actual functionality).
*
* Note: perf test shows that an explicitly created jump-table actually makes the
* overall emulation about 1-2% slower as compared to the ugly switch statement
* currently used in runPrefetchedOp(). This shows that the optimizer already does
* a good job when handling that "switch" statement, but it also shows the
* significant overhead that seems to be associated with calling functions in
* WebAssembly. The code would certainly benefit a lot from some added structuring
* elements. But eventhough high-end mobile phones have become fast enough in the
* past couple of years to run the emulator without a hitch, there are quite a few
* users with somewhat older hardware that is already struggling with the CPU load
* of the current impl. I am therefore reluctant to touch the code just now.
*
*
* REMINDER / CAUTION!!!
*
* Something seems to be seriously fucked-up in the C or Emscripten infrastructure
* and maybe it is a problem specific to blocks used in "case" statements. In any
* case this can lead to surprising phantom bugs, e.g.:
*
*     case foo: {
*	  // tmp= _x & _y;
*        someFunc(_x & _y);
*     }
* The argument passed to someFunc will be total garbage unless the unused tmp
* variable is previously assigned with the same expression. WTF?!
*
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.95
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#ifdef TEST
uint8_t test_running = 0;
char _load_filename[32];
#endif


// ----------------------- CPU state -----------------------------

static uint16_t _pc;					// program counter

#define FLAG_N 128
#define FLAG_V 64
#define FLAG_B1 32	// unused
#define FLAG_B0 16	// Break Command Flag
#define FLAG_D 8
#define FLAG_I 4
#define FLAG_Z 2
#define FLAG_C 1

// compiler used by EMSCRIPTEN is actually too dumb to even properly use
// "inline" for the below local function (using macros as a workaround now)!

// CAUTION: do NOT use this for FLAG_I - use SETFLAG_I below!
#define SETFLAGS(flag, cond) \
    if (cond) _p |= (int32_t)flag;\
    else _p &= ~(int32_t)flag;

#define SETFLAG_I(cond) \
    if (cond) { \
		_no_flag_i = 0; /* better cache this since checked every cycle */\
		_p |= (int32_t)FLAG_I;\
    } else {  \
		_no_flag_i = 1; \
		_p &= ~(int32_t)FLAG_I; \
	}

static uint8_t _p;						// processor status register (see above flags)
static uint8_t _no_flag_i;				// perf opt redundancy (see _p)

// testcase: Game_Player.sid - the timing/behavior of what happens when a NMI
// interrupts a RTI is currently flawed (leading to a stack-corruption in the example song)
// this is just a quick hack band-aid to incorrectly block NMI via via FLAG_I in this case
static uint8_t _no_nmi_hack = 1;

void cpuHackNMI(uint8_t on) {
	_no_nmi_hack= !on;
}



static uint8_t _a, _x, _y;				// accumulator & x, y registers

// stack handling (just wraps around)
static uint8_t _s; 						// stack pointer

static void push(uint8_t val) {
    MEM_WRITE_RAM(0x100 + _s, val);
	_s = (_s - 1) & 0xff;
}
static uint8_t pop() {
	_s = (_s + 1) & 0xff;
    return MEM_READ_RAM(0x100 + _s);
}

#ifdef DEBUG
uint16_t cpuGetPC() {
	return _pc;
}

uint8_t cpuGetSP() {
	return _s;
}
#endif


// ----------------------- CPU instructions -----------------------------


// MOS6510 instruction modes
#define imp 0
#define imm 1
#define abs 2
#define abx 3
#define aby 4
#define zpg 6
#define zpx 7
#define zpy 8
#define ind 9
#define idx 10
#define idy 11
#define acc 12
#define rel 13

// used instruction modes indexed by op-code
static const int32_t _modes[256] = {
	imp,idx,abs,idx,zpg,zpg,zpg,zpg,imp,imm,acc,imm,abs,abs,abs,abs,
	rel,idy,abs,idy,zpx,zpx,zpx,zpx,imp,aby,imp,aby,abx,abx,abx,abx,
	abs,idx,imp,idx,zpg,zpg,zpg,zpg,imp,imm,acc,imm,abs,abs,abs,abs,
	rel,idy,imp,idy,zpx,zpx,zpx,zpx,imp,aby,imp,aby,abx,abx,abx,abx,
	imp,idx,imp,idx,zpg,zpg,zpg,zpg,imp,imm,acc,imm,abs,abs,abs,abs,
	rel,idy,imp,idy,zpx,zpx,zpx,zpx,imp,aby,imp,aby,abx,abx,abx,abx,
	imp,idx,imp,idx,zpg,zpg,zpg,zpg,imp,imm,acc,imm,ind,abs,abs,abs,
	rel,idy,imp,idy,zpx,zpx,zpx,zpx,imp,aby,imp,aby,abx,abx,abx,abx,
	imm,idx,imm,idx,zpg,zpg,zpg,zpg,imp,imm,imp,imm,abs,abs,abs,abs,
	rel,idy,imp,idy,zpx,zpx,zpy,zpy,imp,aby,imp,aby,abx,abx,aby,aby,
	imm,idx,imm,idx,zpg,zpg,zpg,zpg,imp,imm,imp,imm,abs,abs,abs,abs,
	rel,idy,imp,idy,zpx,zpx,zpy,zpy,imp,aby,imp,aby,abx,abx,aby,aby,
	imm,idx,imm,idx,zpg,zpg,zpg,zpg,imp,imm,imp,imm,abs,abs,abs,abs,
	rel,idy,imp,idy,zpx,zpx,zpx,zpx,imp,aby,imp,aby,abx,abx,abx,abx,
	imm,idx,imm,idx,zpg,zpg,zpg,zpg,imp,imm,imp,imm,abs,abs,abs,abs,
	rel,idy,imp,idy,zpx,zpx,zpx,zpx,imp,aby,imp,aby,abx,abx,abx,abx
};


// enum of all mnemonic codes of the MOS6510 operations
enum {
	adc, alr, anc, and, ane, arr, asl, bcc, bcs, beq, bit, bmi, bne, bpl, brk, bvc,
	bvs, clc, cld, cli, clv, cmp, cpx, cpy, dcp, dec, dex, dey, eor, inc, inx, iny,
	isb, jam, jmp, jsr, lae, lax, lda, ldx, ldy, lsr, lxa, nop, ora, pha, php, pla,
	plp, rla, rol, ror, rra, rti, rts, sax, sbc, sbx, sec, sed, sei, sha, shs, shx,
	shy, slo, sre, sta, stx, sty, tax, tay, tsx, txa, txs, tya,
	// added pseudo OPs (replacing unusable JAM OPs):
	sti, stn
};

static uint8_t _opc;						// last executed opcode

static const int32_t _mnemonics[256] = {
	brk,ora,sti,slo,nop,ora,asl,slo,php,ora,asl,anc,nop,ora,asl,slo,
	bpl,ora,stn,slo,nop,ora,asl,slo,clc,ora,nop,slo,nop,ora,asl,slo,
	jsr,and,jam,rla,bit,and,rol,rla,plp,and,rol,anc,bit,and,rol,rla,
	bmi,and,jam,rla,nop,and,rol,rla,sec,and,nop,rla,nop,and,rol,rla,
	rti,eor,jam,sre,nop,eor,lsr,sre,pha,eor,lsr,alr,jmp,eor,lsr,sre,
	bvc,eor,jam,sre,nop,eor,lsr,sre,cli,eor,nop,sre,nop,eor,lsr,sre,
	rts,adc,jam,rra,nop,adc,ror,rra,pla,adc,ror,arr,jmp,adc,ror,rra,
	bvs,adc,jam,rra,nop,adc,ror,rra,sei,adc,nop,rra,nop,adc,ror,rra,
	nop,sta,nop,sax,sty,sta,stx,sax,dey,nop,txa,ane,sty,sta,stx,sax,
	bcc,sta,jam,sha,sty,sta,stx,sax,tya,sta,txs,shs,shy,sta,shx,sha,
	ldy,lda,ldx,lax,ldy,lda,ldx,lax,tay,lda,tax,lxa,ldy,lda,ldx,lax,
	bcs,lda,jam,lax,ldy,lda,ldx,lax,clv,lda,tsx,lae,ldy,lda,ldx,lax,
	cpy,cmp,nop,dcp,cpy,cmp,dec,dcp,iny,cmp,dex,sbx,cpy,cmp,dec,dcp,
	bne,cmp,jam,dcp,nop,cmp,dec,dcp,cld,cmp,nop,dcp,nop,cmp,dec,dcp,
	cpx,sbc,nop,isb,cpx,sbc,inc,isb,inx,sbc,nop,sbc,cpx,sbc,inc,isb,
	beq,sbc,jam,isb,nop,sbc,inc,isb,sed,sbc,nop,isb,nop,sbc,inc,isb
};


void cpuStatusInit() {
	_pc = _a = _x = _y = _s = _p = 0;
	_no_flag_i = 1;

#ifdef TEST
	test_running = 1;

	_s = 0x0; 	// this should be equivalent to the 0xfd that the tests expect:
	push(0);	// use as marker to know when to return
	push(0);
	push(0);

	_p = 0x00;	// idiotic advice to set I-flag! (see "irq" tests)
	_no_flag_i = 1;

	_pc = 0x0801;

#endif

}

void cpuSetProgramCounter(uint16_t pc, uint8_t a) {
	_a = a;
	_pc = pc;

    _x =_y = 0;
    _p = 0;
	_no_flag_i = 1;
    _s = 0xff;

	push(0);	// marker used to detect when "init" returns to non existing "main"
	push(0);
}

// ----------------------- operation's timing -----------------------------

// Cycles per operation (adjustments apply) - note: these timings
// only consider the time to the next OP but not the additional
// cycles at the OPs end that overlap with the fetching of the
// next OP (due to pipelining)!

static const int32_t _opbase_frame_cycles[256] = {
	7,6,7,8,3,3,5,5,3,2,2,2,4,4,6,6,
	2,5,7,8,4,4,6,6,2,4,2,7,4,4,7,7,
	6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
	2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
	6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
	2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
	6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
	2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
	2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
	2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
	2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
	2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
	2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
	2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
	2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
	2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7
};

// The real CPU would perform different steps of an operation in each
// clock cycle. The timing of the respective intra operation steps is
// NOT handled accurately in this emulation but instead everything is
// just updated in one cycle. For most practical purposes this should
// not be a problem: Ops run atomically and nobody else can (for example)
// look at the stack to notice that it should already have been
// updated some cycles earlier. However there is one exception: The
// Flag_I is relevant for the precise timing of what may or may not
// happen directly after the current operation. If an operation clears
// the flag some cycles before its end then this may allow an interrupt
// to be handled immediately after the operation has completed - but if
// this clearing is delayed then that interrupt will also be incorrectly
// delayed.

// The below trigger serves as a "poor man's" workaround here and allows
// to control the cycle at which the updates are performed for each
// operation (i.e. how many cycles before its end the updates should be
// performed) CAUTION: only works ops woth more than 2 cycles and the
// 1. cycle cannot be addressed here (due to where the test is performed).

static const int32_t _opbase_write_trigger[256] = {
	0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,	// irq call
	0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,	// nmi call
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// todo: check if plp (0x28) timing might be improved here
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// rti
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// sei's "check IRQ then update Flag_I" requires special handling (not here)
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

#define INIT_OP(opc, dest_opcode, dest_cycles, dest_lead_time, dest_trigger) \
	(dest_opcode) = opc; \
	(dest_cycles) = _opbase_frame_cycles[opc]; \
	(dest_lead_time) = IRQ_LEAD_DEFAULT;	\
	(dest_trigger) = _opbase_write_trigger[opc];


#define ABS_INDEXED_ADDR(ad, ad2, reg) \
	ad = memGet((*pc)); \
	ad |= memGet((*pc) + 1) <<8; \
	ad2 = ad + reg

#define CHECK_BOUNDARY_CROSSED(ad, ad2) \
	if ((ad2 & 0xff00) != (ad & 0xff00)) { \
		adjustment++; \
	}

#define INDIRECT_INDEXED_ADDR(ad, ad2) \
	ad = memGet((*pc)); \
	ad2 = memGet(ad); \
	ad2 |= memGet((ad + 1) & 0xff) << 8; \
	ad = ad2 + _y;


static uint8_t adjustPageBoundaryCrossing(const uint16_t* pc, int32_t mode) {

	// only relevant/called in abx/aby/idy mode and depending
	// on the operation some of these modes may not exist, e.g.
	// NOP: only exists in "abx" variant
	// LDX: only "aby"
	// LDY: only "abx"

    uint16_t ad, ad2;

	uint8_t adjustment = 0;

    switch(mode) {
        case abx:
			ABS_INDEXED_ADDR(ad, ad2, _x);
			CHECK_BOUNDARY_CROSSED(ad, ad2)
			break;
        case aby:
			ABS_INDEXED_ADDR(ad, ad2, _y);
			CHECK_BOUNDARY_CROSSED(ad, ad2)
			break;
        case idy:
			// indirect indexed, e.g. LDA ($20),Y
			INDIRECT_INDEXED_ADDR(ad, ad2);
			CHECK_BOUNDARY_CROSSED(ad, ad2)
			break;
    }
	return adjustment;
}

static uint8_t adjustBranchTaken(const uint16_t* pc, uint8_t opc, uint8_t* lead_time) {
	uint16_t wval;

    int8_t dist = (int8_t)memGet((*pc));	// like getInput(opc, imm)
    wval = ((*pc) + 1) + dist;

	// + 1 cycle if branches to same page
	// + 2 cycles if branches to different page
	uint8_t adjustment = ((((*pc) + 1) & 0x100) != (wval & 0x100)) ? 2 : 1;

	// special case IRQ lead time.. (applies only when on same page)
	if (adjustment == 1) {
		(*lead_time) += 1;
	}
	return adjustment;
}

// determines next operation with its duration and interrupt lead time
static void prefetchOperation( int16_t* opcode, int8_t* cycles, uint8_t* lead_time, int8_t* trigger) {

	// The operation MUST BE fetched in the 1st cycle (i.e. when
	// prefetching is none - or the wrong command could be used
	// later .. see "cia1tb123" test - where the command byte is
	// updated by the timer - changing the OP while the instruction
	// is already executed)

    uint8_t opc = memGet(_pc++);	// no need to skip this same byte again later

	// NOTE: prefetch must leave the _pc pointing to the 1st byte after the opcode!
	// i.e. the below code MUST NOT update the _pc!

    int32_t mode = _modes[opc];

	INIT_OP(opc, (*opcode), (*cycles), (*lead_time), (*trigger));

	// calc adjustments
	switch (_mnemonics[opc]) {

		// ops that are subject to +1 cycle on page crossing - according to:
		// 1) synertek_programming_manual
		// 2) MOS6510UnintendedOpcodes
		// 3) "Extra Instructions Of The 65XX Series CPU"

		// both 2&3 claim that for "and", "ora" and "lae" the "iny"
		// correction does not apply - however the "irq" test from the
		// test suite shows that this claim is incorrect

		case adc:
        case and:
		case cmp:
		case eor:
		case lae:	// see 2,3: only aby exists
		case lax:	// see 2,3: only aby,idy exist
		case lda:
		case ldx:
		case ldy:
		case nop:	// see 2: only abx exits
        case ora:
		case sbc:
			(*cycles) += adjustPageBoundaryCrossing(&_pc, mode);
            break;

        case bcc:
            if (!(_p & FLAG_C)) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
        case bcs:
            if (_p & FLAG_C) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
        case bne:
            if (!(_p & FLAG_Z)) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
        case beq:
            if (_p & FLAG_Z) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
        case bpl:
            if (!(_p & FLAG_N)) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
        case bmi:
            if (_p & FLAG_N) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
        case bvc:
            if (!(_p & FLAG_V)) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
        case bvs:
            if (_p & FLAG_V) (*cycles) += adjustBranchTaken(&_pc, opc, lead_time);
            break;
		case sei:
			// special case SEI: the Flag_I would be set between the operation's 2
			// cycles but the timing of when the IRQ is checked is also special (as
			// compared to other ops), i.e. it doesn't fit into this emulators impl
			// of checking the IRQ condition at the start of each cycle..

			// only possible if flag wasn't already set before:
			_slip_status = (_p & FLAG_I) ? BLOCKED : POTENTIAL_SLIP;

			// perform the op immediately (i.e. the FLAG_I is blocked too soon and
			// this must be compensated for in the respective IRQ checks later)
			SETFLAG_I(1); // instead of in runPrefetchedOp() this op is directly run now!

			break;
		default:
			break;
    }
}




// ------------------ operation's actual functionality ---------------------


static uint8_t getInput(const int32_t *mode) {
	// reads all the bytes belonging to the operation and
	// advances the _pc accordingly

    uint16_t ad;

    switch(*mode) {
        case acc:
            return _a;
        case imp:
            return 0;
        case imm:
            return memGet(_pc++);
        case abs:
            ad = memGet(_pc++);
            ad |= memGet(_pc++) << 8;
            return memGet(ad);
        case abx:
            ad = memGet(_pc++);
            ad |= memGet(_pc++) << 8;
            return memGet(ad + _x);
       case aby:
            ad = memGet(_pc++);
            ad |= memGet(_pc++) << 8;
            return memGet(ad + _y);
        case zpg:
			ad = memGet(_pc++);
            return memGet(ad);
        case zpx:
            ad = memGet(_pc++) + _x;
			ad &=  0xff;
            return memGet(ad);
        case zpy:
            ad = memGet(_pc++) + _y;
			ad &= 0xff;
            return memGet(ad);
        case idx:
			// indexed indirect, e.g. LDA ($10,X)
            ad = memGet(_pc++) + _x;
            ad = memGet(ad & 0xff) | (memGet((ad + 1) & 0xff) << 8);
            return memGet(ad);
        case idy:
			// indirect indexed, e.g. LDA ($20),Y
            ad = memGet(_pc++);
            ad = (memGet(ad) | (memGet((ad + 1) & 0xff) << 8)) + _y;
            return memGet(ad);
    }
    return 0;
}

static int32_t getOutputAddr(const int32_t *mode) {	// based on copy of setOutput
    uint16_t ad;
    switch(*mode) {
        case acc:
            return -1;
        case abs:
            ad = memGet(_pc - 2) | (memGet(_pc - 1) << 8);
            return ad;
        case abx:
            ad = (memGet(_pc - 2) | (memGet(_pc - 1) << 8)) + _x;
            return ad;
        case aby:
            ad = (memGet(_pc - 2) | (memGet(_pc - 1) << 8)) + _y;
            return ad;
        case zpg:
            ad = memGet(_pc - 1);
            return ad;
        case zpx:
            ad = memGet(_pc - 1) + _x;
			ad &= 0xff;
            return ad;
        case zpy:
            ad = memGet(_pc - 1) + _y;
			ad &= 0xff;
            return ad;
        case idx:
			// indexed indirect, e.g. LDA ($10,X)
            ad = memGet(_pc - 1) + _x;
            ad = memGet(ad & 0xff) | (memGet((ad + 1) & 0xff) << 8);
            return ad;
        case idy:
			// indirect indexed, e.g. LDA ($20),Y
            ad = memGet(_pc - 1);
            ad = (memGet(ad) | (memGet((ad + 1) & 0xff) << 8)) + _y;
            return ad;
    }
    return -2;
}

/*
static void setOutput(const int32_t *mode, uint8_t val) {
	// note: only used after "getInput", i.e. the _pc
	// is already pointing to the next command

    uint16_t ad;
    switch(*mode) {
        case acc:
            _a = val;
            return;
        case abs:
            ad = memGet(_pc - 2) | (memGet(_pc - 1) << 8);
            memSet(ad, val);
            return;
        case abx:
            ad = (memGet(_pc - 2) | (memGet(_pc - 1) << 8)) + _x;
            memSet(ad, val);
            return;
        case aby:
            ad = (memGet(_pc - 2) | (memGet(_pc - 1) << 8)) + _y;
            memSet(ad, val);
            return;
        case zpg:
            ad = memGet(_pc - 1);
            memSet(ad, val);
            return;
        case zpx:
            ad = memGet(_pc - 1) + _x;
			ad &= 0xff;
            memSet(ad, val);
            return;
        case zpy:
            ad = memGet(_pc - 1) + _y;
			ad &= 0xff;
            memSet(ad, val);
            return;
        case idx:
			// indexed indirect, e.g. LDA ($10,X)
            ad = memGet(_pc - 1) + _x;
            ad = memGet(ad & 0xff) | (memGet((ad + 1) & 0xff) << 8);
			memSet(ad, val);
            return;
        case idy:
			// indirect indexed, e.g. LDA ($20),Y
            ad = memGet(_pc - 1);
            ad = (memGet(ad) | (memGet((ad + 1) & 0xff) << 8)) + _y;
			memSet(ad, val);
            return;
    }
}
*/

// gets hi-byte of addr; used for some obscure illegal ops
// CAUTION: This must be called before processing the additional
// op bytes, i.e. before the _pc has been incremented!
static uint8_t getH1(const int32_t *mode) {
    uint16_t ad;
    switch(*mode) {
        case abs:
            return memGet(_pc + 1) + 1;
        case abx:
            ad = (memGet(_pc + 0) | (memGet(_pc + 1) << 8)) + _x;
            return (ad >> 8) + 1;
        case aby:
            ad = (memGet(_pc + 0) | (memGet(_pc + 1) << 8)) + _y;
            return (ad >> 8) + 1;
        case zpg:
			ad = memGet(_pc + 0);
            return (ad >> 8) + 1;
        case idx:
			// indexed indirect, e.g. LDA ($10,X)
            ad = memGet(_pc + 0) + _x;
            ad = memGet(ad & 0xff) | (memGet((ad + 1) & 0xff) << 8);
            return (ad >> 8) + 1;
        case idy:
			// indirect indexed, e.g. LDA ($20),Y
            ad = memGet(_pc + 0);
            ad = (memGet(ad) | (memGet((ad + 1) & 0xff) << 8)) + _y;
            return (ad >> 8) + 1;
    }
    return 0;
}

static void branch(uint8_t is_taken) {
    if (is_taken) {
		int8_t dist = (int8_t)memGet(_pc++);	// like getInput() in "imm" mode
		_pc += dist;
	} else {
		_pc++;	// just skip the byte
	}
}

// handles a "STx" type operation and advances the _pc to the next operation
static void operationSTx(const int32_t* mode, uint8_t val) {
    uint16_t ad;

    switch(*mode) {
        case acc:
            _a = val;
            return;
        case abs:
            ad = memGet(_pc++);
            ad |= memGet(_pc++) << 8;
            memSet(ad, val);
            return;
        case abx:
            ad = memGet(_pc++);
            ad |= memGet(_pc++) << 8;
            ad += _x;
            memSet(ad, val);
            return;
        case aby:
            ad = memGet(_pc++);
            ad |= memGet(_pc++) << 8;
            ad += _y;
            memSet(ad, val);
            return;
        case zpg:
            ad = memGet(_pc++);
            memSet(ad, val);
            return;
        case zpx:
            ad = memGet(_pc++) + _x;
			ad &= 0xff;
            memSet(ad, val);
            return;
        case zpy:
            ad = memGet(_pc++) + _y;
			ad &= 0xff;
            memSet(ad, val);
            return;
        case idx:
            ad = memGet(_pc++) + _x;
            ad = memGet(ad & 0xff) | (memGet((ad + 1) & 0xff) << 8);
            memSet(ad, val);
            return;
        case idy:
            ad = memGet(_pc++);
            ad = ((memGet(ad) | (memGet((ad + 1) & 0xff) << 8))) + _y;
            memSet(ad, val);
            return;
    }
}

// note:  Read-Modify-Write instructions (ASL, LSR, ROL, ROR, INC, DEC, SLO, SRE,
// RLA, RRA, ISB, DCP) write the originally read value before they then write the
// updated value. This is used by some programs to acknowledge/clear interrupts -
// since the 1st write will clear all the originally set bits.

// The below is a perf optimization to avoid repeated address resolution and
// updates: the CPU's "read-modify-write feature" is only relevant for io-area.
// This optimization causes a ~5% speedup (for single-SID emulation)

// CAUTION: This macro can only be used in a context which allows it to declare a
// temp var named "rmw_addr", i.e. it cannot be used more than once in the same
// context.

// USAGE: the supplied 'r' is the variable that the read result is stored in and
// 'w' is the variable used for the final result to be written and the __VA_ARGS__
// argument is the code that is executed in order to calculate that result - i.e.
// it is run between the the read and the final write. (it is the only macro
// expansion syntax that I found to work for the purpose of propagating a
// respective code-block). DO NOT use expressions for r or w since that may lead
// to weird bugs.

// actual usecases?:
//
// - the READ_MODIFY_WRITE is well known to be used with d019 (as a shortcut to
//   ACKN RASTER IRQ)
//
// - respective ops are actually used in the context of SID WF register (see
//   Soundcheck.sid), but re-setting the existing value in the 1st write should NOT
//   have any effect on the SID and it is merely used to save a cycle on a combined
//   AND/STA (however use of the 2x write will break the current digi detection logic
//   for PWM and FM!)
//
// - DC0D/DD0D might be a problem here.. depending on the status read from the register,
//   the 1st write may enable or disable mask bits.. unrelated to whatever the 2nd
//   write may still be changing later.. also there is a timing issue since the 2
//   writes are normally performed with a certain delay (1 cycle?) whereas here
//   everything is done instantly which may well upset the correct CIA behavior..
//   (=> in any case it is nothing that the current test-suite would detect.. nor
//   currently complains about)

#define READ_MODIFY_WRITE(mode, r, w, ...) \
    r = getInput(mode); \
	int32_t rmw_addr= getOutputAddr(mode); \
	if (rmw_addr == 0xd019) { /* only relevant usecase */\
		MEM_SET_IO(rmw_addr, r); \
		__VA_ARGS__ \
		MEM_SET_IO(rmw_addr, w); \
	} else { \
		__VA_ARGS__ \
		if(rmw_addr < 0) { _a = w; } /* acc mode.. no need to recheck */\
		else { memSet(rmw_addr, w); } \
	}

static void runPrefetchedOp() {

//	if (_pc == 0xEA79) fprintf(stderr, "%x %x\n", memGet(0xEA79), memGet(0xEA7a));


	// "prefetch" already loaded the opcode (_pc already points to next byte):
	_opc = _exe_instr_opcode;	// use what was actually valid at the 1st cycle of the op

    switch (_mnemonics[_opc]) {

		// pseudo ops
        case sti:	// run IRQ
			push(_pc >> 8);		// where to resume after the interrupt
			push(_pc & 0xff);

			if (_opc == SEI_OP /*&& (_slip_status == SLIPPED_SEI)*/) {	// 2nd test should be redundant
				// if IRQ occurs while SEI is executing then the Flag_I should also
				// be pushed to the stack, i.e. it RTI will then NOT clear Flag_I!
				SETFLAG_I(1);
			}

			push(_p | FLAG_B1);	// only in the stack copy ( will clear Flag_I upon RTI...)

			SETFLAG_I(1);

			_pc = memGet(0xfffe);			// IRQ vector
			_pc |= memGet(0xffff) << 8;
			break;

        case stn:	// run NMI
			push(_pc >> 8);	// where to resume after the interrupt
			push(_pc & 0xff);
			push(_p | FLAG_B1);	// only in the stack copy

			// "The 6510 will set the IFlag on NMI, too. 6502 untested." (docs of test suite)
			// seems the kernal developers did not know thst... see SEI in FE43 handler..
			SETFLAG_I(1);

			_pc = memGet(0xfffa);			// NMI vector
			_pc |= memGet(0xfffb) << 8;
			break;

		// regular ops
        case adc: {
			uint8_t in1 = _a;
			uint8_t bval = getInput(&(_modes[_opc]));

			// note: The carry flag is used as the carry-in (bit 0)
			// for the operation, and the resulting carry-out (bit 8)
			// value is stored in the carry flag.

            uint16_t wval = (uint16_t)in1 + bval + ((_p & FLAG_C) ? 1 : 0);	// "carry-in"
            SETFLAGS(FLAG_C, wval & 0x100);

			_a = (uint8_t)wval;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);

			// calc overflow flag (http://www.righto.com/2012/12/the-6502-overflow-flag-explained.html)
			// also see http://www.6502.org/tutorials/vflag.html
			SETFLAGS(FLAG_V, (~(in1 ^ bval)) & (in1 ^ _a) & 0x80);
			}
            break;

		case alr: { // aka ASR - Kukle.sid, Raveloop14_xm.sid (that song has other issues though)
			//	ALR #{imm} = AND #{imm} + LSR
            uint8_t bval = getInput(&(_modes[_opc]));
			_a = _a & bval;

            SETFLAGS(FLAG_C, _a & 1);
            _a >>= 1;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case anc: { // Kukle.sid, Axelf.sid (Crowther), Whats_Your_Lame_Excuse.sid, Probing_the_Crack_with_a_Hook.sid
            uint8_t bval = getInput(&(_modes[_opc]));
			_a = _a & bval;

			// http://codebase64.org/doku.php?id=base:some_words_about_the_anc_opcode
            SETFLAGS(FLAG_C, _a & 0x80);
			// supposedly also sets these (http://www.oxyron.de/html/opcodes02.html)
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case and: {
            uint8_t bval = getInput(&(_modes[_opc]));
            _a &= bval;

			SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case ane: { // aka XAA; another useless op that is only used in the tests
	        uint8_t bval = getInput(&(_modes[_opc]));
			const uint8_t con = 0x0; 	// this is HW dependent.. i.e. this OP is bloody useless
			_a = (_a | con) & _x & bval;

            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			}
			break;

        case arr: {		// Whats_Your_Lame_Excurse.sid uses this. sigh.. "the crappier
						// the song...." & Probing_the_Crack_with_a_Hook.sid
			// AND
            uint8_t bval = getInput(&(_modes[_opc]));
            _a &= bval;

			// set C+V based on this intermediate state of bits 6+7 (before ROR)
			uint8_t bit7 = !!(_a & 0x80);
			uint8_t bit6 = !!(_a & 0x40);

			int32_t c = !!(_p & FLAG_C);

			SETFLAGS(FLAG_V, bit7 ^ bit6);
            SETFLAGS(FLAG_C, bit7);

			// ROR - C+V not affected here
            _a >>= 1;

			if (c) {
				_a |= 0x80;	// exchange bit 7 with carry
			}
            SETFLAGS(FLAG_N, _a & 0x80);
            SETFLAGS(FLAG_Z, !_a);
			}
            break;

        case asl: {
			const int32_t *mode = &(_modes[_opc]);
			uint16_t wval;
			READ_MODIFY_WRITE(mode, wval, (uint8_t)wval, {
				wval <<= 1;
			});

            SETFLAGS(FLAG_Z, !(wval & 0xff));
            SETFLAGS(FLAG_N, wval & 0x80);
            SETFLAGS(FLAG_C, wval & 0x100);
			} break;

        case bcc:
            branch(!(_p & FLAG_C));
            break;

        case bcs:
            branch(_p & FLAG_C);
            break;

        case bne:
            branch(!(_p & FLAG_Z));
            break;

        case beq:
            branch(_p & FLAG_Z);
            break;

        case bpl:
            branch(!(_p & FLAG_N));
            break;

        case bmi:
            branch(_p & FLAG_N);
            break;

        case bvc:
            branch(!(_p & FLAG_V));
            break;

        case bvs:
            branch(_p & FLAG_V);
            break;

        case bit: {
            uint8_t bval = getInput(&(_modes[_opc]));

            SETFLAGS(FLAG_Z, !(_a & bval));
            SETFLAGS(FLAG_N, bval & 0x80);
            SETFLAGS(FLAG_V, bval & 0x40);	// bit 6
			} break;

        case brk:
#ifdef TEST
			// tests use various ROM routines which would normally
			// produce screen output (etc)..
	/*		if (_pc == 0xFFE5) {	// get a char
				// tests only call this to wait AFTER an error

#ifdef EMSCRIPTEN
				EM_ASM_({ console.log("test failed?");});
#endif
				uint16_t wval = pop();
				wval |= pop() << 8;	// RTS to where it came from
				_pc = wval + 1;
				break;
			} else */
			if (_pc == 0xFFD3) {	// print char via FFD2

				// BASIC start of a single test would init this to 0 whereas direct
				// start from 0801 will set this to 1. It controls if the tests are
				// chained, and chaining is here deliberately activated so that
				// LOAD trigger (E16F) can be used to detect when a test has completed
				memWriteRAM(0x030C, 0);

				// easier to deal with this in JavaScript (pervent optimizer renaming the func)
#ifdef EMSCRIPTEN
				EM_ASM_({ window['outputPETSCII'](($0));}, _a);
#endif

				uint16_t wval = pop();
				wval |= pop() << 8;	// RTS to where it came from
				_pc = wval + 1;
				break;
			} else if (_pc == 0xBDCE) {	// print AX as number via BDCD
				// just another way of printing PETSCII
				// easier to deal with this in JavaScript (pervent optimizer renaming the func)
#ifdef EMSCRIPTEN
				EM_ASM_({ window['outputPETSCII'](($0));}, _x);
#endif

				uint16_t wval = pop();
				wval |= pop() << 8;	// RTS to where it came from
				_pc = wval + 1;
				break;
			} else if (_pc == 0xE170) {	// load E16F
				// report the next test file (this means that this test was successful)
				uint16_t addr = memReadRAM(0x00bb) | (memReadRAM(0x00bc) << 8);
				uint8_t len = memReadRAM(0x00b7);
				if (len > 31) len = 31;
				for (int i= 0; i<len; i++) {
					_load_filename[i] = memReadRAM(addr++);
				}
				_load_filename[len] = 0;

				// easier to deal with this in JavaScript (pervent optimizer renaming the func)
#ifdef EMSCRIPTEN
				EM_ASM_({ window['loadFileError'](Pointer_stringify($0));}, _load_filename);
#endif

				test_running = 0;
				break;
			} else if (_pc == 0xFFE5) {	// scan keyboard
				_a = 3;			// always return this "key press"
				uint16_t wval = pop();
				wval |= pop() << 8;
				_pc = wval + 1;
				break;
			} else if ((_pc == 0x8001) || (_pc == 0xA475)) {	// exit
				test_running = 0;
				break;
			}
#endif
#ifdef DEBUG
			// excessive printing may block the browser
			
#ifdef EMSCRIPTEN
			EM_ASM_({ console.log('BRK from:        $' + ($0).toString(16));}, _pc-1);
#else
			fprintf(stderr, "BRK from:        $%x\n", _pc-1);
#endif
#endif
			// _pc has already been incremented by 1 (see above)
			// (return address to be stored on the stack is original _pc+2 )
			push((_pc + 1) >> 8);
			push((_pc + 1));
			push(_p | FLAG_B0 | FLAG_B1);	// only in the stack copy

			_pc = memGet(0xfffe);
			_pc |= memGet(0xffff) << 8;		// somebody might finger the IRQ vector or the BRK vector at 0316/0317 to use this?

			SETFLAG_I(1);
            break;

        case clc:
            SETFLAGS(FLAG_C, 0);
            break;

        case cld:
            SETFLAGS(FLAG_D, 0);
            break;

        case cli:
            SETFLAG_I(0);
			// CLI can never clear the I_Flag fast enough to immediately trigger
			// an IRQ after the CLI, i.e. this should work fine without any add-on
			// handling
            break;

        case clv:
            SETFLAGS(FLAG_V, 0);
            break;

        case cmp: {
            uint8_t bval = getInput(&(_modes[_opc]));
            uint16_t wval = (uint16_t)_a - bval;

			SETFLAGS(FLAG_Z, !wval);		// _a == bval
            SETFLAGS(FLAG_N, wval & 0x80);	// _a < bval
            SETFLAGS(FLAG_C, _a >= bval);
			} break;

        case cpx: {
            uint8_t bval = getInput(&(_modes[_opc]));
            uint16_t wval = (uint16_t)_x - bval;

			SETFLAGS(FLAG_Z, !wval);
            SETFLAGS(FLAG_N, wval & 0x80);
            SETFLAGS(FLAG_C, _x >= bval);
			} break;

        case cpy: {
            uint8_t bval = getInput(&(_modes[_opc]));
            uint16_t wval = (uint16_t)_y - bval;

			SETFLAGS(FLAG_Z, !wval);
            SETFLAGS(FLAG_N, wval & 0x80);
            SETFLAGS(FLAG_C, _y >= bval);
			} break;

        case dcp: {		// used by: Clique_Baby.sid, Musik_Run_Stop.sid
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				// dec
				bval--;
			});

			// cmp
            uint16_t wval = (uint16_t)_a - bval;
            SETFLAGS(FLAG_Z, !wval);
            SETFLAGS(FLAG_N, wval & 0x80);
            SETFLAGS(FLAG_C, _a >= bval);
			} break;

        case dec: {
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, { bval--; });

			SETFLAGS(FLAG_Z, !bval);
            SETFLAGS(FLAG_N, bval & 0x80);
			} break;

        case dex:
            _x--;
            SETFLAGS(FLAG_Z, !_x);
            SETFLAGS(FLAG_N, _x & 0x80);
            break;

        case dey:
            _y--;
            SETFLAGS(FLAG_Z, !_y);
            SETFLAGS(FLAG_N, _y & 0x80);
            break;

        case eor: {
            uint8_t bval = getInput(&(_modes[_opc]));
            _a ^= bval;

			SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case inc: {
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				bval++;
			});

			SETFLAGS(FLAG_Z, !bval);
            SETFLAGS(FLAG_N, bval & 0x80);
			} break;

        case inx:
            _x++;

			SETFLAGS(FLAG_Z, !_x);
            SETFLAGS(FLAG_N, _x & 0x80);
            break;

        case iny:
            _y++;

			SETFLAGS(FLAG_Z, !_y);
            SETFLAGS(FLAG_N, _y & 0x80);
            break;

        case isb: {	// aka ISC; see 'insz' tests
			// inc
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				bval++;
			});

			SETFLAGS(FLAG_Z, !bval);
            SETFLAGS(FLAG_N, bval & 0x80);

			// + sbc
			uint8_t in1 = _a;
			uint8_t in2 = (bval ^ 0xff);	// substract

            uint16_t wval = (uint16_t)in1 + in2 + ((_p & FLAG_C) ? 1 : 0);
            SETFLAGS(FLAG_C, wval & 0x100);

            _a = (uint8_t)wval;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);

			SETFLAGS(FLAG_V, (~(in1 ^ in2)) & (in1 ^ _a) & 0x80);
            }
			break;

		case jam:	// this op would have crashed the C64		
#ifdef EMSCRIPTEN
			EM_ASM_({ console.log('JAM 0:  $' + ($0).toString(16));}, _pc-1);	// less mem than inclusion of fprintf
#else
			fprintf(stderr, "JAM 0:  $%x\n", _pc-1);
#endif
		    _pc = 0;           // just quit the emulation
            break;

        case jmp: {
            uint8_t bval = memGet(_pc++);		// low-byte
            uint16_t wval = memGet(_pc++) << 8;	// high-byte
			
			int32_t mode = _modes[_opc];
            switch (mode) {
                case abs:
					_pc = wval | bval;
                    break;
                case ind:
					// 6502 bug: JMP ($12FF) will fetch the low-byte
					// from $12FF and the high-byte from $1200, i.e.
					// there is never an overflow into the high-byte

                    _pc = memGet(wval | bval);
                    _pc |= memGet(wval | ((bval + 1) & 0xff)) << 8;
                    break;
            }
			} break;

        case jsr: {
			// _pc has already been incremented by 1 (see above)
			// (return address to be stored on the stack is original _pc+2 )
            push((_pc + 1) >> 8);
            push((_pc + 1));
            uint16_t wval = memGet(_pc++);
            wval |= memGet(_pc++) << 8;
		
            _pc = wval;

			} break;

		case lae: { // aka LAS, aka LAR .. just for the tests
            uint8_t bval = getInput(&(_modes[_opc]));

			_a = _x = _s = (bval & _s);

            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

		case lax:
			// e.g. Vicious_SID_2-15638Hz.sid, Kukle.sid
            _a = getInput(&(_modes[_opc]));

			_x = _a;

            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
            break;

		case lxa: {	// Whats_Your_Lame_Excuse.sid - LOL only dumbshit player uses this op..
            uint8_t bval = getInput(&(_modes[_opc]));

			const uint8_t con = 0xff;
			_a |= con;	// roulette what the specific CPU uses here
			_a &= bval;
			_x = _a;

            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case lda:
            _a = getInput(&(_modes[_opc]));

            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
            break;

        case ldx:
            _x = getInput(&(_modes[_opc]));

            SETFLAGS(FLAG_Z, !_x);
            SETFLAGS(FLAG_N, _x & 0x80);
            break;

        case ldy:
            _y = getInput(&(_modes[_opc]));

            SETFLAGS(FLAG_Z, !_y);
            SETFLAGS(FLAG_N, _y & 0x80);
            break;

        case lsr: {
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			uint16_t wval;
			READ_MODIFY_WRITE(mode, bval, (uint8_t)wval, {
				wval = (uint8_t)bval;
				wval >>= 1;
			});

            SETFLAGS(FLAG_Z, !wval);
            SETFLAGS(FLAG_N, wval & 0x80);	// always clear?
            SETFLAGS(FLAG_C, bval & 1);
			} break;

        case nop:
			getInput(&(_modes[_opc]));	 // make sure the PC is advanced correctly
            break;

        case ora: {
            uint8_t bval = getInput(&(_modes[_opc]));

            _a |= bval;

			SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case pha:
            push(_a);
            break;

        case php:
            push(_p | FLAG_B0 | FLAG_B1);	// only in the stack copy
            break;

        case pla:
            _a = pop();

            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
            break;

        case plp: {
			uint8_t bval = pop();
            _p = bval & ~(FLAG_B0 | FLAG_B1);
			_no_flag_i = !(_p & FLAG_I);

			// like SEI this frist polls for IRQ before changing the flag
			// while this happends at the end of the 1st cycle in a 2 cycle SEI
			// this propably corresponds to 3rd cycle of the 4 cycle PLP!

			// "clear" is not the critical scenario here (like CLI) since
			// IRQ is never expected to trigger immediately after the PLP.. i.e.
			// it doesn't matter that the flag is cleared too late.
			// however, with regard to the last 2 cycles "set" should have the
			// same timing behavior as SEI (todo: fixme.. maybe write-cycle based
			// impl would be good enough here..)
			}
			break;

        case rla: {				// see Spasmolytic_part_6.sid
			// rol			
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				int32_t c = !!(_p & FLAG_C);
				SETFLAGS(FLAG_C, bval & 0x80);
				bval <<= 1;
				bval |= c;
			});

			// + and
            _a &= bval;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case rol: {
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				int32_t c = !!(_p & FLAG_C);
				SETFLAGS(FLAG_C, bval & 0x80);
				bval <<= 1;
				bval |= c;
			});

            SETFLAGS(FLAG_N, bval & 0x80);
            SETFLAGS(FLAG_Z, !bval);
			} break;

        case ror: {
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				int32_t c = !!(_p & FLAG_C);
				SETFLAGS(FLAG_C, bval & 1);
				bval >>= 1;
				bval |= 0x80 * c;
			});

			SETFLAGS(FLAG_N, bval & 0x80);
            SETFLAGS(FLAG_Z, !bval);
			} break;

        case rra: {
			// ror
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				int32_t c = !!(_p & FLAG_C);
				SETFLAGS(FLAG_C, bval & 1);
				bval >>= 1;
				bval |= 0x80 * c;
			});

			// + adc
			uint8_t in1 = _a;

            uint16_t wval = (uint16_t)in1 + bval + ((_p & FLAG_C) ? 1 : 0);
            SETFLAGS(FLAG_C, wval & 0x100);

            _a = (uint8_t)wval;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);

			SETFLAGS(FLAG_V, (~(in1 ^ bval)) & (in1 ^ _a) & 0x80);
			} break;

        case rti: {
			// timing hack: some optimized progs use JMP to an RTI
			// that is placed such that the nearby interrupt status
			// register is implicitly read - automatically acknowledging
			// the interrupt without having to explicitly read the register.
			switch(_pc) {
				case 0xdc0d:
				case 0xdd0d:	// e.g. LMan - Vortex.sid
					memGet(_pc);
					break;
			}

//			RTI's 6-clock process is the reverse of the interrupt sequence. In the first clock, 
//			the RTI (op code $40) is read. The next two clocks are used for internal operations, 
//			the fourth for pulling the processor status off the stack, the fifth for pulling the 
//			low return address byte off the stack, and the sixth for pulling the high return 
//			address byte off the stack. In the very next clock, the op code at the return address 
//			is read and the non-interrupt execution resumes.
			
			// note: _p is restored in cycle #4, i.e. after that moment the FLAG_I should be clear!
			// fixme; currently _exe_write_trigger triggers *all* updates immediatley (incorrect), i.e.
			// neither IRQ not NMI behavior is correctly covered here!

			uint8_t bval = pop();	// status before the FLAG_I had been set
            _p = bval & ~(FLAG_B0 | FLAG_B1);
			_no_flag_i = !(_p & FLAG_I);

			// testcases: Game_Player and Synthmeld
			// FIXME open issue: the RTI itself may be interrupted at any point by an NMI (which doesn't
			// care about the FLAG_I and it can be interrupted by IRQ after the 4th cycle, i.e. once the status
			// flag has been restored/FLAG_I cleared)!
			// the simplistic model used here (i.e. to update the stack in one atomar operation in 4th cycle)
			// is incorrect and probably responsible for the issues with the above 2 songs..
			
            uint16_t wval = pop();
            wval |= pop() << 8;

            _pc = wval;	// not like 'rts'! correct address is expected here!
			} break;

        case rts: {
            uint16_t wval = pop();
            wval |= pop() << 8;

			_pc = wval + 1;
			} break;

        case sbc: {
            uint8_t bval = getInput(&(_modes[_opc])) ^ 0xff;

			uint8_t in1 = _a;

            uint16_t wval =(uint16_t)in1 + bval + ((_p & FLAG_C) ? 1 : 0);
            SETFLAGS(FLAG_C, wval & 0x100);

            _a = (uint8_t)wval;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			SETFLAGS(FLAG_V, (~(in1 ^ bval)) & (in1 ^ _a) & 0x80);
			} break;

        case sha: {	// aka AHX; for the benefit of the 'SHAAY' test (etc).. have yet to find a song that uses this
			const int32_t *mode = &(_modes[_opc]);
			uint8_t h = getH1(mode);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				bval = _a & _x & h;
			});
			} break;
			
        case shx: {	// for the benefit of the 'SHXAY' test (etc).. have yet to find a song that uses this
			const int32_t *mode = &(_modes[_opc]);
			uint8_t h = getH1(mode);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				bval = _x & h;
			});
			} break;

        case shy: {	// for the benefit of the 'SHYAY' test (etc).. have yet to find a song that uses this
			const int32_t *mode = &(_modes[_opc]);
			uint8_t h = getH1(mode);		// who cares about this OP
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				bval = _y & h;
			});
			} break;

        case sax: {				// aka AXS; e.g. Vicious_SID_2-15638Hz.sid, Kukle.sid, Synthesis.sid, Soundcheck.sid
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				bval = _a & _x;
			});
			// no flags are affected; registers unchanged
			} break;

		case sbx: { // somtimes called SAX; used in Kukle.sid, Artefacts.sid, Whats_Your_Lame_Excuse.sid, Probing_the_Crack_with_a_Hook.sid
			// affects N Z and C (like CMP)
			uint8_t bval = getInput(&(_modes[_opc]));

            SETFLAGS(FLAG_C, (_x & _a) >= bval);	// affects the carry but NOT the overflow

			_x = ((_x & _a) - bval) & 0xff;	// _a unchanged (calc not affected by input carry)

            SETFLAGS(FLAG_Z,!_x);			// _a == bval
            SETFLAGS(FLAG_N, _x & 0x80);	// _a < bval
			} break;

        case sec:
            SETFLAGS(FLAG_C, 1);
            break;

        case sed:
            SETFLAGS(FLAG_D, 1);
            break;

        case sei:
			// Since SEI is handled specially, the below logic has already been executed directly
			// after the "prefetch" (i.e. too early) and there is nothing left to do now, i.e. at the
			// end of SEI's 2 cycle duration (see special handlng in CHECK_IS_IRQ()).

            // SETFLAG_I(1);
            break;

		case shs: {	// 	aka TAS
			// instable op; hard to think of a good reason why
			// anybody would ever use this..
			_s = _a & _x;

			const int32_t *mode = &(_modes[_opc]);
			uint8_t h = getH1(mode);
			uint8_t bval;
			READ_MODIFY_WRITE(mode, bval, bval, {
				bval = _s&h;
			});
			} break;

        case slo: {			// see Spasmolytic_part_6.sid
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			uint16_t wval;
			READ_MODIFY_WRITE(mode, bval, (uint8_t)wval, {
				wval = (uint8_t)bval;
				wval <<= 1;
			});

            SETFLAGS(FLAG_C, wval & 0x100);
			// + ora
            bval = wval & 0xff;
            _a |= bval;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
			} break;

        case sre: {      		// aka LSE; see Spasmolytic_part_6.sid, Halv_2_2.sid
			// like SLO but shifting right and with eor

			// copied section from 'lsr'
			const int32_t *mode = &(_modes[_opc]);
			uint8_t bval;
			uint16_t wval;
			READ_MODIFY_WRITE(mode, bval, (uint8_t)wval, {
				wval = (uint8_t)bval;
				wval >>= 1;
			});

            SETFLAGS(FLAG_Z, !wval);
            SETFLAGS(FLAG_N, wval & 0x80);
            SETFLAGS(FLAG_C, bval & 1);
			// + copied section from 'eor'
            bval = wval & 0xff;

            _a ^= bval;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);

			} break;

        case sta:
            operationSTx(&(_modes[_opc]), _a);
            break;

        case stx:
            operationSTx(&(_modes[_opc]), _x);
            break;

        case sty:
            operationSTx(&(_modes[_opc]), _y);
            break;

        case tax:
            _x = _a;
            SETFLAGS(FLAG_Z, !_x);
            SETFLAGS(FLAG_N, _x & 0x80);
            break;

        case tay:
            _y = _a;
            SETFLAGS(FLAG_Z, !_y);
            SETFLAGS(FLAG_N, _y & 0x80);
            break;

        case tsx:
			_x = _s;
            SETFLAGS(FLAG_Z, !_x);
            SETFLAGS(FLAG_N, _x & 0x80);
            break;

        case txa:
            _a = _x;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
            break;

        case txs:
            _s = _x;
            break;

        case tya:
            _a = _y;
            SETFLAGS(FLAG_Z, !_a);
            SETFLAGS(FLAG_N, _a & 0x80);
            break;

		default:
			// this should be dead code since all ops are actually implemented above
			getInput(&(_modes[_opc]));	 // just advance PC correctly
    }
}

// --------------- PSID crap -----------------------------------------------

void cpuSetProgramCounterPSID(uint16_t pc) {
	_pc= pc;

   SETFLAG_I(0);		// make sure the IRQ isn't blocked
}

uint8_t cpuIsValidPcPSID() {
	// only used to to run PSID INIT separately.. everything else
	// runs without this limitation

	// for RSIDs there is not really "any" invalid PC
	// test-case: Boot_Zak_v2.sid (uses $0000 for IRQ handler).
	return _pc > 1;
}

void cpuIrqFlagPSID(uint8_t on) {
	SETFLAG_I(on);
}
