//=============================================================================
//		SSG PCM Driver 「PPSDRV」 Unit
//			Original Programmed	by NaoNeko.
//			Modified		by Kaja.
//			Windows Converted by C60
//=============================================================================

#include <stdlib.h>
#include <math.h>
#include "ppsdrv.h"

//-----------------------------------------------------------------------------
//	コンストラクタ・デストラクタ
//-----------------------------------------------------------------------------
PPSDRV::PPSDRV(IFILEIO* pfileio)
{
	this->pfileio = pfileio;
	pfileio->AddRef();

	dataarea1 = NULL;
	_Init();
}


PPSDRV::~PPSDRV()
{
	if(dataarea1 != NULL) {
		free(dataarea1);		// メモリ開放
	}
	
	pfileio->Release();
}


//-----------------------------------------------------------------------------
//	File Stream 設定
//-----------------------------------------------------------------------------
void PPSDRV::setfileio(IFILEIO* pfileio) {
	if (this->pfileio != NULL) {
		this->pfileio->Release();
	}

	this->pfileio = pfileio;
	if (this->pfileio != NULL) {
		this->pfileio->AddRef();
	}
}


//-----------------------------------------------------------------------------
//	初期化
//-----------------------------------------------------------------------------
bool PPSDRV::Init(uint32_t r, bool ip)
{
	_Init();
	SetRate(r, ip);
	return true;
}


//-----------------------------------------------------------------------------
//	初期化(内部処理)
//-----------------------------------------------------------------------------
void PPSDRV::_Init(void)
{
	memset(&ppsheader, 0, sizeof(PPSHEADER));
	memset(pps_file, 0, sizeof(pps_file));

	interpolation = false;
	rate = SOUND_44K;			// 再生周波数

	if (dataarea1 != NULL) {
		free(dataarea1);		// メモリ開放
		dataarea1 = NULL;
	}

	single_flag = false;
	//	single_flag = true;		// ２重再生禁止(@暫定)
	low_cpu_check_flag = false;
	keyon_flag = false;
	data_offset1 = NULL;
	data_offset2 = NULL;
	data_xor1 = 0;
	data_xor2 = 0;
	tick1 = 0;
	tick2 = 0;
	tick_xor1 = 0;
	tick_xor2 = 0;
	data_size1 = 0;
	data_size2 = 0;
	volume1 = 0;
	volume2 = 0;
	keyoff_vol = 0;

	//	psg.SetClock(3993600 / 4, SOUND_44K);
	//	psg.SetReg(0x07, 4);						// Tone C のみ
	SetVolume(-10);
}


//-----------------------------------------------------------------------------
//	00H PDR 停止
//-----------------------------------------------------------------------------
bool PPSDRV::Stop(void)
{
	keyon_flag = false;
	data_offset1 = data_offset2 = NULL;
	data_size1 = data_size2 = 0;
//	psg.SetReg(0x0a, 0);
	return true;
}


//-----------------------------------------------------------------------------
//	01H PDR 再生
//-----------------------------------------------------------------------------
bool PPSDRV::Play(int32_t num, int32_t shift, int32_t volshift)
{
	int32_t		al;

	if(ppsheader.pcmnum[num].address == 0) return false;

	al = 225 + ppsheader.pcmnum[num].toneofs;
	al = al % 256;

	if(shift < 0) {
		if((al += shift) <= 0) {
			al = 1;
		}
	} else {
		if((al += shift) > 255) {
			al = 255;
		}
	}

	if(ppsheader.pcmnum[num].volumeofs + volshift >= 15) return false;
			// 音量が０以下の時は再生しない

	if(single_flag == false && keyon_flag) {
		//	２重発音処理
		volume2 = volume1;					// １音目を２音目に移動
		data_offset2 = data_offset1;
		data_size2 = data_size1;
		data_xor2 = data_xor1;
		tick2 = tick1;
		tick_xor2 = tick_xor1;
	} else {
		//	１音目で再生
		data_size2 = 0;						// ２音目は停止中
	}

	volume1 = ppsheader.pcmnum[num].volumeofs + volshift;
	data_offset1
			= &dataarea1[(ppsheader.pcmnum[num].address - PPSHEADERSIZE) * 2];
	data_size1 = ppsheader.pcmnum[num].leng * 2;	// １音目を消して再生
	data_xor1 = 0;
	if(low_cpu_check_flag) {
		tick1 = ((8000 * al / 225) << 16) / rate;
		tick_xor1 = tick1 & 0xffff;
		tick1 >>= 16;
	} else {
		tick1 = ((16000 * al / 225) << 16) / rate;
		tick_xor1 = tick1 & 0xffff;
		tick1 >>= 16;
	}

//	psg.SetReg(0x07, psg.GetReg(0x07) | 0x24);	// Tone/Noise C off
	keyon_flag = true;						// 発音開始
	return true;
}


