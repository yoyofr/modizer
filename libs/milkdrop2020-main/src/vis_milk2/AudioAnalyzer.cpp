
#include <math.h>
#include <vector>
#include <functional>
#include "IAudioAnalyzer.h"

#include "fft.h"
#include "platform.h"
#include "imgui_support.h"


template<typename T>
class MovingAverage
{
public:
    
    MovingAverage(size_t size = 4096)
    :m_size(size)
    {
        m_history.resize(size);
    }
    
    
    T ComputeAverage(size_t count)
    {
        if (count >= m_count) count = m_count;
        if (count == 0) return T();
        
        return ComputeSum(count) / (float)count;
    }
    
    T ComputeSum(size_t count)
    {
        if (count >= m_count) count = m_count;
        
        
        T total = T();
        size_t pos = m_pos;
        while (count > 0)
        {
            if (pos == 0) pos = m_size;
            pos--;
            
            total += m_history[pos];
            count--;
        }
        
        return total;
    }
    
    void Write(T f)
    {
        m_count++;
        if (m_count > m_size) m_count = m_size;
        
        m_history[m_pos] = f;
        m_pos++;
        if (m_pos >= m_size)
            m_pos = 0;
    }
    
    void Reset()
    {
        m_count = 0;
        m_pos = 0;
    }
    
protected:
    size_t m_count = 0;
    size_t m_pos = 0;
    size_t m_size;
    std::vector<T> m_history;
};




struct SpectrumInfo
{
    void Update(float level, float dt)
    {
        // sum spectrum for this band
        imm = level;
        
        
        // store in moving average
        moving_avg.Write(imm);
        
        // compute average and long average
        float ave_count = (4.0f / 60.0f) / dt;
        avg      =  moving_avg.ComputeAverage((int)std::round(ave_count));   // 1/15 of a second
        float long_count = (120.0f / 60.0f) / dt;
        long_avg =  moving_avg.ComputeAverage((int)std::round(long_count)); // 2.0 seconds
        
        // compute relative to average
        imm_rel  = imm / long_avg;
        avg_rel  = avg / long_avg;
        
        
        // keep closer to 1.0 (0.7 to 1.3)
        imm_rel = sqrtf(imm_rel);
        avg_rel = sqrtf(avg_rel);

    }
    
    
    
    
    float   imm = 0.0f;            //  (absolute)
    float   imm_rel = 0.0f;        //  (relative to song; 1=avg, 0.9~below, 1.1~above)
    float   avg = 0.0f;            //  (absolute)
    float   avg_rel = 0.0f;        //  (relative to song; 1=avg, 0.9~below, 1.1~above)
    float   long_avg = 0.0f;       //  (absolute)
    MovingAverage<float>  moving_avg;
};


struct ChannelInfo
{
    std::string           name;
    SampleBuffer<float>   fWaveform;
    SampleBuffer<float>   fSpectrum;

    SpectrumInfo bands[BAND_count];
};




template<typename T>
void Resample(const SampleBuffer<T> &src, int srcCount, SampleBuffer<T> &dest, int destCount)
{
    dest.reserve(destCount);
    
    float step = (float)srcCount / (float)destCount;
    
    float pos = 0.0f;
    
    for (int i=0; i < destCount; i++)
    {
        //            float pos0 = truncf(pos);
        //            float frac = pos - pos0;
        //            int p0 = (int)pos0;
        //            int p1 = p0 + 1;
        //
        //            float f0 = frac;
        //            float f1 = 1.0f - f0;
        //
        //            T r = src[p0] * f0 + src[p1] * f1;
        T r = src.GetSample(pos);
        
        dest.push_back(r);
        
        //   printf("%f %f %f: %f %f\n", pos, pos0, frac, r.left, r.right);
        pos += step;
    }
    
}



void SplitChannels(const SampleBuffer<Sample> &samples,
                   SampleBuffer<float> &fL,
                   SampleBuffer<float> &fR
                   )
{
    int count = (int)samples.size();
    
    fL.clear();
    fR.clear();
    fL.reserve(count);
    fR.reserve(count);
    for (int i=0; i < count; i++)
    {
        const Sample &s = samples[i];
        fL.push_back(s.ch[0]);
        fR.push_back(s.ch[1]);
    }
}


