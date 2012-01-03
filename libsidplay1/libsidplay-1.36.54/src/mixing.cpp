//
// /home/ms/source/sidplay/libsidplay/emu/RCS/mixing.cpp,v
//

#include "mytypes.h"
#include "opstruct.h"
#include "samples.h"


extern sidOperator optr1, optr2, optr3;          // -> 6581_.cpp
extern uword voice4_gainLeft, voice4_gainRight;


static const int maxLogicalVoices = 4;

static const int mix8monoMiddleIndex = 256*maxLogicalVoices/2;
static ubyte mix8mono[256*maxLogicalVoices];

static const int mix8stereoMiddleIndex = 256*(maxLogicalVoices/2)/2;
static ubyte mix8stereo[256*(maxLogicalVoices/2)];

static const int mix16monoMiddleIndex = 256*maxLogicalVoices/2;
static uword mix16mono[256*maxLogicalVoices];

static const int mix16stereoMiddleIndex = 256*(maxLogicalVoices/2)/2;
static uword mix16stereo[256*(maxLogicalVoices/2)];

sbyte *signedPanMix8 = 0;
sword *signedPanMix16 = 0;

static ubyte zero8bit;   // ``zero''-sample
static uword zero16bit;  // either signed or unsigned
udword splitBufferLen;

void* fill8bitMono( void* buffer, udword numberOfSamples );
void* fill8bitMonoControl( void* buffer, udword numberOfSamples );
void* fill8bitStereo( void* buffer, udword numberOfSamples );
void* fill8bitStereoControl( void* buffer, udword numberOfSamples );
void* fill8bitStereoSurround( void* buffer, udword numberOfSamples );
void* fill8bitSplit( void* buffer, udword numberOfSamples );
void* fill16bitMono( void* buffer, udword numberOfSamples );
void* fill16bitMonoControl( void* buffer, udword numberOfSamples );
void* fill16bitStereo( void* buffer, udword numberOfSamples );
void* fill16bitStereoControl( void* buffer, udword numberOfSamples );
void* fill16bitStereoSurround( void* buffer, udword numberOfSamples );
void* fill16bitSplit( void* buffer, udword numberOfSamples );


void MixerInit(bool threeVoiceAmplify, ubyte zero8, uword zero16)
{
	zero8bit = zero8;
	zero16bit = zero16;
	
	long si;
	uword ui;
	
	long ampDiv = maxLogicalVoices;
	if (threeVoiceAmplify)
	{
		ampDiv = (maxLogicalVoices-1);
	}

	// Mixing formulas are optimized by sample input value.
	
	si = (-128*maxLogicalVoices);
	for (ui = 0; ui < sizeof(mix8mono); ui++ )
	{
		mix8mono[ui] = (ubyte)(si/ampDiv) + zero8bit;
		si++;
	}

	si = (-128*maxLogicalVoices);  // optimized by (/2 *2);
	for (ui = 0; ui < sizeof(mix8stereo); ui++ )
	{
		mix8stereo[ui] = (ubyte)(si/ampDiv) + zero8bit;
		si+=2;
	}

	si = (-128*maxLogicalVoices) * 256;
	for (ui = 0; ui < sizeof(mix16mono)/sizeof(uword); ui++ )
	{
		mix16mono[ui] = (uword)(si/ampDiv) + zero16bit;
		si+=256;
	}

	si = (-128*maxLogicalVoices) * 256;  // optimized by (/2 * 512)
	for (ui = 0; ui < sizeof(mix16stereo)/sizeof(uword); ui++ )
	{
		mix16stereo[ui] = (uword)(si/ampDiv) + zero16bit;
		si+=512;
	}
}


