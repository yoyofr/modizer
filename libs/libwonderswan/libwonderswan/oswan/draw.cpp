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

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
//#include <windows.h>
#include "wsr_types.h" //YOYOFR

#include <stdio.h>
#include <stdlib.h>
#include <mbstring.h>
#include <commctrl.h>
#include "oswan.h"
#include "log.h"
#include "draw.h"
#include "gpu.h"
#include "segment.h"
#include "segmentM.h"
#include "io.h"

//-----------------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------------
LPDIRECTDRAW7				lpDD = NULL;
LPDIRECTDRAWSURFACE7		lpDDSPrimary = NULL;
LPDIRECTDRAWSURFACE7		lpDDSBack = NULL;
LPDIRECTDRAWSURFACE7		lpDDSSegment = NULL;
LPDIRECTDRAWCLIPPER			lpClipper = NULL;

RECT		WsRect;
POINT		PosOrig;
DWORD		DispHeight, DispWidth;
int			RGBBitCount, PixelDepth;
DWORD		RBitMask, GBitMask, BBitMask;
int			SftR, SftG, SftB;
long		Pitch;
DWORD		VBFreq;
DWORD		*pfilter = NULL;
int			FullFrag = 0;

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
BYTE header[54] = {
	66,77,54,122,1,0,0,0,
	0,0,54,0,0,0,40,0,
	0,0,144,0,0,0,224,0,
	0,0,1,0,16,0,0,0,
	0,0,0,122,1,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0
};
DWORD filecount=0;

