/**
 *
 * @file
 *
 * @brief  PSG-based renderers implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR

//library includes
#include <devices/details/clock_source.h>
#include <sound/chunk_builder.h>
#include <sound/lpfilter.h>

namespace Devices
{
namespace Details
{

void render_update_modizer(void);

template<class StampType>
class Renderer
{
public:
    virtual ~Renderer() {}
    
    virtual void Render(StampType tillTime, uint_t samples, Sound::ChunkBuilder& target) = 0;
    virtual void Render(StampType tillTime, Sound::ChunkBuilder& target) = 0;
};

/*
 Time (1 uS)
 ||||||||||||||||||||||||||||||||||||||||||||||
 
 PSG (1773400 / 8 Hz, ~5uS)
 
 |    |    |    |    |    |    |    |    |    |    |    |    |    |
 
 Sound (44100 Hz, Clock.SamplePeriod = ~22uS)
 |                     |                     |
 
 ->|-|<-- Clock.TicksDelta
 Clock.NextSampleTime -->|
 
 */

template<class StampType, class PSGType>
class BaseRenderer : public Renderer<StampType>
{
    typedef typename ClockSource<StampType>::FastStamp FastStamp;
public:
    template<class ParameterType>
    BaseRenderer(ClockSource<StampType>& clock, ParameterType& psg)
    : Clock(clock)
    , PSG(psg)
    {
    }
    
    virtual void Render(StampType tillTime, uint_t samples, Sound::ChunkBuilder& target)
    {
        FinishPreviousSample(target);
        RenderMultipleSamples(samples - 1, target);
        StartNextSample(FastStamp(tillTime.Get()));
    }
    
    virtual void Render(StampType tillTime, Sound::ChunkBuilder& target)
    {
        const FastStamp end(tillTime.Get());
        if (Clock.HasSamplesBefore(end))
        {
            FinishPreviousSample(target);
            while (Clock.HasSamplesBefore(end))
            {
                RenderSingleSample(target);
            }
        }
        StartNextSample(end);
    }
private:
    
    void mdz_update_voice(void) {
        //YOYOFR
        if (m_voice_current_samplerate==0) m_voice_current_samplerate=44100;
        int64_t smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/(m_voice_current_samplerate);
        smplIncr=1<<16;
        int m_voice_ofs=m_voicesForceOfs;
        int j;
        for (j=0;j<m_voice_current_total;j++) {
            int64_t ofs_start=m_voice_current_ptr[m_voice_ofs+j];
            int64_t ofs_end=(m_voice_current_ptr[m_voice_ofs+j]+smplIncr);
            int chanout;

            if (generic_mute_mask&(1<<m_voice_ofs+j)) chanout=0;
            else chanout = MAXVAL(PSG.GetLevelsChan(j+m_voicesForceOfs).Left(),PSG.GetLevelsChan(j+m_voicesForceOfs).Right());
            
            if (ofs_end>ofs_start)
                for (;;) {
                    m_voice_buff[m_voice_ofs+j][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=LIMIT8( (chanout>>7) );
                    ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                    if (ofs_start>=ofs_end) break;
                }
            //YOYOFR
            
            while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*2) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*4*2<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
            m_voice_current_ptr[m_voice_ofs+j]=ofs_end;
        }
        //YOYOFR
        
    }
    
    void FinishPreviousSample(Sound::ChunkBuilder& target)
    {
        if (const uint_t ticksPassed = Clock.AdvanceTimeToNextSample())
        {
            PSG.Tick(ticksPassed);
        }
        target.Add(PSG.GetLevels());
        mdz_update_voice();//YOYOFR
        
        Clock.UpdateNextSampleTime();
    }
    
    void RenderMultipleSamples(uint_t samples, Sound::ChunkBuilder& target)
    {
        //YOYOFR
        memset(vgm_last_note,0,sizeof(vgm_last_note));
        memset(vgm_last_vol,0,sizeof(vgm_last_vol));
        //YOYOFR
        for (uint_t count = samples; count != 0; --count)
        {
            const uint_t ticksPassed = Clock.AllocateSample();
            PSG.Tick(ticksPassed);
            target.Add(PSG.GetLevels());
            mdz_update_voice();//YOYOFR
        }
        Clock.CommitSamples(samples);
    }
    
    void RenderSingleSample(Sound::ChunkBuilder& target)
    {
        const uint_t ticksPassed = Clock.AdvanceSample();
        PSG.Tick(ticksPassed);
        target.Add(PSG.GetLevels());
        mdz_update_voice();//YOYOFR
        
    }
    
    void StartNextSample(FastStamp till)
    {
        if (const uint_t ticksPassed = Clock.AdvanceTime(till))
        {
            PSG.Tick(ticksPassed);
        }
    }
protected:
    ClockSource<StampType>& Clock;
    PSGType PSG;
};

