//=============================================================================
//		8 Channel PCM Driver 「PPZ8」 Unit(Light Version)
//			Programmed by UKKY
//			Windows Converted by C60
//=============================================================================

#ifndef PPZ8L_H
#define PPZ8L_H

#ifdef HAVE_WINDOWS_H
# include	<windows.h>
#endif
//#include	"types.h"
#include "compat.h"


#define	SOUND_44K		 44100

#define		_PPZ8_VER		"1.07"
#define		RATE_DEF	 SOUND_44K
#define		VNUM_DEF			12
#define		PCM_CNL_MAX 		 8
#define		X_N0			  0x80
#define		DELTA_N0		   127

//	DLL の 戻り値
#define	_PPZ8_OK				 	  0		// 正常終了
#define	_ERR_OPEN_PPZ_FILE			  1		// PVI/PZI を開けなかった
#define	_ERR_WRONG_PPZ_FILE		 	  2		// PVI/PZI の形式が異なっている
#define	_WARNING_PPZ_ALREADY_LOAD	  3		// PVI/PZI はすでに読み込まれている

#define	_ERR_OUT_OF_MEMORY			 99		// メモリを確保できなかった

typedef int				Sample;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;

typedef struct channelworktag
{
	int		PCM_ADD_L;				// アドレス増加量 LOW
	int		PCM_ADD_H;				// アドレス増加量 HIGH
	int		PCM_ADDS_L;				// アドレス増加量 LOW（元の値）
	int		PCM_ADDS_H;				// アドレス増加量 HIGH（元の値）
	int		PCM_SORC_F;				// 元データの再生レート
	int		PCM_FLG;				// 再生フラグ
	int		PCM_VOL;				// ボリューム
	int		PCM_PAN;				// PAN
	int		PCM_NUM;				// PCM番号
	int		PCM_LOOP_FLG;			// ループ使用フラグ
	uchar	*PCM_NOW;				// 現在の値
	int		PCM_NOW_XOR;			// 現在の値（小数部）
	uchar	*PCM_END;				// 現在の終了アドレス
	uchar	*PCM_END_S;				// 本当の終了アドレス
	uchar	*PCM_LOOP;				// ループ開始アドレス
	uint	PCM_LOOP_START;			// リニアなループ開始アドレス
	uint	PCM_LOOP_END;			// リニアなループ終了アドレス
	bool	pviflag;				// PVI なら true
} CHANNELWORK;

#ifdef WINDOWS
#pragma pack( push, enter_include1 )
#pragma pack(1)
#endif

typedef struct pziheadertag
{
	char	header[4];				// 'PZI1'
	char	dummy1[0x0b-4];			// 予備１
	uchar	pzinum;					// PZIデータの定義数
	char	dummy2[0x20-0x0b-1];	// 予備２
	struct	{
		DWORD	startaddress;		// 先頭アドレス
		DWORD	size;				// データ量
		DWORD	loop_start;			// ループ開始ポインタ
		DWORD	loop_end;			// ループ終了ポインタ
		WORD	rate;				// 再生周波数
	} __PACKED__ pcmnum[128];
} __PACKED__ PZIHEADER;

typedef struct pviheadertag
{
	char	header[4];				// 'PVI2'
	char	dummy1[0x0b-4];			// 予備１
	uchar	pvinum;					// PVIデータの定義数
	char	dummy2[0x10-0x0b-1];	// 予備２
	struct	{
		WORD	startaddress;		// 先頭アドレス
		WORD	endaddress;			// データ量
	} __PACKED__ pcmnum[128];
} __PACKED__ PVIHEADER;

#ifdef WINDOWS
#pragma pack( pop, enter_include1 )
#endif


class PPZ8
{
public:
	PPZ8();
	~PPZ8();
	
	bool Init(uint rate, bool ip);				// 00H 初期化
	bool Play(int ch, int bufnum, int num);		// 01H PCM 発音
	bool Stop(int ch);							// 02H PCM 停止
	int Load(char *filename, int bufnum);		// 03H PVI/PZIファイルの読み込み
	bool SetVol(int ch, int vol);				// 07H ボリューム設定
	bool SetOntei(int ch, uint ontei);			// 0BH 音程周波数の設定
	bool SetLoop(int ch, uint loop_start, uint loop_end);
												// 0EH ループポインタの設定
	void AllStop(void);							// 12H (PPZ8)全停止
	bool SetPan(int ch, int pan);				// 13H (PPZ8)PAN指定
	bool SetRate(uint rate, bool ip);			// 14H (PPZ8)レート設定
	bool SetSourceRate(int ch, int rate);		// 15H (PPZ8)元データ周波数設定
	void SetAllVolume(int vol);					// 16H (PPZ8)全体ボリユームの設定（86B Mixer)
	void SetVolume(int vol);
	//PCMTMP_SET		;17H PCMテンポラリ設定
	//ADPCM_EM_SET		;18H (PPZ8)ADPCMエミュレート
	//REMOVE_FSET		;19H (PPZ8)常駐解除フラグ設定
	//FIFOBUFF_SET		;1AH (PPZ8)FIFOバッファの変更
	//RATE_SET		;1BH (PPZ8)WSS詳細レート設定

	void Mix(Sample* dest, int nsamples);

	PZIHEADER PCME_WORK[2];						// PCMの音色ヘッダー
	bool	pviflag[2];							// PVI なら true
	char	PVI_FILE[2][_MAX_PATH];				// ファイル名

private:
	void	WORK_INIT(void);					// ワーク初期化
	bool	interpolation;						// 補完するか？
	int		AVolume;
	CHANNELWORK	channelwork[PCM_CNL_MAX];		// 各チャンネルのワーク
	uchar	*XMS_FRAME_ADR[2];					// XMSで確保したメモリアドレス（リニア）
	int		PCM_VOLUME;							// 86B Mixer全体ボリューム
												// (DEF=12)
	int		volume;								// 全体ボリューム
												// (opna unit 互換)
	int		DIST_F;								// 再生周波数
	
//	static Sample VolumeTable[16][256];			// 音量テーブル
	Sample VolumeTable[16][256];				// 音量テーブル
	void	MakeVolumeTable(int vol);			// 音量テーブルの作成
};

#endif	// PPZ8L_H
