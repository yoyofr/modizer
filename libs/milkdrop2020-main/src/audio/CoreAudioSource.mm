
#include <TargetConditionals.h>

#if !TARGET_OS_IPHONE

#include "IAudioSource.h"

#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Accelerate/Accelerate.h>
#if TARGET_OS_IPHONE
#import <AVFoundation/AVFoundation.h>
#else
#import <CoreAudio/CoreAudio.h>
#endif

#include <vector>


#define CheckStatus() \
assert( status == 0 )


class CoreAudioSource : public IAudioSource
{
    std::string mDescription;
public:
    CoreAudioSource(AudioDeviceID device)
        :m_device(device)
    {
        mDescription = "CoreAudio";
    }
    
    virtual const std::string &GetDescription() const override
     {
         return mDescription;
     }
    
    virtual void Reset() override
    {
        
    }

    
    
    virtual bool Start()
    {
        OSStatus status;
        
        AudioComponentDescription desc = {0};
        desc.componentType = kAudioUnitType_Output;
#if TARGET_OS_IPHONE
        desc.componentSubType = kAudioUnitSubType_RemoteIO;
#else
        desc.componentSubType = kAudioUnitSubType_HALOutput;
#endif
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        
        
        // Get component
        AudioComponent comp = AudioComponentFindNext(NULL, &desc);
        status = AudioComponentInstanceNew(comp, &m_instance);
        CheckStatus();
        status = AudioUnitInitialize(m_instance);
        CheckStatus();
        
        // Enable input / disable output
        SetProperty<UInt32>(kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Input,  1, 1);
        SetProperty<UInt32>(kAudioOutputUnitProperty_EnableIO,kAudioUnitScope_Output, 0, 0);
        
        SetGlobalProperty(kAudioOutputUnitProperty_CurrentDevice, m_device);
        

        m_format = GetProperty<AudioStreamBasicDescription>(kAudioUnitProperty_StreamFormat,
                                                                   kAudioUnitScope_Input,
                                                                   1);
        m_format_out = GetProperty<AudioStreamBasicDescription>(kAudioUnitProperty_StreamFormat,
                                                                       kAudioUnitScope_Output,
                                                                       0);
        //Get the size of the IO buffer(s)
        UInt32 bufferSizeFrames = GetGlobalProperty<UInt32>(kAudioDevicePropertyBufferFrameSize);
        UInt32 bufferSizeBytes = bufferSizeFrames * sizeof(Float32);
        
        //calculate number of buffers from channels
        int propsize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) * m_format.mChannelsPerFrame);
        
        // malloc buffer lists
        mInputBuffer = (AudioBufferList *)malloc(propsize);
        mInputBuffer->mNumberBuffers = m_format.mChannelsPerFrame;
        
        // pre-malloc buffers for AudioBufferLists
        for(UInt32 i =0; i< mInputBuffer->mNumberBuffers ; i++) {
            mInputBuffer->mBuffers[i].mNumberChannels = 1;
            mInputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes;
            mInputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes);
            memset(mInputBuffer->mBuffers[i].mData, 0, bufferSizeBytes);
        }

        AURenderCallbackStruct inputCB;
        inputCB.inputProc = InputProc;
        inputCB.inputProcRefCon = this;
        SetGlobalProperty(kAudioOutputUnitProperty_SetInputCallback, inputCB);

        AudioOutputUnitStart(m_instance);
        return true;
    }
    
    
    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override
    {
//        const float *left = (const float *)mInputBuffer->mBuffers[0].mData;
//        const float *right = (const float *)mInputBuffer->mBuffers[1].mData;
//        
        int count = 800;
        samples.SetSampleRate(count * 60);
        for (int i=0; i < count; i++)
        {
            Sample s {0.0f,0.0f};
            samples.push_back(s);
        }
    }
    
    //
    //
    
    
    
    template <typename T>
    T GetProperty(AudioUnitPropertyID property,
                  AudioUnitScope	scope,
                  AudioUnitElement element
                  )
    {
        T value;
        UInt32 size = sizeof(value);
        OSErr status = AudioUnitGetProperty(m_instance,
                                            property,
                                            scope,
                                            element,
                                            &value,
                                            &size
                                            );
        CheckStatus();
        return value;
    }
    
    template <typename T>
    void SetProperty(AudioUnitPropertyID property,
                     AudioUnitScope	scope,
                     AudioUnitElement element,
                     const T	value
                     )
    {
        OSErr status = AudioUnitSetProperty(m_instance,
                                            property,
                                            scope,
                                            element,
                                            &value,
                                            sizeof(value)
                                            );
        CheckStatus();
    }
    
    
    template <typename T>
    T GetGlobalProperty(AudioUnitPropertyID property
                        )
    {
        return GetProperty<T>(property, kAudioUnitScope_Global, 0);
    }
    
    template <typename T>
    void SetGlobalProperty(AudioUnitPropertyID property, const T value)
    {
        SetProperty<T>(property,
                    kAudioUnitScope_Global,
                    0,
                    value
                    );
    }
    

    
private:
    AudioDeviceID m_device;
    AudioComponentInstance m_instance;
    AudioStreamBasicDescription m_format;
    AudioStreamBasicDescription m_format_out;
    AudioBufferList *mInputBuffer;

    std::mutex m_mutex;
    std::vector<Sample> m_samples;
    
    
    
    static OSStatus InputProc(void *inRefCon,
                              AudioUnitRenderActionFlags *ioActionFlags,
                              const AudioTimeStamp *inTimeStamp,
                              UInt32 inBusNumber,
                              UInt32 inNumberFrames,
                              AudioBufferList * ioData)
    {
        CoreAudioSource *source = (CoreAudioSource *)inRefCon;
        
        // Get the new audio data
        OSStatus status = AudioUnitRender(source->m_instance,
             ioActionFlags,
             inTimeStamp,
             inBusNumber,
             inNumberFrames, //# of frames requested
             source->mInputBuffer);// Audio Buffer List to hold data
        if (status) {
             return status;
        }
        
         return status;
    }
};


//
//
//

static AudioDeviceID GetDefaultInputDevice()
{
    UInt32 size = sizeof(AudioDeviceID);
    AudioObjectPropertyAddress addr =
    { kAudioHardwarePropertyDefaultInputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster };
    
    AudioDeviceID inputDevice;
    AudioObjectGetPropertyData(kAudioObjectSystemObject,
                               &addr,
                               0,
                               NULL,
                               &size,
                               &inputDevice);
    return inputDevice;
}

//
//
//


IAudioSourcePtr CreateCoreAudioSource(int sampleRate, int frameSize)
{
    AudioDeviceID inputDevice = GetDefaultInputDevice();

    auto source = std::make_shared<CoreAudioSource>(inputDevice);
    if (!source->Start())
    {
        return nullptr;
    }
    return source;

}

#endif
