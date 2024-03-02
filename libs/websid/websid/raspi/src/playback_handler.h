/**
* Abstract base class for all playback handler implementations.
*/
#ifndef _PLAYBACK_HANDLER_H
#define _PLAYBACK_HANDLER_H

#include <stdint.h>

#include "../websid_module/websid_driver_api.h"

class PlaybackHandler {
public:
	PlaybackHandler();
	virtual ~PlaybackHandler() {}
	
	virtual void recordBegin() = 0;
	virtual void recordEnd() = 0;
	
	void recordPokeSID(uint32_t ts, uint8_t reg, uint8_t value);

protected:	
	volatile uint32_t *_script_buf;
	uint16_t _script_len;
};


#endif