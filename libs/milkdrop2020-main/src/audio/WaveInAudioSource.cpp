
#if defined(WIN32) && 0

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <mmsystem.h>
#include <vector>
#include <mutex>

#include "IAudioSource.h"
#include "platform.h"

#pragma comment(lib,"winmm.lib")

class WaveInAudioSource : public IAudioSource
{
public:
	WaveInAudioSource()
		:m_handle()
	{
	}
    
    virtual void Reset() override
    {
        
    }



	virtual bool Start(int sampleRate, int frameSize)
	{
		WAVEFORMATEX fmt;
		fmt.wFormatTag = WAVE_FORMAT_PCM;
		fmt.nChannels = 2;
		fmt.nSamplesPerSec = sampleRate;
		fmt.wBitsPerSample = 16;
		fmt.nBlockAlign = fmt.nChannels * (fmt.wBitsPerSample / 8);
		fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
		fmt.cbSize = 0;

		MMRESULT result = waveInOpen(&m_handle, WAVE_MAPPER, &fmt, (DWORD_PTR)waveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
		if (result != MMSYSERR_NOERROR) return false;

		for (int i = 0; i < 2; i++)
		{
			WAVEHDR *buffer = &m_header[i];
			memset(buffer, 0, sizeof(*buffer));
			buffer->lpData = (LPSTR)new Sample16[frameSize];
			buffer->dwBufferLength = frameSize * sizeof(Sample16);
			
			result = waveInPrepareHeader(m_handle, buffer, sizeof(*buffer));
			if (result != MMSYSERR_NOERROR) return false;
			
			// Insert a wave input buffer
			result = waveInAddBuffer(m_handle, buffer, sizeof(*buffer));
			if (result != MMSYSERR_NOERROR) return false;
		}



		result = waveInStart(m_handle);
		if (result != MMSYSERR_NOERROR) return false;

		return true;
	}

	virtual void Stop()
	{
		if (m_handle)
		{
			waveInStop(m_handle);
			for (int i = 0; i < 2; i++)
			{
				WAVEHDR *buffer = &m_header[i];
				waveInUnprepareHeader(m_handle, buffer, sizeof(*buffer));
				delete buffer->lpData;
			}
			waveInClose(m_handle);
			m_handle = NULL;
		}
	}

	virtual bool ReadAudio(Sample *buffer, int size)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (size > (int)m_samples.size())
		{
			//LogPrint("!read %d of %d\n", 0, m_samples.size());
			return false;
		}

		float scale = 1.0f / 32767.0f;

		// copy samples to output buffer
		for (int i = 0; i < size; i++)
		{
			buffer[i].left  = (float)m_samples[i].left  * scale;
			buffer[i].right = (float)m_samples[i].right * scale;
		}

		m_samples.erase(m_samples.begin(), m_samples.begin() + size);

		//LogPrint("read %d of %d\n", size, m_samples.size());

		if (m_samples.size() > (size_t)(size * 4) ) {
			// LogPrint("clear!\n");
			m_samples.clear();
		}
		return true;
	}


	static void CALLBACK waveInProc(
		HWAVEIN   hwi,
		UINT      uMsg,
		DWORD_PTR dwInstance,
		DWORD_PTR dwParam1,
		DWORD_PTR dwParam2
	)
	{
		auto *self = (WaveInAudioSource *)dwInstance;
		if (uMsg == WIM_DATA)
		{
			self->OnData((WAVEHDR *)dwParam1, (void *)dwParam2);
		}
	}

	virtual void OnData(WAVEHDR *hdr, void *param2)
	{
		if (hdr->dwFlags & WHDR_DONE)
		{
			int count = hdr->dwBytesRecorded / sizeof(Sample16);
			const Sample16 *source = (Sample16 *)hdr->lpData;


			{
				std::lock_guard<std::mutex> lock(m_mutex);
				m_samples.insert(m_samples.end(), source, source + count);
			}

			// Insert a wave input buffer
			MMRESULT result = waveInAddBuffer(m_handle, hdr, sizeof(*hdr));
			if (result != MMSYSERR_NOERROR)
				return;
		}

	}


protected:
	HWAVEIN	m_handle;
	WAVEHDR m_header[2];
	std::mutex			  m_mutex;
	std::vector<Sample16> m_samples;
};




IAudioSourcePtr CreateWaveInAudioSource(int sampleRate, int frameSize)
{
	auto source =  std::make_shared<WaveInAudioSource>();
	if (!source->Start(sampleRate, frameSize)) {
		return nullptr;
	}
	return source;
}

#endif
