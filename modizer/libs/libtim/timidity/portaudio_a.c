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

    portaudio_a.c by skeishi <s_keishi@mutt.freemail.ne.jp>
    based on esd_a.c

    Functions to play sound through Portaudio
*/
#define PORTAUDIO_V19 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#ifdef __POCC__
#include <sys/types.h>
#endif //for off_t
#define _GNU_SOURCE
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <fcntl.h>

#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#if defined(PORTAUDIO_V19) && defined(__W32__)
#include <windows.h>
#endif
#include <portaudio.h>
#if defined(PORTAUDIO_V19) && defined(__W32__)
#include <pa_asio.h>
#endif

#ifdef AU_PORTAUDIO_DLL
#include "w32_portaudio.h"
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "timer.h"
#include "instrum.h"
#include "playmidi.h"
#include "miditrace.h"

static int opt_pa_device_id = -2;

#define DATA_BLOCK_SIZE     (27648*4) /* WinNT Latency is 600 msec read pa_dsound.c */
#define SAMPLE_RATE         (44100)

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);

static void print_device_list(void);

static int framesPerBuffer=128;
static int stereo=2;
static int conv16_32 =0;
static int data_nbyte;
static int numBuffers;
static unsigned int framesPerInBuffer;
static unsigned int bytesPerInBuffer=0;
//static int  firsttime;
static int pa_active=0;
static int first=1;

#if PORTAUDIO_V19
PaHostApiTypeId HostApiTypeId;
PaHostApiIndex HostApiIndex;
const PaHostApiInfo  *HostApiInfo;
PaDeviceIndex DeviceIndex;
const PaDeviceInfo *DeviceInfo;
PaStreamParameters StreamParameters;
PaStream *stream;
PaError  err;
#else
PaDeviceID DeviceID;
const PaDeviceInfo *DeviceInfo;
PortAudioStream *stream;
PaError  err;
#endif
typedef struct {
	char buf[DATA_BLOCK_SIZE*2];
	uint32 samplesToGo;
	char *bufpoint;
	char *bufepoint;
} padata_t;
padata_t pa_data;

/* export the playback mode */

#ifdef AU_PORTAUDIO_DLL
static int open_output_asio(void);
static int open_output_win_ds(void);
static int open_output_win_wmme(void);
PlayMode portaudio_asio_play_mode = {
	(SAMPLE_RATE),
    PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM|PF_BUFF_FRAGM_OPT|PF_CAN_TRACE,
    -1,
    {32}, /* PF_BUFF_FRAGM_OPT  is need for TWSYNTH */
	"PortAudio(ASIO)", 'o',
    NULL,
    open_output_asio,
    close_output,
    output_data,
    acntl
};
PlayMode portaudio_win_ds_play_mode = {
	(SAMPLE_RATE),
    PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM|PF_BUFF_FRAGM_OPT|PF_CAN_TRACE,
    -1,
    {32}, /* PF_BUFF_FRAGM_OPT  is need for TWSYNTH */
	"PortAudio(DirectSound)", 'P',
    NULL,
    open_output_win_ds,
    close_output,
    output_data,
    acntl
};
PlayMode portaudio_win_wmme_play_mode = {
	(SAMPLE_RATE),
    PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM|PF_BUFF_FRAGM_OPT|PF_CAN_TRACE,
    -1,
    {32}, /* PF_BUFF_FRAGM_OPT  is need for TWSYNTH */
	"PortAudio(WMME)", 'p',
    NULL,
    open_output_win_wmme,
    close_output,
    output_data,
    acntl
};
PlayMode * volatile portaudio_play_mode = &portaudio_win_wmme_play_mode;
#define dpm (*portaudio_play_mode)

#else

#define dpm portaudio_play_mode

