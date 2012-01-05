//=============================================================================
//	Professional Music Driver [P.M.D.] version 4.8
//			Programmed By M.Kajihara
//			Windows Converted by C60
//=============================================================================

#ifndef PMDWIN_H
#define PMDWIN_H

#include <stdlib.h>
#include "opnaw.h"
#include "ppz8l.h"
#include "ppsdrv.h"
#include "p86drv.h"
#ifdef ENABLE_COM_INTERFACE
#include "PCMMusDriver.h"
#endif

//#include "types.h"
#include "compat.h"


//=============================================================================
//	バージョン情報
//=============================================================================
#define	DLLVersion			 19		// 上１桁：major, 下２桁：minor version
#define	InterfaceVersion	117		// 上１桁：major, 下２桁：minor version


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
//#define	ERR_ALREADY_MASKED			 31		// 指定パートはすでにマスクされている
#define	ERR_NOT_MASKED				 32		// 指定パートはマスクされていない
#define	ERR_MUSIC_STOPPED			 33		// 曲が止まっているのにマスク操作をした
#define	ERR_EFFECT_USED				 34		// 効果音で使用中なのでマスクを操作できない

#define	ERR_OUT_OF_MEMORY			 99		// メモリを確保できなかった
#define	ERR_OTHER					999		// その他のエラー


//----------------------------------------------------------------------------
//	ＰＭＤＷｉｎ専用の定義
//----------------------------------------------------------------------------
#define	SOUND_55K			  55555
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

#define	DEFAULT_REG_WAIT	  15000
#define	MAX_PCMDIR				 64
#define	MAX_MEMOBUF			   1024


//----------------------------------------------------------------------------
//	その他定義
//----------------------------------------------------------------------------
#define	nbufsample			  30000
#define	OPNAClock		(3993600*2)

#define	NumOfFMPart			      6
#define	NumOfSSGPart		      3
#define	NumOfADPCMPart		      1
#define	NumOFOPNARhythmPart	      1
#define	NumOfExtPart		      3
#define	NumOfRhythmPart		      1
#define	NumOfEffPart		      1
#define	NumOfPPZ8Part		      8
#define	NumOfAllPart		(NumOfFMPart+NumOfSSGPart+NumOfADPCMPart+NumOFOPNARhythmPart+NumOfExtPart+NumOfRhythmPart+NumOfEffPart+NumOfPPZ8Part)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef int Sample;

#ifndef __cplusplus
typedef unsigned char bool;
#endif


typedef struct stereosampletag
{
	Sample left;
	Sample right;
} StereoSample;

#ifdef WINDOWS
#pragma pack( push, enter_include1 )
#pragma pack(2)
#endif

typedef struct stereo16bittag
{
	short left;
	short right;
} __PACKED__ Stereo16bit;

typedef struct pcmendstag
{
	ushort pcmends;
	ushort pcmadrs[256][2];
} __PACKED__ PCMEnds;

#ifdef WINDOWS
#pragma pack( pop, enter_include1 )
#endif


#define	PVIHeader	"PVI2"
#define	PPCHeader	"ADPCM DATA for  PMD ver.4.4-  "
//#define ver 		"4.8o"
//#define vers		0x48
//#define verc		"o"
//#define date		"Jun.19th 1997"

//#define max_part1	22		// ０クリアすべきパート数(for PMDPPZ)
#define max_part2	11		// 初期化すべきパート数  (for PMDPPZ)

#define mdata_def	64
#define voice_def	 8
#define effect_def	64

#define fmvd_init	0		// ９８は８８よりもＦＭ音源を小さく


/******************************************************************************
;	WORK AREA
******************************************************************************/
typedef struct PMDworktag {
	int		partb;				//	処理中パート番号
	int		tieflag;			//	&のフラグ(1 : tie)
	int		volpush_flag;		//	次の１音音量down用のflag(1 : voldown)
	int		rhydmy;				//	R part ダミー演奏データ
	int		fmsel;				//	FM 表(=0)か裏(=0x100)か flag
	int		omote_key[3];		//	FM keyondata表
	int		ura_key[3];			//	FM keyondata裏
	int		loop_work;			//	Loop Work
	bool	ppsdrv_flag;		//	ppsdrv を使用するか？flag(ユーザーが代入)
	int		pcmrepeat1;			//	PCMのリピートアドレス1
	int		pcmrepeat2;			//	PCMのリピートアドレス2
	int		pcmrelease;			//	PCMのRelease開始アドレス
	int		lastTimerAtime;		//	一個前の割り込み時のTimerATime値
	int		music_flag;			//	B0:次でMSTART 1:次でMSTOP のFlag
	int		slotdetune_flag;	//	FM3 Slot Detuneを使っているか
	int		slot3_flag;			//	FM3 Slot毎 要効果音モードフラグ
	int		fm3_alg_fb;			//	FM3chの最後に定義した音色のalg/fb
	int		af_check;			//	FM3chのalg/fbを設定するかしないかflag
	int		lfo_switch;			//	局所LFOスイッチ
} PMDWORK;