void CombineChannels(const SampleBuffer<Sample> &samples,
                     SampleBuffer<float> &fL
                     )
{
    int count = (int)samples.size();
    
    fL.clear();
    fL.reserve(count);
    for (int i=0; i < count; i++)
    {
        const Sample &s = samples[i];
        float v = s.ch[0] + s.ch[1];
        fL.push_back(v);
    }
}


void Dump(SampleBuffer<float> &data)
{
    for (int i=0; i < (int)data.size(); i++)
    {
        printf("[%d] %f\n", (int)i, data[i]);
    }
}



void TestSampleBuffer()
{
    SampleBuffer<float> sb0;
    
    sb0.push_back(0.0f);
    sb0.push_back(0.5f);
    sb0.push_back(1.0f);
    sb0.push_back(1.0f);
    sb0.push_back(1.0f);
    sb0.push_back(1.0f);
    sb0.push_back(1.0f);
    sb0.push_back(1.0f);
    sb0.push_back(1.0f);
    sb0.push_back(1.0f);
    sb0.push_back(0.5f);
    sb0.push_back(0.0f);
    
    
    SampleBuffer<float> sb1;
    
    Resample(sb0, (int)sb0.size(), sb1, (int)sb0.size() * 2 );
    
    
//    for (float fpos = 0.0f; fpos < 3.0f; fpos += 0.25f)
//        printf("[%f] %f\n", fpos, sb0.GetSample(fpos));
    
    Dump(sb0);
    Dump(sb1);

//    SampleBuffer<float> sb2;
    //sb1.ApplyKernel(sb2, 0.5f, 0.5f, 0.0f , 0.0f);
    sb1.Smooth(0.05f);

    Dump(sb1);

}

class AudioAnalyzer : public IAudioAnalyzer
{
public:
    AudioAnalyzer()
    {
        m_fft = CreateFFT(true, 1.0f);

        Reset();

//        memset(m_oldwave[0], 0, sizeof(m_oldwave[0]));
//        memset(m_oldwave[1], 0, sizeof(m_oldwave[1]));
//        m_prev_align_offset[0] = 0;
//        m_prev_align_offset[1] = 0;
//        m_align_weights_ready = 0;
        
        //TestSampleBuffer();

    }
    
    virtual ~AudioAnalyzer()
    {
        delete m_fft;
    }
    
    virtual float GetSampleRate() override {
        return m_samples.GetSampleRate();
    }
    virtual int GetBlockSize() override {
        return (int)m_samples.size();
    }


    void Dump(std::vector<float> &data)
    {
        for (size_t i=0; i < data.size(); i++)
        {
            printf("[%d] %f\n", (int)i, data[i]);
        }
    }
    
    
    void Dump(std::vector<Sample> &data)
    {
        for (size_t i=0; i < data.size(); i++)
        {
            printf("[%d] %f %f\n", (int)i, data[i].ch[0], data[i].ch[1]);
        }
    }

    std::string m_description;
    
