//=============================================================================
//		PCM Music Driver Interface Class : IPCMMUSICDRIVER
//=============================================================================

#ifndef IPMDWIN_H
#define IPMDWIN_H

#include "pcmmusdriver.h"


//=============================================================================
//	バージョン情報
//=============================================================================
#define	InterfaceVersion	118		// 上１桁：major, 下２桁：minor version


//=============================================================================
//	DLL の戻り値
//=============================================================================
#define	PMDWIN_OK				 	  0		// 正常終了
#define	ERR_OPEN_MUSIC_FILE			  1		// 曲 データを開けなかった
#define	ERR_WRONG_MUSIC_FILE		  2		// PMD の曲データではなかった
#define	ERR_OPEN_PPC_FILE		 	  3		// PPC を開けなかった
#define	ERR_OPEN_P86_FILE		 	  4		// P86 を開けなかった
#define	ERR_OPEN_PPS_FILE		 	  5		// PPS を開けなかった
#define	ERR_OPEN_PPZ1_FILE		 	  6		// PPZ1 を開けなかった
#define	ERR_OPEN_PPZ2_FILE		 	  7		// PPZ2 を開けなかった
#define	ERR_WRONG_PPC_FILE		 	  8		// PPC/PVI ではなかった
#define	ERR_WRONG_P86_FILE		 	  9		// P86 ではなかった
#define	ERR_WRONG_PPS_FILE		 	 10		// PPS ではなかった
#define	ERR_WRONG_PPZ1_FILE		 	 11		// PVI/PZI ではなかった(PPZ1)
#define	ERR_WRONG_PPZ2_FILE		 	 12		// PVI/PZI ではなかった(PPZ2)
#define	WARNING_PPC_ALREADY_LOAD	 13		// PPC はすでに読み込まれている
#define	WARNING_P86_ALREADY_LOAD	 14		// P86 はすでに読み込まれている
#define	WARNING_PPS_ALREADY_LOAD	 15		// PPS はすでに読み込まれている
#define	WARNING_PPZ1_ALREADY_LOAD	 16		// PPZ1 はすでに読み込まれている
#define	WARNING_PPZ2_ALREADY_LOAD	 17		// PPZ2 はすでに読み込まれている

#define	ERR_WRONG_PARTNO			 30		// パート番号が不適
//#define	ERR_ALREADY_MASKED		 31		// 指定パートはすでにマスクされている
#define	ERR_NOT_MASKED				 32		// 指定パートはマスクされていない
#define	ERR_MUSIC_STOPPED			 33		// 曲が止まっているのにマスク操作をした
#define	ERR_EFFECT_USED				 34		// 効果音で使用中なのでマスクを操作できない

#define	ERR_OUT_OF_MEMORY			 99		// メモリを確保できなかった
#define	ERR_OTHER					999		// その他のエラー


//----------------------------------------------------------------------------
//	ＰＭＤＷｉｎ専用の定義
//----------------------------------------------------------------------------
#define	SOUND_55K			  55555
#define	SOUND_55K_2			  55466
#define	SOUND_48K			  48000
#define	SOUND_44K			  44100
#define	SOUND_22K			  22050
#define	SOUND_11K			  11025

#define	PPZ8_i0				  44100
#define	PPZ8_i1				  33080
#define	PPZ8_i2				  22050
#define	PPZ8_i3				  16540
#define	PPZ8_i4				  11025
#define	PPZ8_i5				   8270
#define	PPZ8_i6				   5513
#define	PPZ8_i7				   4135

#define	DEFAULT_REG_WAIT		  30000
#define	MAX_PCMDIR			     64
#define	MAX_MEMOBUF			   1024


//----------------------------------------------------------------------------
//	その他定義
//----------------------------------------------------------------------------
#define	nbufsample			  30000
#define	OPNAClock		(3993600*2)

#define	NumOfFMPart			      6
#define	NumOfSSGPart		      3
#define	NumOfADPCMPart		      1
#define	NumOfOPNARhythmPart	      1
#define	NumOfExtPart		      3
#define	NumOfRhythmPart		      1
#define	NumOfEffPart		      1
#define	NumOfPPZ8Part		      8
#define	NumOfAllPart		(NumOfFMPart+NumOfSSGPart+NumOfADPCMPart+NumOfOPNARhythmPart+NumOfExtPart+NumOfRhythmPart+NumOfEffPart+NumOfPPZ8Part)