PlayMode dpm = {
	(SAMPLE_RATE),
    PE_16BIT|PE_SIGNED,
    PF_PCM_STREAM|PF_BUFF_FRAGM_OPT|PF_CAN_TRACE,
    -1,
    {32}, /* PF_BUFF_FRAGM_OPT  is need for TWSYNTH */
	"Portaudio Driver", 'p',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl
};
#endif

#if PORTAUDIO_V19
int paCallback(  const void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
                     const PaStreamCallbackTimeInfo* timeInfo,
                     PaStreamCallbackFlags statusFlags,
	                 void *userData )
#else
int paCallback(  void *inputBuffer, void *outputBuffer,
                     unsigned long framesPerBuffer,
                     PaTimestamp outTime, 
                     void *userData )
#endif
{

    unsigned int i;
	int finished = 0;

/* Cast data passed through stream to our structure type. */
//    pa_data_t pa_data = (pa_data_t*)userData;
	
	int32 samplesToGo;
	char *bufpoint;
    char *out = (char*)outputBuffer;
	unsigned long datalength = framesPerBuffer*data_nbyte*stereo;
	char * buflimit = pa_data.buf+bytesPerInBuffer*2;
	
	samplesToGo=pa_data.samplesToGo;
	bufpoint=pa_data.bufpoint;
	
	if(conv16_32){
		if(samplesToGo < datalength  ){		
			for(i=0;i<samplesToGo/2;i++){
				*out++ = 0;
				*out++ = 0;
				*out++ = *(bufpoint)++;
				*out++ = *(bufpoint)++;
				if( buflimit <= bufpoint ){
					bufpoint=pa_data.buf;
				}
			}
			samplesToGo=0;
			for(;i<datalength/2;i++){
				*out++ = 0;
				*out++ = 0;
				*out++ = 0;
				*out++ = 0;
			}
			finished = 0;
		}else{
			for(i=0;i<datalength/2;i++){
				*out++ = 0;
				*out++ = 0;
				*out++=*(bufpoint)++;
				*out++=*(bufpoint)++;
				if( buflimit <= bufpoint ){
					bufpoint=pa_data.buf;
				}
			}
			samplesToGo -= datalength;
		}
	}else{
		if(samplesToGo < datalength  ){
			if(bufpoint+samplesToGo <= buflimit){
				memcpy(out, bufpoint, samplesToGo);
				bufpoint += samplesToGo;
			}else{
				int32 send;
				send = buflimit-bufpoint;
				if (send !=0) memcpy(out, bufpoint, send);
				out +=send;
				memcpy(out, pa_data.buf, samplesToGo -send);
				bufpoint = pa_data.buf+samplesToGo -send;
				out += samplesToGo -send;
			}
			memset(out, 0x0, datalength-samplesToGo);
			samplesToGo=0;
			finished = 0;
		}else{
			if(bufpoint + datalength <= buflimit){
				memcpy(out, bufpoint, datalength);
				bufpoint += datalength;
			}else{
				int32 send;
				send = buflimit-bufpoint;
				if (send !=0) memcpy(out, bufpoint, send);
				out += send;
				memcpy(out, pa_data.buf, datalength -send);
				bufpoint = pa_data.buf+datalength -send;
			}
			samplesToGo -= datalength;
		}
	}
	pa_data.samplesToGo=samplesToGo;
	pa_data.bufpoint=bufpoint;
	return finished ;

}

#ifdef AU_PORTAUDIO_DLL
static int open_output_asio(void)
{
	portaudio_play_mode = &portaudio_asio_play_mode;
	return open_output();
}
static int open_output_win_ds(void)
{
	portaudio_play_mode = &portaudio_win_ds_play_mode;
	return open_output();
}
static int open_output_win_wmme(void)
{
	portaudio_play_mode = &portaudio_win_wmme_play_mode;
	return open_output();
}
#endif

