//=============================================================================
//		8 Channel PCM Driver 「PPZ8」 Unit(Light Version)
//			Programmed by UKKY
//			Windows Converted by C60
//=============================================================================

#ifndef PPZ8L_H
#define PPZ8L_H

#include "portability_fmpmdcore.h"
#include "ippz8.h"
#include "file_fmgen.h"
#include "ifileio.h"


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

typedef int32_t				Sample;

typedef struct channelworktag
{
	int32_t		PCM_ADD_L;				// アドレス増加量 LOW
	int32_t		PCM_ADD_H;				// アドレス増加量 HIGH
	int32_t		PCM_ADDS_L;				// アドレス増加量 LOW（元の値）
	int32_t		PCM_ADDS_H;				// アドレス増加量 HIGH（元の値）
	int32_t		PCM_SORC_F;				// 元データの再生レート
	int32_t		PCM_FLG;				// 再生フラグ
	int32_t		PCM_VOL;				// ボリューム
	int32_t		PCM_PAN;				// PAN
	int32_t		PCM_NUM;				// PCM番号
	int32_t		PCM_LOOP_FLG;			// ループ使用フラグ
	uint8_t	*PCM_NOW;				// 現在の値
	int32_t		PCM_NOW_XOR;			// 現在の値（小数部）
	uint8_t	*PCM_END;				// 現在の終了アドレス
	uint8_t	*PCM_END_S;				// 本当の終了アドレス
	uint8_t	*PCM_LOOP;				// ループ開始アドレス
	uint32_t	PCM_LOOP_START;			// リニアなループ開始アドレス
	uint32_t	PCM_LOOP_END;			// リニアなループ終了アドレス
	bool	pviflag;				// PVI なら true
} CHANNELWORK;

#if defined _WIN32
#pragma pack( push, enter_include1 )
#pragma pack(1)

typedef struct pziheadertag
{
	char	header[4];				// 'PZI1'
	char	dummy1[0x0b-4];			// 予備１
	uint8_t	pzinum;					// PZIデータの定義数
	char	dummy2[0x20-0x0b-1];	// 予備２
	struct	{
		uint32_t	startaddress;	// 先頭アドレス
		uint32_t	size;			// データ量
		uint32_t	loop_start;		// ループ開始ポインタ
		uint32_t	loop_end;		// ループ終了ポインタ
		uint16_t	rate;			// 再生周波数
	} pcmnum[128];
} PZIHEADER;

typedef struct pviheadertag
{
	char	header[4];				// 'PVI2'
	char	dummy1[0x0b-4];			// 予備１
	uint8_t	pvinum;					// PVIデータの定義数
	char	dummy2[0x10-0x0b-1];	// 予備２
	struct	{
		uint16_t	startaddress;	// 先頭アドレス
		uint16_t	endaddress;		// データ量
	} pcmnum[128];
} PVIHEADER;

#pragma pack( pop, enter_include1 )

#else

typedef struct __attribute__((packed)) pziheadertag
{
	char	header[4];				// 'PZI1'
	char	dummy1[0x0b-4];			// 予備１
	uint8_t	pzinum;					// PZIデータの定義数
	char	dummy2[0x20-0x0b-1];	// 予備２
	struct __attribute__((packed))	{
		uint32_t	startaddress;		// 先頭アドレス
		uint32_t	size;				// データ量
		uint32_t	loop_start;			// ループ開始ポインタ
		uint32_t	loop_end;			// ループ終了ポインタ
		uint16_t	rate;				// 再生周波数
	} pcmnum[128];
} PZIHEADER;

typedef struct __attribute__((packed)) pviheadertag
{
	char	header[4];				// 'PVI2'
	char	dummy1[0x0b-4];			// 予備１
	uint8_t	pvinum;					// PVIデータの定義数
	char	dummy2[0x10-0x0b-1];	// 予備２
	struct __attribute__((packed))	{
		uint16_t	startaddress;		// 先頭アドレス
		uint16_t	endaddress;			// データ量
	} pcmnum[128];
} PVIHEADER;

#endif



class PPZ8 : public IPPZ8
{
public:
	PPZ8(IFILEIO* pfileio);
	virtual ~PPZ8();
	
