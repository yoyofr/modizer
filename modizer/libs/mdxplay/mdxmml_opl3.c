/*
  MDXplayer :  MML parser

  Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.15.1999

  reference : mdxform.doc  ( Takeshi Kouno )
            : MXDRVWIN.pas ( monlight@tkb.att.ne.jp )
 */

/* ------------------------------------------------------------------ */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#if defined(CAN_HANDLE_OPL3) || defined(HAVE_DMFM_SUPPORT)

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "version.h"
#include "mdx.h"
#include "mdxopl3.h"
#include "mdx2151.h"
#include "pcm8.h"

#ifndef timercmp
# define       timercmp(tvp, uvp, cmp)\
               ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
               (tvp)->tv_sec == (uvp)->tv_sec &&\
               (tvp)->tv_usec cmp (uvp)->tv_usec)
#endif

/* ------------------------------------------------------------------ */
/* export symbols */

int mdx_parse_mml_opl3( MDX_DATA *, PDX_DATA * );
int mdx_init_track_work_area_opl3( void );

/* ------------------------------------------------------------------ */
/* local valiables */

static MDX_DATA *mdx = NULL;
static PDX_DATA *pdx = NULL;

static int pcm8_disable = FLAG_TRUE;
static int fade_out = 0;
static int is_sigint_caught = FLAG_FALSE;

static int vol_conv[] = {
   85,  87,  90,  93,  95,  98, 101, 103,
  106, 109, 111, 114, 117, 119, 122, 125
};

/* ------------------------------------------------------------------ */
/* local functions */

static void signal_init( void );
static void priority_init( void );

static int  set_new_event( int );
static void set_tempo( int );
static void note_off( int );
static void note_on( int, int );
static void set_volume( int, int );
static void inc_volume( int );
static void dec_volume( int );
static void set_phase( int, int );
static void set_keyoff( int );
static void set_voice( int, int );
static void set_quantize( int, int );
static void set_detune( int, int, int );
static void set_portament( int, int, int );
static void set_freq( int, int );
static void set_opm_reg( int, int, int );
static void set_fade_out( int );
static void send_sync( int );
static void recv_sync( int );

static void set_plfo_onoff( int, int );
static void set_plfo( int, int, int, int, int, int );
static void set_alfo_onoff( int, int );
static void set_alfo( int, int, int, int, int, int );
static void set_hlfo_onoff( int, int );
static void set_hlfo( int, int, int, int, int, int );
static void set_lfo_delay( int, int );

static void do_quantize( int, int );

/* ------------------------------------------------------------------ */