//-----------------------------------------------------------------------------
//	再生中かどうかのチェック
//-----------------------------------------------------------------------------
bool PPSDRV::Check(void)
{
	return keyon_flag;
}


//-----------------------------------------------------------------------------
//	ヘッダ読み込み
//-----------------------------------------------------------------------------
void PPSDRV::ReadHeader(IFILEIO* file, PPSHEADER &ppsheader)
{
	uint8_t buf[84];
	file->Read(buf, sizeof(buf));

	for(int32_t i = 0; i < MAX_PPS; i++) {
		ppsheader.pcmnum[i].address = 	buf[i * 6    ] | (buf[i * 6 + 1] << 8);
		ppsheader.pcmnum[i].leng =		buf[i * 6 + 2] | (buf[i * 6 + 3] << 8);
		ppsheader.pcmnum[i].toneofs =	buf[i * 6 + 4];
		ppsheader.pcmnum[i].volumeofs =	buf[i * 6 + 5];
	}
}


//-----------------------------------------------------------------------------
//	PPS 読み込み
//-----------------------------------------------------------------------------
int32_t PPSDRV::Load(TCHAR *filename)
{
	int32_t			i, size;
	uint32_t	j, start_pps, end_pps;
	PPSHEADER	ppsheader2;
	uint8_t		*psrc, *psrc2;
	Sample		*pdst;

	Stop();

	filepath.Clear(pps_file, sizeof(pps_file)/sizeof(TCHAR));

	if(*filename == filepath.EmptyChar) {
		return _ERR_OPEN_PPS_FILE;
	}

	if(pfileio->Open(filename, FileIO::flags_readonly) == false) {
		if(dataarea1 != NULL) {
			free(dataarea1);		// 開放
			dataarea1 = NULL;
			memset(&ppsheader, 0, sizeof(ppsheader));
		}
		return _ERR_OPEN_PPS_FILE;						//	ファイルが開けない
	}

	size = (int32_t)pfileio->GetFileSize(filename);		// ファイルサイズ
	//@ pfileio->Read(&ppsheader2, sizeof(ppsheader2));
	ReadHeader(pfileio, ppsheader2);

	if(memcmp(&ppsheader, &ppsheader2, sizeof(ppsheader)) == 0) {
		filepath.Strcpy(pps_file, filename);
		pfileio->Close();
		return _WARNING_PPS_ALREADY_LOAD;		// 同じファイル
	}

	if(dataarea1 != NULL) {
		free(dataarea1);		// いったん開放
		dataarea1 = NULL;
	}

	memcpy(&ppsheader, &ppsheader2, sizeof(ppsheader));

	size -= PPSHEADERSIZE;

	if((pdst = dataarea1
		= (Sample *)malloc(size * sizeof(Sample) * 2 / sizeof(uint8_t))) == NULL) {
		pfileio->Close();
		return _ERR_OUT_OF_MEMORY;			// メモリが確保できない
	}

	if((psrc = psrc2 = (uint8_t*)malloc(size)) == NULL) {
		pfileio->Close();
		return _ERR_OUT_OF_MEMORY;			// メモリが確保できない（テンポラリ）
	}

	//	仮バッファに読み込み
	pfileio->Read(psrc, size);

	for(i = 0; i < size / (int32_t)sizeof(uint8_t); i++) {
		*pdst = (*psrc) / 16;
		pdst++;
		*pdst = (*psrc) & 0x0f;
		pdst++;
		psrc++;
	}


	//	PPS 補正(プチノイズ対策）／160 サンプルで減衰させる
	for(i = 0; i < MAX_PPS; i++) {
		end_pps = ppsheader.pcmnum[i].address - PPSHEADERSIZE * 2
			+ ppsheader.pcmnum[i].leng * 2;
		start_pps = end_pps - 160;
		if(start_pps < ppsheader.pcmnum[i].address - PPSHEADERSIZE * 2) {
			start_pps = ppsheader.pcmnum[i].address - PPSHEADERSIZE * 2;
		}

		for(j = start_pps; j < end_pps; j++) {
			dataarea1[j] = dataarea1[j] - (j - start_pps) * 16 / (end_pps - start_pps);
			if(dataarea1[j] < 0) dataarea1[j] = 0;
		}
	}

	//	仮バッファ開放
	free(psrc2);

	//	ファイル名登録
	filepath.Strcpy(pps_file, filename);

	pfileio->Close();

	data_offset1 = data_offset2 = NULL;

	return _PPSDRV_OK;
}


