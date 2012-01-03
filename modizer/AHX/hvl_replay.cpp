/*
** Changes for the 1.4 release are commented. You can do
** a search for "1.4" and merge them into your own replay
** code.
**
** Changes for 1.5 are marked also.
**
** ... as are those for 1.6
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "hvl_replay.h"

int32 stereopan_left[]  = { 128,  96,  64,  32,   0 };
int32 stereopan_right[] = { 128, 160, 193, 225, 255 };

/*
** Waves
*/
#define WHITENOISELEN (0x280*3)

#define WO_LOWPASSES   0
#define WO_TRIANGLE_04 (WO_LOWPASSES+((0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31))
#define WO_TRIANGLE_08 (WO_TRIANGLE_04+0x04)
#define WO_TRIANGLE_10 (WO_TRIANGLE_08+0x08)
#define WO_TRIANGLE_20 (WO_TRIANGLE_10+0x10)
#define WO_TRIANGLE_40 (WO_TRIANGLE_20+0x20)
#define WO_TRIANGLE_80 (WO_TRIANGLE_40+0x40)
#define WO_SAWTOOTH_04 (WO_TRIANGLE_80+0x80)
#define WO_SAWTOOTH_08 (WO_SAWTOOTH_04+0x04)
#define WO_SAWTOOTH_10 (WO_SAWTOOTH_08+0x08)
#define WO_SAWTOOTH_20 (WO_SAWTOOTH_10+0x10)
#define WO_SAWTOOTH_40 (WO_SAWTOOTH_20+0x20)
#define WO_SAWTOOTH_80 (WO_SAWTOOTH_40+0x40)
#define WO_SQUARES     (WO_SAWTOOTH_80+0x80)
#define WO_WHITENOISE  (WO_SQUARES+(0x80*0x20))
#define WO_HIGHPASSES  (WO_WHITENOISE+WHITENOISELEN)
#define WAVES_SIZE     (WO_HIGHPASSES+((0xfc+0xfc+0x80*0x1f+0x80+3*0x280)*31))

int8 waves[WAVES_SIZE];
int16 waves2[WAVES_SIZE];

int16 vib_tab[] =
{ 
  0,24,49,74,97,120,141,161,180,197,212,224,235,244,250,253,255,
  253,250,244,235,224,212,197,180,161,141,120,97,74,49,24,
  0,-24,-49,-74,-97,-120,-141,-161,-180,-197,-212,-224,-235,-244,-250,-253,-255,
  -253,-250,-244,-235,-224,-212,-197,-180,-161,-141,-120,-97,-74,-49,-24
};

uint16 period_tab[] =
{
  0x0000, 0x0D60, 0x0CA0, 0x0BE8, 0x0B40, 0x0A98, 0x0A00, 0x0970,
  0x08E8, 0x0868, 0x07F0, 0x0780, 0x0714, 0x06B0, 0x0650, 0x05F4,
  0x05A0, 0x054C, 0x0500, 0x04B8, 0x0474, 0x0434, 0x03F8, 0x03C0,
  0x038A, 0x0358, 0x0328, 0x02FA, 0x02D0, 0x02A6, 0x0280, 0x025C,
  0x023A, 0x021A, 0x01FC, 0x01E0, 0x01C5, 0x01AC, 0x0194, 0x017D,
  0x0168, 0x0153, 0x0140, 0x012E, 0x011D, 0x010D, 0x00FE, 0x00F0,
  0x00E2, 0x00D6, 0x00CA, 0x00BE, 0x00B4, 0x00AA, 0x00A0, 0x0097,
  0x008F, 0x0087, 0x007F, 0x0078, 0x0071
};

uint32 panning_left[256], panning_right[256];

void hvl_GenPanningTables( void )
{
  uint32 i;
  float64 aa, ab;

  // Sine based panning table
  aa = (3.14159265f*2.0f)/4.0f;   // Quarter of the way through the sinewave == top peak
  ab = 0.0f;                      // Start of the climb from zero

  for( i=0; i<256; i++ )
  {
    panning_left[i]  = (uint32)(sin(aa)*255.0f);
    panning_right[i] = (uint32)(sin(ab)*255.0f);
    
    aa += (3.14159265*2.0f/4.0f)/256.0f;
    ab += (3.14159265*2.0f/4.0f)/256.0f;
  }
  panning_left[255] = 0;
  panning_right[0] = 0;
}

void hvl_GenSawtooth( int8 *buf, uint32 len )
{
  uint32 i;
  int32  val, add;
  
  add = 256 / (len-1);
  val = -128;
  
  for( i=0; i<len; i++, val += add )
    *buf++ = (int8)val;  
}

void hvl_GenTriangle( int8 *buf, uint32 len )
{
  uint32 i;
  int32  d2, d5, d1, d4;
  int32  val;
  int8   *buf2;
  
  d2  = len;
  d5  = len >> 2;
  d1  = 128/d5;
  d4  = -(d2 >> 1);
  val = 0;
  
  for( i=0; i<d5; i++ )
  {
    *buf++ = val;
    val += d1;
  }
  *buf++ = 0x7f;

  if( d5 != 1 )
  {
    val = 128;
    for( i=0; i<d5-1; i++ )
    {
      val -= d1;
      *buf++ = val;
    }
  }
  
  buf2 = buf + d4;
  for( i=0; i<d5*2; i++ )
  {
    int8 c;
    
    c = *buf2++;
    if( c == 0x7f )
      c = 0x80;
    else
      c = -c;
    
    *buf++ = c;
  }
}

void hvl_GenSquare( int8 *buf )
{
  uint32 i, j;
  
  for( i=1; i<=0x20; i++ )
  {
    for( j=0; j<(0x40-i)*2; j++ )
      *buf++ = 0x80;
    for( j=0; j<i*2; j++ )
      *buf++ = 0x7f;
  }
}

static inline float64 clip( float64 x )
{
  if( x > 127.f )
    x = 127.f;
  else if( x < -128.f )
    x = -128.f;
  return x;
}

void hvl_GenFilterWaves( int8 *buf, int8 *lowbuf, int8 *highbuf )
{
  static const uint16 lentab[45] = { 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 3, 7, 0xf, 0x1f, 0x3f, 0x7f,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    (0x280*3)-1 };

  float64 freq;
  uint32  temp;
  
  for( temp=0, freq=8.f; temp<31; temp++, freq+=3.f )
  {
    uint32 wv;
    int8   *a0 = buf;
    
    for( wv=0; wv<6+6+0x20+1; wv++ )
    {
      float64 fre, high, mid, low;
      uint32  i;
      
      mid = 0.f;
      low = 0.f;
      fre = freq * 1.25f / 100.0f;
      
      for( i=0; i<=lentab[wv]; i++ )
      {
        high  = a0[i] - mid - low;
        high  = clip( high );
        mid  += high * fre;
        mid   = clip( mid );
        low  += mid * fre;
        low   = clip( low );
      }
      
      for( i=0; i<=lentab[wv]; i++ )
      {
        high  = a0[i] - mid - low;
        high  = clip( high );
        mid  += high * fre;
        mid   = clip( mid );
        low  += mid * fre;
        low   = clip( low );
        *lowbuf++  = (int8)low;
        *highbuf++ = (int8)high;
      }
      
      a0 += lentab[wv]+1;
    }
  }
}

void hvl_GenWhiteNoise( int8 *buf, uint32 len )
{
  uint32 ays;

  ays = 0x41595321;

  do {
    uint16 ax, bx;
    int8 s;

    s = ays;

    if( ays & 0x100 )
    {
      s = 0x80;

      if( (int32)(ays & 0xffff) >= 0 )
        s = 0x7f;
    }

    *buf++ = s;
    len--;

    ays = (ays >> 5) | (ays << 27);
    ays = (ays & 0xffffff00) | ((ays & 0xff) ^ 0x9a);
    bx  = ays;
    ays = (ays << 2) | (ays >> 30);
    ax  = ays;
    bx  += ax;
    ax  ^= bx;
    ays  = (ays & 0xffff0000) | ax;
    ays  = (ays >> 3) | (ays << 29);
  } while( len );
}

