/*
* This contains everything to do with the emulation of memory access.
*
* Note: Optionally, original ROM images (kernal, basic, char) can be
* externally supplied to support those few songs that actually depend
* on them.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#include "memory.h"


// memory access interfaces provided by other components
extern uint8_t	sidReadMem(uint16_t addr);
extern void 	sidWriteMem(uint16_t addr, uint8_t value);

extern uint8_t	ciaReadMem(uint16_t addr);
extern void		ciaWriteMem(uint16_t addr, uint8_t value);

extern uint8_t	vicReadMem(uint16_t addr);
extern void		vicWriteMem(uint16_t addr, uint8_t value);


uint8_t*		_memory = 0;

#define BASIC_SIZE 0x2000
static uint8_t _basic_rom[BASIC_SIZE];		// mapped to $a000-$bfff

#define KERNAL_SIZE 0x2000
static uint8_t _kernal_rom[KERNAL_SIZE];	// mapped to $e000-$ffff

#define IO_AREA_SIZE 0x1000
static uint8_t _char_rom[IO_AREA_SIZE];		// mapped to $d000-$dfff

uint8_t*		_io_area = 0;				// mapped to $d000-$dfff


/*
* snapshot of c64 memory right after loading..
* it is restored before playing a new track..
*/
static uint8_t _memory_snapshot[MEMORY_SIZE];

void memSaveSnapshot() {
	memCopyFromRAM(_memory_snapshot, 0, MEMORY_SIZE);
}

void memRestoreSnapshot() {
	memCopyToRAM(_memory_snapshot, 0, MEMORY_SIZE);
}


uint8_t memMatch(uint16_t addr, uint8_t* pattern, uint8_t len) {
	return !memcmp(&(_memory[addr]), pattern, len);
}

static void setMemBank(uint8_t b) {
	// note: processor port related functionality (see addr 0x0) is NOT implemented
	_memory[0x0001] = b;
	/*
	// the only song that I am aware of that uses the "processor port direction"
	// to filter the memory bank settings that it is making is Chocolatebar.sid

	// and to make it work, the below add-on logic would be an additional special
	// case that must be checked for each RAM write (see WRITE_RAM). I'll rather
	// have the emulation run faster for everybody else than to slow it down
	// just to make this one song (let alone a bad one) happy..

	uint8_t read_only_mask = _memory[0x0000] ^ 0xff;
	uint8_t keep = _memory[0x0001] & read_only_mask;
	uint8_t set = b & _memory[0x0000];

	_memory[0x0001] = keep | set;
	*/
}

void memSetDefaultBanksPSID(uint8_t is_rsid, uint16_t init_addr,
							uint16_t load_end_addr) {

	_memory[0x0000] = 0x2f;	// default processor port mask

	// default memory config: basic ROM, IO area & kernal ROM visible
	uint8_t mem_bank_setting = 0x37;

	if (!is_rsid) {
		// problem: some PSID init routines want to initialize
		// registers in the IO area while others actually expect
		// to use the RAM in that area.. none of them setup the
		// memory banks accordingly :(

		if ((init_addr >= 0xd000) && (init_addr < 0xe000)) {
			mem_bank_setting = 0x34;	// default memory config: all RAM

		} else if ((init_addr >= 0xe000)) {
			// PSIDv2 songs like IK_plus.sid, Pandora.sid use
			// the ROM below the kernal *without* setting 0x0001
			// so obviously here we must use a default like this:

			// default memory config: IO area visible, RAM $a000-b000 + $e000-$ffff
			mem_bank_setting = 0x35;

		} else if (load_end_addr >= 0xa000) {
			mem_bank_setting = 0x36;
		} else {
			// normally the kernal ROM should be visible: e.g.
			// A-Maz-Ing.sid uses kernal ROM routines & vectors
			// without setting $01!

			// default memory config: basic ROM, IO area & kernal ROM visible
			mem_bank_setting = 0x37;
		}
	}
	setMemBank(mem_bank_setting);
}

