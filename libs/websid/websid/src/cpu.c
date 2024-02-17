/*
* Emulates MOS Technology 6510 CPU - as far as needed to play RSID files.
*
* This is the main entry point for dealing with everything CPU related.
*
*
* Interactions with external components (VIA, CIA, SID) are emulated on a cycle-
* by-cycle level and allow the correct timing of interrupts (NMI & IRQ) as
* well as the VIC temporarily stunning the CPU (see "badline" handling).
*
* However there may be imprecisions with regard to intra-OP timing (see
* "SUMMARY OF SINGLE CYCLE EXECUTION" in "MOS MCS6500 hardware manual.TXT"),
* e.g. if a 4 cycle OP would actually perform some read in its 3rd cycle it
* is emulated here as if it were to do everything at the end in cycle 4.
*
*
* LIMITATIONS:
*
*  - No real sub-instruction accuracy NOR pipeline (overlapping OPs) handling
*    but some (hopefully) "good enough" approximation. Delays regarding when
*    updated data will be visible on the bus (and to which component) may not
*    be correctly modeled for all special cases.
*
*  - flag handling in BCD mode is not implemented (see
*    http://www.oxyron.de/html/opcodes02.html)
*
*  - unhandled: "When an NMI occurs before clock 4 of a BRKn command, the BRK is
*    finished as a NMI. In this case, BFlag on the stack will not be cleared."
*
*  - BRK instruction handling is not implemented and a BRK is considered to be
*    a non-recoverable error
*
*
* HW clock timing info:
*
* Based on the ø0 clock delivered by VIC, the CPU generates the two output
* clocks ø1 and ø2. The respective phase width and rise/fall times vary slightly
* resulting in something like this:
*
* ø0 ¯¯¯¯¯¯\_______________/¯¯¯¯¯¯¯¯¯\___________
*
* ø1 __________/¯¯¯¯¯¯¯¯¯\_______________/¯¯¯¯¯¯¯
*
* ø2 ¯¯¯¯¯¯¯¯\_______________/¯¯¯¯¯¯¯¯¯\_________
*
* It seems that ø2 is a "delayed" version of the input ø0 whereas ø1 is an
* "inverted version" of ø0 (and ø2). On sub-cycle level ø1 and ø2 are not
* completely in sync. While the CPU is clocked with ø2 all the other
* components are clocked with ø1.
*
* From a simplistic software point of view, the above can be thought of as each
* "system clock" having 2 phases: where the 1st phase is always used by "the
* other components" and the CPU (usually) uses phase 2.
*
*
* useful links/docs:
*
*  http://www.oxyron.de/html/opcodes02.html
*  http://6502.org/tutorials/interrupts.html
*  http://www.zimmers.net/anonftp/pub/cbm/documents/chipdata/64doc
*  http://archive.6502.org/datasheets/mos_6500_mpu_preliminary_may_1976.pdf
*  https://wiki.nesdev.com/w/index.php/CPU_interrupts
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.94
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include <string.h>
#include <stdio.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "cpu.h"
#include "system.h"

// for interrupt handling
#include "vic.h"
#include "cia.h"

#include "memory.h"
#include "memory_opt.h"


// ---- interrupt handling ---

// "Many references will claim that interrupts are polled during the last
// cycle of an instruction, but this is true only when talking about the
// output from the edge and level detectors. As can be deduced from above,
// it's really the status of the interrupt lines at the end of the second-to-last
// cycle that matters." (https://wiki.nesdev.com/w/index.php/CPU_interrupts)


// "When an interrupt occurs 2 or more cycles before the current command
// ends, it is executed immediately after the command. Otherwise, the CPU
// executes the next command first before it calls the interrupt handler.

// The only exception to this rule are "taken branches" to the same page
// which last 3 cycles. Here, the interrupt must have occurred before clock 1
// of the branch command; the normal rule says before clock 2. Branches to a
// different page or branches not taken are behaving normal."

// line "detectors" run in ø2 phase and activate the respective internal
// signal in next ø1, i.e. the next system clock cycle.

// => like so many docs the above claim is closer to the truth but still
// incomplete. There are more special cases involving the FLAG_I...

// how the 6502's pipline affects the handling of the I-flag:

// "The RTI instruction affects IRQ inhibition immediately. If an IRQ is
// pending and an RTI is executed that clears the I flag, the CPU will
// invoke the IRQ handler immediately after RTI finishes executing. This
// is due to RTI restoring the I flag from the stack before polling
// for interrupts.

// The CLI, SEI, and PLP instructions on the other hand change the I flag
// after polling for interrupts (like all two-cycle instructions they poll
// the interrupt lines at the end of the first cycle), meaning they can
// effectively delay an interrupt until after the next instruction. For
// example, if an interrupt is pending and the I flag is currently set,
// executing CLI will execute the next instruction before the CPU invokes
// the IRQ handler."

// as long as the IRQ line stays active new IRQ will trigger as soon
// as the I-flag is cleared ..

// Note on interrupt handling (see http://6502.org/tutorials/interrupts.html):
// an interrupt triggers a 7-clock cycles "virtual OP". An interrupt allows
// the previous OP to complete (it seems reasonable to presume that BRK uses
// the exact same sequence - and JSR does the same just without pushing the
// processor status):
//					2 cycles internal
//					1 cycle push stack: return addr-hi
//					1 cycle push stack: return addr-lo
//					1 cycle push stack: processor status regigster
//				=> BTW: these are the max. 3 consecutive writes that may
//                  occur on a 6502 (see badline handling) - whereas for
//                  the other OPs (except JSR) the writes seem to be at the
//                  end of the OP they are in the middle here..
//					1 cycle pc-lo: get vector
//					1 cycle pc-hi: get vector



// ---- IRQ handling ---

// relevant test suites: "irq", "imr"
// test-case: Humphrey_Bogart.sid, Monster_Museum.sid, Double_Falcon.sid,
// (use SEI in the IRQ handler after the flag already had been set)
// Vaakataso.sid, Vicious_SID_2-Carmina_Burana.sid

// required lead time in cycles before IRQ can trigger
#define IRQ_LEAD_DEFAULT 2
static uint8_t _interrupt_lead_time = IRQ_LEAD_DEFAULT;

static uint8_t _irq_committed = 0;	// CPU is committed to running the IRQ
static uint32_t _irq_line_ts = 0;

// required special handling for SEI operation: on the real hardware the operation
// would block interrupts in its 2nd cycle but not the 1st. And due the special
// sequence of "check 1st then update flag" an IRQ that slips through the SEI
// will even benefit from a shorter 1 cycle lead time, i.e. the IRQ will trigger
// immediately after SEI has completed.

// the special SEI handling here is divided into two parts: 1) the FLAG_I will be
// set immediately during the "prefetch" of the SEI operation. 2) the
// below code then compensates to handle the special scenarios correctly.

// note: the SEI_OP below can only show up in its 2nd cycle due to the way the
// CHECK_FOR_IRQ() is currently performed at the start of each cycle (during the
// 1st cycle _exe_instr_opcode will not yet be set when the check is done.. the
// sei operation is selected after the check and will implicitly
// complete its 1st step during that cycle.. )

typedef enum {
	BLOCKED = 0,
	POTENTIAL_SLIP = 1,
	SLIPPED_SEI = 2
} slip_status_t;

slip_status_t _slip_status;

#define COMMIT_TO_IRQ() \
	if (!_irq_line_ts) { \
		_irq_committed = 1;	/* there will be an IRQ.. but will another op be run 1st? */ \
		_irq_line_ts = SYS_CYCLES();	/* ts when line was activated */ \
	}

