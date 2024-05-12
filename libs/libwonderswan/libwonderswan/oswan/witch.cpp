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
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
//#include "resource.h"
#include "oswan.h"
#include "draw.h"
#include "log.h"
#include "io.h"
#include "witch.h"

#define XMODEM_SOH 0x01
#define XMODEM_EOT 0x04
#define XMODEM_ACK 0x06
#define XMODEM_NAK 0x15
#define XMODEM_CAN 0x18

HWND		hWitchWnd = NULL;
char		*WitchFile = NULL;
char		file[1024];
int			fh = -1;
int			sh = -1;

////////////////////////////////////////////////////////////////////////////////
//Witch -> PC
////////////////////////////////////////////////////////////////////////////////
int WitchReceiveChar(void)
{
//	fprintf(log_get(),"b3=%02X receive %02X\n",ws_ioRam[0xb3],ws_ioRam[0xb1]);
	if (SerialOutputIndex == 0) return -1;
	if (ws_ioRam[0xb2] & 0x01)
		ws_ioRam[0xb6] |= 0x01; // set interrupt
	BYTE value = SerialOutputBuffer[0];
	SerialOutputIndex--;
	for (int i = 0; i < SerialOutputIndex; i++) {
		SerialOutputBuffer[i] = SerialOutputBuffer[i+1];
	}
	return((int)value);
}
////////////////////////////////////////////////////////////////////////////////
//PC -> Witch
////////////////////////////////////////////////////////////////////////////////
void WitchSendChar(BYTE value)
{
	if (SerialInputIndex > sizeof(SerialInputBuffer))
		return;
	if (ws_ioRam[0xb2] & 0x08)
		ws_ioRam[0xb6] |= 0x08; // set interrupt
	SerialInputBuffer[SerialInputIndex] = value;
	SerialInputIndex++;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
char *WitchOpenFile(void)
{
	static OPENFILENAME ofn;
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWitchWnd;
	ofn.lpstrFile = file;
	ofn.nMaxFile = sizeof(file);
	ofn.lpstrFilter = "All file(*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	ofn.hInstance=NULL;
	// Display the Open dialog box. 
	if (GetOpenFileName(&ofn)==TRUE)
	{
		return(ofn.lpstrFile);
	}
	return(NULL);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
char *WitchSaveFile(void)
{
	OPENFILENAME ofn;
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWitchWnd;
	ofn.lpstrFile = file;
	ofn.nMaxFile = sizeof(file);
	ofn.lpstrFilter = "All file(*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST;
	ofn.hInstance=NULL;
	// Display the Open dialog box. 
	if (GetSaveFileName(&ofn)==TRUE)
	{
		return(ofn.lpstrFile);
	}
	return(NULL);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void WitchSendFile(long	filesize)
{
	static	int		i = 0;
	static	char	buf[128];
	int				j, sum;
	int				dat;

	if (i == 0) {
		while(1) {
			dat = WitchReceiveChar();
			if (dat == -1)
				return;
			// wait NAK
			if (dat == XMODEM_NAK) {
				_read(fh, buf, 128);
				break;
			}
		}
	}
	else {
		dat = WitchReceiveChar();
		if (dat == XMODEM_ACK) {
			_read(fh, buf, 128);
		}else if(dat == XMODEM_NAK) {
			i -= 128;
		}
		else
			return;
	}

	// send data 128byte
    if (i < filesize) {
		// send SOH
		WitchSendChar(XMODEM_SOH);
		// send block number
		WitchSendChar(((i>>7)+1) & 0xff);
		WitchSendChar((~((i>>7)+1)) & 0xff);
		sum = 0;
		for(j=0; j<128; j++) {
			if((i+j) >= filesize)
				dat = 0x1A;
			else
				dat = buf[j];
			sum = (sum + dat) & 0xff;
			WitchSendChar(dat);
		}
		// send checksum
		WitchSendChar(sum);
    }

	i += 128;
	if(i >= filesize) {
		// send EOT
		WitchSendChar(XMODEM_EOT);
		_close(fh);
		fh = -1;
		i = 0;
		SendMessage(hWnd, WM_ACTIVATE, WA_ACTIVE, NULL);
	}
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void WitchReceiveFile(void)
{
	static int	start = 0;
	static int	i;
	int			dat;

	dat = WitchReceiveChar();
	log_write("%02X\n",dat);
	switch (start)
	{
	case 0:
		WitchSendChar(XMODEM_NAK);
		start = 1;
		break;
	case 1:
		if (dat == XMODEM_SOH) {
			start = 2;
			i = 0;
		}
		else if(dat == XMODEM_EOT) {
			WitchSendChar(XMODEM_ACK);
			_close(sh);
			sh = -1;
			start = 0;
			return;
		}
		break;
	case 2:
		if (i > 1 && i < 130) {
			_write(sh, &dat, 1);
		}
		if (i >= 130) {
			start = 1;
			WitchSendChar(XMODEM_ACK);
		}
		i++;
		break;
	}
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WitchProc(HWND hWitchWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static long filesize = 0;

	switch (msg)
	{
	case WM_INITDIALOG:
		break;
	case WM_DESTROY:
		break;
	case WM_USER_SEND:
		if (fh != -1)
			WitchSendFile(filesize);
		if (sh != -1)
			WitchReceiveFile();
		break;
	case WM_COMMAND:
		if(HIWORD(wParam) == BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_BUTTON_SEND_FILE:
				WitchFile = WitchOpenFile();
				if (WitchFile) {
					fh = _open(file, _O_RDONLY | _O_BINARY);
					if (fh != -1) {
						filesize = _filelength(fh);
						WitchSendFile(filesize);
					}
				}
				break;
			case IDC_BUTTON_RECEIVE_FILE:
				WitchFile = WitchSaveFile();
				if (WitchFile) {
					sh = _open(file, _O_WRONLY | _O_CREAT | _O_BINARY, _S_IREAD | _S_IWRITE );
					if (sh != -1) {
						WitchReceiveFile();
					}
				}
				break;
			case IDOK:
				DestroyWindow(hWitchWnd);
				break;
			case IDCANCEL:
				DestroyWindow(hWitchWnd);
				break;
			}
		}
		break;
	case WM_MOVE:
		WsDrawFlip();
		break;
	}
	return 0;
}
