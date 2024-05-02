// ---------------------------------------------------------------------------
//	PSG Sound Implementation
//	Copyright (C) cisc 1997, 1999.
// ---------------------------------------------------------------------------
//	$Id: psg.cpp,v 1.10 2002/05/15 21:38:01 cisc Exp $

#include "headers.h"
#include "misc.h"
#include "psg.h"

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR


// ---------------------------------------------------------------------------
//	コンストラクタ・デストラクタ
//
PSG::PSG()
{
    /*
     EmitTable[0x20] = { -1, };
     enveloptable[16][64] = { 0, };
     */
    
    memset(reg, 0, sizeof(reg));
    envelop = NULL;
    olevel[0] = olevel[1] = olevel[2] = 0;
    scount[0] = scount[1] = scount[2] = 0;
    speriod[0] = speriod[1] = speriod[2] = 0;
    ecount = eperiod = 0;
    ncount = nperiod = 0;
    tperiodbase = 0;
    eperiodbase = 0;
    nperiodbase = 0;
    volume = 0;
    mask = 0;
    
    SetVolume(0);
    MakeNoiseTable();
    Reset();
    mask = 0x3f;
}

PSG::~PSG()
{
    
}

// ---------------------------------------------------------------------------
//	PSG を初期化する(RESET)
//
void PSG::Reset()
{
    for (int i=0; i<14; i++)
        SetReg(i, 0);
    SetReg(7, 0xff);
    SetReg(14, 0xff);
    SetReg(15, 0xff);
}

// ---------------------------------------------------------------------------
//	クロック周波数の設定
//
void PSG::SetClock(int clock, int rate)
{
    tperiodbase = int((1 << toneshift ) / 4.0 * clock / rate);
    eperiodbase = int((1 << envshift  ) / 4.0 * clock / rate);
    nperiodbase = int((1 << noiseshift) / 4.0 * clock / rate);
    
    //yoyofr
    psgclock=clock;
    psgrate=rate;
    //yoyofr
    
    // 各データの更新
    int tmp;
    tmp = ((reg[0] + reg[1] * 256) & 0xfff);
    speriod[0] = tmp ? tperiodbase / tmp : tperiodbase;
    tmp = ((reg[2] + reg[3] * 256) & 0xfff);
    speriod[1] = tmp ? tperiodbase / tmp : tperiodbase;
    tmp = ((reg[4] + reg[5] * 256) & 0xfff);
    speriod[2] = tmp ? tperiodbase / tmp : tperiodbase;
    tmp = reg[6] & 0x1f;
    nperiod = tmp ? nperiodbase / tmp / 2 : nperiodbase / 2;
    tmp = ((reg[11] + reg[12] * 256) & 0xffff);
    eperiod = tmp ? eperiodbase / tmp : eperiodbase * 2;
}

// ---------------------------------------------------------------------------
//	ノイズテーブルを作成する
//
void PSG::MakeNoiseTable()
{
    if (!noisetable[0])
    {
        int noise = 14321;
        for (int i=0; i<noisetablesize; i++)
        {
            int n = 0;
            for (int j=0; j<32; j++)
            {
                n = n * 2 + (noise & 1);
                noise = (noise >> 1) | (((noise << 14) ^ (noise << 16)) & 0x10000);
            }
            noisetable[i] = n;
        }
    }
}

// ---------------------------------------------------------------------------
//	出力テーブルを作成
//	素直にテーブルで持ったほうが省スペース。
//
void PSG::SetVolume(int volume)
{
    double base = 0x4000 / 3.0 * pow(10.0, volume / 40.0);
    for (int i=31; i>=2; i--)
    {
        EmitTable[i] = int(base);
        base /= 1.189207115;
    }
    EmitTable[1] = 0;
    EmitTable[0] = 0;
    MakeEnvelopTable();
    
    SetChannelMask(~mask);
}