void memResetBanksPSID(uint16_t play_addr) {
	// some PSID actually switch the ROM back on eventhough their
	// code is located there! (e.g. Ramparts.sid - the respective
	// .sid even claims to be "C64 compatible" - what a joke)

	if ((play_addr >= 0xd000) && (play_addr < 0xe000)) {
		setMemBank(0x34);
	} else if (play_addr >= 0xe000) {
		setMemBank(0x35);
	} else if (play_addr >= 0xa000) {
		setMemBank(0x36);
	} else if (play_addr == 0x0){
		// keep whatever the PSID init setup
	} else {
		setMemBank(0x37);
	}
}

/*
* @return 0 if RAM is visible; 1 if KERNAL ROM is visible
*/
#define IS_KERNAL_VISIBLE() \
	(_memory[0x0001] & 0x2)

/*
* @return 0 if RAM is visible; 1 if BASIC ROM is visible
*/
#define IS_BASIC_VISIBLE() \
	((_memory[0x0001] & 0x3) == 3)

/*
* @return 0 if RAM is visible; 1 if CHAR ROM is visible
*/
#define IS_CHARROM_VISIBLE() \
	!(_memory[0x0001] & 0x4) && (_memory[0x0001] & 0x3)

/*
* @return 0 if RAM/ROM is visible; 1 if IO area is visible
*/
#define IS_IO_VISIBLE() \
	((_memory[0x0001] & 0x4) != 0) && ((_memory[0x0001] & 0x7) != 0x4)


uint8_t memReadIO(uint16_t addr) {
	// mirrored regions not implemented for reads.. nobody
	// seems to use this (unlike write access to SID)
	return _io_area[addr - 0xd000];
}

void memWriteIO(uint16_t addr, uint8_t value) {
	_io_area[addr - 0xd000] = value;
}

uint8_t memReadRAM(uint16_t addr) {
	return _memory[addr];
}
void memWriteRAM(uint16_t addr, uint8_t value) {
	 _memory[addr] = value;
}

void memCopyToRAM(uint8_t* src, uint16_t dest_addr, uint32_t len) {
	memcpy(&_memory[dest_addr], src, len);
}
void memCopyFromRAM(uint8_t* dest, uint16_t src_addr, uint32_t len) {
	memcpy(dest, &_memory[src_addr], len);
}

#define RETURN_RAM_AREA(addr) \
	return  _memory[addr];

#define RETURN_IO_AREA(addr) \
	if (IS_IO_VISIBLE()) { \
		if (addr < 0xd400) { \
			return vicReadMem(addr); \
		} else if (addr < 0xd800) { \
			return sidReadMem(addr); \
		} else if ((addr >= 0xdc00) && (addr < 0xde00)) { \
			return ciaReadMem(addr); \
		} else if ((addr >= 0xde00) && (addr < 0xdf00)) { /* exotic scenario last*/ \
			return sidReadMem(addr); \
		} \
		return memReadIO(addr); \
	} else if(IS_CHARROM_VISIBLE()) { \
		return _char_rom[addr - 0xd000]; \
	} else { \
		return _memory[addr]; /* normal RAM access */ \
	}

#define RETURN_BASIC_AREA(addr) \
	if (IS_BASIC_VISIBLE()) { \
		return _basic_rom[addr - 0xa000]; \
	} else { \
		return _memory[addr]; /* normal RAM access */ \
	}

#define RETURN_KERNAL_AREA(addr) \
	if (IS_KERNAL_VISIBLE()) { \
		return _kernal_rom[addr - 0xe000]; \
	} else { \
		return _memory[addr]; /* normal RAM access */ \
	}

uint8_t memGet(uint16_t addr) {
	// performance opt: try to identify most often used areas with
	// least number of comparisons (narrow down from top
	// or bottom using ONE check per area)

	if (addr < 0xa000) {
		RETURN_RAM_AREA(addr);
	} else if (addr >= 0xe000) {
		// presuming the IRQ/NMI vectors/handlers make this
		// more often used..
		RETURN_KERNAL_AREA(addr);
	} else if ((addr >= 0xd000)) {
		// presuming that BASIC stuff is less relevant
		// the IO comes next (not tested)
		RETURN_IO_AREA(addr);
	} else if (addr >= 0xc000) {
		// for the 1000-2000 songs that are located here
		// one test could be avoided by going straight for
		// it.. however that would slow down every IO or
		// kernal access
		RETURN_RAM_AREA(addr);
	} else {
		RETURN_BASIC_AREA(addr);
	}
}

