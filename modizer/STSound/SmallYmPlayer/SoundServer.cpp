/*********************************************************************************

	SoundServer - Written by Arnaud Carré (aka Leonard / OXYGENE)
	Part of the "Leonard Homepage Articles".
	http://leonard.oxg.free.fr
	Read the complete article on my web page:

	PART 1: Using WaveOut API.
	How to make a SoundServer under windows, to play various sound.
	WARNING: This sample plays a 44.1Khz, 16bits, mono sound. Should be quite easy to modify ! :-)

	arnaud.carre@freesurf.fr

	WARNING: You have to link this with the Windows MultiMedia library. (WINMM.LIB)

*********************************************************************************/

#include <windows.h>
#include "SoundServer.h"


#pragma	comment(lib,"winmm.lib")

// Internal WaveOut API callback function. We just call our sound handler ("playNextBuffer")
static	void CALLBACK waveOutProc(HWAVEOUT hwo,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
{

	if (WOM_DONE == uMsg)
	{
		CSoundServer *pServer = (CSoundServer*)dwInstance;
		if (pServer)
		{
			if (pServer->IsRunning())
				pServer->fillNextBuffer();
		}
	}
}

CSoundServer::CSoundServer()
{
		m_pUserCallback = NULL;
		m_currentBuffer = 0;
}

CSoundServer::~CSoundServer()
{
		close();
}

BOOL	CSoundServer::open(USER_CALLBACK pUserCallback,long totalBufferedSoundLen)
{

		m_pUserCallback = pUserCallback;
		m_bufferSize = ((totalBufferedSoundLen * REPLAY_RATE) / 1000) * REPLAY_SAMPLELEN;
		m_bufferSize /= REPLAY_NBSOUNDBUFFER;

		WAVEFORMATEX	wfx;
		wfx.wFormatTag = 1;		// PCM standart.
		wfx.nChannels = 1;		// Mono
		wfx.nSamplesPerSec = REPLAY_RATE;
		wfx.nAvgBytesPerSec = REPLAY_RATE*REPLAY_SAMPLELEN;
		wfx.nBlockAlign = REPLAY_SAMPLELEN;
		wfx.wBitsPerSample = REPLAY_DEPTH;
		wfx.cbSize = 0;
		MMRESULT errCode = waveOutOpen(	&m_hWaveOut,
										WAVE_MAPPER,
										&wfx,
										(DWORD)waveOutProc,
										(DWORD)this,					// User data.
										(DWORD)CALLBACK_FUNCTION);

		if (errCode != MMSYSERR_NOERROR) return FALSE;

		// Alloc the sample buffers.
		int i;
		for (i=0;i<REPLAY_NBSOUNDBUFFER;i++)
		{
			m_pSoundBuffer[i] = malloc(m_bufferSize);
			memset(&m_waveHeader[i],0,sizeof(WAVEHDR));
			memset(m_pSoundBuffer[i],0,m_bufferSize);
		}

		// Fill all the sound buffers
		m_currentBuffer = 0;
		for (i=0;i<REPLAY_NBSOUNDBUFFER;i++)
			fillNextBuffer();

		return TRUE;
}

void	CSoundServer::close(void)
{

		if (IsRunning())
		{
			m_pUserCallback = NULL;
			waveOutReset(m_hWaveOut);					// Reset tout.
			for (int i=0;i<REPLAY_NBSOUNDBUFFER;i++)
			{
				if (m_waveHeader[i].dwFlags & WHDR_PREPARED)
					waveOutUnprepareHeader(m_hWaveOut,&m_waveHeader[i],sizeof(WAVEHDR));

				free(m_pSoundBuffer[i]);
			}
			waveOutClose(m_hWaveOut);
		}
}

void	CSoundServer::fillNextBuffer(void)
{

		// check if the buffer is already prepared (should not !)
		if (m_waveHeader[m_currentBuffer].dwFlags&WHDR_PREPARED)
			waveOutUnprepareHeader(m_hWaveOut,&m_waveHeader[m_currentBuffer],sizeof(WAVEHDR));

		// Call the user function to fill the buffer with anything you want ! :-)
		if (m_pUserCallback)
			m_pUserCallback(m_pSoundBuffer[m_currentBuffer],m_bufferSize);

		// Prepare the buffer to be sent to the WaveOut API
		m_waveHeader[m_currentBuffer].lpData = (char*)m_pSoundBuffer[m_currentBuffer];
		m_waveHeader[m_currentBuffer].dwBufferLength = m_bufferSize;
		waveOutPrepareHeader(m_hWaveOut,&m_waveHeader[m_currentBuffer],sizeof(WAVEHDR));

		// Send the buffer the the WaveOut queue
		waveOutWrite(m_hWaveOut,&m_waveHeader[m_currentBuffer],sizeof(WAVEHDR));

		m_currentBuffer++;
		if (m_currentBuffer >= REPLAY_NBSOUNDBUFFER) m_currentBuffer = 0;
}
