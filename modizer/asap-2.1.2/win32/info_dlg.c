/*
 * info_dlg.c - file information dialog box
 *
 * Copyright (C) 2007-2010  Piotr Fusik
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

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

#include "asap.h"
#include "info_dlg.h"

LPTSTR appendString(LPTSTR dest, LPCTSTR src)
{
	while (*src != '\0')
		*dest++ = *src++;
	*dest = '\0';
	return dest;
}

BOOL loadModule(LPCTSTR filename, byte *module, int *module_len)
{
	HANDLE fh;
	BOOL ok;
	fh = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ok = ReadFile(fh, module, ASAP_MODULE_MAX, (LPDWORD) module_len, NULL);
	CloseHandle(fh);
	return ok;
}

HWND infoDialog = NULL;

#ifndef _UNICODE /* TODO */

static _TCHAR playing_filename[MAX_PATH];
static int playing_song = 0;
static BOOL playing_info = FALSE;
static byte saved_module[ASAP_MODULE_MAX];
static int saved_module_len;
static ASAP_ModuleInfo saved_module_info;
static ASAP_ModuleInfo edited_module_info;
static int edited_song;
static BOOL can_save;
static _TCHAR convert_filename[MAX_PATH];
static LPCTSTR convert_ext;
static int invalid_fields;
#define INVALID_FIELD_AUTHOR      1
#define INVALID_FIELD_NAME        2
#define INVALID_FIELD_DATE        4
#define INVALID_FIELD_TIME        8
#define INVALID_FIELD_TIME_SHOW  16

static void showSongTime(void)
{
	char str[ASAP_DURATION_CHARS];
	ASAP_DurationToString(str, edited_module_info.durations[edited_song]);
	SendDlgItemMessage(infoDialog, IDC_TIME, WM_SETTEXT, 0, (LPARAM) str);
	CheckDlgButton(infoDialog, IDC_LOOP, edited_module_info.loops[edited_song] ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(infoDialog, IDC_LOOP), str[0] != '\0' && (can_save || convert_ext != NULL));
}

static void showEditTip(int nID, LPCTSTR title, LPCTSTR message)
{
#ifndef _UNICODE

#ifndef EM_SHOWBALLOONTIP
/* missing in MinGW */
typedef struct
{
	DWORD cbStruct;
	LPCWSTR pszTitle;
	LPCWSTR pszText;
	INT ttiIcon;
} EDITBALLOONTIP;
#define TTI_ERROR  3
#define EM_SHOWBALLOONTIP  0x1503
#endif
	WCHAR wTitle[64];
	WCHAR wMessage[64];
	EDITBALLOONTIP ebt = { sizeof(EDITBALLOONTIP), wTitle, wMessage, TTI_ERROR };
	if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, title, -1, wTitle, 64) <= 0
	 || MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, message, -1, wMessage, 64) <= 0
	 || !SendDlgItemMessage(infoDialog, nID, EM_SHOWBALLOONTIP, 0, (LPARAM) &ebt))

#endif /* _UNICODE */

		/* Windows before XP don't support balloon tips */
		MessageBox(infoDialog, message, title, MB_OK | MB_ICONERROR);
}

static BOOL infoChanged(void)
{
	int i;
	if (strcmp(saved_module_info.author, edited_module_info.author) != 0)
		return TRUE;
	if (strcmp(saved_module_info.name, edited_module_info.name) != 0)
		return TRUE;
	if (strcmp(saved_module_info.date, edited_module_info.date) != 0)
		return TRUE;
	for (i = 0; i < saved_module_info.songs; i++) {
		if (saved_module_info.durations[i] != edited_module_info.durations[i])
			return TRUE;
		if (edited_module_info.durations[i] >= 0
		 && saved_module_info.loops[i] != edited_module_info.loops[i])
			return TRUE;
	}
	return FALSE;
}

