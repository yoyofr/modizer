/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#define PORTAUDIO_V19 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_PORTAUDIO_DLL

#ifdef __W32__
#include <windows.h>
#endif
#include <portaudio.h>


#include "w32_portaudio.h"


#ifdef PORTAUDIO_V19

#include <pa_asio.h>

extern int load_portaudio_dll(int);
extern void free_portaudio_dll(void);

/***************************************************************
 name: portaudio_dll  dll: portaudio.dll 
***************************************************************/

typedef int(*type_Pa_GetVersion)( void );
typedef const char*(*type_Pa_GetVersionText)( void );
typedef const char*(*type_Pa_GetErrorText)( PaError errorCode );
typedef PaError(*type_Pa_Initialize)( void );
typedef PaError(*type_Pa_Terminate)( void );
typedef PaHostApiIndex(*type_Pa_GetHostApiCount)( void );
typedef PaHostApiIndex(*type_Pa_GetDefaultHostApi)( void );
typedef const PaHostApiInfo *(*type_Pa_GetHostApiInfo)( PaHostApiIndex hostApi );
typedef PaHostApiIndex(*type_Pa_HostApiTypeIdToHostApiIndex)( PaHostApiTypeId type );
typedef const PaHostErrorInfo*(*type_Pa_GetLastHostErrorInfo)( void );
typedef PaDeviceIndex(*type_Pa_GetDeviceCount)( void );
typedef PaDeviceIndex(*type_Pa_GetDefaultInputDevice)( void );
typedef PaDeviceIndex(*type_Pa_GetDefaultOutputDevice)( void );
typedef const PaDeviceInfo*(*type_Pa_GetDeviceInfo)( PaDeviceIndex device );
typedef PaError(*type_Pa_IsFormatSupported)( const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters, double sampleRate );
typedef PaError(*type_Pa_OpenStream)( PaStream** stream, const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters, double sampleRate, unsigned long framesPerBuffer, PaStreamFlags streamFlags, PaStreamCallback *streamCallback, void *userData );
typedef PaError(*type_Pa_OpenDefaultStream)( PaStream** stream, int numInputChannels, int numOutputChannels, PaSampleFormat sampleFormat, double sampleRate, unsigned long framesPerBuffer, PaStreamCallback *streamCallback, void *userData );
typedef PaError(*type_Pa_CloseStream)( PaStream *stream );
//typedef typedef void(*type_PaStreamFinishedCallback)( void *userData );
typedef PaError(*type_Pa_SetStreamFinishedCallback)( PaStream *stream, PaStreamFinishedCallback* streamFinishedCallback );
typedef PaError(*type_Pa_StartStream)( PaStream *stream );
typedef PaError(*type_Pa_StopStream)( PaStream *stream );
typedef PaError(*type_Pa_AbortStream)( PaStream *stream );
typedef PaError(*type_Pa_IsStreamStopped)( PaStream *stream );
typedef PaError(*type_Pa_IsStreamActive)( PaStream *stream );
typedef const PaStreamInfo*(*type_Pa_GetStreamInfo)( PaStream *stream );
typedef PaTime(*type_Pa_GetStreamTime)( PaStream *stream );
typedef double(*type_Pa_GetStreamCpuLoad)( PaStream* stream );
typedef signed long(*type_Pa_GetStreamReadAvailable)( PaStream* stream );
typedef signed long(*type_Pa_GetStreamWriteAvailable)( PaStream* stream );
typedef PaError(*type_Pa_GetSampleSize)( PaSampleFormat format );
typedef void(*type_Pa_Sleep)( long msec );
typedef PaError(*type_PaAsio_ShowControlPanel)( PaDeviceIndex device, void* systemSpecific );

