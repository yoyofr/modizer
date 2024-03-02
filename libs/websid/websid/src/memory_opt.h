/*
* Performance optimization only.
*
* This violates the normal encapsulation of the "memory" impl for the 
* purpose of slightly faster access in timing critical logic.
*
* It should not normally be used and the same functionality is available
* via the regular memFooBar APIs (corresponding to MEM_FOO_BAR).
* 
* Rationale: For very small functions (e.g. getters) the overhead involved
* in a function call seems to be significant as compared to the run time
* of the actual functionality. In this context here, the problem may be somewhat
* WebAssemby specific: C normally provides the "inline" directive however
* for some reason code seems to break completely when it is used for WebAssembly
* compilation (i.e. "inline" cannot be used here). Also function call overhead
* may be higher than usual: It might be a contributing factor that some 
* WebAssembly engines seem to perform stack consistency checks before every
* function call (Chrome seems to be worse affected than Firefox). I similarly 
* replaced respective small functions that are critical during single-cycle-
* clocking in other parts of the implementation (see cia and cpu). Whithout 
* and functional changes this refactoring alone lead to a decrease of run time 
* by 10% (in my preliminary tests). 
*
* CAUTION: With the big swings of response-times caused by whatever else
* might be happening on the CPU and/or in the browser, the relatively small
* benefits of inlining are hard to quantify reliably. It may always be that some
* supposed speedup was just an "accidental" pause in browser GC activity.
* I will use the below macros for now - such that a simple search/replace 
* can be used in case I might drop the idea at a later stage.  Use 
* OPT_USE_INLINE_ACCESS define to toggle inlining.
*
* <p>WebSid (c) 2020 Jürgen Wothke
* <p>version 0.94
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
 
#ifndef WEBSID_MEM_OPT_H
#define WEBSID_MEM_OPT_H

#include "base.h"


#ifdef __cplusplus
extern "C" {
#endif

// THESE MUST NOT BE USED DIRECTLY!
extern void memSetIO(uint16_t addr, uint8_t value);

#ifdef __cplusplus
}
#endif


#ifdef OPT_USE_INLINE_ACCESS

#ifdef __cplusplus
extern "C" {
#endif

// THESE MUST NOT BE USED DIRECTLY!
extern uint8_t* _io_area;		
extern uint8_t* _memory;

#ifdef __cplusplus
}
#endif

#define	MEM_READ_RAM(addr)\
	_memory[addr]
	
#define	MEM_WRITE_RAM(addr, value)\
	_memory[addr] = value;

#define	MEM_READ_IO(addr)\
	_io_area[(addr) - 0xd000]
	
#define	MEM_WRITE_IO(addr, value)\
	_io_area[(addr) - 0xd000] = value

#define	MEM_SET_IO(addr, value)\
	memSetIO(addr, value)
	
// Note: I had also tried to avoid additional memSet/memGet calls for RAM reads and
// non IO writes by inlining the respective "if MEM_READ_RAM/MEM_WRITE_RAM" parts 
// directly. However that did not seem have any consistent performance impact -
// though it had very negative impact on code structure / added pitfalls since
// expressions used as function params are potentially evaluated repeatedly in the 
// macro context. That idea can therefore be savely abandonned.

#else
	
#define	MEM_READ_RAM(addr)\
	memReadRAM(addr)
	
#define	MEM_WRITE_RAM(addr, value)\
	memWriteRAM((addr), value)

#define	MEM_READ_IO(addr)\
	memReadIO((addr))
	
#define	MEM_WRITE_IO(addr, value)\
	memWriteIO((addr), value)

#define	MEM_SET_IO(addr, value)\
	memSetIO((addr), value)
	
#endif
	
#endif