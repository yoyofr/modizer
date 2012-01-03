/*
  MDXplayer :  OPL3 access routines

  Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.16.1999
 */

/* ------------------------------------------------------------------ */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#if defined(CAN_HANDLE_OPL3) || defined(HAVE_DMFM_SUPPORT)

#include <stdio.h>

#include "version.h"
#include "mdx.h"
#include "mdxopl3.h"
#include "ioaccess.h"

/* ------------------------------------------------------------------ */
/* local valiables */

typedef struct _OPL3_LFO {

  int flag;

  int form;
  int clock;
  int depth;

} OPL3_LFO;

typedef struct _DETUNE_VAL {

  int octave;
  int scale;
  int detune;

} DETUNE_VAL;

typedef struct _OPL3_WORK {

  int        lfo_delay;
  OPL3_LFO   plfo;
  OPL3_LFO   alfo;

  long       portament;
  DETUNE_VAL detune;
  int        note;
  int        note_on;
  int        volume;
  int        pan;
  int        total_level[4];
  int        step;
  int        freq_reg[2];
  int        algorithm;

} OPL3_WORK;

/* ------------------------------------------------------------------ */

static int is_vol_set[4][4]={
  {0,0,0,1},
  {1,0,0,1},
  {0,1,0,1},
  {1,0,1,1}
};

static int master_vol_table[128] = {
    0, 18, 28, 36, 42, 46, 51, 54, 57, 60, 62, 65, 67, 69, 70, 72,
   74, 75, 77, 78, 79, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 90,
   91, 92, 93, 93, 94, 95, 96, 96, 97, 97, 98, 99, 99,100,100,101,
  102,102,103,103,104,104,105,105,105,106,106,107,107,108,108,109,
  109,109,110,110,111,111,111,112,112,112,113,113,113,114,114,114,
  115,115,115,116,116,116,117,117,117,117,118,118,118,119,119,119,
  119,120,120,120,120,121,121,121,122,122,122,122,122,123,123,123,
  123,124,124,124,124,125,125,125,125,125,126,126,126,126,127,127
};

static MDX_DATA *mdx;

static int opl3_register_map[512];

static OPL3_WORK opl3[MDX_MAX_FM_TRACKS];

static int noise_decay        = 0;
static int opl3_enable        = FLAG_FALSE;
static int wave_form          = 0x00;
static int is_voice_changable = FLAG_TRUE;
static int is_use_opl3        = FLAG_TRUE;
static int master_volume      = 127;

/* ------------------------------------------------------------------ */
/* local functions */

static int  opl_read1( int , int );
static int  opl_read2( int , int , int );
static int  opl_read3( int , int , int );
static void opl_write1( int , int , int );
static void opl_write2( int , int , int , int );
static void opl_write3( int , int , int , int );

static void freq_write( int );
static void volume_write( int );

static void reg_write( int, int );
static int  reg_read( int );

/* ------------------------------------------------------------------ */

static int opl3_already_opened=FLAG_FALSE;
int opl3_open( void ) {

  int ret;

  if ( opl3_already_opened == FLAG_TRUE ) { return 0; }

  if ( regopen( OPL3_ADDRESS, 4 ) != 0 ) { ret=1; }
  else { ret=0; }

  if ( ret == 0 ) { opl3_already_opened = FLAG_TRUE; }

  return ret;
}

void opl3_close(void) {

  if (opl3_already_opened == FLAG_FALSE) return;
  regclose();
  opl3_already_opened = FLAG_FALSE;
}