static struct portaudio_dll_ {
	 type_Pa_GetVersion Pa_GetVersion;
	 type_Pa_GetVersionText Pa_GetVersionText;
	 type_Pa_GetErrorText Pa_GetErrorText;
	 type_Pa_Initialize Pa_Initialize;
	 type_Pa_Terminate Pa_Terminate;
	 type_Pa_GetHostApiCount Pa_GetHostApiCount;
	 type_Pa_GetDefaultHostApi Pa_GetDefaultHostApi;
	 type_Pa_GetHostApiInfo Pa_GetHostApiInfo;
	 type_Pa_HostApiTypeIdToHostApiIndex Pa_HostApiTypeIdToHostApiIndex;
	 type_Pa_GetLastHostErrorInfo Pa_GetLastHostErrorInfo;
	 type_Pa_GetDeviceCount Pa_GetDeviceCount;
	 type_Pa_GetDefaultInputDevice Pa_GetDefaultInputDevice;
	 type_Pa_GetDefaultOutputDevice Pa_GetDefaultOutputDevice;
	 type_Pa_GetDeviceInfo Pa_GetDeviceInfo;
	 type_Pa_IsFormatSupported Pa_IsFormatSupported;
	 type_Pa_OpenStream Pa_OpenStream;
	 type_Pa_OpenDefaultStream Pa_OpenDefaultStream;
	 type_Pa_CloseStream Pa_CloseStream;
//	 type_PaStreamFinishedCallback PaStreamFinishedCallback;
	 type_Pa_SetStreamFinishedCallback Pa_SetStreamFinishedCallback;
	 type_Pa_StartStream Pa_StartStream;
	 type_Pa_StopStream Pa_StopStream;
	 type_Pa_AbortStream Pa_AbortStream;
	 type_Pa_IsStreamStopped Pa_IsStreamStopped;
	 type_Pa_IsStreamActive Pa_IsStreamActive;
	 type_Pa_GetStreamInfo Pa_GetStreamInfo;
	 type_Pa_GetStreamTime Pa_GetStreamTime;
	 type_Pa_GetStreamCpuLoad Pa_GetStreamCpuLoad;
	 type_Pa_GetStreamReadAvailable Pa_GetStreamReadAvailable;
	 type_Pa_GetStreamWriteAvailable Pa_GetStreamWriteAvailable;
	 type_Pa_GetSampleSize Pa_GetSampleSize;
	 type_Pa_Sleep Pa_Sleep;
	 type_PaAsio_ShowControlPanel PaAsio_ShowControlPanel;
} portaudio_dll;

static volatile HANDLE h_portaudio_dll = NULL;

void free_portaudio_dll(void)
{
	if(h_portaudio_dll){
		FreeLibrary(h_portaudio_dll);
		h_portaudio_dll = NULL;
	}
}

