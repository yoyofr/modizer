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

#ifndef __OSWAN_H__
#define __OSWAN_H__

#include <stdlib.h>
#include <stdarg.h>

#define OSWAN_CAPTION "Oswan"
#define OSWAN_VERSION "1.7.3"

#define WS_SYSTEM_MONO  0
#define WS_SYSTEM_COLOR 1

#define FILTER_NORMAL    0
#define FILTER_SCANLINES 1
#define FILTER_HALFSCAN  2
#define FILTER_SIMPLE2X  3

#define LCD_SEGMENT_WIDTH 8
#if 0
extern HWND      hWnd;
extern HWND      hSBWnd;
extern int       app_new_rom;
extern int       app_gameRunning;
extern int       DrawFilter;
extern int       DrawSize;
extern int       DrawFull;
extern int       DrawMode;
extern int       OldMode;
extern BYTE      OldPort15;
extern int       LCDSegment;
extern char      recentOfn0[];
extern char      recentOfn1[];
extern char      recentOfn2[];
extern char      recentOfn3[];
extern char      recentOfn4[];
extern char*     recent[];
extern char      WsHomeDir[];
extern char      WsRomDir[];
extern char      WsSramDir[];
extern char      WsSaveDir[];
extern char      WsShotDir[];
extern char      WsWavDir[];
extern char      BaseName[];
extern int       ws_colourScheme;
extern int       ws_system;
extern char      ws_rom_path[];
extern int       ScrShoot;
extern int       StatusWait;
extern DWORD     backbuffer[];

void oswan_set_status_text(int wait, const char* format, ...);
void oswan_set_recent_rom();
void SetStateInfo(void);
void WsResize(void);
void WsPause(void);
#endif
#endif
