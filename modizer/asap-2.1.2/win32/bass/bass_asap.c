/*
 * bass_asap.c - ASAP add-on for BASS
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

#include <string.h>
#include <windows.h>

#include "bass-addon.h"

/* ID3 tag doesn't work in AIMP2 because it doesn't call GetTags */
/* #define SUPPORT_ID3 */

#define BASS_CTYPE_MUSIC_ASAP  0x1f100

#include "asap.h"

static const BASS_PLUGINFORM pluginform = {
	BASS_CTYPE_MUSIC_ASAP, "ASAP", "*.sap;*.cmc;*.cm3;*.cmr;*.cms;*.dmc;*.dlt;*.mpt;*.mpd;*.rmt;*.tmc;*.tm8;*.tm2"
};

static const BASS_PLUGININFO plugininfo = { 0x02040000, 1, &pluginform };

typedef struct
{
	ASAP_State asap;
	int duration;
#ifdef SUPPORT_ID3
	TAG_ID3 tag;
#endif
} ASAPSTREAM;

static void WINAPI ASAP_Free(void *inst)
{
	free(inst);
}

static QWORD WINAPI ASAP_GetLength(void *inst, DWORD mode)
{
	ASAPSTREAM *stream = (ASAPSTREAM *) inst;
	if (mode != BASS_POS_BYTE)
		errorn(BASS_ERROR_NOTAVAIL);
	noerrorn((stream->duration * (ASAP_SAMPLE_RATE / 100) / 10) << stream->asap.module_info.channels); // assumes 16-bit
}

#ifdef SUPPORT_ID3

static const char * WINAPI ASAP_GetTags(void *inst, DWORD tags)
{
	ASAPSTREAM *stream = (ASAPSTREAM *) inst;
	TAG_ID3 *tag;
	if (tags != BASS_TAG_ID3)
		return NULL;
	tag = &stream->tag;
	memset(tag, 0, sizeof(TAG_ID3));
	tag->id[0] = 'T';
	tag->id[1] = 'A';
	tag->id[2] = 'G';
	strncpy(tag->title, stream->asap.module_info.name, sizeof(tag->title));
	strncpy(tag->artist, stream->asap.module_info.author, sizeof(tag->title));
	ASAP_DateToYear(stream->asap.module_info.date, tag->year);
	tag->genre = 52; /* Electronic */
	return (const char *) tag;
}

#endif

static void WINAPI ASAP_GetInfo(void *inst, BASS_CHANNELINFO *info)
{
	info->ctype = BASS_CTYPE_MUSIC_ASAP;
}

static int bytes_to_duration(ASAPSTREAM *stream, int pos)
{
	return (pos >> stream->asap.module_info.channels) * 10 / (ASAP_SAMPLE_RATE / 100);
}

static BOOL WINAPI ASAP_CanSetPosition(void *inst, QWORD pos, DWORD mode)
{
	ASAPSTREAM *stream = (ASAPSTREAM *) inst;
	if ((BYTE) mode != BASS_POS_BYTE)
		error(BASS_ERROR_NOTAVAIL);
	if (stream->duration >= 0 && (pos > 0xffffffff || bytes_to_duration(stream, pos) >= stream->duration))
		error(BASS_ERROR_POSITION);
	return TRUE;
}

static QWORD WINAPI ASAP_SetPosition(void *inst, QWORD pos, DWORD mode)
{
	ASAPSTREAM *stream = (ASAPSTREAM *) inst;
	if ((BYTE) mode != BASS_POS_BYTE)
		errorn(BASS_ERROR_NOTAVAIL);
	ASAP_Seek(&stream->asap, bytes_to_duration(stream, pos));
	return pos;
}

static ADDON_FUNCTIONS ASAPfuncs={
	0,
	ASAP_Free,
	ASAP_GetLength,
#ifdef SUPPORT_ID3
	ASAP_GetTags,
#else
	NULL,
#endif
	NULL,
	ASAP_GetInfo,
	ASAP_CanSetPosition,
	ASAP_SetPosition,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static DWORD CALLBACK StreamProc(HSTREAM handle, void *buffer, DWORD length, void *inst)
{
	ASAPSTREAM *stream = (ASAPSTREAM *) inst;
	int c = ASAP_Generate(&stream->asap, buffer, length, ASAP_FORMAT_S16_LE);
	if (c < length)
		c |= BASS_STREAMPROC_END;
	return c;
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags)
{
	BOOL unicode = FALSE;
	const char *filename;
	char aFilename[MAX_PATH];
	byte module[ASAP_MODULE_MAX];
	int module_len;
	ASAPSTREAM *stream;
	int song;
	int duration;
	HSTREAM handle;
	filename = (const char *) bassfunc->file.GetFileName(file, &unicode);
	if (filename == NULL)
		error(BASS_ERROR_NOTFILE);
	if (unicode) {
		if (WideCharToMultiByte(CP_ACP, 0, (LPCWSTR) filename, -1, aFilename, MAX_PATH, NULL, NULL) <= 0)
			error(BASS_ERROR_NOTFILE);
		filename = aFilename;
	}
	module_len = bassfunc->file.Read(file, module, sizeof(module));
	stream = malloc(sizeof(ASAPSTREAM));
	if (stream == NULL)
		error(BASS_ERROR_MEM);
	if (!ASAP_Load(&stream->asap, filename, module, module_len)) {
		free(stream);
		error(BASS_ERROR_FILEFORM);
	}
	flags &= BASS_SAMPLE_SOFTWARE | BASS_SAMPLE_LOOP | BASS_SAMPLE_3D | BASS_SAMPLE_FX
		| BASS_STREAM_DECODE | BASS_STREAM_AUTOFREE | 0x3f000000; // 0x3f000000 = all speaker flags
	song = stream->asap.module_info.default_song;
	if ((flags & BASS_MUSIC_LOOP) != 0 && stream->asap.module_info.loops[song])
		duration = -1;
	else
		duration = stream->asap.module_info.durations[song];
	stream->duration = duration;
	ASAP_PlaySong(&stream->asap, song, duration);
	handle = bassfunc->CreateStream(ASAP_SAMPLE_RATE, stream->asap.module_info.channels, flags, &StreamProc, stream, &ASAPfuncs);
	if (handle == 0) {
		free(stream);
		return 0; // CreateStream set the error code
	}
	bassfunc->file.SetStream(file, handle);
	noerrorn(handle);
}

__declspec(dllexport) const void *WINAPI BASSplugin(DWORD face)
{
	switch (face) {
	case BASSPLUGIN_INFO:
		return &plugininfo;
	case BASSPLUGIN_CREATE:
		return StreamCreateProc;
	default:
		return NULL;
	}
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH && HIWORD(BASS_GetVersion()) != BASSVERSION) {
		MessageBox(NULL, "Incorrect BASS.DLL version (" BASSVERSIONTEXT " is required)", "ASAP", MB_ICONERROR);
		return FALSE;
	}
	return TRUE;
}
