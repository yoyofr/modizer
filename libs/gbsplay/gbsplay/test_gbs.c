/*
 * gbsplay is a Gameboy sound player
 *
 * 2011-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "libgbs.h"
#include "util.h"

int main(int argc, char **argv)
{
	struct gbs *gbs;

	i18n_init();
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <outfile>\n", argv[0]);
		exit(1);
	}

	gbs = gbs_open("examples/nightmode.gbs");
	if (gbs == NULL) {
		fprintf(stderr, "%s: gbs_open failed\n", argv[0]);
		exit(2);
	}
	if (!gbs_write(gbs, argv[1])) {
		fprintf(stderr, "%s: gbs_write failed\n", argv[0]);
		unlink(argv[1]);
		exit(3);
	}
	unlink(argv[1]);
	return 0;
}
