/*
* Poor man's emulation of the C64's VIC-II (Video-Interface-Chip).
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
*
* Different system versions:
*   PAL:  312 rasters with 63 cycles, system clock: 985248Hz,
*         frame rate: 50.124542Hz,  19656 cycles per frame
*   NTSC: 263 rasters with 65 cyles,  system clock: 1022727Hz,
*         frame rate: 59.8260895Hz, 17095 cycles per frame
*
* 200 visible lines/25 text lines..	
*
* trivia: "Normally the VIC always uses the 1st phase of each
* clock cycle (for bus access) while cpu uses the 2nd phase.
* During badline cycles VIC may completely take over 40-43 cycles
* and stun the CPU. Technically the CPU is told on its "RDY" pin
* (HIGH) if everything runs normal or if a badline mode is about
* to start. (The CPU then may still complete its write ops - for
* a maximum of 3 cycles before it pauses to let VIC take over the
* bus.) Within an affected raster line the "stun phase" starts at
* cycle 12 and lasts until cycle 55."
*
* useful links:
*
*  - Christian Bauer's: "The MOS 6567/6569 video controller
*    (VIC-II) and its application in the Commodore 64"
*
* LIMITATIONS: Enabled sprites (see D015) normally cause the same
*              kind of "CPU stun" effect as the "bad line" but this
*              effect has not been implemented here (it does not
*              seem to be relevant for most songs).
*
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include "vic.h"

#include "memory.h"
#include "memory_opt.h"
#include "cpu.h"


#include "system.h"	// only needed for PSID optimization


static double _fps;
static uint8_t _cycles_per_raster;
static uint16_t _lines_per_screen;

static uint8_t _x;	// in cycles
static uint16_t _y;	// in rasters

// performance optimization PSID
static uint32_t _cycles_per_screen;
static uint32_t _cycles_next_irq_PSID;

static uint8_t _signal_irq;	// redundant to (memReadIO(0xd019) & 0x80)

static uint8_t _badline_den;

static uint32_t _raster_latch;	// optimization: D012 + D011-bit 8 combined

static uint8_t (*_stunFunc)(uint8_t x, uint16_t y, uint8_t cpr);

// default impl
static uint8_t intBadlineStun(uint8_t x, uint16_t y, uint8_t cpr) {
	if (_badline_den) {	
		// note: caching the yscroll related state (see &0x7 below) actually makes
		// songs like Spijkerhoek 3 run 10% slower! bad idea!
		
		if ((y >= 0x30) && (y <= 0xf7) && ((MEM_READ_IO(0xd011) & 0x7) == (y & 0x7))) {
			if ((x >= 11) && (x <= 53)) {
				if ((x >= 14)) {
					return 2;	// stun completely
				} else {
					return 1;	// stun on read
				}
			}
		}
	}
	return 0;
}

void vicSetStunImpl(uint8_t (*f)(uint8_t x, uint16_t y, uint8_t cpr)) {
	_stunFunc= f;
}
uint8_t (*vicGetStunImpl(void))(uint8_t, uint16_t, uint8_t) { // crappy C syntax
	return _stunFunc;
}

double vicFramesPerSecond() {
	return _fps;
}

void vicSetModel(uint8_t ntsc_mode) {
	// emulation only supports new PAL & NTSC model (none of 
	// the special models)
		
//	_x = _y = 0; redundant see below
	
	// note: clocking is derived from the targetted video standard.
	// 14.31818MHz (NTSC) respectively of 17.734475MHz (PAL) -
	// divide by 4 for respective color subcarrier: 3.58Mhz NTSC,
	// 4.43MHz PAL. the 8.18MHz (NTSC) respectively 7.88MHz (PAL)
	// "dot clock" is derived from the above system clock is finally
	// "dot clock" divided by 8
	
	if(ntsc_mode) {
		// NTSC
		_fps = 59.8260895;
		_cycles_per_raster = 65;
		_lines_per_screen = 263;	// with  520 pixels
	} else {
		// PAL
		_fps = 50.124542;
		_cycles_per_raster = 63;	
		_lines_per_screen = 312;	// with 504 pixels
	}
	_cycles_per_screen = _cycles_per_raster * _lines_per_screen; // PSID performance opt

	// init to very end so that next clock will create a raster 0 IRQ...
	_x = _cycles_per_raster - 1;
	_y = _lines_per_screen - 1;
	
	// clocks per frame: NTSC: 17095 - PAL: 19656		
}

#define CHECK_IRQ() \
	/* "The test for reaching the interrupt raster line is done in */ \
	/* cycle 0 of every line (for line 0, in cycle 1)."	*/ \
	 \
	if (_y == _raster_latch) { \
		/* always signal (test case: Wally Beben songs that use */ \
		/* CIA 1 timer for IRQ but check for this flag) */ \
		uint8_t latch = MEM_READ_IO(0xd019) | 0x1; \
		 \
		uint8_t interrupt_enable = MEM_READ_IO(0xd01a) & 0x1; \
		if (interrupt_enable) { \
			_signal_irq = 0x80; \
			latch |= 0x80;	/* signal VIC interrupt */  \
		} \
		 \
		MEM_WRITE_IO(0xd019, latch); \
	}