int mdx_parse_mml_opl3( MDX_DATA *orig_mdx, PDX_DATA *orig_pdx ) {

  int i;
  long count;
  int all_track_finished;
  struct timeval st,et;
  int fade_out_wait;
  int master_volume;
  int infinite_loops;

  mdx = orig_mdx;
  pdx = orig_pdx;

  mdx_init_track_work_area_opl3();

  signal_init();
  if ( mdx->is_output_to_stdout == FLAG_FALSE ) {
    priority_init();
  }
  opl_reg_init(mdx);

  pcm8_disable=FLAG_TRUE;
  if ( mdx->pdx_enable == FLAG_TRUE && mdx->is_use_pcm8 == FLAG_TRUE ) {
    if ( pcm8_open(mdx)==0 ) {
      pcm8_disable=FLAG_FALSE;
      if ( mdx->is_output_to_stdout == FLAG_FALSE ) {
	pcm8_start();
      }
    }
  }

  /* start parsing */

  all_track_finished=FLAG_FALSE;
  fade_out_wait=0;
  master_volume=127;
  gettimeofday(&st, NULL);
  srand((int)st.tv_usec%65536);

  while(all_track_finished==FLAG_FALSE) {

    if ( fade_out > 0 ) {
      if ( fade_out_wait==0 ) { fade_out_wait = fade_out; }
      fade_out_wait--;
      if ( fade_out_wait==0 ) { master_volume--; }
      if ( master_volume==0 ) { break; }
    }
    opl3_set_master_volume( master_volume * mdx->fm_volume / 127 );
    pcm8_set_master_volume( master_volume * mdx->pcm_volume / 127 );

    all_track_finished=FLAG_TRUE;
    infinite_loops = 32767; /* large enough */
    for ( i=0 ; i<mdx->tracks ; i++ ) {

      if ( mdx->track[i].waiting_sync == FLAG_TRUE )
	{ continue; }

      count = mdx->track[i].counter;
      if ( count < 0 ) { continue; } /* this track has finished */
      all_track_finished=FLAG_FALSE;

      mdx->track[i].gate--;
      if ( mdx->track[i].gate == 0 ) { note_off( i ); }

      if ( i<8 ) {
	opl3_set_freq_volume( i ); /* do portament, lfo, detune */
      }

      count--;
      while ( count == 0 ) {
	count=set_new_event( i );
      }

      mdx->track[i].counter = count;

      if ( infinite_loops > mdx->track[i].infinite_loop_times ) {
	infinite_loops = mdx->track[i].infinite_loop_times;
      }
    }

    if ( mdx->max_infinite_loops > 0 ) {
      if ( infinite_loops >= mdx->max_infinite_loops ) {
	fade_out = mdx->fade_out_speed;
      }
    }

    mdx->total_count++;
    mdx->elapsed_time += 1000*1024*(256-mdx->tempo)/4000;

    st.tv_usec += 1000* 1024*(256-mdx->tempo)/4000;
    while ( st.tv_usec >= 1000*1000 ) {
      st.tv_usec-=1000*1000;
      st.tv_sec++;
    }

    gettimeofday(&et, NULL);
    while (timercmp(&st,&et,>)) {
      struct timespec t;
      t.tv_sec=0;
      t.tv_nsec=1000*1000*MDX_WAIT_INTERVAL;
      nanosleep(&t, NULL);
      gettimeofday(&et, NULL);
    }
    if ( (et.tv_sec - st.tv_sec) > MDX_MAX_BROOK_TIME_OF_CATCH ) {
      st.tv_sec  = et.tv_sec;
      st.tv_usec = et.tv_usec;
    }
  }

  opl3_all_note_off();
  pcm8_close();

  return 0;
}

/* ------------------------------------------------------------------ */

static const int signals[]={SIGHUP,SIGQUIT,SIGILL,SIGABRT,SIGFPE,
                              SIGBUS,SIGSEGV,SIGPIPE,SIGTERM,0};

static RETSIGTYPE signal_vector( int num ) {
  opl3_all_note_off();

  /*pcm8_close();*/
  fprintf(stderr,"Signal caught : %d\n", num);
  exit(1);
}

static void signal_play_stop( int num ) {
  int i;

  signal(SIGINT, SIG_DFL);

  if ( is_sigint_caught == FLAG_TRUE ) return;
  for ( i=0 ; i<mdx->tracks ; i++ ) {
    mdx->track[i].counter = -1;
  }
  mdx->is_normal_exit = FLAG_FALSE;
  is_sigint_caught = FLAG_TRUE;

  return;
}

static void signal_init( void ) {
  int i;
  for ( i=0 ; signals[i]!=0 ; i++ )
    signal(signals[i],signal_vector);

  signal(SIGINT,signal_play_stop);
  is_sigint_caught = FLAG_FALSE;
  mdx->is_normal_exit = FLAG_TRUE;

  return;
}

static void priority_init( void ) {
  return;
}

/* ------------------------------------------------------------------ */

int mdx_init_track_work_area_opl3( void ) {

  int i;

  fade_out = 0;

  mdx->tempo        = 200;
  mdx->total_count  = 0;

  for ( i=0 ; i<mdx->tracks ; i++ ) {
    mdx->track[i].counter = 1;
    mdx->track[i].gate = 1;
    mdx->track[i].current_mml_ptr = mdx->mml_data_offset[i];

    mdx->track[i].voice         = 0;
    mdx->track[i].volume        = 64;
    mdx->track[i].volume_normal = 8;
    mdx->track[i].note          = 0;
    mdx->track[i].phase         = MDX_PAN_C;
    mdx->track[i].quantize1     = 8;
    mdx->track[i].quantize2     = 0;
    mdx->track[i].detune        = 0;
    if ( i<8 )
      opl3_set_detune(i,0);
    mdx->track[i].portament     = 0;
    if ( i<8 )
      opl3_set_portament(i,0);

    mdx->track[i].loop_depth = 0;
    mdx->track[i].infinite_loop_times = 0;

    mdx->track[i].p_lfo_flag = FLAG_FALSE;
    mdx->track[i].a_lfo_flag = FLAG_FALSE;
    mdx->track[i].h_lfo_flag = FLAG_FALSE;

    mdx->track[i].p_lfo_form = 0;
    mdx->track[i].p_lfo_clock = 255;
    mdx->track[i].p_lfo_depth = 0;

    mdx->track[i].a_lfo_form = 0;
    mdx->track[i].a_lfo_clock = 255;
    mdx->track[i].a_lfo_depth = 0;

    mdx->track[i].waiting_sync = FLAG_FALSE;

    mdx->track[i].keyoff_disable     = FLAG_FALSE;
    mdx->track[i].last_volume_normal = FLAG_FALSE;
  }

  return 0;
}