typedef struct effworktag {
	int		*effadr;			//	effect address
	int		eswthz;				//	トーンスゥイープ周波数
	int		eswtst;				//	トーンスゥイープ増分
	int		effcnt;				//	effect count
	int		eswnhz;				//	ノイズスゥイープ周波数
	int		eswnst;				//	ノイズスゥイープ増分
	int		eswnct;				//	ノイズスゥイープカウント
	int		effon;				//	効果音　発音中
	int		psgefcnum;			//	効果音番号
	int		hosei_flag;			//	ppsdrv 音量/音程補正をするかどうか
	int		last_shot_data;		//	最後に発音させたPPSDRV音色
} EFFWORK;


//	演奏中のデータエリア
typedef struct qqtag {
	uchar	*address;			//	2 エンソウチュウ ノ アドレス
	uchar	*partloop;			//	2 エンソウ ガ オワッタトキ ノ モドリサキ
	int		leng;				//	1 ノコリ LENGTH
	int		qdat;				//	1 gatetime (q/Q値を計算した値)
	uint	fnum;				//	2 エンソウチュウ ノ BLOCK/FNUM
	int		detune;				//	2 デチューン
	int		lfodat;				//	2 LFO DATA
	int		porta_num;			//	2 ポルタメントの加減値（全体）
	int		porta_num2;			//	2 ポルタメントの加減値（一回）
	int		porta_num3;			//	2 ポルタメントの加減値（余り）
	int		volume;				//	1 VOLUME
	int		shift;				//	1 オンカイ シフト ノ アタイ
	int		delay;				//	1 LFO	[DELAY] 
	int		speed;				//	1	[SPEED]
	int		step;				//	1	[STEP]
	int		time;				//	1	[TIME]
	int		delay2;				//	1	[DELAY_2]
	int		speed2;				//	1	[SPEED_2]
	int		step2;				//	1	[STEP_2]
	int		time2;				//	1	[TIME_2]
	int		lfoswi;				//	1 LFOSW. B0/tone B1/vol B2/同期 B3/porta
								//	         B4/tone B5/vol B6/同期
	int		volpush;			//	1 Volume PUSHarea
	int		mdepth;				//	1 M depth
	int		mdspd;				//	1 M speed
	int		mdspd2;				//	1 M speed_2
	int		envf;				//	1 PSG ENV. [START_FLAG] / -1でextend
	int		eenv_count;			//	1 ExtendPSGenv/No=0 AR=1 DR=2 SR=3 RR=4
	int		eenv_ar;			//	1 	/AR		/旧pat
	int		eenv_dr;			//	1	/DR		/旧pv2
	int		eenv_sr;			//	1	/SR		/旧pr1
	int		eenv_rr;			//	1	/RR		/旧pr2
	int		eenv_sl;			//	1	/SL
	int		eenv_al;			//	1	/AL
	int		eenv_arc;			//	1	/ARのカウンタ	/旧patb
	int		eenv_drc;			//	1	/DRのカウンタ
	int		eenv_src;			//	1	/SRのカウンタ	/旧pr1b
	int		eenv_rrc;			//	1	/RRのカウンタ	/旧pr2b
	int		eenv_volume;		//	1	/Volume値(0〜15)/旧penv
	int		extendmode;			//	1 B1/Detune B2/LFO B3/Env Normal/Extend
	int		fmpan;				//	1 FM Panning + AMD + PMD
	int		psgpat;				//	1 PSG PATTERN [TONE/NOISE/MIX]
	int		voicenum;			//	1 音色番号
	int		loopcheck;			//	1 ループしたら１ 終了したら３
	int		carrier;			//	1 FM Carrier
	int		slot1;				//	1 SLOT 1 ノ TL
	int		slot3;				//	1 SLOT 3 ノ TL
	int		slot2;				//	1 SLOT 2 ノ TL
	int		slot4;				//	1 SLOT 4 ノ TL
	int		slotmask;			//	1 FM slotmask
	int		neiromask;			//	1 FM 音色定義用maskdata
	int		lfo_wave;			//	1 LFOの波形
	int		partmask;			//	1 PartMask b0:通常 b1:効果音 b2:NECPCM用
								//	   b3:none b4:PPZ/ADE用 b5:s0時 b6:m b7:一時
	int		keyoff_flag;		//	1 KeyoffしたかどうかのFlag
	int		volmask;			//	1 音量LFOのマスク
	int		qdata;				//	1 qの値
	int		qdatb;				//	1 Qの値
	int		hldelay;			//	1 HardLFO delay
	int		hldelay_c;			//	1 HardLFO delay Counter
	int		_lfodat;			//	2 LFO DATA
	int		_delay;				//	1 LFO	[DELAY] 
	int		_speed;				//	1		[SPEED]
	int		_step;				//	1		[STEP]
	int		_time;				//	1		[TIME]
	int		_delay2;			//	1		[DELAY_2]
	int		_speed2;			//	1		[SPEED_2]
	int		_step2;				//	1		[STEP_2]
	int		_time2;				//	1		[TIME_2]
	int		_mdepth;			//	1 M depth
	int		_mdspd;				//	1 M speed
	int		_mdspd2;			//	1 M speed_2
	int		_lfo_wave;			//	1 LFOの波形
	int		_volmask;			//	1 音量LFOのマスク
	int		mdc;				//	1 M depth Counter (変動値)
	int		mdc2;				//	1 M depth Counter
	int		_mdc;				//	1 M depth Counter (変動値)
	int		_mdc2;				//	1 M depth Counter
	int		onkai;				//	1 演奏中の音階データ (0ffh:rest)
	int		sdelay;				//	1 Slot delay
	int		sdelay_c;			//	1 Slot delay counter
	int		sdelay_m;			//	1 Slot delay Mask
	int		alg_fb;				//	1 音色のalg/fb
	int		keyon_flag;			//	1 新音階/休符データを処理したらinc
	int		qdat2;				//	1 q 最低保証値
	int		onkai_def;			//	1 演奏中の音階データ (転調処理前 / ?fh:rest)
	int		shift_def;			//	1 マスター転調値
	int		qdat3;				//	1 q Random
} QQ;