void PSG::SetChannelMask(int c)
{
    mask = ~c;
    for (int i=0; i<3; i++)
        olevel[i] = mask & (1 << i) ? EmitTable[(reg[8+i] & 15) * 2 + 1] : 0;
}

// ---------------------------------------------------------------------------
//	エンベロープ波形テーブル
//
void PSG::MakeEnvelopTable()
{
    // 0 lo  1 up 2 down 3 hi
    static uint8 table1[16*2] =
    {
        2,0, 2,0, 2,0, 2,0, 1,0, 1,0, 1,0, 1,0,
        2,2, 2,0, 2,1, 2,3, 1,1, 1,3, 1,2, 1,0,
    };
    static uint8 table2[4] = {  0,  0, 31, 31 };
    static uint8 table3[4] = {  0,  1, (uint8)-1,  0 };
    
    uint* ptr = enveloptable[0];
    
    for (int i=0; i<16*2; i++)
    {
        uint8 v = table2[table1[i]];
        
        for (int j=0; j<32; j++)
        {
            *ptr++ = EmitTable[v & 0x1f];
            v += table3[table1[i]];
        }
    }
}

// ---------------------------------------------------------------------------
//	PSG のレジスタに値をセットする
//	regnum		レジスタの番号 (0 - 15)
//	data		セットする値
//
void PSG::SetReg(uint regnum, uint8 data)
{
    if (regnum < 0x10)
    {
        reg[regnum] = data;
        switch (regnum)
        {
                int tmp;
                
            case 0:		// ChA Fine Tune
            case 1:		// ChA Coarse Tune
                tmp = ((reg[0] + reg[1] * 256) & 0xfff);
                speriod[0] = tmp ? tperiodbase / tmp : tperiodbase;
                break;
                
            case 2:		// ChB Fine Tune
            case 3:		// ChB Coarse Tune
                tmp = ((reg[2] + reg[3] * 256) & 0xfff);
                speriod[1] = tmp ? tperiodbase / tmp : tperiodbase;
                break;
                
            case 4:		// ChC Fine Tune
            case 5:		// ChC Coarse Tune
                tmp = ((reg[4] + reg[5] * 256) & 0xfff);
                speriod[2] = tmp ? tperiodbase / tmp : tperiodbase;
                break;
                
            case 6:		// Noise generator control
                data &= 0x1f;
                nperiod = data ? nperiodbase / data : nperiodbase;
                break;
                
            case 8:
                olevel[0] = mask & 1 ? EmitTable[(data & 15) * 2 + 1] : 0;
                break;
                
            case 9:
                olevel[1] = mask & 2 ? EmitTable[(data & 15) * 2 + 1] : 0;
                break;
                
            case 10:
                olevel[2] = mask & 4 ? EmitTable[(data & 15) * 2 + 1] : 0;
                break;
                
            case 11:	// Envelop period
            case 12:
                tmp = ((reg[11] + reg[12] * 256) & 0xffff);
                eperiod = tmp ? eperiodbase / tmp : eperiodbase * 2;
                break;
                
            case 13:	// Envelop shape
                ecount = 0;
                envelop = enveloptable[data & 15];
                break;
        }
    }
}

// ---------------------------------------------------------------------------
//
//
inline void PSG::StoreSample(Sample& dest, int32 data)
{
    if (sizeof(Sample) == 2)
        dest = (Sample) Limit(dest + data, 0x7fff, -0x8000);
    else
        dest += data;
}

