//=============================================================================
//		8 Channel PCM Driver 「PPZ8」 Unit(Light Version)
//			Programmed by UKKY
//			Windows Converted by C60
//=============================================================================

#ifndef PPZ8L_H
#define PPZ8L_H

#if defined _WIN32
#define		NOMINMAX
#include	<windows.h>
#include	<tchar.h>
#endif
#include	"pmdmini_file.h"
//#include	"types.h"


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

#if defined _WIN32
#pragma pack( push, enter_include1 )
#pragma pack(1)

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
	} pcmnum[128];
} PZIHEADER;

typedef struct pviheadertag
{
	char	header[4];				// 'PVI2'
	char	dummy1[0x0b-4];			// 予備１
	uchar	pvinum;					// PVIデータの定義数
	char	dummy2[0x10-0x0b-1];	// 予備２
	struct	{
		WORD	startaddress;		// 先頭アドレス
		WORD	endaddress;			// データ量
	} pcmnum[128];
} PVIHEADER;

#pragma pack( pop, enter_include1 )

#else

typedef struct pziheadertag
{
	char	header[4] __attribute__((packed));				// 'PZI1'
	char	dummy1[0x0b-4] __attribute__((packed));			// 予備１
	uchar	pzinum __attribute__((packed));					// PZIデータの定義数
	char	dummy2[0x20-0x0b-1] __attribute__((packed));	// 予備２
	struct	{
		DWORD	startaddress __attribute__((packed));		// 先頭アドレス
		DWORD	size __attribute__((packed));				// データ量
		DWORD	loop_start __attribute__((packed));			// ループ開始ポインタ
		DWORD	loop_end __attribute__((packed));			// ループ終了ポインタ
		WORD	rate __attribute__((packed));				// 再生周波数
	} pcmnum[128] __attribute__((packed));
} PZIHEADER __attribute__((packed));

typedef struct pviheadertag
{
	char	header[4] __attribute__((packed));				// 'PVI2'
	char	dummy1[0x0b-4] __attribute__((packed));			// 予備１
	uchar	pvinum __attribute__((packed));					// PVIデータの定義数
	char	dummy2[0x10-0x0b-1] __attribute__((packed));	// 予備２
	struct	{
		WORD	startaddress __attribute__((packed));		// 先頭アドレス
		WORD	endaddress __attribute__((packed));			// データ量
	} pcmnum[128] __attribute__((packed));
} PVIHEADER __attribute__((packed));

#endif

/// <summary>
/// PPZ8 インターフェース
/// </summary>
class IPPZ8
{
public:
	virtual bool __cdecl Init(uint rate, bool ip) = 0;				// 00H 初期化
	virtual bool __cdecl Play(int ch, int bufnum, int num, WORD start, WORD stop) = 0;
												// 01H PCM 発音
	virtual bool __cdecl Stop(int ch) = 0;							// 02H PCM 停止
	virtual int __cdecl Load(TCHAR *filename, int bufnum) = 0;	// 03H PVI/PZIﾌｧｲﾙの読み込み
	virtual bool __cdecl SetVol(int ch, int vol) = 0;				// 07H ﾎﾞﾘｭｰﾑ設定
	virtual bool __cdecl SetOntei(int ch, uint ontei) = 0;			// 0BH 音程周波数の設定
	virtual bool __cdecl SetLoop(int ch, uint loop_start, uint loop_end) = 0;
												// 0EH ﾙｰﾌﾟﾎﾟｲﾝﾀの設定
	virtual void __cdecl AllStop(void) = 0;							// 12H (PPZ8)全停止
	virtual bool __cdecl SetPan(int ch, int pan) = 0;				// 13H (PPZ8)PAN指定
	virtual bool __cdecl SetRate(uint rate, bool ip) = 0;			// 14H (PPZ8)ﾚｰﾄ設定
	virtual bool __cdecl SetSourceRate(int ch, int rate) = 0;		// 15H (PPZ8)元ﾃﾞｰﾀ周波数設定
	virtual void __cdecl SetAllVolume(int vol) = 0;					// 16H (PPZ8)全体ﾎﾞﾘﾕｰﾑの設定（86B Mixer)
	virtual void __cdecl SetVolume(int vol) = 0;
	//PCMTMP_SET		;17H PCMﾃﾝﾎﾟﾗﾘ設定
	virtual void __cdecl ADPCM_EM_SET(bool flag) = 0;				// 18H (PPZ8)ADPCMエミュレート
	//REMOVE_FSET		;19H (PPZ8)常駐解除ﾌﾗｸﾞ設定
	//FIFOBUFF_SET		;1AH (PPZ8)FIFOﾊﾞｯﾌｧの変更
	//RATE_SET		;1BH (PPZ8)WSS詳細ﾚｰﾄ設定
};


