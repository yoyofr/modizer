/***************************************************************************

This impl allows to select one of two different emulations:
1) Mame (default)
2) Nuked-OPN2: This is a computationally expensive emulation - which might
   preduce more accurate results than Mame. But my old PC isn't fast enough to
   run it in the browser and I am leaving it here for potential future use.

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>	// for NULL
#include "stdtype.h"


//#define USE_NUKED_OPL2

#ifndef USE_NUKED_OPL2
#include "fmopn.h"			// Mame
#else
#include "../Nuked-OPN2/ym3438.h"			// nuked-OPN2
#endif

#include "opn2.h"

#include "audioout.hpp"


static UINT8 CHIP_SAMPLING_MODE;
static INT32 CHIP_SAMPLE_RATE;
static UINT8 IsVGMInit;



void initOPN2()
{
	// FM Towns: 16Mhz CPU clock and the old impl uses a 8000000 clock

	IsVGMInit= 0;
	CHIP_SAMPLING_MODE = 0x00;
	CHIP_SAMPLE_RATE = streamAudioRate;

	int rate= device_start_ym2612(8000000);		// even the experts don't know.. https://git.redump.net/mame/tree/src/mame/fujitsu/fmtowns.cpp#n2502
	device_reset_ym2612();						// Mame needs this!
}

void writeOPN2Reg(int regBank, int addr, int value)
{
	int bankOffset = regBank &&  (addr >= 0x30) ? 2 : 0;
	ym2612_w(bankOffset+0, addr);
	ym2612_w(bankOffset+1, value);
}

typedef struct _ym2612_state ym2612_state;
struct _ym2612_state
{
	void *			chip;
};


#define MAX_CHIPS	0x01
static ym2612_state YM2612Data[MAX_CHIPS];

static UINT8 ChipFlags = 0x00;


stream_sample_t* DUMMYBUF[0x02] = {NULL, NULL};

/* update request from fm.c */

void ym2612_update_request(void *param)
{
#ifndef USE_NUKED_OPL2
	ym2612_state *info = (ym2612_state *)param;
	ym2612_update_one(info->chip, 0, DUMMYBUF);
#else
#endif
}

/***********************************************************/
/*    YM2612                                               */
/***********************************************************/

void ym2612_stream_update(stream_sample_t **outputs, int samples)
{
	const UINT8 ChipID= 0;
	ym2612_state *info = &YM2612Data[ChipID];
 
#ifndef USE_NUKED_OPL2
	ym2612_update_one(info->chip, samples, outputs);
#else
	OPN2_GenerateStream(info->chip, outputs, samples);
#endif
}


int device_start_ym2612(int clock)
{
	const UINT8 ChipID= 0;

	ym2612_state *info;
	int rate;
	int chiptype;

	chiptype = clock&0x80000000;
	clock&=0x3fffffff;

	info = &YM2612Data[ChipID];
	rate = clock/72;

#ifndef USE_NUKED_OPL2
	if (! (ChipFlags & 0x04))	// if not ("double rate" required)
		rate /= 2;
#else
	rate /= 2;
#endif

	if ((CHIP_SAMPLING_MODE == 0x01 && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;

	/**** initialize YM2612 ****/
#ifndef USE_NUKED_OPL2
	info->chip = ym2612_init(info, clock, rate, NULL, NULL);
#else
	info->chip = malloc(sizeof(ym3438_t));

	OPN2_SetChipType(ym3438_type_discrete);
	OPN2_Reset(info->chip, rate, clock);
#endif

	return rate;
}


void device_stop_ym2612()
{
	const UINT8 ChipID= 0;
	ym2612_state *info = &YM2612Data[ChipID];

#ifndef USE_NUKED_OPL2
	ym2612_shutdown(info->chip);
#else
	free(info->chip);
#endif
}

void device_reset_ym2612()
{
	const UINT8 ChipID= 0;
	ym2612_state *info = &YM2612Data[ChipID];


#ifndef USE_NUKED_OPL2
	ym2612_reset_chip(info->chip);
#else
	OPN2_Reset(info->chip, 0, 0);
#endif
}


UINT8 ym2612_r( offs_t offset)
{
	const UINT8 ChipID= 0;
	ym2612_state *info = &YM2612Data[ChipID];

#ifndef USE_NUKED_OPL2
	return ym2612_read(info->chip, offset & 3);
#else
	return OPN2_Read(info->chip, offset);
#endif
}

void ym2612_w(offs_t offset, UINT8 data)
{
	const UINT8 ChipID= 0;
	ym2612_state *info = &YM2612Data[ChipID];

#ifndef USE_NUKED_OPL2
	ym2612_write(info->chip, offset & 3, data);
#else
	OPN2_WriteBuffered(info->chip, offset, data);
#endif
}

UINT8 ym2612_status_port_a_r(offs_t offset)
{
	return ym2612_r(0);
}
UINT8 ym2612_status_port_b_r( offs_t offset)
{
	return ym2612_r(2);
}
UINT8 ym2612_data_port_a_r( offs_t offset)
{
	return ym2612_r(1);
}
UINT8 ym2612_data_port_b_r( offs_t offset)
{
	return ym2612_r(3);
}

void ym2612_control_port_a_w( offs_t offset, UINT8 data)
{
	ym2612_w(0, data);
}
void ym2612_control_port_b_w( offs_t offset, UINT8 data)
{
	ym2612_w(2, data);
}
void ym2612_data_port_a_w( offs_t offset, UINT8 data)
{
	ym2612_w(1, data);
}
void ym2612_data_port_b_w( offs_t offset, UINT8 data)
{
	ym2612_w(3, data);
}

void ym2612_setoptions(UINT8 Flags)
{
	ChipFlags = Flags;

	const UINT8 ChipID= 0;
	ym2612_state *info = &YM2612Data[ChipID];

#ifndef USE_NUKED_OPL2
	ym2612_set_options(info->chip, Flags);
#else
	OPN2_SetOptions(Flags);
#endif
}

void ym2612_set_mutemask(UINT32 MuteMask)
{
	const UINT8 ChipID= 0;
	ym2612_state *info = &YM2612Data[ChipID];
#ifndef USE_NUKED_OPL2
	ym2612_set_mute_mask(info->chip, MuteMask);
#else
	OPN2_SetMute(info->chip, MuteMask);
#endif
}



