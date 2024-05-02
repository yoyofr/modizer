//=============================================================================
//		8 Channel PCM Driver 「PPZ8」 Unit(Light Version)
//			Programmed by UKKY
//			Windows Converted by C60
//=============================================================================

#include	<algorithm>
#include	<math.h>
#include	"ppz8l.h"
#include	"pmdmini_file.h"
#include	"misc.h"


//-----------------------------------------------------------------------------
//	定数テーブル(ADPCM Volume → PPZ8 Volume)
//-----------------------------------------------------------------------------
const int ADPCM_EM_VOL[256] = {
	 0, 0, 0, 0, 0, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4,
	 4, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6,
	 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
	10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
	10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
	
};


//-----------------------------------------------------------------------------
//	定数テーブル(ADPCM Pan → PPZ8 Pan)
//-----------------------------------------------------------------------------
const int ADPCM_EM_PAN[4] = {
	0,9,1,5
};


//-----------------------------------------------------------------------------
//	コンストラクタ・デストラクタ
//-----------------------------------------------------------------------------
PPZ8::PPZ8()
{
	XMS_FRAME_ADR[0] = NULL;	// XMSで確保したメモリアドレス（リニア）
	XMS_FRAME_ADR[1] = NULL;	// XMSで確保したメモリアドレス（リニア）
	_Init();
}

PPZ8::~PPZ8()
{
	if(XMS_FRAME_ADR[0] != NULL) {
		free(XMS_FRAME_ADR[0]);
	}
	
	if(XMS_FRAME_ADR[1] != NULL) {
		free(XMS_FRAME_ADR[1]);
	}
}


//-----------------------------------------------------------------------------
//	00H 初期化
//-----------------------------------------------------------------------------
bool PPZ8::Init(uint rate, bool ip)
{
	_Init();
	return SetRate(rate, ip);
}


//-----------------------------------------------------------------------------
//	初期化(内部処理)
//-----------------------------------------------------------------------------
void PPZ8::_Init(void)
{
	memset(PCME_WORK, 0, sizeof(PCME_WORK));
	pviflag[0] = false;
	pviflag[1] = false;
	memset(PVI_FILE, 0, sizeof(PVI_FILE));
	
	ADPCM_EM_FLG = false;
	interpolation = false;
	
	WORK_INIT();
	
	// 一旦開放する
	if(XMS_FRAME_ADR[0] != NULL) {
		free(XMS_FRAME_ADR[0]);
		XMS_FRAME_ADR[0] = NULL;
	}
	XMS_FRAME_SIZE[0] = 0;
	
	if(XMS_FRAME_ADR[1] != NULL) {
		free(XMS_FRAME_ADR[1]);
		XMS_FRAME_ADR[1] = NULL;
	}
	XMS_FRAME_SIZE[1] = 0;
	
	PCM_VOLUME = 0;
	volume = 0;
	SetAllVolume(VNUM_DEF);		// 全体ボリューム(DEF=12)
	
	DIST_F = RATE_DEF;	 		// 再生周波数
}


