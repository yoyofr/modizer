/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//#include <windows.h>
#include "wsr_types.h" //YOYOFR

#include <stdio.h>
#include <stdlib.h>
//#include <direct.h>

#include "input.h"
//#include "resource.h"
#include "config.h"
#include "oswan.h"
#include "log.h"
//#include "draw.h"
#include "ws.h"
#include "keycode.h"
#include "audio.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// variables
/////////////////////////////////////////////////////////////////////////////////////////////////
int key_h_Xup    = DIK_UP;
int key_h_Xdown  = DIK_DOWN;
int key_h_Xright = DIK_RIGHT;
int key_h_Xleft  = DIK_LEFT;
int key_h_Yup    = DIK_W;
int key_h_Ydown  = DIK_S;
int key_h_Yright = DIK_D;
int key_h_Yleft  = DIK_A;
int key_h_start  = DIK_RETURN;
int key_h_A      = DIK_C;
int key_h_B      = DIK_X;
int key_v_Xup    = DIK_W;
int key_v_Xdown  = DIK_S;
int key_v_Xright = DIK_D;
int key_v_Xleft  = DIK_A;
int key_v_Yup    = DIK_UP;
int key_v_Ydown  = DIK_DOWN;
int key_v_Yright = DIK_RIGHT;
int key_v_Yleft  = DIK_LEFT;
int key_v_start  = DIK_RETURN;
int key_v_A      = DIK_C;
int key_v_B      = DIK_X;
int key_vsync    = DIK_SPACE;

int joy_h_Xup    = WS_JOY_AXIS_Y_M;
int joy_h_Xdown  = WS_JOY_AXIS_Y_P;
int joy_h_Xright = WS_JOY_AXIS_X_P;
int joy_h_Xleft  = WS_JOY_AXIS_X_M;
int joy_h_Yup    = WS_JOY_POV_UP;
int joy_h_Ydown  = WS_JOY_POV_DOWN;
int joy_h_Yright = WS_JOY_POV_RIGHT;
int joy_h_Yleft  = WS_JOY_POV_LEFT;
int joy_h_start  = WS_JOY_BUTTON_7;
int joy_h_A      = WS_JOY_BUTTON_2;
int joy_h_B      = WS_JOY_BUTTON_1;
int joy_v_Xup    = WS_JOY_BUTTON_4;
int joy_v_Xdown  = WS_JOY_BUTTON_1;
int joy_v_Xright = WS_JOY_BUTTON_2;
int joy_v_Xleft  = WS_JOY_BUTTON_3;
int joy_v_Yup    = WS_JOY_AXIS_Y_M;
int joy_v_Ydown  = WS_JOY_AXIS_Y_P;
int joy_v_Yright = WS_JOY_AXIS_X_P;
int joy_v_Yleft  = WS_JOY_AXIS_X_M;
int joy_v_start  = WS_JOY_BUTTON_7;
int joy_v_A      = WS_JOY_BUTTON_6;
int joy_v_B      = WS_JOY_BUTTON_5;
int joy_vsync    = WS_JOY_BUTTON_8;

/////////////////////////////////////////////////////////////////////////////////////////////////
// static variables
/////////////////////////////////////////////////////////////////////////////////////////////////
static int tmp_h_Xup;
static int tmp_h_Xdown;
static int tmp_h_Xright;
static int tmp_h_Xleft;
static int tmp_h_Yup;
static int tmp_h_Ydown;
static int tmp_h_Yright;
static int tmp_h_Yleft;
static int tmp_h_start;
static int tmp_h_A;
static int tmp_h_B;
static int tmp_v_Xup;
static int tmp_v_Xdown;
static int tmp_v_Xright;
static int tmp_v_Xleft;
static int tmp_v_Yup;
static int tmp_v_Ydown;
static int tmp_v_Yright;
static int tmp_v_Yleft;
static int tmp_v_start;
static int tmp_v_A;
static int tmp_v_B;
static int tmp_vsync;

//static WNDPROC Org_WndProc1;