// normal RAM or kernal ROM (even if the ROM is visible,
// writes always go to the RAM) example: Vikings.sid copied
// player data to BASIC ROM area while BASIC ROM is turned on..
#define WRITE_RAM(addr, value) \
	/* if (addr == 0x0001) setMemBank(value); else*//* not worth it! */\
	_memory[addr] = value;

// normally all writes to IO areas should "write
// through" to RAM, however PSID garbage does not
// always seem to tolerate that (see Fighting_Soccer)
#define WRITE_IO(addr, value) \
	if (IS_IO_VISIBLE()) { \
		if (addr < 0xd400) {									/* vic stuff */ \
			vicWriteMem(addr, value); \
			return; \
		} else if (((addr >= 0xd400) && (addr < 0xd800)) ||  \
					((addr >= 0xde00) && (addr < 0xdf00))) {	/* SID stuff */ \
			sidWriteMem(addr, value); \
			return; \
		} else if ((addr >= 0xdc00) && (addr < 0xde00)) {		/* CIA timers */ \
			ciaWriteMem(addr, value); \
			/* hack: make sure timer latches can be retrieved from RAM */ \
			_memory[addr] = value; /* write RAM */ \
			return; \
		} \
		_io_area[addr - 0xd000] = value; \
	} else { \
		_memory[addr] = value; /* write RAM */ \
	}

void memSetIO(uint16_t addr, uint8_t value) {
	WRITE_IO(addr, value);
}

void memSet(uint16_t addr, uint8_t value) {
	if (addr < 0xd000) {		// this most often used area should need the least comparisons
		WRITE_RAM(addr, value);
	} else if (addr < 0xe000) {	// handle I/O area
		WRITE_IO(addr, value);
	} else {
		WRITE_RAM(addr, value);
	}
}

#ifdef TEST
const static uint8_t _irq_handler_test_FF48[19] = {0x48,0x8A,0x48,0x98,0x48,0xBA,0xBD,0x04,0x01,0x29,0x10,0xf0,0x03,0x6c,0x16,0x03,0x6C,0x14,0x03};	// test actually uses BRK
const static uint8_t _irq_restore_vectors_FD15[27] = {0xa2,0x30,0xa0,0xfd,0x18,0x86,0xc3,0x84,0xc4,0xa0,0x1f,0xb9,0x14,0x03,0xb0,0x02,0xb1,0xc3,0x91,0xc3,0x99,0x14,0x03,0x88,0x10,0xf1,0x60};
const static uint8_t _init_io_FDA3[86] = {0xA9,0x7F,0x8D,0x0D,0xDC,0x8D,0x0D,0xDD,0x8D,0x00,0xDC,0xA9,0x08,0x8D,0x0E,0xDC,0x8D,0x0E,0xDD,0x8D,0x0F,0xDC,0x8D,0x0F,0xDD,0xA2,0x00,0x8E,0x03,0xDC,0x8E,0x03,0xDD,0x8E,0x18,0xD4,0xCA,0x8E,0x02,0xDC,0xA9,0x07,0x8D,0x00,0xDD,0xA9,0x3F,0x8D,0x02,0xDD,0xA9,0xE7,0x85,0x01,0xA9,0x2F,0x85,0x00,0xAD,0xA6,0x02,0xF0,0x0A,0xA9,0x25,0x8D,0x04,0xDC,0xA9,0x40,0x4C,0xF3,0xFD,0xA9,0x95,0x8D,0x04,0xDC,0xA9,0x42,0x8D,0x05,0xDC,0x4C,0x6E,0xFF};
const static uint8_t _schedule_ta_FF6E[18] = {0xA9,0x81,0x8D,0x0D,0xDC,0xAD,0x0E,0xDC,0x29,0x80,0x09,0x11,0x8D,0x0E,0xDC,0x4C,0x8E,0xEE};
const static uint8_t _serial_clock_hi_EE8E[9] = {0xAD,0x00,0xDD,0x09,0x10,0x8D,0x00,0xDD,0x60};
#endif
const static uint8_t _irq_handler_FF48[19] = {0x48,0x8A,0x48,0x98,0x48,0xBA,0xBD,0x04,0x01,0x29,0x10,0xF0,0x03,0xEA,0xEA,0xEA,0x6C,0x14,0x03};	// disabled BRK branch

