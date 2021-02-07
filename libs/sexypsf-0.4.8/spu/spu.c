/***************************************************************************
                            spu.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2003/03/01 - linuzappz
// - libraryName changes using ALSA
//
// 2003/02/28 - Pete
// - added option for type of interpolation
// - adjusted spu irqs again (Thousant Arms, Valkyrie Profile)
// - added MONO support for MSWindows DirectSound
//
// 2003/02/20 - kode54
// - amended interpolation code, goto GOON could skip initialization of gpos and cause segfault
//
// 2003/02/19 - kode54
// - moved SPU IRQ handler and changed sample flag processing
//
// 2003/02/18 - kode54
// - moved ADSR calculation outside of the sample decode loop, somehow I doubt that
//   ADSR timing is relative to the frequency at which a sample is played... I guess
//   this remains to be seen, and I don't know whether ADSR is applied to noise channels...
//
// 2003/02/09 - kode54
// - one-shot samples now process the end block before stopping
// - in light of removing fmod hack, now processing ADSR on frequency channel as well
//
// 2003/02/08 - kode54
// - replaced easy interpolation with gaussian
// - removed fmod averaging hack
// - changed .sinc to be updated from .iRawPitch, no idea why it wasn't done this way already (<- Pete: because I sometimes fail to see the obvious, haharhar :)
//
// 2003/02/08 - linuzappz
// - small bugfix for one usleep that was 1 instead of 1000
// - added iDisStereo for no stereo (Linux)
//
// 2003/01/22 - Pete
// - added easy interpolation & small noise adjustments
//
// 2003/01/19 - Pete
// - added Neill's reverb
//
// 2003/01/12 - Pete
// - added recording window handlers
//
// 2003/01/06 - Pete
// - added Neill's ADSR timings
//
// 2002/12/28 - Pete
// - adjusted spu irq handling, fmod handling and loop handling
//
// 2002/08/14 - Pete
// - added extra reverb
//
// 2002/06/08 - linuzappz
// - SPUupdate changed for SPUasync
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#define _IN_SPU

#include "stdafx.h"
#include "externals.h"
#include "regs.h"
#include "registers.h"

#include "../driver.h"
#include "spu.h"

////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////

// psx buffer / addresses

static u16  regArea[0x200];
static u16  spuMem[256*1024];
static u8 * spuMemC;
static u8 * pSpuIrq=0;
static u8 * pSpuBuffer;

// user settings
static int             iUseReverb=2;
static int             iUseInterpolation=3; //1;
//static int             iDisStereo=0;
static int				iDownsample22k=0;

// int 			iSpuAsyncWait=0;

// MAIN infos struct for each channel

static SPUCHAN         s_chan[MAXCHAN+1];                     // channel + 1 infos (1 is security for fmod handling)
static REVERBInfo      rvb;

static u32   dwNoiseVal=1;                          // global noise generator

static u16  spuCtrl=0;                             // some vars to store psx reg infos
static u16  spuStat=0;
static u16  spuIrq=0;
static u32  spuAddr=0xffffffff;
static int  bSpuInit=0;
static int  bSPUIsOpen=0;

static const int f[5][2] = { 	{    0,  0  },
						{   60,  0  },
						{  115, -52 },
						{   98, -55 },
						{  122, -60 } 	};

static int SSumR;
static int SSumL;
static int iFMod;
static int iDSPCount;
static s16 * pS;

static int fadeinseek_start,fadeinseek_end;



void sexyPSF_setInterpolation(int interpol) {
	iUseInterpolation=interpol;
}
int sexyPSF_getInterpolation(int interpol) {
	return iUseInterpolation;
}
void sexyPSF_setReverb(int rev) {
	iUseReverb=rev;
}
int sexyPSF_getReverb(int rev) {
	return iUseReverb;
}
////////////////////////////////////////////////////////////////////////
// CODE AREA
////////////////////////////////////////////////////////////////////////

// dirty inline func includes

#include "reverb.c"
#include "adsr.c"

// Try this to increase speed.
#include "registers.c"
#include "dma.c"

unsigned long timeGetTime()
{
 struct timeval tv;
 gettimeofday(&tv, 0);                                 // well, maybe there are better ways
 return tv.tv_sec * 1000 + tv.tv_usec/1000;            // to do that, but at least it works
}

////////////////////////////////////////////////////////////////////////
// helpers for simple interpolation

//
// easy interpolation on upsampling, no special filter, just "Pete's common sense" tm
//
// instead of having n equal sample values in a row like:
//       ____
//           |____
//
// we compare the current delta change with the next delta change.
//
// if curr_delta is positive,
//
//  - and next delta is smaller (or changing direction):
//         \.
//          -__
//
//  - and next delta significant (at least twice) bigger:
//         --_
//            \.
//
//  - and next delta is nearly same:
//          \.
//           \.
//
//
// if curr_delta is negative,
//
//  - and next delta is smaller (or changing direction):
//          _--
//         /
//
//  - and next delta significant (at least twice) bigger:
//            /
//         __-
//
//  - and next delta is nearly same:
//           /
//          /
//

INLINE void InterpolateUp(SPUCHAN * pChannel)
{
 if(pChannel->SB[32]==1)                               // flag == 1? calc step and set flag... and don't change the value in this pass
  {
   const int id1=pChannel->SB[30]-pChannel->SB[29];    // curr delta to next val
   const int id2=pChannel->SB[31]-pChannel->SB[30];    // and next delta to next-next val :)

   pChannel->SB[32]=0;

   if(id1>0)                                           // curr delta positive
    {
     if(id2<id1)
      {pChannel->SB[28]=id1;pChannel->SB[32]=2;}
     else
     if(id2<(id1<<1))
      pChannel->SB[28]=(id1*pChannel->sinc)>>16L;	   // /0x10000L;
     else
      pChannel->SB[28]=(id1*pChannel->sinc)>>17L;	   // /0x20000L;
    }
   else                                                // curr delta negative
    {
     if(id2>id1)
      {pChannel->SB[28]=id1;pChannel->SB[32]=2;}
     else
     if(id2>(id1<<1))
      pChannel->SB[28]=(id1*pChannel->sinc)>>16L;	   // /0x10000L;
     else
      pChannel->SB[28]=(id1*pChannel->sinc)>>17L;	   // /0x20000L;
    }
  }
 else
 if(pChannel->SB[32]==2)                               // flag 1: calc step and set flag... and don't change the value in this pass
  {
   pChannel->SB[32]=0;

   pChannel->SB[28]=(pChannel->SB[28]*pChannel->sinc)>>17L; // /0x20000L;
   if(pChannel->sinc<=0x8000)
        pChannel->SB[29]=pChannel->SB[30]-(pChannel->SB[28]*((0x10000/pChannel->sinc)-1));
   else pChannel->SB[29]+=pChannel->SB[28];
  }
 else                                                  // no flags? add bigger val (if possible), calc smaller step, set flag1
  pChannel->SB[29]+=pChannel->SB[28];
}

//
// even easier interpolation on downsampling, also no special filter, again just "Pete's common sense" tm
//

INLINE void InterpolateDown(SPUCHAN * pChannel)
{
 if(pChannel->sinc>=0x20000L)                                // we would skip at least one val?
  {
   pChannel->SB[29]+=(pChannel->SB[30]-pChannel->SB[29])>>1; // /2;  // add easy weight
   if(pChannel->sinc>=0x30000L)                              // we would skip even more vals?
    pChannel->SB[29]+=(pChannel->SB[31]-pChannel->SB[30])>>1;// /2; // add additional next weight
  }
}

////////////////////////////////////////////////////////////////////////
// helpers for so-called "gauss interpolation"

#define gval0 (((short*)(&pChannel->SB[29]))[gpos])
#define gval(x) (((short*)(&pChannel->SB[29]))[(gpos+x)&3])

#include "gauss_i.h"

//#include "xa.c"

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// START SOUND... called by main thread to setup a new sound on a channel
////////////////////////////////////////////////////////////////////////

INLINE void StartSound(SPUCHAN *pChannel)
{
 StartADSR(pChannel);
 StartREVERB(pChannel);

 pChannel->pCurr=pChannel->pStart;                     // set sample start

 pChannel->s_1=0;                                      // init mixing vars
 pChannel->s_2=0;
 pChannel->iSBPos=28;

 pChannel->bNew=0;                                     // init channel flags
 pChannel->bStop=0;
 pChannel->bOn=1;

 pChannel->SB[29]=0;                                   // init our interpolation helpers
 pChannel->SB[30]=0;

 if(iUseInterpolation>=2)                              // gauss interpolation?
      {pChannel->spos=0x30000L;pChannel->SB[28]=0;}    // -> start with more decoding
 else {pChannel->spos=0x10000L;pChannel->SB[31]=0;}    // -> no/simple interpolation starts with one 44100 decoding
}

INLINE void VoiceChangeFrequency(SPUCHAN * pChannel)
{
 pChannel->iUsedFreq=pChannel->iActFreq;               // -> take it and calc steps
 pChannel->sinc=pChannel->iRawPitch<<4;
 if(!pChannel->sinc) pChannel->sinc=1;
 if(iUseInterpolation==1) pChannel->SB[32]=1;          // -> freq change in simle imterpolation mode: set flag
}

////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// ALL KIND OF HELPERS
////////////////////////////////////////////////////////////////////////

INLINE void FModChangeFrequency(SPUCHAN * pChannel)
{
 int NP=pChannel->iRawPitch;

 NP=((32768L+iFMod)*NP)>>15L;		   			   // / 32768L;

 if(NP>0x3fff) NP=0x3fff;
 if(NP<0x1)    NP=0x1;

 NP=(44100L*NP)>>12L;								   // /(4096L); // calc frequency

 pChannel->iActFreq=NP;
 pChannel->iUsedFreq=NP;
 pChannel->sinc=(((NP/10)<<16)/4410);
 if(!pChannel->sinc) pChannel->sinc=1;
 if(iUseInterpolation==1) pChannel->SB[32]=1;          // freq change in simple interpolation mode

 iFMod=0;
}

////////////////////////////////////////////////////////////////////////

// noise handler... just produces some noise data
// surely wrong... and no noise frequency (spuCtrl&0x3f00) will be used...
// and sometimes the noise will be used as fmod modulation... pfff

INLINE int iGetNoiseVal(SPUCHAN * pChannel)
{
 int fa;

 if((dwNoiseVal<<=1)&0x80000000L)
  {
   dwNoiseVal^=0x0040001L;
   fa=((dwNoiseVal>>2)&0x7fff);
   fa=-fa;
  }
 else fa=(dwNoiseVal>>2)&0x7fff;

 // mmm... depending on the noise freq we allow bigger/smaller changes to the previous val
 fa=pChannel->iOldNoise+((fa-pChannel->iOldNoise)/((0x001f-((spuCtrl&0x3f00)>>9))+1));
 if(fa>32767L)  fa=32767L;
 if(fa<-32767L) fa=-32767L;
 pChannel->iOldNoise=fa;

 if(iUseInterpolation<2)                               // no gauss/cubic interpolation?
  pChannel->SB[29] = fa;                               // -> store noise val in "current sample" slot
 return fa;
}

////////////////////////////////////////////////////////////////////////

INLINE void StoreInterpolationVal(SPUCHAN * pChannel,int fa)
{
 if(pChannel->bFMod==2)                                // fmod freq channel
  pChannel->SB[29]=fa;
 else
  {
   if((spuCtrl&0x4000)==0) fa=0;                       // muted?
   else                                                // else adjust
    {
     if(fa>32767L)  fa=32767L;
     if(fa<-32767L) fa=-32767L;
    }

   if(iUseInterpolation>=2)                            // gauss/cubic interpolation
    {
     int gpos = pChannel->SB[28];
     gval0 = fa;
     gpos = (gpos+1) & 3;
     pChannel->SB[28] = gpos;
    }
   else
   if(iUseInterpolation==1)                            // simple interpolation
    {
     pChannel->SB[28] = 0;
     pChannel->SB[29] = pChannel->SB[30];              // -> helpers for simple linear interpolation: delay real val for two slots, and calc the two deltas, for a 'look at the future behaviour'
     pChannel->SB[30] = pChannel->SB[31];
     pChannel->SB[31] = fa;
     pChannel->SB[32] = 1;                             // -> flag: calc new interpolation
    }
   else pChannel->SB[29]=fa;                           // no interpolation
  }
}

////////////////////////////////////////////////////////////////////////

INLINE int iGetInterpolationVal(SPUCHAN * pChannel)
{
 int fa;

 if(pChannel->bFMod==2) return pChannel->SB[29];

 switch(iUseInterpolation)
  {
   //--------------------------------------------------//
   case 3:                                             // cubic interpolation
    {
     long xd;int gpos;
     xd = ((pChannel->spos) >> 1)+1;
     gpos = pChannel->SB[28];

     fa  = gval(3) - 3*gval(2) + 3*gval(1) - gval0;
     fa *= (xd - (2<<15)) / 6;
     fa >>= 15;
     fa += gval(2) - gval(1) - gval(1) + gval0;
     fa *= (xd - (1<<15)) >> 1;
     fa >>= 15;
     fa += gval(1) - gval0;
     fa *= xd;
     fa >>= 15;
     fa = fa + gval0;

    } break;
   //--------------------------------------------------//
   case 2:                                             // gauss interpolation
    {
     int vl, vr;int gpos;
     vl = (pChannel->spos >> 6) & ~3;
     gpos = pChannel->SB[28];
     vr=(gauss[vl]*gval0)&~2047;
     vr+=(gauss[vl+1]*gval(1))&~2047;
     vr+=(gauss[vl+2]*gval(2))&~2047;
     vr+=(gauss[vl+3]*gval(3))&~2047;
     fa = vr>>11;
    } break;
   //--------------------------------------------------//
   case 1:                                             // simple interpolation
    {
     if(pChannel->sinc<0x10000L)                       // -> upsampling?
          InterpolateUp(pChannel);                     // --> interpolate up
     else InterpolateDown(pChannel);                   // --> else down
     fa=pChannel->SB[29];
    } break;
   //--------------------------------------------------//
   default:                                            // no interpolation
    {
     fa=pChannel->SB[29];
    } break;
   //--------------------------------------------------//
  }

 return fa;
}

////////////////////////////////////////////////////////////////////////
// MAIN SPU FUNCTION
// here is the main job handler... thread, timer or direct func call
// basically the whole sound processing is done in this fat func!
////////////////////////////////////////////////////////////////////////

u32 sampcount,seektime,seekupdate,seeklength,sexy_shouldstop;
static u32 decaybegin;
static u32 decayend;

// Counting to 65536 results in full volume offage.
void sexysetlength(s32 stop, s32 fade)
{
 if(stop==~0)
 {
  decaybegin=~0;
 }
 else
 {
  stop=(stop*441)/10;
  fade=(fade*441)/10;

  decaybegin=stop;
  decayend=stop+fade;
 }
}


static s32 poo;
void sexy_seek(u32 t) {
 seektime=t*441/10;
 if (seektime>sampcount) seeklength=(seektime-sampcount);
 else seeklength=seektime;
 seekupdate=0;
}

int sexySPUasync(u32 cycles)
{
 s32 temp = 0;

 poo+=cycles;
 if( poo < 384 ) return(1);

 while( poo > 384 ) {
  temp++;
  poo -= 384;
 }

 if(decaybegin == 0 && decayend == 0) return(0);

 while(temp)
 {
   if(iDSPCount) {
	  iDSPCount=0;
	  temp--;
 	  SPUasyncDummy();
	  continue;
   }

   iDSPCount=iDownsample22k?1:0;

   int s_1,s_2,fa;
   unsigned char * start;unsigned int nSample;
   register int ch,predict_nr,shift_factor,flags,d,s;
   register SPUCHAN * pChannel;

   temp--;
   //--------------------------------------------------//
   //- main channel loop                              -//
   //--------------------------------------------------//
    // {
	 pChannel=s_chan;
     for(ch=0;ch<MAXCHAN;ch++,pChannel++)                         // loop em all.
      {
       if(pChannel->bNew)
        {
         StartSound(pChannel);                         // start new sound
         // TODO - dwNewChannel&=~(1<<ch);                       // clear new channel bit
        }

	   if(!pChannel->bOn) continue;                    // channel not playing? next


       if(pChannel->iActFreq!=pChannel->iUsedFreq)     // new psx frequency?
        VoiceChangeFrequency(pChannel);

	   if(pChannel->bFMod==1 && iFMod)           // fmod freq channel
        FModChangeFrequency(pChannel);


	   while(pChannel->spos>=0x10000L)
	   {
	    if(pChannel->iSBPos==28)                    // 28 reached?
		{
		 start=pChannel->pCurr;                    // set up the current pos

		 if (start == (unsigned char*)-1)          // special "stop" sign
		 {
		  pChannel->bOn=0;                        // -> turn everything off
		  pChannel->ADSRX.lVolume=0;
		  pChannel->ADSRX.EnvelopeVol=0;
		  goto ENDX;                              // -> and done for this channel
		 }

		pChannel->iSBPos=0;

		//////////////////////////////////////////// spu irq handler here? mmm... do it later

		s_1=pChannel->s_1;
		s_2=pChannel->s_2;

		predict_nr=(int)*start;start++;
		shift_factor=predict_nr&0xf;
		predict_nr >>= 4;
		flags=(int)*start;start++;

		// -------------------------------------- //

		for (nSample=0;nSample<28;start++)
		{
		 d=(int)*start;
		 s=((d&0xf)<<12);
		 if(s&0x8000) s|=0xffff0000;

		 fa=(s >> shift_factor);
		 fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
		 s_2=s_1;s_1=fa;
		 s=((d & 0xf0) << 8);

		 pChannel->SB[nSample++]=fa;

		 if(s&0x8000) s|=0xffff0000;
		 fa=(s>>shift_factor);
		 fa=fa + ((s_1 * f[predict_nr][0])>>6) + ((s_2 * f[predict_nr][1])>>6);
		 s_2=s_1;s_1=fa;

		 pChannel->SB[nSample++]=fa;
		}

		//////////////////////////////////////////// irq check

		if(spuCtrl&0x40)         // some callback and irq active?
		{
		 if((pSpuIrq >  start-16 &&              // irq address reached?
		  pSpuIrq <= start) ||
		   ((flags&1) &&                        // special: irq on looping addr, when stop/loop flag is set
		   (pSpuIrq >  pChannel->pLoop-16 &&
		   pSpuIrq <= pChannel->pLoop)))
		 {
		  pChannel->iIrqDone=1;                 // -> debug flag
		  sexySPUirq();
		  /*irqCallback();                        // -> call main emu
		  if(iSPUIRQWait)                       // -> option: wait after irq for main emu
		  {
		   iSpuAsyncWait=1;
		   bIRQReturn=1;
		  }
		  */
		 }
		}

		//////////////////////////////////////////// flag handler

		if((flags&4) && (!pChannel->bIgnoreLoop))
		 pChannel->pLoop=start-16;                // loop adress

		if(flags&1)                               // 1: stop/loop
		{
		 // We play this block out first...
		 //if(!(flags&2))                          // 1+2: do loop... otherwise: stop
		 if(flags!=3 || pChannel->pLoop==NULL)   // PETE: if we don't check exactly for 3, loop hang ups will happen (DQ4, for example)
		 {                                      // and checking if pLoop is set avoids crashes, yeah
		  start = (unsigned char*)-1;
		 }
		 else
		 {
		  start = pChannel->pLoop;
		 }
		}

		pChannel->pCurr=start;                    // store values for next cycle
		pChannel->s_1=s_1;
		pChannel->s_2=s_2;

		////////////////////////////////////////////
	   }

	   fa=pChannel->SB[pChannel->iSBPos++];        // get sample data

	   StoreInterpolationVal(pChannel,fa);         // store val for later interpolation

	   pChannel->spos -= 0x10000L;
	  }

	  ////////////////////////////////////////////////

	  if(pChannel->bNoise)
	   fa=iGetNoiseVal(pChannel);               // get noise val
	  else fa=iGetInterpolationVal(pChannel);       // get sample val

	  pChannel->sval=(MixADSR(pChannel)*fa)>>10;	   // /1023   // mix adsr

	  if(pChannel->bFMod==2)                        // fmod freq channel
	   iFMod=pChannel->sval;                    // -> store 1T sample data, use that to do fmod on next channel
	  else                                          // no fmod freq channel
	  {
	   //////////////////////////////////////////////
	   // ok, left/right sound volume (psx volume goes from 0 ... 0x3fff)

	   if(pChannel->iMute)
	    pChannel->sval=0;                         // debug mute
	   else
	   {
	    SSumL+=(pChannel->sval*pChannel->iLeftVolume)>>14L;	// /0x4000L;
	    SSumR+=(pChannel->sval*pChannel->iRightVolume)>>14L;	// /0x4000L;
	   }

	   //////////////////////////////////////////////
	   // now let us store sound data for reverb

	   if(pChannel->bRVBActive) StoreREVERB(pChannel);
	  }

	  pChannel->spos += pChannel->sinc;
 ENDX:   ;
	}

  ///////////////////////////////////////////////////////
  // mix all channels (including reverb) into one buffer
	 
  if (seektime==-1) {

	  SSumL+=MixREVERBLeft();
	  SSumR+=MixREVERBRight();
		 
	  if(sampcount>=decaybegin) {
		  s32 dmul;
		  if(decaybegin!=~0) { // Is anyone REALLY going to be playing a song for 13 hours?
			  if(sampcount>=decayend) {//       	    ao_song_done = 1;
					 return(0);
			  }
			  dmul=256-(256*(sampcount-decaybegin)/(decayend-decaybegin));
			  SSumL=(SSumL*dmul)>>8;
			  SSumR=(SSumR*dmul)>>8;
		  }
	  }
	  if ((sampcount>=fadeinseek_start)&&(sampcount<fadeinseek_end)) {
		  s32 dmul;
		  dmul=(256*(sampcount-fadeinseek_start)/(fadeinseek_end-fadeinseek_start));
		  SSumL=(SSumL*dmul)>>8;
		  SSumR=(SSumR*dmul)>>8;
	  }
	  
	  d= SSumL;SSumL=0;
	  // d=(SSumL[ns]*volmul)>>8;SSumL[ns]=0;
	  if(d<-32767) d=-32767;if(d>32767) d=32767;
	  *pS++=d;	 
	  
	  d= SSumR;SSumR=0;
	  // d=(SSumR[ns]*volmul)>>8;SSumR[ns]=0;
	  if(d<-32767) d=-32767;if(d>32767) d=32767;
	  *pS++=d;
	  
	  InitREVERB();
  } else if (seektime==sampcount) {
	  seektime=-1; //got it
	  SSumL=SSumR=0;
	  InitREVERB();
	  fadeinseek_start=sampcount;
	  fadeinseek_end=sampcount+44100/2; //44100 samples/seconds at 44,1Khz
  } else if (seektime<sampcount) return(0); //need to reload and restart
  else if (seektime>sampcount) {//seek in progress
	  if (!seekupdate) {
		  seekupdate=128;
		  if (sexyd_updateseek(100-(seektime-sampcount)*100/seeklength)) { //stop ?
			  sexy_shouldstop=1;
			  return 0;
		  }
	  } else seekupdate--;
  }
  sampcount++;
 }

 return(1);
}

