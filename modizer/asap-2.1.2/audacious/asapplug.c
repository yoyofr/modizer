/*
 * asapplug.c - ASAP plugin for Audacious
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

#include <audacious/plugin.h>

#include "asap.h"

#define BITS_PER_SAMPLE  16

static GMutex *control_mutex;
static ASAP_State asap;

static void plugin_init(void)
{
	control_mutex = g_mutex_new();
}

static void plugin_cleanup(void)
{
	g_mutex_free(control_mutex);
}

static void plugin_about(void)
{
	static GtkWidget *aboutbox = NULL;
	if (aboutbox == NULL) {
		aboutbox = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s",
			ASAP_CREDITS "\n" ASAP_COPYRIGHT);
		gtk_window_set_title((GtkWindow *) aboutbox, "About ASAP plugin " ASAP_VERSION);
		g_signal_connect(aboutbox, "response", (GCallback) gtk_widget_destroy, NULL);
		g_signal_connect(aboutbox, "destroy", (GCallback) gtk_widget_destroyed, &aboutbox);
	}
	gtk_window_present((GtkWindow *) aboutbox);
}

static gint is_our_file_from_vfs(const gchar *filename, VFSFile *file)
{
	return ASAP_IsOurFile(filename);
}

static gboolean load_module(const gchar *filename, VFSFile *file, ASAP_ModuleInfo *module_info, int *song)
{
	byte module[ASAP_MODULE_MAX];
	gboolean ok = FALSE;
#if __AUDACIOUS_PLUGIN_API__ >= 10
	char *real_filename = filename_split_subtune(filename, song);
	if (real_filename != NULL)
		filename = real_filename;
#endif
	if (file == NULL)
		file = vfs_fopen(filename, "rb");
	if (file != NULL) {
		int module_len = vfs_fread(module, 1, sizeof(module), file);
		vfs_fclose(file);
		if (module_info != NULL)
			ok = ASAP_GetModuleInfo(module_info, filename, module, module_len);
		else
			ok = ASAP_Load(&asap, filename, module, module_len);
	}
#if __AUDACIOUS_PLUGIN_API__ >= 10
	if (real_filename != NULL)
		g_free(real_filename);
#endif
	return ok;
}

static void tuple_set(Tuple *tuple, gint nfield, const char *value)
{
	if (value[0] != '\0')
		tuple_associate_string(tuple, nfield, NULL, value);
}

static Tuple *probe_for_tuple(const gchar *filename, VFSFile *file)
{
	int song = -1;
	ASAP_ModuleInfo module_info;
	Tuple *tuple;
	int duration;
	/* char year[5]; */

	if (!load_module(filename, file, &module_info, &song))
		return NULL;

	tuple = tuple_new_from_filename(filename);
	if (tuple == NULL)
		return NULL;
	tuple_set(tuple, FIELD_ARTIST, module_info.author);
	tuple_set(tuple, FIELD_TITLE, module_info.name);
	tuple_set(tuple, FIELD_DATE, module_info.date);
	if (song > 0) {
		tuple_associate_int(tuple, FIELD_SUBSONG_ID, NULL, song);
		tuple_associate_int(tuple, FIELD_SUBSONG_NUM, NULL, module_info.songs);
		song--;
	}
	else {
		if (module_info.songs > 1)
			tuple->nsubtunes = module_info.songs;
		song = module_info.default_song;
	}
	duration = module_info.durations[song];
	if (duration > 0)
		tuple_associate_int(tuple, FIELD_LENGTH, NULL, duration);
	/* doesn't work, need int?
	if (ASAP_DateToYear(module_info.date, year))
		tuple_set(tuple, FIELD_YEAR, year); */
	return tuple;
}

static Tuple *get_song_tuple(const gchar *filename)
{
	return probe_for_tuple(filename, NULL);
}

