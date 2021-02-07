//=============================================================================
//		86B PCM Driver 「P86DRV」 Unit
//			programmed by M.Kajihara 96/01/16
//			Windows Converted by C60
//=============================================================================

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>
#include	"p86drv.h"
#include	"util.h"

//-----------------------------------------------------------------------------
//	コンストラクタ・デストラクタ
//-----------------------------------------------------------------------------
P86DRV::P86DRV()
{
	rate = SOUND_44K;			// 再生周波数
	p86_addr = NULL;			// P86 buffer
	interpolation = false;
	
	memset(&p86header, 0, sizeof(p86header));
	memset(p86_file, 0, sizeof(p86_file));

	srcrate = ratetable[3];		// 16.54kHz
	rate = ratetable[7];		// 44.1kHz
	pcm86_pan_flag = false;		// パンデータ１(bit0=左/bit1=右/bit2=逆)
	pcm86_pan_dat = 0;			// パンデータ２(音量を下げるサイドの音量値)
	
	start_ofs = NULL;
	start_ofs_x = 0;
	size = 0;
	_start_ofs = NULL;
	_size = 0;
	addsize1 = 0;
	addsize2 = 0;
	ontei = 0;
	repeat_ofs = NULL;
	repeat_size = 0;
	release_ofs = NULL;
	release_size = 0;
	repeat_flag = false;
	release_flag1 = false;
	release_flag2 = false;
	play86_flag = false;

	SetVolume(0);
}


P86DRV::~P86DRV()
{
	if(p86_addr != NULL) {
		free(p86_addr);			// メモリ開放
	}
}


//-----------------------------------------------------------------------------
//	初期化
//-----------------------------------------------------------------------------
bool P86DRV::Init(uint r, bool ip)
{
	SetVolume(0);
	SetRate(r, ip);
	return true;
}


//-----------------------------------------------------------------------------
//	再生周波数、一次補完設定設定
//-----------------------------------------------------------------------------
bool P86DRV::SetRate(uint r, bool ip)
{
	uint	_ontei;
	
	rate = r;
	interpolation = ip;

	_ontei = (uint)((unsigned long long)ontei * srcrate / rate);
	addsize2 = (_ontei & 0xffff) >> 4;
	addsize1 = _ontei >> 16;

	return true;
}


//-----------------------------------------------------------------------------
//	音量調整用
//-----------------------------------------------------------------------------
void P86DRV::SetVolume(int volume)
{
	MakeVolumeTable(volume);
}

int P86DRV::read_char(void *value)
{
	int temp;
	
	if ((*(uchar *)value) & 0x80)
		temp = -1;
	else
		temp = 0;

	memcpy(&temp,value,sizeof(char));
		
	return temp;
}


//-----------------------------------------------------------------------------
//	音量テーブル作成
//-----------------------------------------------------------------------------
void P86DRV::MakeVolumeTable(int volume)
{
	int		i, j;
	int		AVolume_temp;
	double	temp;
	
	AVolume_temp = (int)(0x1000 * pow(10.0, volume / 40.0));
	if(AVolume != AVolume_temp) {
		AVolume = AVolume_temp;
		for(i = 0; i < 16; i++) {
			temp = pow(2.0, (i + 15) / 2.0) * AVolume / 0x18000;
			for(j = 0; j < 256; j++) {
				VolumeTable[i][j] = (Sample)(read_char(&j) * temp);
			}
		}
	}
}