// Just do daReverb!!
int SPUasyncSeek()
{
	register int ch;
	register SPUCHAN * pChannel;
	
	pChannel=s_chan;
	for(ch=0;ch<MAXCHAN;ch++,pChannel++)                         // loop em all.
	{
		if(!pChannel->bOn) continue;                    // channel not playing? next
		if(pChannel->bRVBActive) StoreREVERB(pChannel);
		pChannel->spos += pChannel->sinc;
	}
	
	// CHECKME - To avoid saturation reduce by half?
	SSumL=(MixREVERBLeft()>>1);
	SSumR=(MixREVERBRight()>>1);
	
	InitREVERB();
	
	return(1);
}

// Just do daReverb!!
int SPUasyncDummy()
{
  register int ch;
  register SPUCHAN * pChannel;

  pChannel=s_chan;
  for(ch=0;ch<MAXCHAN;ch++,pChannel++)                         // loop em all.
  {
	if(!pChannel->bOn) continue;                    // channel not playing? next
    if(pChannel->bRVBActive) StoreREVERB(pChannel);
	pChannel->spos += pChannel->sinc;
  }

  // CHECKME - To avoid saturation reduce by half?
  SSumL=(MixREVERBLeft()>>1);
  SSumR=(MixREVERBRight()>>1);

  InitREVERB();

 return(1);
}

