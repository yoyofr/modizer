/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2020 (C) by Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

long rand_long(long max);
void rand_seed(uint64_t seed);
void shuffle_long(long *array, long elements);
int fpack(FILE *f, const char *fmt, ...);
int fpackat(FILE *f, long offset, const char *fmt, ...);

#endif
