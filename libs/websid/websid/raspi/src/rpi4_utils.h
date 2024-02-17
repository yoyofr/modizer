/**
* Provides various Raspberry Pi related utilities.
*
* WebSid (c) 2021 Jürgen Wothke
* version 1.0
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef RPI4_SYS_UTILS_H
#define RPI4_SYS_UTILS_H

#include <stdint.h>

// ---------- system timer related -------------
// DO NOT USE BELOW REG DIRECTLY!
extern volatile uint32_t *timer_regs;

/**
* Setup that must be performed before micros can be used.
*/
void systemTimerSetup();
void systemTimerTeardown();

/**
* Gets the low 32-bits of the CPU's mirco second counter.
*
* NOTICE: The address hardcoded in the impl would need to be adapted 
* to make this work in other Raspberry devices!
*
* Since this is used in timing critical polling loop it is 
* implemented as light-weight as possible.
*/
#define micros() timer_regs[1]		/* the low 32-bits of the system counter */

// ---------- exception handling related --------------
typedef void (*callback_function)(int);
void installSigHandler(callback_function callback);

// ---------- performance tuning related --------------
void scheduleRT();
void migrateThreadToCore(int idx);

/**
* Force maximum CPU clock rate on core #0 & core #3.
*/
void startHyperdrive();
/**
* Restore original minimum CPU clock rate on core #0 & core #3.
*/
void stopHyperdrive();


#endif