static int open_output(void)
{
	double rate;
	int n, nrates, include_enc, exclude_enc, ret;
	PaSampleFormat SampleFormat, nativeSampleFormats;
	
	if( dpm.name != NULL)
		ret = sscanf(dpm.name, "%d", &opt_pa_device_id);
	if (dpm.name == NULL || ret == 0 || ret == EOF)
		opt_pa_device_id = -2;
	
#ifdef AU_PORTAUDIO_DLL
#if PORTAUDIO_V19
  {
		if(&dpm == &portaudio_asio_play_mode){
			HostApiTypeId = paASIO;
		} else if(&dpm == &portaudio_win_ds_play_mode){
			HostApiTypeId = paDirectSound;
		} else if(&dpm == &portaudio_win_wmme_play_mode){
			HostApiTypeId = paMME;
		} else {
			return -1;
		}
		if(load_portaudio_dll(0))
				return -1;
  }
#else
  {
		if(&dpm == &portaudio_asio_play_mode){
			if(load_portaudio_dll(PA_DLL_ASIO))
				return -1;
		} else if(&dpm == &portaudio_win_ds_play_mode){
			if(load_portaudio_dll(PA_DLL_WIN_DS))
				return -1;
		} else if(&dpm == &portaudio_win_wmme_play_mode){
			if(load_portaudio_dll(PA_DLL_WIN_WMME))
				return -1;
		} else {
			return -1;
		}
  }
#endif
#endif
	/* if call twice Pa_OpenStream causes paDeviceUnavailable error  */
	if(pa_active == 1) return 0; 
	if(pa_active == 0){
		err = Pa_Initialize();
		if( err != paNoError ) goto error;
		pa_active = 1;
	}

	if (opt_pa_device_id == -1){
		print_device_list();
		goto error2;
	}
#ifdef PORTAUDIO_V19
#ifdef AU_PORTAUDIO_DLL
       {	
        PaHostApiIndex i, ApiCount;
	i = 0;
	ApiCount = Pa_GetHostApiCount();
	do{
		HostApiInfo=Pa_GetHostApiInfo(i);
		if( HostApiInfo->type == HostApiTypeId ) break;
		i++;
	}while ( i < ApiCount );
	if ( i == ApiCount ) goto error;
	
	DeviceIndex = HostApiInfo->defaultOutputDevice;
	if(DeviceIndex==paNoDevice) goto error;
        }
#else
	DeviceIndex = Pa_GetDefaultOutputDevice();
	if(DeviceIndex==paNoDevice) goto error;
#endif
	DeviceInfo = Pa_GetDeviceInfo( DeviceIndex);
	if(DeviceInfo==NULL) goto error;

	if(opt_pa_device_id != -2){
		const PaDeviceInfo *id_DeviceInfo;
    	id_DeviceInfo=Pa_GetDeviceInfo((PaDeviceIndex)opt_pa_device_id);
		if(id_DeviceInfo==NULL) goto error;
		if( DeviceInfo->hostApi == id_DeviceInfo->hostApi){
			DeviceIndex=(PaDeviceIndex)opt_pa_device_id;
			DeviceInfo = id_DeviceInfo;
		}
    }


	if (dpm.encoding & PE_24BIT) {
		SampleFormat = paInt24;
	}else if (dpm.encoding & PE_16BIT) {
		SampleFormat = paInt16;
	}else {
		SampleFormat = paInt8;
	}

	stereo = (dpm.encoding & PE_MONO) ? 1 : 2;
	data_nbyte = (dpm.encoding & PE_16BIT) ? 2 : 1;
	data_nbyte = (dpm.encoding & PE_24BIT) ? 3 : data_nbyte;
	
	pa_data.samplesToGo = 0;
	pa_data.bufpoint = pa_data.buf;
	pa_data.bufepoint = pa_data.buf;
//	firsttime = 1;
	numBuffers = 1; //Pa_GetMinNumBuffers( framesPerBuffer, dpm.rate );
	framesPerInBuffer = numBuffers * framesPerBuffer;
	if (framesPerInBuffer < 4096) framesPerInBuffer = 4096;
	bytesPerInBuffer = framesPerInBuffer * data_nbyte * stereo;

	/* set StreamParameters */
	StreamParameters.device = DeviceIndex;
	StreamParameters.channelCount = stereo;
	if(ctl->id_character != 'r' && ctl->id_character != 'A' && ctl->id_character != 'W' && ctl->id_character != 'P'){
		StreamParameters.suggestedLatency = DeviceInfo->defaultHighOutputLatency;
	}else{
		StreamParameters.suggestedLatency = DeviceInfo->defaultLowOutputLatency;
	}
	StreamParameters.hostApiSpecificStreamInfo = NULL;
	
	if( SampleFormat == paInt16){
		StreamParameters.sampleFormat = paInt16;
		if( paFormatIsSupported != Pa_IsFormatSupported( NULL , 
							&StreamParameters,(double) dpm.rate )){
			StreamParameters.sampleFormat = paInt32;
			conv16_32 = 1;
		} else {
			StreamParameters.sampleFormat = paInt16;
			conv16_32 = 0;
		}
	}else{
		StreamParameters.sampleFormat = SampleFormat;
	}
	err = Pa_IsFormatSupported( NULL ,
                             &StreamParameters,
							(double) dpm.rate );
	if ( err != paNoError) goto error;
	err = Pa_OpenStream(    
		& stream,			/* passes back stream pointer */
		NULL,			 	/* inputStreamParameters */
		&StreamParameters,	/* outputStreamParameters */
		(double) dpm.rate,	/* sample rate */
		paFramesPerBufferUnspecified,	/* frames per buffer */
		paFramesPerBufferUnspecified,	/* PaStreamFlags */
		paCallback,			/* specify our custom callback */
		&pa_data			/* pass our data through to callback */
		);
//		Pa_Sleeep(1);
	if ( err != paNoError) goto error;
	return 0;
	
#else
	if(opt_pa_device_id == -2){
		DeviceID = Pa_GetDefaultOutputDeviceID();
	    if(DeviceID==paNoDevice) goto error2;
	}else{
		DeviceID = opt_pa_device_id;
	}
	DeviceInfo = Pa_GetDeviceInfo( DeviceID);	
	if(DeviceInfo==NULL) goto error2;
	nativeSampleFormats = DeviceInfo->nativeSampleFormats;

	exclude_enc = PE_ULAW | PE_ALAW | PE_BYTESWAP;
	include_enc = PE_SIGNED;
	if (!(nativeSampleFormats & paInt16) && !(nativeSampleFormats & paInt32)) {exclude_enc |= PE_16BIT;}
	if (!(nativeSampleFormats & paInt24)) {exclude_enc |= PE_24BIT;}
    dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);

	if (dpm.encoding & PE_24BIT) {
		SampleFormat = paInt24;
	}else if (dpm.encoding & PE_16BIT) {
		if(nativeSampleFormats & paInt16) SampleFormat = paInt16;
		else{
			SampleFormat = paInt32;
			conv16_32 = 1;
		}
	}else {
		SampleFormat = paInt8;
	}

	stereo = (dpm.encoding & PE_MONO) ? 1 : 2;
	data_nbyte = (dpm.encoding & PE_16BIT) ? 2 : 1;
	data_nbyte = (dpm.encoding & PE_24BIT) ? 3 : data_nbyte;

	nrates = DeviceInfo->numSampleRates;
	if (nrates == -1) {	/* range supported */
		rate = dpm.rate;
		if (dpm.rate < DeviceInfo->sampleRates[0]) rate = DeviceInfo->sampleRates[0];
		if (dpm.rate > DeviceInfo->sampleRates[1]) rate = DeviceInfo->sampleRates[1];
	} else {
		rate = DeviceInfo->sampleRates[nrates-1];
		for (n = nrates - 1; n >= 0; n--) {	/* find nearest sample rate */
			if (dpm.rate <= DeviceInfo->sampleRates[n]) rate=DeviceInfo->sampleRates[n];
		}
	}
	dpm.rate = (int32)rate;
	
	pa_data.samplesToGo = 0;
	pa_data.bufpoint = pa_data.buf;
	pa_data.bufepoint = pa_data.buf;
