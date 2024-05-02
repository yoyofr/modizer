//=============================================================================
//		opnaw : OPNA unit with wait
//			ver 0.05
//=============================================================================

#ifndef OPNAW_H
#define OPNAW_H

#include "portability_fmpmdcore.h"
#include "opna.h"
#include "ifileio.h"


// 一次補間有効時の合成周波数
#define	SOUND_55K				55555
#define	SOUND_55K_2				55466

// wait で計算した分を代入する buffer size(samples)
#define	WAIT_PCM_BUFFER_SIZE	65536

// 線形補間時に計算した分を代入する buffer size(samples)
#define		IP_PCM_BUFFER_SIZE	 2048

// sinc 補間のサンプル数
// #define		NUMOFINTERPOLATION	   32
// #define		NUMOFINTERPOLATION	   64
#define		NUMOFINTERPOLATION	  128
// #define		NUMOFINTERPOLATION	  256
// #define		NUMOFINTERPOLATION	  512


namespace FM {
	
	class OPNAW : public OPNA
	{
	public:
		OPNAW(IFILEIO* pfileio);
		virtual ~OPNAW();
		void	WINAPI setfileio(IFILEIO* pfileio);
		
		bool	Init(uint32_t c, uint32_t r, bool ipflag, const TCHAR* path);
		bool	SetRate(uint32_t c, uint32_t r, bool ipflag = false);
		
		void	SetFMWait(int32_t nsec);				// FM wait(nsec)
		void	SetSSGWait(int32_t nsec);				// SSG wait(nsec)
		void	SetRhythmWait(int32_t nsec);			// Rhythm wait(nsec)
		void	SetADPCMWait(int32_t nsec);				// ADPCM wait(nsec)
		
		int32_t		GetFMWait(void);					// FM wait(nsec)
		int32_t		GetSSGWait(void);					// SSG wait(nsec)
		int32_t		GetRhythmWait(void);				// Rhythm wait(nsec)
		int32_t		GetADPCMWait(void);					// ADPCM wait(nsec)
		
		void	SetReg(uint32_t addr, uint32_t data);		// レジスタ設定
		void	Mix(Sample* buffer, int32_t nsamples);	// 合成
		void	ClearBuffer(void);					// 内部バッファクリア
		
	private:
		Sample	pre_buffer[WAIT_PCM_BUFFER_SIZE * 2];
													// wait 時の合成音の退避
		int32_t		fmwait;								// FM Wait(nsec)
		int32_t		ssgwait;							// SSG Wait(nsec)
		int32_t		rhythmwait;							// Rhythm Wait(nsec)
		int32_t		adpcmwait;							// ADPCM Wait(nsec)
		
		int32_t		fmwaitcount;						// FM Wait(count*1000)
		int32_t		ssgwaitcount;						// SSG Wait(count*1000)
		int32_t		rhythmwaitcount;					// Rhythm Wait(count*1000)
		int32_t		adpcmwaitcount;						// ADPCM Wait(count*1000)
		
		int32_t		read_pos;							// 書き込み位置
		int32_t		write_pos;							// 読み出し位置
		int32_t		count2;								// count 小数部分(*1000)
		
		Sample	ip_buffer[IP_PCM_BUFFER_SIZE * 2];	// 線形補間時のワーク
		uint32_t	rate2;								// 出力周波数
		bool	interpolation2;						// 一次補間フラグ
		int32_t		delta;								// 差分小数部(16384サンプルで分割)
		
		double	delta_double;						// 差分小数部
		
		bool	ffirst;								// データ初回かどうかのフラグ
		double	rest;								// 標本化定理リサンプリング時の前回の残りサンプルデータ位置
		int32_t		write_pos_ip;						// 書き込み位置(ip)
		
		void	_Init(void);						// 初期化(内部処理)
		void	CalcWaitPCM(int32_t waitcount);			// SetReg() wait 時の PCM を計算
		double	Sinc(double x);						// Sinc関数
		void	_Mix(Sample* buffer, int32_t nsamples);	// 合成（一次補間なしVer.)
		double	Fmod2(double x, double y);			// 最小非負剰余
		
		inline int32_t Limit(int32_t v, int32_t max, int32_t min)
		{
			return v > max ? max : (v < min ? min : v);
		}
	};
}

#endif	// OPNAW_H
