/*
 * gbsplay is a Gameboy sound player
 *
 * WAV file writer output plugin
 *
 * 2022 (C) by Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "util.h"
#include "filewriter.h"
#include "plugout.h"

static const uint8_t blank_hdr[44];

static long sample_rate;
static FILE* file = NULL;

static int wav_write_header() {
	const uint32_t fmt_subchunk_length = 16;
	const uint16_t audio_format_uncompressed_pcm = 1;
	const uint16_t num_channels = 2;
	const uint16_t bits_per_sample = 16;
	const uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
	const uint16_t block_align = num_channels * bits_per_sample / 8;

	long filesize = ftell(file);
	if (filesize < 0 || filesize > 0xffffffff)
		return -1;

	fpackat(file, 0, "<{RIFF}d{WAVE}<{fmt }dwwddww{data}d",
	        (uint32_t)filesize - 8,
	        fmt_subchunk_length,
	        audio_format_uncompressed_pcm,
	        num_channels,
	        sample_rate,
	        byte_rate,
	        block_align,
	        bits_per_sample,
	        (uint32_t)filesize - 44);
	return 0;
}

static int wav_open_file(const int subsong) {
	if ((file = file_open("wav", subsong)) == NULL)
		return -1;

	fwrite(blank_hdr, sizeof(blank_hdr), 1, file);
	
	return 0;
}

static int wav_close_file() {
	int result;

	if (wav_write_header())
		return -1;

	result = fclose(file);
	file = NULL;
	return result;
}

static long wav_open(enum plugout_endian *endian,
		     const long rate, long *buffer_bytes,
		     const struct plugout_metadata metadata)
{
	UNUSED(buffer_bytes);
	UNUSED(metadata);

	sample_rate = rate;

	*endian = PLUGOUT_ENDIAN_LITTLE;

	return 0;
}

static int wav_skip(const int subsong)
{
	if (file != NULL)
		if (wav_close_file())
			return -1;

	return wav_open_file(subsong);
}

static ssize_t wav_write(const void *buf, const size_t count)
{
	return fwrite(buf, count, 1, file);
}

static void wav_close()
{
	if (file != NULL)
		wav_close_file();

	return;
}

const struct output_plugin plugout_wav = {
	.name = "wav",
	.description = "WAV file writer",
	.open = wav_open,
	.skip = wav_skip,
	.write = wav_write,
	.close = wav_close,
};
