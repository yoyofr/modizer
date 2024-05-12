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

#include <stdio.h>
//#include "resource.h"
#include "oswan.h"
#include "log.h"
#include "ws.h"
#include "io.h"

//HWND		hMapWnd = NULL;
//HWND		hCtrlWnd = NULL;

LRESULT CALLBACK MapViewerProc(HWND hMapWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char	buf[256];
	int		i, index;
	HFONT	hFont;

	switch (msg)
	{
	case WM_INITDIALOG:
		WsPause();
		hCtrlWnd = GetDlgItem(hMapWnd, IDC_LIST_IO);
		hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
		SendMessage(hCtrlWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		DeleteObject(hFont);
		for (i = 0; i < 256; i++)
		{
			sprintf(buf, "$%02X:0x%02X(%d)", i, ws_ioRam[i], ws_ioRam[i]);
			SendMessage(hCtrlWnd, LB_ADDSTRING, 0, (LPARAM)buf);
		}

		hCtrlWnd = GetDlgItem(hMapWnd, IDC_LIST_MEMORY);
		hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
		SendMessage(hCtrlWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		DeleteObject(hFont);
		for (i = 0x1000; i < 0x4000; i+=8)
		{
			sprintf(buf, "%04X:%02X %02X %02X %02X %02X %02X %02X %02X",
				i, internalRam[i], internalRam[i+1], internalRam[i+2], internalRam[i+3],
				internalRam[i+4], internalRam[i+5], internalRam[i+6], internalRam[i+7]);
			SendMessage(hCtrlWnd, LB_ADDSTRING, 0, (LPARAM)buf);
		}
		break;
	case WM_DESTROY:
		if (!app_gameRunning) WsPause();
		break;
	case WM_COMMAND:
		if(HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				DestroyWindow(hMapWnd);
				break;
			case IDCANCEL:
				DestroyWindow(hMapWnd);
				break;
			case IDC_BUTTON_RUN:
				WsPause();
				if (!app_gameRunning)
				{
					hCtrlWnd = GetDlgItem(hMapWnd, IDC_LIST_IO);
					index = SendMessage(hCtrlWnd, LB_GETTOPINDEX, 0, 0);
					SendMessage(hCtrlWnd, LB_RESETCONTENT, 0, 0);
					for (i = 0; i < 256; i++)
					{
						sprintf(buf, "$%02X:0x%02X(%d)", i, ws_ioRam[i], ws_ioRam[i]);
						SendMessage(hCtrlWnd, LB_ADDSTRING, 0, (LPARAM)buf);
					}
					SendMessage(hCtrlWnd, LB_SETTOPINDEX, (WPARAM)index, 0);

					hCtrlWnd = GetDlgItem(hMapWnd, IDC_LIST_MEMORY);
					index = SendMessage(hCtrlWnd, LB_GETTOPINDEX, 0, 0);
					SendMessage(hCtrlWnd, LB_RESETCONTENT, 0, 0);
					for (i = 0x1000; i < 0x4000; i+=8)
					{
						sprintf(buf, "%04X:%02X %02X %02X %02X %02X %02X %02X %02X",
							i, internalRam[i], internalRam[i+1], internalRam[i+2], internalRam[i+3],
							internalRam[i+4], internalRam[i+5], internalRam[i+6], internalRam[i+7]);
						SendMessage(hCtrlWnd, LB_ADDSTRING, 0, (LPARAM)buf);
					}
					SendMessage(hCtrlWnd, LB_SETTOPINDEX, (WPARAM)index, 0);
				}
			}
			return 1;
		}
		break;
	}
	return 0;
}