static void updateSaveAndConvertButtons(int mask, BOOL ok)
{
	if (ok) {
		invalid_fields &= ~mask;
		ok = invalid_fields == 0;
	}
	else
		invalid_fields |= mask;
	if (can_save)
		EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), ok && infoChanged());
	if (convert_ext != NULL)
		EnableWindow(GetDlgItem(infoDialog, IDC_CONVERT), ok);
}

static void updateInfoString(HWND hDlg, int nID, int mask, char (*s)[ASAP_INFO_CHARS])
{
	int i;
	BOOL ok = TRUE;
	SendDlgItemMessage(hDlg, nID, WM_GETTEXT, sizeof(*s), (LPARAM) s);
	for (i = 0; (*s)[i] != '\0'; i++) {
		char c = (*s)[i];
		if (c < ' ' || c > 'z' || c == '"' || c == '`') {
			showEditTip(nID, _T("Invalid characters"), _T("Avoid national characters and quotation marks"));
			ok = FALSE;
			break;
		}
	}
	updateSaveAndConvertButtons(mask, ok);
}

static BOOL saveFile(LPCTSTR filename, const byte *data, int len)
{
	HANDLE fh;
	DWORD written;
	BOOL ok;
	fh = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ok = WriteFile(fh, data, len, &written, NULL);
	CloseHandle(fh);
	return ok;
}

static BOOL doSaveInfo(void)
{
	_TCHAR filename[MAX_PATH];
	byte out_module[ASAP_MODULE_MAX];
	int out_len;
	out_len = ASAP_SetModuleInfo(&edited_module_info, saved_module, saved_module_len, out_module);
	if (out_len <= 0)
		return FALSE;
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_GETTEXT, MAX_PATH, (LPARAM) filename);
	if (!saveFile(filename, out_module, out_len))
		return FALSE;
	saved_module_info = edited_module_info;
	showSongTime();
	EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), FALSE);
	return TRUE;
}

