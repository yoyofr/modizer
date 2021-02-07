/*********************************************************

    Irem GA20 PCM Sound Chip

*********************************************************/
#pragma once

#ifndef __IREMGA20_H__
#define __IREMGA20_H__

//#include "devlegcy.h"

void IremGA20_update(UINT8 ChipID, stream_sample_t **outputs, int samples);
int device_start_iremga20(UINT8 ChipID, int clock);
void device_stop_iremga20(UINT8 ChipID);
void device_reset_iremga20(UINT8 ChipID);

//WRITE8_DEVICE_HANDLER( irem_ga20_w );
//READ8_DEVICE_HANDLER( irem_ga20_r );
UINT8 irem_ga20_r(UINT8 ChipID, offs_t offset);
void irem_ga20_w(UINT8 ChipID, offs_t offset, UINT8 data);

void iremga20_write_rom(UINT8 ChipID, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
						const UINT8* ROMData);

void iremga20_set_mute_mask(UINT8 ChipID, UINT32 MuteMask);

//DECLARE_LEGACY_SOUND_DEVICE(IREMGA20, iremga20);

#endif /* __IREMGA20_H__ */