// ---------------------------------------------------------------------------
//	PCM データを吐き出す(2ch)
//	dest		PCM データを展開するポインタ
//	nsamples	展開する PCM のサンプル数
//
void PSG::Mix(Sample* dest, int nsamples)
{
    uint8 chenable[3], nenable[3];
    uint8 r7 = ~reg[7];
    
    if ((r7 & 0x3f) | ((reg[8] | reg[9] | reg[10]) & 0x1f))
    {
        chenable[0] = (r7 & 0x01) && (speriod[0] <= (1 << toneshift));
        chenable[1] = (r7 & 0x02) && (speriod[1] <= (1 << toneshift));
        chenable[2] = (r7 & 0x04) && (speriod[2] <= (1 << toneshift));
        nenable[0]  = (r7 >> 3) & 1;
        nenable[1]  = (r7 >> 4) & 1;
        nenable[2]  = (r7 >> 5) & 1;
        
        int noise, sample;
        uint env;
        uint* p1 = ((mask & 1) && (reg[ 8] & 0x10)) ? &env : &olevel[0];
        uint* p2 = ((mask & 2) && (reg[ 9] & 0x10)) ? &env : &olevel[1];
        uint* p3 = ((mask & 4) && (reg[10] & 0x10)) ? &env : &olevel[2];
        
#define SCOUNT(ch)	(scount[ch] >> (toneshift+oversampling))
        
        if (p1 != &env && p2 != &env && p3 != &env)
        {
            // エンベロープ無し
            if ((r7 & 0x38) == 0)
            {
                // ノイズ無し
                for (int i=0; i<nsamples; i++)
                {
                    sample = 0;
                    //YOYOFR
                    int chanout[3];
                    chanout[0]=chanout[1]=chanout[2]=0;
                    //YOYOFR
                    for (int j=0; j < (1 << oversampling); j++)
                    {
                        int x, y, z;
                        x = (SCOUNT(0) & chenable[0]) - 1;
                        sample += (olevel[0] + x) ^ x;
                        chanout[0] += (olevel[0] + x) ^ x;//yoyofr
                        scount[0] += speriod[0];
                        y = (SCOUNT(1) & chenable[1]) - 1;
                        sample += (olevel[1] + y) ^ y;
                        chanout[1] += (olevel[0] + x) ^ x;//yoyofr
                        scount[1] += speriod[1];
                        z = (SCOUNT(2) & chenable[2]) - 1;
                        sample += (olevel[2] + z) ^ z;
                        chanout[2] += (olevel[0] + x) ^ x;//yoyofr
                        scount[2] += speriod[2];
                    }
                    sample /= (1 << oversampling);
                    StoreSample(dest[0], sample);
                    StoreSample(dest[1], sample);
                    dest += 2;
                    
                    //TODO:  MODIZER changes start / YOYOFR
                    int64_t smplIncr;
                    if (psgrate) smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/psgrate;
                    else smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/44100;
                    int voice_ofs=6;
                    for (int jj=0;jj<3;jj++) {
                        chanout[jj] /= (1 << oversampling);
                        int64_t ofs_start=m_voice_current_ptr[jj+voice_ofs];
                        int64_t ofs_end=(m_voice_current_ptr[jj+voice_ofs]+smplIncr);
                        if ( (ofs_end>ofs_start) ) {
                            int val=chanout[jj];
                            for (;;) {
                                m_voice_buff[jj+voice_ofs][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4*4-1)]=LIMIT8((val>>5));
                                ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                if (ofs_start>=ofs_end) break;
                            }
                        }
                        while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*4*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                        m_voice_current_ptr[jj+voice_ofs]=ofs_end;
                    }
                    //TODO:  MODIZER changes end / YOYOFR
                }
            }
            else
            {
                // ノイズ有り
                for (int i=0; i<nsamples; i++)
                {
                    sample = 0;
                    //YOYOFR
                    int chanout[3];
                    chanout[0]=chanout[1]=chanout[2]=0;
                    //YOYOFR
                    for (int j=0; j < (1 << oversampling); j++)
                    {
#ifdef _M_IX86
                        noise = noisetable[(ncount >> (noiseshift+oversampling+6)) & (noisetablesize-1)]
                        >> (ncount >> (noiseshift+oversampling+1));
#else
                        noise = noisetable[(ncount >> (noiseshift+oversampling+6)) & (noisetablesize-1)]
                        >> (ncount >> (noiseshift+oversampling+1) & 31);
#endif
                        ncount += nperiod;
                        
                        int x, y, z;
                        x = ((SCOUNT(0) & chenable[0]) | (nenable[0] & noise)) - 1;		// 0 or -1
                        sample += (olevel[0] + x) ^ x;
                        chanout[0] += (olevel[0] + x) ^ x; //YOYOFR
                        scount[0] += speriod[0];
                        y = ((SCOUNT(1) & chenable[1]) | (nenable[1] & noise)) - 1;
                        sample += (olevel[1] + y) ^ y;
                        chanout[1] += (olevel[1] + y) ^ y; //YOYOFR
                        scount[1] += speriod[1];
                        z = ((SCOUNT(2) & chenable[2]) | (nenable[2] & noise)) - 1;
                        sample += (olevel[2] + z) ^ z;
                        chanout[1] += (olevel[2] + z) ^ z; //YOYOFR
                        scount[2] += speriod[2];
                    }
                    sample /= (1 << oversampling);
                    StoreSample(dest[0], sample);
                    StoreSample(dest[1], sample);
                    dest += 2;
                    
                    //TODO:  MODIZER changes start / YOYOFR
                    int64_t smplIncr;
                    if (psgrate) smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/psgrate;
                    else smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/44100;
                    int voice_ofs=6;
                    for (int jj=0;jj<3;jj++) {
                        chanout[jj] /= (1 << oversampling);
                        int64_t ofs_start=m_voice_current_ptr[jj+voice_ofs];
                        int64_t ofs_end=(m_voice_current_ptr[jj+voice_ofs]+smplIncr);
                        if ( (ofs_end>ofs_start) ) {
                            int val=chanout[jj];
                            for (;;) {
                                m_voice_buff[jj+voice_ofs][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4*4-1)]=LIMIT8((val>>5));
                                ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                if (ofs_start>=ofs_end) break;
                            }
                        }
                        while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*4*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                        m_voice_current_ptr[jj+voice_ofs]=ofs_end;
                    }
                    //TODO:  MODIZER changes end / YOYOFR
                }
            }
            
            // エンベロープの計算をさぼった帳尻あわせ
            ecount = (ecount >> 8) + (eperiod >> (8-oversampling)) * nsamples;
            if (ecount >= (1 << (envshift+6+oversampling-8)))
            {
                if ((reg[0x0d] & 0x0b) != 0x0a)
                    ecount |= (1 << (envshift+5+oversampling-8));
                ecount &= (1 << (envshift+6+oversampling-8)) - 1;
            }
            ecount <<= 8;
        }
        else
        {
            // エンベロープあり
            for (int i=0; i<nsamples; i++)
            {
                sample = 0;
                //YOYOFR
                int chanout[3];
                chanout[0]=chanout[1]=chanout[2]=0;
                //YOYOFR
                for (int j=0; j < (1 << oversampling); j++)
                {
                    env = envelop[ecount >> (envshift+oversampling)];
                    ecount += eperiod;
                    if (ecount >= (1 << (envshift+6+oversampling)))
                    {
                        if ((reg[0x0d] & 0x0b) != 0x0a)
                            ecount |= (1 << (envshift+5+oversampling));
                        ecount &= (1 << (envshift+6+oversampling)) - 1;
                    }
#ifdef _M_IX86
                    noise = noisetable[(ncount >> (noiseshift+oversampling+6)) & (noisetablesize-1)]
                    >> (ncount >> (noiseshift+oversampling+1));
#else
                    noise = noisetable[(ncount >> (noiseshift+oversampling+6)) & (noisetablesize-1)]
                    >> (ncount >> (noiseshift+oversampling+1) & 31);
#endif
                    ncount += nperiod;
                    
                    int x, y, z;
                    x = ((SCOUNT(0) & chenable[0]) | (nenable[0] & noise)) - 1;		// 0 or -1
                    sample += (*p1 + x) ^ x;
                    chanout[0] += (*p1 + x) ^ x; //YOYOFR
                    scount[0] += speriod[0];
                    y = ((SCOUNT(1) & chenable[1]) | (nenable[1] & noise)) - 1;
                    sample += (*p2 + y) ^ y;
                    chanout[1] += (*p2 + y) ^ y; //YOYOFR
                    scount[1] += speriod[1];
                    z = ((SCOUNT(2) & chenable[2]) | (nenable[2] & noise)) - 1;
                    sample += (*p3 + z) ^ z;
                    chanout[2] += (*p3 + z) ^ z; //YOYOFR
                    scount[2] += speriod[2];
                }
                sample /= (1 << oversampling);
                StoreSample(dest[0], sample);
                StoreSample(dest[1], sample);
                dest += 2;
                
                //TODO:  MODIZER changes start / YOYOFR
                int64_t smplIncr;
                if (psgrate) smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/psgrate;
                else smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/44100;
                int voice_ofs=6;
                for (int jj=0;jj<3;jj++) {
                    chanout[jj] /= (1 << oversampling);
                    int64_t ofs_start=m_voice_current_ptr[jj+voice_ofs];
                    int64_t ofs_end=(m_voice_current_ptr[jj+voice_ofs]+smplIncr);
                    if ( (ofs_end>ofs_start) ) {
                        int val=chanout[jj];
                        for (;;) {
                            m_voice_buff[jj+voice_ofs][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4*4-1)]=LIMIT8((val>>5));
                            ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                            if (ofs_start>=ofs_end) break;
                        }
                    }
                    while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*4*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                    m_voice_current_ptr[jj+voice_ofs]=ofs_end;
                }
                //TODO:  MODIZER changes end / YOYOFR
            }
        }
    } else {
        //TODO:  MODIZER changes start / YOYOFR
        int64_t smplIncr;
        if (psgrate) smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/psgrate;
        else smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/44100;
        int voice_ofs=6;
        for (int jj=0;jj<3;jj++) {
            int64_t ofs_start=m_voice_current_ptr[jj+voice_ofs];
            int64_t ofs_end=(m_voice_current_ptr[jj+voice_ofs]+nsamples*smplIncr);
            while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*4*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
            m_voice_current_ptr[jj+voice_ofs]=ofs_end;
        }
        //TODO:  MODIZER changes end / YOYOFR
    }
    
    //YOYOFR
    int voice_ofs=6;
    for (int ii=0;ii<3;ii++) {
        vgm_last_note[ii+voice_ofs]=0;
        vgm_last_vol[ii+voice_ofs]=0;
        
        if (nenable[ii]) {
            if ((olevel[ii])/*&&R->output[ii]*/) {
                int freq=((reg[6]) & 0x1f)+1;
                //if (!(R->sega_style_psg) && (freq==0)) freq=0x400;
                if (freq) {
                    vgm_last_note[ii+voice_ofs]=(double)psgclock/(2*freq)/16/1024;
                    vgm_last_sample_addr[ii+voice_ofs]=voice_ofs+ii;
                    int newvol=olevel[ii]>>5;
                    vgm_last_vol[ii+voice_ofs]=1+newvol;
                }
            }
        }
        
        if (chenable[ii]) {
            if ( (olevel[ii])/*&&R->output[ii]*/) {
                int freq=((reg[ii*2+0] + ((int)reg[ii*2+1]) * 256) & 0xfff)+1;
                //if (!(R->sega_style_psg) && (freq==0)) freq=0x400;
                if (freq) {
                    vgm_last_note[ii+voice_ofs]=(double)psgclock/(2*freq)/16;
                    vgm_last_sample_addr[ii+voice_ofs]=voice_ofs+ii;
                    int newvol=olevel[ii]>>5;
                    vgm_last_vol[ii+voice_ofs]=1+newvol;
                }
            }
        }
        
    }
    //YOYOFR
}

// ---------------------------------------------------------------------------
//	テーブル
//
uint	PSG::noisetable[noisetablesize] = { 0, };