    virtual void Update(float dt, const SampleBuffer<Sample> &samples) override
    {
        PROFILE_FUNCTION();

        // get audio data
        m_samples = samples;

        
        m_sampleRate = m_samples.GetSampleRate();
        m_dt = dt;

//        // append to sample history
//        m_sample_history.insert(m_sample_history.end(), samples.begin(), samples.end());
//
//
//        size_t maxSampleHistory = 4096;
//        if (m_sample_history.size() > maxSampleHistory)
//        {
//            size_t remove = m_sample_history.size() - maxSampleHistory;
//            m_sample_history.erase(m_sample_history.begin(), m_sample_history.begin() + remove);
//        }
        
        
        SplitChannels(m_samples,  m_channel[0].fWaveform, m_channel[1].fWaveform );

        
        
//        Modulate(fWaveform[1], 128.0f);
//
//
//
//        ApplyKernel(temp_wave[1], fWaveform[1], 0.5f, 0.5f, 0.0f, 0.0f);
//
//        int old_i = 0;
//        float temp_wave[2][MAX_SAMPLES];
//        for (int i = 0; i < count; i++)
//        {
//            // damp the input into the FFT a bit, to reduce high-frequency noise:
//            temp_wave[0][i] = 0.5f*(fWaveform[0][i] + fWaveform[0][old_i]);
//            temp_wave[1][i] = 0.5f*(fWaveform[1][i] + fWaveform[1][old_i]);
//            old_i = i;
//        }
        
//        fSpectrum[0].resize(NUM_FREQUENCIES);




        
        
        SampleBuffer<float> temp_wave[2];
        
        for (int ch=0; ch < NUM_CHANNELS; ch++)
        {
            m_channel[ch].fWaveform.Modulate(128.0f);
            
            m_channel[ch].fWaveform.ApplyKernel(temp_wave[ch], 0.5f, 0.5f, 0.0f, 0.0f);
//            SimpleSmooth(temp_wave[i], fWaveform[i]);
            
            time_to_frequency_domain(ch, temp_wave[ch], m_channel[ch].fSpectrum );
        }
        
//        Dump(temp_wave[0]);
//        Dump(twave);
        
        
       // DoCustomSoundAnalysis(dt);    // emulates old pre-vms milkdrop sound analysis
        
        
        
        
        float freqs[4] = {0.0f, 761.0f,  2897.0f, 11025.0f};
        
        // for each band
        for (int ch=0; ch < NUM_CHANNELS; ch++)
        {
            ChannelInfo &channel = m_channel[ch];
            

            int freq_index[4];
            for (int i=0; i < 4; i++) {
                freq_index[i] = FrequencyToIndex(freqs[i], channel.fSpectrum.size());
            }

            
            for (int i=0; i < BAND_count; i++)
            {
                SpectrumInfo &info = channel.bands[i];

                
                float level = AveFrequencyRange(channel.fSpectrum, freqs[i], freqs[i+1]);
                info.Update(level, dt);
            }
        }
        
        
//       SpectrumInfo &info = m_spectrum_band[0][0];
//      LogPrint("%2d: imm[%f] avg[%f] imm_rel[%f]  avg_rel[%f]  long_avg[%f]\n",
//                                           0,
//                                           info.imm,
//                                           info.avg,
//                                           info.imm_rel,
//                                           info.avg_rel,
//                                           info.long_avg
//                                           );
        
    }
    
    
    void AlignWaves();

    
//    void DoCustomSoundAnalysis(float dt);

    virtual float GetImm(AudioBand band) override
    {
        return m_channel[0].bands[band].imm;
    }

    virtual float GetImmRel(AudioBand band) override
    {
        return m_channel[0].bands[band].imm_rel;
    }
    
    virtual float GetAvgRel(AudioBand band) override
    {
        return m_channel[0].bands[band].avg_rel;
    }
    
    
    virtual void GetRawSamples(std::vector<Sample> &samples) override
    {
        samples = m_samples.GetVector();
    }
    
    

    virtual void RenderWaveForm(
                                int count,
                                SampleBuffer<float> &fL,
                                SampleBuffer<float> &fR
                                ) override
    {
        SampleBuffer<Sample> samples;
        Resample(m_samples, (int)m_samples.size(), samples, count);
        SplitChannels(samples, fL, fR);
    }
    
    
    
    virtual void RenderWaveForm(
                                int count,
                                SampleBuffer<Sample> &samples
                                ) override
    {
        Resample(m_samples, (int)m_samples.size(), samples, count);
    }
    
    virtual void RenderSpectrum(int sampleCount, SampleBuffer<float> &sL, SampleBuffer<float> &sR)  override
    {
        Resample(m_channel[0].fSpectrum, (int)m_channel[0].fSpectrum.size(), sL, sampleCount);
        Resample(m_channel[1].fSpectrum, (int)m_channel[1].fSpectrum.size(), sR, sampleCount);
    }

    
    
    virtual void RenderSpectrum(int sampleCount, SampleBuffer<float> &spectrum)  override
    {
        Resample(m_channel[0].fSpectrum, (int)m_channel[0].fSpectrum.size(), spectrum, sampleCount);

        /*
        
        spectrum.clear();
        spectrum.reserve(sampleCount);
        for (int i=0; i < sampleCount; i++)
        {
//            float s =  (fSpectrum[0][i] + fSpectrum[1][i]) * 0.5f;
            float s =  (fSpectrum[0][i] + fSpectrum[1][i]) * 0.5f;

            spectrum.push_back(s);
        }
         */
    }
    
    float SumRange(const SampleBuffer<float> &data, int start, int end)
    {
        float total = 0.0f;
        for (int i=start; i < end; i++)
        {
            total += data[i];
        }
        return total;
    }
    
