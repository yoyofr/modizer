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
char *myname;

void usage(long exitcode)
{
        FILE *out = exitcode ? stderr : stdout;
        fprintf(out,
                _("Usage: %s [option] <gbs-file>\n"
		  "\n"
		  "Available options are:\n"
		  "  -h  display this help and exit\n"
		  "  -V  print version and exit\n"),
                myname);
        exit(exitcode);
}

void version(void)
{
	(void)puts("gbsplay " GBS_VERSION);
	exit(0);
}

void parseopts(int *argc, char ***argv)
{
	long res;
	myname = *argv[0];
	while ((res = getopt(*argc, *argv, "hV")) != -1) {
		switch (res) {
		default:
			usage(1);
			break;
		case 'h':
			usage(0);
			break;
		case 'V':
			version();
			break;
		}
	}
	*argc -= optind;
	*argv += optind;
}

int main(int argc, char **argv)
{
	struct gbs *gbs;

	i18n_init();

        parseopts(&argc, &argv);
	
        if (argc < 1) {
                usage(1);
        }

	if ((gbs = gbs_open(argv[0])) == NULL) exit(EXIT_FAILURE);
	gbs_internal_api.print_info(gbs, 1);
	gbs_close(gbs);

	return 0;
}