void hvl_reset_some_stuff( struct hvl_tune *ht )
{
  uint32 i;

  for( i=0; i<MAX_CHANNELS; i++ )
  {
    ht->ht_Voices[i].vc_Delta=1;
    ht->ht_Voices[i].vc_OverrideTranspose=1000;  // 1.5
    ht->ht_Voices[i].vc_SamplePos=ht->ht_Voices[i].vc_Track=ht->ht_Voices[i].vc_Transpose=ht->ht_Voices[i].vc_NextTrack = ht->ht_Voices[i].vc_NextTranspose = 0;
    ht->ht_Voices[i].vc_ADSRVolume=ht->ht_Voices[i].vc_InstrPeriod=ht->ht_Voices[i].vc_TrackPeriod=ht->ht_Voices[i].vc_VibratoPeriod=ht->ht_Voices[i].vc_NoteMaxVolume=ht->ht_Voices[i].vc_PerfSubVolume=ht->ht_Voices[i].vc_TrackMasterVolume=0;
    ht->ht_Voices[i].vc_NewWaveform=ht->ht_Voices[i].vc_Waveform=ht->ht_Voices[i].vc_PlantSquare=ht->ht_Voices[i].vc_PlantPeriod=ht->ht_Voices[i].vc_IgnoreSquare=0;
    ht->ht_Voices[i].vc_TrackOn=ht->ht_Voices[i].vc_FixedNote=ht->ht_Voices[i].vc_VolumeSlideUp=ht->ht_Voices[i].vc_VolumeSlideDown=ht->ht_Voices[i].vc_HardCut=ht->ht_Voices[i].vc_HardCutRelease=ht->ht_Voices[i].vc_HardCutReleaseF=0;
    ht->ht_Voices[i].vc_PeriodSlideSpeed=ht->ht_Voices[i].vc_PeriodSlidePeriod=ht->ht_Voices[i].vc_PeriodSlideLimit=ht->ht_Voices[i].vc_PeriodSlideOn=ht->ht_Voices[i].vc_PeriodSlideWithLimit=0;
    ht->ht_Voices[i].vc_PeriodPerfSlideSpeed=ht->ht_Voices[i].vc_PeriodPerfSlidePeriod=ht->ht_Voices[i].vc_PeriodPerfSlideOn=ht->ht_Voices[i].vc_VibratoDelay=ht->ht_Voices[i].vc_VibratoCurrent=ht->ht_Voices[i].vc_VibratoDepth=ht->ht_Voices[i].vc_VibratoSpeed=0;
    ht->ht_Voices[i].vc_SquareOn=ht->ht_Voices[i].vc_SquareInit=ht->ht_Voices[i].vc_SquareLowerLimit=ht->ht_Voices[i].vc_SquareUpperLimit=ht->ht_Voices[i].vc_SquarePos=ht->ht_Voices[i].vc_SquareSign=ht->ht_Voices[i].vc_SquareSlidingIn=ht->ht_Voices[i].vc_SquareReverse=0;
    ht->ht_Voices[i].vc_FilterOn=ht->ht_Voices[i].vc_FilterInit=ht->ht_Voices[i].vc_FilterLowerLimit=ht->ht_Voices[i].vc_FilterUpperLimit=ht->ht_Voices[i].vc_FilterPos=ht->ht_Voices[i].vc_FilterSign=ht->ht_Voices[i].vc_FilterSpeed=ht->ht_Voices[i].vc_FilterSlidingIn=ht->ht_Voices[i].vc_IgnoreFilter=0;
    ht->ht_Voices[i].vc_PerfCurrent=ht->ht_Voices[i].vc_PerfSpeed=ht->ht_Voices[i].vc_WaveLength=ht->ht_Voices[i].vc_NoteDelayOn=ht->ht_Voices[i].vc_NoteCutOn=0;
    ht->ht_Voices[i].vc_AudioPeriod=ht->ht_Voices[i].vc_AudioVolume=ht->ht_Voices[i].vc_VoiceVolume=ht->ht_Voices[i].vc_VoicePeriod=ht->ht_Voices[i].vc_VoiceNum=ht->ht_Voices[i].vc_WNRandom=0;
    ht->ht_Voices[i].vc_SquareWait=ht->ht_Voices[i].vc_FilterWait=ht->ht_Voices[i].vc_PerfWait=ht->ht_Voices[i].vc_NoteDelayWait=ht->ht_Voices[i].vc_NoteCutWait=0;
    ht->ht_Voices[i].vc_PerfList=0;
    ht->ht_Voices[i].vc_RingSamplePos=ht->ht_Voices[i].vc_RingDelta=ht->ht_Voices[i].vc_RingPlantPeriod=ht->ht_Voices[i].vc_RingAudioPeriod=ht->ht_Voices[i].vc_RingNewWaveform=ht->ht_Voices[i].vc_RingWaveform=ht->ht_Voices[i].vc_RingFixedPeriod=ht->ht_Voices[i].vc_RingBasePeriod=0;

    ht->ht_Voices[i].vc_RingMixSource = NULL;
    ht->ht_Voices[i].vc_RingAudioSource = NULL;

    memset(&ht->ht_Voices[i].vc_SquareTempBuffer,0,0x80);
    memset(&ht->ht_Voices[i].vc_ADSR,0,sizeof(struct hvl_envelope));
    memset(&ht->ht_Voices[i].vc_VoiceBuffer,0,0x281);
    memset(&ht->ht_Voices[i].vc_RingVoiceBuffer,0,0x281);
  }
  
  for( i=0; i<MAX_CHANNELS; i++ )
  {
    ht->ht_Voices[i].vc_WNRandom          = 0x280;
    ht->ht_Voices[i].vc_VoiceNum          = i;
    ht->ht_Voices[i].vc_TrackMasterVolume = 0x40;
    ht->ht_Voices[i].vc_TrackOn           = 1;
    ht->ht_Voices[i].vc_MixSource         = ht->ht_Voices[i].vc_VoiceBuffer;
  }
}

HVL_BOOL hvl_InitSubsong( struct hvl_tune *ht, uint32 nr )
{
  uint32 PosNr, i;

  if( nr > ht->ht_SubsongNr )
    return FALSE;

  ht->ht_SongNum = nr;
  
  PosNr = 0;
  if( nr ) PosNr = ht->ht_Subsongs[nr-1];
  
  ht->ht_PosNr          = PosNr;
  ht->ht_PosJump        = 0;
  ht->ht_PatternBreak   = 0;
  ht->ht_NoteNr         = 0;
  ht->ht_PosJumpNote    = 0;
  ht->ht_Tempo          = 6;
  ht->ht_StepWaitFrames	= 0;
  ht->ht_GetNewPosition = 1;
  ht->ht_SongEndReached = 0;
  ht->ht_PlayingTime    = 0;
  
  for( i=0; i<MAX_CHANNELS; i+=4 )
  {
    ht->ht_Voices[i+0].vc_Pan          = ht->ht_defpanleft;
    ht->ht_Voices[i+0].vc_SetPan       = ht->ht_defpanleft; // 1.4
    ht->ht_Voices[i+0].vc_PanMultLeft  = panning_left[ht->ht_defpanleft];
    ht->ht_Voices[i+0].vc_PanMultRight = panning_right[ht->ht_defpanleft];
    ht->ht_Voices[i+1].vc_Pan          = ht->ht_defpanright;
    ht->ht_Voices[i+1].vc_SetPan       = ht->ht_defpanright; // 1.4
    ht->ht_Voices[i+1].vc_PanMultLeft  = panning_left[ht->ht_defpanright];
    ht->ht_Voices[i+1].vc_PanMultRight = panning_right[ht->ht_defpanright];
    ht->ht_Voices[i+2].vc_Pan          = ht->ht_defpanright;
    ht->ht_Voices[i+2].vc_SetPan       = ht->ht_defpanright; // 1.4
    ht->ht_Voices[i+2].vc_PanMultLeft  = panning_left[ht->ht_defpanright];
    ht->ht_Voices[i+2].vc_PanMultRight = panning_right[ht->ht_defpanright];
    ht->ht_Voices[i+3].vc_Pan          = ht->ht_defpanleft;
    ht->ht_Voices[i+3].vc_SetPan       = ht->ht_defpanleft;  // 1.4
    ht->ht_Voices[i+3].vc_PanMultLeft  = panning_left[ht->ht_defpanleft];
    ht->ht_Voices[i+3].vc_PanMultRight = panning_right[ht->ht_defpanleft];
  }

  hvl_reset_some_stuff( ht );
  
  return TRUE;
}

void hvl_InitReplayer( void )
{
  hvl_GenPanningTables();
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_04], 0x04 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_08], 0x08 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_10], 0x10 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_20], 0x20 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_40], 0x40 );
  hvl_GenSawtooth( &waves[WO_SAWTOOTH_80], 0x80 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_04], 0x04 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_08], 0x08 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_10], 0x10 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_20], 0x20 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_40], 0x40 );
  hvl_GenTriangle( &waves[WO_TRIANGLE_80], 0x80 );
  hvl_GenSquare( &waves[WO_SQUARES] );
  hvl_GenWhiteNoise( &waves[WO_WHITENOISE], WHITENOISELEN );
  hvl_GenFilterWaves( &waves[WO_TRIANGLE_04], &waves[WO_LOWPASSES], &waves[WO_HIGHPASSES] );
}

struct hvl_tune *hvl_load_ahx( uint8 *buf, uint32 buflen, uint32 defstereo, uint32 freq )
{
  uint8  *bptr;
  TEXT   *nptr;
  uint32  i, j, k, l, posn, insn, ssn, hs, trkn, trkl;
  struct hvl_tune *ht;
  struct  hvl_plsentry *ple;
  int32 defgain[] = { 71, 72, 76, 85, 100 };
  
  posn = ((buf[6]&0x0f)<<8)|buf[7];
  insn = buf[12];
  ssn  = buf[13];
  trkl = buf[10];
  trkn = buf[11];

  hs  = sizeof( struct hvl_tune );
  hs += sizeof( struct hvl_position ) * posn;
  hs += sizeof( struct hvl_instrument ) * (insn+1);
  hs += sizeof( uint16 ) * ssn;

  // Calculate the size of all instrument PList buffers
  bptr = &buf[14];
  bptr += ssn*2;    // Skip past the subsong list
  bptr += posn*4*2; // Skip past the positions
  bptr += trkn*trkl*3;
  if((buf[6]&0x80)==0) bptr += trkl*3;
  
  // *NOW* we can finally calculate PList space
  for( i=1; i<=insn; i++ )
  {
    hs += bptr[21] * sizeof( struct hvl_plsentry );
    bptr += 22 + bptr[21]*4;
  }

  ht = (hvl_tune*) malloc( hs );
  if( !ht )
  {
    free( buf );
    printf( "Out of memory!\n" );
    return NULL;
  }
  
  ht->ht_ModType		 = 0;  //AHX	
  ht->ht_Frequency       = freq;
  ht->ht_FreqF           = (float64)freq;
  
  ht->ht_Positions   = (struct hvl_position *)(&ht[1]);
  ht->ht_Instruments = (struct hvl_instrument *)(&ht->ht_Positions[posn]);
  ht->ht_Subsongs    = (uint16 *)(&ht->ht_Instruments[(insn+1)]);
  ple                = (struct hvl_plsentry *)(&ht->ht_Subsongs[ssn]);

  ht->ht_WaveformTab[0]  = &waves[WO_TRIANGLE_04];
  ht->ht_WaveformTab[1]  = &waves[WO_SAWTOOTH_04];
  ht->ht_WaveformTab[3]  = &waves[WO_WHITENOISE];

  ht->ht_Channels        = 4;
  ht->ht_PositionNr      = posn;
  ht->ht_Restart         = (buf[8]<<8)|buf[9];
  ht->ht_SpeedMultiplier = ((buf[6]>>5)&3)+1;
  ht->ht_TrackLength     = trkl;
  ht->ht_TrackNr         = trkn;
  ht->ht_InstrumentNr    = insn;
  ht->ht_SubsongNr       = ssn;
  ht->ht_defstereo       = defstereo;
  ht->ht_defpanleft      = stereopan_left[ht->ht_defstereo];
  ht->ht_defpanright     = stereopan_right[ht->ht_defstereo];
  ht->ht_mixgain         = (defgain[ht->ht_defstereo]*256)/100;
  