// todo: correctly the IRQ check should be performed 1x, at the right moment,
// within each executed command! i.e. the number of checks could be reduced by
// a factor of 2-4! also that would avoid having to do post mortem analysis of
// what may or may not have happened before - hence avoiding all the delay
// calculations used below... it would also automatically handle the case of
// a stunned CPU

// note: on the real HW the respective check happends in ø2 phase of the
// previous CPU cycle and the respective internal interrupt signal then goes
// high in the ø1 phase after (the potential problem of the below impl is that
// by performing the test here, it might incorrectly pick up some CIA change
// that has just happend in the ø1 phase)

#define CHECK_FOR_IRQ() \
	if ( vicIRQ() || ciaIRQ() ) { \
		if (_no_flag_i) { /* this will also let pass the 1st cycle of a SEI */\
			COMMIT_TO_IRQ(); \
		} else if (_exe_instr_opcode == SEI_OP) { \
			/* the IRQ may already have been commited during the 1st cycle of the SEI,
			but this was done without knowledge of the corresponding reduced lead time
			which must be corrected here */\
			if (_irq_committed && (SYS_CYCLES() - _irq_line_ts == 1)) { \
				_slip_status = SLIPPED_SEI; \
				_interrupt_lead_time = 1; \
			} \
			if (!_irq_committed) _irq_line_ts = 0; \
		} else if (!_irq_committed) _irq_line_ts = 0; \
	} else if (!_irq_committed) _irq_line_ts = 0;



