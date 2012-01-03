/*
 * info_dlg.h - file information dialog box
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
#include <tchar.h>

#define IDD_INFO       300
#define IDC_PLAYING    301
#define IDC_FILENAME   302
#define IDC_AUTHOR     303
#define IDC_NAME       304
#define IDC_DATE       305
#define IDC_SONGNO     306
#define IDC_TIME       307
#define IDC_LOOP       308
#define IDC_SAVE       309
#define IDC_CONVERT    310

#define IDD_PROGRESS   500
#define IDC_PROGRESS   501

LPTSTR appendString(LPTSTR dest, LPCTSTR src);
BOOL loadModule(LPCTSTR filename, byte *module, int *module_len);

extern HWND infoDialog;
void showInfoDialog(HINSTANCE hInstance, HWND hwndParent, LPCTSTR filename, int song);
void updateInfoDialog(LPCTSTR filename, int song);
void setPlayingSong(LPCTSTR filename, int song);