//-----------------------------------------------------------------------------
//	05H PDRパラメータの設定
//-----------------------------------------------------------------------------
bool PPSDRV::SetParam(int32_t paramno, bool data)
{
	switch(paramno) {
		case 0 :
			single_flag = data;
			return true;
		case 1 :
			low_cpu_check_flag = data;
			return true;
		default : return false;
	}
}


//-----------------------------------------------------------------------------
//	06H PDRパラメータの取得
//-----------------------------------------------------------------------------
bool PPSDRV::GetParam(int32_t paramno)
{
	switch(paramno) {
		case 0 : return single_flag;
		case 1 : return low_cpu_check_flag;
		default : return false;
	}
}


//-----------------------------------------------------------------------------
//	再生周波数、一次補完設定設定
//-----------------------------------------------------------------------------
bool PPSDRV::SetRate(uint32_t r, bool ip)		// レート設定
{
	rate = r;
	interpolation = ip;
	return true;
}


//-----------------------------------------------------------------------------
//	音量設定
//-----------------------------------------------------------------------------
void PPSDRV::SetVolume(int32_t vol)					// 音量設定
{
//	psg.SetVolume(vol);

	double base = 0x4000 * 2 / 3.0 * pow(10.0, vol / 40.0);
	for (int32_t i=15; i>=1; i--)
	{
		EmitTable[i] = int32_t(base);
		base /= 1.189207115;
	}
	EmitTable[0] = 0;
}