int load_portaudio_dll(int a)
{
	if(!h_portaudio_dll){
		h_portaudio_dll = LoadLibrary("portaudio.dll");
		if(!h_portaudio_dll) return -1;
	}
	portaudio_dll.Pa_GetVersion = (type_Pa_GetVersion)GetProcAddress(h_portaudio_dll,"Pa_GetVersion");
	if(!portaudio_dll.Pa_GetVersion){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetVersionText = (type_Pa_GetVersionText)GetProcAddress(h_portaudio_dll,"Pa_GetVersionText");
	if(!portaudio_dll.Pa_GetVersionText){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetErrorText = (type_Pa_GetErrorText)GetProcAddress(h_portaudio_dll,"Pa_GetErrorText");
	if(!portaudio_dll.Pa_GetErrorText){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_Initialize = (type_Pa_Initialize)GetProcAddress(h_portaudio_dll,"Pa_Initialize");
	if(!portaudio_dll.Pa_Initialize){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_Terminate = (type_Pa_Terminate)GetProcAddress(h_portaudio_dll,"Pa_Terminate");
	if(!portaudio_dll.Pa_Terminate){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetHostApiCount = (type_Pa_GetHostApiCount)GetProcAddress(h_portaudio_dll,"Pa_GetHostApiCount");
	if(!portaudio_dll.Pa_GetHostApiCount){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDefaultHostApi = (type_Pa_GetDefaultHostApi)GetProcAddress(h_portaudio_dll,"Pa_GetDefaultHostApi");
	if(!portaudio_dll.Pa_GetDefaultHostApi){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetHostApiInfo = (type_Pa_GetHostApiInfo)GetProcAddress(h_portaudio_dll,"Pa_GetHostApiInfo");
	if(!portaudio_dll.Pa_GetHostApiInfo){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_HostApiTypeIdToHostApiIndex = (type_Pa_HostApiTypeIdToHostApiIndex)GetProcAddress(h_portaudio_dll,"Pa_HostApiTypeIdToHostApiIndex");
	if(!portaudio_dll.Pa_HostApiTypeIdToHostApiIndex){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetLastHostErrorInfo = (type_Pa_GetLastHostErrorInfo)GetProcAddress(h_portaudio_dll,"Pa_GetLastHostErrorInfo");
	if(!portaudio_dll.Pa_GetLastHostErrorInfo){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDeviceCount = (type_Pa_GetDeviceCount)GetProcAddress(h_portaudio_dll,"Pa_GetDeviceCount");
	if(!portaudio_dll.Pa_GetDeviceCount){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDefaultInputDevice = (type_Pa_GetDefaultInputDevice)GetProcAddress(h_portaudio_dll,"Pa_GetDefaultInputDevice");
	if(!portaudio_dll.Pa_GetDefaultInputDevice){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDefaultOutputDevice = (type_Pa_GetDefaultOutputDevice)GetProcAddress(h_portaudio_dll,"Pa_GetDefaultOutputDevice");
	if(!portaudio_dll.Pa_GetDefaultOutputDevice){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDeviceInfo = (type_Pa_GetDeviceInfo)GetProcAddress(h_portaudio_dll,"Pa_GetDeviceInfo");
	if(!portaudio_dll.Pa_GetDeviceInfo){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_IsFormatSupported = (type_Pa_IsFormatSupported)GetProcAddress(h_portaudio_dll,"Pa_IsFormatSupported");
	if(!portaudio_dll.Pa_IsFormatSupported){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_OpenStream = (type_Pa_OpenStream)GetProcAddress(h_portaudio_dll,"Pa_OpenStream");
	if(!portaudio_dll.Pa_OpenStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_OpenDefaultStream = (type_Pa_OpenDefaultStream)GetProcAddress(h_portaudio_dll,"Pa_OpenDefaultStream");
	if(!portaudio_dll.Pa_OpenDefaultStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_CloseStream = (type_Pa_CloseStream)GetProcAddress(h_portaudio_dll,"Pa_CloseStream");
	if(!portaudio_dll.Pa_CloseStream){ free_portaudio_dll(); return -1; }
//	portaudio_dll.PaStreamFinishedCallback = (type_PaStreamFinishedCallback)GetProcAddress(h_portaudio_dll,"PaStreamFinishedCallback");
//	if(!portaudio_dll.PaStreamFinishedCallback){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_SetStreamFinishedCallback = (type_Pa_SetStreamFinishedCallback)GetProcAddress(h_portaudio_dll,"Pa_SetStreamFinishedCallback");
	if(!portaudio_dll.Pa_SetStreamFinishedCallback){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StartStream = (type_Pa_StartStream)GetProcAddress(h_portaudio_dll,"Pa_StartStream");
	if(!portaudio_dll.Pa_StartStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StopStream = (type_Pa_StopStream)GetProcAddress(h_portaudio_dll,"Pa_StopStream");
	if(!portaudio_dll.Pa_StopStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_AbortStream = (type_Pa_AbortStream)GetProcAddress(h_portaudio_dll,"Pa_AbortStream");
	if(!portaudio_dll.Pa_AbortStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_IsStreamStopped = (type_Pa_IsStreamStopped)GetProcAddress(h_portaudio_dll,"Pa_IsStreamStopped");
	if(!portaudio_dll.Pa_IsStreamStopped){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_IsStreamActive = (type_Pa_IsStreamActive)GetProcAddress(h_portaudio_dll,"Pa_IsStreamActive");
	if(!portaudio_dll.Pa_IsStreamActive){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetStreamInfo = (type_Pa_GetStreamInfo)GetProcAddress(h_portaudio_dll,"Pa_GetStreamInfo");
	if(!portaudio_dll.Pa_GetStreamInfo){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetStreamTime = (type_Pa_GetStreamTime)GetProcAddress(h_portaudio_dll,"Pa_GetStreamTime");
	if(!portaudio_dll.Pa_GetStreamTime){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetStreamCpuLoad = (type_Pa_GetStreamCpuLoad)GetProcAddress(h_portaudio_dll,"Pa_GetStreamCpuLoad");
	if(!portaudio_dll.Pa_GetStreamCpuLoad){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetStreamReadAvailable = (type_Pa_GetStreamReadAvailable)GetProcAddress(h_portaudio_dll,"Pa_GetStreamReadAvailable");
	if(!portaudio_dll.Pa_GetStreamReadAvailable){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetStreamWriteAvailable = (type_Pa_GetStreamWriteAvailable)GetProcAddress(h_portaudio_dll,"Pa_GetStreamWriteAvailable");
	if(!portaudio_dll.Pa_GetStreamWriteAvailable){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetSampleSize = (type_Pa_GetSampleSize)GetProcAddress(h_portaudio_dll,"Pa_GetSampleSize");
	if(!portaudio_dll.Pa_GetSampleSize){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_Sleep = (type_Pa_Sleep)GetProcAddress(h_portaudio_dll,"Pa_Sleep");
	if(!portaudio_dll.Pa_Sleep){ free_portaudio_dll(); return -1; }
	portaudio_dll.PaAsio_ShowControlPanel = (type_PaAsio_ShowControlPanel)GetProcAddress(h_portaudio_dll,"PaAsio_ShowControlPanel");
	if(!portaudio_dll.PaAsio_ShowControlPanel){ free_portaudio_dll(); return -1; }
	return 0;
}

int Pa_GetVersion( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetVersion();
	}
	return (int)0;
}

const char* Pa_GetVersionText( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetVersionText();
	}
	return (const char*)0;
}

const char* Pa_GetErrorText( PaError errorCode )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetErrorText(errorCode);
	}
	return (const char*)0;
}

PaError Pa_Initialize( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_Initialize();
	}
	return (PaError)0;
}

PaError Pa_Terminate( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_Terminate();
	}
	return (PaError)0;
}

PaHostApiIndex Pa_GetHostApiCount( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetHostApiCount();
	}
	return (PaHostApiIndex)0;
}

PaHostApiIndex Pa_GetDefaultHostApi( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDefaultHostApi();
	}
	return (PaHostApiIndex)0;
}

const PaHostApiInfo * Pa_GetHostApiInfo( PaHostApiIndex hostApi )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetHostApiInfo(hostApi);
	}
	return (const PaHostApiInfo *)0;
}

PaHostApiIndex Pa_HostApiTypeIdToHostApiIndex( PaHostApiTypeId type )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_HostApiTypeIdToHostApiIndex(type);
	}
	return (PaHostApiIndex)0;
}

const PaHostErrorInfo* Pa_GetLastHostErrorInfo( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetLastHostErrorInfo();
	}
	return (const PaHostErrorInfo*)0;
}

PaDeviceIndex Pa_GetDeviceCount( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDeviceCount();
	}
	return (PaDeviceIndex)0;
}

PaDeviceIndex Pa_GetDefaultInputDevice( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDefaultInputDevice();
	}
	return (PaDeviceIndex)0;
}

PaDeviceIndex Pa_GetDefaultOutputDevice( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDefaultOutputDevice();
	}
	return (PaDeviceIndex)0;
}

const PaDeviceInfo* Pa_GetDeviceInfo( PaDeviceIndex device )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDeviceInfo(device);
	}
	return (const PaDeviceInfo*)0;
}

PaError Pa_IsFormatSupported( const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters, double sampleRate )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_IsFormatSupported(inputParameters,outputParameters,sampleRate);
	}
	return (PaError)0;
}

PaError Pa_OpenStream( PaStream** stream, const PaStreamParameters *inputParameters, const PaStreamParameters *outputParameters, double sampleRate, unsigned long framesPerBuffer, PaStreamFlags streamFlags, PaStreamCallback *streamCallback, void *userData )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_OpenStream(stream,inputParameters,outputParameters,sampleRate,framesPerBuffer,streamFlags,streamCallback,userData);
	}
	return (PaError)0;
}

PaError Pa_OpenDefaultStream( PaStream** stream, int numInputChannels, int numOutputChannels, PaSampleFormat sampleFormat, double sampleRate, unsigned long framesPerBuffer, PaStreamCallback *streamCallback, void *userData )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_OpenDefaultStream(stream,numInputChannels,numOutputChannels,sampleFormat,sampleRate,framesPerBuffer,streamCallback,userData);
	}
	return (PaError)0;
}

PaError Pa_CloseStream( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_CloseStream(stream);
	}
	return (PaError)0;
}
/*
typedef void PaStreamFinishedCallback( void *userData )
{
	if(h_portaudio_dll){
		portaudio_dll.PaStreamFinishedCallback(userData);
	}
}
*/
PaError Pa_SetStreamFinishedCallback( PaStream *stream, PaStreamFinishedCallback* streamFinishedCallback )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_SetStreamFinishedCallback(stream,streamFinishedCallback);
	}
	return (PaError)0;
}

PaError Pa_StartStream( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StartStream(stream);
	}
	return (PaError)0;
}

PaError Pa_StopStream( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StopStream(stream);
	}
	return (PaError)0;
}

