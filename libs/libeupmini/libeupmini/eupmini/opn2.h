/*
	Facade for use of different OPN2 emulator implementations.
	
	This is a modified vesion of the respective vgmPlay implementation (see 2612intf.h/.c).
*/

#pragma once

#include "stdtype.h"

typedef UINT32	offs_t;
typedef INT32 stream_sample_t;


void initOPN2();

void writeOPN2Reg(int regBank, int addr, int value);

void ym2612_update_request(void *param);

void ym2612_stream_update( stream_sample_t **outputs, int samples);
int device_start_ym2612( int clock);
void device_stop_ym2612();
void device_reset_ym2612();

UINT8 ym2612_r( offs_t offset);
void ym2612_w( offs_t offset, UINT8 data);

UINT8 ym2612_status_port_a_r(offs_t offset);
UINT8 ym2612_status_port_b_r( offs_t offset);
UINT8 ym2612_data_port_a_r( offs_t offset);
UINT8 ym2612_data_port_b_r(offs_t offset);

void ym2612_control_port_a_w( offs_t offset, UINT8 data);
void ym2612_control_port_b_w( offs_t offset, UINT8 data);
void ym2612_data_port_a_w( offs_t offset, UINT8 data);
void ym2612_data_port_b_w( offs_t offset, UINT8 data);

void ym2612_set_emu_core(UINT8 Emulator);
void ym2612_setoptions(UINT8 Flags);
void ym2612_set_mutemask( UINT32 MuteMask);