//	firsttime = 1;
	numBuffers = Pa_GetMinNumBuffers( framesPerBuffer, dpm.rate );
	framesPerInBuffer = numBuffers * framesPerBuffer;
	if (framesPerInBuffer < 4096) framesPerInBuffer = 4096;
	bytesPerInBuffer = framesPerInBuffer * data_nbyte * stereo;
//	printf("%d\n",framesPerInBuffer);
//	printf("%d\n",dpm.rate);
	err = Pa_OpenDefaultStream(
    	&stream,        /* passes back stream pointer */
    	0,              /* no input channels */
    	stereo,              /* 2:stereo 1:mono output */
    	SampleFormat,      /* 24bit 16bit 8bit output */
		(double)dpm.rate,          /* sample rate */
    	framesPerBuffer,            /* frames per buffer */
    	numBuffers,              /* number of buffers, if zero then use default minimum */
    	paCallback, /* specify our custom callback */
    	&pa_data);   /* pass our data through to callback */
	if ( err != paNoError && err != paHostError) goto error;
	return 0;

#endif

error:
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortAudio error: %s\n", Pa_GetErrorText( err ) );
error2:
	Pa_Terminate(); pa_active = 0;
#ifdef AU_PORTAUDIO_DLL
#ifndef PORTAUDIO_V19
  free_portaudio_dll();
