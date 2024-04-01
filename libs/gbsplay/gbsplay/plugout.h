/*
 * gbsplay is a Gameboy sound player
 *
 * 2004-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _PLUGOUT_H_
#define _PLUGOUT_H_

#include <stdint.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "gbhw.h"

#if PLUGOUT_DSOUND == 1
#  define PLUGOUT_DEFAULT "dsound"
#elif PLUGOUT_PIPEWIRE == 1
#  define PLUGOUT_DEFAULT "pipewire"
#elif PLUGOUT_PULSE == 1
#  define PLUGOUT_DEFAULT "pulse"
#elif PLUGOUT_ALSA == 1
#  define PLUGOUT_DEFAULT "alsa"
#elif PLUGOUT_SDL == 1
#  define PLUGOUT_DEFAULT "sdl"
#elif PLUGOUT_MODIZER == 1
#  define PLUGOUT_DEFAULT "modizer"
#else
#  define PLUGOUT_DEFAULT "oss"
#endif

enum plugout_endian {
	PLUGOUT_ENDIAN_BIG,
	PLUGOUT_ENDIAN_LITTLE,
	PLUGOUT_ENDIAN_AUTOSELECT,
};

#if GBS_BYTE_ORDER == GBS_ORDER_LITTLE_ENDIAN
#define PLUGOUT_ENDIAN_NATIVE PLUGOUT_ENDIAN_LITTLE
#else
#define PLUGOUT_ENDIAN_NATIVE PLUGOUT_ENDIAN_BIG
#endif

struct plugout_metadata {
	const char * player_name;
	const char * filename;
};

/* Initial open of plugout. */
typedef long    (*plugout_open_fn )(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata);
/* Notification when next subsong is about to start. */
typedef int     (*plugout_skip_fn )(int subsong);
/* Notification the the player is paused/resumed. */
typedef void    (*plugout_pause_fn)(int pause);
/* Callback for monitoring IO in dumpers. Cycles restarts at 0 when the subsong is changed. */
typedef int     (*plugout_io_fn   )(cycles_t cycles, uint32_t addr, uint8_t val);
/* Callback for monitoring inferred channel status. */
typedef int     (*plugout_step_fn )(const cycles_t cycles, const struct gbs_channel_status[]);
/* Callback for writing sample data. */
typedef ssize_t (*plugout_write_fn)(const void *buf, size_t count);
/* Close called on player exit. */
typedef void    (*plugout_close_fn)(void);

#define PLUGOUT_USES_STDOUT	1

struct output_plugin {
	char	*name;
	char	*description;
	long	flags;
	plugout_open_fn  open;
	plugout_skip_fn  skip;
	plugout_pause_fn pause;
	plugout_io_fn    io;
	plugout_step_fn  step;
	plugout_write_fn write;
	plugout_close_fn close;
};

void plugout_list_plugins(void);
const struct output_plugin* plugout_select_by_name(const char* const name);

#endif