/******************************************************************************
;	WORK AREA
******************************************************************************/
typedef struct PMDworktag {
	int32_t			partb;				//	処理中パート番号
	int32_t			tieflag;			//	&のフラグ(1 : tie)
	int32_t			volpush_flag;		//	次の１音音量down用のflag(1 : voldown)
	int32_t			rhydmy;				//	R part ダミー演奏データ
	int32_t			fmsel;				//	FM 表(=0)か裏(=0x100)か flag
	int32_t			omote_key[3];		//	FM keyondata表(=0)
	int32_t			ura_key[3];			//	FM keyondata裏(=0x100)
	int32_t			loop_work;			//	Loop Work
	bool		ppsdrv_flag;		//	ppsdrv を使用するか？flag(ユーザーが代入)
	int32_t			pcmrepeat1;			//	PCMのリピートアドレス1
	int32_t			pcmrepeat2;			//	PCMのリピートアドレス2
	int32_t			pcmrelease;			//	PCMのRelease開始アドレス
	int32_t			lastTimerAtime;		//	一個前の割り込み時のTimerATime値
	int32_t			music_flag;			//	B0:次でMSTART 1:次でMSTOP のFlag
	int32_t			slotdetune_flag;	//	FM3 Slot Detuneを使っているか
	int32_t			slot3_flag;			//	FM3 Slot毎 要効果音モードフラグ
	int32_t			fm3_alg_fb;			//	FM3chの最後に定義した音色のalg/fb
	int32_t			af_check;			//	FM3chのalg/fbを設定するかしないかflag
	int32_t			lfo_switch;			//	局所LFOスイッチ
} PMDWORK;


typedef struct effworktag {
	int32_t*		effadr;			//	effect address
	int32_t			eswthz;				//	トーンスゥイープ周波数
	int32_t			eswtst;				//	トーンスゥイープ増分
	int32_t			effcnt;				//	effect count
	int32_t			eswnhz;				//	ノイズスゥイープ周波数
	int32_t			eswnst;				//	ノイズスゥイープ増分
	int32_t			eswnct;				//	ノイズスゥイープカウント
	int32_t			effon;				//	効果音　発音中
	int32_t			psgefcnum;			//	効果音番号
	int32_t			hosei_flag;			//	ppsdrv 音量/音程補正をするかどうか
	int32_t			last_shot_data;		//	最後に発音させたPPSDRV音色
} EFFWORK;


