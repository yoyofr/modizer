#include <TargetConditionals.h>

#if __APPLE__ //  !TARGET_OS_IPHONE

#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Accelerate/Accelerate.h>
#import <AVFoundation/AVFoundation.h>

#include "platform.h"

#include "IAudioSource.h"
#include <thread>
#include <list>
#include <assert.h>



class AVAudioEngineSource : public IAudioSource
{
    std::string mDeviceName;
    std::string mDescription;
    AVAudioEngine *mAudioEngine = nil;
    AVAudioInputNode *mInputNode = nil;
    
    int mSampleRate;
    std::vector<Sample> mSamples;
    
    std::mutex mMutex;
    
    
public:
	AVAudioEngineSource(int sampleRate)
        :mSampleRate(sampleRate)
	{
        
        

        mDescription = "AVAudioEngine - No input devices";
       /*
        NSError *localError;
        AVAudioSession *sharedSession = [AVAudioSession sharedInstance];
        [sharedSession setCategory:AVAudioSessionCategoryRecord
                                      mode:AVAudioSessionModeDefault
                                   options:AVAudioSessionCategoryOptionDefaultToSpeaker
                                     error:&localError];
        
        NSArray *avail_inputs=[sharedSession availableInputs];
        NSLog(@"avail inputs: %d",[avail_inputs count]);
        AVAudioSessionPortDescription *route;
        if ([avail_inputs count]>0) {
            route=[avail_inputs objectAtIndex:0];
            [sharedSession setPreferredInput:route error:&localError];
        }
        [sharedSession setActive:TRUE error:&localError];
        //shareSession setPreferredInput:<#(nullable AVAudioSessionPortDescription *)#> error:<#(NSError *__autoreleasing  _Nullable * _Nullable)#>
        */
        
//        mDescription += [mInputNode.description UTF8String];
        
//        StartCapture();
        
        
	}
    
    virtual ~AVAudioEngineSource()
    {
        StopCapture();
    }
    
    
    virtual const std::string &GetDescription() const override
    {
        return mDescription;
    }

    virtual void Reset() override
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mSamples.clear();
    }


    
    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        
        if (!mAudioEngine)
            StartCapture();
        
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
        
        if (mSamples.size() > mSampleRate)
            mSamples.clear();
    }
    
   // ALCcontext *mContext;
    
    void CaptureSampleData(AVAudioPCMBuffer *buf, AVAudioTime *when)
    {
        std::lock_guard<std::mutex> lock(mMutex);

        float * __nonnull const *data = buf.floatChannelData;
        int frameLength =  buf.frameLength;
        
        float *buffer = data[0];
          
        float scale = 10.0f;
        for (int i=0; i < frameLength; i++)
        {
            mSamples.push_back(Sample::Convert(buffer[i], scale));
        }
       
    }
    
    void StartCapture()
    {
        NSError *localError;        
        
        mAudioEngine = [[AVAudioEngine alloc] init];
        
        mInputNode = [mAudioEngine inputNode];

        AVAudioFormat *format = [mInputNode outputFormatForBus: 0];
        
        NSLog(@"AVAudioFormat %@ %d\n", format, (int)mInputNode.numberOfInputs);
        
        mSampleRate = (int)format.sampleRate;
        
        if (mSampleRate<=0) {
            [mInputNode reset];
            format = [mInputNode outputFormatForBus: 0];
            NSLog(@"AVAudioFormat2 %@ %d\n", format, (int)mInputNode.numberOfInputs);
        }
        
        int bufferSize = mSampleRate / 60;
        
        if (mSampleRate<=0) {
            //LogError("AVAudioEngine: %s\n", error ? error.description.UTF8String : "");
            //mDescription = "AVAudioEngine - Error";
            return;
        }
        [mInputNode installTapOnBus:0
         bufferSize:bufferSize
             format:format
              block: ^(AVAudioPCMBuffer *buf, AVAudioTime *when) {
         
                // ‘buf' contains audio captured from input node at time 'when'
                CaptureSampleData(buf, when);
         
          
            }
         ];
        
        NSError *error = nil;
        
        [mAudioEngine prepare];
        if (![mAudioEngine startAndReturnError:&error]) {
            LogError("AVAudioEngine: %s\n", error ? error.description.UTF8String : "");
            mDescription = "AVAudioEngine - Error";
        }
        else
        {
        
        NSString *name = [mInputNode nameForInputBus:0];
        
         
        std::string spec =  name ? name.UTF8String : "bus0";
         LogPrint("AVAudioEngine: Starting audio capture device:'%s' rate:%d bufferSize:%d\n", spec.c_str(), mSampleRate, bufferSize );
         mDescription = "AVAudioEngine - ";
         mDescription += spec;
        }

    }
    
    
    virtual void StopCapture() override
    {
        std::lock_guard<std::mutex> lock(mMutex);

        if (mInputNode)
        {
            [mInputNode removeTapOnBus:0];
            mInputNode = nil;
        }
        
        mAudioEngine = nil;
    }
    
    
protected:
    
//    template<typename T>
//    int ReadCapturedSamples(int capture_samples_count, float scale)
//    {
//        constexpr int kBufferSize = 4096;
//        int count = std::min(capture_samples_count, kBufferSize);
//        if (count <= 0)
//            return 0;
//
//        T buffer[kBufferSize]; // A buffer to hold captured audio
//
//        alcCaptureSamples(mInputDevice, buffer, count);
//        ALenum errorCode = alcGetError(mInputDevice);
//        if (errorCode != 0)
//        {
//            return 0;
//        }
//
//        for (int i=0; i < count; i++)
//        {
//            mSamples.push_back(Sample::Convert(buffer[i], scale));
//        }
//        return count;
//
//    }
    
    void Process()
    {
//        if (!mInputDevice)
//        {
//            StartCapture();
//            if (!mInputDevice)
//            {
//                return;
//            }
//        }
//
//        // poll for audio
//        ALCint capture_samples_count = 0;
//        alcGetIntegerv(mInputDevice, ALC_CAPTURE_SAMPLES, 1, &capture_samples_count);
//        if (capture_samples_count == 0)
//        {
//            return;
//        }
//
//        ALenum errorCode = alcGetError(mInputDevice);
//        if (errorCode != 0)
//        {
//            printf("errorCode %d\n", errorCode);
//            return;
//        }
//
//
//        float scale = 2.0f;
//
//        switch (mFormat)
//        {
//            case AL_FORMAT_MONO_FLOAT32:
//                ReadCapturedSamples<float>(capture_samples_count, scale);
//                break;
//            case AL_FORMAT_MONO16:
//                ReadCapturedSamples<int16_t>(capture_samples_count, scale);
//                break;
//            case AL_FORMAT_STEREO16:
//                ReadCapturedSamples<Sample16>(capture_samples_count, scale);
//                break;
//        }
//
    }

  

};


IAudioSourcePtr OpenAVAudioEngineSource()
{
    int sampleRate = 44100;
    auto source = std::make_shared<AVAudioEngineSource>(sampleRate);
    return source;
}


#endif
