/**
* Playback handler that uses separate thread for playback.
*
* Due to Linux's RT limitations it is unsuitable for songs that require
* sustained 1 micro timing precision.
*/
#ifndef _FALLBACK_HANDLER_H
#define _FALLBACK_HANDLER_H

#include "playback_handler.h"

class FallbackHandler : public PlaybackHandler {
public:
	FallbackHandler();
	
	~FallbackHandler();
	void recordBegin();
	void recordEnd();
};

#endif