static int set_new_event( int t ) {

  int ptr;
  unsigned char *data;
  int count;
  int follower;

  data = mdx->data;
  ptr = mdx->track[t].current_mml_ptr;
  count = 0;
  follower = 0;

  if ( data[ptr] <= MDX_MAX_REST ) {  /* rest */
    note_off(t);
    count = data[ptr]+1;
    mdx->track[t].gate = count+1;
    follower=0;

  } else if ( data[ptr] <= MDX_MAX_NOTE ) { /* note */
    note_on( t, data[ptr]);
    count = data[ptr+1]+1;
    do_quantize( t, count );
    follower = 1;

  } else {
    switch ( data[ptr] ) {

    case MDX_SET_TEMPO:
      set_tempo( data[ptr+1] );
      follower = 1;
      break;

    case MDX_SET_OPM_REG:
      set_opm_reg( t, data[ptr+1], data[ptr+2] );
      follower = 2;
      break;

    case MDX_SET_VOICE:
      set_voice( t, data[ptr+1] );
      follower = 1;
      break;

    case MDX_SET_PHASE:
      set_phase( t, data[ptr+1] );
      follower = 1;
      break;

    case MDX_SET_VOLUME:
      set_volume( t, data[ptr+1] );
      follower = 1;
      break;

    case MDX_VOLUME_DEC:
      dec_volume( t );
      follower = 0;
      break;

    case MDX_VOLUME_INC:
      inc_volume( t );
      follower = 0;
      break;

    case MDX_SET_QUANTIZE:
      set_quantize( t, data[ptr+1] );
      follower = 1;
      break;

    case MDX_SET_KEYOFF:
      set_keyoff( t );
      follower = 0;
      break;

    case MDX_REPEAT_START:
      mdx->track[t].loop_counter[mdx->track[t].loop_depth] = data[ptr+1];
      if ( mdx->track[t].loop_depth < MDX_MAX_LOOP_DEPTH )
	mdx->track[t].loop_depth++;
      follower = 2;
      break;

    case MDX_REPEAT_END:
      if (--mdx->track[t].loop_counter[mdx->track[t].loop_depth-1] == 0 ) {
	if ( --mdx->track[t].loop_depth < 0 ) mdx->track[t].loop_depth=0;
      } else {
	if ( data[ptr+1] >= 0x80 ) {
	  ptr = ptr+2 - (0x10000-(data[ptr+1]*256 + data[ptr+2])) - 2;
	} else {
	  ptr = ptr+2 + data[ptr+1]*256 + data[ptr+2] - 2;
	}
      }
      follower = 2;
      break;

    case MDX_REPEAT_BREAK:
      if ( mdx->track[t].loop_counter[mdx->track[t].loop_depth-1] == 1 ) {
	if ( --mdx->track[t].loop_depth < 0 ) mdx->track[t].loop_depth=0;
	ptr = ptr+2 + data[ptr+1]*256 + data[ptr+2] -2 +2;
      }
      follower = 2;
      break;

    case MDX_SET_DETUNE:
      set_detune( t, data[ptr+1], data[ptr+2] );
      follower = 2;
      break;

    case MDX_SET_PORTAMENT:
      set_portament( t, data[ptr+1], data[ptr+2] );
      follower = 2;
      break;

    case MDX_DATA_END:
      if ( data[ptr+1] == 0x00 ) {
	count = -1;
	note_off(t);
	follower = 1;
      } else {
	ptr = ptr+2 - (0x10000-(data[ptr+1]*256 + data[ptr+2])) - 2;
	mdx->track[t].infinite_loop_times++;
	follower = 2;
      }
      break;
 
    case MDX_KEY_ON_DELAY:
      follower = 1;
      break;

    case MDX_SEND_SYNC:
      send_sync( data[ptr+1] );
      follower = 1;
      break;

    case MDX_RECV_SYNC:
      recv_sync( t );
      follower = 0;
      count = 1;
      break;

    case MDX_SET_FREQ:
      set_freq( t, data[ptr+1] );
      follower = 1;
      break;

    case MDX_SET_PLFO:
      if ( data[ptr+1] == 0x80 || data[ptr+1] == 0x81 ) {
	set_plfo_onoff( t, data[ptr+1]-0x80 );
	follower = 1;
      } else {
	set_plfo( t,
		  data[ptr+1], data[ptr+2], data[ptr+3],
		  data[ptr+4], data[ptr+5] );
	follower = 5;
      }
      break;

    case MDX_SET_ALFO:
      if ( data[ptr+1] == 0x80 || data[ptr+1] == 0x81 ) {
	set_alfo_onoff( t, data[ptr+1]-0x80 );
	follower = 1;
      } else {
	set_alfo( t,
		  data[ptr+1], data[ptr+2], data[ptr+3],
		  data[ptr+4], data[ptr+5] );
	follower = 5;
      }
      break;

    case MDX_SET_OPMLFO:
      if ( data[ptr+1] == 0x80 || data[ptr+1] == 0x81 ) {
	set_hlfo_onoff( t, data[ptr+1]-0x80 );
	follower = 1;
      } else {
	set_hlfo( t,
		  data[ptr+1], data[ptr+2], data[ptr+3],
		  data[ptr+4], data[ptr+5] );
	follower = 5;
      }
      break;

    case MDX_SET_LFO_DELAY:
      set_lfo_delay( t, data[ptr+1] );
      follower = 1;
      break;

    case MDX_SET_PCM8_MODE:
      follower = 0;
      break;

    case MDX_FADE_OUT:
      if ( data[ptr+1]==0x00 ) {
	follower = 1;
	set_fade_out( 5 );
      } else {
	follower = 2;
	set_fade_out( data[ptr+2] );
      }
      break;

    default:
      count = -1;
      break;
    }
  }

  ptr += 1+follower;
  mdx->track[t].current_mml_ptr = ptr;

  return count;
}