PaError Pa_AbortStream( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_AbortStream(stream);
	}
	return (PaError)0;
}

PaError Pa_IsStreamStopped( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_IsStreamStopped(stream);
	}
	return (PaError)0;
}

PaError Pa_IsStreamActive( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_IsStreamActive(stream);
	}
	return (PaError)0;
}

const PaStreamInfo* Pa_GetStreamInfo( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetStreamInfo(stream);
	}
	return (const PaStreamInfo*)0;
}

PaTime Pa_GetStreamTime( PaStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetStreamTime(stream);
	}
	return (PaTime)0;
}

double Pa_GetStreamCpuLoad( PaStream* stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetStreamCpuLoad(stream);
	}
	return (double)0;
}

signed long Pa_GetStreamReadAvailable( PaStream* stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetStreamReadAvailable(stream);
	}
	return (signed long)0;
}

signed long Pa_GetStreamWriteAvailable( PaStream* stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetStreamWriteAvailable(stream);
	}
	return (signed long)0;
}

PaError Pa_GetSampleSize( PaSampleFormat format )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetSampleSize(format);
	}
	return (PaError)0;
}

void Pa_Sleep( long msec )
{
	if(h_portaudio_dll){
		portaudio_dll.Pa_Sleep(msec);
	}
}
PaError PaAsio_ShowControlPanel( PaDeviceIndex device, void* systemSpecific )
{
	if(h_portaudio_dll){
		return portaudio_dll.PaAsio_ShowControlPanel(device,systemSpecific);
	}
	return (PaError)0;
}