void opl_reg_init( MDX_DATA *emdx ) {
  int i,j;

  mdx = emdx;

  if ( mdx->is_use_fm == FLAG_FALSE ) {
    opl3_enable = FLAG_FALSE;
    return;
  }

  is_use_opl3 = mdx->is_use_opl3;

  for ( i=0 ; i<512 ; i++ ) {
    reg_write(i,0);
  }

  opl3_enable = FLAG_TRUE;
  master_volume = 127;
  if ( mdx->is_use_fm_voice == FLAG_TRUE ) {
    wave_form = mdx->fm_wave_form;
    is_voice_changable = FLAG_TRUE;
  } else {
    wave_form = mdx->fm_wave_form;
    is_voice_changable = FLAG_FALSE;
  }
  if ( wave_form<0 ) { wave_form=0; }

  reg_write( 0x105, 0x01 );               /* OPL3 mode enable */
  reg_write( 0x001, 0x20 );               /* enable Waveform Select */

  if ( is_use_opl3 == FLAG_TRUE ) {
    reg_write( 0x104, 0x3f );             /* any channel uses 4 operator */
  } else {
    reg_write( 0x104, 0x00 );             /* any channel uses 2 operator */
  }

  reg_write( 0x008, 0x00 );               /* disable CSW & NOTE-SEL */
  reg_write( 0x0bd, 0x00 );               /* Tre / Vib / PercusMode / ... */
  reg_write( 0x1bd, 0x00 );

  for ( i=0 ; i<MDX_MAX_FM_TRACKS ; i++ ) {
    opl_write1( 0xc0, i, 0x00 );          /* Left / Right / FeedBack / Algo */
    opl_write1( 0xa0, i, 0x00 );          /* F-Number (low) */
    opl_write1( 0xb0, i, 0x00 );          /* KeyOn / Oct / F-Num (Hi) */
    if ( is_voice_changable == FLAG_FALSE ) {
      for ( j=0 ; j<4 ; j++ ) {
	if ( j==0 ) {
	  opl_write2( 0x40, i, j, 0x10 ); /* KSL / TL */
	} else {
	  opl_write2( 0x40, i, j, 0x3f ); /* KSL / TL */
	}
	opl_write2( 0x20, i, j, 0x21 );   /* Trem / Vib / Sus / KSR / MUL */
	opl_write2( 0x60, i, j, 0xf0 );   /* AR / DR */
	opl_write2( 0x80, i, j, 0xf7 );   /* SL / RR */
	opl_write2( 0xe0, i, j, wave_form ); /* WaveForm */
      }
    }
    else {
      for ( j=0 ; j<4 ; j++ ) {
	opl_write2( 0x40, i, j, 0x3f );   /* KSL / TL */
	opl_write2( 0x20, i, j, 0x21 );   /* Trem / Vib / Sus / KSR / MUL */
	opl_write2( 0x60, i, j, 0x0f );   /* AR / DR */
	opl_write2( 0x80, i, j, 0x0f );   /* SL / RR */
	opl_write2( 0xe0, i, j, wave_form ); /* WaveForm */
      }
    }
    opl_write1( 0xc0, i, 0x31 );          /* Left / Right / FeedBack / Algo */
  }

  for ( i=0 ; i<MDX_MAX_FM_TRACKS ; i++ ) {
    OPL3_WORK *o = &opl3[i];

    o->note          = 0;
    o->note_on       = 0;
    o->step          = 0;
    o->pan           = 3;
    o->detune.octave = 0;
    o->detune.scale  = 0;
    o->detune.detune = 0;
    o->portament     = 0;
    o->freq_reg[0]   = 0;
    o->freq_reg[1]   = 0;
    o->algorithm     = 0;
    o->volume        = 0;
    o->lfo_delay     = 0;

    o->plfo.flag     = FLAG_FALSE;
    o->plfo.form     = 0;
    o->plfo.clock    = 0;
    o->plfo.depth    = 0;

    o->alfo.flag     = FLAG_FALSE;
    o->alfo.form     = 0;
    o->alfo.clock    = 0;
    o->alfo.depth    = 0;

    for ( j=0 ; j<4 ; j++ ) {
      o->total_level[j]=0;
    }
    o->total_level[3]=63;
  }

  mdx->fm_noise_freq = -1;
  mdx->fm_noise_vol  =  0;
  noise_decay        = 15;

  return;
}

