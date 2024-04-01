/*
 * gbsplay is a Gameboy sound player
 *
 * write audio data to file
 *
 * 2022 (C) by Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _FILEWRITER_H_
#define _FILEWRITER_H_

#include "common.h"

FILE* file_open(const char* const extension, const int subsong);

#endif
