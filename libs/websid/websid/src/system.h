/*
* System level emulation stuff.
* 
* WebSid (c) 2020 Jürgen Wothke
* version 0.94
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
 
#ifndef WEBSID_SYS_H
#define WEBSID_SYS_H

#include "base.h"

// setup
void 		sysReset();

// system clock related
uint32_t	sysCycles();		// system cycles since starting the song

void 		sysClock();
void		sysClockOpt();
uint8_t		sysClockTimeout();
#ifdef TEST
uint8_t		sysClockTest();
#endif
uint32_t	sysGetClockRate(uint8_t is_ntsc);



// -------------------- performance optimization --------------------------


#ifdef OPT_USE_INLINE_ACCESS
extern uint32_t _cycles; 	// MUST NOT BE USED DIRECTLY

#define SYS_CYCLES() \
	_cycles

#else
	
#define SYS_CYCLES() \
	sysCycles()
#endif	
	
#endif