//-----------------------------------------------------------------------------
//	P86 読み込み
//-----------------------------------------------------------------------------
int P86DRV::Load(char *filename)
{
	FILE		*fp;
	int			i, size;
	P86HEADER	_p86header;
	P86HEADER2	p86header2;
	
	Stop();

	p86_file[0] = '\0';	
	if((fp = fopen(filename, "rb")) == NULL) {
		if(p86_addr != NULL) {
			free(p86_addr);		// 開放
			p86_addr = NULL;
			memset(&p86header, 0, sizeof(p86header));
		}
		return _ERR_OPEN_P86_FILE;						//	ファイルが開けない
	}
	
		size = (int)GetFileSize_s(filename);		// ファイルサイズ
		fread(&_p86header, 1, sizeof(_p86header), fp);
		
		// P86HEADER → P86HEADER2 へ変換
		memset(&p86header2, 0, sizeof(p86header2));
		
		for(i = 0; i < MAX_P86; i++) {
			p86header2.pcmnum[i].start = 
				_p86header.pcmnum[i].start[0] +
				_p86header.pcmnum[i].start[1] * 0x100 + 
				_p86header.pcmnum[i].start[2] * 0x10000 - 0x610;
			p86header2.pcmnum[i].size = 
				_p86header.pcmnum[i].size[0] +
				_p86header.pcmnum[i].size[1] * 0x100 + 
				_p86header.pcmnum[i].size[2] * 0x10000;
		}
		
		if(memcmp(&p86header, &p86header2, sizeof(p86header)) == 0) {
			strcpy(p86_file, filename);
			return _WARNING_P86_ALREADY_LOAD;		// 同じファイル
		}
		
		if(p86_addr != NULL) {
			free(p86_addr);		// いったん開放
			p86_addr = NULL;
		}
		
		memcpy(&p86header, &p86header2, sizeof(p86header));
		
		size -= sizeof(_p86header);

		if((p86_addr = (uchar *)malloc(size)) == NULL) {
			return _ERR_OUT_OF_MEMORY;			// メモリが確保できない
		}

		fread(p86_addr, size, 1, fp);
	
		//	ファイル名登録
		strcpy(p86_file, filename);
	
		fclose(fp);

	return _P86DRV_OK;
}


//-----------------------------------------------------------------------------
//	PCM 番号設定
//-----------------------------------------------------------------------------
bool P86DRV::SetNeiro(int num)
{
	_start_ofs = p86_addr + p86header.pcmnum[num].start;
	_size = p86header.pcmnum[num].size;
	repeat_flag = false;
	release_flag1 = false;
	return true;
}


//-----------------------------------------------------------------------------
//	PAN 設定
//-----------------------------------------------------------------------------
bool P86DRV::SetPan(int flag, int data)
{
	pcm86_pan_flag = flag;
	pcm86_pan_dat = data;
	return true;	
}


//-----------------------------------------------------------------------------
//	音量設定
//-----------------------------------------------------------------------------
bool P86DRV::SetVol(int _vol)
{
	vol = _vol;
	return true;
}


//-----------------------------------------------------------------------------
//	音程周波数の設定
//		_srcrate : 入力データの周波数
//			0 : 4.13kHz
//			1 : 5.52kHz
//			2 : 8.27kHz
//			3 : 11.03kHz
//			4 : 16.54kHz
//			5 : 22.05kHz
//			6 : 33.08kHz
//			7 : 44.1kHz
//		_ontei : 設定音程
//-----------------------------------------------------------------------------
bool P86DRV::SetOntei(int _srcrate, uint _ontei)
{
	if(_srcrate < 0 || _srcrate > 7) return false;
	if(_ontei > 0x1fffff) return false;

	ontei = _ontei;
	srcrate = ratetable[_srcrate];
	
	_ontei = (uint)((unsigned long long)_ontei * srcrate / rate);

	addsize2 = (_ontei & 0xffff) >> 4;
	addsize1 = _ontei >> 16;
	return true;
}


