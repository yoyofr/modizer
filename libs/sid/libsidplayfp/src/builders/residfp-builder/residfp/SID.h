/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004 Dag Lem <resid@nimrod.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SIDFP_H
#define SIDFP_H

#include <memory>

#include "siddefs-fp.h"

#include "sidcxx11.h"

namespace reSIDfp
{

class Filter;
class Filter6581;
class Filter8580;
class ExternalFilter;
class Potentiometer;
class Voice;
class Resampler;

/**
 * SID error exception.
 */
class SIDError
{
private:
    const char* message;

public:
    SIDError(const char* msg) :
        message(msg) {}
    const char* getMessage() const { return message; }
};

/**
 * MOS6581/MOS8580 emulation.
 */
class SID
{
private:
    /// Currently active filter
    Filter* filter;

    /// Filter used, if model is set to 6581
    Filter6581* const filter6581;

    /// Filter used, if model is set to 8580
    Filter8580* const filter8580;

    /**
     * External filter that provides high-pass and low-pass filtering
     * to adjust sound tone slightly.
     */
    ExternalFilter* const externalFilter;

    /// Resampler used by audio generation code.
    std::unique_ptr<Resampler> resampler;

    /// Paddle X register support
    Potentiometer* const potX;

    /// Paddle Y register support
    Potentiometer* const potY;

    /// SID voices
    std::unique_ptr<Voice> voice[3];

    /// Used to amplify the output by x/2 to get an adequate playback volume
    int scaleFactor;

    /// Time to live for the last written value
    int busValueTtl;

    /// Current chip model's bus value TTL
    int modelTTL;

    /// Time until #voiceSync must be run.
    unsigned int nextVoiceSync;

    /// Currently active chip model.
    ChipModel model;

    /// Currently selected combined waveforms strength.
    CombinedWaveforms cws;

    /// Last written value
    unsigned char busValue;

    /// Flags for muted channels
    bool muted[3];

    /**
     * Emulated nonlinearity of the envelope DAC.
     *
     * @See Dac
     */
    float envDAC[256];

    /**
     * Emulated nonlinearity of the oscillator DAC.
     *
     * @See Dac
     */
    float oscDAC[4096];

private:
    /**
     * Age the bus value and zero it if it's TTL has expired.
     *
     * @param n the number of cycles
     */
    void ageBusValue(unsigned int n);

    /**
     * Get output sample.
     *
     * @return the output sample
     */
    int output() const;

    /**
     * Calculate the numebr of cycles according to current parameters
     * that it takes to reach sync.
     *
     * @param sync whether to do the actual voice synchronization
     */
    void voiceSync(bool sync);

public:
    SID();
    ~SID();

    /**
     * Set chip model.
     *
     * @param model chip model to use
     * @throw SIDError
     */
    void setChipModel(ChipModel model);

    /**
     * Get currently emulated chip model.
     */
    ChipModel getChipModel() const { return model; }

    /**
     * Set combined waveforms strength.
     *
     * @param cws strength of combined waveforms
     * @throw SIDError
     */
    void setCombinedWaveforms(CombinedWaveforms cws);

    /**
     * SID reset.
     */
    void reset();

    /**
     * 16-bit input (EXT IN). Write 16-bit sample to audio input. NB! The caller
     * is responsible for keeping the value within 16 bits. Note that to mix in
     * an external audio signal, the signal should be resampled to 1MHz first to
     * avoid sampling noise.
     *
     * @param value input level to set
     */
    void input(int value);

    /**
     * Read registers.
     *
     * Reading a write only register returns the last char written to any SID register.
     * The individual bits in this value start to fade down towards zero after a few cycles.
     * All bits reach zero within approximately $2000 - $4000 cycles.
     * It has been claimed that this fading happens in an orderly fashion,
     * however sampling of write only registers reveals that this is not the case.
     * NOTE: This is not correctly modeled.
     * The actual use of write only registers has largely been made
     * in the belief that all SID registers are readable.
     * To support this belief the read would have to be done immediately
     * after a write to the same register (remember that an intermediate write
     * to another register would yield that value instead).
     * With this in mind we return the last value written to any SID register
     * for $2000 cycles without modeling the bit fading.
     *
     * @param offset SID register to read
     * @return value read from chip
     */
    unsigned char read(int offset);

    /**
     * Write registers.
     *
     * @param offset chip register to write
     * @param value value to write
     */
    void write(int offset, unsigned char value);