void sexy_stop(void)
{
 decaybegin=decayend=0;
}

void flushboot(void)
{
   if((u8*)pS>((u8*)pSpuBuffer+1024))
   {
    sexyd_update((u8*)pSpuBuffer,(u8*)pS-(u8*)pSpuBuffer);
    pS=(s16 *)pSpuBuffer;
   }
}

////////////////////////////////////////////////////////////////////////
// INIT/EXIT STUFF
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// SPUINIT: this func will be called first by the main emu
////////////////////////////////////////////////////////////////////////

int sexySPUinit(void)
{
 spuMemC=(u8*)spuMem;                      // just small setup
 memset((void *)s_chan,0,MAXCHAN*sizeof(SPUCHAN));
 memset((void *)&rvb,0,sizeof(REVERBInfo));
 memset(regArea,0,sizeof(regArea));
 memset(spuMem,0,sizeof(spuMem));
 InitADSR();
 sampcount=poo=0;
 seektime=-1;
 seekupdate=0;
 sexy_shouldstop=0;
	
 fadeinseek_start=fadeinseek_end=0;	
	
 return 0;
}

////////////////////////////////////////////////////////////////////////
// SETUPTIMER: init of certain buffers and threads/timers
////////////////////////////////////////////////////////////////////////

static void SetupTimer(void)
{
 SSumR=0;							                   // init some mixing buffers
 SSumL=0;
 iFMod=0;

 pS=(short *)pSpuBuffer;                               // setup soundbuffer pointer
 bSpuInit=1;                                           // flag: we are inited
}