    float AveRange(const SampleBuffer<float> &data, int start, int end)
    {
        int count = (int)end - (int)start;
        if (count <= 0) return 0.0f;
        
        float total = 0.0f;
        for (int i=start; i < end; i++)
        {
            total += data[i];
            
        }
        return total / (float)count;
    }
    
    int FrequencyToIndex(float freq, size_t num_freq)
    {
        float sampleRate = (float)m_sampleRate;
        
        float maxFreq = (sampleRate / 2.0f);
        
        int index = (int)floorf(freq * num_freq / maxFreq);
        
        if (index < 0) index = 0;
        if (index >= num_freq) index = (int)num_freq - 1;
        return index;
    }
    
    float IndexToFrequency(int index, size_t num_freq)
       {
           float sampleRate = (float)m_sampleRate;
           
           float maxFreq = (sampleRate / 2.0f);
           
           
           float freq = (float)index * (float)maxFreq / (float)num_freq;
           
           if (freq < 0) freq = 0;
           if (freq >= maxFreq) freq = maxFreq;
           return freq;
       }
       
    
    float SumFrequencyRange(const SampleBuffer<float> &data, float freq_start, float freq_end)
    {
        int i0 = FrequencyToIndex(freq_start, data.size());
        int i1 = FrequencyToIndex(freq_end, data.size());
        return SumRange(data, i0, i1);
    }
    
    
    float AveFrequencyRange(const SampleBuffer<float> &data, float freq_start, float freq_end)
    {
        int i0 = FrequencyToIndex(freq_start, data.size());
        int i1 = FrequencyToIndex(freq_end, data.size());
        return AveRange(data, i0, i1);
    }
    
    virtual void time_to_frequency_domain(int ch, const SampleBuffer<float> &in_wavedata, SampleBuffer<float> &out_spectraldata)
    {
       out_spectraldata.resize(NUM_FREQUENCIES);
 //       out_spectraldata.resize(2048);

        m_fft->time_to_frequency_domain(in_wavedata.data(), (int)in_wavedata.size(), out_spectraldata.data(), (int)out_spectraldata.size());
    }

    
    virtual void DrawChannelUI(int ch) override;

    
    static constexpr int NUM_CHANNELS = 2;
    
    
    IFFT *         m_fft = nullptr;
    
    float m_sampleRate = 0;
    float m_dt = 0;
    SampleBuffer<Sample>         m_samples;
    
    ChannelInfo                 m_channel[NUM_CHANNELS];
    
    
    virtual void Reset() override
    {
        m_samples.clear();
        for (int i=0; i < NUM_CHANNELS; i++)
        {
            m_channel[i] = ChannelInfo();
        }
        m_channel[0].name = "left";
        m_channel[1].name = "right";
    }
    
    
//    float m_oldwave[2][NUM_SAMPLES];        // for wave alignment
//    int   m_prev_align_offset[2];   // for wave alignment
//    int   m_align_weights_ready;

    
};

IAudioAnalyzerPtr CreateAudioAnalyzer()
{
    return std::make_shared<AudioAnalyzer>();
}