/* ------------------------------------------------------------------ */

void opl3_all_note_off( void ) {

  int i;
  int m = master_volume;

  master_volume = 0;
  for ( i=0 ; i<MDX_MAX_FM_TRACKS ; i++ ) {
    opl3_note_off(i);
    freq_write(i);
    volume_write(i);
  }
  master_volume = m;

  for ( i=0 ; i<9 ; i++ ) {
    reg_write( 0x0c0+i, 0x01 );
    if ( is_use_opl3==FLAG_TRUE ) {
      reg_write( 0x1c0+i, 0x01 );
    }
  }

  return;
}

void opl3_note_on( int track, int n ) {

  OPL3_WORK *o = &opl3[track];

  if ( o->step != 0 ) {
    o->portament = 0;
  }
  if ( o->note_on == 0 ) { /* for software-LFO w/ tie */
    o->step      = 0;
  }
  o->note        = n+3;
  o->note_on     = 1;
  o->freq_reg[0] = 0;
  o->freq_reg[1] = 0;

  freq_write(track);
  volume_write(track);

  return;
}

void opl3_note_off(int track) {

  opl3[track].note_on   = 0;
  opl3[track].portament = 0;

  return;
}

void opl3_set_pan( int track, int val ) {

  if ( val < 0 ) { val = 0; }
  if ( val > 3 ) { val = 3; }

  opl3[track].pan = val;

  return;
}

void opl3_set_volume( int track, int val ) {

  if ( val < 0 )   { val = 0; }
  if ( val > 127 ) { val = 127; }
  opl3[track].volume = val;

  return;
}

void opl3_set_master_volume( int val ) {

  if ( val < 0 )   { val = 0; }
  if ( val > 127 ) { val = 127; }
  master_volume = master_vol_table[val];

  return;
}

void opl3_set_detune( int track, int val ) {

  opl3[track].detune.octave = val/(64*12);
  opl3[track].detune.scale  = (val/64)%12;
  opl3[track].detune.detune = val%64;

  return;
}

void opl3_set_portament( int track, int val ) {

  opl3[track].portament = val;
  opl3[track].step      = 0;

  return;
}

void opl3_set_noise_freq( int val ) {

  if ( val < 0x80 ) {
    mdx->fm_noise_freq = -1;
    mdx->fm_noise_vol = 0;
  } else {
    mdx->fm_noise_freq = val-0x80;
  }

  return;
}

void opl3_set_freq_reg( int v1, int v2 ) {

  /* oct, scale */
  if ( v1>=0x28 && v1<0x30 ) {
    opl3[v1-0x28].freq_reg[0] = v2&0x7f;
  }

  /* kf */
  if ( v1>=0x30 && v1<0x38 ) {
    opl3[v1-0x30].freq_reg[1] = v2/4;
  }

  return;
}

void opl3_set_lfo_delay( int track, int delay ) {

  opl3[track].lfo_delay = delay;

  return;
}

void opl3_set_plfo( int track, int flag, int form, int clock, int depth ) {

  opl3[track].plfo.flag  = flag;
  opl3[track].plfo.form  = form;
  opl3[track].plfo.clock = clock;
  opl3[track].plfo.depth = depth;

  opl3[track].step = 0;

  return;
}

void opl3_set_alfo( int track, int flag, int form, int clock, int depth ) {

  opl3[track].alfo.flag  = flag;
  opl3[track].alfo.form  = form;
  opl3[track].alfo.clock = clock;
  opl3[track].alfo.depth = depth;

  return;
}

/* ------------------------------------------------------------------ */

