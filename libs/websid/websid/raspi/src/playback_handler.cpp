#include <iostream>             // std::cout

#include "playback_handler.h"
#include <csignal>

using namespace std;


PlaybackHandler::PlaybackHandler() {
	_script_buf = 0;
	_script_len = 0;
}

void PlaybackHandler::recordPokeSID(uint32_t ts, uint8_t reg, uint8_t value) {
	// since the emulation/recording logic runs fast enough and is NOT on the 
	// timing critical path, there is no harm to check for problems here...
	
	if (_script_buf == 0) {
		// let subclass deal with this in recordBegin()
	} else {
		if (_script_len < MAX_ENTRIES) {
			_script_buf[_script_len<<1] = ts;
			_script_buf[(_script_len<<1) + 1] = (((uint32_t)reg)<<8) | ((uint32_t)value);
			
			_script_len++;
		} else {
			cout << "fatal error: recording buffer too small" << endl;
		}
	}
}	