	bool __cdecl Init(uint32_t rate, bool ip);			// 00H 初期化
	bool __cdecl Play(int32_t ch, int32_t bufnum, int32_t num, uint16_t start, uint16_t stop);
														// 01H PCM 発音
	bool __cdecl Stop(int32_t ch);							// 02H PCM 停止
	int32_t  __cdecl Load(TCHAR *filename, int32_t bufnum);		// 03H PVI/PZIﾌｧｲﾙの読み込み
	bool __cdecl SetVol(int32_t ch, int32_t vol);				// 07H ﾎﾞﾘｭｰﾑ設定
	bool __cdecl SetOntei(int32_t ch, uint32_t ontei);		// 0BH 音程周波数の設定
	bool __cdecl SetLoop(int32_t ch, uint32_t loop_start, uint32_t loop_end);
														// 0EH ﾙｰﾌﾟﾎﾟｲﾝﾀの設定
	void __cdecl AllStop(void);							// 12H (PPZ8)全停止
	bool __cdecl SetPan(int32_t ch, int32_t pan);				// 13H (PPZ8)PAN指定
	bool __cdecl SetRate(uint32_t rate, bool ip);		// 14H (PPZ8)ﾚｰﾄ設定
	bool __cdecl SetSourceRate(int32_t ch, int32_t rate);		// 15H (PPZ8)元ﾃﾞｰﾀ周波数設定
	void __cdecl SetAllVolume(int32_t vol);					// 16H (PPZ8)全体ﾎﾞﾘﾕｰﾑの設定（86B Mixer)
	void __cdecl SetVolume(int32_t vol);
	//PCMTMP_SET		;17H PCMﾃﾝﾎﾟﾗﾘ設定
	void __cdecl ADPCM_EM_SET(bool flag);				// 18H (PPZ8)ADPCMエミュレート
	//REMOVE_FSET		;19H (PPZ8)常駐解除ﾌﾗｸﾞ設定
	//FIFOBUFF_SET		;1AH (PPZ8)FIFOﾊﾞｯﾌｧの変更
	//RATE_SET		;1BH (PPZ8)WSS詳細ﾚｰﾄ設定
	
	void setfileio(IFILEIO* pfileio);
	void Mix(Sample* dest, int32_t nsamples);
	
	PZIHEADER PCME_WORK[2];						// PCMの音色ヘッダー
	bool	pviflag[2];							// PVI なら true
	TCHAR	PVI_FILE[2][_MAX_PATH];				// ファイル名
	
private:
	FilePath	filepath;						// ファイルパス関連のクラスライブラリ
	IFILEIO*	pfileio;						// ファイルアクセス関連のクラスライブラリ
	
	void	WORK_INIT(void);					// ﾜｰｸ初期化
	bool	ADPCM_EM_FLG;						// CH8 でADPCM エミュレートするか？
	bool	interpolation;						// 補完するか？
	int32_t		AVolume;
	CHANNELWORK	channelwork[PCM_CNL_MAX];		// 各チャンネルのワーク
	uint8_t	*XMS_FRAME_ADR[2];					// XMSで確保したメモリアドレス（リニア）
	int32_t		XMS_FRAME_SIZE[2];					// PZI or PVI 内部データサイズ
	int32_t		PCM_VOLUME;							// 86B Mixer全体ボリューム
												// (DEF=12)
	int32_t		volume;								// 全体ボリューム
												// (opna unit 互換)
	int32_t		DIST_F;								// 再生周波数
	
//	static Sample VolumeTable[16][256];			// 音量テーブル
	Sample VolumeTable[16][256];				// 音量テーブル
	
	void	_Init(void);						// 初期化(内部処理)
	void	MakeVolumeTable(int32_t vol);			// 音量テーブルの作成
	void 	ReadHeader(IFILEIO* file, PZIHEADER &pziheader);
	void 	ReadHeader(IFILEIO* file, PVIHEADER &pviheader);
	
	inline int32_t Limit(int32_t v, int32_t max, int32_t min)
	{
		return v > max ? max : (v < min ? min : v);
	}
};

#endif	// PPZ8L_H
