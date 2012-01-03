/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

    npsyn_c.c - Windows synthesizer interface
        Copyright (c) 2007 Keishi Suenaga <s_keishi@yahoo.co.jp>

    I referenced following sources.
        alsaseq_c.c - ALSA sequencer server interface
            Copyright (c) 2000  Takashi Iwai <tiwai@suse.de>
        readmidi.c

    DESCRIPTION
    ===========

    This interface provides a Windows MIDI device interface which receives
    events and plays it in real-time.  On this mode, TiMidity works
    purely as software (real-time) MIDI render.

    For invoking Windows synthesizer interface, run timidity as folows:
      % timidity -iW    (interactively select an Input MIDI device)
    or
      % timidity -iW 2  (connect to MIDI device No. 2)

    TiMidity loads instruments dynamically at each time a PRM_CHANGE
    event is received.  It sometimes causes a noise.
    If you are using a low power machine, invoke timidity as follows:
      % timidity -s 11025 -iW       (set sampling freq. to 11025Hz)
    or
      % timidity -EFreverb=0 -iW    (disable MIDI reverb effect control)

    TiMidity keeps all loaded instruments during executing.

    To use TiMidity as output device, you need a MIDI loopback device.
    I use MIDI Yoke. It can freely be obtained MIDI-OX site
    (http://www.midiox.com).
*/

//#define  USE_PORTMIDI 1
//#define USE_GTK_GUI 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef __POCC__
#include <sys/types.h>
#endif

#include "rtsyn.h"
#ifdef USE_GTK_GUI
#include "wsgtk_main.h"
#endif

#ifndef __W32__
#include <stdio.h>
#include <termios.h>
//#include <term.h>
#include <unistd.h>
#endif


#ifndef __W32__
static struct termios initial_settings, new_settings;
static int peek_character = -1;
#endif

extern int volatile stream_max_compute;	// play_event() ÇÃ compute_data() Ç≈åvéZÇãñÇ∑ç≈ëÂéûä‘
static int seq_quit=~0;

static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);
static int ctl_pass_playing_list(int n, char *args[]);

#ifndef __W32__
static void init_keybord(void);
static void close_keybord(void);
static int kbhit(void);
static char readch(void);
#endif

/**********************************/
/* export the interface functions */

#define ctl npsyn_control_mode

ControlMode ctl=
{
    "Windows Named Pipe Synthesizer interface", 'N',
    "npsyn",
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    NULL,
    cmsg,
    ctl_event
};

static int32 event_time_offset;
static FILE *outfp;

/*ARGSUSED*/

static int ctl_open(int using_stdin, int using_stdout)
{
	ctl.opened = 1;
	ctl.flags &= ~(CTLF_LIST_RANDOM|CTLF_LIST_SORT);
	if (using_stdout)
		outfp = stderr;
	else
		outfp = stdout;
	return 0;
}

static void ctl_close(void)
{
  fflush(outfp);
  if(seq_quit==0){
  	rtsyn_np_synth_stop();
  	rtsyn_close();
  	seq_quit=~0;
  }
  ctl.opened=0;
}

static int ctl_read(int32 *valp)
{
    return RC_NONE;
}

#ifdef IA_W32G_SYN
extern void PutsConsoleWnd(char *str);
extern int ConsoleWndFlag;
#endif
static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
#ifndef WINDRV
#ifndef IA_W32G_SYN

	va_list ap;

  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);
  if(type == CMSG_WARNING || type == CMSG_ERROR || type == CMSG_FATAL)
      dumb_error_count++;
  if (!ctl.opened)
    {
      vfprintf(stderr, fmt, ap);
      fputs(NLS, stderr);
    }
  else
    {
      vfprintf(outfp, fmt, ap);
      fputs(NLS, outfp);
      fflush(outfp);
    }
  va_end(ap);

#else
	if ( !ConsoleWndFlag ) return 0;
	{
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    if((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
       ctl.verbosity<verbosity_level) 
	return 0;
//    if(type == CMSG_FATAL)
//	w32g_msg_box(buffer, "TiMidity Error", MB_OK);
    PutsConsoleWnd(buffer);
    PutsConsoleWnd("\n");
    return 0;
	}
#endif
#endif
    return 0;
}

static void ctl_event(CtlEvent *e)
{
}

static void doit(void);

#ifdef IA_W32G_SYN
extern void w32g_syn_doit(void);
extern int w32g_syn_ctl_pass_playing_list(int n_, char *args_[]);


static int ctl_pass_playing_list(int n, char *args[])
{
	return w32g_syn_ctl_pass_playing_list ( n, args );
}
#endif

