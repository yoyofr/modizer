/*
  LICENSE
  -------
Copyright 2005-2013 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <math.h>
#include <memory.h>
#include "fft.h"
#include <assert.h>
#include "platform.h"
#include <complex>

class FFT : public IFFT
{
public:
    FFT(bool bEqualize, float envelope_power);
    virtual ~FFT();
    virtual void time_to_frequency_domain(const float *in_wavedata, int in_count, float *out_spectraldata, int out_count) override;
    int  GetNumFreq() { return m_NFREQ; };
private:
    int m_NFREQ;
    float m_envelope_power;
    bool m_bEqualize;
    
    void InitEnvelopeTable(float power, int count);
    void InitEqualizeTable(int count);
    void InitBitRevTable(int nfreq);
    
    std::vector<int>   bitrevtable;
    std::vector<float> envelope;
    std::vector<float> equalize;
};


IFFT *CreateFFT(bool bEqualize, float envelope_power)
{
    auto fft = new FFT(bEqualize, envelope_power);
    return fft;
}

/*****************************************************************************/

FFT::~FFT()
{
}

/*****************************************************************************/

FFT::FFT(bool bEqualize, float envelope_power)
{
    // samples_in: # of waveform samples you'll feed into the FFT
    // samples_out: # of frequency samples you want out; MUST BE A POWER OF 2.
    // bEqualize: set to 1 if you want the magnitude of the basses and trebles
    //   to be roughly equalized; 0 to leave them untouched.
    // envelope_power: set to -1 to disable the envelope; otherwise, specify
    //   the envelope power you want here.  See InitEnvelopeTable for more info.

    m_envelope_power = envelope_power;
    m_bEqualize = bEqualize;
    m_NFREQ = 0;

}

/*****************************************************************************/

void FFT::InitEqualizeTable(int count)
{
    float scaling = -0.02f;
    float inv_half_nfreq = 1.0f/(float)(count);

    equalize.resize(count);

    for (int i=0; i<count; i++)
    {
        equalize[i] = scaling * logf( (float)(count-i)*inv_half_nfreq );
      //  printf("[%d] %f\n", i, equalize[i]);
    }
}

/*****************************************************************************/

void FFT::InitEnvelopeTable(float power, int count)
{
    // this precomputation is for multiplying the waveform sample 
    // by a bell-curve-shaped envelope, so we don't see the ugly 
    // frequency response (oscillations) of a square filter.

    // a power of 1.0 will compute the FFT with exactly one convolution.
    // a power of 2.0 is like doing it twice; the resulting frequency
    //   output will be smoothed out and the peaks will be "fatter".
    // a power of 0.5 is closer to not using an envelope, and you start
    //   to get the frequency response of the square filter as 'power'
    //   approaches zero; the peaks get tighter and more precise, but
    //   you also see small oscillations around their bases.

    float mult = 1.0f/(float)count * M_PI * 2;

    envelope.resize(count);

    if (power==0.0f)
    {
        for (int i=0; i<count; i++)
            envelope[i] = 1.0f;
    }
    else
    if (power==1.0f)
        for (int i=0; i<count; i++)
            envelope[i] = 0.5f + 0.5f*sinf(i*mult - (float)M_PI_2);
    else
        for (int i=0; i<count; i++)
            envelope[i] = powf(0.5f + 0.5f*sinf(i*mult - (float)M_PI_2), power);
}

/*****************************************************************************/

void FFT::InitBitRevTable(int nfreq)
{
    bitrevtable.resize(nfreq);

    for (int i=0; i<nfreq; i++)
        bitrevtable[i] = i;

    for (int i=0,j=0; i < nfreq; i++)
    {
        if (j > i) 
        {
            std::swap(bitrevtable[i], bitrevtable[j]);        
        }
        
        int  m = nfreq >> 1;
        
        while (m >= 1 && j >= m) 
        {
            j -= m;
            m >>= 1;
        }

        j += m;
    }
    m_NFREQ = nfreq;
}

/*****************************************************************************/

using Complex = std::complex<float>;