void opl3_set_voice( int track, VOICE_DATA *v ) {

  OPL3_WORK *o = &opl3[track];

  int i,a,b,op;
  int MUL[4];
  int KSR[4], TL[4];
  int AR[4], DR[4], SL[4], RR[4];
  int AL,FB,SM;

  static int mul_opm_opl[16] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 12, 12, 15, 15
  };
  static int opl2_alg_op[8][2] = {
    {0,3}, {0,3}, {0,3}, {0,3},
    {0,2}, {0,3}, {0,2}, {0,3}
  };
  static int opl2_alg[8] = {
    0, 0, 0, 0, 0, 0, 0, 1
  };

  static int opl3_alg_op[8][4] = {
    {0,2,1,3}, {2,0,1,3}, {0,2,1,3}, {1,0,2,3},
    {0,2,1,3}, {0,2,0,3}, {1,0,2,3}, {1,0,2,3}
  };
  static int opl3_alg[8] = {
    0, 1, 1, 1, 2, 2, 3, 3
  };

  if ( is_voice_changable == FLAG_FALSE ) { 
    return;
  }

  AL = v->con;
  FB = v->fl&0x07;
  SM = v->slot_mask;

  for ( i=0 ; i<4 ; i++ ) {
    MUL[i] = mul_opm_opl[v->mul[i]&0x0f];
    KSR[i] = (v->ks[i]>>1)&1;
    TL[i] = v->tl[i];

    if ( v->ar[i] == 0 ) { AR[i] = 0; }
    else { AR[i] = (v->ar[i]>>1)+1; }

    if ( AR[i]>15 ) { AR[i]=15; }

    DR[i] = v->d2r[i]>>1;
    SL[i] = 0x0f - v->sl[i];
    RR[i] = v->rr[i];
  }

  if (is_use_opl3==FLAG_TRUE && track<6) {

    o->algorithm = opl3_alg[AL];
    if ( opl3_alg_op[AL][0]!=0 ) FB=0;
    a = opl_read3( 0xc0, track, 0 ) & 0xf0;
    opl_write3( 0xc0, track, 0, a+(FB<<1)+(o->algorithm&1) );
    a = opl_read3( 0xc0, track, 1 ) & 0xf0;
    opl_write3( 0xc0, track, 1, a+(FB<<1)+((o->algorithm>>1)&1) );

    for ( i=0 ; i<4 ; i++ ) {
      op = opl3_alg_op[AL][i];
      
      b = 0x3f-TL[op];
      if ( b<0  ) { b=0; }
      o->total_level[i] = b;
      if ( is_vol_set[o->algorithm][i]==1 ) { b=0; }
      a = opl_read2( 0x40, track, i )&0xc0;
      opl_write2( 0x40, track, i, a+(0x3f-b) );
      
      a = opl_read2( 0x20, track, i )&0xe0;
      opl_write2( 0x20, track, i, a+(KSR[op]<<4)+MUL[op] );
      opl_write2( 0x60, track, i, (AR[op]<<4)+DR[op] );
      opl_write2( 0x80, track, i, (SL[op]<<4)+RR[op] );
    }
  }
  else {
    int ad[2];
    ad[0]=opl2_alg_op[AL][0]; /* modulator */
    ad[1]=opl2_alg_op[AL][1]; /* carrier */
    o->algorithm = opl2_alg[AL];

    if ( ad[0]!=0 ) { FB=0; }
    a = opl_read1( 0xc0, track ) & 0xf0;
    opl_write1( 0xc0, track, a+(FB<<1)+o->algorithm );

    for ( i=0 ; i<2 ; i++ ) {
      op = ad[i];
      
      b = 0x3f-TL[op];
      if ( b<0  ) { b=0; }
      o->total_level[i] = b;
      if ( o->algorithm==1 || i==1 ) { b=0; }
      a = opl_read2( 0x40, track, i )&0xc0;
      opl_write2( 0x40, track, i, a+(0x3f-b) );
      
      a = opl_read2( 0x20, track, i )&0xe0;
      opl_write2( 0x20, track, i, a+(KSR[op]<<4)+MUL[op] );
      opl_write2( 0x60, track, i, (AR[op]<<4)+DR[op] );
      opl_write2( 0x80, track, i, (SL[op]<<4)+RR[op] );
    }
  }

  if  ( track==7 ) {
    noise_decay = DR[3];
  }

  return;
}