// vicClock function pointer
void (*vicClock)();
	
void vicClockRSID() {
	_x += 1;
	
	if ((_x == 1) && !_y) {	// special case: in line 0 it is cycle 1		
		CHECK_IRQ();
		
		_badline_den = MEM_READ_IO(0xd011) & 0x10;	// default for new frame
		
	} else if (_x >= _cycles_per_raster) {
		_x = 0;
		_y += 1;

		if (_y >= _lines_per_screen) {
			_y = 0;			
		}
				
		if (_y) { CHECK_IRQ(); }	// normal case: check in cycle 0				
	}
}

// PSID performance optimization: disable what isn't used anyway
// see Pipes, Eye_of_tiger: ca. 7% faster 
void vicClockDisabledPSID() {
	// completely removing the vicClock() call by using a dedicated
	// play loop in system might save some more cycles but I'd rather
	// limit the number of places that use PSID special handling..
}
void vicClockPSID() {
	// still more expensive than the old emulator since it is checked
	// every cycle.. but cheaper than correct handling.. as long as 
	// PSID does no D012 or D011 polling, this should be OK
	if (SYS_CYCLES() >= _cycles_next_irq_PSID) {
		_cycles_next_irq_PSID = SYS_CYCLES() + _cycles_per_screen;
		
		_signal_irq = 0x80;
		MEM_WRITE_IO(0xd019, 0x81);
	}
}

uint8_t vicIRQ() {
	return _signal_irq; // memReadIO(0xd019) & 0x80;
}

/*
 "A Bad Line Condition is given at any arbitrary clock cycle, if at
 the negative edge of ø0 at the beginning of the cycle RASTER >= $30
 and RASTER <= $f7 and the lower three bits of RASTER are equal to
 YSCROLL and if the DEN (display enable: $d011, bit 4) bit was set
 during an arbitrary cycle of raster line $30."

 During cycles 15-54(incl) the CPU is *always* completely blocked -
 BUT it may be blocked up to 3 cycles earlier depending on the first
 used sub-instruction "read-access", i.e. from cycle 12 the CPU is
 stunned at its first "read-access" (i.e. the write-access of current
 OP is allowed to complete.. usually at the end of OP - max 3
 consecutive "write" cycles in any 6502 OP)
 
 => this means that "OPs in progress" might be stunned in the middle
 of the execution, i.e. OP has just been started and then is stunned
 before it can read the data that is needs.

 KNOWN LIMITATION: 
 
 "displayed sprites" cause a similar effect of stunning the CPU -
 "stealing" ~2 cycles for one sprite and up to ~19 cycles for all 8
 sprites (if they are shown on the specific line). As for the
 "badline" there is a 3 cycle "stun" period (during which more or less
 cycles are lost depending on the current OP) before the bus is
 completely blocked for the CPU. For more details see "Missing Cycles"
 by Pasi 'Albert' Ojala & "The MOS 6567/6569 video controller (VIC-II)
 and its application in the Commodore 64" by Christian Bauer.

 Few songs actually depend on respective sprite timing and it isn't
 implemented here: The limited benefits do not seem to be worth the 
 extra implementation and runtime cost. examples that actually 
 depend on it: Vicious_SID_2-15638Hz.sid, Fantasmolytic_tune_2).
*/
uint8_t vicStunCPU() {
	return _stunFunc(_x, _y, _cycles_per_raster);		
}

static void cacheRasterLatch() {
	_raster_latch = memReadIO(0xd012) + (((uint16_t)memReadIO(0xd011) & 0x80) << 1);
}