void screenshot(void)
{
	DWORD R, G, B, C;
	char fname[1024];
	FILE *fp;

	if (!ws_rom_path[0])
		return;
	if(DrawMode & 1)
	{
		header[0x12] = 144;
		header[0x16] = 224;
	}
	else
	{
		header[0x12] = 224;
		header[0x16] = 144;
	}

	while (1)
	{
		sprintf(fname, "%s%s[%.3i].bmp", WsShotDir, BaseName, filecount++);
		fp = fopen(fname,"r");
		if (fp)
			fclose(fp);
		else if (filecount > 100)
			return;
		else
			break;
	}

	fp = fopen(fname, "wb");
	fwrite(header, 1, 54, fp);

	if(DrawMode & 1)
	{
		for (int line = 0; line < 144; line++) {
			for (int column = 0; column < 224; column++) {
				pfilter[line + (((223 - column) << 7) + ((223 - column) << 4))]
					= backbuffer[column + (line << 7) + (line << 6) + (line << 5)];
			}
		}
		for (int i = 0; i < 224; i++)
		{
			for (int j = 0; j < 144; j++)
			{
				C = pfilter[(223 - i) * 144 + j];
				R = (C & RBitMask) << SftR;
				G = (C & GBitMask) << SftG;
				B = (C & BBitMask) << SftB;
				C = (R >> 17) | (G >> 22) | (B >> 27);
				fwrite(&C, 1, 2, fp);
			}
		}
	}
	else
	{
		for (int i = 0; i < 144; i++)
		{
			for (int j = 0; j < 224; j++)
			{
				C = backbuffer[(143 - i) * 224 + j];
				R = (C & RBitMask) << SftR;
				G = (C & GBitMask) << SftG;
				B = (C & BBitMask) << SftB;
				C = (R >> 17) | (G >> 22) | (B >> 27);
				fwrite(&C, 1, 2, fp);
			}
		}
	}
	fclose(fp);
    oswan_set_status_text(1, "Screenshot %d", filecount);
}
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
void DrawErr(LPCTSTR szError)
{
	MessageBox(NULL, szError, "DDraw error", MB_ICONEXCLAMATION | MB_OK);
}
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
void WsDrawClear(void)
{
	HRESULT hRet;
	DDBLTFX DDBFX;

	ZeroMemory(&DDBFX, sizeof(DDBFX));
	DDBFX.dwSize = sizeof(DDBFX);
	DDBFX.dwFillColor = 0;
	if(lpDDSBack == NULL)
	{
		return;
	}

	hRet = lpDDSBack->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &DDBFX);
	if (hRet != DD_OK)
	{
		return;
	}

	hRet = lpDDSPrimary->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &DDBFX);
	if(hRet != DD_OK)
	{
		return;
	}

	if (ws_system == WS_SYSTEM_COLOR)
	{
		hRet = lpDDSSegment->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &DDBFX);
		if(hRet != DD_OK)
		{
			return;
		}
	}
	else
	{
		DDBFX.dwFillColor = MB_COLOR;
		hRet = lpDDSSegment->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &DDBFX);
		if(hRet != DD_OK)
		{
			return;
		}
	}
}
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
void WsDrawRelease(void)
{
	if (lpDD != NULL)
	{
		if (lpDDSPrimary != NULL)
		{
			if(lpClipper != NULL)
			{
				lpClipper->Release();
				lpClipper = NULL;
			}
			lpDDSPrimary->Release();
			lpDDSPrimary = NULL;
		}
		if(lpDDSBack != NULL)
		{
			lpDDSBack->Release();
			lpDDSBack = NULL;
		}
		if(lpDDSSegment != NULL)
		{
			lpDDSSegment->Release();
			lpDDSSegment = NULL;
		}
	}
}
void WsDrawDone(void)
{
	WsDrawRelease();
	if (lpDD != NULL)
	{
		if(DrawFull)
		{
			lpDD->RestoreDisplayMode();
			FullFrag = 0;
			DrawFull = 0;
		}
		lpDD->Release();
		lpDD = NULL;
	}
	if (pfilter) {
		free(pfilter);
		pfilter = NULL;
	}
}
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
int WsDrawInit(void)
{
	HRESULT hRet;
	DDSURFACEDESC2 ddsd;

	pfilter = (DWORD*)malloc(224*144*4*sizeof(DWORD));
	hRet = DirectDrawCreateEx(NULL, (VOID**)&lpDD, IID_IDirectDraw7, NULL);
	if(hRet != DD_OK)
	{
		DrawErr("DirectDrawCreate FAILED");
		WsDrawRelease();
		return -1;
	}

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_ALL;
	hRet = lpDD->GetDisplayMode(&ddsd);
	if (hRet !=DD_OK)
	{
		DrawErr("GetDisplayMode FAILED");
		WsDrawRelease();
		return -1;
	}
	DispHeight = ddsd.dwHeight;
	DispWidth = ddsd.dwWidth;
	VBFreq = ddsd.dwRefreshRate;
	RGBBitCount = ddsd.ddpfPixelFormat.dwRGBBitCount;
	RBitMask = ddsd.ddpfPixelFormat.dwRBitMask;
	GBitMask = ddsd.ddpfPixelFormat.dwGBitMask;
	BBitMask = ddsd.ddpfPixelFormat.dwBBitMask;

	log_write("video: display height  %d\n", DispHeight);
	log_write("video: display width   %d\n", DispWidth);
	log_write("video: refresh rate    %d\n", VBFreq);
	log_write("video: RGB bitcount    %d\n", RGBBitCount);
	log_write("video: red bitmask     %08X\n", RBitMask);
	log_write("video: green bitmask   %08X\n", GBitMask);
	log_write("video: blue bitmask    %08X\n\n", BBitMask);

	DWORD			tmp;
	int				sft;
	for (tmp = RBitMask, sft = 0; sft < 32; sft++)
	{
		if (tmp & 0x80000000)
		{
			SftR = sft;
			break;
		}
		tmp <<= 1;
	}
	for (tmp = GBitMask, sft = 0; sft < 32; sft++)
	{
		if (tmp & 0x80000000)
		{
			SftG = sft;
			break;
		}
		tmp <<= 1;
	}
	for (tmp = BBitMask, sft = 0; sft < 32; sft++)
	{
		if (tmp & 0x80000000)
		{
			SftB = sft;
			break;
		}
		tmp <<= 1;
	}

	if (RGBBitCount == 16)
	{
		 PixelDepth = 2;
	}
	else if (RGBBitCount <= 24)
	{
		 PixelDepth = 3;
	}
	else if (RGBBitCount <= 32)
	{
		 PixelDepth = 4;
	}
	else
	{
		DrawErr("RGBBitCount error");
		WsDrawRelease();
		return -1;
	}
	return 0;
}
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
int WsDrawCreate(HWND hWnd)
{
	HRESULT			hRet;
	DDSURFACEDESC2	ddsd;

	if (DrawFull)
	{
		hRet = lpDD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
		if(hRet != DD_OK)
		{
			WsDrawDone();
			DrawErr("SetCooperativeLevel FAILED");
			return -1;
		}
		if (FullFrag == 0) {
			hRet = lpDD->SetDisplayMode(640, 480, RGBBitCount, 0, 0);
			if(hRet != DD_OK)
			{
				WsDrawDone();
				DrawErr("SetDisplayMode FAILED");
				return -1;
			}
			FullFrag = 1;
		}
	}
	else
	{
		hRet = lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL);
		if(hRet != DD_OK)
		{
			DrawErr("SetCooperativeLevel FAILED");
			WsDrawRelease();
			return -1;
		}
	}

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	hRet=lpDD->CreateSurface(&ddsd, &lpDDSPrimary, NULL);
	if (hRet != DD_OK)
	{
		DrawErr("CreateSurface FAILED");
		WsDrawRelease();
		return -1;
	}

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
//	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	if (DrawFilter == FILTER_NORMAL)
	{
		if(DrawMode & 0x01)
		{
			ddsd.dwWidth=144;
			ddsd.dwHeight=224;
		}
		else
		{
			ddsd.dwWidth=224;
			ddsd.dwHeight=144;
		}
	}
	else
	{
		if(DrawMode & 0x01)
		{
			ddsd.dwWidth=144 * 2;
			ddsd.dwHeight=224 * 2;
		}
		else
		{
			ddsd.dwWidth=224 * 2;
			ddsd.dwHeight=144 * 2;
		}
	}
	hRet = lpDD->CreateSurface(&ddsd, &lpDDSBack, NULL);
	if(hRet != DD_OK)
	{
		DrawErr("CreateSurface FAILED");
		WsDrawRelease();
		return -1;
	}
	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_ALL;
	hRet = lpDDSBack->GetSurfaceDesc(&ddsd);
	if (hRet != DD_OK)
	{
		DrawErr("GetSurfaceDesc FAILED");
		WsDrawRelease();
		return -1;
	}
	Pitch=ddsd.lPitch;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

	if(ws_system == WS_SYSTEM_COLOR)
	{
		if(DrawMode & 0x01)
		{
			ddsd.dwWidth = 144 * 2;
			ddsd.dwHeight = LCD_SEGMENT_WIDTH * 2;
		}
		else
		{
			ddsd.dwWidth = LCD_SEGMENT_WIDTH * 2;
			ddsd.dwHeight = 144 * 2;
		}
	}
	else
	{
		if(DrawMode & 0x01)
		{
			ddsd.dwWidth = LCD_SEGMENT_WIDTH * 2;
			ddsd.dwHeight = 224 * 2;
		}
		else
		{
			ddsd.dwWidth = 224 * 2;
			ddsd.dwHeight = LCD_SEGMENT_WIDTH * 2;
		}
	}
	hRet = lpDD->CreateSurface(&ddsd, &lpDDSSegment, NULL);
	if(hRet != DD_OK)
	{
		DrawErr("CreateSurface FAILED");
		WsDrawRelease();
		return -1;
	}

	if (!DrawFull)
	{
		hRet = lpDD->CreateClipper(0, &lpClipper, NULL);
		if (hRet != DD_OK)
		{
			DrawErr("CreateClipper FAILED");
			WsDrawRelease();
			return -1;
		}
		hRet = lpClipper->SetHWnd(0, hWnd);
		if (hRet != DD_OK)
		{
			DrawErr("SetHWnd FAILED");
			WsDrawRelease();
			return -1;
		}
		hRet = lpDDSPrimary->SetClipper(lpClipper);
		if (hRet != DD_OK)
		{
			DrawErr("SetClipper FAILED");
			WsDrawRelease();
			return -1;
		}
	}

	HDC hDc;
	RECT rect;

	hDc = GetDC(hWnd);
	GetDCOrgEx(hDc, &PosOrig);
	ReleaseDC(hWnd, hDc);
	GetClientRect(hWnd, &rect);

	WsRect.left=PosOrig.x + rect.left;
	WsRect.top=PosOrig.y + rect.top;
	if (DrawFull)
	{
		if(DrawMode & 0x01)
		{
			WsRect.left = (640 - 144 * 2) / 2;
			WsRect.top = (480 - 224 * 2) / 2;
			WsRect.right = WsRect.left + 144 * 2;
			WsRect.bottom = WsRect.top + 224 * 2;
		}
		else
		{
			WsRect.left = (640 - 224 * 2) / 2;
			WsRect.top = (480 - 144 * 2) / 2;
			WsRect.right = WsRect.left + 224 * 2;
			WsRect.bottom = WsRect.top + 144 * 2;
		}
	}
	else
	{
		if(DrawMode & 0x01)
		{
			WsRect.right = WsRect.left + 144 * DrawSize;
			WsRect.bottom = WsRect.top + 224 * DrawSize;
		}
		else
		{
			WsRect.right = WsRect.left + 224 * DrawSize;
			WsRect.bottom = WsRect.top + 144 * DrawSize;
		}
	}

	WsDrawClear();
	return 0;
}
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
void WsDrawFilter(void)
{
	DWORD *psrcd, *pdestd;
	int i, j;

	switch (DrawFilter)
	{
	case FILTER_NORMAL:
		memcpy(pfilter, backbuffer, 224 * 144 * 4);
		break;
	case FILTER_SCANLINES:
		pdestd = pfilter;
		psrcd = backbuffer;
		for (i = 0; i < 144; i++) {
			for (j = 0; j < 224; j++) {
				*pdestd++ = *psrcd;
				*pdestd++ = *psrcd++;
			}
			for (j = 0; j < 224; j++) {
				*pdestd++ = 0;
				*pdestd++ = 0;
			}
		}
		break;
	case FILTER_HALFSCAN:
		pdestd = pfilter;
		psrcd = backbuffer;
		for (i = 0; i < 144; i++) {
			for (j = 0; j < 224; j++) {
				*pdestd++ = *psrcd;
				*pdestd++ = *psrcd++;
			}
			psrcd -= 224;
			for (j = 0; j < 224; j++) {
				// Pixel depth = 4
				*pdestd++ = *psrcd & 0x00A8A8A8;
				*pdestd++ = *psrcd++ & 0x00A8A8A8;
			}
		}
		break;
	case FILTER_SIMPLE2X:
		pdestd = pfilter;
		psrcd = backbuffer;
		for (i = 0; i < 144; i++) {
			for (j = 0; j < 224; j++) {
				*pdestd++ = *psrcd;
				*pdestd++ = *psrcd++;
			}
			psrcd -= 224;
			for (j = 0; j < 224; j++) {
				*pdestd++ = *psrcd;
				*pdestd++ = *psrcd++;
			}
		}
	}
}
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
void WsDrawRotate(void)
{
	int pos;
	static DWORD temp[224 * 144 * 4 * 4];
	memcpy(temp, pfilter, 224 * 144 * 4 * 4);

	if (DrawMode & 1)
	{
		if (DrawFilter == FILTER_NORMAL)
		{
			for (int line = 0; line < 144; line++) {
				for (int column = 0; column < 224; column++) {
					pos = line + (((223 - column) << 7) + ((223 - column) << 4));
					pfilter[pos] = temp[column + (line << 7) + (line << 6) + (line << 5)];
				}
			}
		}
		else
		{
			for (int line = 0; line < 144 * 2; line++) {
				for (int column = 0; column < 224 * 2; column++) {
					pos = line + (((447 - column) << 8) + ((447 - column) << 5));
					pfilter[pos] = temp[column + (line << 8) + (line << 7) + (line << 6)];
				}
			}
		}
	}
	if (DrawMode & 2)
	{
		if (DrawFilter == FILTER_NORMAL)
		{
			memcpy(temp, pfilter, 224 * 144 * 4);
			for (int i = 0; i < 144 * 224; i++) {
				pfilter[i] = temp[224 * 144 - 1 - i];
			}
		}
		else
		{
			memcpy(temp, pfilter, 224 * 144 * 4 * 4);
			for (int i = 0; i < 144 * 224 * 4; i++) {
				pfilter[i] = temp[224 * 144 * 4 - 1 - i];
			}
		}
	}
}
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
int WsDrawFrame(void)
{
	HRESULT			hRet;
	DDSURFACEDESC2	ddsd;
	RECT			rect;
	int				bufferSize;
	WORD			i, j;
	BYTE			*pdest, *psrc;
	WORD			*pdestw, *psrcw;
	DWORD			*pdestd, *psrcd;

	if(lpDDSBack == NULL)
	{
		DrawErr("No Backbuffer");
		return -1;
	}

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	WsDrawFilter();
	WsDrawRotate();

	if (DrawFilter == FILTER_NORMAL)
	{
		if (DrawMode & 0x01)
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 144;
			rect.bottom = 224;
		}
		else
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 224;
			rect.bottom = 144;
		}
	}
	else
	{
		if (DrawMode & 0x01)
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 144 * 2;
			rect.bottom = 224 * 2;
		}
		else
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 224 * 2;
			rect.bottom = 144 * 2;
		}
	}
	hRet = lpDDSBack->Lock(&rect, &ddsd, DDLOCK_WAIT, NULL);
	if (hRet != DD_OK)
	{
		DrawErr("Backbuffer Lock error");
		return -1;
	}

	if (PixelDepth == 4) {
		bufferSize = (ddsd.lPitch >> 2) - ddsd.dwWidth;
		psrcd = (DWORD*)pfilter;
		pdestd = (DWORD*)ddsd.lpSurface;
		for (i = 0; i < ddsd.dwHeight; i++) {
			for (j = 0; j < ddsd.dwWidth; j++) {
				*pdestd++ = *psrcd++;
			}
			pdestd += bufferSize;
		}
	}
	else if (PixelDepth == 2) {
		bufferSize = (ddsd.lPitch >> 1) - ddsd.dwWidth;
		psrcw = (WORD*)pfilter;
		pdestw = (WORD*)ddsd.lpSurface;
		for (i = 0; i < ddsd.dwHeight; i++) {
			for (j = 0; j < ddsd.dwWidth; j++) {
				*pdestw++ = *psrcw++;
				psrcw++;
			}
			pdestw += bufferSize;
		}
	}
	else if (PixelDepth == 3) {
		bufferSize = ddsd.lPitch - ddsd.dwWidth * 3;
		psrc = (BYTE*)pfilter;
		pdest = (BYTE*)ddsd.lpSurface;
		for (i = 0; i < ddsd.dwHeight; i++) {
			for (j = 0; j < ddsd.dwWidth; j++) {
				*pdest++ = *psrc++;
				*pdest++ = *psrc++;
				*pdest++ = *psrc++;
				psrc++;
			}
			pdest += bufferSize;
		}
	}

	hRet = lpDDSBack->Unlock(&rect);
	if(hRet != DD_OK)
	{
		DrawErr("Backbuffer Unlock error2");
		return -1;
	}
	return 0;
}
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
int WsDrawFlip(void)
{
	HRESULT hRet;
	RECT rect, GbRect, SegRect;

	if(lpDDSBack == NULL)
	{
		DrawErr("Backbuffer error");
		return -1;
	}

	if(lpDDSPrimary==NULL)
	{
		DrawErr("Primary surface error");
		return -1;
	}

	HDC hdc;
	POINT dwPos;

	hdc = GetDC(hWnd);
	GetDCOrgEx(hdc, &dwPos);
	ReleaseDC(hWnd, hdc);

	if(dwPos.x != PosOrig.x)
	{
		WsRect.left += dwPos.x - PosOrig.x;
		WsRect.right += dwPos.x - PosOrig.x;
		PosOrig.x=dwPos.x;
	}
	if(dwPos.y != PosOrig.y)
	{
		WsRect.top += dwPos.y - PosOrig.y;
		WsRect.bottom += dwPos.y - PosOrig.y;
		PosOrig.y=dwPos.y;
	}

	GbRect.left = WsRect.left;
	GbRect.top = WsRect.top;
	GbRect.right = WsRect.right;
	GbRect.bottom = WsRect.bottom;

	if (LCDSegment && !DrawFull)
	{
		if (ws_system == WS_SYSTEM_COLOR)
		{
			switch(DrawMode)
			{
			case 0:
				SegRect.left = GbRect.right;
				SegRect.top = GbRect.top;
				SegRect.right = SegRect.left + LCD_SEGMENT_WIDTH * DrawSize;
				SegRect.bottom = GbRect.bottom;
				break;
			case 1:
				SegRect.left = GbRect.left;
				SegRect.top = GbRect.top;
				SegRect.right = GbRect.right;
				SegRect.bottom = GbRect.top + LCD_SEGMENT_WIDTH * DrawSize;
				GbRect.top += LCD_SEGMENT_WIDTH * DrawSize;
				GbRect.bottom += LCD_SEGMENT_WIDTH * DrawSize;
				break;
			case 2:
				SegRect.left = GbRect.left;
				SegRect.top = GbRect.top;
				SegRect.right = GbRect.left + LCD_SEGMENT_WIDTH * DrawSize;
				SegRect.bottom = GbRect.bottom;
				GbRect.left += LCD_SEGMENT_WIDTH * DrawSize;
				GbRect.right += LCD_SEGMENT_WIDTH * DrawSize;
				break;
			case 3:
				SegRect.left = GbRect.left;
				SegRect.top = GbRect.bottom;
				SegRect.right = GbRect.right;
				SegRect.bottom = GbRect.bottom + LCD_SEGMENT_WIDTH * DrawSize;
				break;
			}
		}
		else
		{
			switch(DrawMode)
			{
			case 0:
				SegRect.left = GbRect.left;
				SegRect.top = GbRect.bottom;
				SegRect.right = GbRect.right;
				SegRect.bottom = GbRect.bottom + LCD_SEGMENT_WIDTH * DrawSize;
				break;
			case 1:
				SegRect.left = GbRect.right;
				SegRect.top = GbRect.top;
				SegRect.right = SegRect.left + LCD_SEGMENT_WIDTH * DrawSize;
				SegRect.bottom = GbRect.bottom;
				break;
			case 2:
				SegRect.left = GbRect.left;
				SegRect.top = GbRect.top;
				SegRect.right = GbRect.right;
				SegRect.bottom = GbRect.top + LCD_SEGMENT_WIDTH * DrawSize;
				GbRect.top += LCD_SEGMENT_WIDTH * DrawSize;
				GbRect.bottom += LCD_SEGMENT_WIDTH * DrawSize;
				break;
			case 3:
				SegRect.left = GbRect.left;
				SegRect.top = GbRect.top;
				SegRect.right = GbRect.left + LCD_SEGMENT_WIDTH * DrawSize;
				SegRect.bottom = GbRect.bottom;
				GbRect.left += LCD_SEGMENT_WIDTH * DrawSize;
				GbRect.right += LCD_SEGMENT_WIDTH * DrawSize;
				break;
			}
		}
	}

	if (DrawFilter == FILTER_NORMAL)
	{
		if(DrawMode & 0x01)
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 144;
			rect.bottom = 224;
		}
		else
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 224;
			rect.bottom = 144;
		}
	}
	else
	{
		if(DrawMode & 0x01)
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 144 * 2;
			rect.bottom = 224 * 2;
		}
		else
		{
			rect.left = 0;
			rect.top = 0;
			rect.right = 224 * 2;
			rect.bottom = 144 * 2;
		}
	}

	DDBLTFX ddBltFx;
	ddBltFx.dwSize = sizeof(ddBltFx);
	ddBltFx.dwDDFX = 0;
	hRet = lpDDSPrimary->Blt(&GbRect, lpDDSBack, &rect, DDBLT_WAIT, &ddBltFx);
	if(hRet != DD_OK)
	{
		DrawErr("Blt error");
		return -1;
	}

	if (LCDSegment && !DrawFull)
	{
		if (ws_system == WS_SYSTEM_COLOR)
		{
			if(DrawMode & 0x01)
			{
				rect.left = 0;
				rect.top = 0;
				rect.right = 144 * 2;
				rect.bottom = LCD_SEGMENT_WIDTH * 2;
			}
			else
			{
				rect.left = 0;
				rect.top = 0;
				rect.right = LCD_SEGMENT_WIDTH * 2;
				rect.bottom = 144 * 2;
			}
		}
		else
		{
			if(DrawMode & 0x01)
			{
				rect.left = 0;
				rect.top = 0;
				rect.right = LCD_SEGMENT_WIDTH * 2;
				rect.bottom = 224 * 2;
			}
			else
			{
				rect.left = 0;
				rect.top = 0;
				rect.right = 224 * 2;
				rect.bottom = LCD_SEGMENT_WIDTH * 2;
			}
		}
		if(DrawMode & 0x02)
			ddBltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN;
		else
			ddBltFx.dwDDFX = 0;
		hRet = lpDDSPrimary->Blt(&SegRect, lpDDSSegment, &rect, DDBLT_WAIT | DDBLT_DDFX, &ddBltFx);
		if(hRet != DD_OK)
		{
			DrawErr("Segment Blt error");
			return -1;
		}
	}
	return 0;
}
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
int	WsDrawLCDSegment(void)
{
	HRESULT			hRet;
	DDSURFACEDESC2	ddsd;
	RECT			rect;
	DWORD			*pdestd, *psrcd;
	BYTE			*pdest, *psrc;
	WORD			*pdestw, *psrcw;
	int				buffer, pos;
	WORD			i, j;
	BYTE			lcd;
	DWORD			temp;

	if (LCDSegment == 0 || DrawFull)
		return 0;

	ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if(lpDDSSegment == NULL)
	{
		DrawErr("No Segmentbuffer");
		return -1;
	}

	DDBLTFX DDBFX;

	ZeroMemory(&DDBFX, sizeof(DDBFX));
	DDBFX.dwSize = sizeof(DDBFX);

	if (ws_system == WS_SYSTEM_COLOR)
	{
		DDBFX.dwFillColor = 0;
	}
	else
	{
		DDBFX.dwFillColor = MB_COLOR;
	}
	hRet = lpDDSSegment->Blt(NULL, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &DDBFX);
	if(hRet != DD_OK)
	{
		return -1;
	}

	lcd = ws_ioRam[0x15];
	for (int disp = 0; disp < 6; disp++)
	{
		if (lcd & 1)
		{
			if (ws_system ==  WS_SYSTEM_COLOR)
			{
				switch (disp)
				{
				case 0:
					pos = 0x100;
					psrcd = sleep;
					break;
				case 1:
					pos = 0x50;
					psrcd = vertical;
					break;
				case 2:
					pos = 0x40;
					psrcd = horizontal;
					break;
				case 3:
					pos = 0x2E;
					psrcd = circle1;
					break;
				case 4:
					pos = 0x20;
					psrcd = circle2;
					break;
				case 5:
					pos = 0x10;
					psrcd = circle3;
					break;
				}
				if(DrawMode & 0x01)
				{
					rect.left = pos;
					rect.top = 0;
					rect.right = pos + LCD_SEGMENT_WIDTH * 2;
					rect.bottom = LCD_SEGMENT_WIDTH * 2;
				}
				else
				{
					rect.left = 0;
					rect.top = pos;
					rect.right = LCD_SEGMENT_WIDTH * 2;
					rect.bottom = pos + LCD_SEGMENT_WIDTH * 2;
				}
			}
			else
			{
				switch (disp)
				{
				case 0:
					pos = 0x20;
					psrcd = sleepM;
					break;
				case 1:
					pos = 0x150;
					psrcd = verticalM;
					break;
				case 2:
					pos = 0x160;
					psrcd = horizontalM;
					break;
				case 3:
					pos = 0x170;
					psrcd = circle1M;
					break;
				case 4:
					pos = 0x17C;
					psrcd = circle2M;
					break;
				case 5:
					pos = 0x18A;
					psrcd = circle3M;
					break;
				}
				if(DrawMode & 0x01)
				{
					rect.left = 0;
					rect.top = 448-(LCD_SEGMENT_WIDTH * 2)-pos;
					rect.right = LCD_SEGMENT_WIDTH * 2;
					rect.bottom = 448-pos;
				}
				else
				{
					rect.left = pos;
					rect.top = 0;
					rect.right = pos + LCD_SEGMENT_WIDTH * 2;
					rect.bottom = LCD_SEGMENT_WIDTH * 2;
				}
			}

			hRet = lpDDSSegment->Lock(&rect, &ddsd, DDLOCK_WAIT, NULL);
			if (hRet != DD_OK)
			{
				DrawErr("Segmentbuffer Lock error");
				return -1;
			}

			if(DrawMode & 0x01)
			{
				for (int line = 0; line < 16; line++) {
					for (int column = 0; column < 16; column++) {
						temp = psrcd[column + (line << 4)];
						pfilter[(line) + ((15 - column) << 4)] = temp;
					}
				}
				psrcd = pfilter;
			}

			buffer = ddsd.dwWidth - 16;
			if (PixelDepth == 4) {
				buffer += (ddsd.lPitch >> 2) - ddsd.dwWidth;
				pdestd = (DWORD*)ddsd.lpSurface;
				for (i = 0; i < 16; i++) {
					for (j = 0; j < 16; j++) {
						*pdestd++ = *psrcd++;
					}
					pdestd += buffer;
				}
			}
			else if (PixelDepth == 2) {
				buffer += (ddsd.lPitch >> 1) - ddsd.dwWidth;
				psrcw = (WORD*)psrcd;
				pdestw = (WORD*)ddsd.lpSurface;
				for (i = 0; i < 16; i++) {
					for (j = 0; j < 16; j++) {
						*pdestw++ = *psrcw++;
						psrcw++;
					}
					pdestw += buffer;
				}
			}
			else if (PixelDepth == 3) {
				buffer *= 3;
				buffer += ddsd.lPitch - ddsd.dwWidth * 3;
				psrc = (BYTE*)psrcd;
				pdest = (BYTE*)ddsd.lpSurface;
				for (i = 0; i < 16; i++) {
					for (j = 0; j < 16; j++) {
						*pdest++ = *psrc++;
						*pdest++ = *psrc++;
						*pdest++ = *psrc++;
						psrc++;
					}
					pdest += buffer;
				}
			}

			hRet = lpDDSSegment->Unlock(&rect);
			if(hRet != DD_OK)
			{
				DrawErr("Segmentbuffer Unlock error");
				return -1;
			}
		}
		lcd = lcd>>1;
	}
	return 0;
}