class PPZ8 : public IPPZ8
{
public:
	PPZ8();
	~PPZ8();
	
	bool __cdecl Init(uint rate, bool ip);				// 00H 初期化
	bool __cdecl Play(int ch, int bufnum, int num, WORD start, WORD stop);
														// 01H PCM 発音
	bool  __cdecl Stop(int ch);							// 02H PCM 停止
	int  __cdecl Load(TCHAR *filename, int bufnum);		// 03H PVI/PZIﾌｧｲﾙの読み込み
	bool  __cdecl SetVol(int ch, int vol);				// 07H ﾎﾞﾘｭｰﾑ設定
	bool  __cdecl SetOntei(int ch, uint ontei);			// 0BH 音程周波数の設定
	bool  __cdecl SetLoop(int ch, uint loop_start, uint loop_end);
														// 0EH ﾙｰﾌﾟﾎﾟｲﾝﾀの設定
	void  __cdecl AllStop(void);						// 12H (PPZ8)全停止
	bool  __cdecl SetPan(int ch, int pan);				// 13H (PPZ8)PAN指定
	bool  __cdecl SetRate(uint rate, bool ip);			// 14H (PPZ8)ﾚｰﾄ設定
	bool  __cdecl SetSourceRate(int ch, int rate);		// 15H (PPZ8)元ﾃﾞｰﾀ周波数設定
	void  __cdecl SetAllVolume(int vol);				// 16H (PPZ8)全体ﾎﾞﾘﾕｰﾑの設定（86B Mixer)
	void  __cdecl SetVolume(int vol);
	//PCMTMP_SET		;17H PCMﾃﾝﾎﾟﾗﾘ設定
	void  __cdecl ADPCM_EM_SET(bool flag);				// 18H (PPZ8)ADPCMエミュレート
	//REMOVE_FSET		;19H (PPZ8)常駐解除ﾌﾗｸﾞ設定
	//FIFOBUFF_SET		;1AH (PPZ8)FIFOﾊﾞｯﾌｧの変更
	//RATE_SET		;1BH (PPZ8)WSS詳細ﾚｰﾄ設定
	
	void Mix(Sample* dest, int nsamples);
	
	PZIHEADER PCME_WORK[2];						// PCMの音色ヘッダー
	bool	pviflag[2];							// PVI なら true
	TCHAR	PVI_FILE[2][_MAX_PATH];				// ファイル名
	
private:
	FilePath	filepath;						// ファイルパス関連のクラスライブラリ
	
	void	WORK_INIT(void);					// ﾜｰｸ初期化
	bool	ADPCM_EM_FLG;						// CH8 でADPCM エミュレートするか？
	bool	interpolation;						// 補完するか？
	int		AVolume;
	CHANNELWORK	channelwork[PCM_CNL_MAX];		// 各チャンネルのワーク
	uchar	*XMS_FRAME_ADR[2];					// XMSで確保したメモリアドレス（リニア）
	int		XMS_FRAME_SIZE[2];					// PZI or PVI 内部データサイズ
	int		PCM_VOLUME;							// 86B Mixer全体ボリューム
												// (DEF=12)
	int		volume;								// 全体ボリューム
												// (opna unit 互換)
	int		DIST_F;								// 再生周波数
	
//	static Sample VolumeTable[16][256];			// 音量テーブル
	Sample VolumeTable[16][256];				// 音量テーブル
	
	void	_Init(void);						// 初期化(内部処理)
	void	MakeVolumeTable(int vol);			// 音量テーブルの作成
};

#endif	// PPZ8L_H