/////////////////////////////////////////////////////////////////////////////////////////////////
// static functions
/////////////////////////////////////////////////////////////////////////////////////////////////
//void WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nInt, LPCTSTR lpFileName);
//const char* conf_dispCode1(int code);
//const char* conf_dispCode2(int code);
//int* conf_getKey(HWND hEditWnd);
//int conf_get_key_code();
//LRESULT CALLBACK EditProc1(HWND hEditWnd, UINT message, WPARAM wParam, LPARAM lParam);
//LRESULT CALLBACK EditProc2(HWND hEditWnd,UINT message,WPARAM wParam,LPARAM lParam);

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//void conf_loadIniFile(void)
//{
//  char ini_path[1024] = "";
//
//  sprintf(ini_path, "%s\\Oswan.ini", WsHomeDir);
//
//  GetPrivateProfileString("DIRECTORIES", "ROM_DIR" , WsRomDir , WsRomDir , 1024, ini_path);
//  GetPrivateProfileString("DIRECTORIES", "SRAM_DIR", WsSramDir, WsSramDir, 1024, ini_path);
//  GetPrivateProfileString("DIRECTORIES", "SAVE_DIR", WsSaveDir, WsSaveDir, 1024, ini_path);
//  GetPrivateProfileString("DIRECTORIES", "SHOT_DIR", WsShotDir, WsShotDir, 1024, ini_path);
//  GetPrivateProfileString("DIRECTORIES", "WAVE_DIR", WsWavDir , WsWavDir , 1024, ini_path);
//
//  if (WsRomDir[0] == '\0')
//    sprintf(WsRomDir, "%s\\rom", WsHomeDir);
//
//  if (WsSramDir[0] == '\0')
//    sprintf(WsSramDir, "%s\\sram", WsHomeDir);
//
//  if (WsSaveDir[0] == '\0')
//    sprintf(WsSaveDir, "%s\\save", WsHomeDir);
//
//  if (WsShotDir[0] == '\0')
//    sprintf(WsShotDir, "%s\\screenshot", WsHomeDir);
//
//  if (WsWavDir[0] == '\0')
//    sprintf(WsWavDir, "%s\\wav", WsHomeDir);
//
//  int code;
//
//  code = GetPrivateProfileInt("VIDEO", "VIDEO_FILTER", DrawFilter, ini_path);
//  if (code >= 0 && code <= 7)
//    DrawFilter = code;
//
//  code = GetPrivateProfileInt("VIDEO", "VIDEO_COLOUR_SCHEME", ws_colourScheme, ini_path);
//  if (code >= 0 && code <= 2)
//    ws_colourScheme = code;
//
//  code = GetPrivateProfileInt("VIDEO", "VIDEO_SYSTEM", ws_system, ini_path);
//  if (code == 0 || code == 1)
//    ws_system = code;
//
//  code = GetPrivateProfileInt("VIDEO", "VIDEO_SIZE", DrawSize, ini_path);
//  if (code >= 1 && code <= 4)
//    DrawSize = code;
//
//  code = GetPrivateProfileInt("VIDEO", "VIDEO_LCD_DEGMENT", LCDSegment, ini_path);
//  if (code == 0 || code == 1)
//    LCDSegment = code;
//
//  code = GetPrivateProfileInt("SOUND", "SOUND_DEVICE", ws_audio_get_device(), ini_path);
//  if (code >= 0 && code < 2)
//    ws_audio_set_device(code);
//
//  code = GetPrivateProfileInt("SOUND", "SOUND_MAIN_VOLUME", ws_audio_get_main_volume(), ini_path);
//  if (code >= 0 && code <= 15)
//    ws_audio_set_main_volume(code);
//
//  code = GetPrivateProfileInt("SOUND", "SOUND_BPS", ws_audio_get_bps(), ini_path);
//  ws_audio_set_bps(code);
//
//  code = GetPrivateProfileInt("SOUND", "SOUND_BUFFER_SIZE", ws_audio_get_buffer_size(), ini_path);
//  if (code == 10 || code == 20 || code == 40 || code == 80 || code == 160 || code == 320)
//    ws_audio_set_buffer_size(code);
//
//  key_h_Xup    = GetPrivateProfileInt("KEYBOARD", "KEY_HXUP"   , key_h_Xup   , ini_path);
//  key_h_Xdown  = GetPrivateProfileInt("KEYBOARD", "KEY_HXDOWN" , key_h_Xdown , ini_path); 
//  key_h_Xright = GetPrivateProfileInt("KEYBOARD", "KEY_HXRIGHT", key_h_Xright, ini_path);
//  key_h_Xleft  = GetPrivateProfileInt("KEYBOARD", "KEY_HXLEFT" , key_h_Xleft , ini_path);
//  key_h_Yup    = GetPrivateProfileInt("KEYBOARD", "KEY_HYUP"   , key_h_Yup   , ini_path);
//  key_h_Ydown  = GetPrivateProfileInt("KEYBOARD", "KEY_HYDOWN" , key_h_Ydown , ini_path); 
//  key_h_Yright = GetPrivateProfileInt("KEYBOARD", "KEY_HYRIGHT", key_h_Yright, ini_path);
//  key_h_Yleft  = GetPrivateProfileInt("KEYBOARD", "KEY_HYLEFT" , key_h_Yleft , ini_path);
//  key_h_start  = GetPrivateProfileInt("KEYBOARD", "KEY_HSTART" , key_h_start , ini_path);
//  key_h_A      = GetPrivateProfileInt("KEYBOARD", "KEY_HA"     , key_h_A     , ini_path);
//  key_h_B      = GetPrivateProfileInt("KEYBOARD", "KEY_HB"     , key_h_B     , ini_path);
//  key_v_Xup    = GetPrivateProfileInt("KEYBOARD", "KEY_VXUP"   , key_v_Xup   , ini_path);
//  key_v_Xdown  = GetPrivateProfileInt("KEYBOARD", "KEY_VXDOWN" , key_v_Xdown , ini_path); 
//  key_v_Xright = GetPrivateProfileInt("KEYBOARD", "KEY_VXRIGHT", key_v_Xright, ini_path);
//  key_v_Xleft  = GetPrivateProfileInt("KEYBOARD", "KEY_VXLEFT" , key_v_Xleft , ini_path);
//  key_v_Yup    = GetPrivateProfileInt("KEYBOARD", "KEY_VYUP"   , key_v_Yup   , ini_path);
//  key_v_Ydown  = GetPrivateProfileInt("KEYBOARD", "KEY_VYDOWN" , key_v_Ydown , ini_path); 
//  key_v_Yright = GetPrivateProfileInt("KEYBOARD", "KEY_VYRIGHT", key_v_Yright, ini_path);
//  key_v_Yleft  = GetPrivateProfileInt("KEYBOARD", "KEY_VYLEFT" , key_v_Yleft , ini_path);
//  key_v_start  = GetPrivateProfileInt("KEYBOARD", "KEY_VSTART" , key_v_start , ini_path);
//  key_v_A      = GetPrivateProfileInt("KEYBOARD", "KEY_VA"     , key_v_A     , ini_path);
//  key_v_B      = GetPrivateProfileInt("KEYBOARD", "KEY_VB"     , key_v_B     , ini_path);
//  key_vsync    = GetPrivateProfileInt("KEYBOARD", "KEY_VSYNC"  , key_vsync   , ini_path);
//
//  joy_h_Xup    = GetPrivateProfileInt("JOYSTICK", "JOY_HXUP"   , joy_h_Xup   , ini_path);
//  joy_h_Xdown  = GetPrivateProfileInt("JOYSTICK", "JOY_HXDOWN" , joy_h_Xdown , ini_path); 
//  joy_h_Xright = GetPrivateProfileInt("JOYSTICK", "JOY_HXRIGHT", joy_h_Xright, ini_path);
//  joy_h_Xleft  = GetPrivateProfileInt("JOYSTICK", "JOY_HXLEFT" , joy_h_Xleft , ini_path);
//  joy_h_Yup    = GetPrivateProfileInt("JOYSTICK", "JOY_HYUP"   , joy_h_Yup   , ini_path);
//  joy_h_Ydown  = GetPrivateProfileInt("JOYSTICK", "JOY_HYDOWN" , joy_h_Ydown , ini_path); 
//  joy_h_Yright = GetPrivateProfileInt("JOYSTICK", "JOY_HYRIGHT", joy_h_Yright, ini_path);
//  joy_h_Yleft  = GetPrivateProfileInt("JOYSTICK", "JOY_HYLEFT" , joy_h_Yleft , ini_path);
//  joy_h_start  = GetPrivateProfileInt("JOYSTICK", "JOY_HSTART" , joy_h_start , ini_path);
//  joy_h_A      = GetPrivateProfileInt("JOYSTICK", "JOY_HA"     , joy_h_A     , ini_path);
//  joy_h_B      = GetPrivateProfileInt("JOYSTICK", "JOY_HB"     , joy_h_B     , ini_path);
//  joy_v_Xup    = GetPrivateProfileInt("JOYSTICK", "JOY_VXUP"   , joy_v_Xup   , ini_path);
//  joy_v_Xdown  = GetPrivateProfileInt("JOYSTICK", "JOY_VXDOWN" , joy_v_Xdown , ini_path); 
//  joy_v_Xright = GetPrivateProfileInt("JOYSTICK", "JOY_VXRIGHT", joy_v_Xright, ini_path);
//  joy_v_Xleft  = GetPrivateProfileInt("JOYSTICK", "JOY_VXLEFT" , joy_v_Xleft , ini_path);
//  joy_v_Yup    = GetPrivateProfileInt("JOYSTICK", "JOY_VYUP"   , joy_v_Yup   , ini_path);
//  joy_v_Ydown  = GetPrivateProfileInt("JOYSTICK", "JOY_VYDOWN" , joy_v_Ydown , ini_path); 
//  joy_v_Yright = GetPrivateProfileInt("JOYSTICK", "JOY_VYRIGHT", joy_v_Yright, ini_path);
//  joy_v_Yleft  = GetPrivateProfileInt("JOYSTICK", "JOY_VYLEFT" , joy_v_Yleft , ini_path);
//  joy_v_start  = GetPrivateProfileInt("JOYSTICK", "JOY_VSTART" , joy_v_start , ini_path);
//  joy_v_A      = GetPrivateProfileInt("JOYSTICK", "JOY_VA"     , joy_v_A     , ini_path);
//  joy_v_B      = GetPrivateProfileInt("JOYSTICK", "JOY_VB"     , joy_v_B     , ini_path);
//  joy_vsync    = GetPrivateProfileInt("JOYSTICK", "JOY_VSYNC"  , joy_vsync   , ini_path);
//
//  GetPrivateProfileString("RECENT", "RECENT0", recent[0], recent[0], 1024, ini_path);
//  GetPrivateProfileString("RECENT", "RECENT1", recent[1], recent[1], 1024, ini_path);
//  GetPrivateProfileString("RECENT", "RECENT2", recent[2], recent[2], 1024, ini_path);
//  GetPrivateProfileString("RECENT", "RECENT3", recent[3], recent[3], 1024, ini_path);
//  GetPrivateProfileString("RECENT", "RECENT4", recent[4], recent[4], 1024, ini_path);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//void conf_saveIniFile(void)
//{
//  char ini_path[1024] = "";
//
//  sprintf(ini_path, "%s\\Oswan.ini", WsHomeDir);
//
//  WritePrivateProfileString("DIRECTORIES", "ROM_DIR" , WsRomDir , ini_path);
//  WritePrivateProfileString("DIRECTORIES", "SRAM_DIR", WsSramDir, ini_path);
//  WritePrivateProfileString("DIRECTORIES", "SAVE_DIR", WsSaveDir, ini_path);
//  WritePrivateProfileString("DIRECTORIES", "SHOT_DIR", WsShotDir, ini_path);
//  WritePrivateProfileString("DIRECTORIES", "WAVE_DIR", WsWavDir , ini_path);
//
//  WritePrivateProfileInt("VIDEO", "VIDEO_FILTER"       , DrawFilter     , ini_path);
//  WritePrivateProfileInt("VIDEO", "VIDEO_COLOUR_SCHEME", ws_colourScheme, ini_path);
//  WritePrivateProfileInt("VIDEO", "VIDEO_SYSTEM"       , ws_system      , ini_path);
//  WritePrivateProfileInt("VIDEO", "VIDEO_SIZE"         , DrawSize       , ini_path);
//  WritePrivateProfileInt("VIDEO", "VIDEO_LCD_DEGMENT"  , LCDSegment     , ini_path);
//
//  WritePrivateProfileInt("SOUND", "SOUND_DEVICE"     , ws_audio_get_device()     , ini_path);
//  WritePrivateProfileInt("SOUND", "SOUND_MAIN_VOLUME", ws_audio_get_main_volume(), ini_path);
//  WritePrivateProfileInt("SOUND", "SOUND_BPS"        , ws_audio_get_bps()        , ini_path);
//  WritePrivateProfileInt("SOUND", "SOUND_BUFFER_SIZE", ws_audio_get_buffer_size(), ini_path);
//
//  WritePrivateProfileInt("KEYBOARD", "KEY_HXUP"   , key_h_Xup   , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HXDOWN" , key_h_Xdown , ini_path); 
//  WritePrivateProfileInt("KEYBOARD", "KEY_HXRIGHT", key_h_Xright, ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HXLEFT" , key_h_Xleft , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HYUP"   , key_h_Yup   , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HYDOWN" , key_h_Ydown , ini_path); 
//  WritePrivateProfileInt("KEYBOARD", "KEY_HYRIGHT", key_h_Yright, ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HYLEFT" , key_h_Yleft , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HSTART" , key_h_start , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HA"     , key_h_A     , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_HB"     , key_h_B     , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VXUP"   , key_v_Xup   , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VXDOWN" , key_v_Xdown , ini_path); 
//  WritePrivateProfileInt("KEYBOARD", "KEY_VXRIGHT", key_v_Xright, ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VXLEFT" , key_v_Xleft , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VYUP"   , key_v_Yup   , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VYDOWN" , key_v_Ydown , ini_path); 
//  WritePrivateProfileInt("KEYBOARD", "KEY_VYRIGHT", key_v_Yright, ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VYLEFT" , key_v_Yleft , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VSTART" , key_v_start , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VA"     , key_v_A     , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VB"     , key_v_B     , ini_path);
//  WritePrivateProfileInt("KEYBOARD", "KEY_VSYNC"  , key_vsync   , ini_path);
//
//  WritePrivateProfileInt("JOYSTICK", "JOY_HXUP"   , joy_h_Xup   , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HXDOWN" , joy_h_Xdown , ini_path); 
//  WritePrivateProfileInt("JOYSTICK", "JOY_HXRIGHT", joy_h_Xright, ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HXLEFT" , joy_h_Xleft , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HYUP"   , joy_h_Yup   , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HYDOWN" , joy_h_Ydown , ini_path); 
//  WritePrivateProfileInt("JOYSTICK", "JOY_HYRIGHT", joy_h_Yright, ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HYLEFT" , joy_h_Yleft , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HSTART" , joy_h_start , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HA"     , joy_h_A     , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_HB"     , joy_h_B     , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VXUP"   , joy_v_Xup   , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VXDOWN" , joy_v_Xdown , ini_path); 
//  WritePrivateProfileInt("JOYSTICK", "JOY_VXRIGHT", joy_v_Xright, ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VXLEFT" , joy_v_Xleft , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VYUP"   , joy_v_Yup   , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VYDOWN" , joy_v_Ydown , ini_path); 
//  WritePrivateProfileInt("JOYSTICK", "JOY_VYRIGHT", joy_v_Yright, ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VYLEFT" , joy_v_Yleft , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VSTART" , joy_v_start , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VA"     , joy_v_A     , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VB"     , joy_v_B     , ini_path);
//  WritePrivateProfileInt("JOYSTICK", "JOY_VSYNC"  , joy_vsync   , ini_path);
//
//  WritePrivateProfileString("RECENT", "RECENT0", recent[0], ini_path);
//  WritePrivateProfileString("RECENT", "RECENT1", recent[1], ini_path);
//  WritePrivateProfileString("RECENT", "RECENT2", recent[2], ini_path);
//  WritePrivateProfileString("RECENT", "RECENT3", recent[3], ini_path);
//  WritePrivateProfileString("RECENT", "RECENT4", recent[4], ini_path);
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//LRESULT CALLBACK GuiConfigKeyboardProc(HWND hDlgWnd,UINT message,WPARAM wParam,LPARAM lParam)
//{
//  switch (message) {
//  case WM_INITDIALOG:
//    WsInputInit(hDlgWnd);
//    Org_WndProc1 = (WNDPROC) GetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXUP), GWL_WNDPROC);
//    
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXUP)   , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXRIGHT), GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXDOWN) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXLEFT) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYUP)   , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYRIGHT), GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYDOWN) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYLEFT) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HSTART) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HA)     , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HB)     , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXUP)   , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXRIGHT), GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXDOWN) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXLEFT) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYUP)   , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYRIGHT), GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYDOWN) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYLEFT) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VSTART) , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VA)     , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VB)     , GWL_WNDPROC, (LONG) EditProc1);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VSYNC)  , GWL_WNDPROC, (LONG) EditProc1);
//
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXUP   , conf_dispCode1(key_h_Xup   ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXDOWN , conf_dispCode1(key_h_Xdown ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXRIGHT, conf_dispCode1(key_h_Xright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXLEFT , conf_dispCode1(key_h_Xleft ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYUP   , conf_dispCode1(key_h_Yup   ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYDOWN , conf_dispCode1(key_h_Ydown ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYRIGHT, conf_dispCode1(key_h_Yright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYLEFT , conf_dispCode1(key_h_Yleft ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HSTART , conf_dispCode1(key_h_start ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HA     , conf_dispCode1(key_h_A     ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HB     , conf_dispCode1(key_h_B     ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXUP   , conf_dispCode1(key_v_Xup   ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXDOWN , conf_dispCode1(key_v_Xdown ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXRIGHT, conf_dispCode1(key_v_Xright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXLEFT , conf_dispCode1(key_v_Xleft ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYUP   , conf_dispCode1(key_v_Yup   ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYDOWN , conf_dispCode1(key_v_Ydown ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYRIGHT, conf_dispCode1(key_v_Yright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYLEFT , conf_dispCode1(key_v_Yleft ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VSTART , conf_dispCode1(key_v_start ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VA     , conf_dispCode1(key_v_A     ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VB     , conf_dispCode1(key_v_B     ));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VSYNC  , conf_dispCode1(key_vsync   ));
//
//    tmp_h_Xup    = key_h_Xup;
//    tmp_h_Xdown  = key_h_Xdown;
//    tmp_h_Xright = key_h_Xright;
//    tmp_h_Xleft  = key_h_Xleft;
//    tmp_h_Yup    = key_h_Yup;
//    tmp_h_Ydown  = key_h_Ydown;
//    tmp_h_Yright = key_h_Yright;
//    tmp_h_Yleft  = key_h_Yleft;
//    tmp_h_start  = key_h_start;
//    tmp_h_A      = key_h_A;
//    tmp_h_B      = key_h_B;
//
//    tmp_v_Xup    = key_v_Xup;
//    tmp_v_Xdown  = key_v_Xdown;
//    tmp_v_Xright = key_v_Xright;
//    tmp_v_Xleft  = key_v_Xleft;
//    tmp_v_Yup    = key_v_Yup;
//    tmp_v_Ydown  = key_v_Ydown;
//    tmp_v_Yright = key_v_Yright;
//    tmp_v_Yleft  = key_v_Yleft;
//    tmp_v_start  = key_v_start;
//    tmp_v_A      = key_v_A;
//    tmp_v_B      = key_v_B;
//
//    tmp_vsync = key_vsync;
//
//    SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_HXUP));
//    return 0;
//  case WM_DESTROY:
//    WsInputRelease();
//    WsDrawFlip();
//    break;
//  case WM_COMMAND:
//    if (HIWORD(wParam) == BN_CLICKED) {
//      EndDialog(hDlgWnd, LOWORD(wParam));
//
//      if (LOWORD(wParam) == IDOK) {
//        key_h_Xup    = tmp_h_Xup;
//        key_h_Xdown  = tmp_h_Xdown;
//        key_h_Xright = tmp_h_Xright;
//        key_h_Xleft  = tmp_h_Xleft;
//        key_h_Yup    = tmp_h_Yup;
//        key_h_Ydown  = tmp_h_Ydown;
//        key_h_Yright = tmp_h_Yright;
//        key_h_Yleft  = tmp_h_Yleft;
//        key_h_start  = tmp_h_start;
//        key_h_A      = tmp_h_A;
//        key_h_B      = tmp_h_B;
//
//        key_v_Xup    = tmp_v_Xup;
//        key_v_Xdown  = tmp_v_Xdown;
//        key_v_Xright = tmp_v_Xright;
//        key_v_Xleft  = tmp_v_Xleft;
//        key_v_Yup    = tmp_v_Yup;
//        key_v_Ydown  = tmp_v_Ydown;
//        key_v_Yright = tmp_v_Yright;
//        key_v_Yleft  = tmp_v_Yleft;
//        key_v_start  = tmp_v_start;
//        key_v_A      = tmp_v_A;
//        key_v_B      = tmp_v_B;
//
//        key_vsync = tmp_vsync;
//      }
//
//      return 1;
//    }
//    break;
//  case WM_MOVE:
//    WsDrawFlip();
//    break;
//  }
//  
//  return 0;
//}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//LRESULT CALLBACK GuiConfigJoypadProc(HWND hDlgWnd,UINT message,WPARAM wParam,LPARAM lParam)
//{
//  HWND hEditWnd;
//  int* key;
//  int  key_code;
//
//  switch (message) {
//  case WM_INITDIALOG:
//    WsInputJoyInit(hDlgWnd);
//    Org_WndProc1 = (WNDPROC) GetWindowLong(GetDlgItem(hDlgWnd, IDC_EDIT_HXUP), GWL_WNDPROC);
//
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXUP)   , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXRIGHT), GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXDOWN) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HXLEFT) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYUP)   , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYRIGHT), GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYDOWN) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HYLEFT) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HSTART) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HA)     , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_HB)     , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXUP)   , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXRIGHT), GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXDOWN) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VXLEFT) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYUP)   , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYRIGHT), GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYDOWN) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VYLEFT) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VSTART) , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VA)     , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VB)     , GWL_WNDPROC, (LONG) EditProc2);
//    SetWindowLong(GetDlgItem(hDlgWnd,IDC_EDIT_VSYNC)  , GWL_WNDPROC, (LONG) EditProc2);
//
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXUP, conf_dispCode2(joy_h_Xup));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXDOWN, conf_dispCode2(joy_h_Xdown));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXRIGHT, conf_dispCode2(joy_h_Xright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HXLEFT, conf_dispCode2(joy_h_Xleft));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYUP, conf_dispCode2(joy_h_Yup));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYDOWN, conf_dispCode2(joy_h_Ydown));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYRIGHT, conf_dispCode2(joy_h_Yright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HYLEFT, conf_dispCode2(joy_h_Yleft));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HSTART, conf_dispCode2(joy_h_start));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HA, conf_dispCode2(joy_h_A));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_HB, conf_dispCode2(joy_h_B));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXUP, conf_dispCode2(joy_v_Xup));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXDOWN, conf_dispCode2(joy_v_Xdown));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXRIGHT, conf_dispCode2(joy_v_Xright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VXLEFT, conf_dispCode2(joy_v_Xleft));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYUP, conf_dispCode2(joy_v_Yup));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYDOWN, conf_dispCode2(joy_v_Ydown));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYRIGHT, conf_dispCode2(joy_v_Yright));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VYLEFT, conf_dispCode2(joy_v_Yleft));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VSTART, conf_dispCode2(joy_v_start));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VA, conf_dispCode2(joy_v_A));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VB, conf_dispCode2(joy_v_B));
//    SetDlgItemText(hDlgWnd, IDC_EDIT_VSYNC, conf_dispCode2(joy_vsync));
//
//    tmp_h_Xup    = joy_h_Xup;
//    tmp_h_Xdown  = joy_h_Xdown;
//    tmp_h_Xright = joy_h_Xright;
//    tmp_h_Xleft  = joy_h_Xleft;
//    tmp_h_Yup    = joy_h_Yup;
//    tmp_h_Ydown  = joy_h_Ydown;
//    tmp_h_Yright = joy_h_Yright;
//    tmp_h_Yleft  = joy_h_Yleft;
//    tmp_h_start  = joy_h_start;
//    tmp_h_A      = joy_h_A;
//    tmp_h_B      = joy_h_B;
//
//    tmp_v_Xup    = joy_v_Xup;
//    tmp_v_Xdown  = joy_v_Xdown;
//    tmp_v_Xright = joy_v_Xright;
//    tmp_v_Xleft  = joy_v_Xleft;
//    tmp_v_Yup    = joy_v_Yup;
//    tmp_v_Ydown  = joy_v_Ydown;
//    tmp_v_Yright = joy_v_Yright;
//    tmp_v_Yleft  = joy_v_Yleft;
//    tmp_v_start  = joy_v_start;
//    tmp_v_A      = joy_v_A;
//    tmp_v_B      = joy_v_B;
//
//    tmp_vsync = joy_vsync;
//
//    SetFocus(GetDlgItem(hDlgWnd, IDC_EDIT_HXUP));
//    SetTimer(hDlgWnd, 0, 30, NULL);
//    return 0;
//  case WM_DESTROY:
//    KillTimer(hDlgWnd, 0);
//    WsInputJoyRelease();
//    WsDrawFlip();
//    break;
//  case WM_COMMAND:
//    if (HIWORD(wParam) == BN_CLICKED) {
//      EndDialog(hDlgWnd, LOWORD(wParam));
//      
//      if (LOWORD(wParam) == IDOK) {
//        joy_h_Xup    = tmp_h_Xup;
//        joy_h_Xdown  = tmp_h_Xdown;
//        joy_h_Xright = tmp_h_Xright;
//        joy_h_Xleft  = tmp_h_Xleft;
//        joy_h_Yup    = tmp_h_Yup;
//        joy_h_Ydown  = tmp_h_Ydown;
//        joy_h_Yright = tmp_h_Yright;
//        joy_h_Yleft  = tmp_h_Yleft;
//        joy_h_start  = tmp_h_start;
//        joy_h_A      = tmp_h_A;
//        joy_h_B      = tmp_h_B;
//
//        joy_v_Xup    = tmp_v_Xup;
//        joy_v_Xdown  = tmp_v_Xdown;
//        joy_v_Xright = tmp_v_Xright;
//        joy_v_Xleft  = tmp_v_Xleft;
//        joy_v_Yup    = tmp_v_Yup;
//        joy_v_Ydown  = tmp_v_Ydown;
//        joy_v_Yright = tmp_v_Yright;
//        joy_v_Yleft  = tmp_v_Yleft;
//        joy_v_start  = tmp_v_start;
//        joy_v_A      = tmp_v_A;
//        joy_v_B      = tmp_v_B;
//
//        joy_vsync = tmp_vsync;
//      }
//
//      return 1;
//    }
//    break;
//  case WM_TIMER:
//    hEditWnd = GetFocus();
//
//    key = conf_getKey(hEditWnd);
//    if (!key)
//      return 1;
//	
//    key_code = conf_get_key_code();
//    if (key_code < 0 || *key == key_code)
//      return 1;
//
//    SetWindowText(hEditWnd, conf_dispCode2(key_code));
//    *key = key_code;
//
//    return 1;
//  case WM_MOVE:
//    WsDrawFlip();
//    break;
//  }
//  
//  return 0;
//}

