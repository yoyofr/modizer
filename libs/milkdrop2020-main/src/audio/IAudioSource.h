
#pragma once

#include <stdint.h>
#include <math.h>
#include <vector>
#include <string>
#include "MDcommon.h"
#include "SampleBuffer.h"



class IAudioSource
{
public:
    virtual ~IAudioSource() {}
    
    virtual const std::string &GetDescription() const = 0;
    
//    virtual double GetSampleRate() const = 0;
    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &outSamples) = 0;
    virtual void StopCapture() {}
    
    virtual void Reset() = 0;

};

using IAudioSourcePtr = std::shared_ptr<IAudioSource>;

