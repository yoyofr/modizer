/*
* This is the interface for using the actual emulator.
* 
* WebSid (c) 2019 Jürgen Wothke
* version 0.94
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_CORE_H
#define WEBSID_CORE_H

#include "base.h"


class Core {
public:
	// load the C64 program data into the emulator (just the binary 
	// without the .sid file header)
	static void loadSongBinary(uint8_t* src, uint16_t dest_addr, uint16_t len, 
								uint8_t basic_mode);

	// then the emulation can be initiated
	static void startupTune(uint32_t sample_rate, uint8_t selected_track, uint8_t is_rsid, uint8_t is_timer_driven_psid, 
							uint8_t is_ntsc, uint8_t is_compatible, uint8_t basic_mode, 
							uint16_t free_space, uint16_t* init_addr, uint16_t load_end_addr, 
							uint16_t play_addr);

	// run the emulator for the duration of one C64 screen refresh 
	// and return the respective audio output
	static uint8_t runOneFrame(uint8_t is_simple_sid_mode, uint8_t speed, int16_t* synth_buffer, 
								int16_t** synth_trace_bufs, uint16_t samples_per_call);
	
	static void callKernalROMReset();
	
#ifdef TEST
	static void rsidRunTest();
#endif
	static void resetTimings(uint32_t sample_rate, uint8_t is_ntsc);

};

#endif
