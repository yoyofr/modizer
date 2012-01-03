//
// /home/ms/files/source/libsidplay/RCS/samples.cpp,v
//

#include "samples.h"

extern ubyte* c64mem1;
extern ubyte* c64mem2;
extern udword PCMfreq;
extern udword C64_clockSpeed;

void sampleEmuTryStopAll();
sbyte sampleEmuSilence();
sbyte sampleEmu();
sbyte sampleEmuTwo();

void GalwayInit();
sbyte GalwayReturnSample(void);
inline void GetNextFour(void);

sbyte (*sampleEmuRout)() = &sampleEmuSilence;

// ---

udword sampleClock;

enum
{
	FM_NONE,
	FM_GALWAYON,
	FM_GALWAYOFF,
	FM_HUELSON,
	FM_HUELSOFF
};

// Sample Order Modes.
const ubyte SO_LOWHIGH = 0;
const ubyte SO_HIGHLOW = 1;

struct sampleChannel
{
	bool Active;
	char Mode;
	ubyte Repeat;
	ubyte Scale;
	ubyte SampleOrder;
	sbyte VolShift;
	
	uword Address;
	uword EndAddr;
	uword RepAddr;
	
	ubyte Counter;  // Galway
    ubyte GalLastVol;
	uword SamLen, GalNextSam;
	uword LoopWait;
	uword NullWait;
	
	uword Period;
#if defined(DIRECT_FIXPOINT) 
	cpuLword Period_stp;
	cpuLword Pos_stp;
	cpuLword PosAdd_stp;
#elif defined(PORTABLE_FIXPOINT)
	uword Period_stp, Period_pnt;
	uword Pos_stp, Pos_pnt;
	uword PosAdd_stp, PosAdd_pnt;
#else
	udword Period_stp;
	udword Pos_stp;
	udword PosAdd_stp;
#endif
};

sampleChannel ch4, ch5;


const sbyte galwayNoiseTab1[16] =
{
	0x80,0x91,0xa2,0xb3,0xc4,0xd5,0xe6,0xf7,
	0x08,0x19,0x2a,0x3b,0x4c,0x5d,0x6e,0x7f
};

ubyte galwayNoiseVolTab[16];
sbyte galwayNoiseSamTab[16];

const sbyte sampleConvertTab[16] =
{
//  0x81,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff,
//  0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x7f
	0x81,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,
	0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70
};


void channelReset(sampleChannel& ch)
{
	ch.Active = false;
	ch.Mode = FM_NONE;
	ch.Period = 0;
#if defined(DIRECT_FIXPOINT)
	ch.Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch.Pos_stp = (ch.Pos_pnt = 0);
#else
	ch.Pos_stp = 0;
#endif
	ch.GalLastVol = 4;
}

inline void channelFree(sampleChannel& ch, const uword regBase)
{
	ch.Active = false;
	ch.Mode = FM_NONE;
	c64mem2[regBase+0x1d] = 0x00;
}

inline void channelTryInit(sampleChannel& ch, const uword regBase)
{
	if ( ch.Active && ( ch.Mode == FM_GALWAYON ))
		return;

	ch.VolShift = ( 0 - (sbyte)c64mem2[regBase+0x1d] ) >> 1;
	c64mem2[regBase+0x1d] = 0x00;
	ch.Address = readLEword(c64mem2+regBase+0x1e);
	ch.EndAddr = readLEword(c64mem2+regBase+0x3d);
	if (ch.EndAddr <= ch.Address)
	{
		return;
	}
	ch.Repeat = c64mem2[regBase+0x3f];
	ch.RepAddr = readLEword(c64mem2+regBase+0x7e);
	ch.SampleOrder = c64mem2[regBase+0x7d];
	uword tempPeriod = readLEword(c64mem2+regBase+0x5d);
	if ( (ch.Scale=c64mem2[regBase+0x5f]) != 0 )
	{
		tempPeriod >>= ch.Scale;
	}
	if ( tempPeriod == 0 )
	{
		ch.Period = 0;
#if defined(DIRECT_FIXPOINT) 
		ch.Pos_stp.l = (ch.PosAdd_stp.l = 0);
#elif defined(PORTABLE_FIXPOINT)
		ch.Pos_stp = (ch.Pos_pnt = 0);
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.Pos_stp = (ch.PosAdd_stp = 0);
#endif
		ch.Active = false;
		ch.Mode = FM_NONE;
	}
	else
	{ 	  
		if ( tempPeriod != ch.Period ) 
		{
			ch.Period = tempPeriod;
#if defined(DIRECT_FIXPOINT) 
			ch.Pos_stp.l = sampleClock / ch.Period;
#elif defined(PORTABLE_FIXPOINT)
			udword tempDiv = sampleClock / ch.Period;
			ch.Pos_stp = tempDiv >> 16;
			ch.Pos_pnt = tempDiv & 0xFFFF;
#else
			ch.Pos_stp = sampleClock / ch.Period;
#endif
		}
#if defined(DIRECT_FIXPOINT) 
		ch.PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.PosAdd_stp = 0;
#endif
		ch.Active = true;
		ch.Mode = FM_HUELSON;
	}
}