#endif
#endif

	return -1;
}
static int output_data(char *buf, int32 nbytes)
{
	unsigned int i;
	int32 samplesToGo;
	char *bufepoint;

    if(pa_active == 0) return -1; 
	
	while((pa_active==1) && (pa_data.samplesToGo > bytesPerInBuffer)){ Pa_Sleep(1);};
	
//	if(pa_data.samplesToGo > DATA_BLOCK_SIZE){ 
//		Sleep(  (pa_data.samplesToGo - DATA_BLOCK_SIZE)/dpm.rate/4  );
//	}
	samplesToGo=pa_data.samplesToGo;
	bufepoint=pa_data.bufepoint;

	if (pa_data.buf+bytesPerInBuffer*2 >= bufepoint + nbytes){
		memcpy(bufepoint, buf, nbytes);
		bufepoint += nbytes;
		//buf += nbytes;
	}else{
		int32 send = pa_data.buf+bytesPerInBuffer*2 - bufepoint;
		if (send != 0) memcpy(bufepoint, buf, send);
		buf += send;
		memcpy(pa_data.buf, buf, nbytes - send);
		bufepoint = pa_data.buf + nbytes - send;
		//buf += nbytes-send;
	}
	samplesToGo += nbytes;

	pa_data.samplesToGo=samplesToGo;
	pa_data.bufepoint=bufepoint;

/*
	if(firsttime==1){
		err = Pa_StartStream( stream );

		if( err != paNoError ) goto error;
		firsttime=0;
	}
*/
#if PORTAUDIO_V19
	if( 0==Pa_IsStreamActive(stream)){
#else
	if( 0==Pa_StreamActive(stream)){
#endif
		err = Pa_StartStream( stream );

		if( err != paNoError ) goto error;
	}
		
//	if(ctl->id_character != 'r' && ctl->id_character != 'A' && ctl->id_character != 'W' && ctl->id_character != 'P')
//	    while((pa_active==1) && (pa_data.samplesToGo > bytesPerInBuffer)){ Pa_Sleep(1);};
//	Pa_Sleep( (pa_data.samplesToGo - bytesPerInBuffer)/dpm.rate * 1000);
	return 0;

error:
	Pa_Terminate(); pa_active=0;
	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "PortAudio error: %s\n", Pa_GetErrorText( err ) );
	return -1;
}

