

#if 1

#ifdef WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <stdlib.h>
#include <netinet/tcp.h>    // for TCP_NODELAY
#include <unistd.h>

#define INVALID_SOCKET -1
typedef int SOCKET;
#define closesocket(x) close(x);
#endif



#include "IAudioSource.h"
#include "platform.h"

#include <thread>
#include <list>
#include <assert.h>




class UDPAudioSource : public IAudioSource
{
    std::string mDeviceName;
//    ALCdevice *mInputDevice = nullptr;
    
    int mSampleRate;
    std::vector<Sample> mSamples;
    
    SOCKET mSocket = INVALID_SOCKET;
    
public:
	UDPAudioSource()
        :mSampleRate(44100)
	{
        mDeviceName = "UDP Network";
	}
    
    virtual ~UDPAudioSource()
    {
        StopCapture();
    }
    
    virtual const std::string &GetDescription() const override
    {
        return mDeviceName;
    }
    
    virtual void Reset() override
    {
        
    }



    
    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override
    {
        Process();
        
        samples.SetSampleRate(mSampleRate);
        
        int total = (int)((float)mSampleRate * dt);
        int avail = std::min(total, (int)mSamples.size());
        
        // return samples from buffer
        samples.Assign(mSamples.begin(), mSamples.begin() + avail);
        mSamples.erase(mSamples.begin(), mSamples.begin() + avail);
        
        // pad end with empty samples
        while (samples.size() < total)
        {
            samples.push_back(Sample(0,0));
        }
    }
    
    bool StartCapture()
    {
        const char *hostName ="localhost";
        const char *portName ="19991";
        
        struct addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET;
        hints.ai_flags    = AI_PASSIVE;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;
        
        struct addrinfo *addr = NULL;
        int error = getaddrinfo(hostName, portName, &hints, &addr);
        if (error != 0 || addr == NULL)
        {
             return false;
        }
        
        
        // create socket
        SOCKET s = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (s == INVALID_SOCKET)
        {
            freeaddrinfo(addr);
            return false;
        }
        
        int yes=1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

        
        
        if (bind(s, addr->ai_addr, (int)addr->ai_addrlen) < 0)
        {
            freeaddrinfo(addr);
            closesocket(s);
            return false;
        }
        
        mSocket = s;
        return true;


        
        
//        int bufferSize = 1024;
//        ALenum format = AL_FORMAT_STEREO16;
//
//        mInputDevice = alcCaptureOpenDevice(nullptr, mSampleRate, format, bufferSize);
//        ALC_CHECK_ERROR();
//
//        alcCaptureStart(mInputDevice);
//        ALC_CHECK_ERROR();
//
//        std::string spec = alcGetString(mInputDevice, ALC_CAPTURE_DEVICE_SPECIFIER);
//        LogPrint("OpenAL: Starting audio capture thread device:'%s' rate:%d bufferSize:%d\n", spec.c_str(), mSampleRate, bufferSize );
//
    }
    
    
    virtual void StopCapture() override
    {

//        if (mInputDevice)
//        {
//            // Stop capture
//            LogPrint("OpenAL: Stopping audio capture thread.\n");
//            alcCaptureStop(mInputDevice);
//            alcCaptureCloseDevice(mInputDevice);
////            ALC_CHECK_ERROR()
//
//            mInputDevice = nullptr;
//        }
        
       
    }
    
    
protected:
    
    void OnCaptureAudio(const int16_t *samples, int count)
    {
        for (int i=0; i < count; i++)
        {
            mSamples.push_back(Sample::Convert(samples[i]));
        }
    }
    
    
    void OnCaptureAudio(const Sample16 *samples, int count)
    {
        float scale = 4.0f;
        
        for (int i=0; i < count; i++)
        {
            mSamples.push_back(Sample::Convert(samples[i], scale));
        }
    }
    
    void Process()
    {
        if (mSocket == INVALID_SOCKET)
            return;
        
        char buffer[2048];
        
        int result = (int)recv(mSocket, buffer, sizeof(buffer), MSG_DONTWAIT);
        
        if (result <= 0)
            return;
        
        

//
//        // poll for audio
//        ALCint capture_samples_count = 0;
//        alcGetIntegerv(mInputDevice, ALC_CAPTURE_SAMPLES, 1, &capture_samples_count);
//        ALenum errorCode = alcGetError(mInputDevice);
//        if (errorCode != 0)
//            return;
//
//        ALC_CHECK_ERROR()
//
//        constexpr int kBufferSize = 2048;
//        int count = std::min(capture_samples_count, kBufferSize);
//        if (count > 0)
//        {
//            Sample16 buffer[kBufferSize]; // A buffer to hold captured audio
//            alcCaptureSamples(mInputDevice, buffer, count);
//            ALC_CHECK_ERROR();
//
//            OnCaptureAudio(buffer, count);
//        }
    }

  

};


IAudioSourcePtr OpenUDPAudioSource()
{
    auto source = std::make_shared<UDPAudioSource>();
    source->StartCapture();
    return source;
}


#endif
