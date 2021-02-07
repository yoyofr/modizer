/****************************************************************

    MAME / MESS functions

****************************************************************/

#include <stdlib.h>	// for free
#include <string.h>	// for memset
#include <stddef.h>	// for NULL
#include "mamedef.h"
//#include "sndintrf.h"
//#include "streams.h"
#include "ay8910.h"		// must be always included (for YM2149_PIN26_LOW)
#include "emu2149.h"
#include "ay_intf.h"


#ifdef ENABLE_ALL_CORES
#define EC_MAME		0x01	// AY8910 core from MAME
#endif
#define EC_EMU2149	0x00	// EMU2149 from NSFPlay


/* for stream system */
typedef struct _ayxx_state ayxx_state;
struct _ayxx_state
{
	void *chip;
};

extern UINT8 CHIP_SAMPLING_MODE;
extern INT32 CHIP_SAMPLE_RATE;
static UINT8 EMU_CORE = 0x00;

extern UINT32 SampleRate;
#define MAX_CHIPS	0x02
static ayxx_state AYxxData[MAX_CHIPS];

void ayxx_stream_update(UINT8 ChipID, stream_sample_t **outputs, int samples)
{
	ayxx_state *info = &AYxxData[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_update_one(info->chip, outputs, samples);
		break;
#endif
	case EC_EMU2149:
		PSG_calc_stereo((PSG*)info->chip, outputs, samples);
		break;
	}
}

int device_start_ayxx(UINT8 ChipID, int clock, UINT8 chip_type, UINT8 Flags)
{
	ayxx_state *info;
	int rate;
	
	if (ChipID >= MAX_CHIPS)
		return 0;
	
	info = &AYxxData[ChipID];
	if (Flags & YM2149_PIN26_LOW)
		rate = clock / 16;
	else
		rate = clock / 8;
	if (((CHIP_SAMPLING_MODE & 0x01) && rate < CHIP_SAMPLE_RATE) ||
		CHIP_SAMPLING_MODE == 0x02)
		rate = CHIP_SAMPLE_RATE;
	
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		rate = ay8910_start(&info->chip, clock, chip_type, Flags);
		break;
#endif
	case EC_EMU2149:
		if (Flags & YM2149_PIN26_LOW)
			clock /= 2;
		info->chip = PSG_new(clock, rate);
		if (info->chip == NULL)
			return 0;
		PSG_setVolumeMode((PSG*)info->chip, (chip_type & 0x10) ? 1 : 2);
		PSG_setFlags((PSG*)info->chip, Flags & ~YM2149_PIN26_LOW);
		break;
	}
 
	return rate;
}

void device_stop_ayxx(UINT8 ChipID)
{
	ayxx_state *info = &AYxxData[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_stop_ym(info->chip);
		break;
#endif
	case EC_EMU2149:
		PSG_delete((PSG*)info->chip);
		break;
	}
	info->chip = NULL;
}

void device_reset_ayxx(UINT8 ChipID)
{
	ayxx_state *info = &AYxxData[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_reset_ym(info->chip);
		break;
#endif
	case EC_EMU2149:
		PSG_reset((PSG*)info->chip);
		break;
	}
}


void ayxx_w(UINT8 ChipID, offs_t offset, UINT8 data)
{
	ayxx_state *info = &AYxxData[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_write_ym(info->chip, offset, data);
		break;
#endif
	case EC_EMU2149:
		PSG_writeIO((PSG*)info->chip, offset, data);
		break;
	}
}

void ayxx_set_emu_core(UINT8 Emulator)
{
#ifdef ENABLE_ALL_CORES
	EMU_CORE = (Emulator < 0x02) ? Emulator : 0x00;
#else
	EMU_CORE = EC_EMU2149;
#endif
	
	return;
}

void ayxx_set_mute_mask(UINT8 ChipID, UINT32 MuteMask)
{
	ayxx_state *info = &AYxxData[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_set_mute_mask_ym(info->chip, MuteMask);
		break;
#endif
	case EC_EMU2149:
		PSG_setMask((PSG*)info->chip, MuteMask);
		break;
	}
	
	return;
}

void ayxx_set_stereo_mask(UINT8 ChipID, UINT32 StereoMask)
{
	ayxx_state *info = &AYxxData[ChipID];
	switch(EMU_CORE)
	{
#ifdef ENABLE_ALL_CORES
	case EC_MAME:
		ay8910_set_stereo_mask_ym(info->chip, StereoMask);
		break;
#endif
	case EC_EMU2149:
		PSG_setStereoMask((PSG*)info->chip, StereoMask);
		break;
	}
	
	return;
}