//	演奏中のデータエリア
typedef struct qqtag {
	uint8_t*	address;			//	2 ｴﾝｿｳﾁｭｳ ﾉ ｱﾄﾞﾚｽ
	uint8_t*	partloop;			//	2 ｴﾝｿｳ ｶﾞ ｵﾜｯﾀﾄｷ ﾉ ﾓﾄﾞﾘｻｷ
	int32_t			leng;				//	1 ﾉｺﾘ LENGTH
	int32_t			qdat;				//	1 gatetime (q/Q値を計算した値)
	uint32_t		fnum;				//	2 ｴﾝｿｳﾁｭｳ ﾉ BLOCK/FNUM
	int32_t			detune;				//	2 ﾃﾞﾁｭｰﾝ
	int32_t			lfodat;				//	2 LFO DATA
	int32_t			porta_num;			//	2 ポルタメントの加減値（全体）
	int32_t			porta_num2;			//	2 ポルタメントの加減値（一回）
	int32_t			porta_num3;			//	2 ポルタメントの加減値（余り）
	int32_t			volume;				//	1 VOLUME
	int32_t			shift;				//	1 ｵﾝｶｲ ｼﾌﾄ ﾉ ｱﾀｲ
	int32_t			delay;				//	1 LFO	[DELAY]
	int32_t			speed;				//	1	[SPEED]
	int32_t			step;				//	1	[STEP]
	int32_t			time;				//	1	[TIME]
	int32_t			delay2;				//	1	[DELAY_2]
	int32_t			speed2;				//	1	[SPEED_2]
	int32_t			step2;				//	1	[STEP_2]
	int32_t			time2;				//	1	[TIME_2]
	int32_t			lfoswi;				//	1 LFOSW. B0/tone B1/vol B2/同期 B3/porta
									//	         B4/tone B5/vol B6/同期
	int32_t			volpush;			//	1 Volume PUSHarea
	int32_t			mdepth;				//	1 M depth
	int32_t			mdspd;				//	1 M speed
	int32_t			mdspd2;				//	1 M speed_2
	int32_t			envf;				//	1 PSG ENV. [START_FLAG] / -1でextend
	int32_t			eenv_count;			//	1 ExtendPSGenv/No=0 AR=1 DR=2 SR=3 RR=4
	int32_t			eenv_ar;			//	1 	/AR		/旧pat
	int32_t			eenv_dr;			//	1	/DR		/旧pv2
	int32_t			eenv_sr;			//	1	/SR		/旧pr1
	int32_t			eenv_rr;			//	1	/RR		/旧pr2
	int32_t			eenv_sl;			//	1	/SL
	int32_t			eenv_al;			//	1	/AL
	int32_t			eenv_arc;			//	1	/ARのカウンタ	/旧patb
	int32_t			eenv_drc;			//	1	/DRのカウンタ
	int32_t			eenv_src;			//	1	/SRのカウンタ	/旧pr1b
	int32_t			eenv_rrc;			//	1	/RRのカウンタ	/旧pr2b
	int32_t			eenv_volume;		//	1	/Volume値(0?15)/旧penv
	int32_t			extendmode;			//	1 B1/Detune B2/LFO B3/Env Normal/Extend
	int32_t			fmpan;				//	1 FM Panning + AMD + PMD
	int32_t			psgpat;				//	1 PSG PATTERN [TONE/NOISE/MIX]
	int32_t			voicenum;			//	1 音色番号
	int32_t			loopcheck;			//	1 ループしたら１ 終了したら３
	int32_t			carrier;			//	1 FM Carrier
	int32_t			slot1;				//	1 SLOT 1 ﾉ TL
	int32_t			slot3;				//	1 SLOT 3 ﾉ TL
	int32_t			slot2;				//	1 SLOT 2 ﾉ TL
	int32_t			slot4;				//	1 SLOT 4 ﾉ TL
	int32_t			slotmask;			//	1 FM slotmask
	int32_t			neiromask;			//	1 FM 音色定義用maskdata
	int32_t			lfo_wave;			//	1 LFOの波形
	int32_t			partmask;			//	1 PartMask b0:通常 b1:効果音 b2:NECPCM用
									//	   b3:none b4:PPZ/ADE用 b5:s0時 b6:m b7:一時
	int32_t			keyoff_flag;		//	1 KeyoffしたかどうかのFlag
	int32_t			volmask;			//	1 音量LFOのマスク
	int32_t			qdata;				//	1 qの値
	int32_t			qdatb;				//	1 Qの値
	int32_t			hldelay;			//	1 HardLFO delay
	int32_t			hldelay_c;			//	1 HardLFO delay Counter
	int32_t			_lfodat;			//	2 LFO DATA
	int32_t			_delay;				//	1 LFO	[DELAY]
	int32_t			_speed;				//	1		[SPEED]
	int32_t			_step;				//	1		[STEP]
	int32_t			_time;				//	1		[TIME]
	int32_t			_delay2;			//	1		[DELAY_2]
	int32_t			_speed2;			//	1		[SPEED_2]
	int32_t			_step2;				//	1		[STEP_2]
	int32_t			_time2;				//	1		[TIME_2]
	int32_t			_mdepth;			//	1 M depth
	int32_t			_mdspd;				//	1 M speed
	int32_t			_mdspd2;			//	1 M speed_2
	int32_t			_lfo_wave;			//	1 LFOの波形
	int32_t			_volmask;			//	1 音量LFOのマスク
	int32_t			mdc;				//	1 M depth Counter (変動値)
	int32_t			mdc2;				//	1 M depth Counter
	int32_t			_mdc;				//	1 M depth Counter (変動値)
	int32_t			_mdc2;				//	1 M depth Counter
	int32_t			onkai;				//	1 演奏中の音階データ (0ffh:rest)
	int32_t			sdelay;				//	1 Slot delay
	int32_t			sdelay_c;			//	1 Slot delay counter
	int32_t			sdelay_m;			//	1 Slot delay Mask
	int32_t			alg_fb;				//	1 音色のalg/fb
	int32_t			keyon_flag;			//	1 新音階/休符データを処理したらinc
	int32_t			qdat2;				//	1 q 最低保証値
	int32_t			onkai_def;			//	1 演奏中の音階データ (転調処理前 / ?fh:rest)
	int32_t			shift_def;			//	1 マスター転調値
	int32_t			qdat3;				//	1 q Random
} QQ;