  if( ht->ht_Restart >= ht->ht_PositionNr )
    ht->ht_Restart = ht->ht_PositionNr-1;

  // Do some validation  
  if( ( ht->ht_PositionNr > 1000 ) ||
      ( ht->ht_TrackLength > 64 ) ||
      ( ht->ht_InstrumentNr > 64 ) )
  {
    printf( "%d,%d,%d\n", ht->ht_PositionNr,
                          ht->ht_TrackLength,
                          ht->ht_InstrumentNr );
    free( ht );
    free( buf );
    printf( "Invalid file.\n" );
    return NULL;
  }

  strncpy( ht->ht_Name, (TEXT *)&buf[(buf[4]<<8)|buf[5]], 128 );
  nptr = (TEXT *)&buf[((buf[4]<<8)|buf[5])+strlen( ht->ht_Name )+1];

  bptr = &buf[14];
  
  // Subsongs
  for( i=0; i<ht->ht_SubsongNr; i++ )
  {
    ht->ht_Subsongs[i] = (bptr[0]<<8)|bptr[1];
    if( ht->ht_Subsongs[i] >= ht->ht_PositionNr )
      ht->ht_Subsongs[i] = 0;
    bptr += 2;
  }
  
  // Position list
  for( i=0; i<ht->ht_PositionNr; i++ )
  {
    for( j=0; j<4; j++ )
    {
      ht->ht_Positions[i].pos_Track[j]     = *bptr++;
      ht->ht_Positions[i].pos_Transpose[j] = *(int8 *)bptr++;
    }
  }
  
  // Tracks
  for( i=0; i<=ht->ht_TrackNr; i++ )
  {
    if( ( ( buf[6]&0x80 ) == 0x80 ) && ( i == 0 ) )
    {
      for( j=0; j<ht->ht_TrackLength; j++ )
      {
        ht->ht_Tracks[i][j].stp_Note       = 0;
        ht->ht_Tracks[i][j].stp_Instrument = 0;
        ht->ht_Tracks[i][j].stp_FX         = 0;
        ht->ht_Tracks[i][j].stp_FXParam    = 0;
        ht->ht_Tracks[i][j].stp_FXb        = 0;
        ht->ht_Tracks[i][j].stp_FXbParam   = 0;
      }
      continue;
    }
    
    for( j=0; j<ht->ht_TrackLength; j++ )
    {
      ht->ht_Tracks[i][j].stp_Note       = (bptr[0]>>2)&0x3f;
      ht->ht_Tracks[i][j].stp_Instrument = ((bptr[0]&0x3)<<4) | (bptr[1]>>4);
      ht->ht_Tracks[i][j].stp_FX         = bptr[1]&0xf;
      ht->ht_Tracks[i][j].stp_FXParam    = bptr[2];
      ht->ht_Tracks[i][j].stp_FXb        = 0;
      ht->ht_Tracks[i][j].stp_FXbParam   = 0;
      bptr += 3;
    }
  }
  
  // Instruments
  for( i=1; i<=ht->ht_InstrumentNr; i++ )
  {
    if( nptr < (TEXT *)(buf+buflen) )
    {
      strncpy( ht->ht_Instruments[i].ins_Name, nptr, 128 );
      nptr += strlen( nptr )+1;
    } else {
      ht->ht_Instruments[i].ins_Name[0] = 0;
    }
    
    ht->ht_Instruments[i].ins_Volume      = bptr[0];
    ht->ht_Instruments[i].ins_FilterSpeed = ((bptr[1]>>3)&0x1f)|((bptr[12]>>2)&0x20);
    ht->ht_Instruments[i].ins_WaveLength  = bptr[1]&0x07;

    ht->ht_Instruments[i].ins_Envelope.aFrames = bptr[2];
    ht->ht_Instruments[i].ins_Envelope.aVolume = bptr[3];
    ht->ht_Instruments[i].ins_Envelope.dFrames = bptr[4];
    ht->ht_Instruments[i].ins_Envelope.dVolume = bptr[5];
    ht->ht_Instruments[i].ins_Envelope.sFrames = bptr[6];
    ht->ht_Instruments[i].ins_Envelope.rFrames = bptr[7];
    ht->ht_Instruments[i].ins_Envelope.rVolume = bptr[8];
    
    ht->ht_Instruments[i].ins_FilterLowerLimit     = bptr[12]&0x7f;
    ht->ht_Instruments[i].ins_VibratoDelay         = bptr[13];
    ht->ht_Instruments[i].ins_HardCutReleaseFrames = (bptr[14]>>4)&0x07;
    ht->ht_Instruments[i].ins_HardCutRelease       = bptr[14]&0x80?1:0;
    ht->ht_Instruments[i].ins_VibratoDepth         = bptr[14]&0x0f;
    ht->ht_Instruments[i].ins_VibratoSpeed         = bptr[15];
    ht->ht_Instruments[i].ins_SquareLowerLimit     = bptr[16];
    ht->ht_Instruments[i].ins_SquareUpperLimit     = bptr[17];
    ht->ht_Instruments[i].ins_SquareSpeed          = bptr[18];
    ht->ht_Instruments[i].ins_FilterUpperLimit     = bptr[19]&0x3f;
    ht->ht_Instruments[i].ins_PList.pls_Speed      = bptr[20];
    ht->ht_Instruments[i].ins_PList.pls_Length     = bptr[21];
    
    ht->ht_Instruments[i].ins_PList.pls_Entries    = ple;
    ple += bptr[21];
    
    bptr += 22;
    for( j=0; j<ht->ht_Instruments[i].ins_PList.pls_Length; j++ )
    {
      k = (bptr[0]>>5)&7;
      if( k == 6 ) k = 12;
      if( k == 7 ) k = 15;
      l = (bptr[0]>>2)&7;
      if( l == 6 ) l = 12;
      if( l == 7 ) l = 15;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[1]      = k;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[0]      = l;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Waveform   = ((bptr[0]<<1)&6) | (bptr[1]>>7);
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Fixed      = (bptr[1]>>6)&1;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Note       = bptr[1]&0x3f;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] = bptr[2];
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] = bptr[3];

      // 1.6: Strip "toggle filter" commands if the module is
      //      version 0 (pre-filters). This is what AHX also does.
      if( ( buf[3] == 0 ) && ( l == 4 ) && ( (bptr[2]&0xf0) != 0 ) )
        ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] &= 0x0f;
      if( ( buf[3] == 0 ) && ( k == 4 ) && ( (bptr[3]&0xf0) != 0 ) )
        ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] &= 0x0f;

      bptr += 4;
    }
  }
  
  hvl_InitSubsong( ht, 0 );
  free( buf );
  return ht;
}

struct hvl_tune *hvl_LoadTune( TEXT *name, uint32 freq, uint32 defstereo )
{
  struct hvl_tune *ht;
  uint8  *buf, *bptr;
  TEXT   *nptr;
  uint32  buflen, i, j, posn, insn, ssn, chnn, hs, trkl, trkn;
  FILE *fh;
  struct  hvl_plsentry *ple;

  fh = fopen( name, "rb" );
  if( !fh )
  {
    printf( "Can't open file\n" );
    return NULL;
  }

  fseek( fh, 0, SEEK_END );
  buflen = ftell( fh );
  fseek( fh, 0, SEEK_SET );

  buf = (uint8*) malloc( buflen );
  if( !buf )
  {
    fclose( fh );
    printf( "Out of memory!\n" );
    return NULL;
  }
  
  if( fread( buf, 1, buflen, fh ) != buflen )
  {
    fclose( fh );
    free( buf );
    printf( "Unable to read from file!\n" );
    return NULL;
  }
  fclose( fh );
  
  if( ( buf[0] == 'T' ) &&
      ( buf[1] == 'H' ) &&
      ( buf[2] == 'X' ) &&
      ( buf[3] < 3 ) )
    return hvl_load_ahx( buf, buflen, defstereo, freq );

  if( ( buf[0] != 'H' ) ||
      ( buf[1] != 'V' ) ||
      ( buf[2] != 'L' ) ||
      ( buf[3] > 1 ) )
  {
    free( buf );
    printf( "Invalid file.\n" );
    return NULL;
  }
	
  
  posn = ((buf[6]&0x0f)<<8)|buf[7];
  insn = buf[12];
  ssn  = buf[13];
  chnn = (buf[8]>>2)+4;
  trkl = buf[10];
  trkn = buf[11];

  hs  = sizeof( struct hvl_tune );
  hs += sizeof( struct hvl_position ) * posn;
  hs += sizeof( struct hvl_instrument ) * (insn+1);
  hs += sizeof( uint16 ) * ssn;

  // Calculate the size of all instrument PList buffers
  bptr = &buf[16];
  bptr += ssn*2;       // Skip past the subsong list
  bptr += posn*chnn*2; // Skip past the positions
  
  // Skip past the tracks
  // 1.4: Fixed two really stupid bugs that cancelled each other
  //      out if the module had a blank first track (which is how
  //      come they were missed.
  for( i=((buf[6]&0x80)==0x80)?1:0; i<=trkn; i++ )
    for( j=0; j<trkl; j++ )
    {
      if( bptr[0] == 0x3f )
      {
        bptr++;
        continue;
      }
      bptr += 5;
    }
  
  // *NOW* we can finally calculate PList space
  for( i=1; i<=insn; i++ )
  {
    hs += bptr[21] * sizeof( struct hvl_plsentry );
    bptr += 22 + bptr[21]*5;
  }
  
  ht = (hvl_tune*) malloc( hs );    
  if( !ht )
  {
    free( buf );
    printf( "Out of memory!\n" );
    return NULL;
  }
  ht->ht_ModType		 = 1; //HVL
  ht->ht_Version         = buf[3]; // 1.5
  ht->ht_Frequency       = freq;
  ht->ht_FreqF           = (float64)freq;
  
  ht->ht_Positions       = (struct hvl_position *)(&ht[1]);
  ht->ht_Instruments     = (struct hvl_instrument *)(&ht->ht_Positions[posn]);
  ht->ht_Subsongs        = (uint16 *)(&ht->ht_Instruments[(insn+1)]);
  ple                    = (struct hvl_plsentry *)(&ht->ht_Subsongs[ssn]);

