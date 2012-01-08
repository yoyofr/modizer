//=============================================================================
//		SSG PCM Driver 「PPSDRV」 Unit
//			Original Programmed	by NaoNeko.
//			Modified		by Kaja.
//			Windows Converted by C60
//=============================================================================

#ifndef PPSDRV_H
#define PPSDRV_H

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#include "types.h"
//#include "psg.h"
#include "compat.h"

//	DLL の 戻り値
#define	_PPSDRV_OK					  0		// 正常終了
#define	_ERR_OPEN_PPS_FILE			  1		// PPS を開けなかった
#define	_ERR_WRONG_PPS_FILE		 	  2		// PPS の形式が異なっている
#define	_WARNING_PPS_ALREADY_LOAD	  3		// PPS はすでに読み込まれている
#define	_ERR_OUT_OF_MEMORY			 99		// メモリを確保できなかった

#define	SOUND_44K		 44100
#define	SOUND_22K		 22050
#define	SOUND_11K		 11025

#define	PPSDRV_VER		"0.37"
#define	MAX_PPS				14
//#define vers		0x03
//#define date		"1994/06/08"


typedef int				Sample;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;


#ifdef WINDOWS
#pragma pack( push, enter_include1 )
#pragma pack(1)
#endif

typedef struct ppsheadertag
{
	struct {
		WORD	address;				// 先頭アドレス
		WORD	leng;					// データ量
		BYTE	toneofs;				// 音階
		BYTE	volumeofs;				// 音量
	} __PACKED__ pcmnum[MAX_PPS];
} __PACKED__ PPSHEADER;


#ifdef WINDOWS
#pragma pack( pop, enter_include1 )
#endif


class PPSDRV
{
public:
	PPSDRV();
	~PPSDRV();
	
	bool Init(uint r, bool ip);						//     初期化
	bool Stop(void);								// 00H PDR 停止
	bool Play(int num, int shift, int volshift);	// 01H PDR 再生
	bool Check(void);								// 02H 再生中かどうかcheck
	bool SetParam(int paramno, bool data);			// 05H PDRパラメータの設定
	bool GetParam(int paramno);						// 06H PDRパラメータの取得
	
	int Load(char *filename);						// PPS 読み込み
	bool SetRate(uint r, bool ip);					// レート設定
	void SetVolume(int vol);						// 音量設定
	void Mix(Sample* dest, int nsamples);			// 合成

	PPSHEADER ppsheader;							// PCMの音色ヘッダー
	char	pps_file[_MAX_PATH];					// ファイル名

private:
	bool	interpolation;							// 補完するか？
	int		rate;
	Sample	*dataarea1;								// PPS 保存用メモリポインタ
	bool	single_flag;							// 単音モードか？
	bool	low_cpu_check_flag;						// 周波数半分で再生か？
	bool	keyon_flag;								// Keyon 中か？
	Sample	EmitTable[16];
	Sample	*data_offset1;
	Sample	*data_offset2;
	int		data_xor1;								// 現在の位置(小数部)
	int		data_xor2;								// 現在の位置(小数部)
	int		tick1;
	int		tick2;
	int		tick_xor1;
	int		tick_xor2;
	int		data_size1;
	int		data_size2;
	int		volume1;
	int		volume2;
	Sample	keyoff_vol;
//	PSG		psg;									// @暫定

};

#endif	// PPSDRV_H
