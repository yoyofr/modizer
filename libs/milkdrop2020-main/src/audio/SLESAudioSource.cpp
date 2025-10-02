

#if defined(__ANDROID__) || defined(OCULUS) 
#include "IAudioSource.h"
#include "platform.h"

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

//#include <SLES/OpenSLES_Android.h>
#include <thread>
#include <list>
#include <assert.h>




class SLESAudioSource : public IAudioSource
{
    std::string mDeviceName;
    std::string mDescription;

    float mSampleRate;
    std::vector<Sample> m_samples;

    std::mutex           m_mutex;
    std::vector<Sample16> m_samples16;


    SLObjectItf m_engineObject = NULL;
    SLEngineItf m_engineEngine;


    SLObjectItf m_recorderObject = nullptr;
    SLRecordItf m_recorderRecord = nullptr;
    SLAndroidSimpleBufferQueueItf m_recorderBufferQueue = nullptr;

    // record buffers
    int m_bufferIndex = 0;
    int m_bufferSize = 800;
    std::vector<Sample16> m_buffers[2];

    bool m_permissionsFailure = false;

public:
	SLESAudioSource(float sampleRate, SLObjectItf engineObject, SLEngineItf engineEngine)
        :mSampleRate(sampleRate),
        m_engineObject(engineObject),
        m_engineEngine(engineEngine)
	{
	    m_bufferSize = (int)(sampleRate / 60.0f);
        mDescription = "SLES - No input devices";
	}
    
    virtual ~SLESAudioSource()
    {
        StopCapture();


        // destroy engine object, and invalidate all associated interfaces
        if (m_engineObject != NULL) {
            (*m_engineObject)->Destroy(m_engineObject);
            m_engineObject = NULL;
            m_engineEngine = NULL;
        }
    }
    
    
    virtual const std::string &GetDescription() const override
    {
        return mDescription;
    }

    virtual void Reset() override
    {
        m_samples.clear();
    }


    
    virtual void ReadAudioFrame(float dt, SampleBuffer<Sample> &samples) override
    {
        Process();

        // set buffer size
        m_bufferSize = (int)std::floorf(mSampleRate * dt);


        
        samples.SetSampleRate(mSampleRate);
        
        int total = (int)((float)mSampleRate * dt);
        int avail = std::min(total, (int)m_samples.size());

  //      printf("ReadAudioFrame %d %d\n", (int)m_samples.size(),  total );

        // return samples from buffer
        samples.Assign(m_samples.begin(), m_samples.begin() + avail);
        m_samples.erase(m_samples.begin(), m_samples.begin() + avail);
        
        

        // pad end with empty samples
        while (samples.size() < total)
        {
            samples.push_back(Sample(0,0));
        }
        
        if (m_samples.size() > 2000)
            m_samples.clear();
    }


    void RecorderCallback(SLAndroidSimpleBufferQueueItf bq)
    {

	    std::lock_guard<std::mutex> lock(m_mutex);


        SLAndroidSimpleBufferQueueState state;
        (*bq)->GetState(bq, &state);

        std::vector<Sample16> &buffer = m_buffers[m_bufferIndex];

        m_samples16.insert(m_samples16.end(), buffer.begin(), buffer.end());

        buffer.resize(m_bufferSize);
        (*m_recorderBufferQueue)->Enqueue(m_recorderBufferQueue,
                                          buffer.data(),
                                          buffer.size() * sizeof(buffer[0])
        );

        m_bufferIndex ^= 1;

//        LogPrint("SLES: RecorderCallback index:%d count:%d\n", state.index, state.count);

    }

// this callback handler is called every time a buffer finishes recording
    static void RecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
    {
        SLESAudioSource *self = (SLESAudioSource *)context;
        self->RecorderCallback(bq);
    }

#define CHECK_RESULT() if (SL_RESULT_SUCCESS != result) { LogError("SLES: Error %d\n", result); assert(0); return false;}

    
    bool StartCapture()
    {
        SLresult result;
        
        
        // configure audio source
        SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
            SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
        SLDataSource audioSrc = {&loc_dev, NULL};
        
        // configure audio sink
        SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
        SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 2,
//            SL_SAMPLINGRATE_48,
           SL_SAMPLINGRATE_44_1,
            SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN};
        SLDataSink audioSnk = {&loc_bq, &format_pcm};
        