/* ------------------------------------------------------------------ */

static int note_to_fnum( int octave, int note, int kf ) {

  /* from linux/drivers/sound/sequencer.c */

  int note_freq;
  int note2_freq;
  static int notes[] = {
    261632, 277189, 293671, 311132, 329632, 349232,
    369998, 391998, 415306, 440000, 466162, 493880,
    261632*2
  };

#define BASE_OCTAVE     4

  note_freq  = notes[note];
  note2_freq = notes[note+1];
  note_freq += (note2_freq - note_freq) * kf / 64;

  if (octave < BASE_OCTAVE) {
    note_freq >>= (BASE_OCTAVE - octave);
  } else if (octave > BASE_OCTAVE) {
    note_freq <<= (octave - BASE_OCTAVE);
  }

  return (note_freq/1000 * (1 << (20-octave))) / 49716;
}

void opl3_set_freq_volume( int track ) {

  opl3[track].step++;
  freq_write( track );
  volume_write( track );

  return;
}

static void freq_write( int track ) {

  OPL3_WORK *o = &opl3[track];
  int oct, scale, kf;
  int fh, fl;
  int ofs_o, ofs_s, ofs_f;
  long long f;
  int c,d;
  int key;

  if ( o->freq_reg[0] == 0 && o->freq_reg[1] == 0 ) {

    /* detune jobs */
    
    ofs_o = o->detune.octave;  /* octave */
    ofs_s = o->detune.scale;   /* scale */
    ofs_f = o->detune.detune;  /* detune */
    
    
    /* portament jobs */
    
    if ( o->portament != 0 ) {
      f = o->portament * o->step / 256;

      ofs_f += f%64;          /* detune */
      c=0;
      while ( ofs_f > 63 ) { ofs_f-=64; c++; }
      while ( ofs_f <  0 ) { ofs_f+=64; c--; }
      
      ofs_s += c+(f/64)%12;   /* scale */
      c=0;
      while ( ofs_s > 11 ) { ofs_s-=12; c++; }
      while ( ofs_s <  0 ) { ofs_s+=12; c--; }
      
      ofs_o += c+f/(64*12);   /* octave */
    }
    
    /* soft-lfo jobs */

    d = o->step - o->lfo_delay;
    if ( o->plfo.flag == FLAG_TRUE && d >= 0 ) {
      int cl;
      int c;
      switch(o->plfo.form) {
      case 0:
      case 4:
	cl = d % o->plfo.clock;
	f = o->plfo.depth * cl;
	break;

      case 1:
      case 3:
      case 5:
      case 7:
	cl = d % (o->plfo.clock*2);
	if ( cl<o->plfo.clock ) {
	  f =  o->plfo.depth/2;
	} else {
	  f = -(o->plfo.depth/2);
	}
	break;

      case 2:
      case 6:
	cl = d % (o->plfo.clock*2);
	c = cl % (o->plfo.clock/2);
	switch ( cl/(o->plfo.clock/2) ) {
	case 0:
	  f = o->plfo.depth * c;
	  break;
	case 1:
	  f = o->plfo.depth * (o->plfo.clock/2 - c );
	  break;
	case 2:
	  f = -(o->plfo.depth * c);
	  break;
	case 3:
	  f = -(o->plfo.depth * (o->plfo.clock/2 - c));
	  break;

	default:
	  f = 0;
	  break;
	}
	break;

      default:
	f=0;
	break;
      }

      f/=256;
      ofs_f += f%64;          /* detune */
      c = 0;
      while ( ofs_f > 63 ) { ofs_f-=64; c++; }
      while ( ofs_f <  0 ) { ofs_f+=64; c--; }

      ofs_s += c+(f/64)%12;   /* scale */
      c=0;
      while ( ofs_s > 11 ) { ofs_s-=12; c++; }
      while ( ofs_s <  0 ) { ofs_s+=12; c--; }

      ofs_o += c+f/(64*12);   /* octave */
    }
    
    /* generate f-number */
    
    kf = ofs_f;
    c=0;
    while ( kf > 63 ) { kf -= 64; c++; }
    while ( kf <  0 ) { kf += 64; c--; }

    scale = c + (o->note%12) + ofs_s;
    c=0;
    while ( scale > 11 ) { scale-=12; c++; }
    while ( scale <  0 ) { scale+=12; c--; }

    oct = c + (o->note/12) + ofs_o;
    if ( oct>7 ) { oct = 7; }
    else if ( oct<0 ) { oct = 0; }

    f = note_to_fnum( oct, scale, kf );
  }
  else {  /* freq_reg[track][0,1] != 0 */
    if ( o->freq_reg[0] == 0 ) {
      oct   = o->note/12;
      scale = o->note%12;
    } else {
      oct   = o->freq_reg[0]/12;
      scale = o->freq_reg[0]%12;
    }
    if ( scale<0 ) { scale=0; }
    kf = o->freq_reg[1];
    f = note_to_fnum( oct, scale, kf );
  }

  /* write to register */

  if ( oct>7 ) { oct = 7; }
  if ( oct<0 ) { oct = 0; }
    
  fl = f%256;
  if ( fl<0 ) { fl=0; }
  fh = f/256;
  if ( fh>3 ) { fh=3; }
  if ( fh<0 ) { fh=0; }

  if ( o->note_on != 0 ) {
    key=0x20;
  } else {
    key=0;
  }

  if ( track==7 && mdx->fm_noise_freq>=0 ) {
    key=0;
  }

  opl_write1( 0xa0, track, fl );
  opl_write1( 0xb0, track, key + ((oct&7)<<2) + (fh&3) );

  return;
}