  ht->ht_WaveformTab[0]  = &waves[WO_TRIANGLE_04];
  ht->ht_WaveformTab[1]  = &waves[WO_SAWTOOTH_04];
  ht->ht_WaveformTab[3]  = &waves[WO_WHITENOISE];

  ht->ht_PositionNr      = posn;
  ht->ht_Channels        = (buf[8]>>2)+4;
  ht->ht_Restart         = ((buf[8]&3)<<8)|buf[9];
  ht->ht_SpeedMultiplier = ((buf[6]>>5)&3)+1;
  ht->ht_TrackLength     = buf[10];
  ht->ht_TrackNr         = buf[11];
  ht->ht_InstrumentNr    = insn;
  ht->ht_SubsongNr       = ssn;
  ht->ht_mixgain         = (buf[14]<<8)/100;
  ht->ht_defstereo       = buf[15];
  ht->ht_defpanleft      = stereopan_left[ht->ht_defstereo];
  ht->ht_defpanright     = stereopan_right[ht->ht_defstereo];
  
  if( ht->ht_Restart >= ht->ht_PositionNr )
    ht->ht_Restart = ht->ht_PositionNr-1;

  // Do some validation  
  if( ( ht->ht_PositionNr > 1000 ) ||
      ( ht->ht_TrackLength > 64 ) ||
      ( ht->ht_InstrumentNr > 64 ) )
  {
    printf( "%d,%d,%d\n", ht->ht_PositionNr,
                          ht->ht_TrackLength,
                          ht->ht_InstrumentNr );
    free( ht );
    free( buf );
    printf( "Invalid file.\n" );
    return NULL;
  }

  strncpy( ht->ht_Name, (TEXT *)&buf[(buf[4]<<8)|buf[5]], 128 );
  nptr = (TEXT *)&buf[((buf[4]<<8)|buf[5])+strlen( ht->ht_Name )+1];

  bptr = &buf[16];
  
  // Subsongs
  for( i=0; i<ht->ht_SubsongNr; i++ )
  {
    ht->ht_Subsongs[i] = (bptr[0]<<8)|bptr[1];
    bptr += 2;
  }
  
  // Position list
  for( i=0; i<ht->ht_PositionNr; i++ )
  {
    for( j=0; j<ht->ht_Channels; j++ )
    {
      ht->ht_Positions[i].pos_Track[j]     = *bptr++;
      ht->ht_Positions[i].pos_Transpose[j] = *(int8 *)bptr++;
    }
  }
  
  // Tracks
  for( i=0; i<=ht->ht_TrackNr; i++ )
  {
    if( ( ( buf[6]&0x80 ) == 0x80 ) && ( i == 0 ) )
    {
      for( j=0; j<ht->ht_TrackLength; j++ )
      {
        ht->ht_Tracks[i][j].stp_Note       = 0;
        ht->ht_Tracks[i][j].stp_Instrument = 0;
        ht->ht_Tracks[i][j].stp_FX         = 0;
        ht->ht_Tracks[i][j].stp_FXParam    = 0;
        ht->ht_Tracks[i][j].stp_FXb        = 0;
        ht->ht_Tracks[i][j].stp_FXbParam   = 0;
      }
      continue;
    }
    
    for( j=0; j<ht->ht_TrackLength; j++ )
    {
      if( bptr[0] == 0x3f )
      {
        ht->ht_Tracks[i][j].stp_Note       = 0;
        ht->ht_Tracks[i][j].stp_Instrument = 0;
        ht->ht_Tracks[i][j].stp_FX         = 0;
        ht->ht_Tracks[i][j].stp_FXParam    = 0;
        ht->ht_Tracks[i][j].stp_FXb        = 0;
        ht->ht_Tracks[i][j].stp_FXbParam   = 0;
        bptr++;
        continue;
      }
      
      ht->ht_Tracks[i][j].stp_Note       = bptr[0];
      ht->ht_Tracks[i][j].stp_Instrument = bptr[1];
      ht->ht_Tracks[i][j].stp_FX         = bptr[2]>>4;
      ht->ht_Tracks[i][j].stp_FXParam    = bptr[3];
      ht->ht_Tracks[i][j].stp_FXb        = bptr[2]&0xf;
      ht->ht_Tracks[i][j].stp_FXbParam   = bptr[4];
      bptr += 5;
    }
  }
  
  
  // Instruments
  for( i=1; i<=ht->ht_InstrumentNr; i++ )
  {
    if( nptr < (TEXT *)(buf+buflen) )
    {
      strncpy( ht->ht_Instruments[i].ins_Name, nptr, 128 );
      nptr += strlen( nptr )+1;
    } else {
      ht->ht_Instruments[i].ins_Name[0] = 0;
    }
    
    ht->ht_Instruments[i].ins_Volume      = bptr[0];
    ht->ht_Instruments[i].ins_FilterSpeed = ((bptr[1]>>3)&0x1f)|((bptr[12]>>2)&0x20);
    ht->ht_Instruments[i].ins_WaveLength  = bptr[1]&0x07;

    ht->ht_Instruments[i].ins_Envelope.aFrames = bptr[2];
    ht->ht_Instruments[i].ins_Envelope.aVolume = bptr[3];
    ht->ht_Instruments[i].ins_Envelope.dFrames = bptr[4];
    ht->ht_Instruments[i].ins_Envelope.dVolume = bptr[5];
    ht->ht_Instruments[i].ins_Envelope.sFrames = bptr[6];
    ht->ht_Instruments[i].ins_Envelope.rFrames = bptr[7];
    ht->ht_Instruments[i].ins_Envelope.rVolume = bptr[8];
    
    ht->ht_Instruments[i].ins_FilterLowerLimit     = bptr[12]&0x7f;
    ht->ht_Instruments[i].ins_VibratoDelay         = bptr[13];
    ht->ht_Instruments[i].ins_HardCutReleaseFrames = (bptr[14]>>4)&0x07;
    ht->ht_Instruments[i].ins_HardCutRelease       = bptr[14]&0x80?1:0;
    ht->ht_Instruments[i].ins_VibratoDepth         = bptr[14]&0x0f;
    ht->ht_Instruments[i].ins_VibratoSpeed         = bptr[15];
    ht->ht_Instruments[i].ins_SquareLowerLimit     = bptr[16];
    ht->ht_Instruments[i].ins_SquareUpperLimit     = bptr[17];
    ht->ht_Instruments[i].ins_SquareSpeed          = bptr[18];
    ht->ht_Instruments[i].ins_FilterUpperLimit     = bptr[19]&0x3f;
    ht->ht_Instruments[i].ins_PList.pls_Speed      = bptr[20];
    ht->ht_Instruments[i].ins_PList.pls_Length     = bptr[21];
    
    ht->ht_Instruments[i].ins_PList.pls_Entries    = ple;
    ple += bptr[21];
    
    bptr += 22;
    for( j=0; j<ht->ht_Instruments[i].ins_PList.pls_Length; j++ )
    {
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[0] = bptr[0]&0xf;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FX[1] = (bptr[1]>>3)&0xf;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Waveform = bptr[1]&7;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Fixed = (bptr[2]>>6)&1;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_Note  = bptr[2]&0x3f;
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[0] = bptr[3];
      ht->ht_Instruments[i].ins_PList.pls_Entries[j].ple_FXParam[1] = bptr[4];
      bptr += 5;
    }
  }
  
  hvl_InitSubsong( ht, 0 );
  free( buf );
  return ht;
}

void hvl_FreeTune( struct hvl_tune *ht )
{
  if( !ht ) return;
  free( ht );
}

void hvl_process_stepfx_1( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam )
{
  switch( FX )
  {
    case 0x0:  // Position Jump HI
      if( ((FXParam&0x0f) > 0) && ((FXParam&0x0f) <= 9) )
        ht->ht_PosJump = FXParam & 0xf;
      break;

    case 0x5:  // Volume Slide + Tone Portamento
    case 0xa:  // Volume Slide
      voice->vc_VolumeSlideDown = FXParam & 0x0f;
      voice->vc_VolumeSlideUp   = FXParam >> 4;
      break;
    
    case 0x7:  // Panning
      if( FXParam > 127 )
        FXParam -= 256;
      voice->vc_Pan          = (FXParam+128);
      voice->vc_SetPan       = (FXParam+128); // 1.4
      voice->vc_PanMultLeft  = panning_left[voice->vc_Pan];
      voice->vc_PanMultRight = panning_right[voice->vc_Pan];
      break;
    
    case 0xb: // Position jump
      ht->ht_PosJump      = ht->ht_PosJump*100 + (FXParam & 0x0f) + (FXParam >> 4)*10;
      ht->ht_PatternBreak = 1;
      if( ht->ht_PosJump <= ht->ht_PosNr )
        ht->ht_SongEndReached = 1;
      break;
    
    case 0xd: // Pattern break
      ht->ht_PosJump      = ht->ht_PosNr+1;
      ht->ht_PosJumpNote  = (FXParam & 0x0f) + (FXParam>>4)*10;
      ht->ht_PatternBreak = 1;
      if( ht->ht_PosJumpNote >  ht->ht_TrackLength )
        ht->ht_PosJumpNote = 0;
      break;
    
    case 0xe: // Extended commands
      switch( FXParam >> 4 )
      {
        case 0xc: // Note cut
          if( (FXParam & 0x0f) < ht->ht_Tempo )
          {
            voice->vc_NoteCutWait = FXParam & 0x0f;
            if( voice->vc_NoteCutWait )
            {
              voice->vc_NoteCutOn      = 1;
              voice->vc_HardCutRelease = 0;
            }
          }
          break;
          
          // 1.6: 0xd case removed
      }
      break;
    
    case 0xf: // Speed
      ht->ht_Tempo = FXParam;
      if( FXParam == 0 )
        ht->ht_SongEndReached = 1;
      break;
  }  
}