//-----------------------------------------------------------------------------
//	リピート設定
//-----------------------------------------------------------------------------
bool P86DRV::SetLoop(int loop_start, int loop_end, int release_start, bool adpcm)
{
	int		ax, dx, _dx;

	repeat_flag = true;
	release_flag1 = false;
	dx = _dx = _size;
	
	
	// 一個目 = リピート開始位置
	ax = loop_start;
	if(ax >= 0) {
		// 正の場合
		if(adpcm) ax *= 32;
		if(ax >= _size - 1) ax = _size - 2;		// アクセス違反対策
		if(ax < 0) ax = 0;

		repeat_size = _size - ax;		// リピートサイズ＝全体のサイズ-指定値
		repeat_ofs = _start_ofs + ax;	// リピート開始位置から指定値を加算
	} else {
		// 負の場合
		ax = -ax;
		if(adpcm) ax *= 32;
		dx -= ax;
		if(dx < 0) {							// アクセス違反対策
			ax = _dx;
			dx = 0;
		}
		
		repeat_size = ax;	// リピートサイズ＝neg(指定値)
		repeat_ofs = _start_ofs + dx;	//リピート開始位置に(全体サイズ-指定値)を加算
	
	}
	
	// ２個目 = リピート終了位置
	ax = loop_end;
	if(ax > 0) {
		// 正の場合
		if(adpcm) ax *= 32;
		if(ax >= _size - 1) ax = _size - 2;		// アクセス違反対策
		if(ax < 0) ax = 0;

		_size = ax;
		dx -= ax;
		// リピートサイズから(旧サイズ-新サイズ)を引く
		repeat_size -= dx;
	} else if(ax < 0) {
		// 負の場合
		ax = -ax;
		if(adpcm) ax *= 32;
		if(ax > repeat_size) ax = repeat_size;
		repeat_size -= ax;	// リピートサイズからneg(指定値)を引く
		_size -= ax;			// 本来のサイズから指定値を引く
	}
	
	// ３個目 = リリース開始位置
	ax = release_start;
	if((ushort)ax != 0x8000) {				// 8000Hなら設定しない
		// release開始位置 = start位置に設定
		release_ofs = _start_ofs;
		
		// release_size = 今のsizeに設定
		release_size = _dx;
		
		// リリースするに設定
		release_flag1 = true;
		if(ax > 0) {
			// 正の場合
			if(adpcm) ax *= 32;
			if(ax >= _size - 1) ax = _size - 2;		// アクセス違反対策
			if(ax < 0) ax = 0;
			
			// リースサイズ＝全体のサイズ-指定値
			release_size -= ax;
			
			// リリース開始位置から指定値を加算
			release_ofs += ax;
		} else {
			// 負の場合
			ax = -ax;
			if(adpcm) ax *= 32;
			if(ax > _size) ax = _size;
			
			// リリースサイズ＝neg(指定値)
			release_size = ax;
			
			_dx -= ax;
			
			// リリース開始位置に(全体サイズ-指定値)を加算
			release_ofs += _dx;
		}
	}
	return true;
}
	
	
//-----------------------------------------------------------------------------
//	P86 再生
//-----------------------------------------------------------------------------
bool P86DRV::Play(void)
{
	start_ofs = _start_ofs;
	start_ofs_x = 0;
	size = _size;

	play86_flag = true;
	release_flag2 = false;
	return true;
}


//-----------------------------------------------------------------------------
//	P86 停止
//-----------------------------------------------------------------------------
bool P86DRV::Stop(void)
{
	play86_flag = false;
	return true;
}


//-----------------------------------------------------------------------------
//	P86 keyoff
//-----------------------------------------------------------------------------
bool P86DRV::Keyoff(void)
{
	if(release_flag1 == true) {		// リリースが設定されているか?
		start_ofs = release_ofs;
		size = release_size;
		release_flag2 = true;		// リリースした
	} else {
		play86_flag = false;
	}
	return true;
}


