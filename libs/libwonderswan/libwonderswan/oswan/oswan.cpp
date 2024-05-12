////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

//#include <windows.h>
#include "wsr_types.h" //YOYOFR

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
//#include <commctrl.h>
//#include <direct.h>
#include <time.h>
//#include <shlobj.h>
//#include <mbstring.h>
#include <sys/types.h>
#include <sys/stat.h>

//#include "resource.h"
#include "oswan.h"
#include "ws.h"
//#include "draw.h"
#include "input.h"
#include "config.h"
#include "gpu.h"
#include "audio.h"
#include "io.h"
#include "log.h"
#include "rom.h"
//#include "mapviewer.h"
//#include "witch.h"

#define OSWAN_CLASSNAME "OswanWndclass"

HWND  hWnd              = NULL;
HWND  hSBWnd            = NULL;
int   app_new_rom       = 0;
int   app_gameRunning   = 0;
int   DrawFilter        = FILTER_NORMAL;
//int   DrawSize          = DS_2;
int   DrawFull          = 0;
int   DrawMode          = 0;
int   OldMode           = 0;
BYTE  OldPort15         = 0xFF;
int   LCDSegment        = 0;
char  recentOfn0[1024]  = "";
char  recentOfn1[1024]  = "";
char  recentOfn2[1024]  = "";
char  recentOfn3[1024]  = "";
char  recentOfn4[1024]  = "";
char* recent[5]         = {recentOfn0, recentOfn1, recentOfn2, recentOfn3, recentOfn4};
char  WsHomeDir[1024]   = "";
char  WsRomDir[1024]    = "";
char  WsSramDir[1024]   = "";
char  WsSaveDir[1024]   = "";
char  WsShotDir[1024]   = "";
char  WsWavDir[1024]    = "";
char  BaseName[256]     = "";
int   ws_colourScheme   = COLOUR_SCHEME_DEFAULT;
int   ws_system         = WS_SYSTEM_COLOR;
char  ws_rom_path[1024] = "";
int   ScrShoot          = 0;
int   StatusWait        = 0;
DWORD backbuffer[224 * 144];

static HINSTANCE hInst          = NULL;
static char      log_path[1024] = "";

/////////////////////////////////////////////////////////////////////////////////////////////////
// static functions
/////////////////////////////////////////////////////////////////////////////////////////////////
//void ParseCommandLine(LPSTR lpCmdLine);
//void oswan_set_radio_menu(int menu_id, int count, int select);

//void oswan_drop_files(HDROP hDrop);
//void oswan_open_file();
//void oswan_load_state(int slot);
//void oswan_load_state_with_file_name();
//void oswan_save_state(int slot);
//void oswan_save_state_with_file_name();
//void oswan_set_system(int system);
//void oswan_set_video_size(int size);
//void oswan_set_color_scheme(int color_scheme);
//void oswan_set_filter(int filter);
//void oswan_set_channel_enabled(int channel);
//void oswan_set_sound_device(int device_id);
//void oswan_set_sampling_rate(int sampling_rate);
//void oswan_set_buffer_size(int buffer_size);

