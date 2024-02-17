/**
* Playback handler that uses the experimental websid device handler for playback.
*
* CAUTION: this unstable experimantal impl may crash the system
*/
#ifndef _DEVICE_DRIVER_HANDLER_H
#define _DEVICE_DRIVER_HANDLER_H

#include "playback_handler.h"

class DeviceDriverHandler : public PlaybackHandler {
public:
	DeviceDriverHandler(int device_fd, volatile uint32_t *buffer_base);
	~DeviceDriverHandler();

	void recordBegin();
	void recordEnd();
	
	static void detectDeviceDriver(int *fd, volatile uint32_t **buffer_base);

private:
	uint32_t fetchBuffer();
	void feedBuffer(uint32_t addr_offset);
	void flushBuffer(uint32_t addr_offset);
	void pushFeedFlag(uint32_t addr_offset);
};


#endif