// The below check is done at beginning of new cycle after previous op has
// been completed (i.e. _exe_instr_opcode has already been reset)

#define IS_IRQ_PENDING() \
	(_irq_committed ?  \
		( (_no_flag_i || (_slip_status == SLIPPED_SEI)) \
			&& ((SYS_CYCLES() - _irq_line_ts) >= _interrupt_lead_time) )  \
		: 0)


	// ---- NMI handling ---

static uint8_t _nmi_committed = 0;		// CPU is committed to running the NMI
static uint8_t _nmi_line = 0;			// state change detection
static uint32_t _nmi_line_ts = 0;		// for scheduling


// when the CPU detects the "NMI line" activation it "commits" to
// running that NMI handler and no later state change will stop
// that NMI from being executed (see above for scheduling)

// test-case: "ICR01" ("READING ICR=81 MUST PASS NMI"): eventhough
// the NMI line is immediately acknowledged/cleared in the same
// cycle that the CIA sets it, the NMI handler should still be called.
#define CHECK_FOR_NMI() \
	if (ciaNMI() && (_no_nmi_hack || _no_flag_i)) {	/* NMI line is active now */\
	\
		/* NMI is different from IRQ in that only the transition from \
		   high to low signal triggers an NMI, and the line has to be \
		   restored to high (meaning "false" here) before the next NMI \
		   can trigger.*/ \
		\
		if (!_nmi_line) { \
			_nmi_committed = 1;			/* there is no way back now */ \
			_nmi_line = 1;				/* using 1 to model HW line "low" signal */ \
			_nmi_line_ts = SYS_CYCLES(); \
			 \
			/* "If both an NMI and an IRQ are pending at the end of an \
			   instruction, the NMI will be handled and the pending \
			   status of the IRQ forgotten (though it's likely to be \
			   detected again during later polling)." \
			 \
			_irq_committed= 0; \
			_irq_line_ts= 0; */ \
		} else { \
			/* line already/still activated... cannot trigger new  \
			   NMI before previous one has been acknowledged */ \
		} \
	} else { \
		_nmi_line = 0;			/* NMI has been acknowledged */ \
		if (!_nmi_committed) { \
			/* still needed until the committed NMI has been scheduled */ \
			_nmi_line_ts = 0; \
		} \
	}

#define IS_NMI_PENDING()\
	(_nmi_committed ? (SYS_CYCLES() - _nmi_line_ts) >= _interrupt_lead_time : 0)



// pseudo OPs patched into the positions of unsable JAM ops to ease handling:
const static uint8_t START_IRQ_OP = 0x02;	// "sti" pseudo OP for IRQ handling
const static uint8_t START_NMI_OP = 0x12;	// "stn" pseudo OP for NMI handling

const static uint8_t SEI_OP = 0x78;			// regular opcode


