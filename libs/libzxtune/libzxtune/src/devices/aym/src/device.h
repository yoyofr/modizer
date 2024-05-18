/**
 *
 * @file
 *
 * @brief  AY/YM device implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR


//local includes
#include "generators.h"
//library includes
#include <devices/aym/chip.h>

namespace Devices
{
namespace AYM
{
class AYMDevice
{
public:
    AYMDevice()
    : Levels()
    , NoiseMask(HIGH_LEVEL)
    , EnvelopeMask(LOW_LEVEL)
    {
    }
    
    void SetDutyCycle(uint_t value, uint_t mask)
    {
        GenA.SetDutyCycle(0 != (mask & CHANNEL_MASK_A) ? value : NO_DUTYCYCLE);
        GenB.SetDutyCycle(0 != (mask & CHANNEL_MASK_B) ? value : NO_DUTYCYCLE);
        GenC.SetDutyCycle(0 != (mask & CHANNEL_MASK_C) ? value : NO_DUTYCYCLE);
    }
    
    void SetMixer(uint_t mixer)
    {
        GenA.SetMasked(0 != (mixer & Registers::MASK_TONEA));
        GenB.SetMasked(0 != (mixer & Registers::MASK_TONEB));
        GenC.SetMasked(0 != (mixer & Registers::MASK_TONEC));
        NoiseMask = HIGH_LEVEL;
        if (0 == (mixer & Registers::MASK_NOISEA))
        {
            NoiseMask ^= HIGH_LEVEL_A;
        }
        if (0 == (mixer & Registers::MASK_NOISEB))
        {
            NoiseMask ^= HIGH_LEVEL_B;
        }
        if (0 == (mixer & Registers::MASK_NOISEC))
        {
            NoiseMask ^= HIGH_LEVEL_C;
        }
    }
    
    void SetToneA(uint_t toneA)
    {
        GenA.SetPeriod(toneA);
    }
    
    void SetToneB(uint_t toneB)
    {
        GenB.SetPeriod(toneB);
    }
    
    void SetToneC(uint_t toneC)
    {
        GenC.SetPeriod(toneC);
    }
    
    void SetToneN(uint_t toneN)
    {
        GenN.SetPeriod(toneN);
    }
    
    void SetToneE(uint_t toneE)
    {
        GenE.SetPeriod(toneE);
    }
    
    void SetEnvType(uint_t type)
    {
        GenE.SetType(type);
    }
    
    void SetLevel(uint_t levelA, uint_t levelB, uint_t levelC)
    {
        Levels = 0;
        EnvelopeMask = 0;
        
        SetLevel(0, levelA);
        SetLevel(1, levelB);
        SetLevel(2, levelC);
    }
    
    void Reset()
    {
        GenA.Reset();
        GenB.Reset();
        GenC.Reset();
        GenN.Reset();
        GenE.Reset();
        Levels = 0;
        NoiseMask = HIGH_LEVEL;
        EnvelopeMask = LOW_LEVEL;
    }
    
    void Tick(uint_t ticks)
    {
        GenA.Tick(ticks);
        GenB.Tick(ticks);
        GenC.Tick(ticks);
        GenN.Tick(ticks);
        GenE.Tick(ticks);
    }
    
    uint_t GetLevelsChan(int chan) const {
        const uint_t level = EnvelopeMask ? (EnvelopeMask * GenE.GetLevel()) | Levels : Levels;
        const uint_t noise = NoiseMask != HIGH_LEVEL ? (NoiseMask | GenN.GetLevel()) : NoiseMask;
        const uint_t toneA = GenA.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_A, HIGH_LEVEL>();
        const uint_t toneB = GenB.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_B, HIGH_LEVEL>();
        const uint_t toneC = GenC.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_C, HIGH_LEVEL>();
        
        if (generic_mute_mask&(1<<(m_voicesForceOfs+chan))) return 0;
        
        if ((chan==0)||(chan==3)) return ((level & toneA & noise)>>0)&((1<<(BITS_PER_LEVEL+1))-1);
        if ((chan==1)||(chan==4)) return ((level & toneB & noise)>>(BITS_PER_LEVEL))&((1<<(BITS_PER_LEVEL+1))-1);
        return ((level & toneC & noise)>>(BITS_PER_LEVEL*2))&((1<<(BITS_PER_LEVEL+1))-1);
    }
    
    uint_t GetLevels() const
    {
        static uint_t last_level[3];
        const uint_t level = EnvelopeMask ? (EnvelopeMask * GenE.GetLevel()) | Levels : Levels;
        const uint_t noise = NoiseMask != HIGH_LEVEL ? (NoiseMask | GenN.GetLevel()) : NoiseMask;
        /*const*/ uint_t toneA = GenA.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_A, HIGH_LEVEL>();
         /*const*/ uint_t toneB = GenB.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_B, HIGH_LEVEL>();
         /*const*/ uint_t toneC = GenC.GetLevel<HIGH_LEVEL ^ HIGH_LEVEL_C, HIGH_LEVEL>();
        for (int i=0;i<3;i++) {
            float period=0;
            int vol=0;
            
            if (generic_mute_mask&(1<<(m_voicesForceOfs+i))) {
                if (i==0) toneA&=~((1<<(BITS_PER_LEVEL+1))-1);
                else if (i==1) toneB=~(((1<<(BITS_PER_LEVEL+1))-1)<<BITS_PER_LEVEL);
                else if (i==2) toneC=~(((1<<(BITS_PER_LEVEL+1))-1)<<(BITS_PER_LEVEL*2));
            }
            
            if (i==0) {
                period=(float)(GenA.getDoublePeriod());
                vol=(level & toneA & noise);
            } else if (i==1) {
                period=(float)(GenB.getDoublePeriod());
                vol=(level & toneB & noise);
            } else if (i==2) {
                period=(float)(GenC.getDoublePeriod());
                vol=(level & toneC & noise);
            }
            
            if (period && vol && (vol!=last_level[i])) {
                //ay->clock_rate/freq/16;
                float freq=m_voice_current_samplerate/period;
                vgm_last_note[m_voicesForceOfs+i]=freq;
                vgm_last_sample_addr[m_voicesForceOfs+i]=m_voicesForceOfs+i;
                vgm_last_vol[m_voicesForceOfs+i]=1;
            }
                
                last_level[i]=vol;
        }
        
        
        
        return level & toneA & toneB & toneC & noise;
    }
private:
    void SetLevel(uint_t chan, uint_t reg)
    {
        const uint_t shift = chan * BITS_PER_LEVEL;
        if (0 != (reg & Registers::MASK_ENV))
        {
            EnvelopeMask |= 1 << shift;
        }
        else
        {
            Levels |= ((reg << 1) + 1) << shift;
        }
    }
private:
    ToneGenerator GenA;
    ToneGenerator GenB;
    ToneGenerator GenC;
    NoiseGenerator GenN;
    EnvelopeGenerator GenE;
    uint_t Levels;
    uint_t NoiseMask;
    uint_t EnvelopeMask;
};
}
}
