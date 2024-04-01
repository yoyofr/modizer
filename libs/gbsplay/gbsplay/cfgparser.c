/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "cfgparser.h"
#include "plugout.h"

static long cfg_line, cfg_char;
static FILE *cfg_file;

static long nextchar_state;

static char nextchar(void)
{
	long c;

	assert(cfg_file != NULL);

	do {
		if ((c = fgetc(cfg_file)) == EOF) return 0;

		if (c == '\n') {
			cfg_char = 0;
			cfg_line++;
		} else cfg_char++;

		switch (nextchar_state) {
		case 0:
			if (c == '\\') nextchar_state = 1;
			else if (c == '#') nextchar_state = 2;
			break;
		case 1:
			nextchar_state = 0;
			if (c == 'n') c = '\n';
			break;
		case 2:
			if (c == 0 || c == '\n') nextchar_state = 0;
			break;
		}
	} while (nextchar_state != 0);

	return (char)c;
}

static long state;
static long nextstate;
static long c;
static const char *filename;

static void err_expect(const char* const s)
{
	fprintf(stderr, _("'%s' expected at %s line %ld char %ld.\n"),
	        s, filename, cfg_line, cfg_char);
	c = nextchar();
	state = 0;
	nextstate = 1;
}

void cfg_endian(void* const ptr)
{
	enum plugout_endian *endian = ptr;

	c = tolower(c);
	if (c != 'b' && c != 'l' && c!= 'n') {
		err_expect("[bln]");
		return;
	}

	switch (c) {
	case 'b': *endian = PLUGOUT_ENDIAN_BIG; break;
	case 'l': *endian = PLUGOUT_ENDIAN_LITTLE; break;
	default: *endian = PLUGOUT_ENDIAN_NATIVE; break;
	}

	c = nextchar();
	state = 0;
	nextstate = 1;
}

void cfg_string(void * const ptr)
{
	char s[200];
	unsigned long n = 0;

	if (!isalpha(c) && c != '-' && c != '_') {
		err_expect("[a-zA-Z_-]");
		return;
	}
	do {
		s[n++] = c;
		c = nextchar();
	} while ((isalnum(c) || c == '-' || c == '_') &&
	         n < (sizeof(s)-1));
	s[n] = 0;

	*((char**)ptr) = strdup(s);

	state = 0;
	nextstate = 1;
}

void cfg_long(void* const ptr)
{
	char num[20];
	unsigned long n = 0;

	if (!isdigit(c)) {
		err_expect("[0-9]");
		return;
	}
	do {
		num[n++] = c;
		c = nextchar();
	} while (isdigit(c) &&
	         n < (sizeof(num)-1));
	num[n] = 0;

	*((long*)ptr) = atoi(num);

	state = 0;
	nextstate = 1;
}

void cfg_int(void* const ptr)
{
	char num[10];
	unsigned long n = 0;

	if (!isdigit(c)) {
		err_expect("[0-9]");
		return;
	}
	do {
		num[n++] = c;
		c = nextchar();
	} while (isdigit(c) &&
	         n < (sizeof(num)-1));
	num[n] = 0;

	*((int*)ptr) = atoi(num);

	state = 0;
	nextstate = 1;
}

void cfg_parse(const char* const fname, const struct cfg_option* const options)
{
	char option[200] = "";

	filename = fname;
	cfg_file = fopen(fname, "r");
	if (cfg_file == NULL) return;

	nextchar_state = 0;
	state = 0;
	nextstate = 1;
	cfg_line = 1;
	cfg_char = 0;
	c = nextchar();

	do {
		unsigned long n;
		switch (state) {
		case 0:
			if (isspace(c))
				while (isspace(c = nextchar()));
			state = nextstate;
			break;
		case 1:
			n = 0;
			if (!isalpha(c)) {
				err_expect("[a-zA-Z]");
				break;
			}
			do {
				option[n++] = c;
				c = nextchar();
			} while ((isalnum(c) ||
			          c == '-' ||
			          c == '_') &&
			          n < (sizeof(option)-1));
			option[n] = '\0';
			state = 0;
			nextstate = 2;
			break;
		case 2:
			if (c != '=') {
				err_expect("=");
				break;
			}
			state = 0;
			nextstate = 3;
			c = nextchar();
			break;
		case 3:
			n=0;
			while (options[n].name != NULL &&
			       strcmp(options[n].name, option) != 0) n++;
			if (options[n].parse_fn)
				options[n].parse_fn(options[n].ptr);
			else {
				fprintf(stderr, _("Unknown option %s at %s line %ld.\n"), option, fname, cfg_line);
				while ((c = nextchar()) != '\n' && c != 0);
				state = 0;
				nextstate = 1;
			}
			c = nextchar();
			break;
		}
	} while (c != 0);

	(void)fclose(cfg_file);
}

char* get_userconfig(const char* const cfgfile)
{
	char *homedir, *usercfg;
	long length;

	homedir = getenv("HOME");
	if (!homedir || !cfgfile) return NULL;

	length  = strlen(homedir) + strlen(cfgfile) + 2;
	usercfg = malloc(length);
	if (usercfg == NULL) {
		fprintf(stderr, "%s\n", _("Memory allocation failed!"));
		return NULL;
	}
	(void)snprintf(usercfg, length, "%s/%s", homedir, cfgfile);

	return usercfg;
}