void hvl_process_stepfx_2( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam, int32 *Note )
{
  switch( FX )
  {
    case 0x9: // Set squarewave offset
      voice->vc_SquarePos    = FXParam >> (5 - voice->vc_WaveLength);
      voice->vc_PlantSquare  = 1;
      voice->vc_IgnoreSquare = 1;
      break;
    
    case 0x5: // Tone portamento + volume slide
    case 0x3: // Tone portamento
      if( FXParam != 0 ) voice->vc_PeriodSlideSpeed = FXParam;
      
      if( *Note )
      {
        int32 _new, diff;

        _new   = period_tab[*Note];
        diff  = period_tab[voice->vc_TrackPeriod];
        diff -= _new;
        _new   = diff + voice->vc_PeriodSlidePeriod;
        
        if( _new )
          voice->vc_PeriodSlideLimit = -diff;
      }
      voice->vc_PeriodSlideOn        = 1;
      voice->vc_PeriodSlideWithLimit = 1;
      *Note = 0;
      break;      
  } 
}

void hvl_process_stepfx_3( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam )
{
  int32 i;
  
  switch( FX )
  {
    case 0x01: // Portamento up (period slide down)
      voice->vc_PeriodSlideSpeed     = -FXParam;
      voice->vc_PeriodSlideOn        = 1;
      voice->vc_PeriodSlideWithLimit = 0;
      break;
    case 0x02: // Portamento down
      voice->vc_PeriodSlideSpeed     = FXParam;
      voice->vc_PeriodSlideOn        = 1;
      voice->vc_PeriodSlideWithLimit = 0;
      break;
    case 0x04: // Filter override
      if( ( FXParam == 0 ) || ( FXParam == 0x40 ) ) break;
      if( FXParam < 0x40 )
      {
        voice->vc_IgnoreFilter = FXParam;
        break;
      }
      if( FXParam > 0x7f ) break;
      voice->vc_FilterPos = FXParam - 0x40;
      break;
    case 0x0c: // Volume
      FXParam &= 0xff;
      if( FXParam <= 0x40 )
      {
        voice->vc_NoteMaxVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0x50) < 0 ) break;  // 1.6

      if( FXParam <= 0x40 )
      {
        for( i=0; i<ht->ht_Channels; i++ )
          ht->ht_Voices[i].vc_TrackMasterVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0xa0-0x50) < 0 ) break; // 1.6

      if( FXParam <= 0x40 )
        voice->vc_TrackMasterVolume = FXParam;
      break;

    case 0xe: // Extended commands;
      switch( FXParam >> 4 )
      {
        case 0x1: // Fineslide up
          voice->vc_PeriodSlidePeriod = -(FXParam & 0x0f);
          voice->vc_PlantPeriod = 1;
          break;
        
        case 0x2: // Fineslide down
          voice->vc_PeriodSlidePeriod = (FXParam & 0x0f);
          voice->vc_PlantPeriod = 1;
          break;
        
        case 0x4: // Vibrato control
          voice->vc_VibratoDepth = FXParam & 0x0f;
          break;
        
        case 0x0a: // Fine volume up
          voice->vc_NoteMaxVolume += FXParam & 0x0f;
          
          if( voice->vc_NoteMaxVolume > 0x40 )
            voice->vc_NoteMaxVolume = 0x40;
          break;
        
        case 0x0b: // Fine volume down
          voice->vc_NoteMaxVolume -= FXParam & 0x0f;
          
          if( voice->vc_NoteMaxVolume < 0 )
            voice->vc_NoteMaxVolume = 0;
          break;
        
        case 0x0f: // Misc flags (1.5)
          if( ht->ht_Version < 1 ) break;
          switch( FXParam & 0xf )
          {
            case 1:
              voice->vc_OverrideTranspose = voice->vc_Transpose;
              break;
          }
          break;
      } 
      break;
  }
}

void hvl_process_step( struct hvl_tune *ht, struct hvl_voice *voice )
{
  int32  Note, Instr, donenotedel;
  struct hvl_step *Step;
  
  if( voice->vc_TrackOn == 0 )
    return;
  
  voice->vc_VolumeSlideUp = voice->vc_VolumeSlideDown = 0;
  
  Step = &ht->ht_Tracks[ht->ht_Positions[ht->ht_PosNr].pos_Track[voice->vc_VoiceNum]][ht->ht_NoteNr];
  
  Note    = Step->stp_Note;
  Instr   = Step->stp_Instrument;
  
  // --------- 1.6: from here --------------

  donenotedel = 0;

  // Do notedelay here
  if( ((Step->stp_FX&0xf)==0xe) && ((Step->stp_FXParam&0xf0)==0xd0) )
  {
    if( voice->vc_NoteDelayOn )
    {
      voice->vc_NoteDelayOn = 0;
      donenotedel = 1;
    } else {
      if( (Step->stp_FXParam&0x0f) < ht->ht_Tempo )
      {
        voice->vc_NoteDelayWait = Step->stp_FXParam & 0x0f;
        if( voice->vc_NoteDelayWait )
        {
          voice->vc_NoteDelayOn = 1;
          return;
        }
      }
    }
  }

  if( (donenotedel==0) && ((Step->stp_FXb&0xf)==0xe) && ((Step->stp_FXbParam&0xf0)==0xd0) )
  {
    if( voice->vc_NoteDelayOn )
    {
      voice->vc_NoteDelayOn = 0;
    } else {
      if( (Step->stp_FXbParam&0x0f) < ht->ht_Tempo )
      {
        voice->vc_NoteDelayWait = Step->stp_FXbParam & 0x0f;
        if( voice->vc_NoteDelayWait )
        {
          voice->vc_NoteDelayOn = 1;
          return;
        }
      }
    }
  }

  // --------- 1.6: to here --------------

  if( Note ) voice->vc_OverrideTranspose = 1000; // 1.5

  hvl_process_stepfx_1( ht, voice, Step->stp_FX&0xf,  Step->stp_FXParam );  
  hvl_process_stepfx_1( ht, voice, Step->stp_FXb&0xf, Step->stp_FXbParam );
  
  if( ( Instr ) && ( Instr <= ht->ht_InstrumentNr ) )
  {
    struct hvl_instrument *Ins;
    int16  SquareLower, SquareUpper, d6, d3, d4;
    
    /* 1.4: Reset panning to last set position */
    voice->vc_Pan          = voice->vc_SetPan;
    voice->vc_PanMultLeft  = panning_left[voice->vc_Pan];
    voice->vc_PanMultRight = panning_right[voice->vc_Pan];

    voice->vc_PeriodSlideSpeed = voice->vc_PeriodSlidePeriod = voice->vc_PeriodSlideLimit = 0;

    voice->vc_PerfSubVolume    = 0x40;
    voice->vc_ADSRVolume       = 0;
    voice->vc_Instrument       = Ins = &ht->ht_Instruments[Instr];
    voice->vc_SamplePos        = 0;
    
    voice->vc_ADSR.aFrames     = Ins->ins_Envelope.aFrames;
    voice->vc_ADSR.aVolume     = Ins->ins_Envelope.aVolume*256/voice->vc_ADSR.aFrames;
    voice->vc_ADSR.dFrames     = Ins->ins_Envelope.dFrames;
    voice->vc_ADSR.dVolume     = (Ins->ins_Envelope.dVolume-Ins->ins_Envelope.aVolume)*256/voice->vc_ADSR.dFrames;
    voice->vc_ADSR.sFrames     = Ins->ins_Envelope.sFrames;
    voice->vc_ADSR.rFrames     = Ins->ins_Envelope.rFrames;
    voice->vc_ADSR.rVolume     = (Ins->ins_Envelope.rVolume-Ins->ins_Envelope.dVolume)*256/voice->vc_ADSR.rFrames;
    
    voice->vc_WaveLength       = Ins->ins_WaveLength;
    voice->vc_NoteMaxVolume    = Ins->ins_Volume;
    
    voice->vc_VibratoCurrent   = 0;
    voice->vc_VibratoDelay     = Ins->ins_VibratoDelay;
    voice->vc_VibratoDepth     = Ins->ins_VibratoDepth;
    voice->vc_VibratoSpeed     = Ins->ins_VibratoSpeed;
    voice->vc_VibratoPeriod    = 0;
    
    voice->vc_HardCutRelease   = Ins->ins_HardCutRelease;
    voice->vc_HardCut          = Ins->ins_HardCutReleaseFrames;
    
    voice->vc_IgnoreSquare = voice->vc_SquareSlidingIn = 0;
    voice->vc_SquareWait   = voice->vc_SquareOn        = 0;
    
    SquareLower = Ins->ins_SquareLowerLimit >> (5 - voice->vc_WaveLength);
    SquareUpper = Ins->ins_SquareUpperLimit >> (5 - voice->vc_WaveLength);
    
    if( SquareUpper < SquareLower )
    {
      int16 t = SquareUpper;
      SquareUpper = SquareLower;
      SquareLower = t;
    }
    
    voice->vc_SquareUpperLimit = SquareUpper;
    voice->vc_SquareLowerLimit = SquareLower;
    
    voice->vc_IgnoreFilter    = voice->vc_FilterWait = voice->vc_FilterOn = 0;
    voice->vc_FilterSlidingIn = 0;

    d6 = Ins->ins_FilterSpeed;
    d3 = Ins->ins_FilterLowerLimit;
    d4 = Ins->ins_FilterUpperLimit;
    
    if( d3 & 0x80 ) d6 |= 0x20;
    if( d4 & 0x80 ) d6 |= 0x40;
    
    voice->vc_FilterSpeed = d6;
    d3 &= ~0x80;
    d4 &= ~0x80;
    
    if( d3 > d4 )
    {
      int16 t = d3;
      d3 = d4;
      d4 = t;
    }
    
    voice->vc_FilterUpperLimit = d4;
    voice->vc_FilterLowerLimit = d3;
    voice->vc_FilterPos        = 32;
    
    voice->vc_PerfWait  = voice->vc_PerfCurrent = 0;
    voice->vc_PerfSpeed = Ins->ins_PList.pls_Speed;
    voice->vc_PerfList  = &voice->vc_Instrument->ins_PList;
    
    voice->vc_RingMixSource   = NULL;   // No ring modulation
    voice->vc_RingSamplePos   = 0;
    voice->vc_RingPlantPeriod = 0;
    voice->vc_RingNewWaveform = 0;
  }
  
  voice->vc_PeriodSlideOn = 0;
  
  hvl_process_stepfx_2( ht, voice, Step->stp_FX&0xf,  Step->stp_FXParam,  &Note );  
  hvl_process_stepfx_2( ht, voice, Step->stp_FXb&0xf, Step->stp_FXbParam, &Note );

  if( Note )
  {
    voice->vc_TrackPeriod = Note;
    voice->vc_PlantPeriod = 1;
  }
  
  hvl_process_stepfx_3( ht, voice, Step->stp_FX&0xf,  Step->stp_FXParam );  
  hvl_process_stepfx_3( ht, voice, Step->stp_FXb&0xf, Step->stp_FXbParam );  
}

