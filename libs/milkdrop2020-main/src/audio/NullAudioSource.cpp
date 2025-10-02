

#include "IAudioSource.h"
#include "platform.h"

class NullAudioSource : public IAudioSource
{
    std::string mName;
public:
	NullAudioSource()
	{
        mName = "Null";
        mSampleRate = 44100;
	}
    
    virtual void Reset() override
    {
        
    }
    
    virtual ~NullAudioSource()
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
        for (int i=0; i < count; i++)
        {
            samples.push_back(Sample{0,0});
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


IAudioSourcePtr OpenNullAudioSource()
{
    return std::make_shared<NullAudioSource>();
}
