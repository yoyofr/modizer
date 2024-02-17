/*
* Utility used to access "historic" SID register state.
*
* WebSid (c) 2023 Jürgen Wothke
* version 0.95
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#ifndef WEBSID_SNAPSHOT_H
#define WEBSID_SNAPSHOT_H

#include "base.h"

namespace websid {

	class SidSnapshot {
	public:
		static void init(uint16_t chunk_size, uint32_t proc_buf_size);
		static void record();

		static uint16_t getRegister(uint8_t sid_idx, uint16_t reg, uint8_t buf_idx, uint32_t tick);
		static uint16_t readVoiceLevel(uint8_t sid_idx, uint8_t voice_idx, uint8_t buf_idx, uint32_t tick);
	};
};

#endif