#if 1
void FFT::time_to_frequency_domain(const float *in_wavedata, int in_count, float *out_spectraldata, int out_count)
{
    PROFILE_FUNCTION()

    // Converts time-domain samples from in_wavedata[]
    //   into frequency-domain samples in out_spectraldata[].
    // The array lengths are the two parameters to Init().
    
    // The last sample of the output data will represent the frequency
    //   that is 1/4th of the input sampling rate.  For example,
    //   if the input wave data is sampled at 44,100 Hz, then the last
    //   sample of the spectral data output will represent the frequency
    //   11,025 Hz.  The first sample will be 0 Hz; the frequencies of
    //   the rest of the samples vary linearly in between.
    // Note that since human hearing is limited to the range 200 - 20,000
    //   Hz.  200 is a low bass hum; 20,000 is an ear-piercing high shriek.
    // Each time the frequency doubles, that sounds like going up an octave.
    //   That means that the difference between 200 and 300 Hz is FAR more
    //   than the difference between 5000 and 5100, for example!
    // So, when trying to analyze bass, you'll want to look at (probably)
    //   the 200-800 Hz range; whereas for treble, you'll want the 1,400 -
    //   11,025 Hz range.
    // If you want to get 3 bands, try it this way:
    //   a) 11,025 / 200 = 55.125
    //   b) to get the number of octaves between 200 and 11,025 Hz, solve for n:
    //          2^n = 55.125
    //          n = log 55.125 / log 2
    //          n = 5.785
    //   c) so each band should represent 5.785/3 = 1.928 octaves; the ranges are:
    //          1) 200 - 200*2^1.928                    or  200  - 761   Hz
    //          2) 200*2^1.928 - 200*2^(1.928*2)        or  761  - 2897  Hz
    //          3) 200*2^(1.928*2) - 200*2^(1.928*3)    or  2897 - 11025 Hz

    // A simple sine-wave-based envelope is convolved with the waveform
    //   data before doing the FFT, to emeliorate the bad frequency response
    //   of a square (i.e. nonexistent) filter.

    // You might want to slightly damp (blur) the input if your signal isn't
    //   of a very high quality, to reduce high-frequency noise that would
    //   otherwise show up in the output.

    
    
    int nfreq = out_count * 2;
    if (nfreq != m_NFREQ) {
        InitBitRevTable(nfreq);
    }
    
    if (equalize.size() != (size_t)out_count)
    {
        InitEqualizeTable(out_count);
    }

    if (envelope.size() != (size_t)in_count)
    {
        InitEnvelopeTable(m_envelope_power, in_count);
    }

    
	constexpr int MAX = 8192;
	assert(m_NFREQ <= MAX);
    Complex array[MAX];
    
    

    // 1. set up input to the fft
    for (int i=0; i<m_NFREQ; i++)
    {
        array[i] = Complex(0,0);

        int idx = bitrevtable[i];
        if (idx < in_count)
            array[i] = Complex(in_wavedata[idx], 0);

        if (idx < in_count)
            array[i] *= envelope[idx];
    }
    
    // 2. perform FFT
    int dftsize = 2;
    int t = 0;
    while (dftsize <= m_NFREQ)
    {
        float theta = (float)(-2.0f * M_PI/(float)dftsize);
        
        Complex wp = Complex(cosf(theta), sinf(theta));
        Complex w = Complex(1.0f, 0);
        int hdftsize = dftsize >> 1;

        for (int m = 0; m < hdftsize; m+=1)
        {
            for (int i = m; i < m_NFREQ; i+=dftsize)
            {
                int j = i + hdftsize;
                Complex temp = w * array[j];
                array[j] = array[i] - temp;
                array[i] += temp;
            }

            w *= wp;
        }

        dftsize <<= 1;
        t++;
    }

    
    // 3. take the magnitude & equalize it (on a log10 scale) for output
    for (int i=0; i< out_count; i++)
        out_spectraldata[i] = sqrtf(array[i].real()*array[i].real() + array[i].imag()*array[i].imag());

    if (m_bEqualize)
    {
        for (int i=0; i< out_count; i++)
            out_spectraldata[i] *= equalize[i];
    }
}