void hvl_plist_command_parse( struct hvl_tune *ht, struct hvl_voice *voice, int32 FX, int32 FXParam )
{
  switch( FX )
  {
    case 0:
      if( ( FXParam > 0 ) && ( FXParam < 0x40 ) )
      {
        if( voice->vc_IgnoreFilter )
        {
          voice->vc_FilterPos    = voice->vc_IgnoreFilter;
          voice->vc_IgnoreFilter = 0;
        } else {
          voice->vc_FilterPos    = FXParam;
        }
        voice->vc_NewWaveform = 1;
      }
      break;

    case 1:
      voice->vc_PeriodPerfSlideSpeed = FXParam;
      voice->vc_PeriodPerfSlideOn    = 1;
      break;

    case 2:
      voice->vc_PeriodPerfSlideSpeed = -FXParam;
      voice->vc_PeriodPerfSlideOn    = 1;
      break;
    
    case 3:
      if( voice->vc_IgnoreSquare == 0 )
        voice->vc_SquarePos = FXParam >> (5-voice->vc_WaveLength);
      else
        voice->vc_IgnoreSquare = 0;
      break;
    
    case 4:
      if( FXParam == 0 )
      {
        voice->vc_SquareInit = (voice->vc_SquareOn ^= 1);
        voice->vc_SquareSign = 1;
      } else {

        if( FXParam & 0x0f )
        {
          voice->vc_SquareInit = (voice->vc_SquareOn ^= 1);
          voice->vc_SquareSign = 1;
          if(( FXParam & 0x0f ) == 0x0f )
            voice->vc_SquareSign = -1;
        }
        
        if( FXParam & 0xf0 )
        {
          voice->vc_FilterInit = (voice->vc_FilterOn ^= 1);
          voice->vc_FilterSign = 1;
          if(( FXParam & 0xf0 ) == 0xf0 )
            voice->vc_FilterSign = -1;
        }
      }
      break;
    
    case 5:
      voice->vc_PerfCurrent = FXParam;
      break;
    
    case 7:
      // Ring modulate with triangle
      if(( FXParam >= 1 ) && ( FXParam <= 0x3C ))
      {
        voice->vc_RingBasePeriod = FXParam;
        voice->vc_RingFixedPeriod = 1;
      } else if(( FXParam >= 0x81 ) && ( FXParam <= 0xBC )) {
        voice->vc_RingBasePeriod = FXParam-0x80;
        voice->vc_RingFixedPeriod = 0;
      } else {
        voice->vc_RingBasePeriod = 0;
        voice->vc_RingFixedPeriod = 0;
        voice->vc_RingNewWaveform = 0;
        voice->vc_RingAudioSource = NULL; // turn it off
        voice->vc_RingMixSource   = NULL;
        break;
      }    
      voice->vc_RingWaveform    = 0;
      voice->vc_RingNewWaveform = 1;
      voice->vc_RingPlantPeriod = 1;
      break;
    
    case 8:  // Ring modulate with sawtooth
      if(( FXParam >= 1 ) && ( FXParam <= 0x3C ))
      {
        voice->vc_RingBasePeriod = FXParam;
        voice->vc_RingFixedPeriod = 1;
      } else if(( FXParam >= 0x81 ) && ( FXParam <= 0xBC )) {
        voice->vc_RingBasePeriod = FXParam-0x80;
        voice->vc_RingFixedPeriod = 0;
      } else {
        voice->vc_RingBasePeriod = 0;
        voice->vc_RingFixedPeriod = 0;
        voice->vc_RingNewWaveform = 0;
        voice->vc_RingAudioSource = NULL;
        voice->vc_RingMixSource   = NULL;
        break;
      }

      voice->vc_RingWaveform    = 1;
      voice->vc_RingNewWaveform = 1;
      voice->vc_RingPlantPeriod = 1;
      break;

    /* New in HivelyTracker 1.4 */    
    case 9:    
      if( FXParam > 127 )
        FXParam -= 256;
      voice->vc_Pan          = (FXParam+128);
      voice->vc_PanMultLeft  = panning_left[voice->vc_Pan];
      voice->vc_PanMultRight = panning_right[voice->vc_Pan];
      break;

    case 12:
      if( FXParam <= 0x40 )
      {
        voice->vc_NoteMaxVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0x50) < 0 ) break;

      if( FXParam <= 0x40 )
      {
        voice->vc_PerfSubVolume = FXParam;
        break;
      }
      
      if( (FXParam -= 0xa0-0x50) < 0 ) break;
      
      if( FXParam <= 0x40 )
        voice->vc_TrackMasterVolume = FXParam;
      break;
    
    case 15:
      voice->vc_PerfSpeed = voice->vc_PerfWait = FXParam;
      break;
  } 
}