static void close_output(void)
{	
	if( pa_active==0) return;
#ifdef PORTAUDIO_V19
	if(Pa_IsStreamActive(stream)){
#else
	if(Pa_StreamActive(stream)){
#endif
		Pa_Sleep(  bytesPerInBuffer/dpm.rate*1000  );
	}	
	err = Pa_AbortStream( stream );
    if( (err!=paStreamIsStopped) && (err!=paNoError) ) goto error;
	err = Pa_CloseStream( stream );
//	if( err != paNoError ) goto error;
	Pa_Terminate(); 
	pa_active=0;

#ifdef AU_PORTAUDIO_DLL
#ifndef PORTAUDIO_V19
  free_portaudio_dll();
#endif
#endif

	return;

error:
	Pa_Terminate(); pa_active=0;
#ifdef AU_PORTAUDIO_DLL
#ifndef PORTAUDIO_V19
  free_portaudio_dll();
#endif
#endif
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortAudio error: %s\n", Pa_GetErrorText( err ) );
	return;
}


static int acntl(int request, void *arg)
{
    switch(request)
    {
 
      case PM_REQ_GETQSIZ:
		 *(int *)arg = bytesPerInBuffer*2;
    	return 0;
		//break;
      case PM_REQ_GETFILLABLE:
		 *(int *)arg = bytesPerInBuffer*2-pa_data.samplesToGo;
    	return 0;
		//break;
      case PM_REQ_GETFILLED:
		 *(int *)arg = pa_data.samplesToGo;
    	return 0;
		//break;

    	
   	case PM_REQ_DISCARD:
    case PM_REQ_FLUSH:
    	pa_data.samplesToGo=0;
    	pa_data.bufpoint=pa_data.bufepoint;
    	err = Pa_AbortStream( stream );
    	if( (err!=paStreamIsStopped) && (err!=paNoError) ) goto error;
    	err = Pa_StartStream( stream );
    	if(err!=paNoError) goto error;
		return 0;

		//break;

    case PM_REQ_RATE:  
    	{
    		int i;
    		double sampleRateBack;
    		i = *(int *)arg; //* sample rate in and out *
    		close_output();
    		sampleRateBack=dpm.rate;
    		dpm.rate=i;
    		if(0==open_output()){
    			return 0;
    		}else{    		
    			dpm.rate=sampleRateBack;
    			open_output();
    			return -1;
    		}
    	}
    	//break;

//    case PM_REQ_RATE: 
//          return -1;
    	
    case PM_REQ_PLAY_START: //* Called just before playing *
    case PM_REQ_PLAY_END: //* Called just after playing *
        return 0;
      
	default:
    	return -1;

    }
	return -1;
error:
/*
	Pa_Terminate(); pa_active=0;
#ifdef AU_PORTAUDIO_DLL
  free_portaudio_dll();
#endif
*/
	ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "PortAudio error in acntl : %s\n", Pa_GetErrorText( err ) );
	return -1;
}


static void print_device_list(void){
	PaDeviceIndex maxDeviceIndex, i;
	PaHostApiIndex HostApiIndex;
	const PaDeviceInfo* DeviceInfo;

#if PORTAUDIO_V19
	HostApiIndex=Pa_HostApiTypeIdToHostApiIndex(HostApiTypeId);
#endif
	
	maxDeviceIndex=Pa_GetDeviceCount();
	
	for( i = 0; i < maxDeviceIndex; i++){
		DeviceInfo=Pa_GetDeviceInfo(i);
#if PORTAUDIO_V19
		if( DeviceInfo->hostApi == HostApiIndex){
#endif
			if( DeviceInfo->maxOutputChannels > 0){
				ctl->cmsg(  CMSG_ERROR, VERB_NORMAL, "%2d %s",i,DeviceInfo->name);
			}
#if PORTAUDIO_V19
		}
#endif
	}
}