// Used to emulate "CPU stun" (by VIC): 0 for OPs that don't
// use "bus writes", else cycle (starting at 1) in which "write"
// is started (with exception of BRK/JSR these are then all the
// remaining steps of the OP)
// note: the relevant OPs are not affected by page boundary
// crossing, i.e. no further adjustments needed.

static const int32_t _opbase_write_cycle[256] = {
	3,0,3,7,0,0,4,4,3,0,0,0,0,0,5,5,
	0,0,3,7,0,0,5,5,0,0,0,6,0,0,6,6,
	4,0,0,7,0,0,4,4,0,0,0,0,0,0,5,5,
	0,0,0,7,0,0,5,5,0,0,0,6,0,0,6,6,
	0,0,0,7,0,0,4,4,3,0,0,0,0,0,5,5,	// fixme RTI stack updates should NOT occur before 4th cycle! (but different NMI/IRQ behavior cannot be done with this simplistic model anyway)
	0,0,0,7,0,0,5,5,0,0,0,6,0,0,6,6,
	0,0,0,7,0,0,4,4,0,0,0,0,0,0,5,5,
	0,0,0,7,0,0,5,5,0,0,0,6,0,0,6,6,
	0,6,0,6,3,3,3,3,0,0,0,0,4,4,4,4,
	0,6,0,0,4,4,4,4,0,5,0,0,0,5,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,7,0,0,4,4,0,0,0,0,0,0,5,5,
	0,0,0,7,0,0,5,5,0,0,0,6,0,0,6,6,
	0,0,0,7,0,0,4,4,0,0,0,0,0,0,5,5,
	0,0,0,7,0,0,5,5,0,0,0,6,0,0,6,6
};



// instruction that is executing in a "cycle-by-cycle manner"
static int16_t _exe_instr_opcode;
static int8_t _exe_instr_cycles;
static int8_t _exe_instr_cycles_remain;
static int8_t _exe_write_trigger;


#include "cpu_operations.inc"	/* prefetchOperation & runPrefetchedOp */

/*
* Notes on VIC "bad lines": VIC may "stun" the CPU for 40 (+up to 3) cycles
* (or more if sprites are involved - which isn't supported here) and this
* may occur right in the middle of some OP. The "stun" starts whenever the
* current OP does its next "bus read" - only consecutive "writes" of the
* current OP are allowed to complete. (Within a 7 cycle BRK OP the code would
* be allowed to complete the 3 push stack operations - but ONLY if the "OP"
* was already past the 2 initial cycles..)
* => see vic.c for more information
*
* To handle this correctly OPs would need to be simulated at a sub-cycle level.
* But in the context of SID music players this is probably unnecessary overkill
* (except for the most hardcore RSID files - but those most likely avoid the
* badlines problem altogether since they cannot afford the 40 cycles gap when
* they want to play hi-fi digi-samples..).
*
* => CPU may run any cycles that DO NOT use the bus.. and there might be
*    special cases that I overlooked
*/
#define CHECK_FOR_VIC_STUN(is_stunned) \
	/* VIC badline handling (i.e. CPU may be paused/stunned) */ \
	uint8_t stun_mode = vicStunCPU();	/* it won't hurt to also STUN the crappy PSID songs */ \
	if (stun_mode) { \
		if ((stun_mode == 2) || (_exe_instr_opcode < 0)) { \
			is_stunned = 1; \
		} else { \
			uint8_t bus_write = _opbase_write_cycle[_exe_instr_opcode]; \
			if (bus_write) { \
				/* this OP may be allowed to still perform "bus write" (if that's the current step): */ \
				int8_t p = _exe_instr_cycles -_exe_instr_cycles_remain; \
				if (p >= bus_write) { \
					/* allow this "write to complete".. see below */ \
					is_stunned = 0; \
				} else { \
					is_stunned = 1; \
				} \
			} else { \
				is_stunned = 1; \
			} \
		} \
	} else { \
		is_stunned = 0; \
	}


// cpuClock function pointer
void (*cpuClock)();

