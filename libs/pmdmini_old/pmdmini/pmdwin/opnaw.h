//=============================================================================
//		opnaw : OPNA unit with wait
//			ver 0.03
//=============================================================================

#ifndef OPNAW_H
#define OPNAW_H

#include	"opna.h"


// 一次補間有効時の合成周波数
#define	SOUND_55K			  55555
#define	SOUND_55K_2			  55466

// wait で計算した分を代入する buffer size(samples)
#define		WAIT_PCM_BUFFER_SIZE  4096 //65536

// 線形補間時に計算した分を代入する buffer size(samples)
#define		IP_PCM_BUFFER_SIZE  2048

namespace FM {
	
	class OPNAW : public OPNA
	{
	public:
		OPNAW();
		~OPNAW();
		
		bool	Init(uint c, uint r, bool ipflag, const TCHAR* path);
		bool	SetRate(uint c, uint r, bool ipflag = false);
		
		void	SetFMWait(int nsec);				// FM wait(nsec)
		void	SetSSGWait(int nsec);				// SSG wait(nsec)
		void	SetRhythmWait(int nsec);			// Rhythm wait(nsec)
		void	SetADPCMWait(int nsec);				// ADPCM wait(nsec)
		
		int		GetFMWait(void);					// FM wait(nsec)
		int		GetSSGWait(void);					// SSG wait(nsec)
		int		GetRhythmWait(void);				// Rhythm wait(nsec)
		int		GetADPCMWait(void);					// ADPCM wait(nsec)
		
		void	SetReg(uint addr, uint data);		// レジスタ設定
		void	Mix(Sample* buffer, int nsamples);	// 合成
		void	ClearBuffer(void);					// 内部バッファクリア
		
	private:
		Sample	pre_buffer[WAIT_PCM_BUFFER_SIZE * 2];
													// wait 時の合成音の退避
		int		fmwait;								// FM Wait(nsec)
		int		ssgwait;							// SSG Wait(nsec)
		int		rhythmwait;							// Rhythm Wait(nsec)
		int		adpcmwait;							// ADPCM Wait(nsec)
		
		int		fmwaitcount;						// FM Wait(count*1000)
		int		ssgwaitcount;						// SSG Wait(count*1000)
		int		rhythmwaitcount;					// Rhythm Wait(count*1000)
		int		adpcmwaitcount;						// ADPCM Wait(count*1000)
		
		int		read_pos;							// 書き込み位置
		int		write_pos;							// 読み出し位置
		int		count2;								// count 小数部分(*1000)
		
		Sample	ip_buffer[IP_PCM_BUFFER_SIZE * 2];	// 線形補間時のワーク
		uint	rate2;								// 出力周波数
		bool	interpolation2;						// 一次補間フラグ
		int		delta;								// 差分小数部(16384サンプルで分割)
		
		void	_Init(void);						// 初期化(内部処理)
		void	CalcWaitPCM(int waitcount);			// SetReg() wait 時の PCM を計算
		void	_Mix(Sample* buffer, int nsamples);	// 合成（一次補間なしVer.)
	};
}

#endif	// OPNAW_H