static BOOL saveInfo(void)
{
	int song = edited_module_info.songs;
	while (--song >= 0 && edited_module_info.durations[song] < 0);
	while (--song >= 0)
		if (edited_module_info.durations[song] < 0) {
			MessageBox(infoDialog, _T("Cannot save file because time not set for all songs"), _T("Error"), MB_OK | MB_ICONERROR);
			return FALSE;
		}
	if (!doSaveInfo()) {
		MessageBox(infoDialog, _T("Cannot save information to file"), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	return TRUE;
}

static BOOL convert(void)
{
	_TCHAR filename[MAX_PATH];
	byte out_module[ASAP_MODULE_MAX];
	int out_len;
	static _TCHAR filter[32] = _T("*.sap\0*.sap\0All files\0*.*\0");
	static OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		NULL,
		0,
		filter,
		NULL,
		0,
		0,
		convert_filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		_T("Select output file"),
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_GETTEXT, MAX_PATH, (LPARAM) filename);
	out_len = ASAP_Convert(filename, &edited_module_info, saved_module, saved_module_len, out_module);
	if (out_len <= 0) {
		MessageBox(infoDialog, _T("Cannot convert file"), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	ofn.hwndOwner = infoDialog;
	memcpy(filter + 2, convert_ext, 3 * sizeof(_TCHAR));
	memcpy(filter + 8, convert_ext, 3 * sizeof(_TCHAR));
	ofn.lpstrDefExt = convert_ext;
	if (!GetSaveFileName(&ofn))
		return FALSE;
	if (!saveFile(convert_filename, out_module, out_len)) {
		MessageBox(infoDialog, _T("Cannot save file"), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (_tcscmp(convert_ext, _T("sap")) == 0)
		saved_module_info = edited_module_info;
	return TRUE;
}

static INT_PTR CALLBACK infoDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_PLAYING, playing_info ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(hDlg, IDC_AUTHOR, EM_LIMITTEXT, ASAP_INFO_CHARS - 1, 0);
		SendDlgItemMessage(hDlg, IDC_NAME, EM_LIMITTEXT, ASAP_INFO_CHARS - 1, 0);
		SendDlgItemMessage(hDlg, IDC_DATE, EM_LIMITTEXT, ASAP_INFO_CHARS - 1, 0);
		SendDlgItemMessage(hDlg, IDC_TIME, EM_LIMITTEXT, ASAP_DURATION_CHARS - 1, 0);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case MAKEWPARAM(IDC_PLAYING, BN_CLICKED):
			playing_info = (IsDlgButtonChecked(hDlg, IDC_PLAYING) == BST_CHECKED);
			if (playing_info && playing_filename[0] != '\0')
				updateInfoDialog(playing_filename, playing_song);
			return TRUE;
		case MAKEWPARAM(IDC_AUTHOR, EN_CHANGE):
			updateInfoString(hDlg, IDC_AUTHOR, INVALID_FIELD_AUTHOR, &edited_module_info.author);
			return TRUE;
		case MAKEWPARAM(IDC_NAME, EN_CHANGE):
			updateInfoString(hDlg, IDC_NAME, INVALID_FIELD_NAME, &edited_module_info.name);
			return TRUE;
		case MAKEWPARAM(IDC_DATE, EN_CHANGE):
			updateInfoString(hDlg, IDC_DATE, INVALID_FIELD_DATE, &edited_module_info.date);
			return TRUE;
		case MAKEWPARAM(IDC_TIME, EN_CHANGE):
			{
				char str[ASAP_DURATION_CHARS];
				int duration;
				SendDlgItemMessage(hDlg, IDC_TIME, WM_GETTEXT, sizeof(str), (LPARAM) str);
				duration = ASAP_ParseDuration(str);
				edited_module_info.durations[edited_song] = duration;
				EnableWindow(GetDlgItem(infoDialog, IDC_LOOP), str[0] != '\0');
				updateSaveAndConvertButtons(INVALID_FIELD_TIME | INVALID_FIELD_TIME_SHOW, duration >=0 || str[0] == '\0');
			}
			return TRUE;
		case MAKEWPARAM(IDC_TIME, EN_KILLFOCUS):
			if ((invalid_fields & INVALID_FIELD_TIME_SHOW) != 0) {
				invalid_fields &= ~INVALID_FIELD_TIME_SHOW;
				showEditTip(IDC_TIME, _T("Invalid format"), _T("Please type MM:SS.mmm"));
			}
			return TRUE;
		case MAKEWPARAM(IDC_LOOP, BN_CLICKED):
			edited_module_info.loops[edited_song] = (IsDlgButtonChecked(hDlg, IDC_LOOP) == BST_CHECKED);
			updateSaveAndConvertButtons(0, TRUE);
			return TRUE;
		case MAKEWPARAM(IDC_SONGNO, CBN_SELCHANGE):
			edited_song = SendDlgItemMessage(hDlg, IDC_SONGNO, CB_GETCURSEL, 0, 0);
			showSongTime();
			updateSaveAndConvertButtons(INVALID_FIELD_TIME | INVALID_FIELD_TIME_SHOW, TRUE);
			return TRUE;
		case MAKEWPARAM(IDC_SAVE, BN_CLICKED):
			saveInfo();
			return TRUE;
		case MAKEWPARAM(IDC_CONVERT, BN_CLICKED):
			convert();
			return TRUE;
		case MAKEWPARAM(IDCANCEL, BN_CLICKED):
			if (invalid_fields == 0 && infoChanged()) {
				BOOL ok;
				switch (MessageBox(hDlg, can_save ? _T("Save changes?") : _T("Convert to SAP?"), _T("ASAP"), MB_YESNOCANCEL | MB_ICONQUESTION)) {
				case IDYES:
					ok = can_save ? saveInfo() : convert();
					if (!ok)
						return TRUE;
					break;
				case IDCANCEL:
					return TRUE;
				default:
					break;
				}
			}
			DestroyWindow(hDlg);
			infoDialog = NULL;
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

void showInfoDialog(HINSTANCE hInstance, HWND hwndParent, LPCTSTR filename, int song)
{
	if (infoDialog == NULL) {
		edited_module_info = saved_module_info;
		infoDialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_INFO), hwndParent, infoDialogProc);
	}
	if (playing_info || filename == NULL)
		updateInfoDialog(playing_filename, playing_song);
	else
		updateInfoDialog(filename, song);
}

void updateInfoDialog(LPCTSTR filename, int song)
{
	BOOL can_edit;
	int i;
	if (infoDialog == NULL || infoChanged())
		return;
	if (!loadModule(filename, saved_module, &saved_module_len)
	 || !ASAP_GetModuleInfo(&saved_module_info, filename, saved_module, saved_module_len)) {
		DestroyWindow(infoDialog);
		infoDialog = NULL;
		return;
	}
	edited_module_info = saved_module_info;
	can_save = ASAP_CanSetModuleInfo(filename);
	convert_ext = ASAP_CanConvert(filename, &edited_module_info, saved_module, saved_module_len);
	invalid_fields = 0;
	can_edit = can_save || convert_ext != NULL;
	SendDlgItemMessage(infoDialog, IDC_AUTHOR, EM_SETREADONLY, !can_edit, 0);
	SendDlgItemMessage(infoDialog, IDC_NAME, EM_SETREADONLY, !can_edit, 0);
	SendDlgItemMessage(infoDialog, IDC_DATE, EM_SETREADONLY, !can_edit, 0);
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_SETTEXT, 0, (LPARAM) filename);
	SendDlgItemMessage(infoDialog, IDC_AUTHOR, WM_SETTEXT, 0, (LPARAM) saved_module_info.author);
	SendDlgItemMessage(infoDialog, IDC_NAME, WM_SETTEXT, 0, (LPARAM) saved_module_info.name);
	SendDlgItemMessage(infoDialog, IDC_DATE, WM_SETTEXT, 0, (LPARAM) saved_module_info.date);
	SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_RESETCONTENT, 0, 0);
	EnableWindow(GetDlgItem(infoDialog, IDC_SONGNO), saved_module_info.songs > 1);
	for (i = 1; i <= saved_module_info.songs; i++) {
		char str[16];
		sprintf(str, "%d", i);
		SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_ADDSTRING, 0, (LPARAM) str);
	}
	if (song < 0)
		song = saved_module_info.default_song;
	SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_SETCURSEL, song, 0);
	edited_song = song;
	showSongTime();
	SendDlgItemMessage(infoDialog, IDC_TIME, EM_SETREADONLY, !can_edit, 0);
	EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), FALSE);
	if (convert_ext != NULL) {
		_TCHAR convert_command[24] = _T("&Convert to ");
		i = 0;
		do
			convert_command[12 + i] = convert_ext[i] >= 'a' ? convert_ext[i] - 'a' + 'A' : convert_ext[i];
		while (convert_ext[i++] != '\0');
		SendDlgItemMessage(infoDialog, IDC_CONVERT, WM_SETTEXT, 0, (LPARAM) convert_command);
		_tcscpy(convert_filename, filename);
		ASAP_ChangeExt(convert_filename, convert_ext);
		EnableWindow(GetDlgItem(infoDialog, IDC_CONVERT), TRUE);
	}
	else {
		SendDlgItemMessage(infoDialog, IDC_CONVERT, WM_SETTEXT, 0, (LPARAM) "&Convert");
		EnableWindow(GetDlgItem(infoDialog, IDC_CONVERT), FALSE);
	}
}

void setPlayingSong(LPCTSTR filename, int song)
{
	if (filename != NULL && _tcslen(filename) < MAX_PATH - 1)
		_tcscpy(playing_filename, filename);
	playing_song = song;
	if (playing_info)
		updateInfoDialog(playing_filename, song);
}

#endif /* _UNICODE */