        // create audio recorder
        // (requires the RECORD_AUDIO permission)
        const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
        const SLboolean req[1] = {SL_BOOLEAN_TRUE};
        result = (*m_engineEngine)->CreateAudioRecorder(m_engineEngine, &m_recorderObject, &audioSrc, &audioSnk, 1, id, req);
        CHECK_RESULT();
        
        // realize the audio recorder
        result = (*m_recorderObject)->Realize(m_recorderObject, SL_BOOLEAN_FALSE);
        if (result != SL_RESULT_SUCCESS)
        {
            m_recorderObject = nullptr;
            m_permissionsFailure = true;
            return false;
        }

        CHECK_RESULT();
        
        // get the record interface
        result = (*m_recorderObject)->GetInterface(m_recorderObject, SL_IID_RECORD, &m_recorderRecord);
        CHECK_RESULT();
        
        // get the buffer queue interface
        result = (*m_recorderObject)->GetInterface(m_recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                                   &m_recorderBufferQueue);
        CHECK_RESULT();


#if 0
        SLAudioIODeviceCapabilitiesItf recorder_caps;
        // get the buffer queue interface
        result = (*m_recorderObject)->GetInterface(m_recorderObject, SL_IID_AUDIOIODEVICECAPABILITIES,
                                                   &recorder_caps);
        CHECK_RESULT();

        SLAudioInputDescriptor desc;
        result = (*recorder_caps)->QueryAudioInputCapabilities(recorder_caps, SL_DEFAULTDEVICEID_AUDIOINPUT, &desc);
        if (result == SL_RESULT_SUCCESS)
        {
            mDescription = "SLES - ";
            mDescription+= (const char *)desc.deviceName;
            LogPrint("SLES: Selected input device %s\n", desc.deviceName);
        }
#endif

        // register callback on the buffer queue
        result = (*m_recorderBufferQueue)->RegisterCallback(m_recorderBufferQueue, RecorderCallback, (void *)this);
        CHECK_RESULT();
        
        
        // in case already recording, stop recording and clear buffer queue
        result = (*m_recorderRecord)->SetRecordState(m_recorderRecord, SL_RECORDSTATE_STOPPED);
        CHECK_RESULT();
        
        result = (*m_recorderBufferQueue)->Clear(m_recorderBufferQueue);
        CHECK_RESULT();
        

        m_bufferIndex = 0;
        for (int i=0; i < 2; i++)
        {
            std::vector<Sample16> &buffer = m_buffers[i];

            buffer.resize(m_bufferSize);
            (*m_recorderBufferQueue)->Enqueue(m_recorderBufferQueue,
                                              buffer.data(),
                                              buffer.size() * sizeof(buffer[0])
            );
        }
        

        // start recording
        result = (*m_recorderRecord)->SetRecordState(m_recorderRecord, SL_RECORDSTATE_RECORDING);
        CHECK_RESULT();




        mDescription = "SLES - ";
        mDescription+= "default audio input device";

        LogPrint("SLES: Starting audio recording\n");



        return true ;
    }
    
    
    virtual void StopCapture() override
    {

	    if (m_recorderRecord) {
            SLresult result;
            result = (*m_recorderRecord)->SetRecordState(m_recorderRecord, SL_RECORDSTATE_STOPPED);
        }

        // destroy audio recorder object, and invalidate all associated interfaces
        if (m_recorderObject != NULL) {
            LogPrint("SLES: Stopping audio capture thread.\n");

            (*m_recorderObject)->Destroy(m_recorderObject);
            m_recorderObject = NULL;
            m_recorderRecord = NULL;
            m_recorderBufferQueue = NULL;
        }
       
    }
    
    
protected:

    void Process()
    {

        if (!m_recorderRecord)
        {
            if (m_permissionsFailure)
            {
                return;
            }

            if (!StartCapture())
            {
                return;
            }
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        // copy from buffer to m_samples

        float scale = 8.0f;
        for (auto sample : m_samples16)
        {
            m_samples.push_back(Sample::Convert(sample, scale));
        }
        m_samples16.clear();

    }

  

};

IAudioSourcePtr OpenSLESAudioSource()
{
    SLObjectItf engineObject = NULL;
    SLEngineItf engineEngine;
    SLresult result;
    
    // create engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS) return nullptr;

    // realize the engine
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) return nullptr;

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    if (result != SL_RESULT_SUCCESS) return nullptr;
    

    
    
    
    
    int sampleRate = 44100;
    //int sampleRate = 48000;
    auto source = std::make_shared<SLESAudioSource>(sampleRate, engineObject, engineEngine);
    return source;
}


#endif