//-----------------------------------------------------------------------------
//	01H PCM 発音
//-----------------------------------------------------------------------------
bool PPZ8::Play(int ch, int bufnum, int num, WORD start, WORD stop)
{
	if(ch >= PCM_CNL_MAX) return false;
	if(XMS_FRAME_ADR[bufnum] == NULL || XMS_FRAME_SIZE[bufnum] == 0) return false;
	
	// PVIの定義数より大きいとスキップ
	//if(num >= PCME_WORK[bufnum].pzinum) return false;
	
	channelwork[ch].pviflag = pviflag[bufnum];
	channelwork[ch].PCM_FLG = 1;		// 再生開始
	channelwork[ch].PCM_NOW_XOR = 0;	// 小数点部
	channelwork[ch].PCM_NUM = num;
	
	// ADPCM エミュレート処理
	if(ch == 7 && ADPCM_EM_FLG && (ch & 0x80) == 0) {
		channelwork[ch].PCM_NOW 
			= &XMS_FRAME_ADR[bufnum][Limit(((int)start) * 64, XMS_FRAME_SIZE[bufnum] - 1, 0)];
		channelwork[ch].PCM_END_S 
			= &XMS_FRAME_ADR[bufnum][Limit(((int)stop - 1) * 64, XMS_FRAME_SIZE[bufnum] - 1, 0)];
		channelwork[ch].PCM_END = channelwork[ch].PCM_END_S;
	} else {
		channelwork[ch].PCM_NOW
			= &XMS_FRAME_ADR[bufnum][PCME_WORK[bufnum].pcmnum[num].startaddress];
		channelwork[ch].PCM_END_S
			= &XMS_FRAME_ADR[bufnum][PCME_WORK[bufnum].pcmnum[num].startaddress + PCME_WORK[bufnum].pcmnum[num].size];
		
		if(channelwork[ch].PCM_LOOP_FLG == 0) {
			// ループなし
			channelwork[ch].PCM_END = channelwork[ch].PCM_END_S;
		} else {
			// ループあり
			if(channelwork[ch].PCM_LOOP_START >= PCME_WORK[bufnum].pcmnum[num].size) {
				channelwork[ch].PCM_LOOP
					= &XMS_FRAME_ADR[bufnum][PCME_WORK[bufnum].pcmnum[num].startaddress + PCME_WORK[bufnum].pcmnum[num].size - 1];
			} else {
				channelwork[ch].PCM_LOOP
					= &XMS_FRAME_ADR[bufnum][PCME_WORK[bufnum].pcmnum[num].startaddress + channelwork[ch].PCM_LOOP_START];
			}
			
			if(channelwork[ch].PCM_LOOP_END >= PCME_WORK[bufnum].pcmnum[num].size) {
				channelwork[ch].PCM_END
					= &XMS_FRAME_ADR[bufnum][PCME_WORK[bufnum].pcmnum[num].startaddress + PCME_WORK[bufnum].pcmnum[num].size];
			} else {
				channelwork[ch].PCM_END
					= &XMS_FRAME_ADR[bufnum][PCME_WORK[bufnum].pcmnum[num].startaddress + channelwork[ch].PCM_LOOP_END];
			}
		}
	}
	
	return true;
}


//-----------------------------------------------------------------------------
//	02H PCM 停止
//-----------------------------------------------------------------------------
bool PPZ8::Stop(int ch)
{
	if(ch >= PCM_CNL_MAX) return false;
	
	channelwork[ch].PCM_FLG = 0;	// 再生停止
	return true;
}


