/**
 *
 * @file
 *
 * @brief  SAA device implementation
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

namespace Devices
{
namespace SAA
{
inline uint_t LoNibble(uint_t val)
{
    return val & 15;
}

inline uint_t HiNibble(uint_t val)
{
    return val >> 4;
}

class SAASubDevice
{
public:
    SAASubDevice()
    : Tones()
    , Noise()
    , Envelope()
    , Levels()
    {
    }
    
    void SetLevel(uint_t generator, uint_t left, uint_t right)
    {
        Levels[generator] = FastSample(left, right);
    }
    
    void SetToneNumber(uint_t generator, uint_t number)
    {
        Tones[generator].SetNumber(number);
        UpdateLinkedGenerators(generator);
    }
    
    void SetToneOctave(uint_t generator, uint_t octave)
    {
        Tones[generator].SetOctave(octave);
        UpdateLinkedGenerators(generator);
    }
    
    void SetToneMixer(uint_t mixer)
    {
        Tones[0].SetMasked(0 == (mixer & 1));
        Tones[1].SetMasked(0 == (mixer & 2));
        Tones[2].SetMasked(0 == (mixer & 4));
    }
    
    void SetNoiseMixer(uint_t mixer)
    {
        Noise.SetMixer(mixer & 7);
    }
    
    void SetNoiseControl(uint_t ctrl)
    {
        Noise.SetMode(ctrl & 3);
    }
    
    void SetEnvelope(uint_t value)
    {
        Envelope.SetControl(value);
    }
    
    void Reset()
    {
        for (std::size_t chan = 0; chan != 3; ++chan)
        {
            Tones[chan].Reset();
            Levels[chan] = FastSample();
        }
        Noise.Reset();
        Envelope.Reset();
    }
    
    void Tick(uint_t ticks)
    {
        Tones[0].Tick(ticks);
        Tones[1].Tick(ticks);
        Tones[2].Tick(ticks);
        Noise.Tick(ticks);
        Envelope.Tick(ticks);
    }
    
    
    
    FastSample GetLevels() const
    {
        const uint_t noise = Noise.GetLevel();
        
        FastSample out;
        if ( !(generic_mute_mask&(1<<(m_voicesForceOfs+0))) &&  (noise & Tones[0].GetLevel<1>()))
        {
            out.Add(Levels[0]);
        }
        if ( !(generic_mute_mask&(1<<(m_voicesForceOfs+1))) &&  (noise & Tones[1].GetLevel<2>()))
        {
            out.Add(Levels[1]);
        }
        if ( !(generic_mute_mask&(1<<(m_voicesForceOfs+2))) &&  (noise & Tones[2].GetLevel<4>()))
        {
            out.Add(Envelope.GetLevel(Levels[2]));
        }
        return out;
    }
    
    //YOYOFR
    uint_t GetPeriod(int chan) const {
        if ((chan<0)||(chan>=3)) return 0;
        return Tones[chan].GetHalfPeriod();
    }
    
    FastSample GetLevelsChan(int chan) const
    {
        const uint_t noise = Noise.GetLevel();
        
        FastSample out;
        if ( !(generic_mute_mask&(1<<(m_voicesForceOfs+chan))) ) {
            switch (chan) {
                case 0:
                    if (noise & Tones[0].GetLevel<1>())
                    {
                        out.Add(Levels[0]);
                    }
                    
                    break;
                case 1:
                    if (noise & Tones[1].GetLevel<2>())
                    {
                        out.Add(Levels[1]);
                    }
                    break;
                case 2:
                    if (noise & Tones[2].GetLevel<4>())
                    {
                        out.Add(Envelope.GetLevel(Levels[2]));
                    }
                    break;
            }
        }
        return out;
    }
    //YOYOFR
    
    void GetState(MultiChannelState& state) const
    {
        const uint_t MAX_IN_LEVEL = 30;
        LevelType toneLevels[3];
        for (uint_t chan = 0; chan != 3; ++chan)
        {
            toneLevels[chan] = LevelType(Levels[chan].Left() + Levels[chan].Right(), MAX_IN_LEVEL);
            if (!Tones[chan].IsMasked())
            {
                state.push_back(ChannelState(2 * Tones[chan].GetHalfPeriod(), toneLevels[chan]));
            }
        }
        if (Noise.GetMixer())
        {
            const LevelType level = (toneLevels[0] + toneLevels[1] + toneLevels[2]) / 3;
            state.push_back(ChannelState(Noise.GetPeriod(), level));
        }
        if (const uint_t period = Envelope.GetRepetitionPeriod())
        {
            state.push_back(ChannelState(period, toneLevels[2]));
        }
    }
private:
    void UpdateLinkedGenerators(uint_t generator)
    {
        if (0 == generator)
        {
            Noise.SetPeriod(Tones[0].GetHalfPeriod());
        }
        else if (1 == generator)
        {
            Envelope.SetPeriod(Tones[1].GetHalfPeriod());
        }
    }
private:
    ToneGenerator Tones[3];
    NoiseGenerator Noise;
    EnvelopeGenerator Envelope;
    FastSample Levels[3];
};

class SAADevice
{
public:
    SAADevice()
    : Subdevices()
    {
    }
    
    void SetLevel(uint_t generator, uint_t left, uint_t right)
    {
        if (generator < 3)
        {
            Subdevices[0].SetLevel(generator, left, right);
        }
        else
        {
            Subdevices[1].SetLevel(generator - 3, left, right);
        }
    }
    
    void SetToneNumber(uint_t generator, uint_t number)
    {
        if (generator < 3)
        {
            Subdevices[0].SetToneNumber(generator, number);
        }
        else
        {
            Subdevices[1].SetToneNumber(generator - 3, number);
        }
    }
    
    void SetToneOctave(uint_t generator, uint_t octave)
    {
        if (generator < 3)
        {
            Subdevices[0].SetToneOctave(generator, octave);
        }
        else
        {
            Subdevices[1].SetToneOctave(generator - 3, octave);
        }
    }
    
    void SetToneMixer(uint_t mixer)
    {
        Subdevices[0].SetToneMixer(mixer);
        Subdevices[1].SetToneMixer(mixer >> 3);
    }
    
    void SetNoiseMixer(uint_t mixer)
    {
        Subdevices[0].SetNoiseMixer(mixer);
        Subdevices[1].SetNoiseMixer(mixer >> 3);
    }
    
    void SetNoiseControl(uint_t ctrl)
    {
        Subdevices[0].SetNoiseControl(LoNibble(ctrl));
        Subdevices[1].SetNoiseControl(HiNibble(ctrl));
    }
    
    void SetEnvelope(uint_t generator, uint_t value)
    {
        Subdevices[generator].SetEnvelope(value);
    }
    
    void Reset()
    {
        Subdevices[0].Reset();
        Subdevices[1].Reset();
    }
    
    void Tick(uint_t ticks)
    {
        Subdevices[0].Tick(ticks);
        Subdevices[1].Tick(ticks);
    }
    
    Sound::Sample GetLevels() const
    {
        m_voicesForceOfs=0;//YOYOFR
        FastSample out = Subdevices[0].GetLevels();
        m_voicesForceOfs=3;//YOYOFR
        out.Add(Subdevices[1].GetLevels());
        
        m_voicesForceOfs=0;//YOYOFR
        
        for (int i=0;i<6;i++) {
            float period=0;
            int vol=0;
            
            if (generic_mute_mask&(1<<(i))) {
            } else {
                if (i<3) m_voicesForceOfs=0;
                else m_voicesForceOfs=3;
                period=Subdevices[i/3].GetPeriod(i%3);
                vol=MAXVAL(Subdevices[i/3].GetLevelsChan(i%3).Left(),Subdevices[i/3].GetLevelsChan(i%3).Right());
                
                if (period && vol) {
                    float freq=m_voice_current_samplerate/period;
                    vgm_last_note[i]=freq;
                    vgm_last_instr[i]=i;
                    vgm_last_vol[i]=1;
                }
            }
        }
        m_voicesForceOfs=0;//YOYOFR
        
        return out.Convert();
    }
    
    //YOYOFR
    Sound::Sample GetLevelsChan(int chan) const
    {
        FastSample out;
        if (chan<3) {
            m_voicesForceOfs=0;
            out = Subdevices[0].GetLevelsChan(chan);
        }
        else {
            m_voicesForceOfs=3;
            out = Subdevices[1].GetLevelsChan(chan-3);
        }
        return out.Convert();
    }
    //YOYOFR
    
    void GetState(MultiChannelState& state) const
    {
        Subdevices[0].GetState(state);
        Subdevices[1].GetState(state);
    }
private:
    SAASubDevice Subdevices[2];
};
}
}
