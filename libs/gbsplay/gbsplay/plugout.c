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
#include <string.h>

#include "common.h"
#include "plugout.h"

#ifdef PLUGOUT_ALSA
extern const struct output_plugin plugout_alsa;
#endif
#ifdef PLUGOUT_DEVDSP
extern const struct output_plugin plugout_devdsp;
#endif
#ifdef PLUGOUT_DSOUND
extern const struct output_plugin plugout_dsound;
#endif
#ifdef PLUGOUT_IODUMPER
extern const struct output_plugin plugout_iodumper;
#endif
#ifdef PLUGOUT_MIDI
extern const struct output_plugin plugout_midi;
#endif
#ifdef PLUGOUT_ALTMIDI
extern const struct output_plugin plugout_altmidi;
#endif
#ifdef PLUGOUT_NAS
extern const struct output_plugin plugout_nas;
#endif
#ifdef PLUGOUT_PIPEWIRE
extern const struct output_plugin plugout_pipewire;
#endif
#ifdef PLUGOUT_PULSE
extern const struct output_plugin plugout_pulse;
#endif
#ifdef PLUGOUT_SDL
extern const struct output_plugin plugout_sdl;
#endif
#ifdef PLUGOUT_STDOUT
extern const struct output_plugin plugout_stdout;
#endif
#ifdef PLUGOUT_VGM
extern const struct output_plugin plugout_vgm;
#endif
#ifdef PLUGOUT_WAV
extern const struct output_plugin plugout_wav;
#endif
#ifdef PLUGOUT_MODIZER
extern const struct output_plugin plugout_modizer;
#endif

typedef const struct output_plugin* output_plugin_const_t;

/* in order of preference, see also PLUGOUT_DEFAULT in plugout.h */
static output_plugin_const_t plugouts[] = {
#ifdef PLUGOUT_DSOUND
	&plugout_dsound,
#endif
#ifdef PLUGOUT_PULSE
	&plugout_pulse,
#endif
#ifdef PLUGOUT_ALSA
	&plugout_alsa,
#endif
#ifdef PLUGOUT_DEVDSP
	&plugout_devdsp,
#endif
#ifdef PLUGOUT_STDOUT
	&plugout_stdout,
#endif
#ifdef PLUGOUT_PIPEWIRE
	&plugout_pipewire,
#endif
#ifdef PLUGOUT_NAS
	&plugout_nas,
#endif
#ifdef PLUGOUT_MIDI
	&plugout_midi,
#endif
#ifdef PLUGOUT_ALTMIDI
	&plugout_altmidi,
#endif
#ifdef PLUGOUT_IODUMPER
	&plugout_iodumper,
#endif
#ifdef PLUGOUT_SDL
	&plugout_sdl,
#endif
#ifdef PLUGOUT_VGM
	&plugout_vgm,
#endif
#ifdef PLUGOUT_WAV
	&plugout_wav,
#endif
#ifdef PLUGOUT_MODIZER
    &plugout_modizer,
#endif
	NULL
};

void plugout_list_plugins(void)
{
	long idx;

	printf("%s", _("Available output plugins:\n\n"));

	if (plugouts[0] == NULL) {
		printf("%s", _("No output plugins available.\n\n"));
		return;
	}

	for (idx = 0; plugouts[idx] != NULL; idx++) {
		printf("%-8s - %s\n", plugouts[idx]->name, plugouts[idx]->description);
	}
	(void)puts("");
}

const struct output_plugin* plugout_select_by_name(const char* const name)
{
	long idx;

	for (idx = 0; plugouts[idx] != NULL &&
	              strcmp(plugouts[idx]->name, name) != 0; idx++);

	return plugouts[idx];
}