/*
 Simple decimation algorithm without any filtering
 */
template<class PSGType>
class LQWrapper
{
public:
    explicit LQWrapper(PSGType& delegate)
    : Delegate(delegate)
    {
    }
    
    void Tick(uint_t ticks)
    {
        Delegate.Tick(ticks);
    }
    
    Sound::Sample GetLevels() const
    {
        return Delegate.GetLevels();
    }
    
    Sound::Sample GetLevelsChan(int chan) const
    {
        return Delegate.GetLevelsChan(chan);
    }
private:
    PSGType& Delegate;
};

/*
 Simple decimation with post simple FIR filter (0.5, 0.5)
 */
template<class PSGType>
class MQWrapper
{
public:
    explicit MQWrapper(PSGType& delegate)
    : Delegate(delegate)
    {
    }
    
    void Tick(uint_t ticks)
    {
        Delegate.Tick(ticks);
    }
    
    Sound::Sample GetLevels() const
    {
        const Sound::Sample curLevel = Delegate.GetLevels();
        return Interpolate(curLevel);
    }
    
    Sound::Sample GetLevelsChan(int chan) const
    {
        const Sound::Sample curLevel = Delegate.GetLevelsChan(chan);
        return Interpolate(curLevel);
    }
private:
    Sound::Sample Interpolate(Sound::Sample newLevel) const
    {
        const Sound::Sample out = Average(PrevLevel, newLevel);
        PrevLevel = newLevel;
        return out;
    }
    
    static Sound::Sample::Type Average(Sound::Sample::WideType first, Sound::Sample::WideType second)
    {
        return static_cast<Sound::Sample::Type>((first + second) / 2);
    }
    
    static Sound::Sample Average(Sound::Sample first, Sound::Sample second)
    {
        return Sound::Sample(Average(first.Left(), second.Left()), Average(first.Right(), second.Right()));
    }
private:
    PSGType& Delegate;
    mutable Sound::Sample PrevLevel;
};


/*
 Decimation is performed after 2-order IIR LPF
 Cutoff freq of LPF should be less than Nyquist frequency of target signal
 */
const uint_t SOUND_CUTOFF_FREQUENCY = 9500;

template<class PSGType>
class HQWrapper
{
public:
    explicit HQWrapper(PSGType& delegate)
    : Delegate(delegate)
    {
    }
    
    void SetClockFrequency(uint64_t clockFreq)
    {
        Filter.SetParameters(clockFreq, SOUND_CUTOFF_FREQUENCY);
        
        for (int i=0;i<6;i++)
            Filter_mdz[i].SetParameters(clockFreq, SOUND_CUTOFF_FREQUENCY);
    }
    
    void Tick(uint_t ticksPassed)
    {
        while (ticksPassed--)
        {
            Filter.Feed(Delegate.GetLevels());
            
            for (int i=0;i<m_voice_current_total;i++)
                Filter_mdz[(i+m_voicesForceOfs)%6].Feed(Delegate.GetLevelsChan(i));
            
            Delegate.Tick(1);
        }
    }
    
    Sound::Sample GetLevels() const
    {
        return Filter.Get();
    }
    
    Sound::Sample GetLevelsChan(int chan) const
    {
        if (chan<0) chan=0;
        if (chan>5) chan=5;
        return Filter_mdz[chan].Get();
    }
private:
    PSGType& Delegate;
    Sound::LPFilter Filter;
    
    Sound::LPFilter Filter_mdz[6];
};

template<class StampType, class PSGType>
class LQRenderer : public BaseRenderer<StampType, LQWrapper<PSGType> >
{
    typedef BaseRenderer<StampType, LQWrapper<PSGType> > Parent;
public:
    LQRenderer(ClockSource<StampType>& clock, PSGType& psg)
    : Parent(clock, psg)
    {
    }
};

template<class StampType, class PSGType>
class MQRenderer : public BaseRenderer<StampType, MQWrapper<PSGType> >
{
    typedef BaseRenderer<StampType, MQWrapper<PSGType> > Parent;
public:
    MQRenderer(ClockSource<StampType>& clock, PSGType& psg)
    : Parent(clock, psg)
    {
    }
private:
};

template<class StampType, class PSGType>
class HQRenderer : public BaseRenderer<StampType, HQWrapper<PSGType> >
{
    typedef BaseRenderer<StampType, HQWrapper<PSGType> > Parent;
public:
    HQRenderer(ClockSource<StampType>& clock, PSGType& psg)
    : Parent(clock, psg)
    {
    }
    
    void SetClockFrequency(uint64_t clockFreq)
    {
        Parent::PSG.SetClockFrequency(clockFreq);
    }
};
}
}
