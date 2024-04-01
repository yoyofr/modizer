/*
 * gbsplay is a Gameboy sound player
 *
 * common code of MIDI output plugins
 *
 * 2006-2024 (C) by Christian Garbs <mitch@cgarbs.de>
 *                  Vegard Nossum
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdlib.h>

#include "common.h"
#include "util.h"
#include "filewriter.h"
#include "plugout.h"
#include "midifile.h"

/*
 * The hardware emulation runs at 4194304 Hz and provides an exact
 * cycle counter.  Because ~4 MHz are outside of the possible MIDI
 * time resolution, we introduce an arbitrary lower resolution of 256
 * Hz by shifting the cycle counter 14 bits to the right.
 *
 * This lower resolution (let's call it a "tick" because that's what
 * our target on the MIDI side is called) is
 *
 *  1 tick = 4194304 Hz / 2^14 == 1/256 Hz == 3906.25 us
 */
static const uint8_t CYCLE_RESOLUTION_BIT_SHIFT = 14;

/*
 * Now we need to make a single MIDI tick match the 3906.25 us from above.
 * 
 * In a MIDI file, the Time Division in the header chunk is used to
 * define the temporal resolution of a MIDI file.  It is a 16 bit
 * value with two possible calculation modes:
 *
 * Top bit == 0 selects PPQ (Pulses per quarter note)
 * Top bit == 1 selects Frames per Second
 *
 * When using PPQ (pulses per quarter note) mode, the actual length of
 * a single MIDI tick is calculated as:
 *
 *  1 tick = TEMPO / TIME_DIVISION
 *
 * TEMPO is measured in us/beat (1 beat == 1 quarter note)
 * and can be set by the tempo meta command.
 *
 * A nicely matching pair of values would be:
 *
 *  TEMPO         = 500000 us / beat
 *  TIME_DIVISION = 128 / beat
 *
 * so that
 *
 *  1 tick = 500000 us / 128 == 3906.25 us == 2^14 hardware cycles
 *
 * exactly matches our reduced resolution from before.
 *
 * A TEMPO of 500000 us/beat (120 bpm) just happens to be the default
 * value for MIDI files, so we don't bother to actually set the MIDI
 * tempo when writing a track.
 */
static const uint32_t TIME_DIVISION =
	(0 << 16) // select PPQ mode in highest bit
	+ 128;    // actual value as calculated above

static const long TRACK_LENGTH_OFFSET = 18;
static const long TRACK_START_OFFSET = 22;

static long mute[4] = {0, 0, 0, 0};
static FILE* file = NULL;
static cycles_t cycles_prev = 0;

int note[4];

int midi_file_error() {
	return ferror(file);
}

int midi_file_is_closed() {
	return file == NULL;
}

static int midi_file_close() {
	int result;
	if (midi_file_is_closed())
		return -1;

	result = fclose(file);
	file = NULL;
	return result;
}

void midi_update_mute(const struct gbs_channel_status status[]) {
	for (int chan = 0; chan < 4; chan++)
		mute[chan] = status[chan].mute;
}

static void midi_write_varlen(uint32_t value)
{
	/* Big endian. Highest allowed value is 0x0fffffff */
	for (int shift = 21; shift > 0; shift -= 7) {
		uint8_t v = (value >> shift) & 0x7f;
		if (v) {
			fputc(v | 0x80, file);
		}
	}
	fputc(value & 0x7f, file);
}

static void midi_write_event(cycles_t cycles, const uint8_t *data, unsigned int length)
{
	cycles_t cycles_delta = cycles - cycles_prev;
	unsigned long timestamp_delta = (cycles_delta) >> CYCLE_RESOLUTION_BIT_SHIFT;

	midi_write_varlen(timestamp_delta);
	fwrite(data, length, 1, file);

	// only advance as far as the timestamp resolution allows, so we don't
	// accumulate errors from repeatedly throwing away the lower bits
	cycles_prev += timestamp_delta << CYCLE_RESOLUTION_BIT_SHIFT;
}

static int midi_open_track(int subsong)
{
	if ((file = file_open("mid", subsong)) == NULL)
		goto error;

	/* File header */
	fpack(file, ">{MThd}dwww",
		6, /* header length */
		0, /* format */
		1, /* tracks */
		/* TODO: Do some real calculation instead of this magic number :-) */
		TIME_DIVISION /* division */);

	/* Track header */
	fpack(file, ">{MTrk}d", 0 /* length placeholder */);

	if (ferror(file))
		goto error;

	return 0;

error:
	if (file != NULL)
		midi_file_close();
	return 1;
}

static int midi_close_track()
{
	long track_end_offset;
	uint32_t track_length;
	uint8_t event[] = { 0xff, 0x2f, 0x00 }; /* End of track */

	midi_write_event(cycles_prev, event, sizeof(event));

	/* Update length in header */
	track_end_offset = ftell(file);
	if (track_end_offset < 0 || track_end_offset > 0xffffffff)
		goto error;

	track_length = track_end_offset - TRACK_START_OFFSET;
	fpackat(file, TRACK_LENGTH_OFFSET, ">d", track_length);

	/* Close the file */
	if (midi_file_close() == -1)
		return 1;

	return 0;

error:
	if (file != NULL)
		midi_file_close();
	return 1;
}

void midi_note_on(cycles_t cycles, int channel, int new_note, int velocity)
{
	uint8_t event[] = { 0x90 | channel, new_note, velocity };

	if (mute[channel])
		return;

	midi_write_event(cycles, event, sizeof(event));

	note[channel] = new_note;
}

void midi_note_off(cycles_t cycles, int channel)
{
	uint8_t event[] = { 0x80 | channel, note[channel], 0 };

	if (!note[channel])
		return;

	midi_write_event(cycles, event, sizeof(event));

	note[channel] = 0;
}

void midi_pan(cycles_t cycles, int channel, int pan)
{
	uint8_t event[] = { 0xb0 | channel, 0x0a, pan };

	if (mute[channel])
		return;

	midi_write_event(cycles, event, sizeof(event));
}

long midi_open(enum plugout_endian *endian, long rate, long *buffer_bytes, const struct plugout_metadata metadata)
{
	UNUSED(endian);
	UNUSED(rate);
	UNUSED(buffer_bytes);
	UNUSED(metadata);

	return 0;
}

int midi_skip(int subsong)
{
	int channel;

	if (!midi_file_is_closed()) {
		if (midi_close_track())
			return 1;
	}

	cycles_prev = 0;

	for (channel = 0; channel < 4; channel++)
		note[channel] = 0;

	return midi_open_track(subsong);
}

void midi_close(void)
{
	int channel;

	if (midi_file_is_closed())
		return;

	for (channel = 0; channel < 4; channel++)
		midi_note_off(cycles_prev + 1, channel);

	midi_close_track();
}