#if 0
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void WritePrivateProfileInt(LPCTSTR lpAppName,
                                   LPCTSTR lpKeyName, INT nInt, LPCTSTR lpFileName)
{
  char s[32];

  sprintf(s, "%d", nInt);
  WritePrivateProfileString(lpAppName, lpKeyName, s, lpFileName);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static const char* conf_dispCode1(int code)
{
  return (code >= 0 && code < sizeof(keyName) / sizeof(char*) ? keyName[code] : "???");
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static const char* conf_dispCode2(int code)
{
  static char key_name[16];
  char* result;

  switch (code) {
  case WS_JOY_POV_UP:     result = "POV1 UP";       break;
  case WS_JOY_POV_RIGHT:  result = "POV1 RIGHT";    break;
  case WS_JOY_POV_DOWN:   result = "POV1 DOWN";     break;
  case WS_JOY_POV_LEFT:   result = "POV1 LEFT";     break;
  case WS_JOY_POV2_UP:    result = "POV2 UP";       break;
  case WS_JOY_POV2_RIGHT: result = "POV2 RIGHT";    break;
  case WS_JOY_POV2_DOWN:  result = "POV2 DOWN";     break;
  case WS_JOY_POV2_LEFT:  result = "POV2 LEFT";     break;
  case WS_JOY_POV3_UP:    result = "POV3 UP";       break;
  case WS_JOY_POV3_RIGHT: result = "POV3 RIGHT";    break;
  case WS_JOY_POV3_DOWN:  result = "POV3 DOWN";     break;
  case WS_JOY_POV3_LEFT:  result = "POV3 LEFT";     break;
  case WS_JOY_POV4_UP:    result = "POV4 UP";       break;
  case WS_JOY_POV4_RIGHT: result = "POV4 RIGHT";    break;
  case WS_JOY_POV4_DOWN:  result = "POV4 DOWN";     break;
  case WS_JOY_POV4_LEFT:  result = "POV4 LEFT";     break;
  case WS_JOY_AXIS_Y_M:   result = "XYaxes UP";     break;
  case WS_JOY_AXIS_X_P:   result = "XYaxes RIGHT";  break;
  case WS_JOY_AXIS_Y_P:   result = "XYaxes DOWN";   break;
  case WS_JOY_AXIS_X_M:   result = "XYaxes LEFT";   break;
  case WS_JOY_AXIS_Z_P:   result = "Zaxis +";       break;
  case WS_JOY_AXIS_Z_M:   result = "Zaxis -";       break;
  case WS_JOY_AXIS_RY_M:  result = "RXYaxes UP";    break;
  case WS_JOY_AXIS_RX_P:  result = "RXYaxes RIGHT"; break;
  case WS_JOY_AXIS_RY_P:  result = "RXYaxes DOWN";  break;
  case WS_JOY_AXIS_RX_M:  result = "RXYaxes LEFT";  break;
  case WS_JOY_AXIS_RZ_P:  result = "RZaxis +";      break;
  case WS_JOY_AXIS_RZ_M:  result = "RZaxis -";      break;
  case WS_JOY_SLIDER_P:   result = "Slider1 +";     break;
  case WS_JOY_SLIDER_M:   result = "Slider1 -";     break;
  case WS_JOY_SLIDER2_P:  result = "Slider3 +";     break;
  case WS_JOY_SLIDER2_M:  result = "Slider3 -";     break;
  default:
    sprintf(key_name, "[%d]", code);
    result = key_name;
  }

  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static int* conf_getKey(HWND hEditWnd)
{
  int *key;

  switch (GetDlgCtrlID(hEditWnd)) {
  case IDC_EDIT_HXUP:    key = &tmp_h_Xup;    break;
  case IDC_EDIT_HXDOWN:  key = &tmp_h_Xdown;  break;
  case IDC_EDIT_HXRIGHT: key = &tmp_h_Xright; break;
  case IDC_EDIT_HXLEFT:  key = &tmp_h_Xleft;  break;
  case IDC_EDIT_HYUP:    key = &tmp_h_Yup;    break;
  case IDC_EDIT_HYDOWN:  key = &tmp_h_Ydown;  break;
  case IDC_EDIT_HYRIGHT: key = &tmp_h_Yright; break;
  case IDC_EDIT_HYLEFT:  key = &tmp_h_Yleft;  break;
  case IDC_EDIT_HSTART:  key = &tmp_h_start;  break;
  case IDC_EDIT_HA:      key = &tmp_h_A;      break;
  case IDC_EDIT_HB:      key = &tmp_h_B;      break;
  case IDC_EDIT_VXUP:    key = &tmp_v_Xup;    break;
  case IDC_EDIT_VXDOWN:  key = &tmp_v_Xdown;  break;
  case IDC_EDIT_VXRIGHT: key = &tmp_v_Xright; break;
  case IDC_EDIT_VXLEFT:  key = &tmp_v_Xleft;  break;
  case IDC_EDIT_VYUP:    key = &tmp_v_Yup;    break;
  case IDC_EDIT_VYDOWN:  key = &tmp_v_Ydown;  break;
  case IDC_EDIT_VYRIGHT: key = &tmp_v_Yright; break;
  case IDC_EDIT_VYLEFT:  key = &tmp_v_Yleft;  break;
  case IDC_EDIT_VSTART:  key = &tmp_v_start;  break;
  case IDC_EDIT_VA:      key = &tmp_v_A;      break;
  case IDC_EDIT_VB:      key = &tmp_v_B;      break;
  case IDC_EDIT_VSYNC:   key = &tmp_vsync;    break;
  default:               key = NULL;
  }

  return key;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static int conf_get_key_code()
{
  const long joy_center = 0x7fff;

  static int x_flag  = 0;
  static int y_flag  = 0;
  static int z_flag  = 0;
  static int rx_flag = 0;
  static int ry_flag = 0;
  static int rz_flag = 0;
  static int s1_flag = 0;
  static int s2_flag = 0;

  HRESULT      hRet;
  int          joy;
  unsigned int i;
  DIJOYSTATE2  js;
  DIDEVCAPS    diDevCaps;

  diDevCaps.dwSize = sizeof(DIDEVCAPS);

  if (lpJoyDevice == NULL)
    return -1;

  hRet = lpJoyDevice->Poll();
  if (FAILED(hRet)) {
    hRet = lpJoyDevice->Acquire();
    while (hRet == DIERR_INPUTLOST)
      hRet = lpJoyDevice->Acquire();
      
    return -1;
  }

  lpJoyDevice->GetDeviceState(sizeof(DIJOYSTATE2), &js);
  lpJoyDevice->GetCapabilities(&diDevCaps);

  for (i = 0; i < diDevCaps.dwButtons; i++)
    if (js.rgbButtons[i] & 0x80)
      return i + 1;

  for (i = 0; i < diDevCaps.dwPOVs; i++) {
    joy = WS_JOY_POV_UP + (i << 4);

    if      (js.rgdwPOV[i] == JOY_POVFORWARD )
      return joy;
    else if (js.rgdwPOV[i] == JOY_POVRIGHT   )
      return joy + 1;
    else if (js.rgdwPOV[i] == JOY_POVBACKWARD)
      return joy + 3;
    else if (js.rgdwPOV[i] == JOY_POVLEFT    )
      return joy + 7;
  }

  if ((js.lX > joy_center - 0x1000) && (js.lX < joy_center + 0x1000))
    x_flag = 1;

  if      ((js.lX > (joy_center + 0x4000)) && x_flag) {
    x_flag = 0;
    return WS_JOY_AXIS_X_P;
  }
  else if ((js.lX < (joy_center - 0x4000)) && x_flag) {
    x_flag = 0;
    return WS_JOY_AXIS_X_M;
  }

  if ((js.lY > joy_center - 0x1000) && (js.lY < joy_center + 0x1000))
    y_flag = 1;

  if      ((js.lY > (joy_center + 0x4000)) && y_flag) {
    y_flag = 0;
    return WS_JOY_AXIS_Y_P;
  }
  else if ((js.lY < (joy_center - 0x4000)) && y_flag) {
    y_flag = 0;
    return WS_JOY_AXIS_Y_M;
  }

  if ((js.lZ > joy_center - 0x1000) && (js.lZ < joy_center + 0x1000))
    z_flag = 1;

  if      ((js.lZ > (joy_center + 0x4000)) && z_flag) {
    z_flag = 0;
    return WS_JOY_AXIS_Z_P;
  }
  else if ((js.lZ < (joy_center - 0x4000)) && z_flag) {
    z_flag = 0;
    return WS_JOY_AXIS_Z_M;
  }

  if ((js.lRx > joy_center - 0x1000) && (js.lRx < joy_center + 0x1000))
    rx_flag = 1;

  if      ((js.lRx > (joy_center + 0x4000)) && rx_flag) {
    rx_flag = 0;
    return WS_JOY_AXIS_RX_P;
  }
  else if ((js.lRx < (joy_center - 0x4000)) && rx_flag) {
    rx_flag = 0;
    return WS_JOY_AXIS_RX_M;
  }

  if ((js.lRy > joy_center - 0x1000) && (js.lRy < joy_center + 0x1000))
    ry_flag = 1;

  if      ((js.lRy > (joy_center + 0x4000)) && ry_flag) {
    ry_flag = 0;
    return WS_JOY_AXIS_RY_P;
  }
  else if ((js.lRy < (joy_center - 0x4000)) && ry_flag) {
    ry_flag = 0;
    return WS_JOY_AXIS_RY_M;
  }

  if ((js.lRz > joy_center - 0x1000) && (js.lRz < joy_center + 0x1000))
    rz_flag = 1;

  if      ((js.lRz > (joy_center + 0x4000)) && rz_flag) {
    rz_flag = 0;
    return WS_JOY_AXIS_RZ_P;
  }
  else if ((js.lRz < (joy_center - 0x4000)) && rz_flag) {
    rz_flag = 0;
    return WS_JOY_AXIS_RZ_M;
  }
    
  if ((js.rglSlider[0] > joy_center - 0x1000) && (js.rglSlider[0] < joy_center + 0x1000))
    s1_flag = 1;

  if      ((js.rglSlider[0] > (joy_center + 0x4000)) && s1_flag) {
    s1_flag = 0;
    return WS_JOY_SLIDER_P;
  }
  else if ((js.rglSlider[0] < (joy_center - 0x4000)) && s1_flag) {
    s1_flag = 0;
    return WS_JOY_SLIDER_M;
  }

  if ((js.rglSlider[1] > joy_center - 0x1000) && (js.rglSlider[0] < joy_center + 0x1000))
    s2_flag = 1;

  if      ((js.rglSlider[1] > (joy_center + 0x4000)) && s2_flag) {
    s2_flag = 0;
    return WS_JOY_SLIDER2_P;
  }
  else if ((js.rglSlider[1] < (joy_center - 0x4000)) && s2_flag) {
    s2_flag = 0;
    return WS_JOY_SLIDER2_M;
  }

  return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static LRESULT CALLBACK EditProc1(HWND hEditWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (message == WM_CHAR)
    return 0;

  if ((message == WM_KEYDOWN || message == WM_HOTKEY) && (wParam != VK_TAB)) {
    int *key = conf_getKey(hEditWnd);

    wParam = NULL;
		
    HRESULT hRet;
    BYTE    diKeys[256];
	
    hRet = lpKeyDevice->Acquire();
    if (hRet == DI_OK || hRet == S_FALSE) {
      hRet = lpKeyDevice->GetDeviceState(256, diKeys);
      if (hRet == DI_OK) {
        for (int i = 0; i < 256; i++) {
          if (diKeys[i] & 0x80) {
            SetWindowText(hEditWnd, conf_dispCode1(i));
            *key   = i;
            wParam = VK_TAB;
            break;
          }
        }
      }
    }
  }

  return CallWindowProc(Org_WndProc1, hEditWnd, message, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static LRESULT CALLBACK EditProc2(HWND hEditWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
  if (message == WM_CHAR)
    return 0;
	
  if (message == WM_KEYDOWN) {
    int *key;

    switch (wParam) {
    case VK_TAB:
    case VK_RETURN:
      wParam = VK_TAB;
      break;
    default:
      key  = conf_getKey(hEditWnd);
      *key = 0;
      SetWindowText(hEditWnd, "");
      return 0;
    }
  }
  
  return CallWindowProc(Org_WndProc1, hEditWnd, message, wParam, lParam);
}
#endif