typedef struct OpenWorktag {
	QQ *MusPart[NumOfAllPart];	// パートワークのポインタ
	uchar	*mmlbuf;			//	Musicdataのaddress+1
	uchar	*tondat;			//	Voicedataのaddress
	uchar	*efcdat;			//	FM  Effecdataのaddress
	uchar	*prgdat_adr;		//	曲データ中音色データ先頭番地
	ushort	*radtbl;			//	R part offset table 先頭番地
	uchar	*rhyadr;			//	R part 演奏中番地
	int		rhythmmask;			//	Rhythm音源のマスク x8c/10hのbitに対応
	int		fm_voldown;			//	FM voldown 数値
	int		ssg_voldown;		//	PSG voldown 数値
	int		pcm_voldown;		//	ADPCM voldown 数値
	int		rhythm_voldown;		//	RHYTHM voldown 数値
	int		prg_flg;			//	曲データに音色が含まれているかflag
	int		x68_flg;			//	OPM flag
	int		status;				//	status1
	int		status2;			//	status2
	int		tempo_d;			//	tempo (TIMER-B)
	int		fadeout_speed;		//	Fadeout速度
	int		fadeout_volume;		//	Fadeout音量
	int		tempo_d_push;		//	tempo (TIMER-B) / 保存用
	int		syousetu_lng;		//	小節の長さ
	int		opncount;			//	最短音符カウンタ
	int		TimerAtime;			//	TimerAカウンタ
	int		effflag;			//	PSG効果音発声on/off flag(ユーザーが代入)
	int		psnoi;				//	PSG noise周波数
	int		psnoi_last;			//	PSG noise周波数(最後に定義した数値)
	int		pcmstart;			//	PCM音色のstart値
	int		pcmstop;			//	PCM音色のstop値
	int		rshot_dat;			//	リズム音源 shot flag
	int		rdat[6];			//	リズム音源 音量/パンデータ
	int		rhyvol;				//	リズムトータルレベル
	int		kshot_dat;			//	ＳＳＧリズム shot flag
	int		play_flag;			//	play flag
	int		fade_stop_flag;		//	Fadeout後 MSTOPするかどうかのフラグ
	bool	kp_rhythm_flag;		//	K/RpartでRhythm音源を鳴らすかflag
	int		pcm_gs_flag;		//	ADPCM使用 許可フラグ (0で許可)
	int		slot_detune1;		//	FM3 Slot Detune値 slot1
	int		slot_detune2;		//	FM3 Slot Detune値 slot2
	int		slot_detune3;		//	FM3 Slot Detune値 slot3
	int		slot_detune4;		//	FM3 Slot Detune値 slot4
	int		TimerB_speed;		//	TimerBの現在値(=ff_tempoならff中)
	int		fadeout_flag;		//	内部からfoutを呼び出した時1
	int		revpan;				//	PCM86逆相flag
	int		pcm86_vol;			//	PCM86の音量をSPBに合わせるか?
	int		syousetu;			//	小節カウンタ
	int		port22h;			//	OPN-PORT 22H に最後に出力した値(hlfo)
	int		tempo_48;			//	現在のテンポ(clock=48 tの値)
	int		tempo_48_push;		//	現在のテンポ(同上/保存用)
	int		_fm_voldown;		//	FM voldown 数値 (保存用)
	int		_ssg_voldown;		//	PSG voldown 数値 (保存用)
	int		_pcm_voldown;		//	PCM voldown 数値 (保存用)
	int		_rhythm_voldown;	//	RHYTHM voldown 数値 (保存用)
	int		_pcm86_vol;			//	PCM86の音量をSPBに合わせるか? (保存用)
	int		rshot_bd;			//	リズム音源 shot inc flag (BD)
	int		rshot_sd;			//	リズム音源 shot inc flag (SD)
	int		rshot_sym;			//	リズム音源 shot inc flag (CYM)
	int		rshot_hh;			//	リズム音源 shot inc flag (HH)
	int		rshot_tom;			//	リズム音源 shot inc flag (TOM)
	int		rshot_rim;			//	リズム音源 shot inc flag (RIM)
	int		rdump_bd;			//	リズム音源 dump inc flag (BD)
	int		rdump_sd;			//	リズム音源 dump inc flag (SD)
	int		rdump_sym;			//	リズム音源 dump inc flag (CYM)
	int		rdump_hh;			//	リズム音源 dump inc flag (HH)
	int		rdump_tom;			//	リズム音源 dump inc flag (TOM)
	int		rdump_rim;			//	リズム音源 dump inc flag (RIM)
	int		ch3mode;			//	ch3 Mode
	int		ppz_voldown;		//	PPZ8 voldown 数値
	int		_ppz_voldown;		//	PPZ8 voldown 数値 (保存用)
	int		TimerAflag;			//	TimerA割り込み中？フラグ（＠不要？）
	int		TimerBflag;			//	TimerB割り込み中？フラグ（＠不要？）

	// for PMDWin
	int		rate;				//	PCM 出力周波数(11k, 22k, 44k, 55k)
	int		ppzrate;			//	PPZ 出力周波数
	bool	fmcalc55k;							// FM で 55kHz 合成をするか？
	bool	ppz8ip;								//	PPZ8 で補完するか
	bool	ppsip;								//	PPS  で補完するか
	bool	p86ip;								//	P86  で補完するか
	bool	use_p86;							//	P86  を使用しているか
	int		fadeout2_speed;						//	fadeout(高音質)speed(>0で fadeout)
	char	mus_filename[_MAX_PATH];			//	曲のFILE名バッファ
	char	ppcfilename[_MAX_PATH];				//	PPC のFILE名バッファ
	char	pcmdir[MAX_PCMDIR+1][_MAX_PATH];	//	PCM 検索ディレクトリ
} OPEN_WORK;