inline ubyte channelProcess(sampleChannel& ch, const uword regBase)
{
#if defined(DIRECT_FIXPOINT) 
	uword sampleIndex = ch.PosAdd_stp.w[HI] + ch.Address;
#elif defined(PORTABLE_FIXPOINT)
	uword sampleIndex = ch.PosAdd_stp + ch.Address;
#else
	uword sampleIndex = (ch.PosAdd_stp >> 16) + ch.Address;
#endif
    if ( sampleIndex >= ch.EndAddr )
	{
		if ( ch.Repeat != 0xFF )
		{ 
			if ( ch.Repeat != 0 )  
				ch.Repeat--;
			else
			{
				channelFree(ch,regBase);
				return 8;
			}
		}
		sampleIndex = ( ch.Address = ch.RepAddr );
#if defined(DIRECT_FIXPOINT) 
		ch.PosAdd_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
		ch.PosAdd_stp = (ch.PosAdd_pnt = 0);
#else
		ch.PosAdd_stp = 0;
#endif
		if ( sampleIndex >= ch.EndAddr )
		{
			channelFree(ch,regBase);
			return 8;
		}
	}  
  
	ubyte tempSample = c64mem1[sampleIndex];
	if (ch.SampleOrder == SO_LOWHIGH)
	{
		if (ch.Scale == 0)
		{
#if defined(DIRECT_FIXPOINT) 
			if (ch.PosAdd_stp.w[LO] >= 0x8000)
#elif defined(PORTABLE_FIXPOINT)
			if ( ch.PosAdd_pnt >= 0x8000 )
#else
		    if ( (ch.PosAdd_stp & 0x8000) != 0 )
#endif
			{
			    tempSample >>= 4;
			}
		}
		// AND 15 further below.
	}
	else  // if (ch.SampleOrder == SO_HIGHLOW)
	{
		if (ch.Scale == 0)
		{
#if defined(DIRECT_FIXPOINT) 
			if ( ch.PosAdd_stp.w[LO] < 0x8000 )
#elif defined(PORTABLE_FIXPOINT)
		    if ( ch.PosAdd_pnt < 0x8000 )
#else
		    if ( (ch.PosAdd_stp & 0x8000) == 0 )
#endif
			{
			    tempSample >>= 4;
			}
			// AND 15 further below.
		}
		else  // if (ch.Scale != 0)
		{
			tempSample >>= 4;
		}
	}
	
#if defined(DIRECT_FIXPOINT) 
	ch.PosAdd_stp.l += ch.Pos_stp.l;
#elif defined(PORTABLE_FIXPOINT)
	udword temp = (udword)ch.PosAdd_pnt + (udword)ch.Pos_pnt;
	ch.PosAdd_pnt = temp & 0xffff;
	ch.PosAdd_stp += ( ch.Pos_stp + ( temp > 65535 ));
#else
	ch.PosAdd_stp += ch.Pos_stp;
#endif
	
	return (tempSample&0x0F);
}

// ---

void sampleEmuReset()
{
	channelReset(ch4);
	channelReset(ch5);
	sampleClock = (udword) (((C64_clockSpeed / 2.0) / PCMfreq) * 65536UL);
	sampleEmuRout = &sampleEmuSilence;
	if ( c64mem2 != 0 )
	{
		channelFree(ch4,0xd400);
		channelFree(ch5,0xd500);
	}
}

void sampleEmuInit()
{
	sampleEmuReset();
}

sbyte sampleEmuSilence()
{
	return 0;
}

inline void sampleEmuTryStopAll()
{
	if ( !ch4.Active && !ch5.Active )
	{
		sampleEmuRout = &sampleEmuSilence;
	}
}

void sampleEmuCheckForInit()
{
	// Try first sample channel.
	switch ( c64mem2[0xd41d] ) 
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(ch4,0xd400);
		break;
	 case 0xFD:
		channelFree(ch4,0xd400);
		break;
	 case 0xFC:
		channelTryInit(ch4,0xd400);
		break;
	 case 0x00:
		break;
	 default:
		GalwayInit();
		break;
	}
	
	if (ch4.Mode == FM_HUELSON)
	{
		sampleEmuRout = &sampleEmu;
	}
	
	// Try second sample channel. No ``Galway Noise'' allowed on
	// second channel.
	switch ( c64mem2[0xd51d] ) 
	{
	 case 0xFF:
	 case 0xFE:
		channelTryInit(ch5,0xd500);
		break;
	 case 0xFD:
		channelFree(ch5,0xd500);
		break;
	 case 0xFC:
		channelTryInit(ch5,0xd500);
		break;
	 default:
		break;
	}
	
	if (ch5.Mode == FM_HUELSON)
	{
		sampleEmuRout = &sampleEmuTwo;
	}
	sampleEmuTryStopAll();
}