//-----------------------------------------------------------------------------
//	03H PVI/PZIﾌｧｲﾙの読み込み
//-----------------------------------------------------------------------------
int PPZ8::Load(TCHAR *filename, int bufnum)
{
	static const int table1[16] = {
		  1,   3,   5,   7,   9,  11,  13,  15,
		 -1,  -3,  -5,  -7,  -9, -11, -13, -15,
	};
	static const int table2[16] = {
		 57,  57,  57,  57,  77, 102, 128, 153,
		 57,  57,  57,  57,  77, 102, 128, 153,
	};
	
	FileIO		file;
	int			i, j, size, size2;
	uchar		*psrc, *psrc2;
	uchar		*pdst;
	bool		NOW_PCM_CATE;					// 現在のPCMの形式(true : PZI)
	PZIHEADER		pziheader;
	PVIHEADER		pviheader;
	int		X_N;								// Xn     (ADPCM>PCM 変換用)
	int		DELTA_N;							// DELTA_N(ADPCM>PCM 変換用)
	
	const TCHAR	*p = filepath.Strrchr(filename, '.');
	if(p != NULL) {
		if(filepath.Comparepath(filename, _T(".PZI"), FilePath::extractpath_ext) == 0) {
			NOW_PCM_CATE = true;
		} else {
			NOW_PCM_CATE = false;
		}
	} else {
		NOW_PCM_CATE = true;					// とりあえず pzi
	}
	
	WORK_INIT();								// ワーク初期化
	
	filepath.Clear(PVI_FILE[bufnum], sizeof(PVI_FILE[bufnum])/sizeof(TCHAR));
	if(*filename == filepath.EmptyChar) {
		return _ERR_OPEN_PPZ_FILE;
	}
	
	if(file.Open(filename, FileIO::readonly) == false) {
		if(XMS_FRAME_ADR[bufnum] != NULL) {
			free(XMS_FRAME_ADR[bufnum]);		// 開放
			XMS_FRAME_ADR[bufnum] = NULL;
			XMS_FRAME_SIZE[bufnum] = 0;
			memset(&PCME_WORK[bufnum], 0, sizeof(PZIHEADER));
		}
		return _ERR_OPEN_PPZ_FILE;				//	ファイルが開けない
	}
	
	size = (int)filepath.GetFileSize(filename);	// ファイルサイズ
	
	if(NOW_PCM_CATE) {	// PZI 読み込み
		file.Read(&pziheader, sizeof(PZIHEADER));
		
		if(memcmp(&PCME_WORK[bufnum], &pziheader, sizeof(PZIHEADER)) == 0) {
			filepath.Strcpy(PVI_FILE[bufnum], filename);
			file.Close();
			return _WARNING_PPZ_ALREADY_LOAD;		// 同じファイル
		}
		
		if(XMS_FRAME_ADR[bufnum] != NULL) {
			free(XMS_FRAME_ADR[bufnum]);		// いったん開放
			XMS_FRAME_ADR[bufnum] = NULL;
			XMS_FRAME_SIZE[bufnum] = 0;
			memset(&PCME_WORK[bufnum], 0, sizeof(PZIHEADER));
		}
		
		if(strncmp(pziheader.header, "PZI", 3)) {
			file.Close();
			return _ERR_WRONG_PPZ_FILE;		// データ形式が違う
		}
		
		memcpy(&PCME_WORK[bufnum], &pziheader, sizeof(PZIHEADER));
		
		size -= sizeof(PZIHEADER);
		
		if((pdst = XMS_FRAME_ADR[bufnum]
						= (uchar *)malloc(size)) == NULL) {
			file.Close();
			return _ERR_OUT_OF_MEMORY;			// メモリが確保できない
		}
		
		//	読み込み
		file.Read(pdst, size);
		XMS_FRAME_SIZE[bufnum] = size;
		
		//	ファイル名登録
		filepath.Strcpy(PVI_FILE[bufnum], filename);
		pviflag[bufnum] = false;
		
	} else {			// PVI 読み込み
		file.Read(&pviheader, sizeof(PVIHEADER));
		
		strncpy(pziheader.header, "PZI1", 4);
		size2 = 0;
		
//	20020311 修正
		for(i = 0; i < pviheader.pvinum; i++) {
			pziheader.pcmnum[i].startaddress = (pviheader.pcmnum[i].startaddress << (5+1));
			pziheader.pcmnum[i].size
				= ((pviheader.pcmnum[i].endaddress - pviheader.pcmnum[i].startaddress + 1
				) << (5+1));
			size2 += pziheader.pcmnum[i].size;
			
			pziheader.pcmnum[i].loop_start = 0xffffffff;
			pziheader.pcmnum[i].loop_end = 0xffffffff;
			pziheader.pcmnum[i].rate  = 16000;	// 16kHz
		}
		
		for(i = pviheader.pvinum; i < 128; i++) {
			pziheader.pcmnum[i].startaddress = 0;
			pziheader.pcmnum[i].size = 0;
			pziheader.pcmnum[i].loop_start = 0xffffffff;
			pziheader.pcmnum[i].loop_end = 0xffffffff;
			pziheader.pcmnum[i].rate  = 16000;	// 16kHz
		}
		
		if(memcmp(&PCME_WORK[bufnum]. pcmnum, &pziheader.pcmnum, sizeof(PZIHEADER)-0x20) == 0) {
			filepath.Strcpy(PVI_FILE[bufnum], filename);
			file.Close();
			return _WARNING_PPZ_ALREADY_LOAD;		// 同じファイル
		}
		
		if(XMS_FRAME_ADR[bufnum] != NULL) {
			free(XMS_FRAME_ADR[bufnum]);		// いったん開放
			XMS_FRAME_ADR[bufnum] = NULL;
			XMS_FRAME_SIZE[bufnum] = 0;
			memset(&PCME_WORK[bufnum], 0, sizeof(PZIHEADER));
		}
		
		if(strncmp(pviheader.header, "PVI", 3)) {
			file.Close();
			return _ERR_WRONG_PPZ_FILE;		// データ形式が違う
		}
		
		memcpy(&PCME_WORK[bufnum], &pziheader, sizeof(PZIHEADER));
		
		size -= sizeof(PVIHEADER);
		
//	20020922 修正
//@		size2 = size2 / 2 - sizeof(PVIHEADER);		// PVI換算
		size2 = size2 / 2;							// PVI換算
		
		
		if((pdst = XMS_FRAME_ADR[bufnum]
						= (uchar *)malloc(std::max(size, size2) * 2)) == NULL) {
			file.Close();
			return _ERR_OUT_OF_MEMORY;			// メモリが確保できない
		}
		
		if((psrc = psrc2 = (uchar *)malloc(std::max(size, size2))) == NULL) {
			file.Close();
			return _ERR_OUT_OF_MEMORY;			// メモリが確保できない（テンポラリ）
		}
		
		//	仮バッファに読み込み
		file.Read(psrc, size);
		
		// ADPCM > PCM に変換
		for(i = 0; i < pviheader.pvinum; i++) {
			X_N = X_N0;
			DELTA_N = DELTA_N0;
			
			for(j = 0; j < (int)pziheader.pcmnum[i].size / 2; j++) {
				
				X_N = Limit(X_N + table1[(*psrc >> 4) & 0x0f] * DELTA_N / 8, 32767, -32768);
				DELTA_N = Limit(DELTA_N * table2[(*psrc >> 4) & 0x0f] / 64, 24576, 127);
				*pdst++ = X_N / (32768 / 128) + 128;
				
				X_N = Limit(X_N + table1[*psrc & 0x0f] * DELTA_N / 8, 32767, -32768);
				DELTA_N = Limit(DELTA_N * table2[*psrc++ & 0x0f] / 64, 24576, 127);
				*pdst++ = X_N / (32768 / 128) + 128;
			}
		}
		XMS_FRAME_SIZE[bufnum] = size2 * 2;
		
		//	バッファ開放
		free(psrc2);
		
		//	ファイル名登録
		filepath.Strcpy(PVI_FILE[bufnum], filename);
		
		pviflag[bufnum] = true;
		
	}
	
	file.Close();
	
	return _PPZ8_OK;
}


