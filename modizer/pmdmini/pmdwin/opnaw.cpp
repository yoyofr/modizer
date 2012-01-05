//=============================================================================
//		opnaw : OPNA unit with wait
//			ver 0.01
//=============================================================================

#ifdef HAVE_WINDOWS_H
# include	<windows.h>
#endif
#include	<stdio.h>
#include	<string.h>
#include	<math.h>
#include	"opnaw.h"
#include	"ppz8l.h"
#include	"util.h"
#include	"misc.h"


namespace FM {

//-----------------------------------------------------------------------------
//	コンストラクタ・デストラクタ
//-----------------------------------------------------------------------------
OPNAW::OPNAW()
{
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
	memset(pre_buffer, 0, sizeof(pre_buffer));
}

OPNAW::~OPNAW()
{
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
	return OPNA::SetRate(c, r, ipflag);
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

// ---------------------------------------------------------------------------
//	レジスタアレイにデータを設定
//-----------------------------------------------------------------------------
void OPNAW::SetReg(uint addr, uint data)
{
	if(addr < 0x10) {					// SSG
		if(ssgwaitcount) {
			CalcWaitPCM(ssgwaitcount);
		}
	} else if((addr % 100) <= 0x10) {	// ADPCM
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

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
//	合成
//-----------------------------------------------------------------------------
void OPNAW::Mix(Sample* buffer, int nsamples)
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

// ---------------------------------------------------------------------------
//	内部バッファクリア
//-----------------------------------------------------------------------------
void OPNAW::ClearBuffer(void)
{
	read_pos = write_pos = 0;
}

}