#if 0
void AudioAnalyzer::DoCustomSoundAnalysis(float dt)
{
//    // do our own [UN-NORMALIZED] fft
//    float scale = 128.0f;
//    float fWaveLeft[MAX_SAMPLES];
//    for (int i=0; i < m_samples.size(); i++)
//        fWaveLeft[i] = m_samples[i].ch[0] * scale;
//    //fWaveLeft[i] = m_sound.fWaveform[0][i];
//
//    memset(fSpecLeft, 0, sizeof(fSpecLeft));
//
//    myfft->time_to_frequency_domain(fWaveLeft, (int)m_samples.size(), fSpecLeft, NUM_FREQUENCIES);
//
    
    

#if 0
    float fps = 1.0f / dt;

    // sum spectrum up into 3 bands
    for (int i=0; i<BAND_count; i++)
    {
        // note: only look at bottom half of spectrum!  (hence divide by 6 instead of 3)
        int start = NUM_FREQUENCIES*i/6;
        int end   = NUM_FREQUENCIES*(i+1)/6;
        int j;
        
        info.imm[i] = 0;
        
        for (j=start; j<end; j++)
            info.imm[i] += info.fSpecLeft[j];
    }
    
    // do temporal blending to create attenuated and super-attenuated versions
    for (int i=0; i<BAND_count; i++)
    {
        float rate;
        
        if (info.imm[i] > info.avg[i])
            rate = 0.2f;
        else
            rate = 0.5f;
        rate = AdjustRateToFPS(rate, 30.0f, fps);
        info.avg[i] = info.avg[i]*rate + info.imm[i]*(1-rate);
        
        rate = 0.992f;
        rate = AdjustRateToFPS(rate, 30.0f, fps);
        info.long_avg[i] = info.long_avg[i]*rate + info.imm[i]*(1-rate);
        
        
        // also get bass/mid/treble levels *relative to the past*
        if (fabsf(info.long_avg[i]) < 0.001f)
            info.imm_rel[i] = 1.0f;
        else
            info.imm_rel[i]  = info.imm[i] / info.long_avg[i];
        
        if (fabsf(info.long_avg[i]) < 0.001f)
            info.avg_rel[i]  = 1.0f;
        else
            info.avg_rel[i]  = info.avg[i] / info.long_avg[i];
        
        //  info.imm_rel[i] = sqrtf(info.imm_rel[i]);
        //    info.avg_rel[i] = sqrtf(info.avg_rel[i]);
    }

#else
    
    
    float freqs[4] = {0.0f, 761.0f,  2897.0f, 11025.0f};
    
    // for each band
    for (int i=0; i < BAND_count; i++)
    {
        // sum spectrum for this band
        info.imm[i] = AveFrequencyRange(fSpectrum[0], freqs[i], freqs[i+1]);
        
        // store in moving average
        m_sound_avg[i].Write(info.imm[i]);

        // compute average and long average
        info.avg[i]      =  m_sound_avg[i].ComputeAverage(4);
        info.long_avg[i] =  m_sound_avg[i].ComputeAverage(120);
        
        // compute relative to average
        info.imm_rel[i]  = info.imm[i] / info.long_avg[i];
        info.avg_rel[i]  = info.avg[i] / info.long_avg[i];
        
        
        // keep closer to 1.0 (0.7 to 1.3)
        info.imm_rel[i] = sqrtf(info.imm_rel[i]);
        info.avg_rel[i] = sqrtf(info.avg_rel[i]);
    }
#endif

    
    //    info.imm_rel[0]  = 2.0f;
    //    info.imm_rel[1]  = 0.0f;
    //    info.imm_rel[2]  = 0.0f;
    //
    //    info.avg_rel[0]  = 1.0f;
    //    info.avg_rel[1]  = 1.0f;
    //    info.avg_rel[2]  = 1.0f;

    
//        LogPrint("%2d: imm[%f %f %f] avg[%f %f %f] imm_rel[%f %f %f]  avg_rel[%f %f %f]\n",
//                 0,
//                 info.imm[0], info.imm[1], info.imm[2],
//                 info.avg[0], info.avg[1], info.avg[2],
//                 info.imm_rel[0], info.imm_rel[1], info.imm_rel[2],
//                 info.avg_rel[0], info.avg_rel[1], info.avg_rel[2]
//                 );
//

    
//    LogPrint("%2d: imm[%f] avg[%f] long_avg[%f] imm_rel[%f] avg_rel[%f] moving_ave[%f %f %f %f]\n",
//             0,
//             info.imm[0],
//             info.avg[0],
//             info.long_avg[0],
//             info.imm_rel[0],
//             info.avg_rel[0],
//             m_sound_avg[0].ComputeAverage(4),
//             m_sound_avg[0].ComputeAverage(10),
//             m_sound_avg[0].ComputeAverage(60),
//             m_sound_avg[0].ComputeAverage(120)
//             );

}

#endif





