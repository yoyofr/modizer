

#include "IAudioSource.h"
#include "platform.h"

#include "../../../../src/ModizerVoicesData.h"

class MDZAudioSource : public IAudioSource
{
    std::string mName;
public:
    MDZAudioSource()
    {
        mName = "Modizer";
        mSampleRate = 44100;
    }
    
    virtual void Reset() override
    {
        
    }
    
    virtual ~MDZAudioSource()
    {
    }
    
    virtual const std::string &GetDescription() const override
     {
         return mName;
     }


    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override
    {
        samples.SetSampleRate((float)mSampleRate);

        int count = (int)((float)mSampleRate * dt);
        samples.clear();
        samples.reserve(count);
        //printf("%d\n",count);
        int l,r;
        Sample s;
        float scale=1.2f;
        for (int i=0; i < count; i++)
        {
            l=milkBuffer[milkBufferPosRead++];
            r=milkBuffer[milkBufferPosRead++];
            if (milkBufferPosRead>=MILK_BUFFER_SIZE) milkBufferPosRead=0;
            s.ch[0]=(float)l*scale/32768.0f;
            s.ch[1]=(float)r*scale/32768.0f;
            if (s.ch[0]>1.0f) s.ch[0]=1.0f;
            if (s.ch[1]>1.0f) s.ch[1]=1.0f;
            if (s.ch[0]<-1.0f) s.ch[0]=-1.0f;
            if (s.ch[1]<-1.0f) s.ch[1]=-1.0f;
            samples.push_back(s);
            //samples.push_back(Sample{0,0});
        }
        
#if 0

        samples.clear();
        float tinc = 1.0f / mSampleRate;
        for (int i=0; i < count; i++)
        {
            float w = 0.0f;
            
            w += sinf(m_time * 2 * M_PI * 80.0f);
            
  //          w += sinf(m_time * 2 * M_PI * 600.0f);
//            w += sinf(m_time * 2 * M_PI * 200.0f);

            
            w += sinf(m_time * 2 * M_PI * 6000.0f);

            samples.push_back(Sample{w,w});
            m_time += tinc;
        }
#endif

    }
    
    float m_time =0.0;

protected:
    int mSampleRate;
};


IAudioSourcePtr OpenMDZAudioSource()
{
    return std::make_shared<MDZAudioSource>();
}