void hvl_process_frame( struct hvl_tune *ht, struct hvl_voice *voice )
{
  static uint8 Offsets[] = {0x00,0x04,0x04+0x08,0x04+0x08+0x10,0x04+0x08+0x10+0x20,0x04+0x08+0x10+0x20+0x40};

  if( voice->vc_TrackOn == 0 )
    return;

  if( voice->vc_NoteDelayOn )
  {
    if( voice->vc_NoteDelayWait <= 0 )
      hvl_process_step( ht, voice );
    else
      voice->vc_NoteDelayWait--;
  }
  
  if( voice->vc_HardCut )
  {
    int32 nextinst;
    
    if( ht->ht_NoteNr+1 < ht->ht_TrackLength )
      nextinst = ht->ht_Tracks[voice->vc_Track][ht->ht_NoteNr+1].stp_Instrument;
    else
      nextinst = ht->ht_Tracks[voice->vc_NextTrack][0].stp_Instrument;
    
    if( nextinst )
    {
      int32 d1;
      
      d1 = ht->ht_Tempo - voice->vc_HardCut;
      
      if( d1 < 0 ) d1 = 0;
    
      if( !voice->vc_NoteCutOn )
      {
        voice->vc_NoteCutOn       = 1;
        voice->vc_NoteCutWait     = d1;
        voice->vc_HardCutReleaseF = -(d1-ht->ht_Tempo);
      } else {
        voice->vc_HardCut = 0;
      }
    }
  }
    
  if( voice->vc_NoteCutOn )
  {
    if( voice->vc_NoteCutWait <= 0 )
    {
      voice->vc_NoteCutOn = 0;
        
      if( voice->vc_HardCutRelease )
      {
        voice->vc_ADSR.rVolume = -(voice->vc_ADSRVolume - (voice->vc_Instrument->ins_Envelope.rVolume << 8)) / voice->vc_HardCutReleaseF;
        voice->vc_ADSR.rFrames = voice->vc_HardCutReleaseF;
        voice->vc_ADSR.aFrames = voice->vc_ADSR.dFrames = voice->vc_ADSR.sFrames = 0;
      } else {
        voice->vc_NoteMaxVolume = 0;
      }
    } else {
      voice->vc_NoteCutWait--;
    }
  }
    
  // ADSR envelope
  if( voice->vc_ADSR.aFrames )
  {
    voice->vc_ADSRVolume += voice->vc_ADSR.aVolume;
      
    if( --voice->vc_ADSR.aFrames <= 0 )
      voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.aVolume << 8;

  } else if( voice->vc_ADSR.dFrames ) {
    
    voice->vc_ADSRVolume += voice->vc_ADSR.dVolume;
      
    if( --voice->vc_ADSR.dFrames <= 0 )
      voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.dVolume << 8;
    
  } else if( voice->vc_ADSR.sFrames ) {
    
    voice->vc_ADSR.sFrames--;
    
  } else if( voice->vc_ADSR.rFrames ) {
    
    voice->vc_ADSRVolume += voice->vc_ADSR.rVolume;
    
    if( --voice->vc_ADSR.rFrames <= 0 )
      voice->vc_ADSRVolume = voice->vc_Instrument->ins_Envelope.rVolume << 8;
  }

  // VolumeSlide
  voice->vc_NoteMaxVolume = voice->vc_NoteMaxVolume + voice->vc_VolumeSlideUp - voice->vc_VolumeSlideDown;

  if( voice->vc_NoteMaxVolume < 0 )
    voice->vc_NoteMaxVolume = 0;
  else if( voice->vc_NoteMaxVolume > 0x40 )
    voice->vc_NoteMaxVolume = 0x40;

  // Portamento
  if( voice->vc_PeriodSlideOn )
  {
    if( voice->vc_PeriodSlideWithLimit )
    {
      int32  d0, d2;
      
      d0 = voice->vc_PeriodSlidePeriod - voice->vc_PeriodSlideLimit;
      d2 = voice->vc_PeriodSlideSpeed;
      
      if( d0 > 0 )
        d2 = -d2;
      
      if( d0 )
      {
        int32 d3;
         
        d3 = (d0 + d2) ^ d0;
        
        if( d3 >= 0 )
          d0 = voice->vc_PeriodSlidePeriod + d2;
        else
          d0 = voice->vc_PeriodSlideLimit;
        
        voice->vc_PeriodSlidePeriod = d0;
        voice->vc_PlantPeriod = 1;
      }
    } else {
      voice->vc_PeriodSlidePeriod += voice->vc_PeriodSlideSpeed;
      voice->vc_PlantPeriod = 1;
    }
  }
  
  // Vibrato
  if( voice->vc_VibratoDepth )
  {
    if( voice->vc_VibratoDelay <= 0 )
    {
      voice->vc_VibratoPeriod = (vib_tab[voice->vc_VibratoCurrent] * voice->vc_VibratoDepth) >> 7;
      voice->vc_PlantPeriod = 1;
      voice->vc_VibratoCurrent = (voice->vc_VibratoCurrent + voice->vc_VibratoSpeed) & 0x3f;
    } else {
      voice->vc_VibratoDelay--;
    }
  }
  
  // PList
  if( voice->vc_PerfList != 0 )
  {
    if( voice->vc_Instrument && voice->vc_PerfCurrent < voice->vc_Instrument->ins_PList.pls_Length )
    {
      if( --voice->vc_PerfWait <= 0 )
      {
        uint32 i;
        int32 cur;
        
        cur = voice->vc_PerfCurrent++;
        voice->vc_PerfWait = voice->vc_PerfSpeed;
        
        if( voice->vc_PerfList->pls_Entries[cur].ple_Waveform )
        {
          voice->vc_Waveform             = voice->vc_PerfList->pls_Entries[cur].ple_Waveform-1;
          voice->vc_NewWaveform          = 1;
          voice->vc_PeriodPerfSlideSpeed = voice->vc_PeriodPerfSlidePeriod = 0;
        }
        
        // Holdwave
        voice->vc_PeriodPerfSlideOn = 0;
        
        for( i=0; i<2; i++ )
          hvl_plist_command_parse( ht, voice, voice->vc_PerfList->pls_Entries[cur].ple_FX[i]&0xff, voice->vc_PerfList->pls_Entries[cur].ple_FXParam[i]&0xff );
        
        // GetNote
        if( voice->vc_PerfList->pls_Entries[cur].ple_Note )
        {
          voice->vc_InstrPeriod = voice->vc_PerfList->pls_Entries[cur].ple_Note;
          voice->vc_PlantPeriod = 1;
          voice->vc_FixedNote   = voice->vc_PerfList->pls_Entries[cur].ple_Fixed;
        }
      }
    } else {
      if( voice->vc_PerfWait )
        voice->vc_PerfWait--;
      else
        voice->vc_PeriodPerfSlideSpeed = 0;
    }
  }
  
  // PerfPortamento
  if( voice->vc_PeriodPerfSlideOn )
  {
    voice->vc_PeriodPerfSlidePeriod -= voice->vc_PeriodPerfSlideSpeed;
    
    if( voice->vc_PeriodPerfSlidePeriod )
      voice->vc_PlantPeriod = 1;
  }
  
  if( voice->vc_Waveform == 3-1 && voice->vc_SquareOn )
  {
    if( --voice->vc_SquareWait <= 0 )
    {
      int32 d1, d2, d3;
      
      d1 = voice->vc_SquareLowerLimit;
      d2 = voice->vc_SquareUpperLimit;
      d3 = voice->vc_SquarePos;
      
      if( voice->vc_SquareInit )
      {
        voice->vc_SquareInit = 0;
        
        if( d3 <= d1 )
        {
          voice->vc_SquareSlidingIn = 1;
          voice->vc_SquareSign = 1;
        } else if( d3 >= d2 ) {
          voice->vc_SquareSlidingIn = 1;
          voice->vc_SquareSign = -1;
        }
      }
      
      // NoSquareInit
      if( d1 == d3 || d2 == d3 )
      {
        if( voice->vc_SquareSlidingIn )
          voice->vc_SquareSlidingIn = 0;
        else
          voice->vc_SquareSign = -voice->vc_SquareSign;
      }
      
      d3 += voice->vc_SquareSign;
      voice->vc_SquarePos   = d3;
      voice->vc_PlantSquare = 1;
      voice->vc_SquareWait  = voice->vc_Instrument->ins_SquareSpeed;
    }
  }
  
  if( voice->vc_FilterOn && --voice->vc_FilterWait <= 0 )
  {
    uint32 i, FMax;
    int32 d1, d2, d3;
    
    d1 = voice->vc_FilterLowerLimit;
    d2 = voice->vc_FilterUpperLimit;
    d3 = voice->vc_FilterPos;
    
    if( voice->vc_FilterInit )
    {
      voice->vc_FilterInit = 0;
      if( d3 <= d1 )
      {
        voice->vc_FilterSlidingIn = 1;
        voice->vc_FilterSign      = 1;
      } else if( d3 >= d2 ) {
        voice->vc_FilterSlidingIn = 1;
        voice->vc_FilterSign      = -1;
      }
    }
    
    // NoFilterInit
    FMax = (voice->vc_FilterSpeed < 3) ? (5-voice->vc_FilterSpeed) : 1;

    for( i=0; i<FMax; i++ )
    {
      if( ( d1 == d3 ) || ( d2 == d3 ) )
      {
        if( voice->vc_FilterSlidingIn )
          voice->vc_FilterSlidingIn = 0;
        else
          voice->vc_FilterSign = -voice->vc_FilterSign;
      }
      d3 += voice->vc_FilterSign;
    }
    
    if( d3 < 1 )  d3 = 1;
    if( d3 > 63 ) d3 = 63;
    voice->vc_FilterPos   = d3;
    voice->vc_NewWaveform = 1;
    voice->vc_FilterWait  = voice->vc_FilterSpeed - 3;
    
    if( voice->vc_FilterWait < 1 )
      voice->vc_FilterWait = 1;
  }

  if( voice->vc_Waveform == 3-1 || voice->vc_PlantSquare )
  {
    // CalcSquare
    uint32  i;
    int32   Delta;
    int8   *SquarePtr;
    int32  X;
    
    SquarePtr = &waves[WO_SQUARES+(voice->vc_FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3)];
    X = voice->vc_SquarePos << (5 - voice->vc_WaveLength);
    
    if( X > 0x20 )
    {
      X = 0x40 - X;
      voice->vc_SquareReverse = 1;
    }
    
    // OkDownSquare
    if( X > 0 )
      SquarePtr += (X-1) << 7;
    
    Delta = 32 >> voice->vc_WaveLength;
    ht->ht_WaveformTab[2] = voice->vc_SquareTempBuffer;
    
    for( i=0; i<(1<<voice->vc_WaveLength)*4; i++ )
    {
      voice->vc_SquareTempBuffer[i] = *SquarePtr;
      SquarePtr += Delta;
    }
    
    voice->vc_NewWaveform = 1;
    voice->vc_Waveform    = 3-1;
    voice->vc_PlantSquare = 0;
  }
  
  if( voice->vc_Waveform == 4-1 )
    voice->vc_NewWaveform = 1;
  
  if( voice->vc_RingNewWaveform )
  {
    int8 *rasrc;
    
    if( voice->vc_RingWaveform > 1 ) voice->vc_RingWaveform = 1;
    
    rasrc = ht->ht_WaveformTab[voice->vc_RingWaveform];
    rasrc += Offsets[voice->vc_WaveLength];
    
    voice->vc_RingAudioSource = rasrc;
  }    
        
  
  if( voice->vc_NewWaveform )
  {
    int8 *AudioSource;

    AudioSource = ht->ht_WaveformTab[voice->vc_Waveform];

    if( voice->vc_Waveform != 3-1 )
      AudioSource += (voice->vc_FilterPos-0x20)*(0xfc+0xfc+0x80*0x1f+0x80+0x280*3);

    if( voice->vc_Waveform < 3-1)
    {
      // GetWLWaveformlor2
      AudioSource += Offsets[voice->vc_WaveLength];
    }

    if( voice->vc_Waveform == 4-1 )
    {
      // AddRandomMoving
      AudioSource += ( voice->vc_WNRandom & (2*0x280-1) ) & ~1;
      // GoOnRandom
      voice->vc_WNRandom += 2239384;
      voice->vc_WNRandom  = ((((voice->vc_WNRandom >> 8) | (voice->vc_WNRandom << 24)) + 782323) ^ 75) - 6735;
    }

    voice->vc_AudioSource = AudioSource;
  }
  
  // Ring modulation period calculation
  if( voice->vc_RingAudioSource )
  {
    voice->vc_RingAudioPeriod = voice->vc_RingBasePeriod;
  
    if( !(voice->vc_RingFixedPeriod) )
    {
      if( voice->vc_OverrideTranspose != 1000 )  // 1.5
        voice->vc_RingAudioPeriod += voice->vc_OverrideTranspose + voice->vc_TrackPeriod - 1;
      else
        voice->vc_RingAudioPeriod += voice->vc_Transpose + voice->vc_TrackPeriod - 1;
    }
  
    if( voice->vc_RingAudioPeriod > 5*12 )
      voice->vc_RingAudioPeriod = 5*12;
  
    if( voice->vc_RingAudioPeriod < 0 )
      voice->vc_RingAudioPeriod = 0;
  
    voice->vc_RingAudioPeriod = period_tab[voice->vc_RingAudioPeriod];

    if( !(voice->vc_RingFixedPeriod) )
      voice->vc_RingAudioPeriod += voice->vc_PeriodSlidePeriod;

    voice->vc_RingAudioPeriod += voice->vc_PeriodPerfSlidePeriod + voice->vc_VibratoPeriod;

    if( voice->vc_RingAudioPeriod > 0x0d60 )
      voice->vc_RingAudioPeriod = 0x0d60;

    if( voice->vc_RingAudioPeriod < 0x0071 )
      voice->vc_RingAudioPeriod = 0x0071;
  }
  
  // Normal period calculation
  voice->vc_AudioPeriod = voice->vc_InstrPeriod;
  
  if( !(voice->vc_FixedNote) )
  {
    if( voice->vc_OverrideTranspose != 1000 ) // 1.5
      voice->vc_AudioPeriod += voice->vc_OverrideTranspose + voice->vc_TrackPeriod - 1;
    else
      voice->vc_AudioPeriod += voice->vc_Transpose + voice->vc_TrackPeriod - 1;
  }
    
  if( voice->vc_AudioPeriod > 5*12 )
    voice->vc_AudioPeriod = 5*12;
  
  if( voice->vc_AudioPeriod < 0 )
    voice->vc_AudioPeriod = 0;
  
  voice->vc_AudioPeriod = period_tab[voice->vc_AudioPeriod];
  
  if( !(voice->vc_FixedNote) )
    voice->vc_AudioPeriod += voice->vc_PeriodSlidePeriod;

  voice->vc_AudioPeriod += voice->vc_PeriodPerfSlidePeriod + voice->vc_VibratoPeriod;    

  if( voice->vc_AudioPeriod > 0x0d60 )
    voice->vc_AudioPeriod = 0x0d60;

  if( voice->vc_AudioPeriod < 0x0071 )
    voice->vc_AudioPeriod = 0x0071;
  
  voice->vc_AudioVolume = (((((((voice->vc_ADSRVolume >> 8) * voice->vc_NoteMaxVolume) >> 6) * voice->vc_PerfSubVolume) >> 6) * voice->vc_TrackMasterVolume) >> 6);
}

