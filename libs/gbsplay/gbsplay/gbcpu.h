/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _GBCPU_H_
#define _GBCPU_H_

#include <inttypes.h>

#include "common.h"

#define ZF	0x80
#define NF	0x40
#define HF	0x20
#define CF	0x10

#define BC	0
#define DE	1
#define HL	2
#define AF	3
#define SP	4
#define GBS_PC	5

#define DEBUG 0

#if DEBUG == 1

#define DPRINTF(...) printf(__VA_ARGS__)
#define DEB(x) x
#define OPINFO(name, fn, cycles_1, cycles_2) {name, fn, cycles_1, cycles_2}
#define CYCLES1(op) (op->cycles_1)
#define CYCLES2(op) (op->cycles_2)
#define CYCLES_OK(op, cycles) \
	(op->cycles_1 == 0 || cycles == op->cycles_1 || cycles == op->cycles_2)

#else

/*
static inline void foo(void)
{
}


#define DPRINTF(...) foo()
#define DEB(x) foo()
*/
#define DPRINTF(...) do { } while (0)
#define DEB(x)
#define OPINFO(name, fn, cycles_1, cycles_2) {fn}
#define CYCLES1(op) 0
#define CYCLES2(op) 0
#define CYCLES_OK(op, cycles) 1

#endif

#if GBS_BYTE_ORDER == GBS_ORDER_LITTLE_ENDIAN

#define REGS16_R(r, i) (r.rw[i])
#define REGS16_W(r, i, x) (r.rw[i]) = x
#define REGS8_R(r, i) (r.ri[i^1])
#define REGS8_W(r, i, x) (r.ri[i^1]) = x

typedef union {
		uint8_t ri[12];
		uint16_t rw[6];
		struct {
			uint8_t c;
			uint8_t b;
			uint8_t e;
			uint8_t d;
			uint8_t l;
			uint8_t h;
			uint8_t a;
			uint8_t f;
			uint16_t sp;
			uint16_t pc;
		} rn;
} gbcpu_regs_u;

#else

#define REGS16_R(r, i) (r.rw[i])
#define REGS16_W(r, i, x) (r.rw[i]) = x
#define REGS8_R(r, i) (r.ri[i])
#define REGS8_W(r, i, x) (r.ri[i]) = x

typedef union {
		uint8_t ri[12];
		uint16_t rw[6];
		struct {
			uint8_t b;
			uint8_t c;
			uint8_t d;
			uint8_t e;
			uint8_t h;
			uint8_t l;
			uint8_t f;
			uint8_t a;
			uint16_t sp;
			uint16_t pc;
		} rn;
} gbcpu_regs_u;

#endif

typedef void (*gbcpu_put_fn)(void *priv, uint32_t addr, uint8_t val);
typedef uint32_t (*gbcpu_get_fn)(void *priv, uint32_t addr);

struct get_entry {
	void *priv;
	gbcpu_get_fn get;
};

struct put_entry {
	void *priv;
	gbcpu_put_fn put;
};

#define GBCPU_LOOKUP_SIZE 256

struct gbcpu {
	gbcpu_regs_u regs;
	long halt_at_pc;
	long halted;
	long ime;

	long stopped;
	cycles_t cycles;

#if DEBUG == 1
	gbcpu_regs_u oldregs;
#endif
	
	struct get_entry getlookup[GBCPU_LOOKUP_SIZE];
	struct put_entry putlookup[GBCPU_LOOKUP_SIZE];
};

void gbcpu_add_mem(struct gbcpu* const gbcpu, uint32_t start, uint32_t end, gbcpu_put_fn putfn, gbcpu_get_fn getfn, void *priv);
void gbcpu_init(struct gbcpu* const gbcpu);
void gbcpu_init_struct(struct gbcpu* const gbcpu);
long gbcpu_step(struct gbcpu* const gbcpu);
void gbcpu_intr(struct gbcpu* const gbcpu, long vec);
uint8_t gbcpu_mem_get(struct gbcpu* const gbcpu, uint16_t addr);
void gbcpu_mem_put(struct gbcpu* const gbcpu, uint16_t addr, uint8_t val);

#endif