//=============================================================================
//	COM 風 interface class
//=============================================================================
#ifdef ENABLE_COM_INTERFACE
interface IPMDWIN : public IFMPMD {
	virtual void WINAPI setppsuse(bool value) = 0;
	virtual void WINAPI setrhythmwithssgeffect(bool value) = 0;
	virtual void WINAPI setpmd86pcmmode(bool value) = 0;
	virtual bool WINAPI getpmd86pcmmode(void) = 0;
	virtual void WINAPI setppsinterpolation(bool ip) = 0;
	virtual void WINAPI setp86interpolation(bool ip) = 0;
	virtual int WINAPI maskon(int ch) = 0;
	virtual int WINAPI maskoff(int ch) = 0;
	virtual void WINAPI setfmvoldown(int voldown) = 0;
	virtual void WINAPI setssgvoldown(int voldown) = 0;
	virtual void WINAPI setrhythmvoldown(int voldown) = 0;
	virtual void WINAPI setadpcmvoldown(int voldown) = 0;
	virtual void WINAPI setppzvoldown(int voldown) = 0;
	virtual int WINAPI getfmvoldown(void) = 0;
	virtual int WINAPI getfmvoldown2(void) = 0;
	virtual int WINAPI getssgvoldown(void) = 0;
	virtual int WINAPI getssgvoldown2(void) = 0;
	virtual int WINAPI getrhythmvoldown(void) = 0;
	virtual int WINAPI getrhythmvoldown2(void) = 0;
	virtual int WINAPI getadpcmvoldown(void) = 0;
	virtual int WINAPI getadpcmvoldown2(void) = 0;
	virtual int WINAPI getppzvoldown(void) = 0;
	virtual int WINAPI getppzvoldown2(void) = 0;
	virtual char* WINAPI getmemo(char *dest, uchar *musdata, int size, int al) = 0;
	virtual char* WINAPI getmemo2(char *dest, uchar *musdata, int size, int al) = 0;
	virtual char* WINAPI getmemo3(char *dest, uchar *musdata, int size, int al) = 0;
	virtual int	WINAPI fgetmemo(char *dest, char *filename, int al) = 0;
	virtual int	WINAPI fgetmemo2(char *dest, char *filename, int al) = 0;
	virtual int	WINAPI fgetmemo3(char *dest, char *filename, int al) = 0;
	virtual char* WINAPI getppcfilename(char *dest) = 0;
	virtual char* WINAPI getppsfilename(char *dest) = 0;
	virtual char* WINAPI getp86filename(char *dest) = 0;
	virtual int WINAPI ppc_load(char *filename) = 0;
	virtual int WINAPI pps_load(char *filename) = 0;
	virtual int WINAPI p86_load(char *filename) = 0;
	virtual int WINAPI ppz_load(char *filename, int bufnum) = 0;
	virtual OPEN_WORK* WINAPI getopenwork(void) = 0;
	virtual QQ* WINAPI getpartwork(int ch) = 0;
};
#endif /* ENABLE_COM_INTERFACE */

//=============================================================================
//	PMDWin class
//=============================================================================

class PMDWIN /*  : public IPMDWIN */
{
public:
	PMDWIN();
	~PMDWIN();
	
