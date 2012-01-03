/*
 * xmp-asap.c - ASAP plugin for XMPlay
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

#include "xmpin.h"

#include "asap.h"
#include "info_dlg.h"
#include "settings_dlg.h"

#define BITS_PER_SAMPLE  16

static HINSTANCE hInst;
static const XMPFUNC_MISC *xmpfmisc;
static const XMPFUNC_REGISTRY *xmpfreg;
static const XMPFUNC_FILE *xmpffile;
static const XMPFUNC_IN *xmpfin;

ASAP_State asap;

static void WINAPI ASAP_ShowInfo()
{
	showInfoDialog(hInst, xmpfmisc->GetWindow(), NULL, -1);
}

static void WINAPI ASAP_About(HWND win)
{
	MessageBox(win, ASAP_CREDITS "\n" ASAP_COPYRIGHT,
		"About ASAP XMPlay plugin " ASAP_VERSION, MB_OK);
}

static void WINAPI ASAP_Config(HWND win)
{
	if (settingsDialog(hInst, win)) {
		xmpfreg->SetInt("ASAP", "SongLength", &song_length);
		xmpfreg->SetInt("ASAP", "SilenceSeconds", &silence_seconds);
		xmpfreg->SetInt("ASAP", "PlayLoops", &play_loops);
		xmpfreg->SetInt("ASAP", "MuteMask", &mute_mask);
	}
}

static BOOL WINAPI ASAP_CheckFile(const char *filename, XMPFILE file)
{
	return ASAP_IsOurFile(filename);
}

static void GetTag(const char *src, char **dest)
{
	int len;
	if (src[0] == '\0')
		return;
	len = strlen(src) + 1;
	*dest = xmpfmisc->Alloc(len);
	memcpy(*dest, src, len);
}

static void GetTags(const ASAP_ModuleInfo *module_info, char *tags[8])
{
	GetTag(module_info->name, tags);
	GetTag(module_info->author, tags + 1);
	GetTag(module_info->date, tags + 3);
	GetTag("ASAP", tags + 7);
}

static void GetTotalLength(const ASAP_ModuleInfo *module_info, float *length)
{
	int total_duration = 0;
	int song;
	for (song = 0; song < module_info->songs; song++) {
		int duration = getSongDuration(module_info, song);
		if (duration < 0) {
			*length = 0;
			return;
		}
		total_duration += duration;
	}
	*length = total_duration / 1000.0;
}

static void PlaySong(int song)
{
	int duration = playSong(song);
	if (duration >= 0)
		xmpfin->SetLength(duration / 1000.0, TRUE);
	setPlayingSong(NULL, song);
}

static BOOL WINAPI ASAP_GetFileInfo(const char *filename, XMPFILE file, float *length, char *tags[8])
{
	byte module[ASAP_MODULE_MAX];
	int module_len;
	ASAP_ModuleInfo module_info;
	module_len = xmpffile->Read(file, module, sizeof(module));
	if (!ASAP_GetModuleInfo(&module_info, filename, module, module_len))
		return FALSE;
	if (length != NULL)
		GetTotalLength(&module_info, length);
	GetTags(&module_info, tags);
	return TRUE;
}

static DWORD WINAPI ASAP_Open(const char *filename, XMPFILE file)
{
	byte module[ASAP_MODULE_MAX];
	int module_len = xmpffile->Read(file, module, sizeof(module));
	if (!ASAP_Load(&asap, filename, module, module_len))
		return 0;
	setPlayingSong(filename, 0);
	PlaySong(0);
	return 2; /* close file */
}

static void WINAPI ASAP_Close()
{
}

static void WINAPI ASAP_SetFormat(XMPFORMAT *form)
{
	form->rate = ASAP_SAMPLE_RATE;
	form->chan = asap.module_info.channels;
	form->res = BITS_PER_SAMPLE / 8;
}

static BOOL WINAPI ASAP_GetTags(char *tags[8])
{
	GetTags(&asap.module_info, tags);
	return TRUE;
}

static void WINAPI ASAP_GetInfoText(char *format, char *length)
{
	if (format != NULL)
		strcpy(format, "ASAP");
}

static void WINAPI ASAP_GetGeneralInfo(char *buf)
{
	if (asap.module_info.date[0] != '\0')
		buf += sprintf(buf, "Date\t%s\r", asap.module_info.date);
	*buf = '\0';
}

static void WINAPI ASAP_GetMessage(char *buf)
{
}

static double WINAPI ASAP_SetPosition(DWORD pos)
{
	int song = pos - XMPIN_POS_SUBSONG;
	if (song >= 0 && song < asap.module_info.songs) {
		PlaySong(song);
		return 0;
	}
	// TODO: XMPIN_POS
	ASAP_Seek(&asap, pos);
	return pos / 1000.0;
}

static double WINAPI ASAP_GetGranularity()
{
	return 0.001;
}

static DWORD WINAPI ASAP_Process(float *buf, DWORD count)
{
	/* Quick and dirty hack... Need to support floats directly... */
	short *buf2 = (short *) buf;
	DWORD n = ASAP_Generate(&asap, buf2, count * sizeof(short), ASAP_FORMAT_S16_LE) >> 1;
	int i;
	for (i = n; --i >= 0; )
		buf[i] = buf2[i] / 32767.0;
	return n;
}

static DWORD WINAPI ASAP_GetSubSongs(float *length)
{
	GetTotalLength(&asap.module_info, length);
	return asap.module_info.songs;
}

__declspec(dllexport) XMPIN *WINAPI XMPIN_GetInterface(DWORD face, InterfaceProc faceproc)
{
	static XMPIN xmpin = {
		0,
		"ASAP",
		"ASAP\0sap/cmc/cm3/cmr/cms/dmc/dlt/mpt/mpd/rmt/tmc/tm8/tm2",
		ASAP_About,
		ASAP_Config,
		ASAP_CheckFile,
		ASAP_GetFileInfo,
		ASAP_Open,
		ASAP_Close,
		NULL,
		ASAP_SetFormat,
		ASAP_GetTags,
		ASAP_GetInfoText,
		ASAP_GetGeneralInfo,
		ASAP_GetMessage,
		ASAP_SetPosition,
		ASAP_GetGranularity,
		NULL,
		ASAP_Process,
		NULL,
		NULL,
		ASAP_GetSubSongs,
		NULL,
		NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};
	static const XMPSHORTCUT info_shortcut = {
		0x10000, "ASAP - File information", ASAP_ShowInfo
	};

	if (face != XMPIN_FACE)
		return NULL;

	xmpfmisc = (const XMPFUNC_MISC *) faceproc(XMPFUNC_MISC_FACE);
	xmpfreg = (const XMPFUNC_REGISTRY *) faceproc(XMPFUNC_REGISTRY_FACE);
	xmpffile = (const XMPFUNC_FILE *) faceproc(XMPFUNC_FILE_FACE);
	xmpfin = (const XMPFUNC_IN *) faceproc(XMPFUNC_IN_FACE);

	xmpfmisc->RegisterShortcut(&info_shortcut);
	xmpfreg->GetInt("ASAP", "SongLength", &song_length);
	xmpfreg->GetInt("ASAP", "SilenceSeconds", &silence_seconds);
	xmpfreg->GetInt("ASAP", "PlayLoops", &play_loops);
	xmpfreg->GetInt("ASAP", "MuteMask", &mute_mask);
	return &xmpin;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		hInst = hInstance;
	return TRUE;
}
