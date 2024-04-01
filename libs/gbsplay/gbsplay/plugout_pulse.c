/*
 * gbsplay is a Gameboy sound player
 *
 * 2006-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <pulse/error.h>
#include <pulse/simple.h>

#include "common.h"
#include "plugout.h"

static pa_simple *pulse_handle;
static pa_sample_spec pulse_spec;

static long pulse_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	int err;

	UNUSED(buffer_bytes);

	switch (*endian) {
	case PLUGOUT_ENDIAN_BIG:    pulse_spec.format = PA_SAMPLE_S16BE; break;
	case PLUGOUT_ENDIAN_LITTLE: pulse_spec.format = PA_SAMPLE_S16LE; break;
	default:                    pulse_spec.format = PA_SAMPLE_S16NE; break;
	}
	pulse_spec.rate = rate;
	pulse_spec.channels = 2;

	pulse_handle = pa_simple_new(NULL, metadata.player_name, PA_STREAM_PLAYBACK, NULL, metadata.filename, &pulse_spec, NULL, NULL, &err);
	if (!pulse_handle) {
		fprintf(stderr, "pulse: %s\n", pa_strerror(err));
		return -1;
	}

	return 0;
}

static ssize_t pulse_write(const void *buf, size_t count)
{
	return pa_simple_write(pulse_handle, buf, count, NULL);
}

static void pulse_close()
{
	pa_simple_free(pulse_handle);
}

const struct output_plugin plugout_pulse = {
	.name = "pulse",
	.description = "PulseAudio sound driver",
	.open = pulse_open,
	.write = pulse_write,
	.close = pulse_close,
};