#else
void FFT::time_to_frequency_domain(const float *in_wavedata, int in_count, float *out_spectraldata, int out_count)
{
    PROFILE_FUNCTION()

    // Converts time-domain samples from in_wavedata[]
    //   into frequency-domain samples in out_spectraldata[].
    // The array lengths are the two parameters to Init().
    
    // The last sample of the output data will represent the frequency
    //   that is 1/4th of the input sampling rate.  For example,
    //   if the input wave data is sampled at 44,100 Hz, then the last 
    //   sample of the spectral data output will represent the frequency
    //   11,025 Hz.  The first sample will be 0 Hz; the frequencies of 
    //   the rest of the samples vary linearly in between.
    // Note that since human hearing is limited to the range 200 - 20,000
    //   Hz.  200 is a low bass hum; 20,000 is an ear-piercing high shriek.
    // Each time the frequency doubles, that sounds like going up an octave.
    //   That means that the difference between 200 and 300 Hz is FAR more
    //   than the difference between 5000 and 5100, for example!
    // So, when trying to analyze bass, you'll want to look at (probably)
    //   the 200-800 Hz range; whereas for treble, you'll want the 1,400 -
    //   11,025 Hz range.
    // If you want to get 3 bands, try it this way:
    //   a) 11,025 / 200 = 55.125
    //   b) to get the number of octaves between 200 and 11,025 Hz, solve for n:
    //          2^n = 55.125
    //          n = log 55.125 / log 2
    //          n = 5.785
    //   c) so each band should represent 5.785/3 = 1.928 octaves; the ranges are:
    //          1) 200 - 200*2^1.928                    or  200  - 761   Hz
    //          2) 200*2^1.928 - 200*2^(1.928*2)        or  761  - 2897  Hz
    //          3) 200*2^(1.928*2) - 200*2^(1.928*3)    or  2897 - 11025 Hz

    // A simple sine-wave-based envelope is convolved with the waveform
    //   data before doing the FFT, to emeliorate the bad frequency response
    //   of a square (i.e. nonexistent) filter.

    // You might want to slightly damp (blur) the input if your signal isn't
    //   of a very high quality, to reduce high-frequency noise that would
    //   otherwise show up in the output.


    constexpr int MAX = 4096;
    float real[MAX];
    float imag[MAX];

    assert(NFREQ <= MAX);



    // 1. set up input to the fft
    for (int i=0; i<NFREQ; i++)
    {
        real[i] = 0;
        imag[i] = 0.0f;

        int idx = bitrevtable[i];
        if (idx < in_count)
            real[i] = in_wavedata[idx];

        if (idx < m_samples_in)
            real[i] *= envelope[idx];
    }
    
    // 2. perform FFT
    int dftsize = 2;
    int t = 0;
    while (dftsize <= NFREQ) 
    {
        float theta = (float)(-2.0f * M_PI/(float)dftsize);
        
        float wpr = cosf(theta);
        float wpi = sinf(theta);
        float wr = 1.0f;
        float wi = 0.0f;
        int hdftsize = dftsize >> 1;

        for (int m = 0; m < hdftsize; m+=1)
        {
            for (int i = m; i < NFREQ; i+=dftsize)
            {
                int j = i + hdftsize;
                float tempr = wr*real[j] - wi*imag[j];
                float tempi = wr*imag[j] + wi*real[j];
                real[j] = real[i] - tempr;
                imag[j] = imag[i] - tempi;
                real[i] += tempr;
                imag[i] += tempi;
            }

            float wtemp=wr;
            wr = wtemp *wpr - wi*wpi;
            wi = wi*wpr + wtemp*wpi;
        }

        dftsize <<= 1;
        t++;
    }

    
    // 3. take the magnitude & equalize it (on a log10 scale) for output
    for (int i=0; i< m_samples_out; i++)
        out_spectraldata[i] = sqrtf(real[i]*real[i] + imag[i]*imag[i]);

    if (!equalize.empty())
        for (int i=0; i< m_samples_out; i++)
            out_spectraldata[i] *= equalize[i];
}
#endif

/*****************************************************************************/