/***************************************************************/



#else



extern int load_portaudio_dll(int type);
extern void free_portaudio_dll(void);

/***************************************************************
 name: portaudio_dll  dll: portaudio.dll 
***************************************************************/

// (PaError)0 -> paDeviceUnavailable
// (PaDeviceID)0 -> paNoDevice

// #define PA_DLL_ASIO     1
// #define PA_DLL_WIN_DS   2
// #define PA_DLL_WIN_WMME 3
#define PA_DLL_ASIO_FILE     "pa_asio.dll"
#define PA_DLL_WIN_DS_FILE   "pa_win_ds.dll"
#define PA_DLL_WIN_WMME_FILE "pa_win_wmme.dll"

typedef PaError(*type_Pa_Initialize)( void );
typedef PaError(*type_Pa_Terminate)( void );
typedef long(*type_Pa_GetHostError)( void );
typedef const char*(*type_Pa_GetErrorText)( PaError errnum );
typedef int(*type_Pa_CountDevices)( void );
typedef PaDeviceID(*type_Pa_GetDefaultInputDeviceID)( void );
typedef PaDeviceID(*type_Pa_GetDefaultOutputDeviceID)( void );
typedef const PaDeviceInfo*(*type_Pa_GetDeviceInfo)( PaDeviceID device );
typedef PaError(*type_Pa_OpenStream)( PortAudioStream** stream,PaDeviceID inputDevice,int numInputChannels,PaSampleFormat inputSampleFormat,void *inputDriverInfo,PaDeviceID outputDevice,int numOutputChannels,PaSampleFormat outputSampleFormat,void *outputDriverInfo,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PaStreamFlags streamFlags,PortAudioCallback *callback,void *userData );
typedef PaError(*type_Pa_OpenDefaultStream)( PortAudioStream** stream,int numInputChannels,int numOutputChannels,PaSampleFormat sampleFormat,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PortAudioCallback *callback,void *userData );
typedef PaError(*type_Pa_CloseStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_StartStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_StopStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_AbortStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_StreamActive)( PortAudioStream *stream );
typedef PaTimestamp(*type_Pa_StreamTime)( PortAudioStream *stream );
typedef double(*type_Pa_GetCPULoad)( PortAudioStream* stream );
typedef int(*type_Pa_GetMinNumBuffers)( int framesPerBuffer, double sampleRate );
typedef void(*type_Pa_Sleep)( long msec );
typedef PaError(*type_Pa_GetSampleSize)( PaSampleFormat format );