//-----------------------------------------------------------------------------
//	合成
//-----------------------------------------------------------------------------
void PPSDRV::Mix(Sample* dest, int32_t nsamples)	// 合成
{
	int32_t		i, al1, al2, ah1, ah2;
	Sample	data;

/*
	static const int32_t table[16*16] = {
		 0, 0, 0, 5, 9,10,11,12,13,13,14,14,14,15,15,15,
		 0, 0, 3, 5, 9,10,11,12,13,13,14,14,14,15,15,15,
		 0, 3, 5, 7, 9,10,11,12,13,13,14,14,14,15,15,15,
		 5, 5, 7, 9,10,11,12,13,13,13,14,14,14,15,15,15,
		 9, 9, 9,10,11,12,12,13,13,14,14,14,15,15,15,15,
		10,10,10,11,12,12,13,13,13,14,14,14,15,15,15,15,
		11,11,11,12,12,13,13,13,14,14,14,14,15,15,15,15,
		12,12,12,12,13,13,13,14,14,14,14,15,15,15,15,15,
		13,13,13,13,13,13,14,14,14,14,14,15,15,15,15,15,
		13,13,13,13,14,14,14,14,14,14,15,15,15,15,15,15,
		14,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,
		14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15,
		14,14,14,14,15,15,15,15,15,15,15,15,15,15,15,15,
		15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
		15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
		15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15
	};
*/

	if(keyon_flag == false && keyoff_vol == 0) {
		return;
	}

	for(i = 0; i < nsamples; i++) {
		if(data_size1 > 1) {
			al1 = *data_offset1     - volume1;
			al2 = *(data_offset1+1) - volume1;
			if(al1 < 0) al1 = 0;
			if(al2 < 0) al2 = 0;
		} else {
			al1 = al2 = 0;
		}

		if(data_size2 > 1) {
			ah1 = *data_offset2     - volume2;
			ah2 = *(data_offset2+1) - volume2;
			if(ah1 < 0) ah1 = 0;
			if(ah2 < 0) ah2 = 0;
		} else {
			ah1 = ah2 = 0;
		}

//		al1 = table[(al1 << 4) + ah1];
//		psg.SetReg(0x0a, al1);
		if(interpolation) {
			data =
				(EmitTable[al1] * (0x10000 - data_xor1) + EmitTable[al2] * data_xor1 +
				 EmitTable[ah1] * (0x10000 - data_xor2) + EmitTable[ah2] * data_xor2) / 0x10000;
		} else {
			data = EmitTable[al1] + EmitTable[ah1];
		}

		keyoff_vol = (keyoff_vol * 255) / 256;
		data += keyoff_vol;
		*dest++ += data;
		*dest++ += data;

//		psg.Mix(dest, 1);
//		dest += 2;

		if(data_size2 > 1) {	// ２音合成再生
			data_xor2 += tick_xor2;
			if(data_xor2 >= 0x10000) {
				data_size2--;
				data_offset2++;
				data_xor2 -= 0x10000;
			}
			data_size2 -= tick2;
			data_offset2 += tick2;
			
			if(low_cpu_check_flag) {
				data_xor2 += tick_xor2;
				if(data_xor2 >= 0x10000) {
					data_size2--;
					data_offset2++;
					data_xor2 -= 0x10000;
				}
				data_size2 -= tick2;
				data_offset2 += tick2;
			}
		}

		data_xor1 += tick_xor1;
		if(data_xor1 >= 0x10000) {
			data_size1--;
			data_offset1++;
			data_xor1 -= 0x10000;
		}
		data_size1 -= tick1;
		data_offset1 += tick1;

		if(low_cpu_check_flag) {
			data_xor1 += tick_xor1;
			if(data_xor1 >= 0x10000) {
				data_size1--;
				data_offset1++;
				data_xor1 -= 0x10000;
			}
			data_size1 -= tick1;
			data_offset1 += tick1;
		}

		if(data_size1 <= 1 && data_size2 <= 1) {		// 両方停止
			if(keyon_flag) {
				keyoff_vol += EmitTable[data_offset1[data_size1-1]];
			}
			keyon_flag = false;		// 発音停止
//			psg.SetReg(0x0a, 0);	// Volume を0に
//			return;
		} else if(data_size1 <= 1 && data_size2 > 1) {	// １音目のみが停止
			volume1 = volume2;
			data_size1 = data_size2;
			data_offset1 = data_offset2;
			data_xor1 = data_xor2;
			tick1 = tick2;
			tick_xor1 = tick_xor2;
			data_size2 = 0;
/*
			if(data_offset2 != NULL) {
				keyoff_vol += EmitTable[data_offset1[data_size1-1]];
			}
*/
		} else if(data_size1 > 1 && data_size2 < 1) {	// ２音目のみが停止
			if(data_offset2 != NULL) {
				keyoff_vol += EmitTable[data_offset2[data_size2-1]];
				data_offset2 = NULL;
			}
		}
	}
}