static void set_tempo( int val ) {

  if (val<2) return; /* workaround!!! */
  mdx->tempo = val;
  return;
}

static void note_off( int track ) {

  if ( mdx->track[track].keyoff_disable == FLAG_FALSE ) {
    mdx->track[track].note=-1;
    if ( track<8 )
      opl3_note_off(track);
    else
      pcm8_note_off(track-8);
  }
  mdx->track[track].keyoff_disable = FLAG_FALSE;

  return;
}

static void note_on( int track, int note ) {

  int last_note;

  last_note = mdx->track[track].note;
  note -= MDX_MIN_NOTE;
  mdx->track[track].note = note;

  if ( mdx->track[track].phase != MDX_PAN_N ) {
    if ( track<8 ){
      opl3_note_on(track, mdx->track[track].note);
    }
    else {
      if ( note<MDX_MAX_PDX_TONE_NUMBER && pcm8_disable == FLAG_FALSE && pdx != NULL ) {
	int n = note+mdx->track[track].voice*MDX_MAX_PDX_TONE_NUMBER;
	pcm8_note_on( track-8,
		      pdx->tone[n].data,
		      pdx->tone[n].size,
		      pdx->tone[n].orig_data,
		      pdx->tone[n].orig_size );
      }
    }
  }

  return;
}

static void do_quantize( int t, int count ) {

  int gate;

  if ( mdx->track[t].keyoff_disable == FLAG_TRUE ) {
    gate = count+2;
    mdx->track[t].keyoff_disable = FLAG_FALSE;

  } else {
    if ( mdx->track[t].quantize2>0 ) {
      gate = count - mdx->track[t].quantize2;
      if ( gate <= 0 ) gate=1;

    } else {
      gate = count * mdx->track[t].quantize1/8;
    }
  }

  /* FIXME : we must tune up the note-off job */
  if ( gate+1 < count )
    gate++;

  mdx->track[t].gate = gate;

  return;
}