//-----------------------------------------------------------------------------
//	07H ボリューム設定
//-----------------------------------------------------------------------------
bool PPZ8::SetVol(int ch, int vol)
{
	if(ch >= PCM_CNL_MAX) return false;
	
	if(ch != 7 || !ADPCM_EM_FLG) {
		channelwork[ch].PCM_VOL = vol;
	} else {
		channelwork[ch].PCM_VOL = ADPCM_EM_VOL[vol & 0xff];
	}
	return true;
}


//-----------------------------------------------------------------------------
//	0BH 音程周波数の設定
//-----------------------------------------------------------------------------
bool PPZ8::SetOntei(int ch, uint ontei)
{
	if(ch >= PCM_CNL_MAX) return false;
	if(ch == 7 && ADPCM_EM_FLG) {						// ADPCM エミュレート中
		ontei = (ontei & 0xffff) * 0x8000 / 0x49ba;
	}
	
	channelwork[ch].PCM_ADDS_L = ontei & 0xffff;
	channelwork[ch].PCM_ADDS_H = ontei >> 16;
	
	channelwork[ch].PCM_ADD_H = (int)(
			(_int64)((channelwork[ch].PCM_ADDS_H << 16) + channelwork[ch].PCM_ADDS_L)
			* 2 * channelwork[ch].PCM_SORC_F / DIST_F);
	channelwork[ch].PCM_ADD_L = channelwork[ch].PCM_ADD_H & 0xffff;
	channelwork[ch].PCM_ADD_H = channelwork[ch].PCM_ADD_H >> 16;
	
	return true;
}


//-----------------------------------------------------------------------------
//	0EH ﾙｰﾌﾟﾎﾟｲﾝﾀの設定
//-----------------------------------------------------------------------------
bool PPZ8::SetLoop(int ch, uint loop_start, uint loop_end)
{
	if(ch >= PCM_CNL_MAX) return false;
	
	if(loop_start != 0xffff && loop_end > loop_start) {
		// ループ設定
		// PCM_LPS_02:
		
		channelwork[ch].PCM_LOOP_FLG = 1;
		channelwork[ch].PCM_LOOP_START = loop_start;
		channelwork[ch].PCM_LOOP_END = loop_end;
	} else {
		// ループ解除
		// PCM_LPS_01:
		
		channelwork[ch].PCM_LOOP_FLG = 0;
		channelwork[ch].PCM_END = channelwork[ch].PCM_END_S;
	}
	return true;
}


//-----------------------------------------------------------------------------
//	12H (PPZ8)全停止
//-----------------------------------------------------------------------------
void PPZ8::AllStop(void)
{
	int		i;
	
	// とりあえず各パート停止で対応
	for(i = 0; i < PCM_CNL_MAX; i++) {
		Stop(i);
	}
}


