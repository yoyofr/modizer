/*
 * gbsplay is a Gameboy sound player
 * This file contains the player code common to both CLI and X11 frontends.
 *
 * 2003-2020 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _GBS_PLAYER_H_  //YOYOFR, renamed
#define _GBS_PLAYER_H_

#include "common.h"
#include "libgbs.h"
#include "plugout.h"
#include "cfgparser.h"

#define MAXOCTAVE 9

/* global variables */
extern char *myname;
extern char *filename;

/* default values */
extern long verbosity;

extern plugout_open_fn  sound_open;
extern plugout_skip_fn  sound_skip;
extern plugout_pause_fn sound_pause;
extern plugout_io_fn    sound_io;
extern plugout_step_fn  sound_step;
extern plugout_write_fn sound_write;
extern plugout_close_fn sound_close;

struct displaytime {
	long played_min, played_sec, total_min, total_sec;
};

long is_running();
long nextsubsong_cb(struct gbs *gbs, void *priv);
void play_next_subsong(struct gbs *gbs);
void play_prev_subsong(struct gbs *gbs);
long step_emulation(struct gbs *gbs);
void toggle_pause();
long get_pause();
void update_displaytime(struct displaytime *time, const struct gbs_status *status);
struct gbs *common_init(int argc, char **argv);
void common_cleanup(struct gbs *gbs);

#endif