static void volume_write( int track ) {

  OPL3_WORK *o = &opl3[track];
  int i,v;
  int vol;
  int key=0;

  /* set volume */

  if ( is_use_opl3==FLAG_TRUE && track<6 ) {
    if ( is_voice_changable == FLAG_FALSE ) {
      v = opl_read2( 0x40, track, 0 )&0xc0;

      vol = master_volume * o->volume/127;
      vol = 127-vol;
      if ( vol < 100 ) { key = 1; }
      if ( vol > 0x3f ) { vol = 0x3f; }

      opl_write2( 0x40, track, 0, v+vol );

    } else {
      for ( i=0 ; i<4 ; i++ ) {
	if ( is_vol_set[o->algorithm][i]==0 ) continue;
	v = opl_read2( 0x40, track, i )&0xc0;

	vol = master_volume * o->volume * o->total_level[i] / 127 / 63;
	vol = 127-vol;
	if ( vol < 100 ) { key = 1; }
	if ( vol > 0x3f ) { vol = 0x3f; }
	
	opl_write2( 0x40, track, i, v+vol );
      }
    }
  }
  else {
    if ( is_voice_changable == FLAG_TRUE ) {
      for ( i=0 ; i<2 ; i++ ) {
	if ( i==0 && o->algorithm==0 ) {
	  continue;
	}

	v = opl_read2( 0x40, track, i ) &0xc0;
	vol = master_volume * o->volume * o->total_level[i] / 127 / 63;
	if ( track==7 && mdx->fm_noise_freq>=0 ) { vol = 0; }
	vol = 127-vol;
	if ( vol < 100 ) { key = 1; }
	if ( vol > 0x3f ) { vol = 0x3f; }
	
	opl_write2( 0x40, track, i, v+vol);
      }
    }
    else {
      v = opl_read2( 0x40, track, 0 ) &0xc0;
      vol = master_volume * o->volume / 127;
      if ( track==7 && mdx->fm_noise_freq>=0 ) vol = 0;
      vol = 127-vol;
      if ( vol < 100 ) { key = 1; }
      if ( vol > 0x3f ) { vol = 0x3f; }
      
      opl_write2( 0x40, track, 0, v+vol);
    }
  }

  /* set panpot */

  if ( is_use_opl3==FLAG_TRUE && track<6) {
    if ( key != 0 ) {
      v=opl_read3(0xc0, track, 0);
      if ( ((v&0x0f0)>>4) != o->pan ) {
	opl_write3(0xc0, track, 0, (v&0x0f)+(o->pan<<4));
	v=opl_read3(0xc0, track, 1);
	opl_write3(0xc0, track, 1, (v&0x0f)+(o->pan<<4));
      }
    } else {
      v=opl_read3(0xc0, track, 0);
      if ( (v&0x0f0) != 0 ) {
	opl_write3(0xc0, track, 0, v&0x0f);
	v=opl_read3(0xc0, track, 1);
	opl_write3(0xc0, track, 1, v&0x0f);
      }
    }
  }
  else {
    if ( key != 0 ) {
      v=opl_read1(0xc0,track);
      if ( ((v&0x0f0)>>4) != o->pan )
	opl_write1(0xc0, track, (v&0x0f) + (o->pan<<4));
      
    } else {
      v=opl_read1(0xc0,track);
      if ( ((v&0x0f0)>>4) != 0 )
	opl_write1(0xc0, track, v&0x0f);
    }
  }
  
  /* set NOISE part */

  if ( track==7 && mdx->fm_noise_freq>=0 ) {
    if ( o->note_on != 0 ) {
      mdx->fm_noise_vol = 2*master_volume * o->volume *
	o->total_level[1] / 127 / 63;
      mdx->fm_noise_vol -= o->step*(15-noise_decay)*2;
      if ( mdx->fm_noise_vol < 0 ) { mdx->fm_noise_vol = 0; }
      if ( mdx->fm_noise_vol > 127 ) { mdx->fm_noise_vol = 127; }
    }
    else {
      mdx->fm_noise_vol = 0;
    }
  }

  return;
}

