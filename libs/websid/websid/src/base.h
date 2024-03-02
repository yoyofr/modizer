#ifndef WEBSID_BASE_H
#define WEBSID_BASE_H

typedef signed char int8_t;
typedef unsigned char uint8_t;

// 16-bit must be exactly 16-bit!
typedef short int16_t;
typedef unsigned short uint16_t;

#ifndef EMSCRIPTEN
#include <sys/types.h>
typedef __uint32_t uint32_t;

#else
typedef signed int int32_t;
typedef unsigned int uint32_t;
#endif


#define OPT_USE_INLINE_ACCESS

//#define DEBUG

/*
* turn on trace output to debug ADSR handling of a specific voice: 
* debug output will be printed to console and audio output will be
* limited to the specified voice and the specified frame-range 
*/

#define PSID_DEBUG_VOICE 1
#define PSID_DEBUG_FRAME_START 10
#define PSID_DEBUG_FRAME_END 30

#define MAX_SIDS 			10

/*
* diagnostic information for GUI use
*/
typedef enum {
	DigiNone = 0,
	DigiD418 = 1,			// legacy 4-bit approach
	DigiMahoneyD418 = 2,	// Mahoney's "8-bit" D418 samples
	
	// all the filterable digi types
	DigiFM = 3,				// 8-bit frequency modulation; e.g. Vicious_SID_2-15638Hz.sid, LMan - Vortex.sid, etc
	DigiPWM = 4,			// older PWM impls, e.g. Bla_Bla.sid, Bouncy_Balls_RCA_Intro.sid
	DigiPWMTest = 5			// new test-bit based PWM; e.g. Wonderland_XII-Digi_part_1.sid, GhostOrGoblin.sid, etc
} DigiType;

#define MAX_SIDS 10			// might eventually need to be increased 


#endif