static struct portaudio_dll_ {
	 type_Pa_Initialize Pa_Initialize;
	 type_Pa_Terminate Pa_Terminate;
	 type_Pa_GetHostError Pa_GetHostError;
	 type_Pa_GetErrorText Pa_GetErrorText;
	 type_Pa_CountDevices Pa_CountDevices;
	 type_Pa_GetDefaultInputDeviceID Pa_GetDefaultInputDeviceID;
	 type_Pa_GetDefaultOutputDeviceID Pa_GetDefaultOutputDeviceID;
	 type_Pa_GetDeviceInfo Pa_GetDeviceInfo;
	 type_Pa_OpenStream Pa_OpenStream;
	 type_Pa_OpenDefaultStream Pa_OpenDefaultStream;
	 type_Pa_CloseStream Pa_CloseStream;
	 type_Pa_StartStream Pa_StartStream;
	 type_Pa_StopStream Pa_StopStream;
	 type_Pa_AbortStream Pa_AbortStream;
	 type_Pa_StreamActive Pa_StreamActive;
	 type_Pa_StreamTime Pa_StreamTime;
	 type_Pa_GetCPULoad Pa_GetCPULoad;
	 type_Pa_GetMinNumBuffers Pa_GetMinNumBuffers;
	 type_Pa_Sleep Pa_Sleep;
	 type_Pa_GetSampleSize Pa_GetSampleSize;
} portaudio_dll;

static volatile HANDLE h_portaudio_dll = NULL;

void free_portaudio_dll(void)
{
	if(h_portaudio_dll){
		FreeLibrary(h_portaudio_dll);
		h_portaudio_dll = NULL;
	}
}