void hvl_set_audio( struct hvl_voice *voice, float64 freqf )
{
  if( voice->vc_TrackOn == 0 )
  {
    voice->vc_VoiceVolume = 0;
    return;
  }
  
  voice->vc_VoiceVolume = voice->vc_AudioVolume;
  
  if( voice->vc_PlantPeriod )
  {
    float64 freq2;
    uint32  delta;
    
    voice->vc_PlantPeriod = 0;
    voice->vc_VoicePeriod = voice->vc_AudioPeriod;
    
    freq2 = Period2Freq( voice->vc_AudioPeriod );
    delta = (uint32)(freq2 / freqf);

    if( delta > (0x280<<16) ) delta -= (0x280<<16);
    if( delta == 0 ) delta = 1;
    voice->vc_Delta = delta;
  }
  
  if( voice->vc_NewWaveform )
  {
    int8 *src;
    
    src = voice->vc_AudioSource;
    
    if( voice->vc_Waveform == 4-1 )
    {
      memcpy( &voice->vc_VoiceBuffer[0], src, 0x280 );
    } else {
      uint32 i, WaveLoops;

      WaveLoops = (1 << (5 - voice->vc_WaveLength)) * 5;

      for( i=0; i<WaveLoops; i++ )
        memcpy( &voice->vc_VoiceBuffer[i*4*(1<<voice->vc_WaveLength)], src, 4*(1<<voice->vc_WaveLength) );
    }

    voice->vc_VoiceBuffer[0x280] = voice->vc_VoiceBuffer[0];
    voice->vc_MixSource          = voice->vc_VoiceBuffer;
  }

  /* Ring Modulation */
  if( voice->vc_RingPlantPeriod )
  {
    float64 freq2;
    uint32  delta;
    
    voice->vc_RingPlantPeriod = 0;
    freq2 = Period2Freq( voice->vc_RingAudioPeriod );
    delta = (uint32)(freq2 / freqf);
    
    if( delta > (0x280<<16) ) delta -= (0x280<<16);
    if( delta == 0 ) delta = 1;
    voice->vc_RingDelta = delta;
  }
  
  if( voice->vc_RingNewWaveform )
  {
    int8 *src;
    uint32 i, WaveLoops;
    
    src = voice->vc_RingAudioSource;

    WaveLoops = (1 << (5 - voice->vc_WaveLength)) * 5;

    for( i=0; i<WaveLoops; i++ )
      memcpy( &voice->vc_RingVoiceBuffer[i*4*(1<<voice->vc_WaveLength)], src, 4*(1<<voice->vc_WaveLength) );

    voice->vc_RingVoiceBuffer[0x280] = voice->vc_RingVoiceBuffer[0];
    voice->vc_RingMixSource          = voice->vc_RingVoiceBuffer;
  }
}

void hvl_play_irq( struct hvl_tune *ht )
{
  uint32 i;

  if( ht->ht_StepWaitFrames <= 0 )
  {
    if( ht->ht_GetNewPosition )
    {
      int32 nextpos = (ht->ht_PosNr+1==ht->ht_PositionNr)?0:(ht->ht_PosNr+1);

      for( i=0; i<ht->ht_Channels; i++ )
      {
        ht->ht_Voices[i].vc_Track         = ht->ht_Positions[ht->ht_PosNr].pos_Track[i];
        ht->ht_Voices[i].vc_Transpose     = ht->ht_Positions[ht->ht_PosNr].pos_Transpose[i];
        ht->ht_Voices[i].vc_NextTrack     = ht->ht_Positions[nextpos].pos_Track[i];
        ht->ht_Voices[i].vc_NextTranspose = ht->ht_Positions[nextpos].pos_Transpose[i];
      }
      ht->ht_GetNewPosition = 0;
    }
    
    for( i=0; i<ht->ht_Channels; i++ )
      hvl_process_step( ht, &ht->ht_Voices[i] );
    
    ht->ht_StepWaitFrames = ht->ht_Tempo;
  }
  
  for( i=0; i<ht->ht_Channels; i++ )
    hvl_process_frame( ht, &ht->ht_Voices[i] );

  ht->ht_PlayingTime++;
  if( ht->ht_Tempo > 0 && --ht->ht_StepWaitFrames <= 0 )
  {
    if( !ht->ht_PatternBreak )
    {
      ht->ht_NoteNr++;
      if( ht->ht_NoteNr >= ht->ht_TrackLength )
      {
        ht->ht_PosJump      = ht->ht_PosNr+1;
        ht->ht_PosJumpNote  = 0;
        ht->ht_PatternBreak = 1;
      }
    }
    
    if( ht->ht_PatternBreak )
    {
      ht->ht_PatternBreak = 0;
      ht->ht_PosNr        = ht->ht_PosJump;
      ht->ht_NoteNr       = ht->ht_PosJumpNote;
      if( ht->ht_PosNr == ht->ht_PositionNr )
      {
        ht->ht_SongEndReached = 1;
        ht->ht_PosNr          = ht->ht_Restart;
      }
      ht->ht_PosJumpNote  = 0;
      ht->ht_PosJump      = 0;

      ht->ht_GetNewPosition = 1;
    }
  }

  for( i=0; i<ht->ht_Channels; i++ )
    hvl_set_audio( &ht->ht_Voices[i], ht->ht_Frequency );
}

void hvl_mixchunk( struct hvl_tune *ht, uint32 samples, int8 *buf1, int8 *buf2, int32 bufmod )
{
  int8   *src[MAX_CHANNELS];
  int8   *rsrc[MAX_CHANNELS];
  uint32  delta[MAX_CHANNELS];
  uint32  rdelta[MAX_CHANNELS];
  int32   vol[MAX_CHANNELS];
  uint32  pos[MAX_CHANNELS];
  uint32  rpos[MAX_CHANNELS];
  uint32  cnt;
  int32   panl[MAX_CHANNELS];
  int32   panr[MAX_CHANNELS];
//  uint32  vu[MAX_CHANNELS];
  int32   a=0, b=0, j;
  uint32  i, chans, loops;
  
  chans = ht->ht_Channels;
  for( i=0; i<chans; i++ )
  {
    delta[i] = ht->ht_Voices[i].vc_Delta;
    vol[i]   = ht->ht_Voices[i].vc_VoiceVolume;
    pos[i]   = ht->ht_Voices[i].vc_SamplePos;
    src[i]   = ht->ht_Voices[i].vc_MixSource;
    panl[i]  = ht->ht_Voices[i].vc_PanMultLeft;
    panr[i]  = ht->ht_Voices[i].vc_PanMultRight;
    
    /* Ring Modulation */
    rdelta[i]= ht->ht_Voices[i].vc_RingDelta;
    rpos[i]  = ht->ht_Voices[i].vc_RingSamplePos;
    rsrc[i]  = ht->ht_Voices[i].vc_RingMixSource;
    
//    vu[i] = 0;
  }
  
  do
  {
    loops = samples;
    for( i=0; i<chans; i++ )
    {
      if( pos[i] >= (0x280 << 16)) pos[i] -= 0x280<<16;
      cnt = ((0x280<<16) - pos[i] - 1) / delta[i] + 1;
      if( cnt < loops ) loops = cnt;
      
      if( rsrc[i] )
      {
        if( rpos[i] >= (0x280<<16)) rpos[i] -= 0x280<<16;
        cnt = ((0x280<<16) - rpos[i] - 1) / rdelta[i] + 1;
        if( cnt < loops ) loops = cnt;
      }
      
    }
    
    samples -= loops;
    
    // Inner loop
    do
    {
      a=0;
      b=0;
      for( i=0; i<chans; i++ )
      {
        if( rsrc[i] )
        {
          /* Ring Modulation */
          j = ((src[i][pos[i]>>16]*rsrc[i][rpos[i]>>16])>>7)*vol[i];
          rpos[i] += rdelta[i];
        } else {
          j = src[i][pos[i]>>16]*vol[i];
        }
        
//        if( abs( j ) > vu[i] ) vu[i] = abs( j );

        a += (j * panl[i]) >> 7;
        b += (j * panr[i]) >> 7;
        pos[i] += delta[i];
      }
      
      a = (a*ht->ht_mixgain)>>8;
      b = (b*ht->ht_mixgain)>>8;
      
      *(int16 *)buf1 = a;
      *(int16 *)buf2 = b;
      
      loops--;
      
      buf1 += bufmod;
      buf2 += bufmod;
    } while( loops > 0 );
  } while( samples > 0 );

  for( i=0; i<chans; i++ )
  {
    ht->ht_Voices[i].vc_SamplePos = pos[i];
    ht->ht_Voices[i].vc_RingSamplePos = rpos[i];
//    ht->ht_Voices[i].vc_VUMeter = vu[i];
  }
}

void hvl_DecodeFrame( struct hvl_tune *ht, int8 *buf1, int8 *buf2, int32 bufmod )
{
  uint32 samples, loops;
  
  samples = ht->ht_Frequency/50/ht->ht_SpeedMultiplier;
  loops   = ht->ht_SpeedMultiplier;
  
  do
  {
    hvl_play_irq( ht );
    hvl_mixchunk( ht, samples, buf1, buf2, bufmod );
    buf1 += samples * 4;
    buf2 += samples * 4;
    loops--;
  } while( loops );
}

int hvl_GetPlayTime( struct hvl_tune *ht ) {
	int count_irq=0;
	while (ht->ht_SongEndReached==0) {
		hvl_play_irq( ht );
		count_irq++;
	};
	hvl_InitSubsong(ht,ht->ht_SongNum);
	return count_irq*1000/50/ht->ht_SpeedMultiplier;
}

int hvl_Seek( struct hvl_tune *ht, int seek_time ) {
	int count_irq=seek_time/1000*ht->ht_SpeedMultiplier*50;
	int real_time=count_irq*1000/50/ht->ht_SpeedMultiplier;
	hvl_InitSubsong(ht,ht->ht_SongNum);
	while (count_irq--) {
		hvl_play_irq( ht );
		if (ht->ht_SongEndReached) return -1;
	};
	return real_time;
}