static void set_phase( int track, int val ) {

  mdx->track[track].phase = val;

  if ( track<8 )
    opl3_set_pan(track, val);
  else
    pcm8_set_pan(val);

  if ( val == MDX_PAN_N ) {
    if ( track<8 )
      opl3_note_off(track);
    else
      pcm8_note_off(track-8);
  }

  return;
}

static void set_volume( int track, int val ) {

  int v;
  int m;

  if ( val < 0x10 ) {
    mdx->track[track].volume_normal = val;
    v = vol_conv[val];
    m = FLAG_TRUE;
  }
  else if ( val >= 0x80 ) {
    v = 255-val;
    m = FLAG_FALSE;
  }
  else {
    v=0;
    m=FLAG_FALSE;
  }
  mdx->track[track].volume = v;
  mdx->track[track].last_volume_normal = m;

  if ( track<8 )
    opl3_set_volume( track, v );
  else
    pcm8_set_volume( track-8, v );

  return;
}

static void inc_volume( int track ) {

  if ( mdx->track[track].last_volume_normal == FLAG_TRUE ) {
    if ( ++mdx->track[track].volume_normal > 0x0f ) {
      mdx->track[track].volume_normal = 0x0f;
    }
    mdx->track[track].volume = vol_conv[mdx->track[track].volume_normal];
  }
  else {
    if ( ++mdx->track[track].volume > 0x7f ) {
      mdx->track[track].volume = 0x7f;
    }
  }

  if ( track<8 )
    opl3_set_volume( track, mdx->track[track].volume );
  else
    pcm8_set_volume( track-8, mdx->track[track].volume );

  return;
}

static void dec_volume( int track ) {

  if ( mdx->track[track].last_volume_normal == FLAG_TRUE) {
    if ( --mdx->track[track].volume_normal < 0 ) {
      mdx->track[track].volume_normal = 0;
    }
    mdx->track[track].volume = vol_conv[mdx->track[track].volume_normal];
  }
  else {
    if ( --mdx->track[track].volume < 0 ) {
      mdx->track[track].volume = 0;
    }
  }

  if ( track<8 )
    opl3_set_volume( track, mdx->track[track].volume );
  else
    pcm8_set_volume( track-8, mdx->track[track].volume );

  return;
}

static void set_keyoff( int track ) {

  mdx->track[track].keyoff_disable = FLAG_TRUE;

  return;
}

static void set_voice( int track, int val ) {

  if ( track<8 && val >= MDX_MAX_VOICE_NUMBER ) return;
  if ( track>7 && val >= MDX_MAX_PDX_BANK_NUMBER ) return;
  mdx->track[track].voice = val;
  if ( track<8 ) {
    opl3_set_voice( track, &mdx->voice[val] );
  }

  return;
}

static void set_quantize( int track, int val ) {

  if ( val < 9 ) {
    mdx->track[track].quantize1 = val;
    mdx->track[track].quantize2 = 0;
  }
  else {
    mdx->track[track].quantize1 = 8;
    mdx->track[track].quantize2 = 0x100-val;
  }

  return;
}

static void set_detune( int track, int v1, int v2 ) {
  int v;

  if ( track<8 ) {
    v = v1*256 + v2;
    if ( v1 >= 0x80 ) v = v-0x10000;
    mdx->track[track].detune = v;
    opl3_set_detune( track, v );
  }

  return;
}

static void set_portament( int track, int v1, int v2 ) {
  int v;

  if ( track<8 ) {
    v = v1*256 + v2;
    if ( v1 >= 0x80 ) v = v - 0x10000;
    mdx->track[track].portament = v;
    opl3_set_portament( track, v );
  }

  return;
}

static void set_freq( int track, int val ) {

  if ( track >= 8 ) {
    pcm8_set_pcm_freq( track-8, val );
  }
  else {
    opl3_set_noise_freq( val );
  }
  return;
}

static void set_fade_out( int speed ) {
  fade_out = speed+1;
  return;
}

static void set_opm_reg( int track, int v1, int v2 ) {

  if ( track<8 ) {
    if ( v1>=0x28 && v1<0x30 ) {
      opl3_set_freq_reg( v1, v2 );
    } else if ( v1>=0x30 && v1<0x38 ) {
      opl3_set_freq_reg( v1, v2 );
    } else if ( v1==0x12 ) {
      mdx->tempo = v2;
    }
  }

  return;
}