/* ------------------------------------------------------------------ */
/* register actions */

static int opl2_op[8][2]={
  { 0, 3},
  { 1, 4},
  { 2, 5},
  { 6, 9},
  { 7,10},
  { 8,11},
  {12,15},
  {13,16}
};
static int opl3_ch[]={
  0x000, 0x001, 0x002,
  0x100, 0x101, 0x102,
  0x006, 0x007,
  0x106, 0x107
};
static int opl3_op[8][4]={
  {  0, 3, 6, 9 },
  {  1, 4, 7,10 },
  {  2, 5, 8,11 },
  { 18,21,24,27 },
  { 19,22,25,28 },
  { 20,23,26,29 },
  { 12,15,30,33 },
  { 13,16,31,34 }
};
static int rg[] = {
  0x000, 0x001, 0x002, 0x003, 0x004, 0x005,
  0x008, 0x009, 0x00a, 0x00b, 0x00c, 0x00d,
  0x010, 0x011, 0x012, 0x013, 0x014, 0x015,
  0x100, 0x101, 0x102, 0x103, 0x104, 0x105,
  0x108, 0x109, 0x10a, 0x10b, 0x10c, 0x10d,
  0x110, 0x111, 0x112, 0x113, 0x114, 0x115
};

static int chtrk[] = {  /* you can change the assignment of 4-op FM */
  0,1,2,3,4,5,6,7
};

/* ------------------------------------------------------------------ */

static int opl3_reg_4op( int track, int op ) {

  if ( track<0 || op<0 ) { return 0; }
  if ( track>=MDX_MAX_FM_TRACKS || op>=4 ) { return 0; }

  return rg[opl3_op[track][op]];
}