////////////////////////////////////////////////////////////////////////
// REMOVETIMER: kill threads/timers
////////////////////////////////////////////////////////////////////////

static void RemoveTimer(void)
{
 bSpuInit=0;
}

////////////////////////////////////////////////////////////////////////
// SETUPSTREAMS: init most of the spu buffers
////////////////////////////////////////////////////////////////////////

static void SetupStreams(void)
{
 int i;

 pSpuBuffer=(unsigned char *)malloc(32768);            // alloc mixing buffer

 if(iDownsample22k) iDSPCount=0;

 // TODO - find sRVBStart size (88200*2)
 if(iUseReverb==1) i=44100;
 else              i=NSSIZE*2;

 sRVBStart = (int *)malloc(i*4);                       // alloc reverb buffer
 memset(sRVBStart,0,i*4);
 sRVBEnd  = sRVBStart + i;
 sRVBPlay = sRVBStart;

 /* TODO -
 XAStart =                                             // alloc xa buffer
  (unsigned long *)malloc(44100*4);
 XAPlay  = XAStart;
 XAFeed  = XAStart;
 XAEnd   = XAStart + 44100;
 */
 for(i=0;i<MAXCHAN;i++)                                // loop sound channels
  {
// we don't use mutex sync... not needed, would only
// slow us down:
//   s_chan[i].hMutex=CreateMutex(NULL,FALSE,NULL);
   s_chan[i].ADSRX.SustainLevel = 0xf<<27;             // -> init sustain
   s_chan[i].iMute=0;
   s_chan[i].iIrqDone=0;
   s_chan[i].pLoop=spuMemC;
   s_chan[i].pStart=spuMemC;
   s_chan[i].pCurr=spuMemC;
  }


}

