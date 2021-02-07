//=============================================================================
//		PCM Music Driver Interface Class : IPCMMUSICDRIVER
//=============================================================================

#ifndef IPCMMUSICDRIVER_H
#define IPCMMUSICDRIVER_H


typedef unsigned char uchar;
/*  typedef unsigned short ushort; */
/*  typedef unsigned int uint; */
/*  typedef unsigned long ulong; */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
#ifndef __cplusplus
typedef unsigned char bool;
#endif


//=============================================================================
// IPCMMUSICDRIVER : 音源ドライバの基本的なインターフェイスを定義したクラス
//=============================================================================

interface IPCMMUSICDRIVER : public IUnknown {
	virtual bool WINAPI init(char *path) = 0;
	virtual int WINAPI music_load(char *filename) = 0;
	virtual int WINAPI music_load2(uchar *musdata, int size) = 0;
	virtual char* WINAPI getmusicfilename(char *dest) = 0;
	virtual void WINAPI music_start(void) = 0;
	virtual void WINAPI music_stop(void) = 0;
	virtual int WINAPI getloopcount(void) = 0;
	virtual bool WINAPI getlength(char *filename, int *length, int *loop) = 0;
	virtual int WINAPI getpos(void) = 0;
	virtual void WINAPI setpos(int pos) = 0;
	virtual void WINAPI getpcmdata(short *buf, int nsamples) = 0;
};


//=============================================================================
// IFMPMD : WinFMP, PMDWin に共通なインターフェイスを定義したクラス
//=============================================================================
interface IFMPMD : public IPCMMUSICDRIVER {
	virtual bool WINAPI loadrhythmsample(char *path) = 0;
	virtual bool WINAPI setpcmdir(char **path) = 0;
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
	virtual bool WINAPI getlength2(char *filename, int *length, int *loop) = 0;
	virtual int WINAPI getpos2(void) = 0;
	virtual void WINAPI setpos2(int pos) = 0;
	virtual char* WINAPI getpcmfilename(char *dest) = 0;
	virtual char* WINAPI getppzfilename(char *dest, int bufnum) = 0;
};


//=============================================================================
// Interface ID(IID)
//=============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// GUID of IPCmMUSICDRIVER Interface ID
interface	__declspec(uuid("9D4D6317-F40A-455E-9E2C-CB517556BA02")) IPCMMUSICDRIVER;	

// GUID of IFMPMD Interface ID
interface	__declspec(uuid("81977D60-9496-4F20-A3BB-19B19943DA6D")) IFMPMD;


const IID IID_IPCMMUSICDRIVER	= _uuidof(IPCMMUSICDRIVER);	// IPCMMUSICDRIVER Interface ID
const IID IID_IFMPMD			= _uuidof(IFMPMD);			// IFMPMD Interface ID

#ifdef __cplusplus
}
#endif


#endif	// IPCMMUSICDRIVER_H
