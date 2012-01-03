/*********************************************************************************

	SoundServer - Written by Arnaud Carr� (aka Leonard / OXYGENE)
	Part of the "Leonard Homepage Articles".

	PART 1: Using WaveOut API.
	How to make a SoundServer under windows, to play various sound.
	WARNING: This sample plays a 44.1Khz, 16bits, mono sound. Should be quite easy to modify ! :-)

	Read the complete article on my web page:

	http://leonard.oxg.free.fr
	arnaud.carre@freesurf.fr

	WARNING: You have to link this with the Windows MultiMedia library. (WINMM.LIB)

*********************************************************************************/


#ifndef	__SOUNDSERVER__
#define	__SOUNDSERVER__


#define	REPLAY_RATE				44100
#define	REPLAY_DEPTH			16
#define	REPLAY_SAMPLELEN		(REPLAY_DEPTH/8)
#define	REPLAY_NBSOUNDBUFFER	2

typedef void (*USER_CALLBACK) (void *pBuffer,long bufferLen);

class	CSoundServer
{

public:

		CSoundServer();
		~CSoundServer();

		BOOL	open(	USER_CALLBACK	pUserCallback,
						long totalBufferedSoundLen=4000);		// Buffered sound, in ms

		void	close(void);

		BOOL	IsRunning()			{ return m_pUserCallback != NULL; }
		
		// Do *NOT* call this internal function:
		void	fillNextBuffer(void);

private:
	
		HWND							m_hWnd;
		long							m_bufferSize;
		long							m_currentBuffer;
		HWAVEOUT						m_hWaveOut;
		WAVEHDR							m_waveHeader[REPLAY_NBSOUNDBUFFER];
		void						*	m_pSoundBuffer[REPLAY_NBSOUNDBUFFER];
		volatile	USER_CALLBACK		m_pUserCallback;
};


#endif
