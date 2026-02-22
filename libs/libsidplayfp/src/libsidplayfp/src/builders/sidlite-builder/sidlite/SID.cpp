/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2025-2026 Leandro Nini
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Based on cRSID lightweight RealSID by Hermit (Mihaly Horvath)

#include "SID.h"

#include "sl_defs.h"
#include "sl_constants.h"
#include "filt_tables.h"
#include "cw_tables.h"

#include <algorithm>
#include <iterator>

//TODO:  MODIZER changes start / YOYOFR
//static float sid_v1;
//static float sid_v2;
//static float sid_v3;
extern "C" int sid_v4;

extern "C" {
#include "../../../../../src/ModizerVoicesData.h"
extern char mSIDSeekInProgress;
extern void* m_sid_chipId[MAXSID_CHIPS];
extern char sid_firstcall[MAXSID_CHIPS];
static int sid_idx=0;
extern int m_sid_chipNb;
}
//TODO:  MODIZER changes end / YOYOFR


namespace SIDLite
{

int SID::clock(unsigned int cycles, short* buf)
{
    int i = 0;
    
    //TODO:  MODIZER changes start / YOYOFR
    sid_idx=0;//chipId*4;
    while (sid_idx<MAXSID_CHIPS) {
        if (m_sid_chipId[sid_idx]==NULL) {
            m_sid_chipId[sid_idx]=(void*)this;
            break;
        }
        if (m_sid_chipId[sid_idx]==(void*)this) break;
        sid_idx++;
    }
    sid_idx=sid_idx%m_sid_chipNb;
    int64_t smplIncr=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
    sid_idx=sid_idx*4;
    //TODO:  MODIZER changes end / YOYOFR
    
    if (mdz_ratio_fp_inc!=0) cycles=cycles*mdz_ratio_fp_inc/65536;
    
    while (cycles > 0)
    {
        //YOYOFR
        if (mdz_ratio_fp_inv_inc==0) mdz_ratio_fp_cnt+=65536;
        else mdz_ratio_fp_cnt+=mdz_ratio_fp_inv_inc;
        
        buf[i] = generateSample(cycles);
        
        int sid_v1=(wavgen.ChannelOutput[0]>>6);
        int sid_v2=(wavgen.ChannelOutput[1]>>6);
        int sid_v3=(wavgen.ChannelOutput[2]>>6);
        //sid_v4=0;//(wavgen.PrevWavData[0]+wavgen.PrevWavData[1]+wavgen.PrevWavData[2])/3;
        for (int j=0;j<4;j++) {
            m_voice_buff[sid_idx+0][m_voice_current_ptr[sid_idx+0]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((int)(sid_v1));
            m_voice_buff[sid_idx+1][m_voice_current_ptr[sid_idx+1]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((int)(sid_v2));
            m_voice_buff[sid_idx+2][m_voice_current_ptr[sid_idx+2]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((int)(sid_v3));
            m_voice_buff[sid_idx+3][m_voice_current_ptr[sid_idx+3]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((sid_v4>>10));
            
            m_voice_current_ptr[sid_idx+j]+=smplIncr;
            if ((m_voice_current_ptr[sid_idx+j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*2) m_voice_current_ptr[sid_idx+j]-=(SOUND_BUFFER_SIZE_SAMPLE*2)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
        }
        i++;
    }
    
    //YOYOFR
    for (int j=0;j<3;j++) {
        if (wavgen.ChannelFreq[j]) {
            vgm_last_note[sid_idx+j]=wavgen.ChannelFreq[j];
            vgm_last_vol[sid_idx+j]=wavgen.ChannelEnv[j];
        }
    }
    //YOYOFR
    
    return i;
}

inline signed short SID::generateSample(unsigned int &cycles)
{
    // call this from custom buffer-filler
    int Output = emulateC64(cycles);
    // saturation logic on overflow
    if (Output > 32767)
        Output = 32767;
    else if (Output < -32768)
        Output = -32768;
    return static_cast<signed short>(Output);
}


inline int SID::emulateC64(unsigned int &cycles)
{
    // Cycle-based part of emulations:

    while ((SampleCycleCnt <= s.SampleClockRatio) && cycles)
    {
        unsigned char InstructionCycles = std::min(7u, cycles);
        SampleCycleCnt += InstructionCycles << 4;
        cycles -= InstructionCycles;

        adsr.clock(InstructionCycles);
    }

    SampleCycleCnt -= s.SampleClockRatio;

    // Samplerate-based part of emulations:

    wg_output_t output = wavgen.clock(&adsr);
    return filter.clock(output.first, output.second);
}

void SID::write(int addr, int value)
{
    regs[addr] = value;
}

int SID::read(int addr)
{
    if (addr == 0x1B)
        return wavgen.getOsc3();
    if (addr == 0x1C)
        return wavgen.getEnv3();
    return 0;
}

SID::SID() :
    adsr(regs),
    filter(&s, regs),
    wavgen(&s, regs)
{
    setChipModel(8580);
    reset();
}

void SID::reset()
{
    SampleCycleCnt = 0;

    std::fill(std::begin(regs), std::end(regs), 0);
}

void SID::setSamplingParameters(unsigned int clockFrequency, unsigned short samplingFrequency)
{
    filter.rebuildCutoffTables(samplingFrequency);

    // shifting (multiplication) enhances SampleClockRatio precision
    s.SampleClockRatio = (clockFrequency << 4) / samplingFrequency;
}

void SID::setChipModel(int model)
{
    s.ChipModel = model;
}

void SID::setRealSIDmode(bool mode)
{
    s.RealSIDmode = mode;
}

}