static void send_sync( int track ) {
  mdx->track[track].waiting_sync = FLAG_FALSE;
  return;
}

static void recv_sync( int track ) {
  mdx->track[track].waiting_sync = FLAG_TRUE;
  return;
}

static void set_plfo_onoff( int track, int onoff ) {
  if ( onoff == 0 ) {
    mdx->track[track].p_lfo_flag = FLAG_FALSE;
  }
  else {
    mdx->track[track].p_lfo_flag = FLAG_TRUE;
  }

  if ( track < 8 ) {
    opl3_set_plfo( track,
		   mdx->track[track].p_lfo_flag,
		   mdx->track[track].p_lfo_form,
		   mdx->track[track].p_lfo_clock,
		   mdx->track[track].p_lfo_depth );
  }

  return;
}

static void set_plfo( int track, 
		      int v1, int v2, int v3, int v4, int v5 ) {
  int t,d;

  t = v2*256+v3;
  d = v4*256+v5;
  if ( d>=0x8000 ) {
    d = d-0x10000;
  }
  if ( v1 > 4 ) d*=256;
  mdx->track[track].p_lfo_form  = v1;
  mdx->track[track].p_lfo_clock = t;
  mdx->track[track].p_lfo_depth = d;
  mdx->track[track].p_lfo_flag  = FLAG_TRUE;

  if ( track < 8 ) {
    opl3_set_plfo( track,
		   mdx->track[track].p_lfo_flag,
		   mdx->track[track].p_lfo_form,
		   mdx->track[track].p_lfo_clock,
		   mdx->track[track].p_lfo_depth );
  }
  return;
}

static void set_alfo_onoff( int track, int onoff ) {
  if ( onoff == 0 ) {
    mdx->track[track].a_lfo_flag = FLAG_FALSE;
  }
  else {
    mdx->track[track].a_lfo_flag = FLAG_TRUE;
  }

  if ( track < 8 ) {
    opl3_set_alfo( track,
		   mdx->track[track].a_lfo_flag,
		   mdx->track[track].a_lfo_form,
		   mdx->track[track].a_lfo_clock,
		   mdx->track[track].a_lfo_depth );
  }
  return;
}

static void set_alfo( int track, 
		      int v1, int v2, int v3, int v4, int v5 ) {
  int t,d;

  t = v2*256+v3;
  d = v4*256+v5;
  if ( v4>=0x80 ) {
    d = d-0x10000;
  }
  mdx->track[track].a_lfo_form  = v1;
  mdx->track[track].a_lfo_clock = t;
  mdx->track[track].a_lfo_depth = d;
  mdx->track[track].a_lfo_flag  = FLAG_TRUE;

  if ( track < 8 ) {
    opl3_set_alfo( track,
		   mdx->track[track].a_lfo_flag,
		   mdx->track[track].a_lfo_form,
		   mdx->track[track].a_lfo_clock,
		   mdx->track[track].a_lfo_depth );
  }
  return;
}

static void set_hlfo_onoff( int track, int onoff ) {
  if ( onoff == 0 ) {
    mdx->track[track].h_lfo_flag = FLAG_FALSE;
  }
  else {
    mdx->track[track].h_lfo_flag = FLAG_TRUE;
  }
  return;
}

static void set_hlfo( int track, 
		      int v1, int v2, int v3, int v4, int v5 ) {
  int wa, sp, pd, ad, ps, as, sy;

  wa = v1&0x03;
  sp = v2;
  pd = v3 & 0x7f;
  ad = v4;
  ps = (v5/16) & 0x07;
  as = v5 & 0x03;
  sy = (v1/64) & 0x01;

  return;
}

static void set_lfo_delay( int track, int delay ) {
  mdx->track[track].lfo_delay = delay;

  if ( track < 8 ) {
    opl3_set_lfo_delay( track, delay );
  }
  return;
}

#else /* CAN_HANDLE_OPL3 */

#include "mdx.h"

int mdx_parse_mml_opl3( MDX_DATA *mdx, PDX_DATA *pdx ) {
  return -1;
}

int mdx_init_track_work_area_opl3( void ) {
  return -1;
}

#endif /* CAN_HANDLE_OPL3 */
