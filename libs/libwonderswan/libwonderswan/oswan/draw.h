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

#ifndef __DRAW_H__
#define __DRAW_H__
//#include <ddraw.h>

#define DS_1		1
#define DS_2		2
#define DS_3		3
#define DS_4		4
#define DS_FULL		0xFF

//extern LPDIRECTDRAW7	lpDD;
extern int				PixelDepth;
extern DWORD			RBitMask, GBitMask, BBitMask;
extern int				SftR, SftG, SftB;
extern int				FullFrag;

void	screenshot(void);
void	WsDrawClear(void);
int		WsDrawInit(void);
int		WsDrawCreate(HWND hWnd);
void	WsDrawDone(void);
void	WsDrawRelease(void);
int		WsDrawFrame(void);
int		WsDrawFlip(void);
int		WsDrawLCDSegment(void);

#endif
