

#if 1
#include "IAudioSource.h"
#include "platform.h"

#if defined(EMSCRIPTEN)
#include <AL/al.h>
#include <AL/alc.h>
#else
#include <OpenAL/OpenAL.h>
#endif
#include <thread>
#include <list>
#include <assert.h>


#define AL_FORMAT_MONO_FLOAT32 0x10010
#define AL_FORMAT_STEREO_FLOAT32 0x10011


#define ALC_CHECK_ERROR() \
    {ALenum errorCode = alcGetError(mInputDevice); \
    assert(errorCode == 0); }

#define AL_CHECK_ERROR() \
    {  ALenum errorCode = alGetError(); \
    assert(errorCode == 0); }


class ALAudioSource : public IAudioSource
{
    std::string mDeviceName;
    std::string mDescription;
    ALCdevice *mInputDevice = nullptr;
    
    int mSampleRate;
    std::vector<Sample> mSamples;
    ALenum mFormat;
    
    
public:
	ALAudioSource(int sampleRate)
        :mSampleRate(sampleRate)
	{
        mFormat = AL_FORMAT_MONO16;
        //        ALenum format = AL_FORMAT_STEREO16;
        
        mDescription = "OpenAL - No input devices";
	}
    
    virtual ~ALAudioSource()
    {
        StopCapture();
    }
    
    
    virtual const std::string &GetDescription() const override
    {
        return mDescription;
    }

    virtual void Reset() override
    {
        mSamples.clear();
    }


    
    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override
    {
        Process();
        
        samples.SetSampleRate(mSampleRate);
        
        int total = (int)((float)mSampleRate * dt);
        int avail = std::min(total, (int)mSamples.size());

  //      printf("ReadAudioFrame %d %d\n", (int)mSamples.size(),  total );

        // return samples from buffer
        samples.Assign(mSamples.begin(), mSamples.begin() + avail);
        mSamples.erase(mSamples.begin(), mSamples.begin() + avail);
        
        

        // pad end with empty samples
        while (samples.size() < total)
        {
            samples.push_back(Sample(0,0));
        }
        
        if (mSamples.size() > 2000)
            mSamples.clear();
    }
    
   // ALCcontext *mContext;
    
    void StartCapture()
    {
        if (mInputDevice)
        {
            StopCapture();
        }

        int bufferSize = mSampleRate * 1;

        std::string  defaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
        
        mInputDevice = alcCaptureOpenDevice(defaultCaptureDevice.c_str(), mSampleRate, mFormat, bufferSize);
        ALC_CHECK_ERROR();
        if (!mInputDevice) {
            return;
        }
        
        alcCaptureStart(mInputDevice);
        ALC_CHECK_ERROR();
        
        std::string spec = alcGetString(mInputDevice, ALC_CAPTURE_DEVICE_SPECIFIER);
        LogPrint("OpenAL: Starting audio capture device:'%s' rate:%d bufferSize:%d\n", spec.c_str(), mSampleRate, bufferSize );
        mDescription = "OpenAL - ";
        mDescription += spec;
        return;
    }
    
    
    virtual void StopCapture() override
    {
        if (mInputDevice)
        {
            // Stop capture
            LogPrint("OpenAL: Stopping audio capture thread.\n");
            alcCaptureStop(mInputDevice);
            alcCaptureCloseDevice(mInputDevice);
//            ALC_CHECK_ERROR()
            
            mInputDevice = nullptr;
        }
       
    }
    
    
protected:
    
    template<typename T>
    int ReadCapturedSamples(int capture_samples_count, float scale)
    {
        constexpr int kBufferSize = 4096;
        int count = std::min(capture_samples_count, kBufferSize);
        if (count <= 0)
            return 0;
        
        T buffer[kBufferSize]; // A buffer to hold captured audio
        
        alcCaptureSamples(mInputDevice, buffer, count);
        ALenum errorCode = alcGetError(mInputDevice);
        if (errorCode != 0)
        {
            return 0;
        }
        
        for (int i=0; i < count; i++)
        {
            mSamples.push_back(Sample::Convert(buffer[i], scale));
        }
        return count;

    }
    
    void Process()
    {
        if (!mInputDevice)
        {
            StartCapture();
            if (!mInputDevice)
            {
                return;
            }
        }
        
        // poll for audio
        ALCint capture_samples_count = 0;
        alcGetIntegerv(mInputDevice, ALC_CAPTURE_SAMPLES, 1, &capture_samples_count);
        if (capture_samples_count == 0)
        {
            return;
        }
        
        ALenum errorCode = alcGetError(mInputDevice);
        if (errorCode != 0)
        {
            printf("errorCode %d\n", errorCode);
            return;
        }
        
        
        float scale = 2.0f;
        
        switch (mFormat)
        {
            case AL_FORMAT_MONO_FLOAT32:
                ReadCapturedSamples<float>(capture_samples_count, scale);
                break;
            case AL_FORMAT_MONO16:
                ReadCapturedSamples<int16_t>(capture_samples_count, scale);
                break;
            case AL_FORMAT_STEREO16:
                ReadCapturedSamples<Sample16>(capture_samples_count, scale);
                break;
        }
    
    }

  

};


#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// Avoid calling this more than once! Caching the value is up to you.
unsigned query_sample_rate_of_audiocontexts() {
    return EM_ASM_INT({
        var AudioContext = window.AudioContext || window.webkitAudioContext;
        var ctx = new AudioContext();
        var sr = ctx.sampleRate;
        ctx.close();
        return sr;
    });
}
#endif

IAudioSourcePtr OpenALAudioSource()
{
    int sampleRate = 44100;
    #ifdef __EMSCRIPTEN__
    sampleRate = query_sample_rate_of_audiocontexts();
    #endif
    auto source = std::make_shared<ALAudioSource>(sampleRate);
    return source;
}


#endif
