/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com
    Copyright (C) 2006 Theo Berkau
    Copyright (C) 2008-2009 DeSmuME team

    Ideas borrowed from Stephane Dallongeville's SCSP core

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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

//TODO:  MODIZER changes start / YOYOFR
extern "C" {
#include "../../../../../src/ModizerVoicesData.h"
}
//TODO:  MODIZER changes end / YOYOFR


//#undef FORCEINLINE
//#define FORCEINLINE

#define _SPU_CPP_

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932386
#endif

#define K_ADPCM_LOOPING_RECOVERY_INDEX 99999

#include "debug.h"
#include "MMU.h"
#include "vio2sf_SPU.h"
#include "mem.h"
#include "armcpu.h"
#include "NDSSystem.h"
#include "matrix.h"

#include "state.h"

//===================CONFIGURATION========================
bool isChannelMuted(NDS_state *state, int num) { return state->dwChannelMute&(1<<num) ? true : false; }
SPUInterpolationMode spuInterpolationMode(NDS_state *state) { return (SPUInterpolationMode)state->dwInterpolation; }
//=========================================================

//#undef FORCEINLINE
//#define FORCEINLINE

//const int shift = (FORMAT == 0 ? 2 : 1);
static const int format_shift[] = { 2, 1, 3, 0 };

static const s8 indextbl[8] =
{
	-1, -1, -1, -1, 2, 4, 6, 8
};

static const u16 adpcmtbl[89] =
{
	0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0010,
	0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F, 0x0022, 0x0025,
	0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042, 0x0049, 0x0050, 0x0058,
	0x0061, 0x006B, 0x0076, 0x0082, 0x008F, 0x009D, 0x00AD, 0x00BE, 0x00D1,
	0x00E6, 0x00FD, 0x0117, 0x0133, 0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE,
	0x0220, 0x0256, 0x0292, 0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E,
	0x0502, 0x0583, 0x0610, 0x06AB, 0x0756, 0x0812, 0x08E0, 0x09C3, 0x0ABD,
	0x0BD0, 0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954,
	0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B, 0x3BB9,
	0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462, 0x7FFF
};

static const s16 wavedutytbl[8][8] = {
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF }
};

static s32 precalcdifftbl[89][16];
static u8 precalcindextbl[89][8];

static const double ARM7_CLOCK = 33513982;

//////////////////////////////////////////////////////////////////////////////

template<typename T>
static FORCEINLINE T MinMax(T val, T min, T max)
{
	if (val < min)
		return min;
	else if (val > max)
		return max;

	return val;
}

//////////////////////////////////////////////////////////////////////////////

extern "C" int SPU_ChangeSoundCore(NDS_state *state, int coreid, int buffersize)
{
	int i;

	delete state->SPU_user; state->SPU_user = 0;

	// Make sure the old core is freed
	if (state->SNDCore)
		state->SNDCore->DeInit(state);

	// So which core do we want?
	if (coreid == SNDCORE_DEFAULT)
		coreid = 0; // Assume we want the first one

	state->SPU_currentCoreNum = coreid;

	// Go through core list and find the id
	for (i = 0; vio2sf_SNDCoreList[i] != NULL; i++)
	{
		if (vio2sf_SNDCoreList[i]->id == coreid)
		{
			// Set to current core
			state->SNDCore = vio2sf_SNDCoreList[i];
			break;
		}
	}

	//If the user picked the dummy core, disable the user spu
	if(state->SNDCore == &SNDDummy)
		return 0;

	//If the core wasnt found in the list for some reason, disable the user spu
	if (state->SNDCore == NULL)
		return -1;

	// Since it failed, instead of it being fatal, disable the user spu
	if (state->SNDCore->Init(state, buffersize * 2) == -1)
	{
		state->SNDCore = 0;
		return -1;
	}

	//enable the user spu
	//well, not really
	//SPU_user = new vio2sf_SPU_struct(buffersize);

	return 0;
}

SoundInterface_struct *SPU_SoundCore(NDS_state *state)
{
	return state->SNDCore;
}

extern "C" void SPU_Reset(NDS_state *state)
{
	int i;

	state->SPU_core->reset();
	if(state->SPU_user) state->SPU_user->reset();

	if(state->SNDCore && state->SPU_user) {
		state->SNDCore->DeInit(state);
		state->SNDCore->Init(state, state->SPU_user->bufsize*2);
		//todo - check success?
	}

	// Reset Registers
	for (i = 0x400; i < 0x51D; i++)
		T1WriteByte(state->MMU->ARM7_REG, i, 0);
    
	state->samples = 0;
}


//////////////////////////////////////////////////////////////////////////////

//static double cos_lut[256];



