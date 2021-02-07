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


    rtsyn_npipe.c
        Copyright (c) 2007 Keishi Suenaga <s_keishi@mutt.freemail.ne.jp>

    I referenced following sources.
        alsaseq_c.c - ALSA sequencer server interface
            Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>
        readmidi.c

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef __POCC__
#include <sys/types.h>
#endif //for off_t

#include <stdio.h>

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <math.h>
#include <signal.h>

#include "server_defs.h"

#ifdef __W32__
#include <windows.h>
#endif

#include "timidity.h"
#include "common.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "recache.h"
#include "output.h"
#include "aq.h"
#include "timer.h"

#include "rtsyn.h"


#define PIPE_BUFFER_SIZE (8192)
static HANDLE hPipe=NULL;




static char pipe_name[256];

#define EVBUFF_SIZE 512
typedef struct rtsyn_evbuf_t{
	UINT wMsg;
	UINT port;
	DWORD	dwParam1;
	DWORD	dwParam2;
	int  exlen;
	char *exbuffer;
}  RtsynEvBuf;
static RtsynEvBuf evbuf[EVBUFF_SIZE];
static UINT  np_evbwpoint=0;
static UINT  np_evbrpoint=0;
static UINT evbsysexpoint;

static CRITICAL_SECTION mim_np_section;

int first_ev = 1;
static double mim_start_time;

static char npipe_buffer[2*PIPE_BUFFER_SIZE];
static int npipe_len=0;



void rtsyn_np_set_pipe_name(char* name)
{
	strncpy(pipe_name, name, 256-1);
	pipe_name[256-1]='\0';
}

static int npipe_input_open(const char *pipe_name)
{
  static OVERLAPPED overlapped;
  static HANDLE hEvent;
  char PipeName[256];
  DWORD ret;
 

 hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
 memset( &overlapped, 0, sizeof(OVERLAPPED));
 overlapped.hEvent = hEvent;
	
  sprintf(PipeName, "\\\\.\\pipe\\%s", pipe_name);
  hPipe = CreateNamedPipe(PipeName, PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED, 
//    PIPE_WAIT|
  	PIPE_READMODE_BYTE |PIPE_TYPE_BYTE, 2, 
   0, PIPE_BUFFER_SIZE, 0, NULL);
  if (hPipe == INVALID_HANDLE_VALUE) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Can't create Named Pipe %s : %ld",
    	pipe_name, GetLastError());
 	return -1;
  }
  ret = ConnectNamedPipe(hPipe, &overlapped);
	if ( (ret == 0)  && (ERROR_IO_PENDING!=GetLastError()) ){
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "CnnectNamePipe(%ld) error %s",
    	GetLastError(), pipe_name);
        CloseHandle(hPipe);
  	    hPipe=NULL;
        return -1;
   }
//	WaitForSingleObject(overlapped.hEvent, 1000);
	CloseHandle(hEvent);
  return 0;
}
	
int rtsyn_np_synth_start()
{
	if( 0 != npipe_input_open(pipe_name)) return 0;
//	npipe_input_open(pipe_name);
	np_evbwpoint=0;
	np_evbrpoint=0;
	InitializeCriticalSection(&mim_np_section);
	first_ev = 1;
	npipe_len=0;
	return ~0;
}

void rtsyn_np_synth_stop()
{
	rtsyn_stop_playing();
	//	play_mode->close_output();
	DeleteCriticalSection(&mim_np_section);
	CloseHandle(hPipe);
	hPipe=NULL;
	return;
}

int rtsyn_np_buf_check(void)
{
	int retval;
	EnterCriticalSection(&mim_np_section);
	retval = (np_evbrpoint != np_evbwpoint) ? 0 :  -1;
	LeaveCriticalSection(&mim_np_section);
	return retval;
}

static int read_pipe_data(void);

int rtsyn_np_play_some_data(void)
{
	UINT wMsg;
	DWORD	dwParam1;
	DWORD	dwParam2;
	MidiEvent ev;
	MidiEvent evm[260];
	UINT evbpoint;
	MIDIHDR *IIMidiHdr;
	int exlen;
	char *sysexbuffer;
	int ne,i,j,chk,played;
	UINT port;
	static DWORD  pre_time;
	static DWORD timeoffset;
	double event_time;
	
//	rtsyn_play_one_data (0,0x007f3c90, get_current_calender_time());
	if ( 0 != read_pipe_data()) return 0;
	
	played=0;
		if( -1 == rtsyn_np_buf_check() ){ 
			played=~0;
			return played;
		}

		do{

			
			EnterCriticalSection(&mim_np_section);
			evbpoint=np_evbrpoint;
			if (++np_evbrpoint >= EVBUFF_SIZE)
					np_evbrpoint -= EVBUFF_SIZE;
			wMsg=evbuf[evbpoint].wMsg;
			port=evbuf[evbpoint].port;
			dwParam1=evbuf[evbpoint].dwParam1;
			dwParam2=evbuf[evbpoint].dwParam2;
			exlen = evbuf[evbpoint].exlen;
			sysexbuffer = evbuf[evbpoint].exbuffer;			
			LeaveCriticalSection(&mim_np_section);
			
			if(rtsyn_sample_time_mode !=1){
				if((first_ev == 1)  || ( pre_time > dwParam2)){
					pre_time=dwParam2;
					timeoffset=dwParam2;
					mim_start_time = get_current_calender_time();
					first_ev=0;
				}
				if(dwParam2 !=0){
			    	 event_time= mim_start_time+((double)(dwParam2-timeoffset))*(double)1.0/(double)1000.0;
				}else{
					event_time = get_current_calender_time();
				}
			}
			
			switch (wMsg) {
			case RTSYN_NP_DATA:
				if(rtsyn_sample_time_mode !=1){
					rtsyn_play_one_data(port, dwParam1, event_time);
				}else{
					rtsyn_play_one_data(port, dwParam1, dwParam2);
				}
				break;
			case RTSYN_NP_LONGDATA:
				if(rtsyn_sample_time_mode !=1){
					rtsyn_play_one_sysex(sysexbuffer,exlen, event_time);
				}else{
					rtsyn_play_one_sysex(sysexbuffer,exlen, dwParam2);
				}
				free(sysexbuffer);
				break;
			}
			pre_time =dwParam2;
			
		}while( 0==rtsyn_np_buf_check());

	return played;
}

