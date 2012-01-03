/*
 * asap-sdl.c - simple SDL ASAP player
 *
 * Copyright (C) 2010  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <SDL.h>

#include "asap.h"

static ASAP_State asap;
static int song = -1;

static void print_help(void)
{
	printf(
		"Usage: asap_sdl [OPTIONS] INPUTFILE\n"
		"INPUTFILE must be in a supported format:\n"
		"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8 or TM2.\n"
		"Options:\n"
		"-s SONG     --song=SONG        Select subsong number (zero-based)\n"
		"-h          --help             Display this information\n"
		"-v          --version          Display version information\n"
	);
}

static void fatal_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "asap_sdl: ");
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);
	exit(1);
}

static void sdl_error(const char *fun)
{
	fatal_error("%s failed: %s", fun, SDL_GetError());
}

static void set_song(const char *s)
{
	song = 0;
	do {
		if (*s < '0' || *s > '9')
			fatal_error("subsong number must be an integer");
		song = 10 * song + *s++ - '0';
		if (song >= ASAP_SONGS_MAX)
			fatal_error("maximum subsong number is %d", ASAP_SONGS_MAX - 1);
	} while (*s != '\0');
}

static void print_header(const char *name, const char *value)
{
	if (value[0] != '\0')
		printf("%s: %s\n", name, value);
}

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
	ASAP_Generate(&asap, stream, len, ASAP_FORMAT_S16_LE);
}

static void process_file(const char *input_file)
{
	FILE *fp;
	static byte module[ASAP_MODULE_MAX];
	int module_len;
	SDL_AudioSpec desired;

	fp = fopen(input_file, "rb");
	if (fp == NULL)
		fatal_error("cannot open %s", input_file);
	module_len = fread(module, 1, sizeof(module), fp);
	fclose(fp);

	if (!ASAP_Load(&asap, input_file, module, module_len))
		fatal_error("%s: unsupported file", input_file);
	if (song < 0)
		song = asap.module_info.default_song;
	ASAP_PlaySong(&asap, song, -1);
	print_header("Name", asap.module_info.name);
	print_header("Author", asap.module_info.author);
	print_header("Date", asap.module_info.date);

	if (SDL_Init(SDL_INIT_AUDIO) != 0)
		sdl_error("SDL_Init");
	desired.freq = ASAP_SAMPLE_RATE;
	desired.format = AUDIO_S16LSB;
	desired.channels = asap.module_info.channels;
	desired.samples = 4096;
	desired.callback = audio_callback;
	if (SDL_OpenAudio(&desired, NULL) != 0)
		sdl_error("SDL_OpenAudio");
	SDL_PauseAudio(0);
	printf(" playing - press Enter to quit\n");
	getchar();
	SDL_Quit();
}

int main(int argc, char *argv[])
{
	const char *options_error = "no input file";
	int i;
	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (arg[0] != '-') {
			process_file(arg);
			options_error = NULL;
			continue;
		}
		options_error = "options must be specified before the input file";
#define is_opt(c)  (arg[1] == c && arg[2] == '\0')
		if (is_opt('s'))
			set_song(argv[++i]);
		else if (strncmp(arg, "--song=", 7) == 0)
			set_song(arg + 7);
		else if (is_opt('h') || strcmp(arg, "--help") == 0) {
			print_help();
			options_error = NULL;
		}
		else if (is_opt('v') || strcmp(arg, "--version") == 0) {
			printf("asap_sdl " ASAP_VERSION "\n");
			options_error = NULL;
		}
		else
			fatal_error("unknown option: %s", arg);
	}
	if (options_error != NULL) {
		fprintf(stderr, "asap_sdl: %s\n", options_error);
		print_help();
		return 1;
	}
	return 0;
}