static int opl3_reg_2op( int track, int op ) {

  if ( track<0 || op<0 ) { return 0; }
  if ( track>=MDX_MAX_FM_TRACKS || op>=2 ) { return 0; }

  return rg[opl2_op[track][op]];
}

static void opl_write1( int base, int track, int val ) {
  int adr;

  if  ( track>=MDX_MAX_FM_TRACKS || track< 0 ) { return; }

  track=chtrk[track];

  if ( is_use_opl3==FLAG_TRUE && track<6 ) {

    adr = base + opl3_ch[track];
    reg_write( adr, val );

    adr = base + opl3_ch[track]+3;
    reg_write( adr, val );
  }
  else {
    adr = base + track;
    reg_write( adr, val );
  }

  return;
}

static int opl_read1( int base, int track ) {
  int adr, val;

  if  ( track>=MDX_MAX_FM_TRACKS || track<0 ) { return 0; }

  track=chtrk[track];

  if ( is_use_opl3==FLAG_TRUE && track<6 ) {
    adr = base + opl3_ch[track]+0;
  } else {
    adr = base + track;
  }
  val=reg_read( adr );

  return val;
}

static void opl_write2( int base, int track ,int op, int val ) {
  int adr;

  if ( track>=MDX_MAX_FM_TRACKS || track<0 ) { return; }
  if ( op<0 ) { return; }

  track=chtrk[track];

  if ( is_use_opl3==FLAG_TRUE && track<6 ) {
    if ( op>3 ) { return; }
    adr = base + opl3_reg_4op(track,op);
  }
  else {
    if ( op>1 ) { return; }
    adr = base + opl3_reg_2op(track,op);
  }
  reg_write( adr, val );

  return;
}

static int opl_read2( int base, int track, int op ) {
  int adr, val;

  if ( track>=MDX_MAX_FM_TRACKS ) { return 0; }
  if ( op<0 ) { return 0; }

  track=chtrk[track];

  if ( is_use_opl3==FLAG_TRUE && track<6 ) {
    if ( op>3 ) { return 0; }
    adr = base + opl3_reg_4op(track,op);
  }
  else {
    if ( op>1 ) { return 0; }
    adr = base + opl3_reg_2op(track,op);
  }
  val=reg_read( adr );

  return val;
}

static void opl_write3( int base, int track, int op, int val ) {
  int adr;

  if  (track>=MDX_MAX_FM_TRACKS || track<0) { return; }
  if ( op<0 || op>1 ) { return; }

  track=chtrk[track];

  if ( is_use_opl3==FLAG_TRUE && track<6 ) {
    adr = base + opl3_ch[track]+op*3;
  }else {
    adr = base + track+op;
  }
  reg_write( adr, val );
  return;
}

static int opl_read3( int base, int track, int op ) {
  int adr, val;

  if  (track>=MDX_MAX_FM_TRACKS || track<0) { return 0; }
  if ( op<0 || op>1 ) { return 0; }

  track=chtrk[track];

  if ( is_use_opl3==FLAG_TRUE && track<6 ) {
    adr = base + opl3_ch[track]+op*3;
  } else {
    adr = base + track+op;
  }
  val=reg_read( adr );

  return val;
}

static void reg_write( int adr, int val ) {

  int bank;

  if ( adr > 0x1ff ) { return; }
  opl3_register_map[adr] = val;

  if ( adr > 0xff ) {
    bank=1;
    adr -= 0x100;
  }
  else {
    bank=0;
  }

  if ( opl3_enable == FLAG_TRUE ) {
    regwr( OPL3_ADDRESS+bank*2, adr );
    regwr( OPL3_ADDRESS+1+bank*2, val );
  }

  return;
}

static int reg_read( int address ) {

  if ( address > 0x1ff ) { return 0; }
  return opl3_register_map[address];
}

#else /* CAN_HANDLE_OPL3 */
int opl3_open( void ) {
  return -1;
}
void opl3_close(void)
{
  return -1;
}
#endif /* CAN_HANDLE_OPL3 */