static void parse_ev(char* buffer, int *len){
	UINT wMsg;
	UINT port;
	DWORD	dwParam1;
	DWORD	dwParam2;
	int  exlen;
	char *exbuffer;
	
	char *bp, *sp;
	UINT evbpoint;
	RtsynNpEvBuf *npevbuf;

/*
	printf ("buhihi %d \n", *len);
	{
		int i,j;
		
		for(j=0; j < 4; j++){
			for(i=0; i <16;i++){
				printf("%2X:", buffer[j*16+i]);
			}
			printf ("\n");
		}	
	}
*/
	bp=buffer;
	sp=buffer;
	while(1){
		npevbuf=(RtsynNpEvBuf*)sp;
		if(*len >= sizeof(RtsynNpEvBuf)){
			wMsg=npevbuf->wMsg;
		}else{
			memmove(buffer, sp, *len);
			return;
		}
		if( ( wMsg != RTSYN_NP_LONGDATA ) && ( wMsg != RTSYN_NP_DATA ) ){
			*len = 0;
			return;
		}
		if( wMsg == RTSYN_NP_DATA){
			port=npevbuf->port;
			dwParam1=npevbuf->dwParam1;
			dwParam2=npevbuf->dwParam2;
			bp = sp+sizeof(RtsynNpEvBuf);
		}

		if( wMsg == RTSYN_NP_LONGDATA ){
			exlen=npevbuf->exlen;
			bp = sp+sizeof(RtsynNpEvBuf);
			
			dwParam2=npevbuf->dwParam2;
		    if (*len >= sizeof(RtsynNpEvBuf)+exlen){
				exbuffer= (char *)malloc( sizeof(char) * exlen);
				memmove(exbuffer,sp+sizeof(RtsynNpEvBuf), exlen);
				bp += exlen;
		    }else{
		    	memmove(buffer, sp, *len);
				return;
			}

		}
		EnterCriticalSection(&mim_np_section);
		evbpoint = np_evbwpoint;
		if (++np_evbwpoint >= EVBUFF_SIZE)
			np_evbwpoint -= EVBUFF_SIZE;
		evbuf[evbpoint].wMsg = wMsg;
		evbuf[evbpoint].port = port;
		evbuf[evbpoint].dwParam1 = dwParam1;
		evbuf[evbpoint].dwParam2 = dwParam2;
		evbuf[evbpoint].exlen = exlen;
		evbuf[evbpoint].exbuffer = exbuffer;
		LeaveCriticalSection(&mim_np_section);

		*len -= (bp -sp);
		sp=bp;
		if(*len <= 0){
			len = 0;
			return;
		}
	};
}

static int read_pipe_data()
{
	DWORD last_error;
	DWORD n;
	DWORD length;
	
	OVERLAPPED overlapped;
	HANDLE hEvent;
	
	if( hPipe == NULL ) return -1;
	
//	if ( ( 0 == PeekNamedPipe(hPipe,NULL,0,NULL,&length,NULL)) &&
//		 (GetLastError()==ERROR_BAD_PIPE) )  return -1;
	if ( 0 == PeekNamedPipe(hPipe,NULL,0,NULL,&length,NULL))  return -1;
	if(length == 0) return -1;

	 hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
 	memset( &overlapped, 0, sizeof(OVERLAPPED));
 	overlapped.hEvent = hEvent;
	npipe_len=0;   // not good fix
	memset(npipe_buffer+npipe_len, 0, PIPE_BUFFER_SIZE-npipe_len);
	if(length <=  PIPE_BUFFER_SIZE - npipe_len){
		if( length > 0 ){
			ReadFile(hPipe, npipe_buffer+npipe_len,length, &n, &overlapped);
			last_error = GetLastError();
			if(last_error == ERROR_IO_PENDING){
				GetOverlappedResult(hPipe, &overlapped,&n,TRUE) ;
				last_error = GetLastError();
			}
		}
	}else{
	  	ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Named Pipe buffer overlow");
		return -1;
	}
	if(last_error == ERROR_SUCCESS){
		npipe_len += n;
		parse_ev(npipe_buffer,&npipe_len);
	}
	CloseHandle(hEvent);
	if ( (last_error != ERROR_SUCCESS) &&
		(last_error != ERROR_NOACCESS) ){ 
		ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Named Pipe Error: %ld",last_error);
			return -1;
	}
	return 0;
}
	
void rtsyn_np_pipe_close()
{
	CloseHandle(hPipe);
}
