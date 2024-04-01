/*
 * gbsplay is a Gameboy sound player
 *
 * alternative MIDI output plugin
 *
 * 2006-2022 (C) by Christian Garbs <mitch@cgarbs.de>
 *                  Vegard Nossum
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <math.h>

#include "common.h"
#include "midifile.h"
#include "plugout.h"

static int volume[4]  = {0, 0, 0, 0};
static int playing[4] = {0, 0, 0, 0};

static int midi_step(cycles_t cycles, const struct gbs_channel_status status[])
{
	int c;
	int new_playing;
	int new_note;
	const struct gbs_channel_status *ch;

	midi_update_mute(status);
	
	for (c = 0; c < 3; c++) {
		ch = &status[c];
		new_playing = ch->playing;

		if (playing[c]) {
			if (new_playing) {
				new_note = NOTE(ch->div_tc, c);
				if (new_note != note[c]) {
					midi_note_off(cycles, c);
					if (new_note < 0 || new_note >= 0x80)
						continue;
					midi_note_on(cycles, c, new_note, volume[c]);
				}
			} else {
				midi_note_off(cycles, c);
				playing[c] = 0;
			}
		} else {
			if (new_playing) {
				new_note = NOTE(ch->div_tc, c);
				if (new_note < 0 || new_note >= 0x80)
					continue;
				midi_note_on(cycles, c, new_note, volume[c]);
				playing[c] = 1;
			}
		}
		
	}

	return midi_file_error();
}

static int midi_io(cycles_t cycles, uint32_t addr, uint8_t val)
{
	long chan = (addr - 0xff10) / 5;

	if (midi_file_is_closed())
		return 1;

	switch (addr) {
	case 0xff12:
	case 0xff17:
		volume[chan] = 8 * (val >> 4);
		break;
		
	case 0xff14:
	case 0xff19:
	case 0xff1e:
		/* Channel start trigger */
		if ((val & 0x80) == 0x80) {
			midi_note_off(cycles, chan);
			playing[chan] = 0;
		}
		break;

	case 0xff1c:
		volume[2] = 32 * ((4 - (val >> 5)) & 3);
		break;
		
	case 0xff25:
		for (chan = 0; chan < 4; chan++)
			switch ((val >> chan) & 0x11) {
			case 0x10:
				midi_pan(cycles, chan, 0);
				break;
			case 0x01:
				midi_pan(cycles, chan, 127);
				break;
			default:
				midi_pan(cycles, chan, 64);
			}
		break;
		
	}

	return midi_file_error();
}

const struct output_plugin plugout_altmidi = {
	.name = "altmidi",
	.description = "alternative MIDI file writer",
	.open = midi_open,
	.skip = midi_skip,
	.io = midi_io,
	.step = midi_step,
	.close = midi_close,
};
