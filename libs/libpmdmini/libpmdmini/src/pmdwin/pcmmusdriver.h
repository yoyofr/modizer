//=============================================================================
//		PCM Music Driver Interface Class : PCMMUSICDRIVER
//=============================================================================

#ifndef PCMMUSICDRIVER_H
#define PCMMUSICDRIVER_H

#if defined _WIN32
	#include <windows.h>
	#include <objbase.h>
	#include <tchar.h>
	
#else
	#include <sys/types.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#endif

#include "portability_fmpmd.h"

//=============================================================================
// PCMMUSICDRIVER : 音源ドライバの基本的なインターフェイスを定義したクラス
//=============================================================================

struct IPCMMUSICDRIVER {
	virtual bool WINAPI init(TCHAR *path) = 0;
	virtual int32_t WINAPI music_load(TCHAR *filename) = 0;
	virtual int32_t WINAPI music_load2(uint8_t *musdata, int32_t size) = 0;
	virtual TCHAR* WINAPI getmusicfilename(TCHAR *dest) = 0;
	virtual void WINAPI music_start(void) = 0;
	virtual void WINAPI music_stop(void) = 0;
	virtual int32_t WINAPI getloopcount(void) = 0;
	virtual bool WINAPI getlength(TCHAR *filename, int32_t *length, int32_t *loop) = 0;
	virtual int32_t WINAPI getpos(void) = 0;
	virtual void WINAPI setpos(int32_t pos) = 0;
	virtual void WINAPI getpcmdata(int16_t *buf, int32_t nsamples) = 0;
};


//=============================================================================
// IFMPMD : WinFMP, PMDWin に共通なインターフェイスを定義したクラス
//=============================================================================
struct IFMPMD : public IPCMMUSICDRIVER {
	virtual bool WINAPI loadrhythmsample(TCHAR *path) = 0;
	virtual bool WINAPI setpcmdir(TCHAR **path) = 0;
	virtual void WINAPI setpcmrate(int32_t rate) = 0;
	virtual void WINAPI setppzrate(int32_t rate) = 0;
	virtual void WINAPI setfmcalc55k(bool flag) = 0;
	virtual void WINAPI setppzinterpolation(bool ip) = 0;
	virtual void WINAPI setfmwait(int32_t nsec) = 0;
	virtual void WINAPI setssgwait(int32_t nsec) = 0;
	virtual void WINAPI setrhythmwait(int32_t nsec) = 0;
	virtual void WINAPI setadpcmwait(int32_t nsec) = 0;
	virtual void WINAPI fadeout(int32_t speed) = 0;
	virtual void WINAPI fadeout2(int32_t speed) = 0;
	virtual bool WINAPI getlength2(TCHAR *filename, int32_t *length, int32_t *loop) = 0;
	virtual int32_t WINAPI getpos2(void) = 0;
	virtual void WINAPI setpos2(int32_t pos) = 0;
	virtual TCHAR* WINAPI getpcmfilename(TCHAR *dest) = 0;
	virtual TCHAR* WINAPI getppzfilename(TCHAR *dest, int32_t bufnum) = 0;
};


#endif	// PCMMUSICDRIVER_H