    /**
     * SID voice muting.
     *
     * @param channel channel to modify
     * @param enable is muted?
     */
    void mute(int channel, bool enable) { muted[channel] = enable; }

    /**
     * Setting of SID sampling parameters.
     *
     * Use a clock freqency of 985248Hz for PAL C64, 1022730Hz for NTSC C64.
     * The default end of passband frequency is pass_freq = 0.9*sample_freq/2
     * for sample frequencies up to ~ 44.1kHz, and 20kHz for higher sample frequencies.
     *
     * For resampling, the ratio between the clock frequency and the sample frequency
     * is limited as follows: 125*clock_freq/sample_freq < 16384
     * E.g. provided a clock frequency of ~ 1MHz, the sample frequency can not be set
     * lower than ~ 8kHz. A lower sample frequency would make the resampling code
     * overfill its 16k sample ring buffer.
     *
     * The end of passband frequency is also limited: pass_freq <= 0.9*sample_freq/2
     *
     * E.g. for a 44.1kHz sampling rate the end of passband frequency
     * is limited to slightly below 20kHz.
     * This constraint ensures that the FIR table is not overfilled.
     *
     * @param clockFrequency System clock frequency at Hz
     * @param method sampling method to use
     * @param samplingFrequency Desired output sampling rate
     * @param highestAccurateFrequency
     * @throw SIDError
     */
    void setSamplingParameters(double clockFrequency, SamplingMethod method, double samplingFrequency, double highestAccurateFrequency);

    /**
     * Clock SID forward using chosen output sampling algorithm.
     *
     * @param cycles c64 clocks to clock
     * @param buf audio output buffer
     * @return number of samples produced
     */
    int clock(unsigned int cycles, short* buf);

    /**
     * Clock SID forward with no audio production.
     *
     * _Warning_:
     * You can't mix this method of clocking with the audio-producing
     * clock() because components that don't affect OSC3/ENV3 are not
     * emulated.
     *
     * @param cycles c64 clocks to clock.
     */
    void clockSilent(unsigned int cycles);

    /**
     * Set filter curve parameter for 6581 model.
     *
     * @see Filter6581::setFilterCurve(double)
     */
    void setFilter6581Curve(double filterCurve);

    /**
    * Set filter range parameter for 6581 model
    *
    * @see Filter6581::setFilterRange(double)
    */
    void setFilter6581Range ( double adjustment );

    /**
     * Set filter curve parameter for 8580 model.
     *
     * @see Filter8580::setFilterCurve(double)
     */
    void setFilter8580Curve(double filterCurve);

    /**
     * Enable filter emulation.
     *
     * @param enable false to turn off filter emulation
     */
    void enableFilter(bool enable);
};

} // namespace reSIDfp

#if RESID_INLINING || defined(SID_CPP)

#include <algorithm>

#include "Filter.h"
#include "ExternalFilter.h"
#include "Voice.h"
#include "resample/Resampler.h"