#if 0
void AudioAnalyzer::AnalyzeNewSound(const Sample *samples, int sampleCount, float fps)
{
    // we get NUM_SAMPLES samples in from winamp.
    // the output of the fft has 'num_frequencies' samples,
    //   and represents the frequency range 0 hz - 22,050 hz.
    // usually, plugins only use half of this output (the range 0 hz - 11,025 hz),
    //   since >10 khz doesn't usually contribute much.
    
    float temp_wave[2][NUM_SAMPLES];
    
    float scale = 128.0f;
    int old_i = 0;
    for (int i = 0; i<NUM_SAMPLES; i++)
    {
        fWaveform[0][i] = samples[i].left * scale;
        fWaveform[1][i] = samples[i].right * scale;
        
        // simulating single frequencies from 200 to 11,025 Hz:
        //float freq = 1.0f + 11050*(GetFrame() % 100)*0.01f;
        //fWaveform[0][i] = 10*sinf(i*freq*6.28f/44100.0f);
        
        //     float freq = 800;
        //     fWaveform[0][i] = 10*sinf(i*freq*6.28f/44100.0f);
        //      fWaveform[1][i] =  fWaveform[0][i];
        
        // damp the input into the FFT a bit, to reduce high-frequency noise:
        temp_wave[0][i] = 0.5f*(fWaveform[0][i] + fWaveform[0][old_i]);
        temp_wave[1][i] = 0.5f*(fWaveform[1][i] + fWaveform[1][old_i]);
        old_i = i;
    }
    
    myfft->time_to_frequency_domain(0, temp_wave[0], fSpectrum[0]);
    myfft->time_to_frequency_domain(1, temp_wave[1], fSpectrum[1]);
    
}
#endif


#if 0
void AudioAnalyzer::AlignWaves()
{
    // align waves, using recursive (mipmap-style) least-error matching
    // note: NUM_WAVEFORM_SAMPLES must be between 32 and NUM_SAMPLES.
    
    int align_offset[2] = { 0, 0 };

    int nSamples = NUM_WAVEFORM_SAMPLES;

    if (nSamples >=  NUM_SAMPLES)
        return;
    
    
#define MAX_OCTAVES 10
    int octaves = (int)floorf(logf((float)(NUM_SAMPLES - nSamples)) / logf(2.0f));
    if (octaves < 4)
        return;
    if (octaves > MAX_OCTAVES)
        octaves = MAX_OCTAVES;
    
    for (int ch = 0; ch<2; ch++)
    {
        // only worry about matching the lower 'nSamples' samples
        float temp_new[MAX_OCTAVES][NUM_SAMPLES];
        float temp_old[MAX_OCTAVES][NUM_SAMPLES];
        static float temp_weight[MAX_OCTAVES][NUM_SAMPLES];
        static int   first_nonzero_weight[MAX_OCTAVES];
        static int   last_nonzero_weight[MAX_OCTAVES];
        int spls[MAX_OCTAVES];
        int space[MAX_OCTAVES];
        
        memcpy(temp_new[0], fWaveform[ch], sizeof(float) * NUM_SAMPLES);
        memcpy(temp_old[0], &m_oldwave[ch][m_prev_align_offset[ch]], sizeof(float)*nSamples);
        spls[0] = NUM_SAMPLES;
        space[0] = NUM_SAMPLES - nSamples;
        
        // potential optimization: could reuse (instead of recompute) mip levels for m_oldwave[2][]?
        for (int octave = 1; octave<octaves; octave++)
        {
            spls[octave] = spls[octave - 1] / 2;
            space[octave] = space[octave - 1] / 2;
            for (int n = 0; n<spls[octave]; n++)
            {
                temp_new[octave][n] = 0.5f*(temp_new[octave - 1][n * 2] + temp_new[octave - 1][n * 2 + 1]);
                temp_old[octave][n] = 0.5f*(temp_old[octave - 1][n * 2] + temp_old[octave - 1][n * 2 + 1]);
            }
        }
        
        if (!m_align_weights_ready)
        {
            m_align_weights_ready = 1;
            for (int octave = 0; octave<octaves; octave++)
            {
                int compare_samples = spls[octave] - space[octave];
                for (int n = 0; n<compare_samples; n++)
                {
                    // start with pyramid-shaped pdf, from 0..1..0
                    if (n < compare_samples / 2)
                        temp_weight[octave][n] = n * 2 / (float)compare_samples;
                    else
                        temp_weight[octave][n] = (compare_samples - 1 - n) * 2 / (float)compare_samples;
                    
                    // TWEAK how much the center matters, vs. the edges:
                    temp_weight[octave][n] = (temp_weight[octave][n] - 0.8f)*5.0f + 0.8f;
                    
                    // clip:
                    if (temp_weight[octave][n]>1) temp_weight[octave][n] = 1;
                    if (temp_weight[octave][n]<0) temp_weight[octave][n] = 0;
                }
                
                int n = 0;
                while (temp_weight[octave][n] == 0 && n < compare_samples)
                    n++;
                first_nonzero_weight[octave] = n;
                
                n = compare_samples - 1;
                while (temp_weight[octave][n] == 0 && n >= 0)
                    n--;
                last_nonzero_weight[octave] = n;
            }
        }
        
        int n1 = 0;
        int n2 = space[octaves - 1];
        for (int octave = octaves - 1; octave >= 0; octave--)
        {
            // for example:
            //  space[octave] == 4
            //  spls[octave] == 36
            //  (so we test 32 samples, w/4 offsets)
            //int compare_samples = spls[octave]-space[octave];
            
            int lowest_err_offset = -1;
            float lowest_err_amount = 0;
            for (int n = n1; n<n2; n++)
            {
                float err_sum = 0;
                //for (int i=0; i<compare_samples; i++)
                for (int i = first_nonzero_weight[octave]; i <= last_nonzero_weight[octave]; i++)
                {
                    float x = (temp_new[octave][i + n] - temp_old[octave][i]) * temp_weight[octave][i];
                    if (x>0)
                        err_sum += x;
                    else
                        err_sum -= x;
                }
                
                if (lowest_err_offset == -1 || err_sum < lowest_err_amount)
                {
                    lowest_err_offset = n;
                    lowest_err_amount = err_sum;
                }
            }
            
            // now use 'lowest_err_offset' to guide bounds of search in next octave:
            //  space[octave] == 8
            //  spls[octave] == 72
            //     -say 'lowest_err_offset' was 2
            //     -that corresponds to samples 4 & 5 of the next octave
            //     -also, expand about this by 2 samples?  YES.
            //  (so we'd test 64 samples, w/8->4 offsets)
            if (octave > 0)
            {
                n1 = lowest_err_offset * 2 - 1;
                n2 = lowest_err_offset * 2 + 2 + 1;
                if (n1 < 0) n1 = 0;
                if (n2 > space[octave - 1]) n2 = space[octave - 1];
            }
            else
                align_offset[ch] = lowest_err_offset;
        }
    }
    
    memcpy(m_oldwave[0], fWaveform[0], sizeof(m_oldwave[0]));
    memcpy(m_oldwave[1], fWaveform[1], sizeof(m_oldwave[1]));
    m_prev_align_offset[0] = align_offset[0];
    m_prev_align_offset[1] = align_offset[1];
    
    // finally, apply the results: modify fWaveform[2][0..NUM_SAMPLES]
    // by scooting the aligned samples so that they start at fWaveform[2][0].
    for (int ch = 0; ch<2; ch++)
        if (align_offset[ch]>0)
        {
            for (int i = 0; i<nSamples; i++)
                fWaveform[ch][i] = fWaveform[ch][i + align_offset[ch]];
            // zero the rest out, so it's visually evident that these samples are now bogus:
            memset(&fWaveform[ch][nSamples], 0, (NUM_SAMPLES - nSamples)*sizeof(float));
        }
}

