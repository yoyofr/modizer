/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "impulsegen.h"

#define IMPULSE_N_SHIFT 7 /* 128 shifted impulses */
#define IMPULSE_W_SHIFT 5 /* 32 samples per impulse */
#define IMPULSE_CUTOFF 1.0 /* Cutoff at nyquist limit (no cutoff) */

#define IMPULSE_WIDTH (1 << IMPULSE_W_SHIFT)
#define IMPULSE_W_MASK (IMPULSE_WIDTH - 1)
#define IMPULSE_N (1 << IMPULSE_N_SHIFT)
#define IMPULSE_N_MASK (IMPULSE_N - 1)

int main(int argc, char **argv)
{
	int32_t *impulsetab = gen_impulsetab(IMPULSE_W_SHIFT, IMPULSE_N_SHIFT, IMPULSE_CUTOFF);
	int n = (1 << IMPULSE_N_SHIFT) * (1 << IMPULSE_W_SHIFT);
	int i;

	if (impulsetab == NULL) {
		fprintf(stderr, "Failed to generate impulse table.");
		return 1;
	}

	printf("#define IMPULSE_N_SHIFT %d\n", IMPULSE_N_SHIFT);
	printf("#define IMPULSE_W_SHIFT %d\n", IMPULSE_W_SHIFT);
	printf("static const int32_t base_impulse[] = {");
	for (i=0; i<n; i++) {
		if ((i & IMPULSE_W_MASK) == 0) {
			printf("\n\t");
		}
		printf("%9d,", impulsetab[i]);
	}
	printf("\n};\n");

	return 0;
}
