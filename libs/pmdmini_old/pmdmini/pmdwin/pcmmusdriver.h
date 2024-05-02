//=============================================================================
//		PCM Music Driver Interface Class : IPCMMUSICDRIVER
//=============================================================================

#ifndef IPCMMUSICDRIVER_H
#define IPCMMUSICDRIVER_H

#if defined _WIN32
#include <objbase.h>
#include <tchar.h>
#else
#include "pmdmini_file.h"
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
//typedef unsigned DWORD;
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#ifndef __cplusplus
typedef unsigned char bool;
#endif


//=============================================================================
// IPCMMUSICDRIVER : 音源ドライバの基本的なインターフェイスを定義したクラス
//=============================================================================

interface IPCMMUSICDRIVER : public IUnknown {
	virtual bool WINAPI init(TCHAR *path) = 0;
	virtual int WINAPI music_load(TCHAR *filename) = 0;
	virtual int WINAPI music_load2(uchar *musdata, int size) = 0;
	virtual TCHAR* WINAPI getmusicfilename(TCHAR *dest) = 0;
	virtual void WINAPI music_start(void) = 0;
	virtual void WINAPI music_stop(void) = 0;
	virtual int WINAPI getloopcount(void) = 0;
	virtual bool WINAPI getlength(TCHAR *filename, int *length, int *loop) = 0;
	virtual int WINAPI getpos(void) = 0;
	virtual void WINAPI setpos(int pos) = 0;
	virtual void WINAPI getpcmdata(short *buf, int nsamples) = 0;
};


//=============================================================================
// IFMPMD : WinFMP, PMDWin に共通なインターフェイスを定義したクラス
//=============================================================================
interface IFMPMD : public IPCMMUSICDRIVER {
	virtual bool WINAPI loadrhythmsample(TCHAR *path) = 0;
	virtual bool WINAPI setpcmdir(TCHAR **path) = 0;
	virtual void WINAPI setpcmrate(int rate) = 0;
	virtual void WINAPI setppzrate(int rate) = 0;
	virtual void WINAPI setfmcalc55k(bool flag) = 0;
	virtual void WINAPI setppzinterpolation(bool ip) = 0;
	virtual void WINAPI setfmwait(int nsec) = 0;
	virtual void WINAPI setssgwait(int nsec) = 0;
	virtual void WINAPI setrhythmwait(int nsec) = 0;
	virtual void WINAPI setadpcmwait(int nsec) = 0;
	virtual void WINAPI fadeout(int speed) = 0;
	virtual void WINAPI fadeout2(int speed) = 0;
	virtual bool WINAPI getlength2(TCHAR *filename, int *length, int *loop) = 0;
	virtual int WINAPI getpos2(void) = 0;
	virtual void WINAPI setpos2(int pos) = 0;
	virtual TCHAR* WINAPI getpcmfilename(TCHAR *dest) = 0;
	virtual TCHAR* WINAPI getppzfilename(TCHAR *dest, int bufnum) = 0;
};


//=============================================================================
// Interface ID(IID)
//=============================================================================

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32
// GUID of IPCmMUSICDRIVER Interface ID
interface	__declspec(uuid("9D4D6317-F40A-455E-9E2C-CB517556BA02")) IPCMMUSICDRIVER;

// GUID of IFMPMD Interface ID
interface	__declspec(uuid("81977D60-9496-4F20-A3BB-19B19943DA6D")) IFMPMD;


const IID IID_IPCMMUSICDRIVER	= _uuidof(IPCMMUSICDRIVER);	// IPCMMUSICDRIVER Interface ID
const IID IID_IFMPMD			= _uuidof(IFMPMD);			// IFMPMD Interface ID
#endif

#ifdef __cplusplus
}
#endif


#endif	// IPCMMUSICDRIVER_H