namespace reSIDfp
{

RESID_INLINE
void SID::ageBusValue(unsigned int n)
{
    if (likely(busValueTtl != 0))
    {
        busValueTtl -= n;

        if (unlikely(busValueTtl <= 0))
        {
            busValue = 0;
            busValueTtl = 0;
        }
    }
}

//TODO:  MODIZER changes start / YOYOFR
static float sid_v1;
static float sid_v2;
static float sid_v3;
extern "C" int sid_v4;


extern "C" {
#include "../../../../src/ModizerVoicesData.h"
extern char mSIDSeekInProgress;
extern void* m_sid_chipId[MAXSID_CHIPS];
char sid_firstcall[MAXSID_CHIPS];
}
//TODO:  MODIZER changes end / YOYOFR


RESID_INLINE
int SID::output() const
{
//    const float v1 = voice[0]->output(voice[2]->wave());
//    const float v2 = voice[1]->output(voice[0]->wave());
//    const float v3 = voice[2]->output(voice[1]->wave());
    //YOYOFR
    sid_v1 = voice[0]->output(voice[2]->wave());
    sid_v2 = voice[1]->output(voice[0]->wave());
    sid_v3 = voice[2]->output(voice[1]->wave());

//    const int input = static_cast<int>(filter->clock(v1, v2, v3));
    const int input = static_cast<int>(filter->clock(sid_v1, sid_v2, sid_v3));
    //YOYOFR
    
    return externalFilter->clock(input);
}


RESID_INLINE
int SID::clock(unsigned int cycles, short* buf)
{
    ageBusValue(cycles);
    int s = 0;
    
    //TODO:  MODIZER changes start / YOYOFR
    int sid_idx=0;//chipId*4;
    while (sid_idx<MAXSID_CHIPS) {
        if (m_sid_chipId[sid_idx]==NULL) {
            m_sid_chipId[sid_idx]=(void*)this;
            break;
        }
        if (m_sid_chipId[sid_idx]==(void*)this) break;
        sid_idx++;
    }
    int64_t smplIncr=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
    bool all_muted=muted[0]&muted[1]&muted[2];
    sid_idx=sid_idx*4;
    //TODO:  MODIZER changes end / YOYOFR
    
    while (cycles != 0)
    {
        unsigned int delta_t = std::min(nextVoiceSync, cycles);
        
        if (likely(delta_t > 0))
        {
            for (unsigned int i = 0; i < delta_t; i++)
            {
                if (mdz_ratio_fp_inc==0) mdz_ratio_fp_cnt+=65536;
                else mdz_ratio_fp_cnt+=mdz_ratio_fp_inc;
                
                while (mdz_ratio_fp_cnt>=65536) {
                    mdz_ratio_fp_cnt-=65536;
                    
                    // clock waveform generators
                    voice[0]->wave()->clock();
                    voice[1]->wave()->clock();
                    voice[2]->wave()->clock();
                    
                    // clock envelope generators
                    voice[0]->envelope()->clock();
                    voice[1]->envelope()->clock();
                    voice[2]->envelope()->clock();
                    
                    //TODO:  MODIZER changes start / YOYOFR
                    /*if (unlikely(resampler->input(output())))
                     {
                     buf[s++] = resampler->getOutput(scaleFactor);
                     }*/
                    if (!mSIDSeekInProgress) {
                        if ((unlikely(resampler->input(output()))))
                        {
                            if (all_muted) {
                                buf[s]=0;
                                sid_v4=0;
                            } else {
                                buf[s] = resampler->getOutput(scaleFactor);
                                s++;
                            }
                            
                            for (int j=0;j<4;j++) {
                                m_voice_buff[sid_idx+0][m_voice_current_ptr[sid_idx+0]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((int)(sid_v1*127));
                                m_voice_buff[sid_idx+1][m_voice_current_ptr[sid_idx+1]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((int)(sid_v2*127));
                                m_voice_buff[sid_idx+2][m_voice_current_ptr[sid_idx+2]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((int)(sid_v3*127));
                                m_voice_buff[sid_idx+3][m_voice_current_ptr[sid_idx+3]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=LIMIT8((sid_v4>>7));
                                
                                
                                m_voice_current_ptr[sid_idx+j]+=smplIncr;
                                if ((m_voice_current_ptr[sid_idx+j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*2) m_voice_current_ptr[sid_idx+j]-=(SOUND_BUFFER_SIZE_SAMPLE*2)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                            }
                            
                            //TODO:  MODIZER changes end / YOYOFR
                        }
                    } else {
                        if (unlikely(resampler->input(0))) {
                            int cnt=0;
                            //s++;
                            buf[s]=0;
                            s++;
                            
                            //TODO:  MODIZER changes start / YOYOFR
                            for (int j=0;j<4;j++) {
                                m_voice_buff[sid_idx+j][m_voice_current_ptr[sid_idx+j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT]=0;
                                
                                m_voice_current_ptr[sid_idx+j]+=smplIncr;
                                if ((m_voice_current_ptr[sid_idx+j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*2) m_voice_current_ptr[sid_idx+j]-=(SOUND_BUFFER_SIZE_SAMPLE*2)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                            }
                        }
                    }
                }
            }
            //TODO:  MODIZER changes end / YOYOFR
            
            cycles -= delta_t;
            nextVoiceSync -= delta_t;
        }

        if (unlikely(nextVoiceSync == 0))
        {
            voiceSync(true);
        }
    }
    
    if (sid_firstcall[sid_idx]) {
        //sid_firstcall[sid_idx]=0;
        for (int j=0;j<3;j++) {
            if (voice[j]->wave()->readFreq()) {
                vgm_last_note[sid_idx+j]=voice[j]->wave()->readFreq();
                vgm_last_vol[sid_idx+j]=voice[j]->envelope()->readENV();
            }
        }
    }
    
    return s;
}

} // namespace reSIDfp

#endif

#endif
