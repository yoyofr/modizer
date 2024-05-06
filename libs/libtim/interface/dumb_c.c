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

    dumb_c.c
    Minimal control mode -- no interaction, just prints out messages.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#ifdef __W32__
#include "wrd.h"
#endif /* __W32__ */

#include <math.h> //YOYOFR

static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_total_time(long tt);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct);
static void ctl_lyric(int lyricid);
static void ctl_event(CtlEvent *e);

int tim_midilength,tim_pending_seek,tim_current_voices,tim_lyrics_started;
extern int mod_message_updated;


/**********************************/
/* export the interface functions */

#define ctl dumb_control_mode

ControlMode ctl=
{
    "dumb interface", 'd',
    "dumb",
    VERB_VERBOSE,0,0,
    0,
    ctl_open,
    ctl_close,
    dumb_pass_playing_list,
    ctl_read,
    NULL,
    cmsg,
    ctl_event
};

static FILE *outfp;
int dumb_error_count;

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
  if(using_stdout)
    outfp=stderr;
  else
    outfp=stdout;
  ctl.opened=1;
  return 0;
}

static void ctl_close(void)
{
  fflush(outfp);
  ctl.opened=0;
}

/*ARGSUSED*/
static int ctl_read(int32 *valp)
{
    if (tim_pending_seek!=-1){
        *valp=tim_pending_seek*play_mode->rate;
        tim_pending_seek=-1;
        return RC_JUMP;
    }
    //YOYOFR
    if (roundf(tim_tempo_ratio*100)!=roundf(midi_time_ratio*100)) {
        float *valpf=(float*)valp;
        *valpf=tim_tempo_ratio;
        tim_tempo_ratio=midi_time_ratio; //realign to avoid issues, will be updated in playmidi based on valpf
        return RC_SPEEDCHANGE;
    }
  return RC_NONE;
}

extern char *mod_message;
char tim_tmp_message[1024];

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
  va_list ap;

  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);
  if (type==CMSG_FILE) return 0;
    if (type==CMSG_INFO) return 0;
  if(type == CMSG_WARNING || type == CMSG_ERROR || type == CMSG_FATAL)
      dumb_error_count++;
  /*if (!ctl.opened)
    {
      vfprintf(stderr, fmt, ap);
      fputs(NLS, stderr);
    }
  else
    {
      vfprintf(outfp, fmt, ap);
      fputs(NLS, outfp);
      fflush(outfp);
    }*/
    
    vsprintf(tim_tmp_message,fmt,ap);
  va_end(ap);
//    printf("%d %s\n",strlen(tim_tmp_message),tim_tmp_message);
    if (type!=CMSG_LYRICS) strcat(mod_message,"\n");
    strcat(mod_message,tim_tmp_message);
  mod_message_updated=3;

  return 0;
}


static void ctl_total_time(long tt)
{
  int mins, secs;
  tim_midilength=(int)((float)tt*1000/(float)(play_mode->rate));
  if (ctl.trace_playing)
    {
      secs=(int)(tt/play_mode->rate);
      mins=secs/60;
      secs-=mins*60;
      cmsg(CMSG_INFO, VERB_NORMAL,
	   "Total playing time: %3d min %02d s", mins, secs);
    }
}

static void ctl_file_name(char *name)
{
/*  if (ctl.verbosity>=0 || ctl.trace_playing)
      cmsg(CMSG_INFO, VERB_NORMAL, "Playing %s", name);*/
}

static void ctl_current_time(int secs)
{
  int mins;
  static int prev_secs = -1;

#ifdef __W32__
	  if(wrdt->id == 'w')
	    return;
#endif /* __W32__ */
  if (ctl.trace_playing && secs != prev_secs)
    {
      prev_secs = secs;
      mins=secs/60;
      secs-=mins*60;
      fprintf(outfp, "\r%3d:%02d", mins, secs);
      fflush(outfp);
    }
}

static void ctl_lyric(int lyricid)
{
    char *lyric;

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
        if (!tim_lyrics_started) {
            tim_lyrics_started=1;
            cmsg(CMSG_LYRICS, VERB_NORMAL, "\n[LYRICS]\n--------\n");
        }
        
	if(lyric[0] == ME_KARAOKE_LYRIC)
	{
	    if(lyric[1] == '/' || lyric[1] == '\\')
	    {
		//fprintf(outfp, "\n%s", lyric + 2);
		//fflush(outfp);
            cmsg(CMSG_LYRICS, VERB_NORMAL, "\n%s", lyric+2);
	    }
	    else if(lyric[1] == '@')
	    {
		if(lyric[2] == 'L')
		    //fprintf(outfp, "\nLanguage: %s\n", lyric + 3);
            cmsg(CMSG_LYRICS, VERB_NORMAL, "\nLanguage: %s\n", lyric+3);
		else if(lyric[2] == 'T')
		    //fprintf(outfp, "Title: %s\n", lyric + 3);
            cmsg(CMSG_LYRICS, VERB_NORMAL, "Title: %s\n", lyric+3);
		else
		    //fprintf(outfp, "%s\n", lyric + 1);
            cmsg(CMSG_LYRICS, VERB_NORMAL, "%s\n", lyric+1);
	    }
	    else
	    {
		//fputs(lyric + 1, outfp);
		//fflush(outfp);
            cmsg(CMSG_LYRICS, VERB_NORMAL, "%s", lyric+1);
	    }
	}
	else
	{
	    if(lyric[0] == ME_CHORUS_TEXT || lyric[0] == ME_INSERT_TEXT)
		//fprintf(outfp, "\r");
            cmsg(CMSG_LYRICS, VERB_NORMAL, "\r");
	    //fputs(lyric + 1, outfp);
        char *c=lyric+1;
        while (*c) {
            c++;
        }
        c--;
        if ((c>lyric)&&(*c=='.')) cmsg(CMSG_LYRICS, VERB_NORMAL, "%s\n", lyric+1);
        else cmsg(CMSG_LYRICS, VERB_NORMAL, "%s", lyric+1);
//	    fflush(outfp);
	}
    }
}



static void ctl_note(int status, int ch, int note, int vel,int voiceID) {    
    switch(status)
    {
        case VOICE_DIE:
            
            break;
        case VOICE_FREE:
            
            break;
        case VOICE_ON:
            
            break;
        case VOICE_SUSTAINED:
            
            break;
        case VOICE_OFF:
            
            break;      
    }
    
}

static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
      case CTLE_NOW_LOADING:
	ctl_file_name((char *)e->v1);
	break;
      case CTLE_PLAY_START:
	ctl_total_time(e->v1);
	break;
      case CTLE_CURRENT_TIME:
	ctl_current_time((int)e->v1);
    //printf("voices: %d\n",(int)e->v2);
            tim_current_voices=e->v2;
	break;
      #ifndef CFG_FOR_SF
      case CTLE_LYRIC:
	ctl_lyric((int)e->v1);
	break;
      #endif
        case CTLE_NOTE:
            ctl_note((int)e->v1, (int)e->v2, (int)e->v3, (int)e->v4, (int)e->v5);
            break;
    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_d_loader(void)
{
    return &ctl;
}
