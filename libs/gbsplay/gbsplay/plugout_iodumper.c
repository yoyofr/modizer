/*
 * gbsplay is a Gameboy sound player
 *
 * 2015-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "plugout.h"

static FILE *file;
static cycles_t cycles_prev = 0;

static long iodumper_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	int fd;

	UNUSED(endian);
	UNUSED(rate);
	UNUSED(buffer_bytes);
	UNUSED(metadata);

	/*
	 * clone and close STDOUT_FILENO
	 * to make sure nobody else can write to stdout
	 */
	fd = dup(STDOUT_FILENO);
	if (fd == -1) return -1;
	(void)close(STDOUT_FILENO);
	file = fdopen(fd, "w");

	return 0;
}

static int iodumper_skip(int subsong)
{
	cycles_prev = 0;
	fprintf(file, "\nsubsong %d\n", subsong);
	fprintf(stderr, "dumping subsong %d\n", subsong);

	return 0;
}

static int iodumper_io(cycles_t cycles, uint32_t addr, uint8_t val)
{
	long cycle_diff = cycles - cycles_prev;

	fprintf(file, "%08lx %04x=%02x\n", cycle_diff, addr, val);
	cycles_prev = cycles;

	return 0;
}

static void iodumper_close(void)
{
	fflush(file);
	fclose(file);
}

const struct output_plugin plugout_iodumper = {
	.name = "iodumper",
	.description = "STDOUT io dumper",
	.open = iodumper_open,
	.skip = iodumper_skip,
	.io = iodumper_io,
	.close = iodumper_close,
	.flags = PLUGOUT_USES_STDOUT,
};