int load_portaudio_dll(int type)
{
	char* dll_file = NULL;
	switch(type){
	case PA_DLL_ASIO:
		dll_file = PA_DLL_ASIO_FILE;
		break;
	case PA_DLL_WIN_DS:
		dll_file = PA_DLL_WIN_DS_FILE;
		break;
	case PA_DLL_WIN_WMME:
		dll_file = PA_DLL_WIN_WMME_FILE;
		break;
	default:
		return -1;
	}
	if(!h_portaudio_dll){
		h_portaudio_dll = LoadLibrary(dll_file);
		if(!h_portaudio_dll) return -1;
	}
	portaudio_dll.Pa_Initialize = (type_Pa_Initialize)GetProcAddress(h_portaudio_dll,"Pa_Initialize");
	if(!portaudio_dll.Pa_Initialize){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_Terminate = (type_Pa_Terminate)GetProcAddress(h_portaudio_dll,"Pa_Terminate");
	if(!portaudio_dll.Pa_Terminate){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetHostError = (type_Pa_GetHostError)GetProcAddress(h_portaudio_dll,"Pa_GetHostError");
	if(!portaudio_dll.Pa_GetHostError){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetErrorText = (type_Pa_GetErrorText)GetProcAddress(h_portaudio_dll,"Pa_GetErrorText");
	if(!portaudio_dll.Pa_GetErrorText){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_CountDevices = (type_Pa_CountDevices)GetProcAddress(h_portaudio_dll,"Pa_CountDevices");
	if(!portaudio_dll.Pa_CountDevices){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDefaultInputDeviceID = (type_Pa_GetDefaultInputDeviceID)GetProcAddress(h_portaudio_dll,"Pa_GetDefaultInputDeviceID");
	if(!portaudio_dll.Pa_GetDefaultInputDeviceID){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDefaultOutputDeviceID = (type_Pa_GetDefaultOutputDeviceID)GetProcAddress(h_portaudio_dll,"Pa_GetDefaultOutputDeviceID");
	if(!portaudio_dll.Pa_GetDefaultOutputDeviceID){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDeviceInfo = (type_Pa_GetDeviceInfo)GetProcAddress(h_portaudio_dll,"Pa_GetDeviceInfo");
	if(!portaudio_dll.Pa_GetDeviceInfo){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_OpenStream = (type_Pa_OpenStream)GetProcAddress(h_portaudio_dll,"Pa_OpenStream");
	if(!portaudio_dll.Pa_OpenStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_OpenDefaultStream = (type_Pa_OpenDefaultStream)GetProcAddress(h_portaudio_dll,"Pa_OpenDefaultStream");
	if(!portaudio_dll.Pa_OpenDefaultStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_CloseStream = (type_Pa_CloseStream)GetProcAddress(h_portaudio_dll,"Pa_CloseStream");
	if(!portaudio_dll.Pa_CloseStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StartStream = (type_Pa_StartStream)GetProcAddress(h_portaudio_dll,"Pa_StartStream");
	if(!portaudio_dll.Pa_StartStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StopStream = (type_Pa_StopStream)GetProcAddress(h_portaudio_dll,"Pa_StopStream");
	if(!portaudio_dll.Pa_StopStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_AbortStream = (type_Pa_AbortStream)GetProcAddress(h_portaudio_dll,"Pa_AbortStream");
	if(!portaudio_dll.Pa_AbortStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StreamActive = (type_Pa_StreamActive)GetProcAddress(h_portaudio_dll,"Pa_StreamActive");
	if(!portaudio_dll.Pa_StreamActive){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StreamTime = (type_Pa_StreamTime)GetProcAddress(h_portaudio_dll,"Pa_StreamTime");
	if(!portaudio_dll.Pa_StreamTime){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetCPULoad = (type_Pa_GetCPULoad)GetProcAddress(h_portaudio_dll,"Pa_GetCPULoad");
	if(!portaudio_dll.Pa_GetCPULoad){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetMinNumBuffers = (type_Pa_GetMinNumBuffers)GetProcAddress(h_portaudio_dll,"Pa_GetMinNumBuffers");
	if(!portaudio_dll.Pa_GetMinNumBuffers){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_Sleep = (type_Pa_Sleep)GetProcAddress(h_portaudio_dll,"Pa_Sleep");
	if(!portaudio_dll.Pa_Sleep){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetSampleSize = (type_Pa_GetSampleSize)GetProcAddress(h_portaudio_dll,"Pa_GetSampleSize");
	if(!portaudio_dll.Pa_GetSampleSize){ free_portaudio_dll(); return -1; }
	return 0;
}

PaError Pa_Initialize( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_Initialize();
	}
	return paDeviceUnavailable;
}

PaError Pa_Terminate( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_Terminate();
	}
	return paDeviceUnavailable;
}

long Pa_GetHostError( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetHostError();
	}
	return (long)0;
}

const char* Pa_GetErrorText( PaError errnum )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetErrorText(errnum);
	}
	return (const char*)0;
}

int Pa_CountDevices( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_CountDevices();
	}
	return (int)0;
}

PaDeviceID Pa_GetDefaultInputDeviceID( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDefaultInputDeviceID();
	}
	return paNoDevice;
}

PaDeviceID Pa_GetDefaultOutputDeviceID( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDefaultOutputDeviceID();
	}
	return paNoDevice;
}

const PaDeviceInfo* Pa_GetDeviceInfo( PaDeviceID device )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDeviceInfo(device);
	}
	return (const PaDeviceInfo*)0;
}

PaError Pa_OpenStream( PortAudioStream** stream,PaDeviceID inputDevice,int numInputChannels,PaSampleFormat inputSampleFormat,void *inputDriverInfo,PaDeviceID outputDevice,int numOutputChannels,PaSampleFormat outputSampleFormat,void *outputDriverInfo,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PaStreamFlags streamFlags,PortAudioCallback *callback,void *userData )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_OpenStream(stream,inputDevice,numInputChannels,inputSampleFormat,inputDriverInfo,outputDevice,numOutputChannels,outputSampleFormat,outputDriverInfo,sampleRate,framesPerBuffer,numberOfBuffers,streamFlags,callback,userData);
	}
	return (PaError)0;
}

PaError Pa_OpenDefaultStream( PortAudioStream** stream,int numInputChannels,int numOutputChannels,PaSampleFormat sampleFormat,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PortAudioCallback *callback,void *userData )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_OpenDefaultStream(stream,numInputChannels,numOutputChannels,sampleFormat,sampleRate,framesPerBuffer,numberOfBuffers,callback,userData);
	}
	return (PaError)0;
}

PaError Pa_CloseStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_CloseStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_StartStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StartStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_StopStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StopStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_AbortStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_AbortStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_StreamActive( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StreamActive(stream);
	}
	return paDeviceUnavailable;
}

PaTimestamp Pa_StreamTime( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StreamTime(stream);
	}
	return (PaTimestamp)0;
}

double Pa_GetCPULoad( PortAudioStream* stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetCPULoad(stream);
	}
	return (double)0;
}

int Pa_GetMinNumBuffers( int framesPerBuffer, double sampleRate )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetMinNumBuffers(framesPerBuffer,sampleRate);
	}
	return (int)0;
}

void Pa_Sleep( long msec )
{
	if(h_portaudio_dll){
		portaudio_dll.Pa_Sleep(msec);
	}
}

PaError Pa_GetSampleSize( PaSampleFormat format )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetSampleSize(format);
	}
	return paDeviceUnavailable;
}

/***************************************************************/
#endif

#endif /* AU_PORTAUDIO_DLL */

