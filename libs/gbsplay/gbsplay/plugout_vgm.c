/*
 * gbsplay is a Gameboy sound player
 *
 * VGM output plugin
 * 2022 (C) by Maximilian Rehkopf <otakon@gmx.net>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "filewriter.h"
#include "plugout.h"
#include "util.h"

#define VGM_FILE_VERSION          (0x161)
#define VGM_DMG_CLOCK             (0x400000)
#define VGM_WAITSAMPLES_MAX       (0xffff)
#define VGM_TICKS_PER_SECOND      (44100)

#define VGM_OFS_NUMSAMPLES        (0x18)
#define VGM_OFS_DATA_START        (0x34)
#define VGM_OFS_DMG_CLOCK         (0x80)
#define VGM_HDR_LEN               (0x84)

#define VGM_CMD_WAITSAMPLES       (0x61)
#define VGM_CMD_WAITSAMPLES_SHORT (0x70)
#define VGM_CMD_END               (0x66)
#define VGM_CMD_DMGWRITE          (0xb3)

#define VGM_DATA_START_REL        (VGM_HDR_LEN - VGM_OFS_DATA_START)

static FILE *vgmfile;
static double samples_total = 0;
static double samples_prev = 0;
static double sample_diff_acc = 0;
static const uint8_t blank_hdr[VGM_HDR_LEN];

/* finalize VGM output file */
static void vgm_finalize(void) {
	size_t eof_offset;
	fpack(vgmfile, "<bwb",
		/* append a second of delay at the end (for sounds to finish) */
		VGM_CMD_WAITSAMPLES, VGM_TICKS_PER_SECOND,
		/* write end of data marker */
		VGM_CMD_END
	);

	/* fill in header entries */
	eof_offset = ftell(vgmfile) - 4;
	fpackat(vgmfile, 0, "<{Vgm }dd", eof_offset, VGM_FILE_VERSION);
	fpackat(vgmfile, VGM_OFS_DMG_CLOCK,  "<d", VGM_DMG_CLOCK);
	fpackat(vgmfile, VGM_OFS_DATA_START, "<d", VGM_DATA_START_REL);
	fpackat(vgmfile, VGM_OFS_NUMSAMPLES, "<d", (uint32_t)samples_total);
}

static long vgm_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	UNUSED(endian);
	UNUSED(rate);
	UNUSED(buffer_bytes);
	UNUSED(metadata);
	return 0;
}

static int vgm_open_file(int subsong) {
	vgmfile = file_open("vgm", subsong);

	if (vgmfile == NULL) {
		fprintf(stderr, "Can't open output file: %s\n", strerror(errno));
		return -1;
	}

	/* zero-pad header area */
	fwrite(blank_hdr, sizeof(blank_hdr), 1, vgmfile);

	/* basic HW initialization */
	fpack(vgmfile, "<bbb", VGM_CMD_DMGWRITE, 0xff26 - 0xff10, 0x80);  /* NR52: APU on */
	fpack(vgmfile, "<bbb", VGM_CMD_DMGWRITE, 0xff25 - 0xff10, 0xf3);  /* NR51: L/R panning */
	fpack(vgmfile, "<bbb", VGM_CMD_DMGWRITE, 0xff24 - 0xff10, 0x77);  /* NR50: Master volume */

	sample_diff_acc = 0;
	samples_prev = 0;
	return 0;
}

static int vgm_close_file(void) {
	int result;
	vgm_finalize();
	result = fclose(vgmfile);
	vgmfile = NULL;
	return result;
}

static int vgm_skip(int subsong) {
	if (vgmfile) {
		if (vgm_close_file()) {
			return 1;
		};
	}

	return vgm_open_file(subsong);
}

static int vgm_io(cycles_t cycles, uint32_t addr, uint8_t val) {
	double sample_diff;
	int vgm_sample_diff;
	uint8_t vgmreg;

	/* calculate fractional samples (VGM counts everything as 44100Hz samples) */
	samples_total = (double)cycles * (double)VGM_TICKS_PER_SECOND / (double)VGM_DMG_CLOCK;
	sample_diff = samples_total - samples_prev;

	/* accumulate fractional samples, use integer part as delay time in samples */
	sample_diff_acc += sample_diff;
	vgm_sample_diff = sample_diff_acc;

	/* subtract integer part, keeping fractional part for further accumulation */
	sample_diff_acc -= vgm_sample_diff;

	/* write calculated sample delay commands in chunks of <= 65535 samples.
	   use single-byte delay shortcut command on 1..16 sample delays. */
	while (vgm_sample_diff > 0) {
		if (vgm_sample_diff < 17) {
			fputc(VGM_CMD_WAITSAMPLES_SHORT | (vgm_sample_diff - 1), vgmfile);
		} else {
			fpack(vgmfile, "<bw",
			      VGM_CMD_WAITSAMPLES,
			      (vgm_sample_diff > VGM_WAITSAMPLES_MAX)
			      ? VGM_WAITSAMPLES_MAX
			      : vgm_sample_diff);
		}
		vgm_sample_diff -= VGM_WAITSAMPLES_MAX;
	}

	/* dump the register write if register is valid for VGM dump. */
	if (addr >= 0xff10) {
		vgmreg = addr - 0xff10;

		fpack(vgmfile, "<bbb", VGM_CMD_DMGWRITE, vgmreg, val);
	}

	samples_prev = samples_total;

	return 0;
}

static void vgm_close(void) {
	if (vgmfile) {
		vgm_close_file();
	}
}

const struct output_plugin plugout_vgm = {
	.name = "vgm",
	.description = "VGM file writer",
	.open = vgm_open,
	.skip = vgm_skip,
	.io = vgm_io,
	.close = vgm_close
};