//-----------------------------------------------------------------------------
//	13H (PPZ8)PAN指定
//-----------------------------------------------------------------------------
bool PPZ8::SetPan(int ch, int pan)
{
	if(ch >= PCM_CNL_MAX) return false;
	
	if(ch != 7 || !ADPCM_EM_FLG) {
		channelwork[ch].PCM_PAN = pan;
	} else {
		channelwork[ch].PCM_PAN = ADPCM_EM_PAN[pan & 3];
	}
	return true;
}


//-----------------------------------------------------------------------------
//	14H (PPZ8)ﾚｰﾄ設定
//-----------------------------------------------------------------------------
bool PPZ8::SetRate(uint rate, bool ip)
{
	DIST_F = rate;
	interpolation = ip;
	return true;
}


//-----------------------------------------------------------------------------
//	15H (PPZ8)元ﾃﾞｰﾀ周波数設定
//-----------------------------------------------------------------------------
bool PPZ8::SetSourceRate(int ch, int rate)
{
	if(ch >= PCM_CNL_MAX) return false;
	
	channelwork[ch].PCM_SORC_F = rate;
	return true;
}


//-----------------------------------------------------------------------------
//	16H (PPZ8)全体ﾎﾞﾘﾕｰﾑの設定（86B Mixer)
//-----------------------------------------------------------------------------
void PPZ8::SetAllVolume(int vol)
{
	if(vol < 16 && vol != PCM_VOLUME) {
		PCM_VOLUME = vol;
		MakeVolumeTable(volume);
	}
}


//-----------------------------------------------------------------------------
//	音量調整用
//-----------------------------------------------------------------------------
void PPZ8::SetVolume(int vol)
{
	if(vol != volume) {
		MakeVolumeTable(vol);
	}
}


//-----------------------------------------------------------------------------
//	ADPCM エミュレート設定
//-----------------------------------------------------------------------------
void PPZ8::ADPCM_EM_SET(bool flag)
{
	ADPCM_EM_FLG = flag;
}


//-----------------------------------------------------------------------------
//	音量テーブル作成
//-----------------------------------------------------------------------------
void PPZ8::MakeVolumeTable(int vol)
{
	int		i, j;
	double	temp;
	
	volume = vol;
	AVolume = (int)(0x1000 * pow(10.0, vol / 40.0));
	
	for(i = 0; i < 16; i++) {
		temp = pow(2.0, (i + PCM_VOLUME) / 2.0) * AVolume / 0x18000;
		for(j = 0; j < 256; j++) {
			VolumeTable[i][j] = (Sample)((j-128) * temp);
		}
	}
}


//-----------------------------------------------------------------------------
//	ﾜｰｸ初期化
//-----------------------------------------------------------------------------
void PPZ8::WORK_INIT(void)
{
	int		i;
	
	memset(channelwork, 0, sizeof(channelwork));
	
	for(i = 0; i < PCM_CNL_MAX; i++) {
		channelwork[i].PCM_ADD_H = 1;
		channelwork[i].PCM_ADD_L = 0;
		channelwork[i].PCM_ADDS_H = 1;
		channelwork[i].PCM_ADDS_L = 0;
		channelwork[i].PCM_SORC_F = 16000;		// 元データの再生レート
		channelwork[i].PCM_PAN = 5;			// PAN中心
		channelwork[i].PCM_VOL = 8;			// ボリュームデフォルト
	}
	
	// MOV	PCME_WORK0+PVI_NUM_MAX,0	;@ PVIのMAXを０にする
}


