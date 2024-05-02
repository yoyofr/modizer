//=============================================================================
//		86B PCM Driver 「P86DRV」 Unit
//			programmed by M.Kajihara 96/01/16
//			Windows Converted by C60
//=============================================================================

#ifndef P86DRV_H
#define P86DRV_H

#include "portability_fmpmdcore.h"
#include "file_fmgen.h"
#include "ifileio.h"


//	DLL の 戻り値
#define	_P86DRV_OK					  0		// 正常終了
#define	_ERR_OPEN_P86_FILE			 81		// P86 を開けなかった
#define	_ERR_WRONG_P86_FILE		 	 82		// P86 の形式が異なっている
#define	_WARNING_P86_ALREADY_LOAD	 83		// P86 はすでに読み込まれている
#define	_ERR_OUT_OF_MEMORY			 99		// メモリを確保できなかった

#define	SOUND_44K				  44100
#define	SOUND_22K				  22050
#define	SOUND_11K				  11025

#define	P86DRV_VER				 "1.1c"
#define	MAX_P86						256
#define vers					   0x11
#define date			"Sep.11th 1996"


typedef int32_t				Sample;


#if defined _WIN32
#pragma pack( push, enter_include1 )
#pragma pack(1)

typedef struct p86headertag		// header(original)
{
	char	header[12];			// "PCM86 DATA",0,0
	uint8_t	Version;
	char	All_Size[3];
	struct {
		uint8_t	start[3];
		uint8_t	size[3];
	} pcmnum[MAX_P86];
} P86HEADER;

const size_t P86HEADERSIZE = 
	  sizeof(char) * 12 
	+ sizeof(uint8_t)
	+ sizeof(char) * 3 
	+ sizeof(uint8_t) * (3 + 3) * MAX_P86;


#pragma pack( pop, enter_include1 )

typedef struct p86headertag2	// header(for PMDWin, int32_t alignment)
{
	char	header[12];			// "PCM86 DATA",0,0
	int32_t		Version;
	int32_t		All_Size;
	struct {
		int32_t		start;
		int32_t		size;
	} pcmnum[MAX_P86];
} P86HEADER2;

#else

typedef struct __attribute__((packed)) p86headertag		// header(original)
{
	char	header[12];			// "PCM86 DATA",0,0
	uint8_t	Version;
	char	All_Size[3];
	struct __attribute__((packed)) {
		uint8_t	start[3];
		uint8_t	size[3];
	} pcmnum[MAX_P86];
} P86HEADER;

const size_t P86HEADERSIZE =
	  sizeof(char) * 12
	+ sizeof(uint8_t)
	+ sizeof(char) * 3
	+ sizeof(uint8_t) * (3 + 3) * MAX_P86;


typedef struct p86headertag2	// header(for PMDWin, int32_t alignment)
{
	char	header[12] __attribute__((aligned(4)));			// "PCM86 DATA",0,0
	int32_t		Version __attribute__((aligned(4)));
	int32_t		All_Size __attribute__((aligned(4)));
	struct {
		int32_t		start __attribute__((aligned(4)));
		int32_t		size __attribute__((aligned(4)));
	} pcmnum[MAX_P86] __attribute__((aligned(4)));
} P86HEADER2;

#endif


const int32_t ratetable[] = {
	4135, 5513, 8270, 11025, 16540, 22050, 33080, 44100
};


class P86DRV
{
public:
	P86DRV(IFILEIO* pfileio);
	virtual ~P86DRV();
	void setfileio(IFILEIO* pfileio);
	
	bool Init(uint32_t r, bool ip);						// 初期化
	bool Stop(void);								// P86 停止
	bool Play(void);								// P86 再生
	bool Keyoff(void);								// P86 keyoff
	int32_t Load(TCHAR *filename);						// P86 読み込み
	bool SetRate(uint32_t r, bool ip);					// レート設定
	void SetVolume(int32_t volume);						// 全体音量調節用
	bool SetVol(int32_t _vol);							// 音量設定
	bool SetOntei(int32_t rate, uint32_t ontei);			// 音程周波数の設定
	bool SetPan(int32_t flag, int32_t data);				// PAN 設定
	bool SetNeiro(int32_t num);							// PCM 番号設定
	bool SetLoop(int32_t loop_start, int32_t loop_end, int32_t release_start, bool adpcm);
													// ループ設定
	void Mix(Sample* dest, int32_t nsamples);			// 合成
	TCHAR	p86_file[_MAX_PATH];					// ファイル名
	P86HEADER2 p86header;							// P86 の音色ヘッダー
	
private:
	FilePath	filepath;							// ファイルパス関連のクラスライブラリ
	IFILEIO*	pfileio;							// ファイルアクセス関連のクラスライブラリ
	
	bool	interpolation;							// 補完するか？
	int32_t		rate;									// 再生周波数
	int32_t		srcrate;								// 元データの周波数
	uint32_t	ontei;									// 音程(fnum)
	int32_t		vol;									// 音量
	uint8_t	*p86_addr;								// P86 保存用メモリポインタ
	uint8_t	*start_ofs;								// 発音中PCMデータ番地
	int32_t		start_ofs_x;							// 発音中PCMデータ番地（小数部）
	int32_t		size;									// 残りサイズ
	uint8_t	*_start_ofs;							// 発音開始PCMデータ番地
	int32_t		_size;									// PCMデータサイズ
	int32_t		addsize1;								// PCMアドレス加算値 (整数部)
	int32_t		addsize2;								// PCMアドレス加算値 (小数部)
	uint8_t	*repeat_ofs;							// リピート開始位置
	int32_t		repeat_size;							// リピート後のサイズ
	uint8_t	*release_ofs;							// リリース開始位置
	int32_t		release_size;							// リリース後のサイズ
	bool	repeat_flag;							// リピートするかどうかのflag
	bool	release_flag1;							// リリースするかどうかのflag
	bool	release_flag2;							// リリースしたかどうかのflag
	
	int32_t		pcm86_pan_flag;		// パンデータ１(bit0=左/bit1=右/bit2=逆)
	int32_t		pcm86_pan_dat;		// パンデータ２(音量を下げるサイドの音量値)
	bool	play86_flag;							// 発音中?flag
	
	int32_t		AVolume;
//	static	Sample VolumeTable[16][256];			// 音量テーブル
	Sample VolumeTable[16][256];					// 音量テーブル
	
	void	_Init(void);							// 初期化(内部処理)
	void	MakeVolumeTable(int32_t volume);
	void 	ReadHeader(IFILEIO* file, P86HEADER &p86header);
	void	double_trans(Sample* dest, int32_t nsamples);
	void	double_trans_g(Sample* dest, int32_t nsamples);
	void	left_trans(Sample* dest, int32_t nsamples);
	void	left_trans_g(Sample* dest, int32_t nsamples);
	void	right_trans(Sample* dest, int32_t nsamples);
	void	right_trans_g(Sample* dest, int32_t nsamples);
	void	double_trans_i(Sample* dest, int32_t nsamples);
	void	double_trans_g_i(Sample* dest, int32_t nsamples);
	void	left_trans_i(Sample* dest, int32_t nsamples);
	void	left_trans_g_i(Sample* dest, int32_t nsamples);
	void	right_trans_i(Sample* dest, int32_t nsamples);
	void	right_trans_g_i(Sample* dest, int32_t nsamples);
	bool	add_address(void);
};

#endif	// P86DRV_H