	// IUnknown
#ifdef ENABLE_COM_INTERFACE
	HRESULT WINAPI QueryInterface(
           /* [in] */ REFIID riid,
           /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG WINAPI AddRef(void);
	ULONG WINAPI Release(void);
#endif
	
	// IPCMMUSICDRIVER
	bool WINAPI init(char *path);
	int WINAPI music_load(char *filename);
	int WINAPI music_load2(uchar *musdata, int size);
	char* WINAPI getmusicfilename(char *dest);
	void WINAPI music_start(void);
	void WINAPI music_stop(void);
	int WINAPI getloopcount(void);
	bool WINAPI getlength(char *filename, int *length, int *loop);
	int WINAPI getpos(void);
	void WINAPI setpos(int pos);
	void WINAPI getpcmdata(short *buf, int nsamples);
	
	// IFMPMD
	bool WINAPI loadrhythmsample(char *path);
	bool WINAPI setpcmdir(char **path);
	void WINAPI setpcmrate(int rate);
	void WINAPI setppzrate(int rate);
	void WINAPI setfmcalc55k(bool flag);
	void WINAPI setppzinterpolation(bool ip);
	void WINAPI setfmwait(int nsec);
	void WINAPI setssgwait(int nsec);
	void WINAPI setrhythmwait(int nsec);
	void WINAPI setadpcmwait(int nsec);
	void WINAPI fadeout(int speed);
	void WINAPI fadeout2(int speed);
	void WINAPI setpos2(int pos);
	int WINAPI getpos2(void);
	char* WINAPI getpcmfilename(char *dest);
	char* WINAPI getppzfilename(char *dest, int bufnum);
	bool WINAPI getlength2(char *filename, int *length, int *loop);
	
	// IPMDWIN
	void WINAPI setppsuse(bool value);
	void WINAPI setrhythmwithssgeffect(bool value);
	void WINAPI setpmd86pcmmode(bool value);
	bool WINAPI getpmd86pcmmode(void);
	void WINAPI setppsinterpolation(bool ip);
	void WINAPI setp86interpolation(bool ip);
	int WINAPI maskon(int ch);
	int WINAPI maskoff(int ch);
	void WINAPI setfmvoldown(int voldown);
	void WINAPI setssgvoldown(int voldown);
	void WINAPI setrhythmvoldown(int voldown);
	void WINAPI setadpcmvoldown(int voldown);
	void WINAPI setppzvoldown(int voldown);
	int WINAPI getfmvoldown(void);
	int WINAPI getfmvoldown2(void);
	int WINAPI getssgvoldown(void);
	int WINAPI getssgvoldown2(void);
	int WINAPI getrhythmvoldown(void);
	int WINAPI getrhythmvoldown2(void);
	int WINAPI getadpcmvoldown(void);
	int WINAPI getadpcmvoldown2(void);
	int WINAPI getppzvoldown(void);
	int WINAPI getppzvoldown2(void);
	
	char* WINAPI getmemo(char *dest, uchar *musdata, int size, int al);
	char* WINAPI getmemo2(char *dest, uchar *musdata, int size, int al);
	char* WINAPI getmemo3(char *dest, uchar *musdata, int size, int al);
	int	WINAPI fgetmemo(char *dest, char *filename, int al);
	int	WINAPI fgetmemo2(char *dest, char *filename, int al);
	int	WINAPI fgetmemo3(char *dest, char *filename, int al);
	char* WINAPI getppcfilename(char *dest);
	char* WINAPI getppsfilename(char *dest);
	char* WINAPI getp86filename(char *dest);
	int WINAPI ppc_load(char *filename);
	int WINAPI pps_load(char *filename);
	int WINAPI p86_load(char *filename);
	int WINAPI ppz_load(char *filename, int bufnum);
	OPEN_WORK* WINAPI getopenwork(void);
	QQ* WINAPI getpartwork(int ch);
	
private:
	FM::OPNAW	opna;
	PPZ8		ppz8;
	PPSDRV		ppsdrv;
	P86DRV		p86drv;
	
	OPEN_WORK open_work;
	QQ FMPart[NumOfFMPart], SSGPart[NumOfSSGPart], ADPCMPart, RhythmPart;
	QQ ExtPart[NumOfExtPart], DummyPart, EffPart, PPZ8Part[NumOfPPZ8Part];
	
	PMDWORK pmdwork;
	EFFWORK effwork;
	
	Stereo16bit	wavbuf2[nbufsample];
	StereoSample	wavbuf[nbufsample];
	StereoSample	wavbuf_conv[nbufsample];
	
	char	*pos2;						// buf に余っているサンプルの先頭位置
	int		us2;						// buf に余っているサンプル数
	_int64	upos;						// 演奏開始からの時間(μs)
	_int64	fpos;						// fadeout2 開始時間
	
	uchar mdataarea[mdata_def*1024];
	uchar vdataarea[voice_def*1024];		//@不要？
	uchar edataarea[effect_def*1024];		//@不要？
	PCMEnds pcmends;

protected:
	int uRefCount;		// 参照カウンタ
	short read_short(void *value);
	ushort read_word(void *value);
	long read_long(void *value);
	int  read_char(void *value);

	void opnint_start(void);
	void data_init(void);
	void opn_init(void);
	void mstop(void);
	void mstop_f(void);
	void silence(void);
	void mstart(void);
	void mstart_f(void);
	void play_init(void);
	void setint(void);
	void calc_tb_tempo(void);
	void calc_tempo_tb(void);
	void settempo_b(void);
	void FM_Timer_main(void);
	void TimerA_main(void);
	void TimerB_main(void);
	void mmain(void);
	void syousetu_count(void);
	void fmmain(QQ *qq);
	void psgmain(QQ *qq);
	void rhythmmain(QQ *qq);
	void adpcmmain(QQ *qq);
	void pcm86main(QQ *qq);
	void ppz8main(QQ *qq);
	uchar *rhythmon(QQ *qq, uchar *bx, int al, int *result);
	void effgo(QQ *qq, int al);
	void eff_on2(QQ *qq, int al);
	void eff_main(QQ *qq, int al);
	void effplay(void);
	void efffor(const int *si);
	void effend(void);
	void effsweep(void);
	uchar *pdrswitch(QQ *qq, uchar *si);

	int silence_fmpart(QQ *qq);
	void keyoff(QQ *qq);
	void kof1(QQ *qq);
	void keyoffp(QQ *qq);
	void keyoffm(QQ *qq);
	void keyoff8(QQ *qq);
	void keyoffz(QQ *qq);
	int ssgdrum_check(QQ *qq, int al);
	uchar *commands(QQ *qq, uchar *si);
	uchar *commandsp(QQ *qq, uchar *si);
	uchar *commandsr(QQ *qq, uchar *si);
	uchar *commandsm(QQ *qq, uchar *si);
	uchar *commands8(QQ *qq, uchar *si);
	uchar *commandsz(QQ *qq, uchar *si);
	uchar *special_0c0h(QQ *qq, uchar *si, uchar al);
	uchar *_vd_fm(QQ *qq, uchar *si);
	uchar *_vd_ssg(QQ *qq, uchar *si);
	uchar *_vd_pcm(QQ *qq, uchar *si);
	uchar *_vd_rhythm(QQ *qq, uchar *si);
	uchar *_vd_ppz(QQ *qq, uchar *si);
	uchar *comt(uchar *si);
	uchar *comat(QQ *qq, uchar *si);
	uchar *comatm(QQ *qq, uchar *si);
	uchar *comat8(QQ *qq, uchar *si);
	uchar *comatz(QQ *qq, uchar *si);
	uchar *comstloop(QQ *qq, uchar *si);
	uchar *comedloop(QQ *qq, uchar *si);
	uchar *comexloop(QQ *qq, uchar *si);
	uchar *extend_psgenvset(QQ *qq, uchar *si);
	void lfoinit(QQ *qq, int al);
	void lfoinitp(QQ *qq, int al);
	uchar *lfoset(QQ *qq, uchar *si);
	uchar *psgenvset(QQ *qq, uchar *si);
	uchar *rhykey(uchar *si);
	uchar *rhyvs(uchar *si);
	uchar *rpnset(uchar *si);
	uchar *rmsvs(uchar *si);
	uchar *rmsvs_sft(uchar *si);
	uchar *rhyvs_sft(uchar *si);

	uchar *vol_one_up_psg(QQ *qq, uchar *si);
	uchar *vol_one_up_pcm(QQ *qq, uchar *si);
	uchar *vol_one_down(QQ *qq, uchar *si);
	uchar *portap(QQ *qq, uchar *si);
	uchar *portam(QQ *qq, uchar *si);
	uchar *portaz(QQ *qq, uchar *si);
	uchar *psgnoise_move(uchar *si);
	uchar *mdepth_count(QQ *qq, uchar *si);
	uchar *toneadr_calc(QQ *qq, int dl);
	void neiroset(QQ *qq, int dl);

	int oshift(QQ *qq, int al);
	int oshiftp(QQ *qq, int al);
	void fnumset(QQ *qq, int al);
	void fnumsetp(QQ *qq, int al);
	void fnumsetm(QQ *qq, int al);
	void fnumset8(QQ *qq, int al);
	void fnumsetz(QQ *qq, int al);
	uchar *panset(QQ *qq, uchar *si);
	uchar *panset_ex(QQ *qq, uchar *si);
	uchar *pansetm(QQ *qq, uchar *si);
	uchar *panset8(QQ *qq, uchar *si);
	uchar *pansetz(QQ *qq, uchar *si);
	uchar *pansetz_ex(QQ *qq, uchar *si);
	void panset_main(QQ *qq, int al);
	uchar calc_panout(QQ *qq);
	uchar *calc_q(QQ *qq, uchar *si);
	void fm_block_calc(int *cx, int *ax);
	int ch3_setting(QQ *qq);
	void cm_clear(int *ah, int *al);
	void ch3mode_set(QQ *qq);
	void ch3_special(QQ *qq, int ax, int cx);
	void volset(QQ *qq);
	void volsetp(QQ *qq);
	void volsetm(QQ *qq);
	void volset8(QQ *qq);
	void volsetz(QQ *qq);
	void otodasi(QQ *qq);
	void otodasip(QQ *qq);
	void otodasim(QQ *qq);
	void otodasi8(QQ *qq);
	void otodasiz(QQ *qq);
	void keyon(QQ *qq);
	void keyonp(QQ *qq);
	void keyonm(QQ *qq);
	void keyon8(QQ *qq);
	void keyonz(QQ *qq);
	int lfo(QQ *qq);
	int lfop(QQ *qq);
	uchar *lfoswitch(QQ *qq, uchar *si);
	void lfoinit_main(QQ *qq);
	void lfo_change(QQ *qq);
	void lfo_exit(QQ *qq);
	void lfin1(QQ *qq);
	void lfo_main(QQ *qq);
	void fmlfo_sub(QQ *qq, int al, int bl, uchar *vol_tbl);
	void volset_slot(int dh, int dl, int al);
	void porta_calc(QQ *qq);
	int soft_env(QQ *qq);
	int soft_env_main(QQ *qq);
	int soft_env_sub(QQ *qq);
	int ext_ssgenv_main(QQ *qq);
	void esm_sub(QQ *qq, int ah);
	void psgmsk(void);
	int get07(void);
	void md_inc(QQ *qq);
	uchar *pcmrepeat_set(QQ *qq, uchar * si);
	uchar *pcmrepeat_set8(QQ *qq, uchar * si);
	uchar *ppzrepeat_set(QQ *qq, uchar * si);
	uchar *pansetm_ex(QQ *qq, uchar * si);
	uchar *panset8_ex(QQ *qq, uchar * si);
	uchar *pcm_mml_part_mask(QQ *qq, uchar * si);
	uchar *pcm_mml_part_mask8(QQ *qq, uchar * si);
	uchar *ppz_mml_part_mask(QQ *qq, uchar * si);
	void pcmstore(ushort pcmstart, ushort pcmstop, uchar *buf);
	void pcmread(ushort pcmstart, ushort pcmstop, uchar *buf);

	uchar *hlfo_set(QQ *qq, uchar *si);
	uchar *vol_one_up_fm(QQ *qq, uchar *si);
	uchar *porta(QQ *qq, uchar *si);
	uchar *slotmask_set(QQ *qq, uchar *si);
	uchar *slotdetune_set(QQ *qq, uchar *si);
	uchar *slotdetune_set2(QQ *qq, uchar *si);
	void fm3_partinit(QQ *qq, uchar *ax);
	uchar *fm3_extpartset(QQ *qq, uchar *si);
	uchar *ppz_extpartset(QQ *qq, uchar *si);
	uchar *volmask_set(QQ *qq, uchar *si);
	uchar *fm_mml_part_mask(QQ *qq, uchar *si);
	uchar *ssg_mml_part_mask(QQ *qq, uchar *si);
	uchar *rhythm_mml_part_mask(QQ *qq, uchar *si);
	uchar *_lfoswitch(QQ *qq, uchar *si);
	uchar *_volmask_set(QQ *qq, uchar *si);
	uchar *tl_set(QQ *qq, uchar *si);
	uchar *fb_set(QQ *qq, uchar *si);
	uchar *fm_efct_set(QQ *qq, uchar *si);
	uchar *ssg_efct_set(QQ *qq, uchar *si);
	void fout(void);
	void neiro_reset(QQ *qq);
	int music_load3(uchar *musdata, int size, char *current_dir);
	int ppc_load2(char *filename);
	int ppc_load3(uchar *pcmdata, int size);
	char *search_pcm(char *dest, const char *filename, const char *current_dir);
	void swap(int *a, int *b);
};


//=============================================================================
//	Interface ID(IID) & Class ID(CLSID)
//=============================================================================
#ifdef ENABLE_COM_INTERFACE
// GUID of IPMDWIN Interface ID
interface	__declspec(uuid("C07008F4-CAE0-421C-B08F-D8B319AFA4B4")) IPMDWIN;	

// GUID of PMDWIN Class ID
class		__declspec(uuid("97C7C3F0-35D8-4304-8C1B-AA926E7AEC5C")) PMDWIN;

const IID	IID_IPMDWIN		= _uuidof(IPMDWIN);	// IPMDWIN Interface ID
const CLSID	CLSID_PMDWIN	= _uuidof(PMDWIN);	// PMDWIN Class ID
#endif

//============================================================================
//	DLL Export Functions
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) int WINAPI getversion(void);
__declspec(dllexport) int WINAPI getinterfaceversion(void);
__declspec(dllexport) bool WINAPI pmdwininit(char *path);
__declspec(dllexport) bool WINAPI loadrhythmsample(char *path);
__declspec(dllexport) bool WINAPI setpcmdir(char **path);
__declspec(dllexport) void WINAPI setpcmrate(int rate);
__declspec(dllexport) void WINAPI setppzrate(int rate);
__declspec(dllexport) void WINAPI setppsuse(bool value);
__declspec(dllexport) void WINAPI setrhythmwithssgeffect(bool value);
__declspec(dllexport) void WINAPI setpmd86pcmmode(bool value);
__declspec(dllexport) bool WINAPI getpmd86pcmmode(void);
__declspec(dllexport) int WINAPI music_load(char *filename);
__declspec(dllexport) int WINAPI music_load2(uchar *musdata, int size);
__declspec(dllexport) void WINAPI music_start(void);
__declspec(dllexport) void WINAPI music_stop(void);
__declspec(dllexport) void WINAPI fadeout(int speed);
__declspec(dllexport) void WINAPI fadeout2(int speed);
__declspec(dllexport) void WINAPI getpcmdata(short *buf, int nsamples);
__declspec(dllexport) void WINAPI setfmcalc55k(bool flag);
__declspec(dllexport) void WINAPI setppsinterpolation(bool ip);
__declspec(dllexport) void WINAPI setp86interpolation(bool ip);
__declspec(dllexport) void WINAPI setppzinterpolation(bool ip);
__declspec(dllexport) char * WINAPI getmemo(char *dest, uchar *musdata, int size, int al);
__declspec(dllexport) char * WINAPI getmemo2(char *dest, uchar *musdata, int size, int al);
__declspec(dllexport) char * WINAPI getmemo3(char *dest, uchar *musdata, int size, int al);
__declspec(dllexport) int WINAPI fgetmemo(char *dest, char *filename, int al);
__declspec(dllexport) int WINAPI fgetmemo2(char *dest, char *filename, int al);
__declspec(dllexport) int WINAPI fgetmemo3(char *dest, char *filename, int al);
__declspec(dllexport) char * WINAPI getmusicfilename(char *dest);
__declspec(dllexport) char * WINAPI getpcmfilename(char *dest);
__declspec(dllexport) char * WINAPI getppcfilename(char *dest);
__declspec(dllexport) char * WINAPI getppsfilename(char *dest);
__declspec(dllexport) char * WINAPI getp86filename(char *dest);
__declspec(dllexport) char * WINAPI getppzfilename(char *dest, int bufnum);
__declspec(dllexport) int WINAPI ppc_load(char *filename);
__declspec(dllexport) int WINAPI pps_load(char *filename);
__declspec(dllexport) int WINAPI p86_load(char *filename);
__declspec(dllexport) int WINAPI ppz_load(char *filename, int bufnum);
__declspec(dllexport) int WINAPI maskon(int ch);
__declspec(dllexport) int WINAPI maskoff(int ch);
__declspec(dllexport) void WINAPI setfmvoldown(int voldown);
__declspec(dllexport) void WINAPI setssgvoldown(int voldown);
__declspec(dllexport) void WINAPI setrhythmvoldown(int voldown);
__declspec(dllexport) void WINAPI setadpcmvoldown(int voldown);
__declspec(dllexport) void WINAPI setppzvoldown(int voldown);
__declspec(dllexport) int WINAPI getfmvoldown(void);
__declspec(dllexport) int WINAPI getfmvoldown2(void);
__declspec(dllexport) int WINAPI getssgvoldown(void);
__declspec(dllexport) int WINAPI getssgvoldown2(void);
__declspec(dllexport) int WINAPI getrhythmvoldown(void);
__declspec(dllexport) int WINAPI getrhythmvoldown2(void);
__declspec(dllexport) int WINAPI getadpcmvoldown(void);
__declspec(dllexport) int WINAPI getadpcmvoldown2(void);
__declspec(dllexport) int WINAPI getppzvoldown(void);
__declspec(dllexport) int WINAPI getppzvoldown2(void);
__declspec(dllexport) void WINAPI setpos(int pos);
__declspec(dllexport) void WINAPI setpos2(int pos);
__declspec(dllexport) int WINAPI getpos(void);
__declspec(dllexport) int WINAPI getpos2(void);
__declspec(dllexport) bool WINAPI getlength(char *filename, int *length, int *loop);
__declspec(dllexport) bool WINAPI getlength2(char *filename, int *length, int *loop);
__declspec(dllexport) int WINAPI getloopcount(void);
__declspec(dllexport) void WINAPI setfmwait(int nsec);
__declspec(dllexport) void WINAPI setssgwait(int nsec);
__declspec(dllexport) void WINAPI setrhythmwait(int nsec);
__declspec(dllexport) void WINAPI setadpcmwait(int nsec);
__declspec(dllexport) OPEN_WORK * WINAPI getopenwork(void);
__declspec(dllexport) QQ * WINAPI getpartwork(int ch);

#ifdef ENABLE_COM_INTERFACE
__declspec(dllexport) HRESULT WINAPI pmd_CoCreateInstance(
  REFCLSID rclsid,     //Class identifier (CLSID) of the object
  LPUNKNOWN pUnkOuter, //Pointer to whether object is or isn't part 
                       // of an aggregate
  DWORD dwClsContext,  //Context for running executable code
  REFIID riid,         //Reference to the identifier of the interface
  LPVOID * ppv         //Address of output variable that receives 
                       // the interface pointer requested in riid
);
#endif

#ifdef __cplusplus
}
#endif


#endif // PMDWIN_H
