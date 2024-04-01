/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "gbhw.h"
#include "gbcpu.h"
#include "libgbs.h"
#include "gbs_internal.h"

/* global variables */
uint8_t logo_data[0x30];

void usage(long exitcode)
{
	FILE *out = exitcode ? stderr : stdout;
	fputs(_("Usage: gbs2gb [option] <gbs-file> <out-file>\n"
		"\n"
		"Available options are:\n"
		"  -t  rom template\n"
		"  -h  display this help and exit\n"
		"  -V  print version and exit\n"),
	      out);
	exit(exitcode);
}

void version(void)
{
	(void)puts("gbs2gb " GBS_VERSION);
	exit(EXIT_SUCCESS);
}

void read_rom_template(const char* const name)
{
	FILE *f = fopen(name, "rb");
	uint8_t hdr[0x200];
	if (!f) {
		fprintf(stderr, _("Could not open ROM template: %s"), name);
		exit(EXIT_FAILURE);
	}
	if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) {
		fclose(f);
		fprintf(stderr, _("Could not open ROM template: %s"), name);
		exit(EXIT_FAILURE);
	}
	memcpy(logo_data, &hdr[0x104], 0x30);
	fclose(f);
}

void parseopts(int *argc, char ***argv)
{
	long res;
	while ((res = getopt(*argc, *argv, "t:hV")) != -1) {
		switch (res) {
		default:
			usage(EXIT_FAILURE);
			break;
		case 't':
			read_rom_template(optarg);
			break;
		case 'h':
			usage(EXIT_SUCCESS);
			break;
		case 'V':
			version();
			break;
		}
	}
	*argc -= optind;
	*argv += optind;
}

void read_default_template()
{
	const uint8_t *bootrom = gbs_internal_api.get_bootrom();
	if (!bootrom) {
		return;
	}
	memcpy(logo_data, &bootrom[0xa8], 0x30);
}

int main(int argc, char **argv)
{
	struct gbs *gbs;
	FILE *out;

	assert_internal_api_valid();
	i18n_init();
	read_default_template();

	parseopts(&argc, &argv);
	
	if (argc < 2) {
		usage(EXIT_FAILURE);
	}

	if (logo_data[0] == 0) {
		fputs(_("ROM template data not found!\n"), stderr);
		usage(EXIT_FAILURE);
	}

	if ((gbs = gbs_open(argv[0])) == NULL)
		exit(EXIT_FAILURE);

	out = fopen(argv[1], "wb");
	if (!out) {
		fprintf(stderr, _("Could not open output file: %s"), argv[1]);
		exit(EXIT_FAILURE);
	}

	gbs_internal_api.write_rom(gbs, out, logo_data);
	gbs_close(gbs);
	fclose(out);

	return 0;
}
