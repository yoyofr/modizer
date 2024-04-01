/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _IMPULSEGEN_H_
#define _IMPULSEGEN_H_

#include <inttypes.h>

#define IMPULSE_HEIGHT (double)(1 << 24)

int32_t *gen_impulsetab(long w_shift, long n_shift, double cutoff);

#endif /* _IMPULSEGEN_H_ */
