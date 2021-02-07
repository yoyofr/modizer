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


    rtsyn.h
        Copyright (c) 2003  Keishi Suenaga <s_keishi@mutt.freemail.ne.jp>

    I referenced following sources.
        alsaseq_c.c - ALSA sequencer server interface
            Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>
        readmidi.c
*/
#include "interface.h"

#include <stdio.h>

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
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
#include <mmsystem.h>
#endif


#ifndef __W32__
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
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


#define  USE_PORTMIDI 1

/******************************************************************************/
/*                                                                            */
/*  Interface independent functions (see rtsyn_common.c)                      */
/*                                                                            */
/******************************************************************************/

/* peek playmidi.c */
extern int32 current_sample;

/* peek timidity.c */
extern VOLATILE int intr;

/* How often data pass to the buffer */
#define TICKTIME_HZ 200

/* latency (sec)  > 1.0 / TICKTIME_HZ * 4.0 */
#define RTSYN_LATENCY 0.20


extern double rtsyn_latency;   /* = RTYSN_LATENCY */
extern int rtsyn_system_mode;

extern int32 rtsyn_start_sample;
extern int rtsyn_sample_time_mode; //bool 1 ture 0 false

/* reset synth    */
void rtsyn_gm_reset(void);
void rtsyn_gs_reset(void);
void rtsyn_xg_reset(void);
void rtsyn_normal_reset(void);

/* mode change                                            *
 * only in nomalmode program can accept reset(sysex) data */
void rtsyn_gm_modeset(void);
void rtsyn_gs_modeset(void);
void rtsyn_xg_modeset(void);
void rtsyn_normal_modeset(void);

void rtsyn_init(void);
void rtsyn_close(void);
double rtsyn_set_latency(double latency);
void rtsyn_play_event(MidiEvent *ev);
void rtsyn_play_event_time(MidiEvent *ev, double event_time);
void rtsyn_tmr_reset(void);
void rtsyn_server_reset(void);
void rtsyn_reset(void);
void rtsyn_stop_playing(void);
int rtsyn_play_one_data (int port, int32 dwParam1, double event_time);
void rtsyn_play_one_sysex (char *sysexbuffer, int exlen, double event_time );
void rtsyn_play_calculate(void);



/******************************************************************************/
/*                                                                            */
/*  Interface dependent functions (see rtsyn_winmm.c rtsyn_portmidi.c)        */
/*                                                                            */
/******************************************************************************/
#if defined(IA_WINSYN) || defined(IA_PORTMIDISYN) || defined(IA_W32G_SYN) 

#define MAX_PORT 4
extern int rtsyn_portnumber;
extern unsigned int portID[MAX_PORT];
extern char  rtsyn_portlist[32][80];
extern int rtsyn_nportlist;

void rtsyn_get_port_list(void);
int rtsyn_synth_start(void);
void rtsyn_synth_stop(void);
int rtsyn_play_some_data (void);
void rtsyn_midiports_close(void);

#if defined(IA_WINSYN) || defined(IA_W32G_SYN)
int rtsyn_buf_check(void);
#endif

#endif /* IA_WINSYN IA_PORTMIDISYN IA_W32G_SYN */

#ifdef IA_NPSYN
#define RTSYN_NP_DATA 1
#define RTSYN_NP_LONGDATA 2

typedef struct rtsyn_np_evbuf_t{
	uint32 wMsg;
	union {
		uint32 port;
		uint32  exlen;
	};
	union {
		uint32	dwParam1;
	};
	uint32	dwParam2;
}  RtsynNpEvBuf;

void rtsyn_np_set_pipe_name(char*);
int rtsyn_np_synth_start(void);
void rtsyn_np_synth_stop(void);
int rtsyn_np_play_some_data (void);
void rtsyn_np_pipe_close();
#endif

#ifdef USE_WINSYN_TIMER_I

#if defined(__W32__)
typedef CRITICAL_SECTION  rtsyn_mutex_t;
#define rtsyn_mutex_init(_m)	InitializeCriticalSection(&_m)
#define rtsyn_mutex_destroy(_m) DeleteCriticalSection(&_m)
#define rtsyn_mutex_lock(_m)    EnterCriticalSection(&_m)
#define rtsyn_mutex_unlock(_m)  LeaveCriticalSection(&_m)

#else
typedef pthread_mutex_t rtsyn_mutex_t;
#define rtsyn_mutex_init(_m)      pthread_mutex_init(&(_m), NULL)
#define rtsyn_mutex_destroy(_m)   pthread_mutex_destroy(&(_m))
#define rtsyn_mutex_lock(_m)      pthread_mutex_lock(&(_m))
#define rtsyn_mutex_unlock(_m)    pthread_mutex_unlock(&(_m))
#endif

#endif