//void oswan_open_rom(const char* path);

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void oswan_set_status_text(int wait, const char* format, ...)
{
  va_list ap;
  char    s[128];

  va_start(ap, format);
  vsprintf(s, format, ap);
  va_end(ap);

  //SendMessage(hSBWnd, SB_SETTEXT, 255 | 0, (LPARAM) s);
  StatusWait = wait;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//void oswan_set_recent_rom()
//{
//  int          i;
//  char*        temp;
//  BYTE*        filename;
//  char         buf[256];
//  MENUITEMINFO minfo;
//  minfo.cbSize     = sizeof(MENUITEMINFO);
//  minfo.fMask      = MIIM_STATE | MIIM_TYPE;
//  minfo.fType      = MFT_STRING;
//  minfo.fState     = MFS_ENABLED;
//  minfo.dwTypeData = buf;
//
//  if (ws_rom_path[0]) {
//    for (i = 0; i < 5; i++)
//      if (strcmp(ws_rom_path, recent[i]) == 0) {
//        temp = recent[i];
//       while (i) {
//          recent[i] = recent[i - 1];
//          i--;
//       }
//        recent[0] = temp;
//        break;
//      }
//
//    if (i) {
//      temp      = recent[4];
//      recent[4] = recent[3];
//      recent[3] = recent[2];
//      recent[2] = recent[1];
//      recent[1] = recent[0];
//      recent[0] = temp;
//      strcpy(recent[0], ws_rom_path);
//    }
//  }
//
//  HMENU menu = GetMenu(hWnd);
//  for (i = 0; i < 5; i++) {
//    if (recent[i][0]) {
//      filename = _mbsrchr((BYTE*) recent[i], '\\');
//      sprintf(buf, "&%d %s", i + 1, ++filename);
//    }
//    else
//      buf[0] = '\0';
//
//    SetMenuItemInfo(menu, ID_FILE_RECENT0 + i, FALSE, &minfo);
//  }
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//void SetStateInfo(void)
//{
//  char         temp[1024];
//  struct _stat fstat;
//  struct tm*   ftm;
//  HMENU        menu;
//  MENUITEMINFO minfo;
//
//  menu = GetMenu(hWnd);
//
//  minfo.cbSize     = sizeof(MENUITEMINFO);
//  minfo.fMask      = MIIM_TYPE | MIIM_STATE;
//  minfo.fType      = MFT_STRING;
//  minfo.dwTypeData = temp;
//
//  for (int i = 0; i < 4; i++) {
//    sprintf(temp, "%s%s.%03d", WsSaveDir, BaseName, i);
//    if (_stat(temp, &fstat)) {
//      sprintf(temp, "Slot #&%d  empty\tShift+F%d", i + 1, i + 5);
//      minfo.fState = MFS_ENABLED;
//      SetMenuItemInfo(menu, ID_FILE_SAVE_STATE1 + i, FALSE, &minfo);
//
//      sprintf(temp, "Slot #&%d  empty\tF%d", i + 1 , i + 5);
//      minfo.fState = MFS_GRAYED;
//      SetMenuItemInfo(menu, ID_FILE_LOAD_STATE1 + i, FALSE, &minfo);
//    }
//    else {
//      ftm = localtime(&fstat.st_mtime);
//
//      sprintf(temp, "Slot #&%d  %04d/%02d/%02d %02d:%02d:%02d\tShift+F%d",
//              i + 1 , ftm->tm_year + 1900, ftm->tm_mon,
//              ftm->tm_mday, ftm->tm_hour, ftm->tm_min, ftm->tm_sec, i + 5);
//      minfo.fState = MFS_ENABLED;
//      SetMenuItemInfo(menu, ID_FILE_SAVE_STATE1 + i, FALSE, &minfo);
//
//      sprintf(temp, "Slot #&%d  %04d/%02d/%02d %02d:%02d:%02d\tF%d",
//              i + 1 , ftm->tm_year + 1900, ftm->tm_mon, 
//              ftm->tm_mday, ftm->tm_hour, ftm->tm_min, ftm->tm_sec, i + 5);
//      SetMenuItemInfo(menu, ID_FILE_LOAD_STATE1 + i, FALSE, &minfo);
//    }
//  }
//
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//void WsResize(void)
//{
//  WsDrawRelease();
//  WsDrawCreate(hWnd);
//
//  if (DrawFull) {
//    SetWindowPos(hWnd  , NULL, 0, 0, 640, 480, SWP_SHOWWINDOW);
//    SetWindowPos(hSBWnd, NULL, 0, 0,   0,   0, SWP_HIDEWINDOW);
//    WsDrawClear();
//    return;
//  }
//
//  RECT wind;
//  RECT wind2;
//  RECT bar;
//  int  lcdHeight = 144;
//  int  lcdWidth  = 224;
//  int  barHeight;
//  int  client_width;
//  int  client_height;
//
//  GetWindowRect(hSBWnd, &bar);
//  barHeight = bar.bottom - bar.top;
//
//  if (LCDSegment) {
//    if (ws_system == WS_SYSTEM_MONO)
//      lcdHeight += 8;
//    else
//      lcdWidth  += 8;
//
//    OldPort15 = 0xff;
//  }
//
//  if (DrawMode & 0x01) {
//    client_width  = lcdHeight * DrawSize;
//    client_height = lcdWidth  * DrawSize + barHeight;
//  }
//  else {
//    client_width  = lcdWidth  * DrawSize;
//    client_height = lcdHeight * DrawSize + barHeight;
//  }
//  
//  wind.top    = 0;
//  wind.left   = 0;
//  wind.right  = client_width;
//  wind.bottom = client_height;
//  AdjustWindowRectEx(&wind, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
//                     TRUE, WS_EX_APPWINDOW | WS_EX_ACCEPTFILES);
//
//  wind2 = wind;
//  //SendMessage(hWnd, WM_NCCALCSIZE, FALSE, (LPARAM) &wind2);
//
//  SetWindowPos(hWnd, NULL, 0, 0,
//               (wind.right  - wind.left) + client_width  - (wind2.right  - wind2.left),
//               (wind.bottom - wind.top ) + client_height - (wind2.bottom - wind2.top ),
//               SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
//
//  SetWindowPos(hSBWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//int CALLBACK callbackGetFolderName(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
//{
//    if(uMsg == BFFM_INITIALIZED)
//	{
//        //SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
//    }
//    return 0;
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//void GetFolderName(char *folder)
//{
//	BROWSEINFO      bi;
//    LPSTR           lpBuffer;
//    LPITEMIDLIST    pidlRoot;
//    LPITEMIDLIST    pidlBrowse;
//    LPMALLOC        lpMalloc = NULL;
//
//	HRESULT hr = SHGetMalloc(&lpMalloc);
//	if(FAILED(hr)) return;
//
//	if ((lpBuffer = (LPSTR) lpMalloc->Alloc(_MAX_PATH)) == NULL) {
//		return;
//	}
//
//	if (!SUCCEEDED(SHGetSpecialFolderLocation(  hWnd, CSIDL_DESKTOP, &pidlRoot))) { 
//		lpMalloc->Free(lpBuffer);
//		return;
//	}
//
//	bi.hwndOwner = hWnd;
//	bi.pidlRoot = pidlRoot;
//	bi.pszDisplayName = lpBuffer;
//	bi.lpszTitle = "Select Folder";
//	bi.ulFlags = 0;
//	bi.lpfn = callbackGetFolderName;
//	bi.lParam = (LPARAM)folder;
//
//	pidlBrowse = SHBrowseForFolder(&bi);
//	if (pidlBrowse != NULL) {
//
//		if (SHGetPathFromIDList(pidlBrowse, lpBuffer)) {
//			strcpy(folder, lpBuffer);
//		}
//		lpMalloc->Free(pidlBrowse);
//	}
//	lpMalloc->Free(pidlRoot); 
//	lpMalloc->Free(lpBuffer);
//	lpMalloc->Release();
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//void GetBaseName(char *path)
//{
//	char *filename = (char*)_mbsrchr((BYTE*)path, '\\');
//	strcpy(BaseName, filename);
//	if (!stricmp(&BaseName[strlen(BaseName) - 4], ".wsc")) {
//		BaseName[strlen(BaseName) - 4] = 0;
//	}
//	else if (!stricmp(&BaseName[strlen(BaseName) - 3], ".ws")) {
//		BaseName[strlen(BaseName) - 3] = 0;
//	}
//	else if (!stricmp(&BaseName[strlen(BaseName) - 4], ".zip")) {
//		BaseName[strlen(BaseName) - 4] = 0;
//	}
//	else BaseName[0] = 0;
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//void WsPause(void)
//{
//  HMENU	menu = GetMenu(hWnd);
//
//  if (app_gameRunning) {
//    ws_audio_stop();
//    app_gameRunning = 0;
//    CheckMenuItem(menu, ID_FILE_PAUSE,MF_CHECKED);
//    oswan_set_status_text(0, "Pause");
//  }
//  else if (ws_rom_path[0]) {
//    app_gameRunning = 1;
//    CheckMenuItem(menu,ID_FILE_PAUSE,MF_UNCHECKED);
//  }
//}
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//void WsReset(void)
//{
//  if (ws_rom_path[0]) {
//    ws_reset();
//    oswan_set_status_text(1, "Reset");
//  }
//}
//
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//void WsStop(void)
//{
//  if (!app_gameRunning)
//    return;
//
//	HMENU	menu;
//	int		i;
//
//	WsDrawClear();
//	WsWaveClose();
//    ws_audio_stop();
//	app_gameRunning = 0;
//	if (ws_rom_path[0]) {
//		ws_save_sram(ws_rom_path);
//		ws_rom_path[0] = '\0';
//	}
//	//WsResize();
//	menu = GetMenu(hWnd);
//	EnableMenuItem(menu, ID_OPTIONS_WS_COLOR, MF_ENABLED);
//	EnableMenuItem(menu, ID_OPTIONS_WS_MONO, MF_ENABLED);
//	CheckMenuItem(menu,ID_FILE_PAUSE,MF_UNCHECKED);
//	EnableMenuItem(menu, ID_OPTIONS_SOUND_DIRECTSOUND, MF_ENABLED);
//	EnableMenuItem(menu, ID_OPTIONS_SOUND_WAVEOUT, MF_ENABLED);
//	for (i = 0; i < 4; i++) {
//		EnableMenuItem(menu, ID_FILE_LOAD_STATE1 + i, MF_GRAYED);
//		EnableMenuItem(menu, ID_FILE_SAVE_STATE1 + i, MF_GRAYED);
//	}
//	SetWindowText(hWnd, OSWAN_CAPTION);
//    oswan_set_status_text(0, "Stop");
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//LRESULT CALLBACK DirectorySetProc(HWND hDlgWnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	HWND	hDlgItem;
//
//	static char ws_rom_dir_tmp[1024];
//	static char ws_sram_dir_tmp[1024];
//	static char ws_save_dir_tmp[1024];
//	static char ws_shot_dir_tmp[1024];
//	static char ws_wav_dir_tmp[1024];
//
//	switch (msg)
//	{
//	case WM_INITDIALOG:
//		strcpy(ws_rom_dir_tmp, WsRomDir);
//		strcpy(ws_sram_dir_tmp, WsSramDir);
//		strcpy(ws_save_dir_tmp, WsSaveDir);
//		strcpy(ws_shot_dir_tmp, WsShotDir);
//		strcpy(ws_wav_dir_tmp, WsWavDir);
//		hDlgItem = GetDlgItem(hDlgWnd, IDC_ROM_DIR);
//		SetWindowText(hDlgItem, WsRomDir);
//		hDlgItem = GetDlgItem(hDlgWnd, IDC_SRAM_DIR);
//		SetWindowText(hDlgItem, WsSramDir);
//		hDlgItem = GetDlgItem(hDlgWnd, IDC_SAVE_DIR);
//		SetWindowText(hDlgItem, WsSaveDir);
//		hDlgItem = GetDlgItem(hDlgWnd, IDC_SCRSHOT_DIR);
//		SetWindowText(hDlgItem, WsShotDir);
//		hDlgItem = GetDlgItem(hDlgWnd, IDC_WAV_DIR);
//		SetWindowText(hDlgItem, WsWavDir);
//		SetFocus(GetDlgItem(hDlgWnd, IDOK));
//		break;
//	case WM_DESTROY:
//		WsDrawFlip();
//		break;
//	case WM_COMMAND:
//		if(HIWORD(wParam)==BN_CLICKED)
//		{
//			switch (LOWORD(wParam))
//			{
//			case IDC_BUTTON_ROM_DIR:
//				GetFolderName(ws_rom_dir_tmp);
//				hDlgItem = GetDlgItem(hDlgWnd, IDC_ROM_DIR);
//				SetWindowText(hDlgItem, ws_rom_dir_tmp);
//				break;
//			case IDC_BUTTON_SRAM_DIR:
//				GetFolderName(ws_sram_dir_tmp);
//				hDlgItem = GetDlgItem(hDlgWnd, IDC_SRAM_DIR);
//				SetWindowText(hDlgItem, ws_sram_dir_tmp);
//				break;
//			case IDC_BUTTON_SAVE_DIR:
//				GetFolderName(ws_save_dir_tmp);
//				hDlgItem = GetDlgItem(hDlgWnd, IDC_SAVE_DIR);
//				SetWindowText(hDlgItem, ws_save_dir_tmp);
//				break;
//			case IDC_BUTTON_SCRSHOT_DIR:
//				GetFolderName(ws_shot_dir_tmp);
//				hDlgItem = GetDlgItem(hDlgWnd, IDC_SCRSHOT_DIR);
//				SetWindowText(hDlgItem, ws_shot_dir_tmp);
//				break;
//			case IDC_BUTTON_WAV_DIR:
//				GetFolderName(ws_wav_dir_tmp);
//				hDlgItem = GetDlgItem(hDlgWnd, IDC_WAV_DIR);
//				SetWindowText(hDlgItem, ws_wav_dir_tmp);
//				break;
//			case IDOK:
//				strcpy(WsRomDir, ws_rom_dir_tmp);
//				strcpy(WsSramDir, ws_sram_dir_tmp);
//				strcpy(WsSaveDir, ws_save_dir_tmp);
//				strcpy(WsShotDir, ws_shot_dir_tmp);
//				strcpy(WsWavDir, ws_wav_dir_tmp);
//				EndDialog(hDlgWnd,LOWORD(wParam));
//				break;
//			case IDCANCEL:
//				EndDialog(hDlgWnd,LOWORD(wParam));
//				break;
//			}
//			return 1;
//		}
//		break;
//	case WM_MOVE:
//		WsDrawFlip();
//		break;
//	}
//
//	return 0;
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//LRESULT CALLBACK AboutProc(HWND hDlgWnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	HWND	hDlgItem;
//	char	buf[256];
//
//	switch (msg)
//	{
//	case WM_INITDIALOG:
//		sprintf(buf, "%s %s", OSWAN_CAPTION, OSWAN_VERSION);
//		hDlgItem = GetDlgItem(hDlgWnd, IDC_STATIC_VERSION);
//		SetWindowText(hDlgItem, buf);
//		SetFocus(GetDlgItem(hDlgWnd, IDOK));
//		break;
//	case WM_COMMAND:
//		if(HIWORD(wParam)==BN_CLICKED)
//		{
//			switch (LOWORD(wParam))
//			{
//			case IDOK:
//			case IDCANCEL:
//				EndDialog(hDlgWnd,LOWORD(wParam));
//				WsDrawFlip();
//				break;
//			}
//			return 1;
//		}
//		break;
//	case WM_MOVE:
//		WsDrawFlip();
//		break;
//	}
//	return 0;
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#if 0
HRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HMENU menu;

  switch (msg) {
  case WM_CREATE:
    InitCommonControls();
    hSBWnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM, NULL, hWnd, ID_STATUS);
    break;
  case WM_SYSCOMMAND:
    ws_audio_stop();
    break;
  case WM_MENUSELECT:
    WsDrawFlip();
    break;
  case WM_DROPFILES:
    oswan_drop_files((HDROP) wParam);
	break;
/*
  case MM_WOM_DONE:
    if (ws_audio_is_playing())
      WsWaveOutProc();
    
    break;
*/
  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case ID_FILE_OPENROM:
      oswan_open_file();
      break;
    case ID_FILE_RECENT0:
      oswan_open_rom(recent[0]);
      break;
    case ID_FILE_RECENT1:
      oswan_open_rom(recent[1]);
      break;
    case ID_FILE_RECENT2:
      oswan_open_rom(recent[2]);
      break;
    case ID_FILE_RECENT3:
      oswan_open_rom(recent[3]);
      break;
    case ID_FILE_RECENT4:
      oswan_open_rom(recent[4]);
      break;
    case ID_FILE_LOAD_STATE1:
      oswan_load_state(0);
      break;
    case ID_FILE_LOAD_STATE2:
      oswan_load_state(1);
      break;
    case ID_FILE_LOAD_STATE3:
      oswan_load_state(2);
      break;
    case ID_FILE_LOAD_STATE4:
      oswan_load_state(3);
      break;
    case ID_FILE_LOAD_STATE5:
      oswan_load_state_with_file_name();
      break;
    case ID_FILE_SAVE_STATE1:
      oswan_save_state(0);
      break;
    case ID_FILE_SAVE_STATE2:
      oswan_save_state(1);
      break;
    case ID_FILE_SAVE_STATE3:
      oswan_save_state(2);
      break;
    case ID_FILE_SAVE_STATE4:
      oswan_save_state(3);
      break;
    case ID_FILE_SAVE_STATE5:
      oswan_save_state_with_file_name();
      break;
    case ID_FILE_PAUSE:
      WsPause();
      break;
    case ID_FILE_RESET:
      WsReset();
      break;
    case ID_FILE_STOP:
      WsStop();
      break;
    case ID_FILE_SCREENSHOT:
      if (ws_rom_path[0])
        ScrShoot = 1;

      break;
    case ID_FILE_EXIT:
      PostQuitMessage(0);
      break;
    case ID_OPTIONS_WS_COLOR:
      oswan_set_system(WS_SYSTEM_COLOR);
      break;
    case ID_OPTIONS_WS_MONO:
      oswan_set_system(WS_SYSTEM_MONO);
      break;
    case ID_OPTIONS_LCDSEGMENT_ON:
      menu = GetMenu(hWnd);
      if (LCDSegment) {
        CheckMenuItem(menu, ID_OPTIONS_LCDSEGMENT_ON, MF_UNCHECKED);
        LCDSegment = 0;
      }
      else {
        CheckMenuItem(menu, ID_OPTIONS_LCDSEGMENT_ON, MF_CHECKED);
        LCDSegment = 1;
      }
      WsResize();
      break;
    case ID_OPTIONS_VIDEO_SIZE_1:
      oswan_set_video_size(DS_1);
      break;
    case ID_OPTIONS_VIDEO_SIZE_2:
      oswan_set_video_size(DS_2);
      break;
    case ID_OPTIONS_VIDEO_SIZE_3:
      oswan_set_video_size(DS_3);
      break;
    case ID_OPTIONS_VIDEO_SIZE_4:
      oswan_set_video_size(DS_4);
      break;
    case ID_OPTIONS_VIDEO_FULL:
      menu = GetMenu(hWnd);
      if (DrawFull) {
        CheckMenuItem(menu,ID_OPTIONS_VIDEO_FULL,MF_UNCHECKED);
        lpDD->RestoreDisplayMode();
        DrawFull = 0;
        FullFrag = 0;
      }
      else {
        CheckMenuItem(menu,ID_OPTIONS_VIDEO_FULL,MF_CHECKED);
        DrawFull = 1;
      }
      WsResize();
      break;
    case ID_WS_COLOR_DEFAULT:
      oswan_set_color_scheme(COLOUR_SCHEME_DEFAULT);
      break;
    case ID_WS_COLOR_AMBER:
      oswan_set_color_scheme(COLOUR_SCHEME_AMBER);
      break;
    case ID_WS_COLOR_GREEN:
      oswan_set_color_scheme(COLOUR_SCHEME_GREEN);
      break;
    case ID_OPTIONS_FILTER_NORMAL:
      oswan_set_filter(FILTER_NORMAL);
      break;
    case ID_OPTIONS_FILTER_SCANLINES:
      oswan_set_filter(FILTER_SCANLINES);
      break;
    case ID_OPTIONS_FILTER_HALFSCAN:
      oswan_set_filter(FILTER_HALFSCAN);
      break;
    case ID_OPTIONS_FILTER_SIMPLE2X:
      oswan_set_filter(FILTER_SIMPLE2X);
      break;
    case ID_OPTIONS_SOUND_CH1:
      oswan_set_channel_enabled(0);
      break;
    case ID_OPTIONS_SOUND_CH2:
      oswan_set_channel_enabled(1);
      break;
    case ID_OPTIONS_SOUND_CH3:
      oswan_set_channel_enabled(2);
      break;
    case ID_OPTIONS_SOUND_CH4:
      oswan_set_channel_enabled(3);
      break;
    case ID_OPTIONS_SOUND_VOICE:
      oswan_set_channel_enabled(4);
      break;
    case ID_OPTIONS_SOUND_SWEEP:
      oswan_set_channel_enabled(6);
      break;
    case ID_OPTIONS_SOUND_NOISE:
      oswan_set_channel_enabled(5);
      break;
    case ID_OPTIONS_SOUND_HVOICE:
      oswan_set_channel_enabled(7);
      break;
    case ID_OPTIONS_SOUND_DIRECTSOUND:
      oswan_set_sound_device(1);
      break;
    case ID_OPTIONS_SOUND_WAVEOUT:
      oswan_set_sound_device(0);
      break;
    case ID_OPTIONS_SOUND_BPS_12000:
      oswan_set_sampling_rate(12000);
      break;
    case ID_OPTIONS_SOUND_BPS_24000:
      oswan_set_sampling_rate(24000);
      break;
    case ID_OPTIONS_SOUND_BPS_48000:
      oswan_set_sampling_rate(48000);
      break;
    case ID_OPTIONS_SOUND_BUFFER_10:
      oswan_set_buffer_size(10);
      break;
    case ID_OPTIONS_SOUND_BUFFER_20:
      oswan_set_buffer_size(20);
      break;
    case ID_OPTIONS_SOUND_BUFFER_40:
      oswan_set_buffer_size(40);
      break;
    case ID_OPTIONS_SOUND_BUFFER_80:
      oswan_set_buffer_size(80);
      break;
    case ID_OPTIONS_SOUND_BUFFER_160:
      oswan_set_buffer_size(160);
      break;
    case ID_OPTIONS_SOUND_BUFFER_320:
      oswan_set_buffer_size(320);
      break;
    case ID_OPTIONS_SOUND_REC_START:
      menu = GetMenu(hWnd);
      WsWaveOpen();
      if (ws_audio_is_recording()) {
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_START, MF_GRAYED );
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_STOP , MF_ENABLED);
      }
      else {
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_START, MF_ENABLED);
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_STOP , MF_GRAYED );
      }
      break;
    case ID_OPTIONS_SOUND_REC_STOP:
      menu = GetMenu(hWnd);
      WsWaveClose();
      if (!ws_audio_is_recording()) {
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_START, MF_ENABLED);
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_STOP , MF_GRAYED );
      }
      else {
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_START, MF_GRAYED );
        EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_STOP , MF_ENABLED);
      }
      break;
    case ID_OPTIONS_LAYER_BACKGROUND:
      menu = GetMenu(hWnd);
      if (layer1_on) {
        layer1_on = 0;
        CheckMenuItem(menu,ID_OPTIONS_LAYER_BACKGROUND,MF_UNCHECKED);
      }
      else {
        layer1_on = 1;
        CheckMenuItem(menu,ID_OPTIONS_LAYER_BACKGROUND,MF_CHECKED);
      }
      break;
    case ID_OPTIONS_LAYER_FOREGROUND:
      menu = GetMenu(hWnd);
      if (layer2_on) {
        layer2_on = 0;
        CheckMenuItem(menu,ID_OPTIONS_LAYER_FOREGROUND,MF_UNCHECKED);
      }
      else {
        layer2_on = 1;
        CheckMenuItem(menu,ID_OPTIONS_LAYER_FOREGROUND,MF_CHECKED);
      }
      break;
    case ID_OPTIONS_LAYER_SPRITE:
      menu = GetMenu(hWnd);
      if (sprite_on) {
        sprite_on = 0;
        CheckMenuItem(menu,ID_OPTIONS_LAYER_SPRITE,MF_UNCHECKED);
      }
      else {
        sprite_on = 1;
        CheckMenuItem(menu,ID_OPTIONS_LAYER_SPRITE,MF_CHECKED);
      }
      break;
    case ID_OPTIONS_CONFIGURE_KEY:
      WsInputRelease();
      DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONFIG),
				hWnd,(DLGPROC) GuiConfigKeyboardProc);
      WsInputInit(hWnd);
      break;
    case ID_OPTIONS_CONFIGURE_JOY:
      WsInputJoyRelease();
      DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONFIG),
				hWnd, (DLGPROC) GuiConfigJoypadProc);
      WsInputJoyInit(hWnd);
      break;
    case ID_OPTIONS_DIRECTORIES:
      DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_DIR_SET),
				hWnd, (DLGPROC) DirectorySetProc);
      break;
    case ID_WITCH_OPEN:
      hWitchWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_WITCH),
                               hWnd, (DLGPROC) WitchProc);
      ShowWindow(hWitchWnd, SW_SHOW);
      break;
    case ID_HELP_ABOUT:
      DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT),
                hWnd, (DLGPROC) AboutProc);
      break;
    case ID_VIDEO_ROTATE_LEFT:
      DrawMode = (DrawMode + 1) % 4;
      break;
    case ID_VIDEO_ROTATE_RIGHT:
      DrawMode = (DrawMode + 3) % 4;
      break;
    case ID_VIDEO_FRAME_SKIP_UP:
      FrameSkip = (FrameSkip + 1) % 10;
      break;
    case ID_VIDEO_FRAME_SKIP_DOWN:
      FrameSkip = (FrameSkip + 9) % 10;
      break;
    case ID_SOUND_VOLUME_UP:
      ws_audio_set_main_volume(ws_audio_get_main_volume() + 1);
      oswan_set_status_text(1, "Main volume = %d", ws_audio_get_main_volume());
      break;
    case ID_SOUND_VOLUME_DOWN:
      ws_audio_set_main_volume(ws_audio_get_main_volume() - 1);
      oswan_set_status_text(1, "Main volume = %d", ws_audio_get_main_volume());
      break;
    case ID_DEBUG_MAP:
      if (ws_rom_path[0]) {
        hMapWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_MAP_VIEWER),
                               hWnd, (DLGPROC) MapViewerProc);
        ShowWindow(hMapWnd, SW_SHOW);
      }
      break;
    }
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//void InitMenu(void)
//{
//  HMENU menu = GetMenu(hWnd);
//
//  oswan_set_radio_menu(ID_OPTIONS_WS_MONO     , 2, ws_system      );
//  oswan_set_radio_menu(ID_OPTIONS_VIDEO_SIZE_1, 4, DrawSize - DS_1);
//
//  if (DrawFull)
//    CheckMenuItem(menu, ID_OPTIONS_VIDEO_FULL, MF_CHECKED);
//
//  oswan_set_radio_menu(ID_WS_COLOR_DEFAULT     , 3, ws_colourScheme - COLOUR_SCHEME_DEFAULT);
//  oswan_set_radio_menu(ID_OPTIONS_FILTER_NORMAL, 4, DrawFilter - FILTER_NORMAL             );
//
//  if (LCDSegment)
//    CheckMenuItem(menu, ID_OPTIONS_LCDSEGMENT_ON, MF_CHECKED);
//
//  int  i;
//
//  for (i = 0; i < 8; i++)
//    if (ws_audio_get_channel_enabled(i))
//      CheckMenuItem(menu, ID_OPTIONS_SOUND_CH1 + i, MF_CHECKED  );
//    else
//      CheckMenuItem(menu, ID_OPTIONS_SOUND_CH1 + i, MF_UNCHECKED);
//
//  oswan_set_radio_menu(ID_OPTIONS_SOUND_WAVEOUT, 2, ws_audio_get_device());
//
//  int sampling_rate = ws_audio_get_bps() / 12000;
//
//  i = 0;
//  while ((sampling_rate >>= 1) > 0)
//    i++;
//
//  oswan_set_radio_menu(ID_OPTIONS_SOUND_BPS_12000, 3, i);
//
//  int buffer_size  = ws_audio_get_buffer_size() / 10;
//
//  i = 0;
//  while ((buffer_size >>= 1) > 0)
//    i++;
//
//  oswan_set_radio_menu(ID_OPTIONS_SOUND_BUFFER_10, 6, i);
//
//  oswan_set_recent_rom();
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static void ParseCommandLine(LPSTR lpCmdLine)
{
  if (lpCmdLine == NULL || *lpCmdLine == '\0')
    return;

  ws_rom_path[0] = '\0';

  int filenameIndex = 0;
  int readMode      = 0;

  do {
    switch (readMode) {
    case 1:
      if (*lpCmdLine == 'f')
        DrawFull = 1;

      readMode = 0;

      break;
    case 2:
      if (*lpCmdLine == '"') {
        ws_rom_path[filenameIndex] = '\0';
        readMode      = 0;
        filenameIndex = 0;
      }
      else
        ws_rom_path[filenameIndex++] = *lpCmdLine;

      break;
    case 3:
      if (*lpCmdLine == ' ') {
        ws_rom_path[filenameIndex] = '\0';
        readMode      = 0;
        filenameIndex = 0;
      }
      else
        ws_rom_path[filenameIndex++] = *lpCmdLine;

      break;
    default:
      if (*lpCmdLine == '-')
        readMode = 1;
      else if (*lpCmdLine == '"')
        readMode = 2;
      else if (*lpCmdLine != ' ') {
        ws_rom_path[filenameIndex++] = *lpCmdLine;
        readMode = 3;
      }
    }
  }
  while (*(++lpCmdLine) != '\0');

  if (ws_rom_path[0])
    app_new_rom = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void oswan_set_radio_menu(int menu_id, int count, int select)
{
  HMENU menu = GetMenu(hWnd);

  for (int i = 0; i < count; i++)
    CheckMenuItem(menu, menu_id + i, i == select ? MF_CHECKED : MF_UNCHECKED);
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_drop_files(HDROP hDrop)
//{
//  char path[1024] = "";
//
//  DragQueryFile(hDrop, 0, path, sizeof(path));
//  DragFinish(hDrop);
//
//  oswan_open_rom(path);
//
//  SetForegroundWindow(hWnd);
//}

///////////////////////////////////////////////////////////////////////////////////////////////////
////
///////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_open_file()
//{
//  char         path[1024] = "";
//  OPENFILENAME ofn;
//
//  ZeroMemory(&ofn, sizeof(OPENFILENAME));
//  ofn.lStructSize     = sizeof(OPENFILENAME);
//  ofn.hwndOwner       = hWnd;
//  ofn.lpstrFile       = path;
//  ofn.nMaxFile        = sizeof(path);
//  ofn.lpstrFilter     = "Wonderswan(*.ws,*.wsc,*.zip)\0*.ws;*.wsc;*.zip\0Wonderswan mono(*.ws)\0*.ws\0Wonderswan color(*.wsc)\0*.wsc\0Wonderswan zip(*.zip)\0*.zip\0\0";
//  ofn.nFilterIndex    = 1;
//  ofn.lpstrFileTitle  = NULL;
//  ofn.nMaxFileTitle   = 0;
//  ofn.lpstrInitialDir = WsRomDir;
//  ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
//  ofn.hInstance       = NULL;
//
//  if (GetOpenFileName(&ofn) != TRUE)
//    return;
//
//  oswan_open_rom(path);
//}
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static void oswan_load_state(int slot)
{
  char temp[1024];

  if (!ws_rom_path[0])
    return;

  sprintf(temp, "%s%s.%03d", WsSaveDir, BaseName, slot);
  ws_audio_stop();

  if (ws_loadState(temp) == 0) 
    MessageBox(hWnd, "State could not be loaded", "Error", MB_OK);
  else
    oswan_set_status_text(1, "Loaded state %d", slot + 1);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void oswan_load_state_with_file_name()
{
  OPENFILENAME ofn;
  char         temp[1024] = "";

  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize     = sizeof(OPENFILENAME);
  ofn.hwndOwner       = hWnd;
  ofn.lpstrFile       = temp;
  ofn.nMaxFile        = sizeof(temp);
  ofn.lpstrFilter     = "All files(*.*)\0*.*\0\0";
  ofn.nFilterIndex    = 1;
  ofn.lpstrFileTitle  = NULL;
  ofn.nMaxFileTitle   = 0;
  ofn.lpstrInitialDir = WsSaveDir;
  ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
  ofn.hInstance       = NULL;


  if (GetOpenFileName(&ofn) != TRUE)
    return;

  if (ws_loadState(temp) == 0)
    MessageBox(hWnd, "State could not be loaded", "Error", MB_OK);
  else
    oswan_set_status_text(1, "Loaded state");
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void oswan_save_state(int slot)
{
  char temp[1024];

  if (!ws_rom_path[0])
    return;

  sprintf(temp, "%s%s.%03d", WsSaveDir, BaseName, slot);
  ws_audio_stop();

  if (ws_saveState(temp) == 0) 
    MessageBox(hWnd, "State could not be saved", "Error", MB_OK);
  else
    oswan_set_status_text(1, "Saved state %d", slot + 1);

  SetStateInfo();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void oswan_save_state_with_file_name()
{
  OPENFILENAME ofn;
  char         temp[1024] = "";

  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ofn.lStructSize     = sizeof(OPENFILENAME);
  ofn.hwndOwner       = hWnd;
  ofn.lpstrFile       = temp;
  ofn.nMaxFile        = sizeof(temp);
  ofn.lpstrFilter     = "All files(*.*)\0*.*\0\0";
  ofn.nFilterIndex    = 1;
  ofn.lpstrFileTitle  = NULL;
  ofn.nMaxFileTitle   = 0;
  ofn.lpstrInitialDir = WsSaveDir;
  ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
  ofn.hInstance       = NULL;


  if (GetSaveFileName(&ofn) != TRUE)
    return;

  if (ws_saveState(temp) == 0)
    MessageBox(hWnd, "State could not be saved", "Error", MB_OK);
  else
    oswan_set_status_text(1, "Saved state");
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_system(int system)
//{
//  if (!app_gameRunning) {
//    ws_system = system;
//    //WsResize();
//    oswan_set_radio_menu(ID_OPTIONS_WS_MONO, 2, system);
//  }
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_video_size(int size)
//{
//  DrawSize = size;
//  //WsResize();
//  oswan_set_radio_menu(ID_OPTIONS_VIDEO_SIZE_1, 4, size - DS_1);
//}
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_color_scheme(int color_scheme)
//{
//  ws_colourScheme = color_scheme;
//  ws_gpu_set_colour_scheme(color_scheme);
//  oswan_set_radio_menu(ID_WS_COLOR_DEFAULT, 3, color_scheme - COLOUR_SCHEME_DEFAULT);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_filter(int filter)
//{
//  DrawFilter = filter;
//  //WsResize();
//  oswan_set_radio_menu(ID_OPTIONS_FILTER_NORMAL, 4, filter - FILTER_NORMAL);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_channel_enabled(int channel)
//{
//  HMENU menu = GetMenu(hWnd);
//
//  if (ws_audio_get_channel_enabled(channel)) {
//    ws_audio_set_channel_enabled(channel, 0);
//    CheckMenuItem(menu, ID_OPTIONS_SOUND_CH1 + channel, MF_UNCHECKED);
//  }
//  else {
//    ws_audio_set_channel_enabled(channel, 1);
//    CheckMenuItem(menu, ID_OPTIONS_SOUND_CH1 + channel, MF_CHECKED  );
//  }
//}
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_sound_device(int device_id)
//{
//  ws_audio_set_device(device_id);
//  oswan_set_radio_menu(ID_OPTIONS_SOUND_WAVEOUT, 2, device_id);
//}
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_sampling_rate(int sampling_rate)
//{
//  ws_audio_set_bps(sampling_rate);
//
//  sampling_rate /= 12000;
//
//  int select = 0;
//  while ((sampling_rate >>= 1) > 0)
//    select++;
//
//  oswan_set_radio_menu(ID_OPTIONS_SOUND_BPS_12000, 3, select);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_set_buffer_size(int buffer_size)
//{
//  ws_audio_set_buffer_size(buffer_size);
//
//  buffer_size /= 10;
//
//  int select = 0;
//  while ((buffer_size >>= 1) > 0)
//    select++;
//
//  oswan_set_radio_menu(ID_OPTIONS_SOUND_BUFFER_10, 6, select);
//}
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//static void oswan_open_rom(const char* path)
//{
//  if (path == NULL || !path[0])
//    return;
//
//  WsStop();
//  ws_done();
//  app_new_rom = 1;
//  strcpy(ws_rom_path, path);
//  GetBaseName(ws_rom_path);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
  hInst = hInstance;

  GetModuleFileName(NULL, WsHomeDir, sizeof(WsHomeDir));

  char* s1 = WsHomeDir;
  char* s2 = WsHomeDir;
  while(*s1) {
    if (*s1 == '\\')
      s2 = s1;

    s1++;
  }

  if (s2 != WsHomeDir)
    *s2 = '\0';

  sprintf(log_path, "%s\\OSwan.log", WsHomeDir);
  if (!log_init(log_path))
    printf("Warning: cannot open log file %s\n", log_path);

  log_write("Wonderswan emulator %s %s (last build on %s %s)\n\n",
            OSWAN_CAPTION, OSWAN_VERSION, __DATE__, __TIME__);

  conf_loadIniFile();
  ParseCommandLine(lpCmdLine);

  WNDCLASS wc;
  wc.lpszClassName = OSWAN_CLASSNAME;
  wc.lpfnWndProc   = WndProc;
  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
  wc.hCursor       = NULL;
  wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  if (!RegisterClass(&wc)) {
    log_write("Can't register window class '%s'\n\n", OSWAN_CLASSNAME);
    return FALSE;
  }

  hWnd = CreateWindowEx(
    WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
    OSWAN_CLASSNAME,
    OSWAN_CAPTION,
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
    CW_USEDEFAULT, 
    CW_USEDEFAULT, 
    CW_USEDEFAULT, 
    CW_USEDEFAULT, 
    NULL, 
    NULL, 
    hInstance, 
    NULL
  );

  HACCEL accel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

  InitMenu();
  WsDrawInit();
  WsResize();
  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);
  WsInputInit(hWnd);
  WsInputJoyInit(hWnd);
  ws_audio_init();

  MSG msg;
  while (1) {
    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
      if (!GetMessage(&msg, NULL, 0, 0))
        break;

      if (!TranslateAccelerator(hWnd, accel, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    else
      WsEmulate();
  }

  if (ws_rom_path[0])
    ws_save_sram(ws_rom_path);

  ws_audio_done();
  ws_done();
  WsDrawDone();
  WsInputJoyRelease();
  WsInputRelease();
  conf_saveIniFile();

  return 0;
}
#endif