/*
* Simulates what the CPU does within the next system clock cycle.
*/
static void cpuClockRSID() {
	CHECK_FOR_IRQ();	// check 1st (so NMI can overrule if needed)
	CHECK_FOR_NMI();

	uint8_t is_stunned;
	CHECK_FOR_VIC_STUN(is_stunned);		// todo: check if some processing could be saved checking this 1st
	if (is_stunned) return;

	if (_exe_instr_opcode < 0) {	// get next instruction

		if(IS_NMI_PENDING()) {				// has higher prio than IRQ

			_nmi_committed = 0;

			// make that same trigger unusable (interrupt must be
			// acknowledged before a new one can be triggered)
			_nmi_line_ts = 0;

			INIT_OP(START_NMI_OP,_exe_instr_opcode, _exe_instr_cycles, _interrupt_lead_time, _exe_write_trigger);

		} else if (IS_IRQ_PENDING()) {	// interrupts are like a BRK command

			_irq_committed = 0;
			INIT_OP(START_IRQ_OP,_exe_instr_opcode, _exe_instr_cycles, _interrupt_lead_time, _exe_write_trigger);

		} else {
			// default: start execution of next instruction (i.e. determine the "exact" timing)
			prefetchOperation( &_exe_instr_opcode, &_exe_instr_cycles, &_interrupt_lead_time, &_exe_write_trigger);
		}
		// since there are no 1-cycle ops nothing else needs to be done right now
		_exe_instr_cycles_remain =  _exe_instr_cycles - 1;	// we already are in 1st cycle here
	} else {

		// handle "current" instruction
		_exe_instr_cycles_remain--;

		if(_exe_instr_cycles_remain == _exe_write_trigger) {
			// output results of current instruction (may be before op ends)
			runPrefetchedOp();
		}
		if(_exe_instr_cycles_remain == 0) {
			// current operation has been completed.. get something new to do in the next cycle
			_exe_instr_opcode = -1;	// completed current OP
		}
	}
}

static void cpuClockPSID() {
	// optimization: this is a 1:1 copy of the regular cpuClock() with all the
	// NMI handling thrown out (tested songs ran about 5% faster with this optimization)

	CHECK_FOR_IRQ();	// check 1st (so NMI can overrule if needed)

	/* if a PSID depends on badline timing then by definition it MUST be an RSID!
	uint8_t is_stunned;
	CHECK_FOR_VIC_STUN(is_stunned);		// todo: check if some processing could be saved checking this 1st
	if (is_stunned) return;
	*/
	if (_exe_instr_opcode < 0) {	// get next instruction

		if (IS_IRQ_PENDING()) {	// interrupts are like a BRK command

			_irq_committed = 0;
			INIT_OP(START_IRQ_OP,_exe_instr_opcode, _exe_instr_cycles, _interrupt_lead_time, _exe_write_trigger);

		} else {
			// default: start execution of next instruction (i.e. determine the "exact" timing)
			prefetchOperation( &_exe_instr_opcode, &_exe_instr_cycles, &_interrupt_lead_time, &_exe_write_trigger);
		}
		// since there are no 1-cycle ops nothing else needs to be done right now
		_exe_instr_cycles_remain =  _exe_instr_cycles - 1;	// we already are in 1st cycle here
	} else {

		// handle "current" instruction
		_exe_instr_cycles_remain--;

		if(_exe_instr_cycles_remain == _exe_write_trigger) {
			// output results of current instruction (may be before op ends)
			runPrefetchedOp();
		}
		if(_exe_instr_cycles_remain == 0) {
			// current operation has been completed.. get something new to do in the next cycle
			_exe_instr_opcode = -1;	// completed current OP
		}
	}
}

void cpuInit(uint8_t is_rsid) {
	cpuClock = is_rsid ? &cpuClockRSID : &cpuClockPSID;

	cpuStatusInit();

	_exe_instr_cycles = _exe_instr_cycles_remain = _exe_write_trigger = 0;
	_exe_instr_opcode = -1;

	_irq_line_ts = _irq_committed = 0;
	_nmi_line = _nmi_line_ts = _nmi_committed = 0;
}
