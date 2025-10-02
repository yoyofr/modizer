
#pragma once

#include <stdio.h>


#include "audio/IAudioSource.h"
#include "render/MDcontext.h"

//static constexpr int NUM_SAMPLES = 576;
//static constexpr int NUM_SAMPLES = 735;
static constexpr int MAX_SAMPLES = 1024;

static constexpr int NUM_WAVEFORM_SAMPLES = 480;    // RANGE: 32-576.  This is the # of samples of waveform data that you want.
//   Note that if it is less than 576, then VMS will do its best
//   to line up the waveforms from frame to frame for you, using
//   the extra samples as 'squish' space.
// Note: the more 'slush' samples you leave, the better the alignment
//   will be.  512 samples gives you decent alignment; 400 samples
//   leaves room for fantastic alignment.
// Observe that if you specify a value here (say 400) and then only
//   render a sub-portion of that in some cases (say, 200 samples),
//   make sure you render the *middle* 200 samples (#100-300), because
//   the alignment happens *mostly at the center*.
static constexpr int NUM_FREQUENCIES = 512;


enum AudioBand
{
    BAND_BASS = 0,
    BAND_MID = 1,
    BAND_TREBLE = 2,

    BAND_count = 3
};

class IAudioAnalyzer
{
public:
    virtual ~IAudioAnalyzer() = default;
    
    virtual void Reset() = 0;
    
    virtual void Update(float dt, const SampleBuffer<Sample> &samples) = 0;

    virtual float GetSampleRate() = 0;
    virtual int GetBlockSize() = 0;

    virtual float GetImm(AudioBand band) = 0;
    virtual float GetImmRel(AudioBand band) = 0;
    virtual float GetAvgRel(AudioBand band) = 0;

    
    virtual void GetRawSamples(std::vector<Sample> &samples) = 0;


    virtual void RenderWaveForm(
                                int count,
                                SampleBuffer<float> &fL, SampleBuffer<float> &fR
                                ) = 0;
    
    virtual void RenderWaveForm(
                                int count,
                                SampleBuffer<Sample> &samples
                                ) = 0;
    
    virtual void RenderSpectrum(int sampleCount, SampleBuffer<float> &fSpectrum) = 0;
    virtual void RenderSpectrum(int sampleCount, SampleBuffer<float> &sL, SampleBuffer<float> &sR) = 0;


    virtual void DrawChannelUI(int ch) = 0;
};


using IAudioAnalyzerPtr = std::shared_ptr<IAudioAnalyzer>;


extern IAudioAnalyzerPtr CreateAudioAnalyzer();

