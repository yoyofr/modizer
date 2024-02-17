/*
* Emulates MOS Technology 6510 CPU - as far as needed to play RSID files.
* 
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
 
#ifndef WEBSID_CPU_H
#define WEBSID_CPU_H

#include "base.h"


// setup
void		cpuInit(uint8_t is_rsid);
void 		cpuSetProgramCounter(uint16_t pc, uint8_t a);

extern void (*cpuClock)();		// cpuClock function pointer (crappy C requires different syntax here)

// PSID only crap
uint8_t		cpuIsValidPcPSID();
void		cpuSetProgramCounterPSID(uint16_t pc);
void		cpuIrqFlagPSID(uint8_t on);

#ifdef DEBUG
uint16_t	cpuGetPC();
uint8_t		cpuGetSP();
#endif


void		cpuHackNMI(uint8_t on);

#endif