/* original:
EA75   A5 01      LDA $01
EA77   29 1F      AND #$1F
EA79   85 01      STA $01
EA7B   20 87 EA   JSR $EA87   ; scan keyboard call	replaced by => E6 A2 EA		INC $A2 / NOP		limitation: potential conflict with songs that use $a2 on their own!
EA7E   AD 0D DC   LDA $DC0D
EA81   68         PLA
EA82   A8         TAY
EA83   68         PLA
EA84   AA         TAX
EA85   68         PLA
EA86   40         RTI
*/
const static uint8_t _irq_handler_EA75[18] = {0xa5,0x01,0x29,0x1f,0x85,0x01,0xE6,0xA2,0xEA, 0xAD,0x0D,0xDC,0x68,0xA8,0x68,0xAA,0x68,0x40};	// added "INC $a2" (i.e. "TOD" frame count); Double_Falcon.sid depends on the extra NOP

// standard handler invoked via kernal 0xfffa/b NMI vector (that redirects via 0x0318 vector - which
// by default points to the kernal 0xfe47 handler - which here is the puny RTI at the end..)
const static uint8_t _nmi_handler_FE43[5] = {0x78,0x6c,0x18,0x03,0x40};
const static uint8_t _irq_end_handler_FEBC[6] = {0x68,0xa8,0x68,0xaa,0x68,0x40};

const static uint8_t _delay_handler_EEB3[8] = {0x8a,0xa2,0xb8,0xca,0xd0,0xfd,0xaa,0x60};	// used by Random_Ninja.sid

const static uint8_t _driverPSID[33] = {
	// PSID main
	0x4c,0x00,0x00,		// JMP endless loop

	// PSID playAddress specific IRQ handler
	0x48,				// PHA
	0x8A,				// TXA
	0x48,				// PHA
	0x98,				// TYA
	0x48,				// PHA
	0xA5,0x01,			// LDA $01
	0x48,				// PHA
	0xA9,0xFF,			// LDA #$FF
	0x85,0x01,			// STA $01		PSID bank reset
	0x20,0x00,0x00,		// JSR $0000	playAddress
	0x68,				// PLA
	0x85,0x01,			// STA $01		restore whatever INIT had setup.. not really needed
	0xEE,0x19,0xD0,		// INC $D019
	0xAD,0x0D,0xDC,		// LDA $DC0D	ACKN whatever IRQ might have been used
	0x68,				// PLA
	0xA8,				// TAY
	0x68,				// PLA
	0xAA,				// TAX
	0x68,				// PLA
	0x40,				// RTI
};

void allocIO() {
	// this array cannot be allocated statically if the pointer is to be used
	// directly from other compilation units.. bloody C crap..
	if (_io_area == 0) {
		_io_area = (uint8_t*) calloc(1, IO_AREA_SIZE);
	}
}

