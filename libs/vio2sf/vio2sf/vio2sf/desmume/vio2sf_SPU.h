/*  vio2sf_SPU.h

	Copyright 2006 Theo Berkau
    Copyright (C) 2006-2009 DeSmuME team

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef vio2sf_SPU_H
#define vio2sf_SPU_H

#include <string>
#include "types.h"
#include <math.h>
#include <assert.h>

#include "resampler.h"

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define FORCEINLINE __inline__ __attribute__((always_inline))
#else
#define FORCEINLINE
#endif

FORCEINLINE u32 u32floor(float f)
{
#ifdef ENABLE_SSE2
	return (u32)_mm_cvtt_ss2si(_mm_set_ss(f));
#else
	return (u32)f;
#endif
}
FORCEINLINE u32 u32floor(double d)
{
#ifdef ENABLE_SSE2
	return (u32)_mm_cvttsd_si32(_mm_set_sd(d));
#else
	return (u32)d;
#endif
}

//same as above but works for negative values too.
//be sure that the results are the same thing as floorf!
FORCEINLINE s32 s32floor(float f)
{
#ifdef ENABLE_SSE2
	return _mm_cvtss_si32( _mm_add_ss(_mm_set_ss(-0.5f),_mm_add_ss(_mm_set_ss(f), _mm_set_ss(f))) ) >> 1;
#else
	return (s32)floorf(f);
#endif
}


static FORCEINLINE u32 sputrunc(float f) { return u32floor(f); }
static FORCEINLINE u32 sputrunc(double d) { return u32floor(d); }
static FORCEINLINE s32 spumuldiv7(s32 val, u8 multiplier) {
	assert(multiplier <= 127);
	return (multiplier == 127) ? val : ((val * multiplier) >> 7);
}

#define SNDCORE_DEFAULT         -1
#define SNDCORE_DUMMY           0

#define CHANSTAT_STOPPED          0
#define CHANSTAT_PLAY             1
#define CHANSTAT_EMPTYBUFFER      2

enum SPUInterpolationMode
{
	SPUInterpolation_None = 0,
    SPUInterpolation_Blep = 1,
	SPUInterpolation_Linear = 2,
    SPUInterpolation_Cubic = 3,
	SPUInterpolation_Sinc = 4
};

typedef struct NDS_state NDS_state;

typedef struct SoundInterface_struct
{
   int id;
   const char *Name;
   int (*Init)(NDS_state *, int buffersize);
   void (*DeInit)(NDS_state *);
   void (*UpdateAudio)(NDS_state *, s16 *buffer, u32 num_samples);
   u32 (*GetAudioSpace)(NDS_state *);
   void (*MuteAudio)(NDS_state *);
   void (*UnMuteAudio)(NDS_state *);
   void (*SetVolume)(NDS_state *, int volume);
} SoundInterface_struct;

extern SoundInterface_struct SNDDummy;
extern SoundInterface_struct SNDFile;

static bool resampler_initialized = false;

struct vio2sf_channel_struct
{
	vio2sf_channel_struct()
	{
		resampler = 0;
	}
	~vio2sf_channel_struct()
	{
		if (resampler)
			resampler_delete(resampler);
	}
	void init_resampler()
	{
		if (!resampler)
		{
			if (!resampler_initialized)
			{
				resampler_init();
				resampler_initialized = true;
			}
			resampler = resampler_create();
		}
	}
	u32 num;
   u8 vol;
   u8 datashift;
   u8 hold;
   u8 pan;
   u8 waveduty;
   u8 repeat;
   u8 format;
   u8 status;
   u32 addr;
   u16 timer;
   u16 loopstart;
   u32 length;
   u32 totlength;
   double double_totlength_shifted;
   union {
		s8 *buf8;
		s16 *buf16;
   };
   double sampcnt;
   double sampinc;
   // ADPCM specific
   u32 lastsampcnt;
   s16 pcm16b, pcm16b_last;
   s16 loop_pcm16b;
   int index;
   int loop_index;
   u16 x;
   s16 psgnoise_last;
   void *resampler;
} ;

struct vio2sf_SPU_struct
{
	vio2sf_SPU_struct(NDS_state *state, int buffersize);
   u32 bufpos;
   u32 buflength;
   s32 *sndbuf;
   s16 *outbuf;
   u32 bufsize;
   NDS_state *state;
   vio2sf_channel_struct channels[16];

   void reset();
   ~vio2sf_SPU_struct();
   void KeyOn(int channel);
   void WriteByte(u32 addr, u8 val);
   void WriteWord(u32 addr, u16 val);
   void WriteLong(u32 addr, u32 val);
   
   //kills all channels but leaves SPU otherwise running normally
   void ShutUp();
};

SoundInterface_struct *SPU_SoundCore(NDS_state *);

void SPU_Pause(NDS_state *, int pause);
void SPU_SetVolume(NDS_state *, int volume);
void SPU_KeyOn(NDS_state *, int channel);
void SPU_Emulate_core(NDS_state *);
void SPU_Emulate_user(NDS_state *, bool mix = true);

class WavWriter
{
public:
	WavWriter();
	bool open(const std::string & fname);
	void close();
	void update(void* soundData, int numSamples);
	bool isRecording() const;
private:
	FILE *spufp;
};


void WAV_End(NDS_state *);
bool WAV_Begin(NDS_state *, const char* fname);
bool WAV_IsRecording(NDS_state *);
void WAV_WavSoundUpdate(NDS_state *, void* soundData, int numSamples);

#endif