#endif


void AudioAnalyzer::DrawChannelUI(int ch)
{
    if (ch >= NUM_CHANNELS) return;
    
//    for (int ch=0; ch < NUM_CHANNELS; ch++)
    {
   //     const auto &wfd = fWaveform[ch];
        

        
        
        const ChannelInfo &channel = m_channel[ch];
        

        const char *name = channel.name.c_str();
       
        float scale = 8;
        float h = 120;
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f,1.0f,0.2f,1.0f));
//            ImGui::PlotHistogram("", wfd.data(), (int)wfd.size(), 0, "waveform", -128.0f, 128.0f, ImVec2(256, 120));
        
        
        /*
        ImGui::PlotHistogram("", &m_samples.data()->ch[ch], (int)m_samples.size(), 0,
                             "samples", -1.0f, 1.0f,
                             ImVec2(256, 120),
                             sizeof(m_samples[0])
                             );
         */
        
        ImGui::PlotHistogram("",
                      [this, ch](int idx) {return m_samples[idx].ch[ch];},
                      (int)m_samples.size(), 0,
                      name, -1.0f, 1.0f,
                      ImVec2(256, 120)
                    );
                      
                      

        ImGui::PopStyleColor();

        ImGui::SameLine();

        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f,0.4f,0.6f,1.0f));
