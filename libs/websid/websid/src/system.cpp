/*
* System level emulation stuff.
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.94
*
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

extern "C" {
#include "system.h"
#include "cpu.h"
#include "vic.h"
#include "cia.h"
}
#include "sid.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#include <cstdio>
#endif

// if PSID 'init' subroutine takes longer than 4 secs then something
// is wrong (mb in endless loop) test case: PSID "ALiH" type players
// (e.g. Simulcra.sid) use more than 2M cycles in their INIT

#define CYCLELIMIT 4000000

// ----------- system clock -----------

uint32_t _cycles = 0;		// counter of elapsed cycles

extern "C" void sysReset() {
	_cycles = 0;
}

extern "C" uint32_t sysCycles() {
	return _cycles;
}

extern "C" uint8_t sysClockTimeout() {
	cpuClock();
	
	if (sysCycles() >= CYCLELIMIT ) {
#ifdef EMSCRIPTEN
		EM_ASM_({ console.log('ERROR: PSID INIT hangs');});	// less mem than inclusion of fprintf
#else
		fprintf(stderr, "ERROR: PSID INIT hangs\n");
#endif
		return 0;
	}
	// this is probably overkill for PSID crap..
	vicClock(); 
	ciaClock(); 
	SID::clockAll();
	
	_cycles += 1;
	return 1;
}

extern "C" void sysClockOpt() {
	vicClock();
	ciaClock();
	if (SID::isAudible()) {
		SID::clockAll(); 
	}
	cpuClock();
	
	_cycles += 1;
}

extern "C" void sysClock() {
	vicClock();
	ciaClock();
	SID::clockAll();
	cpuClock();	
	
	_cycles += 1;
}

extern "C" uint32_t sysGetClockRate(uint8_t is_ntsc) {
	// note: on the real HW the system clock originates from
	// VIC chip (see comments in vic.c)
	
	if(is_ntsc) {
		return 1022727;	// NTSC system clock (14.31818MHz/14)
	} else {
		return 985249;	// PAL system clock (17.734475MHz/18)
	}
}

#ifdef TEST
extern "C" uint8_t sysClockTest() {
	sysClock();
	
	return cpuIsValidPcPSID();
}
#endif