extern "C" int SPU_Init(NDS_state *state, int coreid, int buffersize)
{
	int i, j;

	//__asm int 3;

	//for(int i=0;i<256;i++)
	//	cos_lut[i] = cos(i/256.0*M_PI);

	state->SPU_core = new vio2sf_SPU_struct(state, 44100); //pick a really big number just to make sure the plugin doesnt request more
	SPU_Reset(state);

	for(i = 0; i < 16; i++)
	{
		for(j = 0; j < 89; j++)
		{
			precalcdifftbl[j][i] = (((i & 0x7) * 2 + 1) * adpcmtbl[j] / 8);
			if(i & 0x8) precalcdifftbl[j][i] = -precalcdifftbl[j][i];
		}
	}

	for(i = 0; i < 8; i++)
	{
		for(j = 0; j < 89; j++)
		{
			precalcindextbl[j][i] = MinMax((j + indextbl[i]), 0, 88);
		}
	}

	//return SPU_ChangeSoundCore(coreid, buffersize);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SPU_Pause(NDS_state *state, int pause)
{
	if (state->SNDCore == NULL) return;

	if(pause)
		state->SNDCore->MuteAudio(state);
	else
		state->SNDCore->UnMuteAudio(state);
}

//////////////////////////////////////////////////////////////////////////////

void SPU_SetVolume(NDS_state *state, int volume)
{
	if (state->SNDCore)
		state->SNDCore->SetVolume(state, volume);
}

//////////////////////////////////////////////////////////////////////////////



void vio2sf_SPU_struct::reset()
{
	memset(sndbuf,0,bufsize*2*4);
	memset(outbuf,0,bufsize*2*2);

	memset((void *)channels, 0, sizeof(vio2sf_channel_struct) * 16);

	for(int i = 0; i < 16; i++)
	{
		channels[i].num = i;
	}
}

vio2sf_SPU_struct::vio2sf_SPU_struct(struct NDS_state *pstate, int buffersize)
	: state(pstate)
    , bufpos(0)
	, buflength(0)
	, sndbuf(0)
	, outbuf(0)
	, bufsize(buffersize)
{
	sndbuf = new s32[buffersize*2];
	outbuf = new s16[buffersize*2];
	reset();
}

vio2sf_SPU_struct::~vio2sf_SPU_struct()
{
	if(sndbuf) delete[] sndbuf;
	if(outbuf) delete[] outbuf;
}

extern "C" void SPU_DeInit(NDS_state *state)
{
	if(state->SNDCore)
		state->SNDCore->DeInit(state);
	state->SNDCore = 0;

	delete state->SPU_core; state->SPU_core=0;
	delete state->SPU_user; state->SPU_user=0;
}

//////////////////////////////////////////////////////////////////////////////

void vio2sf_SPU_struct::ShutUp()
{
	for(int i=0;i<16;i++)
		 channels[i].status = CHANSTAT_STOPPED;
}

static FORCEINLINE void adjust_channel_timer(vio2sf_channel_struct *chan)
{
	chan->sampinc = (((double)ARM7_CLOCK) / (44100 * 2)) / (double)(0x10000 - chan->timer);
}

void vio2sf_SPU_struct::KeyOn(int channel)
{
	vio2sf_channel_struct &thischan = channels[channel];

    thischan.init_resampler();
    resampler_clear(thischan.resampler);
    resampler_set_quality(thischan.resampler, thischan.format == 3 ? RESAMPLER_QUALITY_BLEP : spuInterpolationMode(state));

	adjust_channel_timer(&thischan);

	//   LOG("Channel %d key on: vol = %d, datashift = %d, hold = %d, pan = %d, waveduty = %d, repeat = %d, format = %d, source address = %07X, timer = %04X, loop start = %04X, length = %06X, cpu->state->MMUARM7_REG[0x501] = %02X\n", channel, chan->vol, chan->datashift, chan->hold, chan->pan, chan->waveduty, chan->repeat, chan->format, chan->addr, chan->timer, chan->loopstart, chan->length, T1ReadByte(MMU->ARM7_REG, 0x501));
	switch(thischan.format)
	{
	case 0: // 8-bit
		thischan.buf8 = (s8*)&state->MMU->MMU_MEM[1][(thischan.addr>>20)&0xFF][(thischan.addr & state->MMU->MMU_MASK[1][(thischan.addr >> 20) & 0xFF])];
	//	thischan.loopstart = thischan.loopstart << 2;
	//	thischan.length = (thischan.length << 2) + thischan.loopstart;
		thischan.sampcnt = 0;
		break;
	case 1: // 16-bit
	
#ifdef EMSCRIPTEN
	// NOTE: IF the respective buffer had not been aligned properly for 16-bit data, then a local fix here would not help.. so just live with the compiler warning..
#endif		
		thischan.buf16 = (s16 *)&state->MMU->MMU_MEM[1][(thischan.addr>>20)&0xFF][(thischan.addr & state->MMU->MMU_MASK[1][(thischan.addr >> 20) & 0xFF])];
	//	thischan.loopstart = thischan.loopstart << 1;
	//	thischan.length = (thischan.length << 1) + thischan.loopstart;
		thischan.sampcnt = 0;
		break;
	case 2: // ADPCM
		{
			thischan.buf8 = (s8*)&state->MMU->MMU_MEM[1][(thischan.addr>>20)&0xFF][(thischan.addr & state->MMU->MMU_MASK[1][(thischan.addr >> 20) & 0xFF])];
			thischan.pcm16b = (s16)((thischan.buf8[1] << 8) | thischan.buf8[0]);
			thischan.pcm16b_last = thischan.pcm16b;
			thischan.index = thischan.buf8[2] & 0x7F;
			thischan.lastsampcnt = 7;
			thischan.sampcnt = 8;
			thischan.loop_index = K_ADPCM_LOOPING_RECOVERY_INDEX;
		//	thischan.loopstart = thischan.loopstart << 3;
		//	thischan.length = (thischan.length << 3) + thischan.loopstart;
			break;
		}
	case 3: // PSG
		{
			thischan.x = 0x7FFF;
			break;
		}
	default: break;
	}

	if(thischan.format != 3)
	{
		if(thischan.double_totlength_shifted == 0)
		{
			thischan.status = CHANSTAT_STOPPED;
		}
	}
	
	thischan.double_totlength_shifted = (double)(thischan.totlength << format_shift[thischan.format]);
}

//////////////////////////////////////////////////////////////////////////////

void vio2sf_SPU_struct::WriteByte(u32 addr, u8 val)
{
	vio2sf_channel_struct &thischan=channels[(addr >> 4) & 0xF];
	switch(addr & 0xF) {
		case 0x0:
			thischan.vol = val & 0x7F;
			break;
		case 0x1: {
			thischan.datashift = val & 0x3;
			if (thischan.datashift == 3)
				thischan.datashift = 4;
			thischan.hold = (val >> 7) & 0x1;
			break;
		}
		case 0x2:
			thischan.pan = val & 0x7F;
			break;
		case 0x3: {
			thischan.waveduty = val & 0x7;
			thischan.repeat = (val >> 3) & 0x3;
			thischan.format = (val >> 5) & 0x3;
			thischan.status = (val >> 7) & 0x1;
			if(thischan.status)
				KeyOn((addr >> 4) & 0xF);
			break;
		}
	}

}

extern "C" void SPU_WriteByte(struct NDS_state *state, u32 addr, u8 val)
{
	addr &= 0xFFF;

	if (addr < 0x500)
	{
		state->SPU_core->WriteByte(addr,val);
		if(state->SPU_user) state->SPU_user->WriteByte(addr,val);
	}

	T1WriteByte(state->MMU->ARM7_REG, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void vio2sf_SPU_struct::WriteWord(u32 addr, u16 val)
{
	vio2sf_channel_struct &thischan=channels[(addr >> 4) & 0xF];
	switch(addr & 0xF)
	{
	case 0x0:
		thischan.vol = val & 0x7F;
		thischan.datashift = (val >> 8) & 0x3;
		if (thischan.datashift == 3)
			thischan.datashift = 4;
		thischan.hold = (val >> 15) & 0x1;
		break;
	case 0x2:
		thischan.pan = val & 0x7F;
		thischan.waveduty = (val >> 8) & 0x7;
		thischan.repeat = (val >> 11) & 0x3;
		thischan.format = (val >> 13) & 0x3;
		thischan.status = (val >> 15) & 0x1;
		if (thischan.status)
			KeyOn((addr >> 4) & 0xF);
		break;
	case 0x8:
		thischan.timer = val & 0xFFFF;
		adjust_channel_timer(&thischan);
		break;
	case 0xA:
		thischan.loopstart = val;
		thischan.totlength = thischan.length + thischan.loopstart;
		thischan.double_totlength_shifted = (double)(thischan.totlength << format_shift[thischan.format]);
		break;
	case 0xC:
		WriteLong(addr,((u32)T1ReadWord(state->MMU->ARM7_REG, addr+2) << 16) | val);
		break;
	case 0xE:
		WriteLong(addr,((u32)T1ReadWord(state->MMU->ARM7_REG, addr-2)) | ((u32)val<<16));
		break;
	}
}

extern "C" void SPU_WriteWord(NDS_state *state, u32 addr, u16 val)
{
	addr &= 0xFFF;

	if (addr < 0x500)
	{
		state->SPU_core->WriteWord(addr,val);
		if(state->SPU_user) state->SPU_user->WriteWord(addr,val);
	}

	T1WriteWord(state->MMU->ARM7_REG, addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void vio2sf_SPU_struct::WriteLong(u32 addr, u32 val)
{
	vio2sf_channel_struct &thischan=channels[(addr >> 4) & 0xF];
	switch(addr & 0xF)
	{
	case 0x0:
		thischan.vol = val & 0x7F;
		thischan.datashift = (val >> 8) & 0x3;
		if (thischan.datashift == 3)
			thischan.datashift = 4;
		thischan.hold = (val >> 15) & 0x1;
		thischan.pan = (val >> 16) & 0x7F;
		thischan.waveduty = (val >> 24) & 0x7;
		thischan.repeat = (val >> 27) & 0x3;
		thischan.format = (val >> 29) & 0x3;
		thischan.status = (val >> 31) & 0x1;
		if (thischan.status)
			KeyOn((addr >> 4) & 0xF);
		break;
	case 0x4:
		thischan.addr = val & 0x7FFFFFF;
		break;
	case 0x8:
		thischan.timer = val & 0xFFFF;
		thischan.loopstart = val >> 16;
		adjust_channel_timer(&thischan);
		break;
	case 0xC:
		thischan.length = val & 0x3FFFFF;
		thischan.totlength = thischan.length + thischan.loopstart;
		thischan.double_totlength_shifted = (double)(thischan.totlength << format_shift[thischan.format]);
		break;
	}
}

extern "C" void SPU_WriteLong(NDS_state *state, u32 addr, u32 val)
{
	addr &= 0xFFF;

	if (addr < 0x500)
	{
		state->SPU_core->WriteLong(addr,val);
		if(state->SPU_user) state->SPU_user->WriteLong(addr,val);
	}

	T1WriteLong(state->MMU->ARM7_REG, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
static FORCEINLINE void Fetch8BitDataInternal(vio2sf_channel_struct *chan, s32 *data)
{
	u32 loc = sputrunc(chan->sampcnt);
	*data = (s32)chan->buf8[loc] << 8;
}

static FORCEINLINE void Fetch16BitDataInternal(const vio2sf_channel_struct * const chan, s32 *data)
{
	const s16* const buf16 = chan->buf16;
	*data = (s32)buf16[sputrunc(chan->sampcnt)];
}

static FORCEINLINE void FetchADPCMDataInternal(vio2sf_channel_struct * const chan, s32 * const data)
{
	// No sense decoding, just return the last sample
	if (chan->lastsampcnt != sputrunc(chan->sampcnt)){

	    const u32 endExclusive = sputrunc(chan->sampcnt+1);
	    for (u32 i = chan->lastsampcnt+1; i < endExclusive; i++)
	    {
	    	const u32 shift = (i&1)<<2;
	    	const u32 data4bit = (((u32)chan->buf8[i >> 1]) >> shift);

	    	const s32 diff = precalcdifftbl[chan->index][data4bit & 0xF];
	    	chan->index = precalcindextbl[chan->index][data4bit & 0x7];

	    	chan->pcm16b_last = chan->pcm16b;
			chan->pcm16b = (s16)(MinMax(chan->pcm16b+diff, -0x8000, 0x7FFF));

			if(i == (chan->loopstart<<3)) {
				chan->loop_pcm16b = chan->pcm16b;
				chan->loop_index = chan->index;
			}
	    }

	    chan->lastsampcnt = sputrunc(chan->sampcnt);
    }

	*data = (s32)chan->pcm16b;
}

static FORCEINLINE void FetchPSGDataInternal(vio2sf_channel_struct *chan, s32 *data)
{
	if(chan->num < 8)
	{
		*data = 0;
	}
	else if(chan->num < 14)
	{
		*data = (s32)wavedutytbl[chan->waveduty][(sputrunc(chan->sampcnt)) & 0x7];
	}
	else
	{
		if(chan->lastsampcnt == sputrunc(chan->sampcnt))
		{
			*data = (s32)chan->psgnoise_last;
			return;
		}

		u32 max = sputrunc(chan->sampcnt);
		for(u32 i = chan->lastsampcnt; i < max; i++)
		{
			if(chan->x & 0x1)
			{
				chan->x = (chan->x >> 1);
				chan->psgnoise_last = -0x7FFF;
			}
			else
			{
				chan->x = ((chan->x >> 1) ^ 0x6000);
				chan->psgnoise_last = 0x7FFF;
			}
		}

		chan->lastsampcnt = sputrunc(chan->sampcnt);

		*data = (s32)chan->psgnoise_last;
	}
}

//////////////////////////////////////////////////////////////////////////////

static FORCEINLINE void MixL(vio2sf_SPU_struct* SPU, vio2sf_channel_struct *chan, s32 data)
{
	data = spumuldiv7(data, chan->vol) >> chan->datashift;
	SPU->sndbuf[SPU->bufpos<<1] += data;
}

static FORCEINLINE void MixR(vio2sf_SPU_struct* SPU, vio2sf_channel_struct *chan, s32 data)
{
	data = spumuldiv7(data, chan->vol) >> chan->datashift;
	SPU->sndbuf[(SPU->bufpos<<1)+1] += data;
}

static FORCEINLINE void MixLR(vio2sf_SPU_struct* SPU, vio2sf_channel_struct *chan, s32 data)
{
	data = spumuldiv7(data, chan->vol) >> chan->datashift;
	SPU->sndbuf[SPU->bufpos<<1] += spumuldiv7(data, 127 - chan->pan);
	SPU->sndbuf[(SPU->bufpos<<1)+1] += spumuldiv7(data, chan->pan);
}

//////////////////////////////////////////////////////////////////////////////

static FORCEINLINE void TestForLoop(NDS_state *state, int FORMAT, vio2sf_SPU_struct *SPU, vio2sf_channel_struct *chan)
{
	const int shift = (FORMAT == 0 ? 2 : 1);

	chan->sampcnt += chan->sampinc;

	if (chan->sampcnt > chan->double_totlength_shifted)
	{
		// Do we loop? Or are we done?
		if (chan->repeat == 1)
		{
			while (chan->sampcnt > chan->double_totlength_shifted)
				chan->sampcnt -= chan->double_totlength_shifted - (double)(chan->loopstart << shift);
			//chan->sampcnt = (double)(chan->loopstart << shift);
		}
		else
		{
			if (!chan->resampler || !resampler_get_sample_count(chan->resampler))
			{
				chan->status = CHANSTAT_STOPPED;

				if(SPU == state->SPU_core)
					state->MMU->ARM7_REG[0x403 + (((chan-SPU->channels) ) * 0x10)] &= 0x7F;
				SPU->bufpos = SPU->buflength;
			}
			else
			{
				chan->status = CHANSTAT_EMPTYBUFFER;
			}
		}
	}
}

static FORCEINLINE void TestForLoop2(NDS_state *state, vio2sf_SPU_struct *SPU, vio2sf_channel_struct *chan)
{
	chan->sampcnt += chan->sampinc;

	if (chan->sampcnt > chan->double_totlength_shifted)
	{
		// Do we loop? Or are we done?
		if (chan->repeat == 1)
		{
			while (chan->sampcnt > chan->double_totlength_shifted)
				chan->sampcnt -= chan->double_totlength_shifted - (double)(chan->loopstart << 3);

			if(chan->loop_index == K_ADPCM_LOOPING_RECOVERY_INDEX)
			{
				chan->pcm16b = (s16)((chan->buf8[1] << 8) | chan->buf8[0]);
				chan->index = chan->buf8[2] & 0x7F;
				chan->lastsampcnt = 7;
			}
			else
			{
				chan->pcm16b = chan->loop_pcm16b;
				chan->index = chan->loop_index;
				chan->lastsampcnt = (chan->loopstart << 3);
			}
		}
		else
		{
			if (!chan->resampler || !resampler_get_sample_count(chan->resampler))
			{
				chan->status = CHANSTAT_STOPPED;
				if(SPU == state->SPU_core)
					state->MMU->ARM7_REG[0x403 + (((chan-SPU->channels) ) * 0x10)] &= 0x7F;
				SPU->bufpos = SPU->buflength;
			}
			else
			{
				chan->status = CHANSTAT_EMPTYBUFFER;
			}
		}
	}
}

static FORCEINLINE void Fetch8BitData(SPUInterpolationMode INTERPOLATE_MODE, NDS_state *state, vio2sf_SPU_struct* const SPU, vio2sf_channel_struct *chan, s32 *data)
{
	double saved_inc = chan->sampinc;
	chan->sampinc = 1.0;

	resampler_set_rate( chan->resampler, saved_inc );

	while (chan->status != CHANSTAT_EMPTYBUFFER && resampler_get_free_count(chan->resampler))
	{
		s32 sample;
		Fetch8BitDataInternal(chan, &sample);
		TestForLoop(state, 0, SPU, chan);
		resampler_write_sample(chan->resampler, sample);
	}

	chan->sampinc = saved_inc;

	if (!resampler_get_sample_count(chan->resampler))
	{
		chan->status = CHANSTAT_STOPPED;
		if(SPU == state->SPU_core)
			state->MMU->ARM7_REG[0x403 + (((chan-SPU->channels) ) * 0x10)] &= 0x7F;
		SPU->bufpos = SPU->buflength;
	}

	*data = resampler_get_sample(chan->resampler);
	resampler_remove_sample(chan->resampler, 1);
}

static FORCEINLINE void Fetch16BitData(SPUInterpolationMode INTERPOLATE_MODE, NDS_state *state, vio2sf_SPU_struct* const SPU, vio2sf_channel_struct *chan, s32 *data)
{
	double saved_inc = chan->sampinc;
	chan->sampinc = 1.0;

	resampler_set_rate( chan->resampler, saved_inc );

	while (chan->status != CHANSTAT_EMPTYBUFFER && resampler_get_free_count(chan->resampler))
	{
		s32 sample;
		Fetch16BitDataInternal(chan, &sample);
		TestForLoop(state, 1, SPU, chan);
		resampler_write_sample(chan->resampler, sample);
	}

	chan->sampinc = saved_inc;

	if (!resampler_get_sample_count(chan->resampler))
	{
		chan->status = CHANSTAT_STOPPED;
		if(SPU == state->SPU_core)
			state->MMU->ARM7_REG[0x403 + (((chan-SPU->channels) ) * 0x10)] &= 0x7F;
		SPU->bufpos = SPU->buflength;
	}

	*data = resampler_get_sample(chan->resampler);
	resampler_remove_sample(chan->resampler, 1);
}

static FORCEINLINE void FetchADPCMData(SPUInterpolationMode INTERPOLATE_MODE, NDS_state *state, vio2sf_SPU_struct* const SPU, vio2sf_channel_struct *chan, s32 *data)
{
	double saved_inc = chan->sampinc;
	chan->sampinc = 1.0;

	resampler_set_rate( chan->resampler, saved_inc );

	while (chan->status != CHANSTAT_EMPTYBUFFER && resampler_get_free_count(chan->resampler))
	{
		s32 sample;
		FetchADPCMDataInternal(chan, &sample);
		TestForLoop2(state, SPU, chan);
		resampler_write_sample(chan->resampler, sample);
	}

	chan->sampinc = saved_inc;

	if (!resampler_get_sample_count(chan->resampler))
	{
		chan->status = CHANSTAT_STOPPED;
		if(SPU == state->SPU_core)
			state->MMU->ARM7_REG[0x403 + (((chan-SPU->channels) ) * 0x10)] &= 0x7F;
		SPU->bufpos = SPU->buflength;
	}

	*data = resampler_get_sample(chan->resampler);
	resampler_remove_sample(chan->resampler, 1);
}

static FORCEINLINE void FetchPSGData(SPUInterpolationMode INTERPOLATE_MODE, vio2sf_channel_struct *chan, s32 *data)
{
	resampler_set_rate( chan->resampler, chan->sampinc );
    
	while (resampler_get_free_count(chan->resampler))
	{
		s32 sample;
		FetchPSGDataInternal(chan, &sample);
		chan->sampcnt += 1.0;
		resampler_write_sample(chan->resampler, sample);
	}

    /* No need to check if resampler is empty since we always fill it completely, 
     * and PSG channels never report terminating on their own.
     */
    
	*data = resampler_get_sample(chan->resampler);
	resampler_remove_sample(chan->resampler, 1);
}

FORCEINLINE static void SPU_Mix(int CHANNELS, vio2sf_SPU_struct* SPU, vio2sf_channel_struct *chan, s32 data)
{
	switch(CHANNELS)
	{
		case 0: MixL(SPU, chan, data); break;
		case 1: MixLR(SPU, chan, data); break;
		case 2: MixR(SPU, chan, data); break;
	}
}

//YOYOFR
static int twosf_getNote(double freq)
{
  const double LOG2_440 = 8.7813597135246596040696824762152;
  const double LOG_2 = 0.69314718055994530941723212145818;
    const int NOTE_440HZ = 60;//0x69;

  if(freq>1.0)
    return (int)((12 * ( log(freq)/LOG_2 - LOG2_440 ) + NOTE_440HZ + 0.5));
  else
    return 0;
}

int twosf_spu_getNote(int ch) {
    if (vgm_last_note[ch]==0) return 0;
    double freq=440.0f*(ARM7_CLOCK / (44100 * 2)) / (0x10000 - vgm_last_note[ch]);
    int note=twosf_getNote(freq);
    return note;
}

int twosf_spu_getInstr(int ch) {
    int idx=0;
    while (vgm_instr_addr[idx]) {
        if (vgm_instr_addr[idx]==vgm_last_sample_addr[ch]) {
            break;
        }
        if (idx==255) {
            //all occupied -> reset
            memset(vgm_instr_addr,0,sizeof(vgm_instr_addr));
            idx=0;
            break;
        }
        idx++;
    }
    vgm_instr_addr[idx]=vgm_last_sample_addr[ch];
    return idx;
}
//YOYOFR


FORCEINLINE static void ____SPU_ChanUpdate(NDS_state *state, int CHANNELS, int FORMAT, SPUInterpolationMode INTERPOLATE_MODE, vio2sf_SPU_struct* const SPU, vio2sf_channel_struct* const chan)
{
	for (; SPU->bufpos < SPU->buflength; SPU->bufpos++)
	{
		if(CHANNELS != -1)
		{
			s32 data;
			switch(FORMAT)
			{
				case 0: 
                    Fetch8BitData(INTERPOLATE_MODE, state, SPU, chan, &data);
                    break;
				case 1: 
                    Fetch16BitData(INTERPOLATE_MODE, state, SPU, chan, &data);
                    break;
				case 2: 
                    FetchADPCMData(INTERPOLATE_MODE, state, SPU, chan, &data);
                    break;
				case 3: 
                    FetchPSGData(INTERPOLATE_MODE, chan, &data);
                    break;
			}
			SPU_Mix(CHANNELS, SPU, chan, data);
            
            //TODO:  MODIZER changes start / YOYOFR
            if (m_voice_current_systemSub>=0) {
                int i=m_voice_current_systemSub;
                
                if (chan->status==CHANSTAT_PLAY) {
                    vgm_last_note[i]=(int)(chan->timer);
                    vgm_last_sample_addr[i]=(int)(chan->addr);
                } else {
                    vgm_last_note[i]=0;
                    vgm_last_sample_addr[i]=0;
                }

                
                m_voice_buff[i][(m_voice_current_ptr[i]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4-1)]=LIMIT8(((spumuldiv7(data, chan->vol) >> chan->datashift)>>8));
                m_voice_current_ptr[i]+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                if ((m_voice_current_ptr[i]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4) m_voice_current_ptr[i]-=(SOUND_BUFFER_SIZE_SAMPLE*4)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
            }
            //TODO:  MODIZER changes end / YOYOFR
		} else {
            //TODO:  MODIZER changes start / YOYOFR
            if (m_voice_current_systemSub>=0) {
                int i=m_voice_current_systemSub;
                
                vgm_last_note[i]=0;
                vgm_last_sample_addr[i]=0;
                
                m_voice_current_ptr[i]+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                if ((m_voice_current_ptr[i]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4) m_voice_current_ptr[i]-=(SOUND_BUFFER_SIZE_SAMPLE*4)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
            }
            //TODO:  MODIZER changes end / YOYOFR
        }
	}
}

FORCEINLINE static void ___SPU_ChanUpdate(NDS_state *state, int FORMAT, SPUInterpolationMode INTERPOLATE_MODE, const bool actuallyMix, vio2sf_SPU_struct* const SPU, vio2sf_channel_struct* const chan)
{
	if(!actuallyMix)
		____SPU_ChanUpdate(state,-1,FORMAT,INTERPOLATE_MODE,SPU,chan);
	else if (chan->pan == 0)
		____SPU_ChanUpdate(state,0,FORMAT,INTERPOLATE_MODE,SPU,chan);
	else if (chan->pan == 127)
		____SPU_ChanUpdate(state,2,FORMAT,INTERPOLATE_MODE,SPU,chan);
	else
		____SPU_ChanUpdate(state,1,FORMAT,INTERPOLATE_MODE,SPU,chan);
}

FORCEINLINE static void __SPU_ChanUpdate(NDS_state *state, SPUInterpolationMode INTERPOLATE_MODE, const bool actuallyMix, vio2sf_SPU_struct* const SPU, vio2sf_channel_struct* const chan)
{
	___SPU_ChanUpdate(state,chan->format,INTERPOLATE_MODE,actuallyMix, SPU, chan);
}

FORCEINLINE static void _SPU_ChanUpdate(NDS_state *state, const bool actuallyMix, vio2sf_SPU_struct* const SPU, vio2sf_channel_struct* const chan)
{
	__SPU_ChanUpdate(state, spuInterpolationMode(state),actuallyMix, SPU, chan);
}


static void SPU_MixAudio(NDS_state *state, bool actuallyMix, vio2sf_SPU_struct *SPU, int length)
{
	u8 vol;

	if(actuallyMix)
	{
		memset(SPU->sndbuf, 0, length*4*2);
		memset(SPU->outbuf, 0, length*2*2);
	}
	
	//---not appropriate for 2sf player
	// If the sound speakers are disabled, don't output audio
	//if(!(T1ReadWord(MMU->ARM7_REG, 0x304) & 0x01))
	//	return;

	// If Master Enable isn't set, don't output audio
	//if (!(T1ReadByte(MMU->ARM7_REG, 0x501) & 0x80))
	//	return;
	//------------------------

	vol = T1ReadByte(state->MMU->ARM7_REG, 0x500) & 0x7F;

	for(int i=0;i<16;i++)
	{
		vio2sf_channel_struct *chan = &SPU->channels[i];
        
        //YOYOFR
        m_voice_current_systemSub=i;
        //YOYOFR
	
        if (chan->status == CHANSTAT_STOPPED) {
            //YOYOFR
            m_voice_current_ptr[i]+=(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)*(int64_t)(length);
            if ((m_voice_current_ptr[i]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4) m_voice_current_ptr[i]-=(SOUND_BUFFER_SIZE_SAMPLE*4)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
            //YOYOFR
			continue;
        }

		SPU->bufpos = 0;
		SPU->buflength = length;

		// Mix audio
		_SPU_ChanUpdate(state, !isChannelMuted(state, i) && actuallyMix, SPU, chan);
	}

	// convert from 32-bit->16-bit
	if(actuallyMix)
		for (int i = 0; i < length*2; i++)
		{
			// Apply Master Volume
			SPU->sndbuf[i] = spumuldiv7(SPU->sndbuf[i], vol);

			if (SPU->sndbuf[i] > 0x7FFF)
				SPU->outbuf[i] = 0x7FFF;
			else if (SPU->sndbuf[i] < -0x8000)
				SPU->outbuf[i] = -0x8000;
			else
				SPU->outbuf[i] = (s16)SPU->sndbuf[i];
		}
}

//////////////////////////////////////////////////////////////////////////////


//emulates one hline of the cpu core.
//this will produce a variable number of samples, calculated to keep a 44100hz output
//in sync with the emulator framerate
static const int dots_per_clock = 6;
static const int dots_per_hline = 355;
static const double time_per_hline = (double)1.0/((double)ARM7_CLOCK/dots_per_clock/dots_per_hline);
static const double samples_per_hline = time_per_hline * 44100;
void SPU_Emulate_core(NDS_state *state)
{
	state->samples += samples_per_hline;
	state->spu_core_samples = (int)(state->samples);
	state->samples -= state->spu_core_samples;
	
	//TODO 
	/*if(driver->AVI_IsRecording() || driver->WAV_IsRecording())
		SPU_MixAudio<true>(SPU_core,spu_core_samples);
	else 
		SPU_MixAudio<false>(SPU_core,spu_core_samples);*/
}

extern "C" void SPU_EmulateSamples(NDS_state *state, int numsamples)
{
	SPU_MixAudio(state,true,state->SPU_core,numsamples);
	state->SNDCore->UpdateAudio(state,state->SPU_core->outbuf,numsamples);
}

extern "C" void SPU_Emulate_user(NDS_state *state, BOOL mix)
{
	if(!state->SPU_user)
		return;

	u32 audiosize;

	// Check to see how much free space there is
	// If there is some, fill up the buffer
	audiosize = state->SNDCore->GetAudioSpace(state);

	if (audiosize > 0)
	{
		//printf("mix %i samples\n", audiosize);
		if (audiosize > state->SPU_user->bufsize)
			audiosize = state->SPU_user->bufsize;
		if (mix) SPU_MixAudio(state,true,state->SPU_user,audiosize);
		state->SNDCore->UpdateAudio(state,state->SPU_user->outbuf, audiosize);
	}
}

//////////////////////////////////////////////////////////////////////////////
// Dummy Sound Interface
//////////////////////////////////////////////////////////////////////////////

int SNDDummyInit(NDS_state *, int buffersize);
void SNDDummyDeInit(NDS_state *);
void SNDDummyUpdateAudio(NDS_state *, s16 *buffer, u32 num_samples);
u32 SNDDummyGetAudioSpace(NDS_state *);
void SNDDummyMuteAudio(NDS_state *);
void SNDDummyUnMuteAudio(NDS_state *);
void SNDDummySetVolume(NDS_state *, int volume);

SoundInterface_struct SNDDummy = {
	SNDCORE_DUMMY,
	"Dummy Sound Interface",
	SNDDummyInit,
	SNDDummyDeInit,
	SNDDummyUpdateAudio,
	SNDDummyGetAudioSpace,
	SNDDummyMuteAudio,
	SNDDummyUnMuteAudio,
	SNDDummySetVolume
};

//////////////////////////////////////////////////////////////////////////////

int SNDDummyInit(NDS_state *, int buffersize)
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyDeInit(NDS_state *)
{
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyUpdateAudio(NDS_state *, s16 *buffer, u32 num_samples)
{
}

//////////////////////////////////////////////////////////////////////////////

u32 SNDDummyGetAudioSpace(NDS_state *)
{
	return 740;
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyMuteAudio(NDS_state *)
{
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummyUnMuteAudio(NDS_state *)
{
}

//////////////////////////////////////////////////////////////////////////////

void SNDDummySetVolume(NDS_state *, int volume)
{
}

//////////////////////////////////////////////////////////////////////////////
//
//typedef struct {
//	char id[4];
//	u32 size;
//} chunk_struct;
//
//typedef struct {
//	chunk_struct riff;
//	char rifftype[4];
//} waveheader_struct;
//
//typedef struct {
//	chunk_struct chunk;
//	u16 compress;
//	u16 numchan;
//	u32 rate;
//	u32 bytespersec;
//	u16 blockalign;
//	u16 bitspersample;
//} fmt_struct;
//
//WavWriter::WavWriter() 
//: spufp(NULL)
//{
//}
//bool WavWriter::open(const std::string & fname)
//{
//	waveheader_struct waveheader;
//	fmt_struct fmt;
//	chunk_struct data;
//	size_t elems_written = 0;
//
//	if ((spufp = fopen(fname.c_str(), "wb")) == NULL)
//		return false;
//
//	// Do wave header
//	memcpy(waveheader.riff.id, "RIFF", 4);
//	waveheader.riff.size = 0; // we'll fix this after the file is closed
//	memcpy(waveheader.rifftype, "WAVE", 4);
//	elems_written += fwrite((void *)&waveheader, 1, sizeof(waveheader_struct), spufp);
//
//	// fmt chunk
//	memcpy(fmt.chunk.id, "fmt ", 4);
//	fmt.chunk.size = 16; // we'll fix this at the end
//	fmt.compress = 1; // PCM
//	fmt.numchan = 2; // Stereo
//	fmt.rate = 44100;
//	fmt.bitspersample = 16;
//	fmt.blockalign = fmt.bitspersample / 8 * fmt.numchan;
//	fmt.bytespersec = fmt.rate * fmt.blockalign;
//	elems_written += fwrite((void *)&fmt, 1, sizeof(fmt_struct), spufp);
//
//	// data chunk
//	memcpy(data.id, "data", 4);
//	data.size = 0; // we'll fix this at the end
//	elems_written += fwrite((void *)&data, 1, sizeof(chunk_struct), spufp);
//
//	return true;
//}
//
//void WavWriter::close()
//{
//	if(!spufp) return;
//	size_t elems_written = 0;
//	long length = ftell(spufp);
//
//	// Let's fix the riff chunk size and the data chunk size
//	fseek(spufp, sizeof(waveheader_struct)-0x8, SEEK_SET);
//	length -= 0x8;
//	elems_written += fwrite((void *)&length, 1, 4, spufp);
//
//	fseek(spufp, sizeof(waveheader_struct)+sizeof(fmt_struct)+0x4, SEEK_SET);
//	length -= sizeof(waveheader_struct)+sizeof(fmt_struct);
//	elems_written += fwrite((void *)&length, 1, 4, spufp);
//	fclose(spufp);
//	spufp = NULL;
//}
//
//void WavWriter::update(void* soundData, int numSamples)
//{
//	if(!spufp) return;
//	//TODO - big endian for the s16 samples??
//	size_t elems_written = fwrite(soundData, numSamples*2, 2, spufp);
//}
//
//bool WavWriter::isRecording() const
//{
//	return spufp != NULL;
//}
//
//
//static WavWriter wavWriter;
//
//void WAV_End()
//{
//	wavWriter.close();
//}
//
//bool WAV_Begin(const char* fname)
//{
//	WAV_End();
//
//	if(!wavWriter.open(fname))
//		return false;
//
//	driver->USR_InfoMessage("WAV recording started.");
//
//	return true;
//}
//
//bool WAV_IsRecording()
//{as
//	return wavWriter.isRecording();
//}
//
//void WAV_WavSoundUpdate(void* soundData, int numSamples)
//{
//	wavWriter.update(soundData, numSamples);
//}
//