typedef struct OpenWorktag {
	QQ*			MusPart[NumOfAllPart];	// パートワークのポインタ
	uint8_t*	mmlbuf;				//	Musicdataのaddress+1
	uint8_t*	tondat;				//	Voicedataのaddress
	uint8_t*	efcdat;				//	FM Effecdataのaddress
	uint8_t*	prgdat_adr;			//	曲データ中音色データ先頭番地
	uint16_t*	radtbl;				//	R part offset table 先頭番地
	uint8_t*	rhyadr;				//	R part 演奏中番地
	int32_t			rhythmmask;			//	Rhythm音源のマスク x8c/10hのbitに対応
	int32_t			fm_voldown;			//	FM voldown 数値
	int32_t			ssg_voldown;		//	PSG voldown 数値
	int32_t			pcm_voldown;		//	ADPCM voldown 数値
	int32_t			rhythm_voldown;		//	RHYTHM voldown 数値
	int32_t			prg_flg;			//	曲データに音色が含まれているかflag
	int32_t			x68_flg;			//	OPM flag
	int32_t			status;				//	status1
	int32_t			status2;			//	status2
	int32_t			tempo_d;			//	tempo (TIMER-B)
	int32_t			fadeout_speed;		//	Fadeout速度
	int32_t			fadeout_volume;		//	Fadeout音量
	int32_t			tempo_d_push;		//	tempo (TIMER-B) / 保存用
	int32_t			syousetu_lng;		//	小節の長さ
	int32_t			opncount;			//	最短音符カウンタ
	int32_t			TimerAtime;			//	TimerAカウンタ
	int32_t			effflag;			//	PSG効果音発声on/off flag(ユーザーが代入)
	int32_t			psnoi;				//	PSG noise周波数
	int32_t			psnoi_last;			//	PSG noise周波数(最後に定義した数値)
	int32_t			pcmstart;			//	PCM音色のstart値
	int32_t			pcmstop;			//	PCM音色のstop値
	int32_t			rshot_dat;			//	リズム音源 shot flag
	int32_t			rdat[6];			//	リズム音源 音量/パンデータ
	int32_t			rhyvol;				//	リズムトータルレベル
	int32_t			kshot_dat;			//	ＳＳＧリズム shot flag
	int32_t			play_flag;			//	play flag
	int32_t			fade_stop_flag;		//	Fadeout後 MSTOPするかどうかのフラグ
	bool		kp_rhythm_flag;		//	K/RpartでRhythm音源を鳴らすかflag
	int32_t			pcm_gs_flag;		//	ADPCM使用 許可フラグ (0で許可)
	int32_t			slot_detune1;		//	FM3 Slot Detune値 slot1
	int32_t			slot_detune2;		//	FM3 Slot Detune値 slot2
	int32_t			slot_detune3;		//	FM3 Slot Detune値 slot3
	int32_t			slot_detune4;		//	FM3 Slot Detune値 slot4
	int32_t			TimerB_speed;		//	TimerBの現在値(=ff_tempoならff中)
	int32_t			fadeout_flag;		//	内部からfoutを呼び出した時1
	int32_t			revpan;				//	PCM86逆相flag
	int32_t			pcm86_vol;			//	PCM86の音量をSPBに合わせるか?
	int32_t			syousetu;			//	小節カウンタ
	int32_t			port22h;			//	OPN-PORT 22H に最後に出力した値(hlfo)
	int32_t			tempo_48;			//	現在のテンポ(clock=48 tの値)
	int32_t			tempo_48_push;		//	現在のテンポ(同上/保存用)
	int32_t			_fm_voldown;		//	FM voldown 数値 (保存用)
	int32_t			_ssg_voldown;		//	PSG voldown 数値 (保存用)
	int32_t			_pcm_voldown;		//	PCM voldown 数値 (保存用)
	int32_t			_rhythm_voldown;	//	RHYTHM voldown 数値 (保存用)
	int32_t			_pcm86_vol;			//	PCM86の音量をSPBに合わせるか? (保存用)
	int32_t			rshot_bd;			//	リズム音源 shot inc flag (BD)
	int32_t			rshot_sd;			//	リズム音源 shot inc flag (SD)
	int32_t			rshot_sym;			//	リズム音源 shot inc flag (CYM)
	int32_t			rshot_hh;			//	リズム音源 shot inc flag (HH)
	int32_t			rshot_tom;			//	リズム音源 shot inc flag (TOM)
	int32_t			rshot_rim;			//	リズム音源 shot inc flag (RIM)
	int32_t			rdump_bd;			//	リズム音源 dump inc flag (BD)
	int32_t			rdump_sd;			//	リズム音源 dump inc flag (SD)
	int32_t			rdump_sym;			//	リズム音源 dump inc flag (CYM)
	int32_t			rdump_hh;			//	リズム音源 dump inc flag (HH)
	int32_t			rdump_tom;			//	リズム音源 dump inc flag (TOM)
	int32_t			rdump_rim;			//	リズム音源 dump inc flag (RIM)
	int32_t			ch3mode;			//	ch3 Mode
	int32_t			ppz_voldown;		//	PPZ8 voldown 数値
	int32_t			_ppz_voldown;		//	PPZ8 voldown 数値 (保存用)
	int32_t			TimerAflag;			//	TimerA割り込み中？フラグ（＠不要？）
	int32_t			TimerBflag;			//	TimerB割り込み中？フラグ（＠不要？）
	
	// for PMDWin
	int32_t			rate;								//	PCM 出力周波数(11k, 22k, 44k, 55k)
	int32_t			ppzrate;							//	PPZ 出力周波数
	bool		fmcalc55k;							//	FM で 55kHz 合成をするか？
	bool		ppz8ip;								//	PPZ8 で補完するか
	bool		ppsip;								//	PPS  で補完するか
	bool		p86ip;								//	P86  で補完するか
	bool		use_p86;							//	P86  を使用しているか
	int32_t			fadeout2_speed;						//	fadeout(高音質)speed(>0で fadeout)
	TCHAR		mus_filename[_MAX_PATH];			//	曲のFILE名バッファ
	TCHAR		ppcfilename[_MAX_PATH];				//	PPC のFILE名バッファ
	TCHAR		pcmdir[MAX_PCMDIR+1][_MAX_PATH];	//	PCM 検索ディレクトリ
} OPEN_WORK;