inline void syncEm()
{
	extern sbyte waveCalcNormal(sidOperator* pVoice);
	extern sbyte waveCalcRangeCheck(sidOperator* pVoice);
	optr1.cycleLenCount--;
	optr2.cycleLenCount--;
	optr3.cycleLenCount--;
	bool sync1 = (optr1.modulator->cycleLenCount <= 0);
	bool sync2 = (optr2.modulator->cycleLenCount <= 0);
	bool sync3 = (optr3.modulator->cycleLenCount <= 0);
	if (optr1.sync && sync1)
	{
		optr1.cycleLenCount = 0;
		optr1.outProc = &waveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr1.waveStep.l = 0;
#else
		optr1.waveStep = (optr1.waveStepPnt = 0);
#endif
	}
	if (optr2.sync && sync2)
	{
		optr2.cycleLenCount = 0;
		optr2.outProc = &waveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr2.waveStep.l = 0;
#else
		optr2.waveStep = (optr2.waveStepPnt = 0);
#endif
	}
	if (optr3.sync && sync3)
	{
		optr3.cycleLenCount = 0;
		optr3.outProc = &waveCalcNormal;
#if defined(DIRECT_FIXPOINT)
		optr3.waveStep.l = 0;
#else
		optr3.waveStep = (optr3.waveStepPnt = 0);
#endif
	}
}


//
// -------------------------------------------------------------------- 8-bit
//

void* fill8bitMono( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
	    *buffer8bit++ = mix8mono[(unsigned)(mix8monoMiddleIndex
											+(*optr1.outProc)(&optr1)
											+(*optr2.outProc)(&optr2)
											+(*optr3.outProc)(&optr3)
											+(*sampleEmuRout)())];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitMonoControl( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+(*optr1.outProc)(&optr1)]
			+signedPanMix8[optr2.gainLeft+(*optr2.outProc)(&optr2)]
			+signedPanMix8[optr3.gainLeft+(*optr3.outProc)(&optr3)]
			+signedPanMix8[voice4_gainLeft+(*sampleEmuRout)()];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitStereo( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
  	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		// left
	    *buffer8bit++ = mix8stereo[(unsigned)(mix8stereoMiddleIndex
											 +(*optr1.outProc)(&optr1)
											 +(*optr3.outProc)(&optr3))];
		// right
	    *buffer8bit++ = mix8stereo[(unsigned)(mix8stereoMiddleIndex
											 +(*optr2.outProc)(&optr2)
											 +(*sampleEmuRout)())];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitStereoControl( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
		// left
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+voice1data]
		    +signedPanMix8[optr2.gainLeft+voice2data]
			+signedPanMix8[optr3.gainLeft+voice3data]
			+signedPanMix8[voice4_gainLeft+voice4data];
		// right
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainRight+voice1data]
			+signedPanMix8[optr2.gainRight+voice2data]
			+signedPanMix8[optr3.gainRight+voice3data]
			+signedPanMix8[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitStereoSurround( void* buffer, udword numberOfSamples )
{
	ubyte* buffer8bit = (ubyte*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
		// left
		*buffer8bit++ = zero8bit
			+signedPanMix8[optr1.gainLeft+voice1data]
		    +signedPanMix8[optr2.gainLeft+voice2data]
			+signedPanMix8[optr3.gainLeft+voice3data]
			+signedPanMix8[voice4_gainLeft+voice4data];
		// right
		*buffer8bit++ = zero8bit
			-signedPanMix8[optr1.gainRight+voice1data]
			-signedPanMix8[optr2.gainRight+voice2data]
			-signedPanMix8[optr3.gainRight+voice3data]
			-signedPanMix8[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer8bit;
}

void* fill8bitSplit( void* buffer, udword numberOfSamples )
{
	ubyte* v1buffer8bit = (ubyte*)buffer;
	ubyte* v2buffer8bit = v1buffer8bit + splitBufferLen;
	ubyte* v3buffer8bit = v2buffer8bit + splitBufferLen;
	ubyte* v4buffer8bit = v3buffer8bit + splitBufferLen;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*v1buffer8bit++ = zero8bit+(*optr1.outProc)(&optr1);
		*v2buffer8bit++ = zero8bit+(*optr2.outProc)(&optr2);
		*v3buffer8bit++ = zero8bit+(*optr3.outProc)(&optr3);
		*v4buffer8bit++ = zero8bit+(*sampleEmuRout)();
		syncEm();
	}
	return v1buffer8bit;
}

//
// ------------------------------------------------------------------- 16-bit
//

void* fill16bitMono( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
	    *buffer16bit++ = mix16mono[(unsigned)(mix16monoMiddleIndex
											 +(*optr1.outProc)(&optr1)
											 +(*optr2.outProc)(&optr2)
											 +(*optr3.outProc)(&optr3)
											 +(*sampleEmuRout)())];
		syncEm();
	}
	return buffer16bit;
}

void* fill16bitMonoControl( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+(*optr1.outProc)(&optr1)]
			+signedPanMix16[optr2.gainLeft+(*optr2.outProc)(&optr2)]
			+signedPanMix16[optr3.gainLeft+(*optr3.outProc)(&optr3)]
			+signedPanMix16[voice4_gainLeft+(*sampleEmuRout)()];
		syncEm();
	}
	return buffer16bit;
}

