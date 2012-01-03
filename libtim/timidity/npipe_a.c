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

    npipe_.c

    Functions to output raw sound data to a windows Named Pipe.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>

#ifdef __POCC__
#include <sys/types.h>
#endif //for off_t

#ifdef __W32__
#include <windows.h>
#include <stdlib.h>
#include <io.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

#include <fcntl.h>

#ifdef __FreeBSD__
#include <stdio.h>
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"

//#define PIPE_BUFFER_SIZE (AUDIO_BUFFER_SIZE * 8)
#define PIPE_BUFFER_SIZE (88200)
static HANDLE hPipe=NULL;
  OVERLAPPED pipe_ovlpd;
  HANDLE hPipeEvent;
static volatile int pipe_close=-1;
static volatile int clear_pipe=-1;

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 bytes);
static int acntl(int request, void *arg);

/* export the playback mode */

#define dpm npipe_play_mode

PlayMode dpm = {
    DEFAULT_RATE,
	PE_16BIT|PE_SIGNED, PF_PCM_STREAM|PF_CAN_TRACE,
    -1,
    {0,0,0,0,0},
    "Windows Named Pipe", 'N',
    NULL,
    open_output,
    close_output,
    output_data,
    acntl
};

/*************************************************************************/

static int npipe_output_open(const char *pipe_name)
{
  char PipeName[256];  
  DWORD ret;
  DWORD n;
  int i;
	

 if (hPipe != NULL) return 0;
 hPipeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
 memset( &pipe_ovlpd, 0, sizeof(OVERLAPPED));
 pipe_ovlpd.hEvent = hPipeEvent;
	
  sprintf(PipeName, "\\\\.\\pipe\\%s", pipe_name);
  hPipe = CreateNamedPipe(PipeName, PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED, 
//    PIPE_WAIT|
  	PIPE_READMODE_BYTE |PIPE_TYPE_BYTE, 2, 
   PIPE_BUFFER_SIZE, 0, 0, NULL);
  if (hPipe == INVALID_HANDLE_VALUE) {
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "Can't create Named Pipe %s : %ld",
    	pipe_name, GetLastError());
 	return -1;
  }
  ret = ConnectNamedPipe(hPipe, &pipe_ovlpd);
	if ( (ret == 0)  && (ERROR_IO_PENDING!=GetLastError()) ){
    ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "CnnectNamePipe(%ld) error %s",
    	GetLastError(), pipe_name);
        CloseHandle(hPipe);
  	    hPipe=NULL;
        return -1;
   }
//	WaitForSingleObject(pipe_ovlpd.hPipeEvent, 1000);

	return 0;
}

static int open_output(void)
{
  dpm.encoding = validate_encoding(dpm.encoding, 0, 0);

  if(dpm.name == NULL) {
    if (NULL==current_file_info || NULL==current_file_info->filename){
      return -1;
  	}
  }else{
  	      if ( -1 == npipe_output_open(dpm.name))
  			return -1;
  }
  dpm.fd = 1;
//  clear_pipe = 0;
  return 0;
}

static int output_data(char *buf, int32 bytes)
{
	DWORD n;
	int ret;
	int32 retnum;
	retnum = bytes;
	DWORD length;
	char *clear_data;
		
	if (hPipe == NULL) return -1;
	if ( ( 0 == PeekNamedPipe(hPipe,NULL,0,NULL,&length,NULL)) &&
		 (GetLastError()==ERROR_BAD_PIPE) )  return -1;
	if(dpm.fd == -1) return -1;
	if (pipe_close = 0){
		  hPipe=NULL;
		  dpm.fd = -1;
		  return retnum;
	}
	
	if (0 != PeekNamedPipe(hPipe,NULL,0,NULL,&length,NULL)){
		if(clear_pipe == -1){
			if(bytes <= PIPE_BUFFER_SIZE-length){
				ret=WriteFile(hPipe,buf,bytes,&n,&pipe_ovlpd);
				if( (GetLastError() != ERROR_SUCCESS) &&
					(GetLastError() != ERROR_IO_PENDING) &&
					(GetLastError() != ERROR_BAD_PIPE) ){      //why BAD_PIPE occurs here?
	      			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "npipe_a_error %s: %ld",
		    		dpm.name, GetLastError());
					if(hPipe != NULL) CloseHandle(hPipe);
		 			hPipe=NULL;
					dpm.fd = -1;					
					return -1;
				}
			}else{
				ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "npipe_a_errror overflow");
			}
		}else{
			if(length < audio_buffer_size){
				clear_data = (char*)safe_malloc(audio_buffer_size-length);
				memset(clear_data, 0, audio_buffer_size-length);
				WriteFile(hPipe,clear_data,audio_buffer_size-length,&n,&pipe_ovlpd);
				free(clear_data);
				if( (GetLastError() != ERROR_SUCCESS) &&
					(GetLastError() != ERROR_IO_PENDING) &&
					(GetLastError() != ERROR_BAD_PIPE) ){      //why BAD_PIPE occurs here?
	      			ctl->cmsg(CMSG_ERROR, VERB_NORMAL, "npipe_a_error %s: %ld",
		    		dpm.name, GetLastError());
					if(hPipe != NULL) CloseHandle(hPipe);
		 			hPipe=NULL;
					dpm.fd = -1;
					return -1;
				}
				clear_pipe =-1;
			}
		}
	}
	return retnum;
}

static void close_output(void)
{
	if(dpm.fd != -1){
		CloseHandle(hPipeEvent);
		CloseHandle(hPipe);
		hPipe=NULL;
    	dpm.fd = -1;
	}
}

static int acntl(int request, void *arg)
{
  switch(request) {
  case PM_REQ_PLAY_START:
    return 0;
  case PM_REQ_PLAY_END:
    return 0;
  case PM_REQ_DISCARD:
  case PM_REQ_FLUSH:
//		clear_pipe=0;
  	return 0;

  }
  return -1;
}

int npipe_a_pipe_close()
{
	int count;
	if (hPipe == NULL) return;
	pipe_close=0;
	count = 100;
	while( (hPipe != NULL) && (count >0)) Sleep(10);
	if (hPipe != NULL){
		CloseHandle(hPipe);
		hPipe=NULL;
	}
	
}



