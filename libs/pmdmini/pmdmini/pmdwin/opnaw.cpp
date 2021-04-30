//=============================================================================
//		opnaw : OPNA unit with wait
//			ver 0.04
//=============================================================================

#if defined _WIN32
#include	<windows.h>
#else
#include	<cstdint>
typedef std::int64_t _int64;
#endif
#include	<stdio.h>
#include	<string.h>
#include	<math.h>
#include	"opnaw.h"
#include	"ppz8l.h"
#include	"util.h"
#include	"misc.h"

#define		M_PI	3.14159265358979323846



//-----------------------------------------------------------------------------
//	このユニットで線形補間を行う場合に宣言する
//	fmgen 007 で線形補間が廃止されたため追加
//-----------------------------------------------------------------------------
#define INTERPOLATION_IN_THIS_UNIT


namespace FM {

//-----------------------------------------------------------------------------
//	コンストラクタ・デストラクタ
//-----------------------------------------------------------------------------
OPNAW::OPNAW()
{
	_Init();
}


OPNAW::~OPNAW()
{
}


//-----------------------------------------------------------------------------
//	初期化
//-----------------------------------------------------------------------------
bool OPNAW::Init(uint c, uint r, bool ipflag, const TCHAR* path)
{
	_Init();
	rate2 = r;
	
#ifdef INTERPOLATION_IN_THIS_UNIT
	if(ipflag) {
		return OPNA::Init(c, SOUND_55K_2, false, path);
	} else {
		return OPNA::Init(c, r, false, path);
	}
#else
	return OPNA::Init(c, r, ipflag, path);
#endif
}


//-----------------------------------------------------------------------------
//	初期化(内部処理)
//-----------------------------------------------------------------------------
void OPNAW::_Init()
{
	memset(pre_buffer, 0, sizeof(pre_buffer));
	
	fmwait = 0;
	ssgwait = 0;
	rhythmwait = 0;
	adpcmwait = 0;
	
	fmwaitcount = 0;
	ssgwaitcount = 0;
	rhythmwaitcount = 0;
	adpcmwaitcount = 0;
	
	read_pos = 0;
	write_pos = 0;
	count2 = 0;
	
	memset(ip_buffer, 0, sizeof(ip_buffer));
	rate2 = 0;
	interpolation2 = false;
	delta = 0;
}


//-----------------------------------------------------------------------------
//	サンプリングレート変更
//-----------------------------------------------------------------------------
bool OPNAW::SetRate(uint c, uint r, bool ipflag)
{
	SetFMWait(fmwait);
	SetSSGWait(ssgwait);
	SetRhythmWait(rhythmwait);
	SetADPCMWait(adpcmwait);
	
	interpolation2 = ipflag;
	rate2 = r;
	
#ifdef INTERPOLATION_IN_THIS_UNIT
	if(ipflag) {
		return OPNA::SetRate(c, SOUND_55K_2, false);
	} else {
		return OPNA::SetRate(c, r, false);
	}
#else
	return OPNA::SetRate(c, r, ipflag);
#endif
	fmwaitcount = fmwait * rate / 1000000;
	ssgwaitcount = ssgwait * rate / 1000000;
	rhythmwaitcount = rhythmwait * rate / 1000000;
	adpcmwaitcount = adpcmwait * rate / 1000000;
}


//-----------------------------------------------------------------------------
//	Wait 設定
//-----------------------------------------------------------------------------
void OPNAW::SetFMWait(int nsec)
{
	fmwait = nsec;
	fmwaitcount = nsec * rate / 1000000;
}

void OPNAW::SetSSGWait(int nsec)
{
	ssgwait = nsec;
	ssgwaitcount = nsec * rate / 1000000;
}

void OPNAW::SetRhythmWait(int nsec)
{
	rhythmwait = nsec;
	rhythmwaitcount = nsec * rate / 1000000;
}

void OPNAW::SetADPCMWait(int nsec)
{
	adpcmwait = nsec;
	adpcmwaitcount = nsec * rate / 1000000;
}


//-----------------------------------------------------------------------------
//	Wait 取得
//-----------------------------------------------------------------------------
int OPNAW::GetFMWait(void)
{
	return fmwait;
}

int OPNAW::GetSSGWait(void)
{
	return ssgwait;
}

int OPNAW::GetRhythmWait(void)
{
	return rhythmwait;
}

int OPNAW::GetADPCMWait(void)
{
	return adpcmwait;
}


//-----------------------------------------------------------------------------
//	レジスタアレイにデータを設定
//-----------------------------------------------------------------------------
void OPNAW::SetReg(uint addr, uint data)
{
	if(addr < 0x10) {					// SSG
		if(ssgwaitcount) {
			CalcWaitPCM(ssgwaitcount);
		}
	} else if((addr % 0x100) <= 0x10) {	// ADPCM
		if(adpcmwaitcount) {
			CalcWaitPCM(adpcmwaitcount);
		}
	} else if(addr < 0x20) {			// RHYTHM
		if(rhythmwaitcount) {
			CalcWaitPCM(rhythmwaitcount);
		}
	} else {							// FM
		if(fmwaitcount) {
			CalcWaitPCM(fmwaitcount);
		}
	}
	
	OPNA::SetReg(addr, data);
}


//-----------------------------------------------------------------------------
//	SetReg() wait 時の PCM を計算
//-----------------------------------------------------------------------------
void OPNAW::CalcWaitPCM(int waitcount)
{
	int		outsamples;
	
	count2 += waitcount % 1000;
	waitcount /= 1000;
	if(count2 > 1000) {
		waitcount++;
		count2 -= 1000;
	}
	
	do {
		if(write_pos + waitcount > WAIT_PCM_BUFFER_SIZE) {
			outsamples = WAIT_PCM_BUFFER_SIZE - write_pos;
		} else {
			outsamples = waitcount;
		}
		
		memset(&pre_buffer[write_pos * 2], 0, outsamples * 2 * sizeof(Sample));
		OPNA::Mix(&pre_buffer[write_pos * 2], outsamples);
		write_pos += outsamples;
		if(write_pos == WAIT_PCM_BUFFER_SIZE) {
			write_pos = 0;
		}
		waitcount -= outsamples;
	} while (waitcount > 0);
}


//-----------------------------------------------------------------------------
//	合成（一次補間なしVer.)
//-----------------------------------------------------------------------------
void OPNAW::_Mix(Sample* buffer, int nsamples)
{
	int		bufsamples;
	int		outsamples;
	int		i;
	
	if(read_pos != write_pos) {			// buffer から出力
		if(read_pos < write_pos) {
			bufsamples = write_pos - read_pos;
		} else {
			bufsamples = write_pos - read_pos + WAIT_PCM_BUFFER_SIZE;
		}
		if(bufsamples > nsamples) {
			bufsamples = nsamples;
		}
		
		do {
			if(read_pos + bufsamples > WAIT_PCM_BUFFER_SIZE) {
				outsamples = WAIT_PCM_BUFFER_SIZE - read_pos;
			} else {
				outsamples = bufsamples;
			}
			
			for(i = 0; i < outsamples * 2; i++) {
				*buffer += pre_buffer[read_pos * 2 + i];
				buffer++;
			}
			
//			memcpy(buffer, &pre_buffer[read_pos * 2], outsamples * 2 * sizeof(Sample));
			read_pos += outsamples;
			if(read_pos == WAIT_PCM_BUFFER_SIZE) {
				read_pos = 0;
			}
			nsamples -= outsamples;
			bufsamples -= outsamples;
		} while (bufsamples > 0);
	}
	
	OPNA::Mix(buffer, nsamples);
}


//-----------------------------------------------------------------------------
//	合成
//-----------------------------------------------------------------------------
void OPNAW::Mix(Sample* buffer, int nsamples)
{

#ifdef INTERPOLATION_IN_THIS_UNIT
	int	nmixdata2;
	
	if(interpolation2) {
		while(nsamples > 0) {
			int nmixdata = (int)(delta + ((_int64)nsamples) * (SOUND_55K_2 * 16384 / rate2)) / 16384;
			if(nmixdata > (IP_PCM_BUFFER_SIZE - 1)) {
				int snsamples = (IP_PCM_BUFFER_SIZE - 2) * rate2 / SOUND_55K_2;
				nmixdata = (delta + (snsamples) * (SOUND_55K_2 * 16384 / rate2)) / 16384;
			}
			memset(&ip_buffer[2], 0, sizeof(Sample) * 2 * nmixdata);
			_Mix(&ip_buffer[2], nmixdata);
			
			nmixdata2 = 0;
			while(nmixdata > nmixdata2) {
				*buffer++ += (ip_buffer[nmixdata2*2    ] * (16384 - delta) + ip_buffer[nmixdata2*2+2] * delta) / 16384;
				*buffer++ += (ip_buffer[nmixdata2*2 + 1] * (16384 - delta) + ip_buffer[nmixdata2*2+3] * delta) / 16384;
				delta += SOUND_55K_2 * 16384 / rate2;
				nmixdata2 += delta / 16384;
				delta %= 16384;
				nsamples--;
			}
			ip_buffer[0] = ip_buffer[nmixdata * 2    ];
			ip_buffer[1] = ip_buffer[nmixdata * 2 + 1];
		}
	} else {
		_Mix(buffer, nsamples);
	}
#else
	_Mix(buffer, nsamples);
#endif
}


//-----------------------------------------------------------------------------
//	内部バッファクリア
//-----------------------------------------------------------------------------
void OPNAW::ClearBuffer(void)
{
	read_pos = write_pos = 0;
	memset(pre_buffer, 0, sizeof(pre_buffer));
	memset(ip_buffer, 0, sizeof(ip_buffer));
}


}
