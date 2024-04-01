/*
 * gbsplay is a Gameboy sound player
 *
 * common code of MIDI output plugins
 *
 * 2022 (C) by Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include "common.h"
#include "plugout.h"

extern int note[4];

extern int  midi_file_error();
extern int  midi_file_is_closed();
extern void midi_update_mute(const struct gbs_channel_status status[]);

extern void midi_note_on(cycles_t cycles, int channel, int new_note, int velocity);
extern void midi_note_off(cycles_t cycles, int channel);
extern void midi_pan(cycles_t cycles, int channel, int pan);

extern long midi_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata);
extern int  midi_skip(int subsong);
extern void midi_close(void);

#endif