#ifndef IA_W32G_SYN
static int ctl_pass_playing_list(int n, char *args[])
#else
// 0: OK, 2: Require to reset.
int ctl_pass_playing_list2(int n, char *args[])
#endif
{ 
	if( (n < 1) || (n > 2) ){
		ctl.cmsg(CMSG_WARNING, VERB_NORMAL, "Usage: timidity -iN [Named Pipe Name] SampeTimeMode(1 or 0) \n");
	 	return 1;
	}
	
	rtsyn_np_set_pipe_name(args[0]);
	if( n==1 ){
		rtsyn_sample_time_mode = 0;
	}else{
		rtsyn_sample_time_mode = atoi(args[1]);
	}

#if !defined(IA_W32G_SYN) && !defined(USE_GTK_GUI)
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, 
		"TiMidity starting in Windows Named Pipe Synthesizer mode\n");
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, 
		"Usage: timidity -iN [Named Pipe Name] SampeTimeMode(1 or 0) \n");
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, "\n");
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, 
		"N (Normal mode) M(GM mode) S(GS mode) X(XG mode) \n");
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, 
		"(Only in Normal mode, Mode can be changed by MIDI data)\n");
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, 
		"m(GM reset) s(GS reset) x(XG reset)\n");
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, 
		"\n");
	ctl.cmsg(CMSG_WARNING, VERB_NORMAL, 
		"Press 'q' key to stop\n");

#endif

	rtsyn_init();

#ifdef USE_GTK_GUI
	twgtk_main();
#else 
#ifdef IA_W32G_SYN
	if(0!=rtsyn_np_synth_start()){
		seq_quit=0;
		while(seq_quit==0) {
			w32g_syn_doit();
		}
		rtsyn_np_synth_stop();
	}
#else
	if(0!=rtsyn_np_synth_start()){
		seq_quit=0;
		while(seq_quit==0) {
			doit();
		}
		rtsyn_np_synth_stop();
	}
#endif /* IA_W32G_SYN */
#endif /* USE_GTK_GUI */
	rtsyn_close();

	return 0;
}


#ifndef IA_W32G_SYN


#ifndef __W32__
static void init_keybord(void){
	tcgetattr(0,&initial_settings);
	tcgetattr(0,&new_settings);
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
}

static void close_keybord(void){
	tcsetattr(0, TCSANOW, &initial_settings);
}

static int kbhit(void){
	char ch;
	int nread;
	
	if(peek_character != -1)
		return 1;
	new_settings.c_cc[VMIN]=0;
	tcsetattr(0,TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN]=1;
	tcsetattr(0,TCSANOW, &new_settings);
	
	if(nread == 1) {
		peek_character = ch;
		return 1;
	}
	return 0;
}


static char readch(void){
	char ch;
	if(peek_character != -1){
		ch = peek_character;
		peek_character = -1;
		return ch;
	}
	read(0,&ch,1);
	return ch;
}
#endif		


static void doit(void)
{
#ifndef __W32__
		init_keybord();
#endif

	while(seq_quit==0){
#ifdef __W32__
		if(kbhit()){
			switch(getch()){
#else			
		if(kbhit()){
			switch(readch()){
#endif
				case 'Q':
				case 'q':
					seq_quit=~0;
				break;
				case 'm':
					rtsyn_gm_reset();
				break;
				case 's':
					rtsyn_gs_reset();
				break;
				case 'x':
					rtsyn_xg_reset();
				break;
				case 'c':
					rtsyn_normal_reset();
				break;
				case 'M':
					rtsyn_gm_modeset();
				break;
				case 'S':
					rtsyn_gs_modeset();
				break;
				case 'X':
					rtsyn_xg_modeset();
				break;
				case 'N':
					rtsyn_normal_modeset();
				break;
			}
		}
		rtsyn_np_play_some_data();
		if(rtsyn_sample_time_mode == 0)
			rtsyn_play_calculate();
		if(intr) seq_quit=~0;
		sleep(1);
	}
#ifndef __W32__
	close_keybord();
#endif
}

#endif /* !IA_W32G_SYN */


#ifdef IA_W32G_SYN
static int winplaymidi_sleep_level = 2;
static DWORD winplaymidi_active_start_time = 0;


void winplaymidi(void){

	if ( winplaymidi_sleep_level < 1 ) {
		winplaymidi_sleep_level = 1;
	}
	if( 0 != rtsyn_buf_check() ){
			winplaymidi_sleep_level =0;
	}
	rtsyn_np_play_some_data();
	if ( winplaymidi_sleep_level == 1 ) {
		DWORD ct = GetCurrentTime ();
		if ( winplaymidi_active_start_time == 0 || ct < winplaymidi_active_start_time ) {
			winplaymidi_active_start_time = ct;
		} else if ( ct - winplaymidi_active_start_time > 2000 ) {
			winplaymidi_sleep_level = 2;
		}
	} else if ( winplaymidi_sleep_level == 0 ) {
		winplaymidi_active_start_time = 0;
	}
	
	rtsyn_play_calculate();
	
	if ( winplaymidi_sleep_level >= 2) {
		Sleep ( 100 );
	} else if ( winplaymidi_sleep_level > 0 ) {
		Sleep ( 1 );
	}
}
#endif
		

/*
 * interface_<id>_loader();
 */
ControlMode *interface_N_loader(void)
{
    return &ctl;
}


