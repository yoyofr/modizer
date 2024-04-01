/*
 * gbsplay is a Gameboy sound player
 *
 * MIDI output plugin
 *
 * 2008-2022 (C) by Vegard Nossum
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <math.h>

#include "common.h"
#include "midifile.h"
#include "plugout.h"

static int midi_io(cycles_t cycles, uint32_t addr, uint8_t val)
{
	static long div[4] = {0, 0, 0, 0};
	static int volume[4] = {0, 0, 0, 0};
	static int running[4] = {0, 0, 0, 0};
	static int master[4] = {0, 0, 0, 0};
	int new_note;

	long chan = (addr - 0xff10) / 5;

	if (midi_file_is_closed())
		return 1;

	switch (addr) {
	case 0xff12:
	case 0xff17:
		volume[chan] = 8 * (val >> 4);
		master[chan] = (val & 0xf8) != 0;
		if (!master[chan] && running[chan]) {
			/* DAC turned off, disable channel */
			midi_note_off(cycles, chan);
			running[chan] = 0;
		}
		if (volume[chan]) {
			/* volume set to >0, restart current note */
			if (running[chan] && !note[chan]) {
				new_note = NOTE(2048 - div[chan], chan);
				if (new_note < 0 || new_note >= 0x80)
					break;
				midi_note_on(cycles, chan, new_note, volume[chan]);
			}
		} else {
			/* volume set to 0, stop note (if any) */
			midi_note_off(cycles, chan);
		}
		break;
	case 0xff13:
	case 0xff18:
	case 0xff1d:
		div[chan] &= 0xff00;
		div[chan] |= val;

		if (running[chan]) {
			new_note = NOTE(2048 - div[chan], chan);

			if (new_note != note[chan]) {
				/* portamento: retrigger with new note */
				midi_note_off(cycles, chan);

				if (new_note < 0 || new_note >= 0x80)
					break;

				midi_note_on(cycles, chan, new_note, volume[chan]);
			}
		}

		break;
	case 0xff14:
	case 0xff19:
	case 0xff1e:
		div[chan] &= 0x00ff;
		div[chan] |= ((long) (val & 7)) << 8;

		new_note = NOTE(2048 - div[chan], chan);

		/* Channel start trigger */
		if ((val & 0x80) == 0x80) {
			midi_note_off(cycles, chan);

			if (new_note < 0 || new_note >= 0x80)
				break;

			if (master[chan]) {
				midi_note_on(cycles, chan, new_note, volume[chan]);
				running[chan] = 1;
			}
		} else {
			if (running[chan]) {
				if (new_note != note[chan]) {
					/* portamento: retrigger with new note */
					midi_note_off(cycles, chan);

					if (new_note < 0 || new_note >= 0x80)
						break;

					midi_note_on(cycles, chan, new_note, volume[chan]);
				}
			}
		}

		break;
	case 0xff1a:
		master[2] = (val & 0x80) == 0x80;
		if (!master[2] && running[2]) {
			/* DAC turned off, disable channel */
			midi_note_off(cycles, 2);
			running[2] = 0;
		}
		break;
	case 0xff1c:
		volume[2] = 32 * ((4 - (val >> 5)) & 3);
		if (volume[2]) {
			/* volume set to >0, restart current note */
			if (running[2] && !note[2]) {
				new_note = NOTE(2048 - div[chan], chan);
				if (new_note < 0 || new_note >= 0x80)
					break;
				midi_note_on(cycles, 2, new_note, volume[2]);
			}
		} else {
			/* volume set to 0, stop note (if any) */
			midi_note_off(cycles, 2);
		}
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
	case 0xff26:
		if ((val & 0x80) == 0) {
			for (chan = 0; chan < 4; chan++) {
				div[chan] = 0;
				volume[chan] = 0;
				running[chan] = 0;
				master[chan] = 1;
				midi_note_off(cycles, chan);
			}
		}
		break;
	}

	return midi_file_error();
}

static int midi_step(cycles_t cycles, const struct gbs_channel_status status[]) {
	UNUSED(cycles);

	midi_update_mute(status);

	return 0;
}

const struct output_plugin plugout_midi = {
	.name = "midi",
	.description = "MIDI file writer",
	.open = midi_open,
	.skip = midi_skip,
	.step = midi_step,
	.io = midi_io,
	.close = midi_close,
};