sbyte sampleEmu()
{
	// One sample channel. Return signed 8-bit sample.
	sbyte sample;
	if ( ch4.Active )
		sample = (sampleConvertTab[channelProcess(ch4,0xd400)]>>ch4.VolShift);
	else
		sample = 0;
	return sample;
}

sbyte sampleEmuTwo()
{
	// At most two sample channels. Return signed 8-bit sample.
	// Note: Implicit two-channel mixing.
	sbyte sample = 0;
	if ( ch4.Active )
		sample += (sampleConvertTab[channelProcess(ch4,0xd400)]>>ch4.VolShift);
	if ( ch5.Active )
		sample += (sampleConvertTab[channelProcess(ch5,0xd500)]>>ch5.VolShift);
	return sample;
}

// --- Galway Noise ---
  
inline void GetNextFour()
{
	uword tempMul = (uword)(c64mem1[ch4.Address+(uword)ch4.Counter])
		* ch4.LoopWait + ch4.NullWait;
	ch4.Counter--;
#if defined(DIRECT_FIXPOINT)
	if ( tempMul != 0 )
		ch4.Period_stp.l = (sampleClock<<1) / tempMul;
	else
		ch4.Period_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	udword tempDiv; 
	if ( tempMul != 0 )
		tempDiv = (sampleClock<<1) / tempMul;
	else
		tempDiv = 0;
	ch4.Period_stp = tempDiv >> 16;
	ch4.Period_pnt = tempDiv & 0xFFFF;
#else
	if ( tempMul != 0 )
		ch4.Period_stp = (sampleClock<<1) / tempMul;
	else
		ch4.Period_stp = 0;
#endif
	ch4.Period = tempMul;
}

void GalwayInit()
{
	if (ch4.Active)  // Don't interrupt active sound.
		return;
	
	sampleEmuRout = &sampleEmuSilence;
  
	ch4.Counter = c64mem2[0xd41d];  
	c64mem2[0xd41d] = 0; 
  
	if ((ch4.Address=readLEword(c64mem2+0xd41e)) == 0)
		return;
  
	if ((ch4.LoopWait=c64mem2[0xd43f]) == 0)
		return;
  
	if ((ch4.NullWait=c64mem2[0xd45d]) == 0)
		return;

	ubyte add;
	if ((add=(c64mem2[0xd43e]&15)) == 0)
		return;
	ubyte vol = ch4.GalLastVol;
	for ( int i = 0; i < 16; i++ )
	{
		vol += add;
		galwayNoiseVolTab[i] = vol&15;
		galwayNoiseSamTab[i] = galwayNoiseTab1[vol&15];
	}
  
	if ((ch4.SamLen=c64mem2[0xd43d]) == 0)
		return;
	ch4.GalNextSam = ch4.SamLen;
	
	ch4.Active = true;
	ch4.Mode = FM_GALWAYON;
	sampleEmuRout = &GalwayReturnSample;
  
#if defined(DIRECT_FIXPOINT)
	ch4.Pos_stp.l = 0;
#elif defined(PORTABLE_FIXPOINT)
	ch4.Pos_stp = 0;
	ch4.Pos_pnt = 0;
#else
	ch4.Pos_stp = 0;
#endif
	
	GetNextFour();
}

sbyte GalwayReturnSample()
{
#if defined(DIRECT_FIXPOINT)
	sbyte tempSample = galwayNoiseSamTab[ch4.Pos_stp.w[HI]&15];
	ch4.GalLastVol = galwayNoiseVolTab[ch4.Pos_stp.w[HI]&15];

	ch4.Pos_stp.l += ch4.Period_stp.l;
	
	if ( ch4.Pos_stp.w[HI] >= ch4.GalNextSam )
	{
#elif defined(PORTABLE_FIXPOINT)
	sbyte tempSample = galwayNoiseSamTab[ch4.Pos_stp&15];
	ch4.GalLastVol = galwayNoiseVolTab[ch4.Pos_stp&15];
		
	udword temp = (udword)ch4.Pos_pnt;
	temp += (udword)ch4.Period_pnt;
	ch4.Pos_pnt = temp & 0xffff;
   	ch4.Pos_stp += ( ch4.Period_stp + ( temp > 65535 ));

	if ( ch4.Pos_stp >= ch4.GalNextSam )
	{
#else
	sbyte tempSample = galwayNoiseSamTab[(ch4.Pos_stp>>16)&15];
	ch4.GalLastVol = galwayNoiseVolTab[(ch4.Pos_stp>>16)&15];

	ch4.Pos_stp += ch4.Period_stp;

	if ( (ch4.Pos_stp>>16) >= ch4.GalNextSam )
	{
#endif
		ch4.GalNextSam += ch4.SamLen;
		if ( ch4.Counter == 0xff )
		{
			ch4.Active = false;
			ch4.Mode = FM_GALWAYOFF;
			sampleEmuRout = &sampleEmuSilence;
			// Check for any new sound that has been set up meanwhile.
			sampleEmuCheckForInit();
		}
		else  
		{
			GetNextFour();
		}
	}
	return tempSample;
}
