/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _CFGPARSER_H_
#define _CFGPARSER_H_

typedef void (*cfg_parse_fn)(void* const ptr);

struct cfg_option {
	const char* const name;
	void* const ptr;
	const cfg_parse_fn parse_fn;
};

void  cfg_string(void* const ptr);
void  cfg_long(void* const ptr);
void  cfg_int(void* const ptr);
void  cfg_endian(void* const ptr);
void  cfg_parse(const char* const fname, const struct cfg_option* const options);
char* get_userconfig(const char* const cfgfile);

#endif
