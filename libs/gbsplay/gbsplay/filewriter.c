/*
 * gbsplay is a Gameboy sound player
 *
 * write audio data to file
 *
 * 2006-2022 (C) by Christian Garbs <mitch@cgarbs.de>
 *                  Vegard Nossum
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "filewriter.h"

#define FILENAME_SIZE 23

static char filename[FILENAME_SIZE];

FILE* file_open(const char* const extension, const int subsong) {
	FILE* file = NULL;

	if (snprintf(filename, FILENAME_SIZE, "gbsplay-%d.%s", subsong + 1, extension) >= FILENAME_SIZE)
		goto error;

	if ((file = fopen(filename, "wb")) == NULL)
		goto error;

	return file;

error:
	if (file != NULL) {
		fclose(file);
	}
	return NULL;
}
