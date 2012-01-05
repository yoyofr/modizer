//=============================================================================
//		86B PCM Driver 「P86DRV」 Unit
//			programmed by M.Kajihara 96/01/16
//			Windows Converted by C60
//=============================================================================

#ifndef P86DRV_H
#define P86DRV_H

#ifdef HAVE_WINDOWS_H
# include <windows.h>
#endif
#include "types.h"
#include "compat.h"

//	DLL の 戻り値
#define	_P86DRV_OK					  0		// 正常終了
#define	_ERR_OPEN_P86_FILE			  1		// P86 を開けなかった
#define	_ERR_WRONG_P86_FILE		 	  2		// P86 の形式が異なっている
#define	_WARNING_P86_ALREADY_LOAD	  3		// P86 はすでに読み込まれている
#define	_ERR_OUT_OF_MEMORY			 99		// メモリを確保できなかった

#define	SOUND_44K				  44100
#define	SOUND_22K				  22050
#define	SOUND_11K				  11025

#define	P86DRV_VER				 "1.1c"
#define	MAX_P86						256
#define vers					   0x11
#define date			"Sep.11th 1996"


typedef int				Sample;
typedef	unsigned char	uchar;
typedef	unsigned int	uint;


#ifdef WINDOWS
#pragma pack( push, enter_include1 )
#pragma pack(1)
#endif

typedef struct p86headertag		// header(original)
{
	char	header[12];			// "PCM86 DATA",0,0
	uchar	Version;
	char	All_Size[3];
	struct {
		uchar	start[3];
		uchar	size[3];
	} __PACKED__ pcmnum[MAX_P86];
} __PACKED__ P86HEADER;

#ifdef WINDOWS
#pragma pack( pop, enter_include1 )
#endif

typedef struct p86headertag2	// header(for PMDWin, int alignment)
{
	char	header[12];			// "PCM86 DATA",0,0
	int		Version;
	int		All_Size;
	struct {
		int		start;
		int		size;
	} pcmnum[MAX_P86];
} P86HEADER2;


const int ratetable[] = {
	4135, 5513, 8270, 11025, 16540, 22050, 33080, 44100
};


class P86DRV
{
public:
	P86DRV();
	~P86DRV();
	
	bool Init(uint r, bool ip);						// 初期化
	bool Stop(void);								// P86 停止
	bool Play(void);								// P86 再生
	bool Keyoff(void);								// P86 keyoff
	int Load(char *filename);						// P86 読み込み
	bool SetRate(uint r, bool ip);					// レート設定
	void SetVolume(int volume);						// 全体音量調節用
	bool SetVol(int _vol);							// 音量設定
	bool SetOntei(int rate, uint ontei);			// 音程周波数の設定
	bool SetPan(int flag, int data);				// PAN 設定
	bool SetNeiro(int num);							// PCM 番号設定
	bool SetLoop(int loop_start, int loop_end, int release_start, bool adpcm);
													// ループ設定
	void Mix(Sample* dest, int nsamples);			// 合成
	char	p86_file[_MAX_PATH];					// ファイル名
	P86HEADER2 p86header;							// P86 の音色ヘッダー

private:
	bool	interpolation;							// 補完するか？
	int		rate;									// 再生周波数
	int		srcrate;								// 元データの周波数
	uint	ontei;									// 音程(fnum)
	int		vol;									// 音量
	uchar	*p86_addr;								// P86 保存用メモリポインタ
	uchar	*start_ofs;								// 発音中PCMデータ番地
	int		start_ofs_x;							// 発音中PCMデータ番地（小数部）
	int		size;									// 残りサイズ
	uchar	*_start_ofs;							// 発音開始PCMデータ番地
	int		_size;									// PCMデータサイズ
	int		addsize1;								// PCMアドレス加算値 (整数部)
	int		addsize2;								// PCMアドレス加算値 (小数部)
	uchar	*repeat_ofs;							// リピート開始位置
	int		repeat_size;							// リピート後のサイズ
	uchar	*release_ofs;							// リリース開始位置
	int		release_size;							// リリース後のサイズ
	bool	repeat_flag;							// リピートするかどうかのflag
	bool	release_flag1;							// リリースするかどうかのflag
	bool	release_flag2;							// リリースしたかどうかのflag

	int		pcm86_pan_flag;		// パンデータ１(bit0=左/bit1=右/bit2=逆)
	int		pcm86_pan_dat;		// パンデータ２(音量を下げるサイドの音量値)
	bool	play86_flag;							// 発音中?flag

	int		AVolume;
//	static	Sample VolumeTable[16][256];			// 音量テーブル
	Sample VolumeTable[16][256];					// 音量テーブル

	int     read_char(void *value);
	void	MakeVolumeTable(int volume);
	void	double_trans(Sample* dest, int nsamples);
	void	double_trans_g(Sample* dest, int nsamples);
	void	left_trans(Sample* dest, int nsamples);
	void	left_trans_g(Sample* dest, int nsamples);
	void	right_trans(Sample* dest, int nsamples);
	void	right_trans_g(Sample* dest, int nsamples);
	void	double_trans_i(Sample* dest, int nsamples);
	void	double_trans_g_i(Sample* dest, int nsamples);
	void	left_trans_i(Sample* dest, int nsamples);
	void	left_trans_g_i(Sample* dest, int nsamples);
	void	right_trans_i(Sample* dest, int nsamples);
	void	right_trans_g_i(Sample* dest, int nsamples);
	bool	add_address(void);
};

#endif	// P86DRV_H
