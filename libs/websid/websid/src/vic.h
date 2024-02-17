/*
* Poor man's emulation of the C64's VIC-II (Video-Interface-Chip).
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_VIC_H
#define WEBSID_VIC_H

#include "base.h"

// setup
void		vicReset(uint8_t is_rsid, uint8_t ntsc_mode);
void		vicSetModel(uint8_t ntsc_mode);
void 		vicSetDefaultsPSID(uint8_t timerDrivenPSID);

// clocking
//void		vicClock();
extern void (*vicClock)();		// vicClock function pointer (crappy C requires different syntax here)

// CPU interactions
uint8_t		vicStunCPU();	// 0: no stun; 1: allow "bus write"; 2: stun
uint8_t		vicIRQ();

// static configuration information
double		vicFramesPerSecond();

// memory access interface (for memory.c)
void		vicWriteMem(uint16_t addr, uint8_t value);
uint8_t		vicReadMem(uint16_t addr);


// hack used to replace default "badline" handling
void		vicSetStunImpl(uint8_t (*f)(uint8_t x, uint16_t y, uint8_t cpr));
uint8_t		(*vicGetStunImpl(void))(uint8_t, uint16_t, uint8_t);

#endif