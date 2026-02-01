/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2024 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include "gblfsr.h"
#include "test.h"

#define FB_6		(1 << 6)
#define MASK_FULL	((1 << 15) - 1)
#define MASK_NARROW	((1 << 7) - 1)

void gblfsr_reset(struct gblfsr* gblfsr) {
	gblfsr->lfsr = MASK_FULL;
	gblfsr->narrow = false;
}

void gblfsr_trigger(struct gblfsr* gblfsr) {
	gblfsr->lfsr = MASK_FULL;
}

void gblfsr_set_narrow(struct gblfsr* gblfsr, bool narrow) {
	gblfsr->narrow = narrow;
}

int gblfsr_next_value(struct gblfsr* gblfsr) {
	uint32_t val = gblfsr->lfsr;
	uint32_t shifted = val >> 1;
	uint32_t xor_out = (val ^ shifted) & 1; /* TAP_0 ^ TAP_1 */
	uint32_t wide = shifted | (xor_out << 14); /* FB_14 */
	uint32_t narrow = (wide & ~FB_6) | (xor_out * FB_6);
	uint32_t new = gblfsr->narrow ? narrow : wide;
	gblfsr->lfsr = new;
	return new & 1;
}

test void test_lsfr(void)
{
	struct gblfsr state;
	uint32_t x = 0;
	int n;

	/* Full period should be 32767 */
	gblfsr_reset(&state);
	n = 0;
	do {
		gblfsr_next_value(&state);
		n++;
	} while ((state.lfsr & MASK_FULL) != MASK_FULL && n < 99999);
	ASSERT_EQUAL("%d", n, 32767);

	/* Narrow period should be 127 */
	gblfsr_reset(&state);
	gblfsr_set_narrow(&state, true);
	n = 0;
	do {
		gblfsr_next_value(&state);
		n++;
	} while ((state.lfsr & MASK_NARROW) != MASK_NARROW && n < 999);
	ASSERT_EQUAL("%d", n, 127);

	/* Test first 32 full output bits from reset */
	gblfsr_reset(&state);
	for (n = 0; n < 32; n++) {
		x <<= 1;
		x |= gblfsr_next_value(&state);
	}
	ASSERT_EQUAL("%08x", x, 0xfffc0008);

	/* Test first 32 narrow output bits from reset */
	gblfsr_reset(&state);
	gblfsr_set_narrow(&state, true);
	for (n = 0; n < 32; n++) {
		x <<= 1;
		x |= gblfsr_next_value(&state);
	}
	ASSERT_EQUAL("%08x", x, 0xfc0830a3);

	/* Test output bits after switching from narrow to long without reset */
	gblfsr_set_narrow(&state, false);
	for (n = 0; n < 32; n++) {
		x <<= 1;
		x |= gblfsr_next_value(&state);
	}
	ASSERT_EQUAL("%08x", x, 0xcbc8b8b3);
}
TEST(test_lsfr);
TEST_EOF;