static gboolean play_start(InputPlayback *playback, const gchar *filename, VFSFile *file, gint start_time, gint stop_time, gboolean pause)
{
	int song = -1;
	int channels;

	if (!load_module(filename, file, NULL, &song))
		return FALSE;
	channels = asap.module_info.channels;
	if (song > 0)
		song--;
	else
		song = asap.module_info.default_song;
	if (stop_time < 0)
		stop_time = asap.module_info.durations[song];
	ASAP_PlaySong(&asap, song, stop_time);
	if (start_time > 0)
		ASAP_Seek(&asap, start_time);

	if (!playback->output->open_audio(BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_LE, ASAP_SAMPLE_RATE, channels))
		return FALSE;
	playback->set_params(playback, NULL, 0, 0, ASAP_SAMPLE_RATE, channels);
	if (pause)
		playback->output->pause(TRUE);
	playback->playing = TRUE;
	playback->set_pb_ready(playback);

	for (;;)
	{
		static byte buffer[4096];
		int len;
		g_mutex_lock(control_mutex);
		if (!playback->playing) {
			g_mutex_unlock(control_mutex);
			break;
		}
		len = ASAP_Generate(&asap, buffer, sizeof(buffer), BITS_PER_SAMPLE);
		g_mutex_unlock(control_mutex);
		if (len <= 0) {
			playback->eof = TRUE;
			break;
		}
#if __AUDACIOUS_PLUGIN_API__ >= 14
		playback->output->write_audio(buffer, len);
#else
		playback->pass_audio(playback, BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_LE, channels, len, buffer, NULL);
#endif
	}

	while (playback->playing && playback->output->buffer_playing())
		g_usleep(10000);
	g_mutex_lock(control_mutex);
	playback->playing = FALSE;
	g_mutex_unlock(control_mutex);
	playback->output->close_audio();
	return TRUE;
}

static void play_file(InputPlayback *playback)
{
	play_start(playback, playback->filename, NULL, 0, -1, FALSE);
}

static void play_pause(InputPlayback *playback, gshort pause)
{
	g_mutex_lock(control_mutex);
	if (playback->playing)
		playback->output->pause(pause);
	g_mutex_unlock(control_mutex);
}

static void play_mseek(InputPlayback *playback, gulong time)
{
	g_mutex_lock(control_mutex);
	if (playback->playing) {
		ASAP_Seek(&asap, time);
#if __AUDACIOUS_PLUGIN_API__ >= 15
		playback->output->abort_write();
#endif
	}
	g_mutex_unlock(control_mutex);
}

static void play_stop(InputPlayback *playback)
{
	g_mutex_lock(control_mutex);
	if (playback->playing) {
#if __AUDACIOUS_PLUGIN_API__ >= 15
		playback->output->abort_write();
#endif
		playback->playing = FALSE;
	}
	g_mutex_unlock(control_mutex);
}

static
#if __AUDACIOUS_PLUGIN_API__ >= 16
	const 
#endif
	gchar *exts[] = { "sap", " cmc", " cm3", " cmr", " cms", " dmc", " dlt", " mpt", " mpd", " rmt", " tmc", " tm8", "tm2", NULL };

static InputPlugin asap_ip = {
	.description = "ASAP Plugin",
	.init = plugin_init,
	.cleanup = plugin_cleanup,
	.about = plugin_about,
	.have_subtune = TRUE,
	.vfs_extensions = exts,
	.is_our_file_from_vfs = is_our_file_from_vfs,
#if __AUDACIOUS_PLUGIN_API__ >= 16
	.probe_for_tuple = probe_for_tuple,
	.play = play_start,
#else
	.get_song_tuple = get_song_tuple,
	.play_file = play_file,
#endif
	.pause = play_pause,
	.mseek = play_mseek,
	.stop = play_stop,
};

static InputPlugin *asap_iplist[] = { &asap_ip, NULL };

SIMPLE_INPUT_PLUGIN(ASAP, asap_iplist)
