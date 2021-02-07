#pragma once

#ifndef __ES5503_H__
#define __ES5503_H__

///#include "devlegcy.h"

/*typedef struct _es5503_interface es5503_interface;
struct _es5503_interface
{
	void (*irq_callback)(running_device *device, int state);
	read8_device_func adc_read;
	UINT8 *wave_memory;
};*/

//READ8_DEVICE_HANDLER( es5503_r );
//WRITE8_DEVICE_HANDLER( es5503_w );
void es5503_w(UINT8 ChipID, offs_t offset, UINT8 data);
UINT8 es5503_r(UINT8 ChipID, offs_t offset);

void es5503_pcm_update(UINT8 ChipID, stream_sample_t **outputs, int samples);
int device_start_es5503(UINT8 ChipID, int clock, int channels);
void device_stop_es5503(UINT8 ChipID);
void device_reset_es5503(UINT8 ChipID);
//void es5503_set_base(running_device *device, UINT8 *wavemem);

void es5503_write_ram(UINT8 ChipID, offs_t DataStart, offs_t DataLength, const UINT8* RAMData);

void es5503_set_mute_mask(UINT8 ChipID, UINT32 MuteMask);
void es5503_set_srchg_cb(UINT8 ChipID, SRATE_CALLBACK CallbackFunc, void* DataPtr);

//DECLARE_LEGACY_SOUND_DEVICE(ES5503, es5503);

#endif /* __ES5503_H__ */