#ifdef TEST
void memInitTest() {
	allocIO();

	// environment needed for Wolfgang Lorenz's test suite
    memset(&_kernal_rom[0], 0x00, KERNAL_SIZE);					// BRK by default

	memset(&_kernal_rom[0x0a31], 0xea, 0x4d);				// $ea31 fill some NOPs
    memcpy(&_kernal_rom[0x0a75], _irq_handler_EA75, 18);	// $ea31 return sequence with added 0xa2 increment to sim time of day (see P_A_S_S_Demo_3.sid)

	// this is actuallly used by "oneshot" test
    memcpy(&_kernal_rom[0x0e8e], _serial_clock_hi_EE8E, 9);			// $ee8e set serial clock line high

    memcpy(&_kernal_rom[0x1d15], _irq_restore_vectors_FD15, 27);	// $fd15 restore I/O vectors
    memcpy(&_kernal_rom[0x1da3], _init_io_FDA3, 86);				// $fda3 initaliase I/O devices
    memcpy(&_kernal_rom[0x1f6e], _schedule_ta_FF6E, 18);			// $ff6e scheduling TA

    memcpy(&_kernal_rom[0x1e43], _nmi_handler_FE43, 5);		// $fe43 nmi handler
    memcpy(&_kernal_rom[0x1ebc], _irq_end_handler_FEBC, 6);	// $febc irq return sequence (e.g. used by Contact_Us_tune_2)

    memcpy(&_kernal_rom[0x1f48], _irq_handler_test_FF48, 19);	// $ff48 irq routine


	// the mmufetch test relies on the below BASIC & KERNAL ROM snippets
	// to switch back the bank setting.. it is only interested in the
	// one operation that is used to switch back the setting at $01
	_basic_rom[0x04E1] = 0x91;		// A4E1   91 24      STA ($24),Y
	_basic_rom[0x04E2] = 0x24;
	_basic_rom[0x182A] = 0x91;		// B82A   91 14      STA ($14),Y
	_basic_rom[0x182B] = 0x14;
	// already in _irq_handler_EA75:
//	_kernal_rom[0x0a79] = 0x85;		// EA79   85 01      STA $01
//	_kernal_rom[0x0a7a] = 0x01;
	_kernal_rom[0x1d27] = 0x91;		// FD27   91 C3      STA ($C3),Y
	_kernal_rom[0x1d28] = 0xc3;
	_char_rom[0x0402] = 0x91;		// D402   91 91      STA ($91),Y
	_char_rom[0x0403] = 0xc3;

	_kernal_rom[0x1ffa] = 0x43;	// NMI vector	(see "icr_01" test)
	_kernal_rom[0x1ffb] = 0xfe;
	_kernal_rom[0x1ffe] = 0x48;	// IRQ vector
	_kernal_rom[0x1fff] = 0xff;

	memResetRAM(0, 0);
//    memset(&_memory[0], 0x0, MEMORY_SIZE);

	_memory[0xa003] = 0x80;
	_memory[0x01fe] = 0xff;
	_memory[0x01ff] = 0x7f;

	// put trap instructions at $FFD2 (PRINT), $E16F (LOAD), $FFE4 (KEY), $8000 and $A474	(EXIT)
	// => memset 0 took care of those

	setMemBank(0x37);

	// set cursor position for PETSCII output
	_memory[0x00d3] = 0x1;	// X
	_memory[0x00d6] = 0x0;	// Y


    memcpy(&_memory[0xe000], _kernal_rom, KERNAL_SIZE);	// just in case there is a banking issue
}
#endif

void memResetBasicROM(uint8_t* rom) {
	if (rom) {
		// optional: for those that bring their own ROM
		memcpy(_basic_rom, rom, BASIC_SIZE);

		memWriteRAM(0x1, 0x37);	// todo: better move to memRsidInit?
	} else {
		memset(&_basic_rom[0], 0x60, BASIC_SIZE);			// RTS by default
	}
}

void memResetCharROM(uint8_t* rom) {
	if (rom) {
		// optional: for those that bring their own ROM
		memcpy(_char_rom, rom, IO_AREA_SIZE);
	}
}