//-----------------------------------------------------------------------------
//	合成、出力
//-----------------------------------------------------------------------------
void PPZ8::Mix(Sample* dest, int nsamples)
{
	int		i;
	Sample	*di;
	Sample	bx;
	
	for(i = 0; i < PCM_CNL_MAX; i++) {
		if(PCM_VOLUME == 0) break;
		if(channelwork[i].PCM_FLG == 0) continue;
		if(channelwork[i].PCM_VOL == 0) {
			// PCM_NOW ポインタの移動(ループ、end 等も考慮して)
			channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L * nsamples;
			channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H * nsamples + channelwork[i].PCM_NOW_XOR / 0x10000;
			channelwork[i].PCM_NOW_XOR %= 0x10000;
			
			while(channelwork[i].PCM_NOW >= channelwork[i].PCM_END-1) {
				// @一次補間のときもちゃんと動くように実装        ^^
				if(channelwork[i].PCM_LOOP_FLG) {
					// ループする場合
					channelwork[i].PCM_NOW -= (channelwork[i].PCM_END - 1 - channelwork[i].PCM_LOOP);
				} else {
					channelwork[i].PCM_FLG = 0;
					break;
				}
			}
			continue;
		}
		if(channelwork[i].PCM_PAN == 0) {
			channelwork[i].PCM_FLG = 0;
			continue;
		}
		
		if(interpolation) {
			di = dest;
			switch(channelwork[i].PCM_PAN) {
				case 1 :	//  1 , 0
					while(di < &dest[nsamples * 2]) {
						*di++ += (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						di++;		// 左のみ
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 2 :	//  1 ,1/4
					while(di < &dest[nsamples * 2]) {
						bx = (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						*di++ += bx;
						*di++ += bx / 4;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 3 :	//  1 ,2/4
					while(di < &dest[nsamples * 2]) {
						bx = (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						*di++ += bx;
						*di++ += bx / 2;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 4 :	//  1 ,3/4
					while(di < &dest[nsamples * 2]) {
						bx = (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						*di++ += bx;
						*di++ += bx * 3 / 4;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 5 :	//  1 , 1
					while(di < &dest[nsamples * 2]) {
						bx = (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						*di++ += bx;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 6 :	// 3/4, 1
					while(di < &dest[nsamples * 2]) {
						bx = (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						*di++ += bx * 3 / 4;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 7 :	// 2/4, 1
					while(di < &dest[nsamples * 2]) {
						bx = (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						*di++ += bx / 2;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 8 :	// 1/4, 1
					while(di < &dest[nsamples * 2]) {
						bx = (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						*di++ += bx / 4;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 9 :	//  0 , 1
					while(di < &dest[nsamples * 2]) {
						
						di++;		// 右のみ
						*di++ += (VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW] * (0x10000-channelwork[i].PCM_NOW_XOR)
							+ VolumeTable[channelwork[i].PCM_VOL][*(channelwork[i].PCM_NOW+1)] * channelwork[i].PCM_NOW_XOR) >> 16;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END-1) {
							// @一次補間のときもちゃんと動くように実装   ^^
							
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
			}
		} else {
			di = dest;
			switch(channelwork[i].PCM_PAN) {
				case 1 :	//  1 , 0
					while(di < &dest[nsamples * 2]) {
						*di++ += VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						di++;		// 左のみ
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 2 :	//  1 ,1/4
					while(di < &dest[nsamples * 2]) {
						bx = VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						*di++ += bx;
						*di++ += bx / 4;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 3 :	//  1 ,2/4
					while(di < &dest[nsamples * 2]) {
						bx = VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						*di++ += bx;
						*di++ += bx / 2;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 4 :	//  1 ,3/4
					while(di < &dest[nsamples * 2]) {
						bx = VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						*di++ += bx;
						*di++ += bx * 3 / 4;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 5 :	//  1 , 1
					while(di < &dest[nsamples * 2]) {
						bx = VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						*di++ += bx;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 6 :	// 3/4, 1
					while(di < &dest[nsamples * 2]) {
						bx = VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						*di++ += bx * 3 / 4;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 7 :	// 2/4, 1
					while(di < &dest[nsamples * 2]) {
						bx = VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						*di++ += bx / 2;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 8 :	// 1/4, 1
					while(di < &dest[nsamples * 2]) {
						bx = VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						*di++ += bx / 4;
						*di++ += bx;
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
					
				case 9 :	//  0 , 1
					while(di < &dest[nsamples * 2]) {
						di++;			// 右のみ
						*di++ += VolumeTable[channelwork[i].PCM_VOL][*channelwork[i].PCM_NOW];
						
						channelwork[i].PCM_NOW += channelwork[i].PCM_ADD_H;
						channelwork[i].PCM_NOW_XOR += channelwork[i].PCM_ADD_L;
						if(channelwork[i].PCM_NOW_XOR > 0xffff) {
							channelwork[i].PCM_NOW_XOR -= 0x10000;
							channelwork[i].PCM_NOW++;
						}
						
						if(channelwork[i].PCM_NOW>=channelwork[i].PCM_END) {
							if(channelwork[i].PCM_LOOP_FLG) {
								// ループする場合
								channelwork[i].PCM_NOW
										= channelwork[i].PCM_LOOP;
							} else {
								channelwork[i].PCM_FLG = 0;
								break;
							}
						}
					}
					break;
			}
		}
	}
}


/*
//-----------------------------------------------------------------------------
//	テーブル
//-----------------------------------------------------------------------------
Sample PPZ8::VolumeTable[16][256] = {0,};
*/
