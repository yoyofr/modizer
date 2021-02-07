#pragma once

#include <cstdint>

struct memoryMap
{
	uint8_t *address;
	uint32_t mask;
};

union reg_pair
{
	struct
	{
#ifdef WORDS_BIGENDIAN
		uint8_t B3;
		uint8_t B2;
		uint8_t B1;
		uint8_t B0;
#else
		uint8_t B0;
		uint8_t B1;
		uint8_t B2;
		uint8_t B3;
#endif
	} B;
	struct
	{
#ifdef WORDS_BIGENDIAN
		uint16_t W1;
		uint16_t W0;
#else
		uint16_t W0;
		uint16_t W1;
#endif
	} W;
#ifdef WORDS_BIGENDIAN
	volatile uint32_t I;
#else
	uint32_t I;
#endif
};

extern memoryMap map[256];

extern reg_pair reg[45];
extern uint8_t biosProtected[4];

extern bool N_FLAG;
extern bool Z_FLAG;
extern bool C_FLAG;
extern bool V_FLAG;
extern bool armIrqEnable;
extern bool armState;
extern int armMode;

int CPULoadRom();
void CPUUpdateRegister(uint32_t, uint16_t);
void CPUInit();
void CPUReset();
void CPULoop(int);
void CPUCheckDMA(int, int);

enum
{
	R13_IRQ = 18,
	R14_IRQ,
	SPSR_IRQ,
	R13_USR = 26,
	R14_USR,
	R13_SVC,
	R14_SVC,
	SPSR_SVC,
	R13_ABT,
	R14_ABT,
	SPSR_ABT,
	R13_UND,
	R14_UND,
	SPSR_UND,
	R8_FIQ,
	R9_FIQ,
	R10_FIQ,
	R11_FIQ,
	R12_FIQ,
	R13_FIQ,
	R14_FIQ,
	SPSR_FIQ
};

#include "Globals.h"