void memResetKernelROM(uint8_t* rom) {
	allocIO();	// todo place properly

	if (rom) {
		// optional: for those that bring their own ROM (precondition for BASIC songs)
		memcpy(_kernal_rom, rom, KERNAL_SIZE);

		// note: RESET routine at E394 originally performs;
			// e453	init 0300 vectors
			// e3bf initialization of basic
			// e422 print BASIC startup messages

		_kernal_rom[0x039a] = 0x60;	// abort "BASIC start up messages" so as not to enter BASIC idle-loop

	} else {
		// we dont have the complete rom but in order to ensure consistent
		// stack handling (regardless of which vector the sid-program is
		// using) we provide dummy versions of the respective standard
		// IRQ/NMI routines..

		// use RTS as default ROM content: some songs actually try to call stuff, e.g. mountain march.sid,
		// Soundking_V1.sid(basic rom init routines: 0x1D50, 0x1D15, 0x1F5E), Voodoo_People_part_1.sid (0x1F81)
		memset(&_kernal_rom[0], 0x60, KERNAL_SIZE);			// RTS by default

		_kernal_rom[0x113e] = 0xa9;		// LDA #$0   "get a character" testcase: Oisac.sid
		_kernal_rom[0x113f] = 0x00;


		memset(&_kernal_rom[0x0a31], 0xea, 0x4d);			// $ea31 fill some NOPs		(limitation?: nops may take longer than original..)
		memcpy(&_kernal_rom[0x0a75], _irq_handler_EA75, 18);	// $ea31 return sequence with added 0xa2 increment to sim time of day (see P_A_S_S_Demo_3.sid)
		memcpy(&_kernal_rom[0x0eb3], _delay_handler_EEB3, 8);	// delay 1 milli

		memcpy(&_kernal_rom[0x1e43], _nmi_handler_FE43, 5);	// $fe43 nmi handler; limitation?: RTI as handler may not be enough.. better add the DD0D handling too?
		memcpy(&_kernal_rom[0x1ebc], _irq_end_handler_FEBC, 6);	// $febc irq return sequence (e.g. used by Contact_Us_tune_2)

		memcpy(&_kernal_rom[0x1f48], _irq_handler_FF48, 19);	// $ff48 irq routine

		_kernal_rom[0x1fe4] = 0x6c;	// JMP ($032A)   ; (F13E) get a character testcase: Oisac.sid
		_kernal_rom[0x1fe5] = 0x2a;
		_kernal_rom[0x1fe6] = 0x03;


		_kernal_rom[0x1ffe] = 0x48;
		_kernal_rom[0x1fff] = 0xff;

		_kernal_rom[0x1ffa] = 0x43;	// standard NMI vectors (this will point into the void at: 0318/19)
		_kernal_rom[0x1ffb] = 0xfe;
	}
}

void memSetupBASIC(uint16_t len) {
	uint16_t basic_end = 0x801 + len;

	// after loading the program this actually points to whatever "LOAD" has loaded
	memWriteRAM(0x002d, basic_end & 0xff);	// bullshit doc: "Pointer to beginning of variable area. (End of program plus 1.)"
	memWriteRAM(0x002e, basic_end >> 8);
	memWriteRAM(0x002f, basic_end & 0xff);
	memWriteRAM(0x0030, basic_end >> 8);
	memWriteRAM(0x0031, basic_end & 0xff);
	memWriteRAM(0x0032, basic_end >> 8);
	memWriteRAM(0x00ae, basic_end & 0xff);	// also needed according to Wilfred
	memWriteRAM(0x00af, basic_end >> 8);


	// todo: according to Wilfred 0803/0804 should be copied to these - but I havn't found a valid testcase yet
	memWriteRAM(0x0039, 0x00);				// line number / Direct mode, no BASIC program is being executed.
	memWriteRAM(0x003a, 0xff);

	memWriteRAM(0x0041, 0x00);				// next data item.
	memWriteRAM(0x0042, 0x08);

	memWriteRAM(0x007a, 0x00);				// Pointer to current byte in BASIC program or direct command.
	memWriteRAM(0x007b, 0x08);

	memWriteRAM(0x002b, memReadRAM(0x007a) + 1);	// Pointer to beginning of BASIC area - Default: $0801
	memWriteRAM(0x002c, memReadRAM(0x007b));

	// note: some BASIC songs use the TI variable.. which is automatically updated via
	// the RASTER IRQ but some songs are REALLY slow before they play anything...
}

