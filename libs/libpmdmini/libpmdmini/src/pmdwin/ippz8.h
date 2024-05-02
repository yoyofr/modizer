//=============================================================================
//		8 Channel PCM Driver 「PPZ8」 Unit(Light Version)
//			Programmed by UKKY
//			Windows Converted by C60
//=============================================================================

#ifndef IPPZ8_H
#define IPPZ8_H

#include "portability_fmpmd.h"


/// <summary>
/// PPZ8 インターフェース
/// </summary>
class IPPZ8
{
public:
	virtual bool __cdecl Init(uint32_t rate, bool ip) = 0;			// 00H 初期化
	virtual bool __cdecl Play(int32_t ch, int32_t bufnum, int32_t num, uint16_t start, uint16_t stop) = 0;
																	// 01H PCM 発音
	virtual bool __cdecl Stop(int32_t ch) = 0;							// 02H PCM 停止
	virtual int32_t __cdecl Load(TCHAR *filename, int32_t bufnum) = 0;		// 03H PVI/PZIﾌｧｲﾙの読み込み
	virtual bool __cdecl SetVol(int32_t ch, int32_t vol) = 0;				// 07H ﾎﾞﾘｭｰﾑ設定
	virtual bool __cdecl SetOntei(int32_t ch, uint32_t ontei) = 0;		// 0BH 音程周波数の設定
	virtual bool __cdecl SetLoop(int32_t ch, uint32_t loop_start, uint32_t loop_end) = 0;
																	// 0EH ﾙｰﾌﾟﾎﾟｲﾝﾀの設定
	virtual void __cdecl AllStop(void) = 0;							// 12H (PPZ8)全停止
	virtual bool __cdecl SetPan(int32_t ch, int32_t pan) = 0;				// 13H (PPZ8)PAN指定
	virtual bool __cdecl SetRate(uint32_t rate, bool ip) = 0;		// 14H (PPZ8)ﾚｰﾄ設定
	virtual bool __cdecl SetSourceRate(int32_t ch, int32_t rate) = 0;		// 15H (PPZ8)元ﾃﾞｰﾀ周波数設定
	virtual void __cdecl SetAllVolume(int32_t vol) = 0;					// 16H (PPZ8)全体ﾎﾞﾘﾕｰﾑの設定（86B Mixer)
	virtual void __cdecl SetVolume(int32_t vol) = 0;
	//PCMTMP_SET		;17H PCMﾃﾝﾎﾟﾗﾘ設定
	virtual void __cdecl ADPCM_EM_SET(bool flag) = 0;				// 18H (PPZ8)ADPCMエミュレート
	//REMOVE_FSET		;19H (PPZ8)常駐解除ﾌﾗｸﾞ設定
	//FIFOBUFF_SET		;1AH (PPZ8)FIFOﾊﾞｯﾌｧの変更
	//RATE_SET		;1BH (PPZ8)WSS詳細ﾚｰﾄ設定
};



#endif	// IPPZ8