void* fill16bitStereo( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		// left
	    *buffer16bit++ = mix16stereo[(unsigned)(mix16stereoMiddleIndex
											 +(*optr1.outProc)(&optr1)
											 +(*optr3.outProc)(&optr3))];
		// right
	    *buffer16bit++ = mix16stereo[(unsigned)(mix16stereoMiddleIndex
											 +(*optr2.outProc)(&optr2)
											 +(*sampleEmuRout)())];
		syncEm();
	}
	return buffer16bit;
}

void* fill16bitStereoControl( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
		// left
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+voice1data]
		    +signedPanMix16[optr2.gainLeft+voice2data]
			+signedPanMix16[optr3.gainLeft+voice3data]
			+signedPanMix16[voice4_gainLeft+voice4data];
		// right
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainRight+voice1data]
			+signedPanMix16[optr2.gainRight+voice2data]
			+signedPanMix16[optr3.gainRight+voice3data]
			+signedPanMix16[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer16bit;
}

void* fill16bitStereoSurround( void* buffer, udword numberOfSamples )
{
	sword* buffer16bit = (sword*)buffer;
	sbyte voice1data, voice2data, voice3data, voice4data;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		voice1data = (*optr1.outProc)(&optr1);
		voice2data = (*optr2.outProc)(&optr2);
		voice3data = (*optr3.outProc)(&optr3);
		voice4data = (*sampleEmuRout)();
		// left
		*buffer16bit++ = zero16bit
			+signedPanMix16[optr1.gainLeft+voice1data]
		    +signedPanMix16[optr2.gainLeft+voice2data]
			+signedPanMix16[optr3.gainLeft+voice3data]
			+signedPanMix16[voice4_gainLeft+voice4data];
		// right
		*buffer16bit++ = zero16bit
			-signedPanMix16[optr1.gainRight+voice1data]
			-signedPanMix16[optr2.gainRight+voice2data]
			-signedPanMix16[optr3.gainRight+voice3data]
			-signedPanMix16[voice4_gainRight+voice4data];
		syncEm();
	}
	return buffer16bit;
}

void* fill16bitSplit( void* buffer, udword numberOfSamples )
{
	sword* v1buffer16bit = (sword*)buffer;
	sword* v2buffer16bit = v1buffer16bit + splitBufferLen;
	sword* v3buffer16bit = v2buffer16bit + splitBufferLen;
	sword* v4buffer16bit = v3buffer16bit + splitBufferLen;
	for ( ; numberOfSamples > 0; numberOfSamples-- )
	{
		*v1buffer16bit++ = zero16bit+((*optr1.outProc)(&optr1)<<8);
		*v2buffer16bit++ = zero16bit+((*optr2.outProc)(&optr2)<<8);
		*v3buffer16bit++ = zero16bit+((*optr3.outProc)(&optr3)<<8);
		*v4buffer16bit++ = zero16bit+((*sampleEmuRout)()<<8);
		syncEm();
	}
	return v1buffer16bit;
}