////////////////////////////////////////////////////////////////////////
// REMOVESTREAMS: free most buffer
////////////////////////////////////////////////////////////////////////

static void RemoveStreams(void)
{
 free(pSpuBuffer);                                     // free mixing buffer
 pSpuBuffer=NULL;
 free(sRVBStart);                                      // free reverb buffer
 sRVBStart=0;
 /* TODO
 free(XAStart);                                        // free XA buffer
 XAStart=0;
 */

/*
 int i;
 for(i=0;i<MAXCHAN;i++)
  {
   WaitForSingleObject(s_chan[i].hMutex,2000);
   ReleaseMutex(s_chan[i].hMutex);
   if(s_chan[i].hMutex)
    {CloseHandle(s_chan[i].hMutex);s_chan[i].hMutex=0;}
  }
*/
}


////////////////////////////////////////////////////////////////////////
// SPUOPEN: called by main emu after init
////////////////////////////////////////////////////////////////////////

int sexySPUopen(void)
{
 if(bSPUIsOpen) return 0;                              // security for some stupid main emus


 iReverbOff=-1;
 spuIrq=0;
 spuAddr=0xffffffff;
 spuMemC=(unsigned char *)spuMem;
 memset((void *)s_chan,0,(MAXCHAN+1)*sizeof(SPUCHAN));
 pSpuIrq=0;
 //iSPUIRQWait=0;

 // TODO - ReadConfig();                                         // read user stuff

 SetupStreams();                                       // prepare streaming

 SetupTimer();                                         // timer for feeding data

 bSPUIsOpen=1;

 return 1;
}

////////////////////////////////////////////////////////////////////////

void SPUsetConfigFile(char * pCfg)
{
 // TODO - pConfigFile=pCfg;
}

////////////////////////////////////////////////////////////////////////
// SPUCLOSE: called before shutdown
////////////////////////////////////////////////////////////////////////

long sexySPUclose(void)
{
 if(!bSPUIsOpen) return 0;                             // some security

 bSPUIsOpen=0;                                         // no more open

 RemoveTimer();                                        // no more feeding

 RemoveStreams();                                      // no more streaming

 return 0;
}

////////////////////////////////////////////////////////////////////////
// SPUSHUTDOWN: called by main emu on final exit
////////////////////////////////////////////////////////////////////////

long  sexySPUshutdown(void)
{
 return 0;
}

////////////////////////////////////////////////////////////////////////
// SPUTEST: we don't test, we are always fine ;)
////////////////////////////////////////////////////////////////////////

long  sexySPUtest(void)
{
 return 0;
}

////////////////////////////////////////////////////////////////////////
// SPUCONFIGURE: call config dialog
////////////////////////////////////////////////////////////////////////

long  sexySPUconfigure(void)
{
// StartCfgTool("CFG");
 return 0;
}