//=============================================================================
//	COM 風 interface class
//=============================================================================
struct IPMDWIN : public IFMPMD {
	virtual void WINAPI setppsuse(bool value) = 0;
	virtual void WINAPI setrhythmwithssgeffect(bool value) = 0;
	virtual void WINAPI setpmd86pcmmode(bool value) = 0;
	virtual bool WINAPI getpmd86pcmmode(void) = 0;
	virtual void WINAPI setppsinterpolation(bool ip) = 0;
	virtual void WINAPI setp86interpolation(bool ip) = 0;
	virtual int32_t WINAPI maskon(int32_t ch) = 0;
	virtual int32_t WINAPI maskoff(int32_t ch) = 0;
	virtual void WINAPI setfmvoldown(int32_t voldown) = 0;
	virtual void WINAPI setssgvoldown(int32_t voldown) = 0;
	virtual void WINAPI setrhythmvoldown(int32_t voldown) = 0;
	virtual void WINAPI setadpcmvoldown(int32_t voldown) = 0;
	virtual void WINAPI setppzvoldown(int32_t voldown) = 0;
	virtual int32_t WINAPI getfmvoldown(void) = 0;
	virtual int32_t WINAPI getfmvoldown2(void) = 0;
	virtual int32_t WINAPI getssgvoldown(void) = 0;
	virtual int32_t WINAPI getssgvoldown2(void) = 0;
	virtual int32_t WINAPI getrhythmvoldown(void) = 0;
	virtual int32_t WINAPI getrhythmvoldown2(void) = 0;
	virtual int32_t WINAPI getadpcmvoldown(void) = 0;
	virtual int32_t WINAPI getadpcmvoldown2(void) = 0;
	virtual int32_t WINAPI getppzvoldown(void) = 0;
	virtual int32_t WINAPI getppzvoldown2(void) = 0;
	virtual char* WINAPI getmemo(char *dest, uint8_t *musdata, int32_t size, int32_t al) = 0;
	virtual char* WINAPI getmemo2(char *dest, uint8_t *musdata, int32_t size, int32_t al) = 0;
	virtual char* WINAPI getmemo3(char *dest, uint8_t *musdata, int32_t size, int32_t al) = 0;
	virtual int32_t	WINAPI fgetmemo(char *dest, TCHAR *filename, int32_t al) = 0;
	virtual int32_t	WINAPI fgetmemo2(char *dest, TCHAR *filename, int32_t al) = 0;
	virtual int32_t	WINAPI fgetmemo3(char *dest, TCHAR *filename, int32_t al) = 0;
	virtual TCHAR* WINAPI getppcfilename(TCHAR *dest) = 0;
	virtual TCHAR* WINAPI getppsfilename(TCHAR *dest) = 0;
	virtual TCHAR* WINAPI getp86filename(TCHAR *dest) = 0;
	virtual int32_t WINAPI ppc_load(TCHAR *filename) = 0;
	virtual int32_t WINAPI pps_load(TCHAR *filename) = 0;
	virtual int32_t WINAPI p86_load(TCHAR *filename) = 0;
	virtual int32_t WINAPI ppz_load(TCHAR *filename, int32_t bufnum) = 0;
	virtual OPEN_WORK* WINAPI getopenwork(void) = 0;
	virtual QQ* WINAPI getpartwork(int32_t ch) = 0;
};


#endif	// IPMDWIN_H
