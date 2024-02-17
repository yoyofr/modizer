/*
* Poor man's emulation of the C64's MOS 6526 CIA (Complex Interface Adapter) timers.
*
* WebSid (c) 2019 Jürgen Wothke
* version 0.93
* 
* Only those features actually observed in RSID files have been implemented,
* i.e. simple cycle counting and timer B to timer A linking.
*
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#ifndef WEBSID_CIA_H
#define WEBSID_CIA_H

#include "base.h"

// init
void 		ciaReset(uint8_t is_rsid, uint8_t is_ntsc);
void 		ciaSetDefaultsPSID(uint8_t is_timer_driven);

// clocking
//void 		ciaClock();
extern void (*ciaClock)();		// ciaClock function pointer (crappy C requires different syntax here)

// CPU interactions
uint8_t 	ciaNMI();
uint8_t 	ciaIRQ();

// memory access interface (for memory.c)
uint8_t 	ciaReadMem(uint16_t addr);
void 		ciaWriteMem(uint16_t addr, uint8_t value);

// hack
void 		ciaUpdateTOD(uint8_t song_speed);

#endif