//-----------------------------------------------------------------------------
//	合成
//-----------------------------------------------------------------------------
void P86DRV::Mix(Sample* dest, int nsamples)
{
	if(play86_flag == false) return;
	if(size <= 1) {		// 一次補間対策
		play86_flag = false;
		return;	
	}

//	double_trans(dest, nsamples); return;		// @test

	if(interpolation) {
		switch(pcm86_pan_flag) {
			case 0 : double_trans_i(dest, nsamples); break;
			case 1 : left_trans_i(dest, nsamples); break;
			case 2 : right_trans_i(dest, nsamples); break;
			case 3 : double_trans_i(dest, nsamples); break;
			case 4 : double_trans_g_i(dest, nsamples); break;
			case 5 : left_trans_g_i(dest, nsamples); break;
			case 6 : right_trans_g_i(dest, nsamples); break;
			case 7 : double_trans_g_i(dest, nsamples); break;
		}
	} else {
		switch(pcm86_pan_flag) {
			case 0 : double_trans(dest, nsamples); break;
			case 1 : left_trans(dest, nsamples); break;
			case 2 : right_trans(dest, nsamples); break;
			case 3 : double_trans(dest, nsamples); break;
			case 4 : double_trans_g(dest, nsamples); break;
			case 5 : left_trans_g(dest, nsamples); break;
			case 6 : right_trans_g(dest, nsamples); break;
			case 7 : double_trans_g(dest, nsamples); break;
		}
	}
}


//-----------------------------------------------------------------------------
//	真ん中（一次補間あり）
//-----------------------------------------------------------------------------
void P86DRV::double_trans_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	真ん中（逆相、一次補間あり）
//-----------------------------------------------------------------------------
void P86DRV::double_trans_g_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;


	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	左寄り（一次補間あり）
//-----------------------------------------------------------------------------
void P86DRV::left_trans_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	左寄り（逆相、一次補間あり）
//-----------------------------------------------------------------------------
void P86DRV::left_trans_g_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	右寄り（一次補間あり）
//-----------------------------------------------------------------------------
void P86DRV::right_trans_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	右寄り（逆相、一次補間あり）
//-----------------------------------------------------------------------------
void P86DRV::right_trans_g_i(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = (VolumeTable[vol][*start_ofs] * (0x1000-start_ofs_x)
				+ VolumeTable[vol][*(start_ofs+1)] * start_ofs_x) >> 12;
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	真ん中（一次補間なし）
//-----------------------------------------------------------------------------
void P86DRV::double_trans(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	真ん中（逆相、一次補間なし）
//-----------------------------------------------------------------------------
void P86DRV::double_trans_g(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;


	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	左寄り（一次補間なし）
//-----------------------------------------------------------------------------
void P86DRV::left_trans(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	左寄り（逆相、一次補間なし）
//-----------------------------------------------------------------------------
void P86DRV::left_trans_g(Sample* dest, int nsamples)
{
	int		i;
	Sample	data;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		*dest++ += data;
		data = data * pcm86_pan_dat / (256 / 2);
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	右寄り（一次補間なし）
//-----------------------------------------------------------------------------
void P86DRV::right_trans(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ += data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	右寄り（逆相、一次補間なし）
//-----------------------------------------------------------------------------
void P86DRV::right_trans_g(Sample* dest, int nsamples)
{
	int		i;
	Sample	data, data2;

	for(i = 0; i < nsamples; i++) {
		data = VolumeTable[vol][*start_ofs];
		data2 = data * pcm86_pan_dat / (256 / 2);
		*dest++ += data2;
		*dest++ -= data;

		if(add_address()) {
			play86_flag = false;
			return;
		}
	}
}


//-----------------------------------------------------------------------------
//	アドレス加算
//-----------------------------------------------------------------------------
bool P86DRV::add_address(void)
{
	start_ofs_x += addsize2;
	if(start_ofs_x >= 0x1000) {
		start_ofs_x -= 0x1000;
		start_ofs++;
		size--;
	}
	start_ofs += addsize1;
	size -= addsize1;
	
	if(size > 1) {		// 一次補間対策
		return false;
	} else if(repeat_flag == false || release_flag2) {
		return true;
	}
	
	size = repeat_size;
	start_ofs = repeat_ofs;	
	return false;
}