uint16_t memPsidMain(uint16_t free_space, uint16_t play_addr) {
	memResetBanksPSID(play_addr);
	uint8_t bank = memReadRAM(0x1); // test-case: Madonna_Mix.sid

	if (!free_space) {
#ifdef EMSCRIPTEN
		EM_ASM_({ console.log("FATAL ERROR: no free memory for driver");});
#else
		fprintf(stderr, "FATAL ERROR: no free memory for driver");
#endif
		return 0;
	} else {
	    memcpy(&_memory[free_space], _driverPSID, 33);

		// set JMP addr for endless loop
		_memory[free_space + 1] = free_space & 0xff;
		_memory[free_space + 2] = free_space >> 8;

		if (play_addr) {
			// register IRQ handler
			uint16_t handler = free_space + 3;
			uint16_t handler314 = handler + 5;	// skip initial register php sequence already part of the default handler

			_memory[0x0314] = handler314 & 0xff;
			_memory[0x0315] = handler314 >> 8;

			_memory[0xFFFE] = handler & 0xff;
			_memory[0xFFFF] = handler >> 8;

			// patch-in the bank setting
			_memory[free_space + 12] = bank;

			// patch-in the playAddress
			_memory[free_space + 16] = play_addr & 0xff;
			_memory[free_space + 17] = play_addr >> 8;
		} else {
			// just use the endless loop for main
		}
		return free_space;
	}
}

void memRsidInit(uint16_t free_space, uint16_t *init_addr, uint8_t actual_subsong, uint8_t basic_mode) {
	if (basic_mode) {
		memWriteRAM(0x030c, actual_subsong);
	} else {
		memRsidMain(free_space, init_addr);
	}
}

void memRsidMain(uint16_t free_space, uint16_t *init_addr) {
	// For RSIDs that just RTS from their INIT (relying just on the IRQ/NMI
	// they have started) there should better be something to return to..
	// 6-byte driver (as compared to bigger 33 bytes PSID driver)

	if (!free_space) {
		return;	// no free space anywhere.. that shit better not RTS!
	} else {
		uint16_t loopAddr = free_space + 3;
        
		_memory[free_space] = 0x20;	// JSR
		_memory[free_space + 1] = (*init_addr) & 0xff;
		_memory[free_space + 2] = (*init_addr) >> 8;
		_memory[free_space + 3] = 0x4c;	// JMP
		_memory[free_space + 4] = loopAddr & 0xff;
		_memory[free_space + 5] = loopAddr >> 8;

		(*init_addr) = free_space;
        
	}
}

void memResetRAM(uint8_t is_ntsc, uint8_t is_psid) {
	if (_memory == 0) _memory = (uint8_t*) calloc(1, MEMORY_SIZE);

    memset(&_memory[0], 0x0, MEMORY_SIZE);

	_memory[0x0314] = 0x31;		// standard IRQ vector
	_memory[0x0315] = 0xea;

	_memory[0x032a] = 0x3e;		// "get a character" vector used from FFE4
	_memory[0x032b] = 0xf1;

	// Vager_3.sid
	_memory[0x0091] = 0xff;		// "stop" key not pressed

	// Master_Blaster_intro.sid actually checks this:
	_memory[0x00c5] = _memory[0x00cb] = 0x40;		// no key pressed

	// Dill_Pickles.sid depends on this
	 _memory[0x0000] = 0x2f;		//default: processor port data direction register

	// for our PSID friends who don't know how to properly use memory banks lets mirror the kernal ROM into RAM
	if (is_psid) {
	// seems some idiot RSIDs also benefit from this. test-case: Vicious_SID_2-Escos.sid
		memcpy(&_memory[0xe000], &_kernal_rom[0], 0x2000);
	}
	// see "The SID file environment" though this alone might be rather useless
	// without the added timer tweaks mentioned in that spec? (the MUS player is
	// actually checking this - but also setting it)
	// (https://www.hvsc.c64.org/download/C64Music/DOCUMENTS/SID_file_format.txt)
	_memory[0x02a6] = (!is_ntsc) & 0x1;
}

void memResetIO() {
    memset(&_io_area[0], 0x0, IO_AREA_SIZE);
}