void vicReset(uint8_t is_rsid, uint8_t ntsc_mode) {
	
	vicClock = &vicClockRSID;	// default
	_stunFunc = &intBadlineStun;
	
	vicSetModel(ntsc_mode); 
	
	// by default C64 is configured with CIA1 timer / not raster irq
			
	// presumable "the machine" has been running for more than 
	// one frame before the prog is run (raster has already fired)
	memWriteIO(0xd019, 0x01);
	_signal_irq= 0;
	
	memWriteIO(0xd01a, 0x00); 	// raster irq not active
	
	if (is_rsid) {
		// RASTER default to 0x137 (see real HW) 
		memWriteIO(0xd011, 0x9B);
		memWriteIO(0xd012, 0x37); 	// raster at line x
	} else {
		memWriteIO(0xd011, 0x1B);
		memWriteIO(0xd012, 0x0); 	// raster must be below 0x100
		
		_cycles_next_irq_PSID = 0; // trigger it right away
	}
	_badline_den = 1;	// see d011-defaults above
	
	cacheRasterLatch();
}

void vicSetDefaultsPSID(uint8_t timerDrivenPSID) {
	// NOTE: braindead SID File specs apparently allow PSID INIT to
	// specifically DISABLE the IRQ trigger that their PLAY depends
	// on (actually another one of those UNDEFINED features - that
	// "need not to be documented")
		
	if (timerDrivenPSID) {
//		memWriteIO(0xd019, 0x81);	// no need since not active before
		vicClock = &vicClockDisabledPSID;
	} else {		
		memWriteIO(0xd01a, 0x81);	// enable RASTER IRQ
		vicClock = &vicClockPSID;
	}
}

// ------------------------  VIC I/O --------------------------------

static void writeD019(uint8_t value) {
	// ackn vic interrupt, i.e. a setting a bit actually clears it
	
	// note: :some players use "INC D019" (etc) to ackn the interrupt
	// (all Read-Modify-Write instructions write the original value
	// before they write the new value, i.e. the intermediate write
	// does the clearing.. - see cpu.c emulation)
	
	// "The bit 7 in the latch $d019 reflects the inverted state of
	// the IRQ output of the VIC.", i.e. if the source conditions
	// clear, so does the overall output.
	
	// test-case: some songs only clear the "RASTER" but not the "IRQ"
	// flag (e.g. Ozone.sid)
		
	uint8_t v =  memReadIO(0xd019);
	v = v & (~value);					// clear (source) flags directly
	
	if (!(v & 0xf)) {
		v &= 0x7f; 	// all sources are gone: IRQ flag should also be cleared
	}
	
	_signal_irq = v & 0x80;	
	memWriteIO(0xd019, v);
}

static void writeD01A(uint8_t value) {	
	memWriteIO(0xd01A, value);
		
	if (value & 0x1) {
		// check if RASTER condition has already fired previously
		uint8_t d = memReadIO(0xd019);
		if (d & 0x1) {
			_signal_irq = 0x80;	
			memWriteIO(0xd019, d | 0x80); 	// signal VIC interrupt
		}
	}
}

void vicWriteMem(uint16_t addr, uint8_t value) {
	switch (addr) {
		case 0xd011: {
			const uint8_t new_den = value & 0x10;
			
			// badlineCondition: "..if the DEN bit was set during an
			// arbitrary cycle of raster line $30 [for at least one cycle]"
			
			if (_y < 0x2f) {			// our 1st line is 0
				_badline_den = new_den;
			
			} else if (_y == 0x2f) {
				// theoretically the "flag" could still be cleared in the
				// very 1st cycle of the line (ignore this special case)
				_badline_den |= new_den;
			}
			memWriteIO(addr, value);
			cacheRasterLatch();
		}
		case 0xd012:
			memWriteIO(addr, value);
			cacheRasterLatch();
			break;
		case 0xd019:
			writeD019(value);
			break;
		case 0xd01A:
			writeD01A(value);
			break;
		default:
			memWriteIO(addr, value);
	}
}

uint8_t vicReadMem(uint16_t addr) {
	switch (addr) {
		case 0xd011:
			return (memReadIO(0xd011) & 0x7f) | ((_y & 0x100) >> 1);
		case 0xd012:					
			return  _y & 0xff;
		case 0xd019:
			return memReadIO(0xd019);
	}
	return memReadIO(addr);
}