//            ImGui::PlotHistogram("", sfd.data(), (int)sfd.size(), 0, "spectrum", 0.0f, 8.0f, ImVec2(256, 120));

        
        

        int numSamples = (int)(channel.fSpectrum.size()) / 2;
        float spec_width = 256;
        ImGui::PlotHistogram("",
                      channel.fSpectrum.data(),
                      numSamples, 0,
                      "spectrum", 0.0f, scale,
                      ImVec2(spec_width, h)
                      );
        
             if (ImGui::IsItemHovered())
             {
                 auto rmin = ImGui::GetItemRectMin();
                 auto rmax = ImGui::GetItemRectMax();
                 ImVec2 mouse_pos = ImGui::GetIO().MousePos;
                 float t = ((mouse_pos.x - rmin.x) / (rmax.x - rmin.x));
                 t = std::min(t, 0.9999f);
                 t = std::max(t, 0.0f);

                 const int v_idx = (int)(t * numSamples);
                 
                 float freq = IndexToFrequency(v_idx, numSamples );
                 ImGui::SetTooltip("frequency: %.1f", (float) freq);
             }
             
        
        
        ImGui::PopStyleColor();
        

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f,0.0f,0.2f,1.0f));

        float data[4][BAND_count];

        int count = BAND_count;
        
        for (int i=0; i < count; i++){
          
            data[0][i] = channel.bands[i].imm;
            data[1][i] = channel.bands[i].imm_rel;
            data[2][i] = channel.bands[i].avg_rel;
            data[3][i] = channel.bands[i].long_avg;
        }
        
        
        
         float w = 20.0f;
       
        
        ImGui::SameLine();
        ImGui::PlotHistogram("", data[0], count, 0, "imm", 0.0f, scale, ImVec2(count * w, h));
        ImGui::SameLine();
        ImGui::PlotHistogram("", data[1], count, 0, "imm_rel", 0.0f, scale, ImVec2(count * w, h));
        ImGui::SameLine();
        ImGui::PlotHistogram("", data[2], count, 0, "avg_rel", 0.0f, scale, ImVec2(count * w, h));
        ImGui::SameLine();
        ImGui::PlotHistogram("", data[3], count, 0, "long_avg", 0.0f, scale, ImVec2(count * w, h));

//
//        ImGui::PlotHistogram("",
//                      [this, channel](int idx) {return channel.bands[idx].imm;},
//                      BAND_count, 0,
//                      "imm", 0.0f, scale,
//                      ImVec2(BAND_count * w, h)
//                      );
//
        
        
        
//            for (int i=0; i < BAND_count; i++) data[i] = m_spectrum_band[ch][i].imm_rel;
//
//        ImGui::PlotHistogram("",
//                      [this, channel](int idx) {return channel.bands[idx].imm_rel;},
//                      BAND_count, 0,
//                      "imm_rel", 0.0f, scale,
//                      ImVec2(BAND_count * w, h)
//                      );
//
//
////            ImGui::PlotHistogram("", data, BAND_count, 0, "imm_rel", 0.0f, 8.0f, ImVec2(BAND_count * w, 120));
//
////            for (int i=0; i < BAND_count; i++) data[i] = m_spectrum_band[ch][i].avg_rel;
//        ImGui::SameLine();
//
//        ImGui::PlotHistogram("",
//                     [this, channel](int idx) {return channel.bands[idx].avg_rel;},
//                      BAND_count, 0,
//                      "avg_rel", 0.0f, scale,
//                      ImVec2(BAND_count * w, h)
//                      );
//
////          ImGui::PlotHistogram("", data, BAND_count, 0, "avg_rel", 0.0f, 8.0f, ImVec2(BAND_count * w, 120));
//
////        for (int i=0; i < BAND_count; i++) data[i] = m_spectrum_band[ch][i].long_avg;
//        ImGui::SameLine();
//
//        ImGui::PlotHistogram("",
//                       [this, channel](int idx) {return channel.bands[idx].long_avg;},
//                      BAND_count, 0,
//                      "long_avg", 0.0f, scale,
//                      ImVec2(BAND_count * w, h)
//                      );
//
        ImGui::PopStyleColor();


        
  //      ImGui::PlotHistogram("", data, BAND_count, 0, "long_avg", 0.0f, 8.0f, ImVec2(BAND_count * w, 120));

    }



}

