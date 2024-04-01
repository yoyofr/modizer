/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2024 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _GBLFSR_H_
#define _GBLFSR_H_

#include <inttypes.h>
#include <stdbool.h>

#include "common.h"

struct gblfsr {
	uint16_t lfsr;
	bool narrow;
};

void gblfsr_reset(struct gblfsr* gblfsr);
void gblfsr_trigger(struct gblfsr* gblfsr);
void gblfsr_set_narrow(struct gblfsr* gblfsr, bool narrow);
int gblfsr_next_value(struct gblfsr* gblfsr);

#endif
